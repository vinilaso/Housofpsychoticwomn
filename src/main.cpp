#include "spotify/SpotifyClient.h"
#include "ui/MainWindow.h"

#include <qapplication.h>
#include <qmessagebox.h>

#include <cstdlib>

using namespace hpw::spotify;
using namespace hpw::network;
using namespace hpw::ui;

namespace {
	constexpr auto kClientIdVariables = "SPOTIFY_CLIENT_ID";
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QApplication::setOrganizationName(QStringLiteral("Housofpsychoticwomn"));
	QApplication::setApplicationName(QStringLiteral("Housofpsychoticwomn"));

	const QString clientId = qEnvironmentVariable(kClientIdVariables);
	if (clientId.isEmpty()) {
		QMessageBox::critical(nullptr, QObject::tr("Configuração ausente"), QObject::tr("Defina a variável de ambiente %1 com o client ID do Spotify").arg(kClientIdVariables));
		return EXIT_FAILURE;
	}

	SpotifyClient client(clientId, new HttpClient());
	ImageLoader imageLoader;
	MainWindow window(&client, &imageLoader);
	window.show();
	client.restoreSession();

	return app.exec();
}