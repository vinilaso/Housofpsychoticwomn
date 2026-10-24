#pragma once

#include <qobject.h>

class QAbstractScrollArea;
class QPropertyAnimation;
class QScrollBar;

namespace hpw::ui::widgets {
	class SmoothWheelScroll : public QObject {
	public:
		explicit SmoothWheelScroll(QAbstractScrollArea* area);

	protected:
		bool eventFilter(QObject* watched, QEvent* event) override;

	private:
		QScrollBar* m_bar;
		QPropertyAnimation* m_animation;
		int m_target = 0;
	};
}