#pragma once

#include "TokenStore.h"

#include <qnetworkaccessmanager.h>
#include <qoauth2authorizationcodeflow.h>
#include <qobject.h>
#include <qstringlist.h>

#include <functional>

class QNetworkRequest;

struct Track {
	QString name;
	QStringList artists;
	QUrl coverUrl;
	bool isPlaying = false;
};

struct Artist {
	QString name;
	QUrl imageUrl;
};

class SpotifyClient : public QObject {
	Q_OBJECT;

public:
	enum class Error {
		SessionExpired,
		PremiumRequired,
		NoActiveDevice,
		LoginFailed,
		Network,
		Unknown
	};
	Q_ENUM(Error);

	enum class TimeRange {
		LastMonth,
		LastSixMonths,
		LastYear
	};
	Q_ENUM(TimeRange);

	enum class TopType {
		Tracks,
		Artists
	};
	Q_ENUM(TopType);

	explicit SpotifyClient(const QString& clientId, QObject* parent = nullptr);

	void login();
	void fetchCurrentTrack();
	void fetchTopArtists(const TimeRange range);
	void fetchTopTracks(const TimeRange range);
	void play();
	void pause();
	void next();
	void previous();
	bool restoreSession();

signals:
	void authenticated();
	void currentTrackChanged(const Track& track);
	void nothingPlaying();
	void commandSucceeded();
	void errorOcurred(SpotifyClient::Error error, const QString& details);
	void topArtistsReceived(const QList<Artist>& artists);
	void topTracksReceived(const QList<Track>& tracks);

private:
	using ResponseHandler = std::function<void(int status, const QByteArray& body)>;

	void onGranted();
	void endSession();
	void configureOAuth(const QString& clientId);
	QNetworkRequest buildRequest(const QString& path) const;
	void sendRequest(const QByteArray& method, const QString& path, ResponseHandler onSuccess);
	void sendCommand(const QByteArray& method, const QString& path);
	void handleHttpError(int status, const QByteArray& body);
	void refreshSession();

	QOAuth2AuthorizationCodeFlow m_oauth;
	QNetworkAccessManager m_network;
	TokenStore m_tokenStore;
	Session m_session;
	bool m_isRefreshing = false;
};