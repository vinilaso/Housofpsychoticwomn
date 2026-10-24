#pragma once

#include <Windows.h>
#include <dpapi.h>
#include <qbytearray.h>

namespace hpw::security {
	[[nodiscard]] DATA_BLOB toBlob(const QByteArray& bytes);
	[[nodiscard]] QByteArray takeBlob(DATA_BLOB& blob);

	[[nodiscard]] QByteArray encrypt(const QByteArray& bytes);
	[[nodiscard]] QByteArray decrypt(const QByteArray& bytes);
}