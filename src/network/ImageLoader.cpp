#include "ImageLoader.h"

#include <qnetworkreply.h>
#include <qnetworkrequest.h>

using namespace hpw::network;

ImageLoader::ImageLoader(QObject* parent) : QObject(parent)
{
}

void ImageLoader::load(const QUrl& url)
{
	if (m_pending)
		m_pending->abort();

	QNetworkReply* reply = m_network.get(QNetworkRequest(url));
	m_pending = reply;

	connect(reply, &QNetworkReply::finished, this, [this, reply, url] {
		reply->deleteLater();

		QNetworkReply::NetworkError error = reply->error();

		if (error == QNetworkReply::OperationCanceledError) {
			return;
		}

		if (error != QNetworkReply::NoError) {
			emit failed(url, reply->errorString());
			return;
		}

		QPixmap pixmap;
		if (!pixmap.loadFromData(reply->readAll())) {
			emit failed(url, tr("Formato de imagem não suportado."));
			return;
		}

		emit loaded(url, pixmap);
	});
}

void ImageLoader::fetch(const QUrl& url, const QObject* context, LoadedCallback onLoaded)
{
	QNetworkReply* reply = m_network.get(QNetworkRequest(url));

	connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);

	connect(reply, &QNetworkReply::finished, context, [reply, onLoaded = std::move(onLoaded)] {
		if (reply->error() != QNetworkReply::NoError)
			return;

		QPixmap pixMap;
		if (pixMap.loadFromData(reply->readAll()))
			onLoaded(pixMap);
	});
}