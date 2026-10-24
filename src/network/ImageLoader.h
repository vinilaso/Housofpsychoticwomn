#pragma once

#include <qnetworkaccessmanager.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qpointer.h>
#include <qurl.h>

#include <functional>

class QNetworkReply;

namespace hpw::network {
	class ImageLoader : public QObject {
		Q_OBJECT

	public:
		explicit ImageLoader(QObject* parent = nullptr);

		using LoadedCallback = std::function<void(const QPixmap& pixmap)>;

		void fetch(const QUrl& url, const QObject* context, LoadedCallback onLoaded);
		void load(const QUrl& url);

	signals:
		void loaded(const QUrl& url, const QPixmap& pixmap);
		void failed(const QUrl& url, const QString& error);

	private:
		QNetworkAccessManager m_network;
		QPointer<QNetworkReply> m_pending;
	};
}