#pragma once

#include "../spotify/SpotifyClient.h"
#include "../network/ImageLoader.h"

#include <qdialog.h>

class QComboBox;
class QDialogButtonBox;
class QListWidget;

class TopItemsDialog : public QDialog {
	Q_OBJECT

public:
	explicit TopItemsDialog(SpotifyClient* client, ImageLoader* imageLoader, QWidget* parent = nullptr);

private:
	void setupUi();
	void connectSignals();
	void reload();
	void showTracks(const QList<Track>& tracks);
	void showArtists(const QList<Artist>& artists);
	void addTimeRangeOption(const char* title, const SpotifyClient::TimeRange value);
	void loadIcon(QListWidget* list, int row, const QUrl& url);
	void setLoading(QListWidget* list);
	void setScrollDefaultSettings(QListWidget* list);

	SpotifyClient* m_client;
	ImageLoader* m_imageLoader;
	int m_generation;
	
	QComboBox* m_rangeBox = nullptr;
	QListWidget* m_tracksList = nullptr;
	QListWidget* m_artistsList = nullptr;
	QDialogButtonBox* m_buttons = nullptr;
};