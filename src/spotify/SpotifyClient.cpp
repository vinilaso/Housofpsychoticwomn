#include "SpotifyClient.h"

#include <qdebug.h>
#include <qdesktopservices.h>
#include <qoauthhttpserverreplyhandler.h>
#include <qurl.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>

using namespace hpw::spotify;

namespace {
	constexpr quint16 kRedirectPort = 8888;

	constexpr auto kAuthorizationUrl = "https://accounts.spotify.com/authorize";
	constexpr auto kTokenUrl = "https://accounts.spotify.com/api/token";
	constexpr auto kApiBaseUrl = "https://api.spotify.com/v1";

	constexpr auto kScopes = "user-read-currently-playing user-read-playback-state user-modify-playback-state user-top-read";

	namespace HttpStatus {
		constexpr int NoContent = 204;
		constexpr int BadRequest = 400;
		constexpr int Unauthorized = 401;
		constexpr int Forbidden = 403;
		constexpr int NotFound = 404;
	}

	Track parseTrackObject(const QJsonObject& item)
	{
		Track track;
		track.name = item.value("name").toString();

		const QJsonArray artists = item.value("artists").toArray();
		for (const QJsonValue& artist : artists)
			track.artists << artist.toObject().value("name").toString();

		const QJsonArray images = item.value("album").toObject().value("images").toArray();
		if (!images.isEmpty())
			track.coverUrl = images.first().toObject().value("url").toString();

		return track;
	}

	Track parseCurrentlyPlaying(const QJsonObject& json)
	{
		Track track = parseTrackObject(json.value("item").toObject());
		track.isPlaying = json.value("is_playing").toBool();
		return track;
	}

	QList<Track> parseTracks(const QJsonObject& json)
	{
		QList<Track> tracks;
		const QJsonArray items = json.value("items").toArray();

		for (const QJsonValue& track : items)
			tracks << parseTrackObject(track.toObject());

		return tracks;
	}

	QList<Artist> parseArtists(const QJsonObject& json)
	{
		QList<Artist> artists;

		const QJsonArray items = json.value("items").toArray();
		for (const QJsonValue& item : items) {
			const QJsonObject itemObject = item.toObject();

			Artist artist;
			artist.name = itemObject.value("name").toString();

			const QJsonArray images = itemObject.value("images").toArray();
			if (!images.isEmpty()) {
				artist.imageUrl = images.first().toObject().value("url").toString();
			}

			artists << artist;
		}

		return artists;
	}

	QString getTypeQueryParam(SpotifyClient::TopType type)
	{
		return type == SpotifyClient::TopType::Tracks
			? "tracks"
			: "artists";
	}

	QString getTimeRangeQueryParam(SpotifyClient::TimeRange range)
	{
		switch (range) {
		case SpotifyClient::TimeRange::LastMonth:
			return "short_term";
		case SpotifyClient::TimeRange::LastSixMonths:
			return "medium_term";
		case SpotifyClient::TimeRange::LastYear:
			return "long_term";
		}

		return "medium_term";
	}

	QString buildTopItemsUri(SpotifyClient::TimeRange range, SpotifyClient::TopType type) 
	{
		return QStringLiteral("/me/top/%1?time_range=%2")
			.arg(getTypeQueryParam(type))
			.arg(getTimeRangeQueryParam(range));
	}
}

SpotifyClient::SpotifyClient(const QString& clientId, QObject* parent) : QObject(parent)
{
	configureOAuth(clientId);
}

void SpotifyClient::configureOAuth(const QString& clientId)
{
	m_oauth.setAuthorizationUrl(QUrl(kAuthorizationUrl));
	m_oauth.setClientIdentifier(clientId);

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
	m_oauth.setTokenUrl(QUrl(kTokenUrl));
	QSet<QByteArray> scopes;
	for (const QByteArray& scope : QByteArray(kScopes).split(' '))
		scopes.insert(scope);
	m_oauth.setRequestedScopeTokens(scopes);
#else
	m_oauth.setAccessTokenUrl(QUrl(kTokenUrl));
	m_oauth.setScope(kScopes);
#endif

	auto* replyHandler = new QOAuthHttpServerReplyHandler(kRedirectPort, this);
	m_oauth.setReplyHandler(replyHandler);
	qDebug() << "Redirect URI usada:" << replyHandler->callback();

	connect(&m_oauth, &QAbstractOAuth::authorizeWithBrowser, &QDesktopServices::openUrl);

	connect(&m_oauth, &QAbstractOAuth::granted, this, &SpotifyClient::onGranted);

	connect(&m_oauth, &QAbstractOAuth::requestFailed, this, [this](QAbstractOAuth::Error error) {
		if (!m_isRefreshing) {
			emit errorOcurred(Error::LoginFailed, {});
			return;
		}

		if (error == QAbstractOAuth::Error::NetworkError) {
			m_isRefreshing = false;
			emit errorOcurred(Error::Network, {});
			return;
		}

		endSession();
	});
}

