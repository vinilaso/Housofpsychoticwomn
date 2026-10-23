#include "TokenStore.h"

#ifndef NOMINMAX
#	define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <dpapi.h>
#include <qdebug.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qsettings.h>

namespace {
	constexpr auto kSettingsKey = "spotify/session";
	constexpr qint64 kExpiryMarginSeconds = 60;

	DATA_BLOB toBlob(const QByteArray& data)
	{
		return {
			static_cast<DWORD>(data.size()),
			reinterpret_cast<BYTE*>(const_cast<char*>(data.data()))
		};
	}

	QByteArray takeBlob(DATA_BLOB& blob)
	{
		QByteArray result(
			reinterpret_cast<const char*>(blob.pbData),
			static_cast<qsizetype>(blob.cbData)
		);

		LocalFree(blob.pbData);
		blob = {};
		return result;
	}

	QByteArray encrypt(const QByteArray& data)
	{
		DATA_BLOB input = toBlob(data);

		DATA_BLOB output{};

		if (!CryptProtectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
			return {};

		return takeBlob(output);
	}

	QByteArray decrypt(const QByteArray& data)
	{
		DATA_BLOB input = toBlob(data);

		DATA_BLOB output{};

		if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
			return {};

		return takeBlob(output);
	}
}

bool Session::hasValidAccessToken() const
{
	const QDateTime limit = QDateTime::currentDateTimeUtc().addSecs(kExpiryMarginSeconds);
	return !accessToken.isEmpty() && expiresAt.isValid() && limit < expiresAt;
}

void TokenStore::save(const Session& session) const 
{
	const QJsonObject json{
		{"access_token", session.accessToken},
		{"refresh_token", session.refreshToken},
		{"expires_at", session.expiresAt.toString(Qt::ISODate)}
	};

	const QByteArray encrypted = encrypt(QJsonDocument(json).toJson(QJsonDocument::Compact));

	if (encrypted.isEmpty()) {
		qWarning() << "Não foi possível proteger a sessão; ela não será salva.";
		return;
	}

	QSettings().setValue(kSettingsKey, QString::fromLatin1(encrypted.toBase64()));
}

std::optional<Session> TokenStore::load() const 
{
	const QByteArray stored = QByteArray::fromBase64(QSettings().value(kSettingsKey).toString().toLatin1());

	if (stored.isEmpty())
		return std::nullopt;

	const QJsonObject json = QJsonDocument::fromJson(decrypt(stored)).object();

	Session session;
	session.accessToken = json.value("access_token").toString();
	session.refreshToken = json.value("refresh_token").toString();
	session.expiresAt = QDateTime::fromString(json.value("expires_at").toString(), Qt::ISODate);

	if (session.accessToken.isEmpty() && session.refreshToken.isEmpty())
		return std::nullopt;

	return session;
}

void TokenStore::clear() const 
{
	QSettings().remove(kSettingsKey);
}