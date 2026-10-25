#include "HttpMethod.h"

using namespace hpw::network::http;

inline const HttpMethod HttpMethod::Get{
	QStringLiteral("GET"), false
};

inline const HttpMethod HttpMethod::Post{
	QStringLiteral("POST"), false
};

inline const HttpMethod HttpMethod::Put{
	QStringLiteral("PUT"), false
};

inline const HttpMethod HttpMethod::Delete{
	QStringLiteral("DELETE"), false
};

HttpMethod::HttpMethod(QString httpValue, bool requiresBody) : m_verb(httpValue), m_requiresBody(requiresBody) {
}

QString HttpMethod::verb() const {
	return m_verb;
}

bool HttpMethod::requiresBody() const {
	return m_requiresBody;
}