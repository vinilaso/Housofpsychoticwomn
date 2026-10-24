#include "MainWindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtimer.h>

#include <chrono>

using namespace std::chrono_literals;
using namespace hpw::network;
using namespace hpw::ui;
using namespace hpw::spotify;

namespace {
	constexpr auto kPollInterval = 3s;
	constexpr auto kRefreshDelayAfterCommand = 500ms;
	constexpr int kTrackFontSize = 13;
	constexpr QSize kCoverSize{ 120, 120 };
	constexpr QSize kInitialSize{ 420, 220 };
}

MainWindow::MainWindow(SpotifyClient* client, ImageLoader* imageLoader, QWidget* parent)
	: QWidget(parent)
	, m_client(client)
	, m_imageLoader(imageLoader)
	, m_pollTimer(new QTimer(this))
{
	Q_ASSERT(m_client);

	m_pollTimer->setInterval(kPollInterval);

	setupUi();
	connectSignals();
	showLoggedOut();
}

void MainWindow::setupUi()
{
	setWindowTitle(tr("Meu Spotify"));
	resize(kInitialSize);

	m_loginButton = new QPushButton(tr("Entrar com Spotify"), this);
	m_previousButton = new QPushButton(tr("Anterior"), this);
	m_playPauseButton = new QPushButton(tr("Tocar"), this);
	m_nextButton = new QPushButton(tr("Próxima"), this);
	m_topButton = new QPushButton(tr("Mais ouvidos"), this);

	m_trackLabel = new QLabel(QStringLiteral("-"), this);
	m_trackLabel->setWordWrap(true);
	QFont trackFont = m_trackLabel->font();
	trackFont.setPointSize(kTrackFontSize);
	trackFont.setBold(true);
	m_trackLabel->setFont(trackFont);

	m_coverLabel = new QLabel(this);
	m_coverLabel->setFixedSize(kCoverSize);
	m_coverLabel->setAlignment(Qt::AlignCenter);

	m_statusLabel = new QLabel(this);

	auto* controlsLayout = new QHBoxLayout;
	controlsLayout->addWidget(m_previousButton);
	controlsLayout->addWidget(m_playPauseButton);
	controlsLayout->addWidget(m_nextButton);

	auto* infoLayout = new QHBoxLayout;
	infoLayout->addWidget(m_coverLabel);
	infoLayout->addWidget(m_trackLabel, 1);

	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(m_loginButton);
	mainLayout->addLayout(infoLayout);
	mainLayout->addLayout(controlsLayout);
	mainLayout->addWidget(m_topButton);
	mainLayout->addWidget(m_statusLabel);
}

void MainWindow::setControlsEnabled(bool enabled)
{
	m_previousButton->setEnabled(enabled);
	m_playPauseButton->setEnabled(enabled);
	m_nextButton->setEnabled(enabled);
}

void MainWindow::connectSignals()
{
	connect(m_loginButton, &QPushButton::clicked, m_client, &SpotifyClient::login);
	connect(m_previousButton, &QPushButton::clicked, m_client, &SpotifyClient::previous);
	connect(m_nextButton, &QPushButton::clicked, m_client, &SpotifyClient::next);
	connect(m_playPauseButton, &QPushButton::clicked, this, &MainWindow::togglePlayback);
	connect(m_topButton, &QPushButton::clicked, this, &MainWindow::openTopItems);
	connect(m_pollTimer, &QTimer::timeout, m_client, &SpotifyClient::fetchCurrentTrack);

	connect(m_client, &SpotifyClient::authenticated, this, &MainWindow::onAuthenticated);
	connect(m_client, &SpotifyClient::currentTrackChanged, this, &MainWindow::showTrack);
	connect(m_client, &SpotifyClient::nothingPlaying, this, &MainWindow::showNothingPlaying);
	connect(m_client, &SpotifyClient::errorOcurred, this, &MainWindow::showError);

	connect(m_imageLoader, &ImageLoader::loaded, this, &MainWindow::showCover);

	connect(m_client, &SpotifyClient::commandSucceeded, this, [this] {
		QTimer::singleShot(kRefreshDelayAfterCommand, m_client, &SpotifyClient::fetchCurrentTrack);
	});
}

void MainWindow::onAuthenticated()
{
	m_loginButton->hide();
	setControlsEnabled(true);
	m_statusLabel->setText(tr("Conectado"));

	m_client->fetchCurrentTrack();
	m_pollTimer->start();
}

void MainWindow::togglePlayback()
{
	if (m_isPlaying)
		m_client->pause();
	else
		m_client->play();
}

void MainWindow::showTrack(const Track& track)
{
	m_isPlaying = track.isPlaying;

	const QString name = track.name.isEmpty() ? QStringLiteral("-") : track.name;
	m_trackLabel->setText(name + '\n' + track.artists.join(QStringLiteral(", ")));
	m_playPauseButton->setText(m_isPlaying ? tr("Pausar") : tr("Tocar"));
	m_statusLabel->setText(m_isPlaying ? tr("Tocando") : tr("Pausado"));

	if (track.coverUrl != m_currentCoverUrl)
		m_imageLoader->load(track.coverUrl);
}

void MainWindow::showNothingPlaying()
{
	m_isPlaying = false;
	m_trackLabel->setText(tr("Nada tocando agora"));
	m_playPauseButton->setText(tr("Tocar"));
}

void MainWindow::showError(SpotifyClient::Error error, const QString& details)
{
	switch (error) {
	case SpotifyClient::Error::SessionExpired:
		showLoggedOut();
		m_statusLabel->setText(tr("Sessão expirou, entre de novo"));
		break;
	case SpotifyClient::Error::LoginFailed:
		m_statusLabel->setText(tr("Não foi possível realizar o login no Spotify"));
		break;
	case SpotifyClient::Error::PremiumRequired:
		m_statusLabel->setText(tr("Sem permissão, é necessário utilizar uma conta Premium"));
		break;
	case SpotifyClient::Error::NoActiveDevice:
		m_statusLabel->setText(tr("Nenhum dispositivo ativo."));
		break;
	case SpotifyClient::Error::Network:
		m_statusLabel->setText(tr("Falha de conexão: %1").arg(details));
		break;
	case SpotifyClient::Error::Unknown:
		m_statusLabel->setText(tr("Erro %1").arg(details));
		break;
	}
}

void MainWindow::showCover(const QUrl& url, const QPixmap& pixmap)
{
	if (url == m_currentCoverUrl)
		return;

	const qreal ratio = devicePixelRatioF();
	QPixmap scaled = pixmap.scaled(
		m_coverLabel->size() * ratio,
		Qt::KeepAspectRatio,
		Qt::SmoothTransformation
	);

	scaled.setDevicePixelRatio(ratio);
	m_coverLabel->setPixmap(scaled);
}

void MainWindow::showLoggedOut()
{
	m_pollTimer->stop();
	m_loginButton->show();
	setControlsEnabled(false);
	m_statusLabel->setText(tr("Não conectado"));
}

void MainWindow::openTopItems()
{
	auto* dialog = new TopItemsDialog(m_client, m_imageLoader, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->open();
}
