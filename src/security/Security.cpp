#include "Security.h"

#include <Windows.h>
#include <dpapi.h>
#include <qbytearray.h>

namespace hpw::security {

	DATA_BLOB toBlob(const QByteArray& bytes)
	{
		return {
			static_cast<DWORD>(bytes.size()),
			reinterpret_cast<BYTE*>(const_cast<char*>(bytes.data()))
		};
	}

	QByteArray takeBlob(DATA_BLOB& blob)
	{
		QByteArray bytes(
			reinterpret_cast<const char*>(blob.pbData),
			static_cast<qsizetype>(blob.cbData)
		);

		LocalFree(blob.pbData);
		blob = {};
		return bytes;
	}

	QByteArray encrypt(const QByteArray& bytes)
	{
		DATA_BLOB input = toBlob(bytes);
		DATA_BLOB output{};

		if (!CryptProtectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
			return {};

		return takeBlob(output);
	}

	QByteArray decrypt(const QByteArray& bytes)
	{
		DATA_BLOB input = toBlob(bytes);
		DATA_BLOB output{};

		if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
			return {};

		return takeBlob(output);
	}
}