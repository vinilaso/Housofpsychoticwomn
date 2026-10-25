#pragma once

#include <qstring.h>

namespace hpw::network::http {
	class HttpMethod {
	public:
		static const HttpMethod Get;
		static const HttpMethod Post;
		static const HttpMethod Put;
		static const HttpMethod Delete;

		QString verb() const;
		bool requiresBody() const;

	private:
		HttpMethod(QString httpValue, bool requiresBody);
		QString m_verb;
		bool m_requiresBody = false;
	};
}