#include "TopItemsDialog.h"
#include "widgets/SmoothWheelScroll.h"

#include <qcombobox.h>
#include <qlistwidget.h>
#include <qtabwidget.h>
#include <qdialogbuttonbox.h>
#include <qlabel.h>
#include <qicon.h>
#include <qpixmap.h>

#include <QHBoxLayout>
#include <QVBoxLayout>

using namespace hpw::ui;
using namespace hpw::network;
using namespace hpw::spotify;
using hpw::ui::widgets::SmoothWheelScroll;

namespace {
	constexpr QSize kInitialSize{ 420, 480 };
	constexpr QSize kIconSize{ 48, 47 };
}

TopItemsDialog::TopItemsDialog(SpotifyClient* client, ImageLoader* imageLoader, QWidget* parent)
	: QDialog(parent)
	, m_client(client)
	, m_imageLoader(imageLoader)
{
	Q_ASSERT(m_client);

	setupUi();
	connectSignals();
	reload();
}

void TopItemsDialog::setupUi()
{
	setWindowTitle(tr("Mais ouvidos"));
	resize(kInitialSize);

	m_rangeBox = new QComboBox(this);
	addTimeRangeOption("Último mês", SpotifyClient::TimeRange::LastMonth);
	addTimeRangeOption("Últimos 6 meses", SpotifyClient::TimeRange::LastSixMonths);
	addTimeRangeOption("Último ano", SpotifyClient::TimeRange::LastYear);
	m_rangeBox->setCurrentIndex(1);

	m_tracksList = new QListWidget(this);
	m_artistsList = new QListWidget(this);

	setScrollDefaultSettings(m_tracksList);
	setScrollDefaultSettings(m_artistsList);

	auto* tabs = new QTabWidget(this);
	tabs->addTab(m_tracksList, tr("Músicas"));
	tabs->addTab(m_artistsList, tr("Artistas"));

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);

	auto* rangeLayout = new QHBoxLayout;
	rangeLayout->addWidget(new QLabel(tr("Período:"), this));
	rangeLayout->addWidget(m_rangeBox, 1);

	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(rangeLayout);
	mainLayout->addWidget(tabs);
	mainLayout->addWidget(m_buttons);
}

void TopItemsDialog::addTimeRangeOption(const char* title, const SpotifyClient::TimeRange value)
{
	m_rangeBox->addItem(tr(title), QVariant::fromValue(value));
}

void TopItemsDialog::connectSignals()
{
	connect(m_rangeBox, &QComboBox::currentIndexChanged, this, &TopItemsDialog::reload);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(m_client, &SpotifyClient::topTracksReceived, this, &TopItemsDialog::showTracks);
	connect(m_client, &SpotifyClient::topArtistsReceived, this, &TopItemsDialog::showArtists);
}

void TopItemsDialog::reload()
{
	++m_generation;
	const auto range = m_rangeBox->currentData().value<SpotifyClient::TimeRange>();

	setLoading(m_tracksList);
	setLoading(m_artistsList);

	m_client->fetchTopTracks(range);
	m_client->fetchTopArtists(range);
}

void TopItemsDialog::setLoading(QListWidget* list)
{
	list->clear();
	list->addItem(tr("Carregando..."));
}

void TopItemsDialog::loadIcon(QListWidget* list, int row, const QUrl& url)
{
	if (url.isEmpty())
		return;

	const int generation = m_generation;
	m_imageLoader->fetch(url, this, [this, list, row, generation](const QPixmap& pixmap) {
		if (generation != m_generation)
			return;

		if (QListWidgetItem* item = list->item(row))
			item->setIcon(QIcon(pixmap));
	});
}

void TopItemsDialog::showTracks(const QList<Track>& tracks)
{
	m_tracksList->clear();

	if (tracks.isEmpty()) {
		m_tracksList->addItem(tr("Nenhuma faixa encontrada"));
		return;
	}

	QPixmap placeholder(kIconSize);
	placeholder.fill(Qt::transparent);

	for (int row = 0; row < tracks.size(); ++row) {
		const Track& track = tracks[row];
		auto* item = new QListWidgetItem(QStringLiteral("%1. %2 - %3").arg(row + 1).arg(track.name, track.artists.join(QStringLiteral(", "))), m_tracksList);

		item->setIcon(placeholder);
		loadIcon(m_tracksList, row, track.coverUrl);
	}
}

void TopItemsDialog::showArtists(const QList<Artist>& artists)
{
	m_artistsList->clear();

	if (artists.isEmpty()) {
		m_artistsList->addItem(tr("Nenhum artista encontrado"));
		return;
	}

	QPixmap placeholder(kIconSize);
	placeholder.fill(Qt::transparent);

	for (int row = 0; row < artists.size(); ++row) {
		const Artist& artist = artists[row];
		auto* item = new QListWidgetItem(QStringLiteral("%1. %2").arg(row + 1).arg(artist.name), m_artistsList);
		item->setIcon(QIcon(placeholder));
		loadIcon(m_artistsList, row, artist.imageUrl);
	}
}

void TopItemsDialog::setScrollDefaultSettings(QListWidget* list)
{
	list->setIconSize(kIconSize);
	list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	new SmoothWheelScroll(list);
}