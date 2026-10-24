#pragma once

#include <qdatetime.h>
#include <qstring.h>

#include <optional>

namespace hpw::spotify {
	struct Session {
		QString accessToken;
		QString refreshToken;
		QDateTime expiresAt;

		bool hasValidAccessToken() const;
	};

	class TokenStore {
	public:
		void save(const Session& session) const;
		std::optional<Session> load() const;
		void clear() const;
	};
}