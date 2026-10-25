#include "HttpClient.h"
#include "../UrlPath.h"

#include <qobject.h>
#include <qnetworkreply.h>
#include <qurlquery.h>
#include <qpointer.h>
#include <filesystem>

using namespace hpw::network::http;

HttpClient::HttpClient(QObject* parent) : QObject(parent) {

}

void HttpClient::send(const HttpRequest& request, const QObject* context, SuccessHandler onSuccess) {

	QString validationError;
	if (!validateRequest(request, validationError)) {
		emit validationErrorOcurred(validationError);
		return;
	}

	QNetworkReply* reply = sendRequest(request);

	const bool hasContext = context != nullptr;
	const QPointer<const QObject> guard(context);

	connect(reply, &QNetworkReply::finished, this, [this, reply, hasContext, guard, onSuccess = std::move(onSuccess)] {
		reply->deleteLater();

		const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		const QByteArray body = reply->readAll();

		if (status == 0) {
			emit networkErrorOcurred(reply->errorString().toUtf8());
			return;
		}

		if (!isSuccess(status)) {
			emit httpErrorOcurred(status, body);
			return;
		}

		emit requestSucceeded(status, body);

		if (onSuccess && (!hasContext || guard))
			onSuccess(status, body);
	});
}

QNetworkReply* HttpClient::sendRequest(const HttpRequest& request) {
	QNetworkRequest networkRequest = buildRequest(request);

	QByteArray body = request.body().isEmpty()
		? QByteArray()
		: QJsonDocument(request.body()).toJson(QJsonDocument::Compact);

	return m_network.sendCustomRequest(networkRequest, request.method().verb().toUtf8(), body);
}

QNetworkRequest HttpClient::buildRequest(const HttpRequest& request) {
	QString urlText = request.baseUrl();

	// Só junta o recurso quando ele existe: uma "/" a mais no fim muda o
	// endereço (o servidor de imagens do Spotify responde 404, por exemplo).
	const QString path = UrlPath(request.resource()).toString();
	if (!path.isEmpty()) {
		while (urlText.endsWith(u'/'))
			urlText.chop(1);
		urlText += u'/' + path;
	}

	QUrl url(urlText);

	if (!request.queryParams().isEmpty()) {
		QUrlQuery query;

		for (const auto [name, value] : request.queryParams().asKeyValueRange())
			query.addQueryItem(name, value);

		url.setQuery(query);
	}

	QNetworkRequest networkRequest = QNetworkRequest(url);
	
	for (const auto [name, value] : request.headers().asKeyValueRange()) {
		networkRequest.setRawHeader(name.toUtf8(), value.toUtf8());
	}

	return networkRequest;
}