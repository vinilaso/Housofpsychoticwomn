#include "SpotifyClient.h"
#include "../network/http/HttpClient.h"

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

	QString buildTopItemsUri(SpotifyClient::TopType type) {
		return QStringLiteral("/me/top/%1").arg(getTypeQueryParam(type));
	}
}

SpotifyClient::SpotifyClient(const QString& clientId, HttpClient* httpClient, QObject* parent) 
	: QObject(parent), 
	m_httpClient(httpClient)
{
	Q_ASSERT(m_httpClient);
	configureOAuth(clientId);
	connectSignals();
}

void SpotifyClient::connectSignals()
{
	connect(m_httpClient, &HttpClient::networkErrorOcurred, this, [this](const QByteArray& errorString) {
		emit errorOcurred(Error::Network, QString::fromUtf8(errorString));
	});

	connect(m_httpClient, &HttpClient::httpErrorOcurred, this, [this](const int status, const QByteArray& response) {
		handleHttpError(status, response);
	});
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

void SpotifyClient::sendRequest(HttpRequest& request, HttpClient::SuccessHandler onSuccess)
{
	if (m_isRefreshing || m_session.accessToken.isEmpty())
		return;

	if (!m_session.hasValidAccessToken()) {
		refreshSession();
		return;
	}

	request
		.withBaseUrl(QString::fromLatin1(kApiBaseUrl))
		.withBearerToken(m_session.accessToken);

	m_httpClient->send(request, this, std::move(onSuccess));
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
	HttpRequest request = HttpRequest()
		.withHttpMethod(HttpMethod::Get)
		.withResource("me/player/currently-playing");

	sendRequest(request, [this](int status, const QByteArray& body) {
		if (status == HttpStatus::NoContent) {
			emit nothingPlaying();
			return;
		}

		QJsonObject json = QJsonDocument::fromJson(body).object();
		emit currentTrackChanged(parseCurrentlyPlaying(json));
	});
}

void SpotifyClient::fetchTopArtists(const TimeRange range)
{
	HttpRequest request = HttpRequest()
		.withHttpMethod(HttpMethod::Get)
		.withResource(buildTopItemsUri(TopType::Artists))
		.withQueryParam("time_range", getTimeRangeQueryParam(range));

	sendRequest(request, [this](int statusCode, const QByteArray& body) {
		QJsonObject json = QJsonDocument::fromJson(body).object();
		emit topArtistsReceived(parseArtists(json));
	});
}

void SpotifyClient::fetchTopTracks(const TimeRange range) 
{
	HttpRequest request = HttpRequest()
		.withHttpMethod(HttpMethod::Get)
		.withResource(buildTopItemsUri(TopType::Tracks))
		.withQueryParam("time_range", getTimeRangeQueryParam(range));

	sendRequest(request, [this](int statusCode, const QByteArray& body) {
		QJsonObject json = QJsonDocument::fromJson(body).object();
		emit topTracksReceived(parseTracks(json));
	});
}

void SpotifyClient::play() {
	sendRequest(HttpMethod::Put, "me/player/play");
}

void SpotifyClient::pause() {
	sendRequest(HttpMethod::Put, "me/player/pause");
}

void SpotifyClient::next() {
	sendRequest(HttpMethod::Post, "me/player/next");
}

void SpotifyClient::previous() {
	sendRequest(HttpMethod::Post, "me/player/previous");
}

void SpotifyClient::sendRequest(const HttpMethod& method, const QString& resource)
{
	HttpRequest request = HttpRequest()
		.withHttpMethod(method)
		.withResource(resource);

	sendRequest(request, [this](int, const QByteArray&) {
		emit commandSucceeded();
	});
}