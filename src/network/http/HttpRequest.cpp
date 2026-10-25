#include "HttpRequest.h"

#include <qstring.h>
#include <qjsonobject.h>
#include <qhash.h>

using namespace hpw::network::http;

// URL base
HttpRequest& HttpRequest::setBaseUrl(QString baseUrl) {
	m_baseUrl = std::move(baseUrl);
	return *this;
}

HttpRequest& HttpRequest::withBaseUrl(QString baseUrl) {
	return setBaseUrl(std::move(baseUrl));
}

const QString& HttpRequest::baseUrl() const {
	return m_baseUrl;
}

// Recurso
HttpRequest& HttpRequest::setResource(QString resource) {
	m_resource = std::move(resource);
	return *this;
}

HttpRequest& HttpRequest::withResource(QString resource) {
	return setResource(std::move(resource));
}

const QString& HttpRequest::resource() const {
	return m_resource;
}

// Método HTTP
HttpRequest& HttpRequest::setHttpMethod(HttpMethod method) {
	m_method = std::move(method);
	return *this;
}

HttpRequest& HttpRequest::withHttpMethod(HttpMethod method) {
	return setHttpMethod(std::move(method));
}

const HttpMethod& HttpRequest::method() const {
	return m_method;
}

// Corpo da requisição
HttpRequest& HttpRequest::setBody(QJsonObject body) {
	m_body = std::move(body);
	return *this;
}

HttpRequest& HttpRequest::withBody(QJsonObject body) {
	return setBody(std::move(body));
}

const QJsonObject& HttpRequest::body() const {
	return m_body;
}

// Tipo do conteúdo
HttpRequest& HttpRequest::setContentType(QString contentType) {
	m_contentType = std::move(contentType);
	return *this;
}

HttpRequest& HttpRequest::withContentType(QString contentType) {
	return setContentType(std::move(contentType));
}

const QString& HttpRequest::contentType() const {
	return m_contentType;
}

// Headers
HttpRequest& HttpRequest::addHeader(const QString& header, QString value) {
	m_headers.insert(header, std::move(value));
	return *this;
}

HttpRequest& HttpRequest::withHeader(const QString& header, QString value) {
	return addHeader(header, std::move(value));
}

const QHash<QString, QString>& HttpRequest::headers() const {
	return m_headers;
}

// Autorização
HttpRequest& HttpRequest::addAuthorizationHeader(QString authorization) {
	return addHeader(QStringLiteral("Authorization"), std::move(authorization));
}

HttpRequest& HttpRequest::withAuthorization(QString authorization) {
	return addAuthorizationHeader(std::move(authorization));
}

HttpRequest& HttpRequest::withBearerToken(const QString& token) {
	return addAuthorizationHeader(QStringLiteral("Bearer ") + token);
}

// Parâmetros de query
HttpRequest& HttpRequest::addQueryParam(const QString& name, QString value) {
	m_queryParams.insert(name, std::move(value));
	return *this;
}

HttpRequest& HttpRequest::withQueryParam(const QString& name, QString value) {
	return addQueryParam(name, std::move(value));
}

const QHash<QString, QString>& HttpRequest::queryParams() const {
	return m_queryParams;
}

bool hpw::network::http::validateRequest(const HttpRequest& request, QString& error) {
	if (request.baseUrl().isEmpty()) {
		error = QStringLiteral("A URL base não foi definida.");
		return false;
	}

	if (request.contentType().isEmpty()) {
		error = QStringLiteral("O tipo do conteúdo não foi definido.");
		return false;
	}

	if (request.method().requiresBody() && request.body().isEmpty()) {
		error = QStringLiteral("O corpo da requisição não foi definido.");
		return false;
	}

	return true;
}

bool hpw::network::http::isSuccess(const int statusCode) {
	return statusCode >= 200 && statusCode < 300;
}
