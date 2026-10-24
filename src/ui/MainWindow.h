#pragma once

#include "../spotify/SpotifyClient.h"
#include "../network/ImageLoader.h"	
#include "../ui/TopItemsDialog.h"

#include <qwidget.h>

class QLabel;
class QPushButton;
class QTimer;

namespace hpw::ui {
	class MainWindow : public QWidget {
		Q_OBJECT

	public:
		explicit MainWindow(spotify::SpotifyClient* client, network::ImageLoader* imageLoader, QWidget* parent = nullptr);

	private:
		void setupUi();
		void connectSignals();
		void setControlsEnabled(bool enabled);

		void onAuthenticated();
		void togglePlayback();
		void showTrack(const spotify::Track& track);
		void showNothingPlaying();
		void showError(spotify::SpotifyClient::Error error, const QString& details);
		void showLoggedOut();
		void showCover(const QUrl& url, const QPixmap& pixmap);
		void openTopItems();

		spotify::SpotifyClient* m_client;
		network::ImageLoader* m_imageLoader;
		QTimer* m_pollTimer;

		QPushButton* m_loginButton = nullptr;
		QPushButton* m_previousButton = nullptr;
		QPushButton* m_playPauseButton = nullptr;
		QPushButton* m_nextButton = nullptr;
		QPushButton* m_topButton = nullptr;

		QLabel* m_trackLabel = nullptr;
		QLabel* m_statusLabel = nullptr;
		QLabel* m_coverLabel = nullptr;

		bool m_isPlaying = false;
		QUrl m_currentCoverUrl;
	};
}