#include "UrlPath.h"

#include <qurl.h>

QString UrlPath::toString() const {
	QStringList encoded;
	encoded.reserve(m_segments.size());

	for (QString segment : m_segments) {
		while (segment.startsWith(u'/'))
			segment.remove(0, 1);
		while (segment.endsWith(u'/'))
			segment.chop(1);

		if (!segment.isEmpty())
			encoded << QString::fromLatin1(QUrl::toPercentEncoding(segment, "/"));
	}

	return encoded.join(u'/');
}