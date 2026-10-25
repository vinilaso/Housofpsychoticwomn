#pragma once

#include "HttpRequest.h"

#include <qobject.h>
#include <qnetworkrequest.h>
#include <qnetworkaccessmanager.h>
#include <functional>

namespace hpw::network::http {

	class HttpClient : public QObject {
		Q_OBJECT

	public:
		using SuccessHandler = std::function<void(int status, const QByteArray& body)>;

		explicit HttpClient(QObject* parent = nullptr);

		void send(const HttpRequest& request, const QObject* context = nullptr, SuccessHandler onSuccess = {});

	signals:
		void validationErrorOcurred(const QString& description);
		void networkErrorOcurred(const QByteArray& errorString);
		void httpErrorOcurred(const int status, const QByteArray& response);
		void requestSucceeded(const int status, const QByteArray& response);

	private:
		QNetworkReply* sendRequest(const HttpRequest& request);
		QNetworkRequest buildRequest(const HttpRequest& request);
		QNetworkAccessManager m_network;
	};
}