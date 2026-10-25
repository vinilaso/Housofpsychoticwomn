#pragma once

#include <qstring.h>
#include <qstringlist.h>

#include <type_traits>
#include <utility>

class UrlPath {
public:
	template <typename... Segments>
	explicit UrlPath(Segments&&... segments)
	{
		static_assert((std::is_convertible_v<Segments, QString>&&...), "UrlPath aceita apenas textos");
		m_segments.reserve(sizeof...(segments));
		(m_segments.append(QString(std::forward<Segments>(segments))), ...);
	}

	[[nodiscard]] QString toString() const;
private:
	QStringList m_segments;
};