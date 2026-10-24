#pragma once

#include "../spotify/SpotifyClient.h"
#include "../network/ImageLoader.h"

#include <qdialog.h>

class QComboBox;
class QDialogButtonBox;
class QListWidget;

namespace hpw::ui {
	class TopItemsDialog : public QDialog {
		Q_OBJECT

	public:
		explicit TopItemsDialog(spotify::SpotifyClient* client, network::ImageLoader* imageLoader, QWidget* parent = nullptr);

	private:
		void setupUi();
		void connectSignals();
		void reload();
		void showTracks(const QList<spotify::Track>& tracks);
		void showArtists(const QList<spotify::Artist>& artists);
		void addTimeRangeOption(const char* title, const spotify::SpotifyClient::TimeRange value);
		void loadIcon(QListWidget* list, int row, const QUrl& url);
		void setLoading(QListWidget* list);
		void setScrollDefaultSettings(QListWidget* list);

		spotify::SpotifyClient* m_client;
		network::ImageLoader* m_imageLoader;
		int m_generation = 0;

		QComboBox* m_rangeBox = nullptr;
		QListWidget* m_tracksList = nullptr;
		QListWidget* m_artistsList = nullptr;
		QDialogButtonBox* m_buttons = nullptr;
	};
}