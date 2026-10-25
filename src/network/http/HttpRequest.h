#pragma once

#include "HttpMethod.h"
#include <qstring.h>
#include <qjsonobject.h>
#include <qhash.h>

namespace hpw::network::http {
	class HttpRequest {
	public:

		// URL base
		HttpRequest& withBaseUrl(QString baseUrl);

		const QString& baseUrl() const;

		// Recurso
		HttpRequest& withResource(QString resource);

		const QString& resource() const;

		// Método HTTP
		HttpRequest& withHttpMethod(HttpMethod method);

		const HttpMethod& method() const;

		// Corpo da requisição
		HttpRequest& withBody(QJsonObject body);

		const QJsonObject& body() const;

		// Tipo do conteúdo
		HttpRequest& withContentType(QString contentType);

		const QString& contentType() const;

		// Autorização
		HttpRequest& withAuthorization(QString authorization);
		HttpRequest& withBearerToken(const QString& token);

		// Headers
		HttpRequest& withHeader(const QString& header, QString value);

		const QHash<QString, QString>& headers() const;

		// Parâmetros de query
		HttpRequest& withQueryParam(const QString& name, QString value);

		const QHash<QString, QString>& queryParams() const;

	private:
		HttpRequest& setBaseUrl(QString baseUrl);
		HttpRequest& setResource(QString resource);
		HttpRequest& setHttpMethod(HttpMethod method);
		HttpRequest& setBody(QJsonObject body);
		HttpRequest& setContentType(QString contentType);
		HttpRequest& addHeader(const QString& header, QString value);
		HttpRequest& addAuthorizationHeader(QString authorization);
		HttpRequest& addQueryParam(const QString& name, QString value);

		QString m_baseUrl;
		QString m_resource;
		HttpMethod m_method = HttpMethod::Get;
		QJsonObject m_body;
		QString m_contentType = QStringLiteral("application/json");
		QHash<QString, QString> m_headers;
		QHash<QString, QString> m_queryParams;
	};

	[[nodiscard]] bool validateRequest(const HttpRequest& request, QString& error);
	[[nodiscard]] bool isSuccess(const int statusCode);
}