void SpotifyClient::login()
{
	m_oauth.grant();
}

bool SpotifyClient::restoreSession()
{
	const std::optional<Session> saved = m_tokenStore.load();

	if (!saved)
		return false;

	m_session = *saved;

	if (m_session.hasValidAccessToken()) {
		emit authenticated();
		return true;
	}

	if (!m_session.refreshToken.isEmpty()) {
		refreshSession();
		return true;
	}

	m_tokenStore.clear();
	return false;
}

void SpotifyClient::onGranted() 
{
	m_isRefreshing = false;

	const QString newRefreshToken = m_oauth.refreshToken();

	if (!newRefreshToken.isEmpty())
		m_session.refreshToken = newRefreshToken;

	m_session.accessToken = m_oauth.token();
	m_session.expiresAt = m_oauth.expirationAt().toUTC();
	m_tokenStore.save(m_session);

	emit authenticated();
}

void SpotifyClient::endSession()
{
	m_isRefreshing = false;
	m_session = {};
	m_tokenStore.clear();
	emit errorOcurred(Error::SessionExpired, {});
}

QNetworkRequest SpotifyClient::buildRequest(const QString& path) const
{
	QNetworkRequest request(QUrl(QString::fromLatin1(kApiBaseUrl) + path));
	request.setRawHeader("Authorization", "Bearer " + m_session.accessToken.toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	return request;
}

void SpotifyClient::sendRequest(const QByteArray& method, const QString& path, ResponseHandler onSuccess)
{
	if (m_isRefreshing || m_session.accessToken.isEmpty())
		return;

	if (!m_session.hasValidAccessToken()) {
		refreshSession();
		return;
	}

	QNetworkReply* reply = m_network.sendCustomRequest(buildRequest(path), method, QByteArray());

	connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess] {
		reply->deleteLater();

		const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		const QByteArray body = reply->readAll();

		if (status == 0) {
			emit errorOcurred(Error::Network, reply->errorString());
			return;
		}

		if (status >= HttpStatus::BadRequest) {
			handleHttpError(status, body);
			return;
		}

		if (onSuccess)
			onSuccess(status, body);
	});
}

void SpotifyClient::handleHttpError(int status, const QByteArray& body)
{
	switch (status) {
	case HttpStatus::Unauthorized:
		refreshSession();
		break;
	case HttpStatus::Forbidden:
		emit errorOcurred(Error::PremiumRequired, QString::fromUtf8(body));
		break;
	case HttpStatus::NotFound:
		emit errorOcurred(Error::NoActiveDevice, QString::fromUtf8(body));
		break;
	default:
		qWarning() << "Erro HTTP" << status << body;
		emit errorOcurred(Error::Unknown, QString::number(status));
		break;
	}
}

void SpotifyClient::refreshSession()
{
	if (m_isRefreshing)
		return;

	if (m_session.refreshToken.isEmpty()) {
		endSession();
		return;
	}

	m_isRefreshing = true;
	m_oauth.setRefreshToken(m_session.refreshToken);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
	m_oauth.refreshTokens();
#else
	m_oauth.refreshAccessToken();
#endif
}

void SpotifyClient::fetchCurrentTrack()
{
	sendRequest("GET", "/me/player/currently-playing", [this](int status, const QByteArray& body) {
		if (status == HttpStatus::NoContent) {
			emit nothingPlaying();
			return;
		}

		emit currentTrackChanged(parseCurrentlyPlaying(QJsonDocument::fromJson(body).object()));
	});
}

void SpotifyClient::fetchTopArtists(const TimeRange range)
{
	sendRequest("GET", buildTopItemsUri(range, SpotifyClient::TopType::Artists), [this](int, const QByteArray& body) {
		emit topArtistsReceived(parseArtists(QJsonDocument::fromJson(body).object()));
	});
}

void SpotifyClient::fetchTopTracks(const TimeRange range) 
{
	sendRequest("GET", buildTopItemsUri(range, SpotifyClient::TopType::Tracks), [this](int, const QByteArray& body) {
		emit topTracksReceived(parseTracks(QJsonDocument::fromJson(body).object()));
	});
}

void SpotifyClient::play()
{
	sendCommand("PUT", "/me/player/play");
}

void SpotifyClient::pause()
{
	sendCommand("PUT", "/me/player/pause");
}

void SpotifyClient::next()
{
	sendCommand("POST", "/me/player/next");
}

void SpotifyClient::previous()
{
	sendCommand("POST", "/me/player/previous");
}

void SpotifyClient::sendCommand(const QByteArray& method, const QString& path)
{
	sendRequest(method, path, [this](int, const QByteArray&) {
		emit commandSucceeded();
	});
}