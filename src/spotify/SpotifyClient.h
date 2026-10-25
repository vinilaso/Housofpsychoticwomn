#pragma once

#include "TokenStore.h"
#include "../network/http/HttpClient.h"

#include <qnetworkaccessmanager.h>
#include <qoauth2authorizationcodeflow.h>
#include <qobject.h>
#include <qstringlist.h>

#include <functional>

class QNetworkRequest;

using namespace hpw::network::http;

namespace hpw::spotify {
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

		explicit SpotifyClient(const QString& clientId, HttpClient* httpClient, QObject* parent = nullptr);

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
		void handleHttpError(int status, const QByteArray& body);
		void refreshSession();
		void sendRequest(HttpRequest& request, HttpClient::SuccessHandler onSuccess);
		void sendRequest(const HttpMethod& method, const QString& resource);
		void connectSignals();

		QOAuth2AuthorizationCodeFlow m_oauth;
		QNetworkAccessManager m_network;
		TokenStore m_tokenStore;
		Session m_session;
		HttpClient* m_httpClient;
		bool m_isRefreshing = false;
	};
}