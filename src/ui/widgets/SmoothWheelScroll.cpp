#include "SmoothWheelScroll.h"

#include <qabstractscrollarea.h>
#include <qpropertyanimation.h>
#include <qscrollbar.h>

#include <QWheelEvent>
#include <algorithm>

using namespace hpw::ui::widgets;

namespace {
	constexpr int kDurationMs = 250;
	constexpr int kPixelsPerNotch = 60;
	constexpr int kAngleDeltaPerNotch = 120;
}

SmoothWheelScroll::SmoothWheelScroll(QAbstractScrollArea* area)
	: QObject(area)
	, m_bar(area->verticalScrollBar())
	, m_animation(new QPropertyAnimation(m_bar, "value", this))
{
	m_animation->setDuration(kDurationMs);
	m_animation->setEasingCurve(QEasingCurve::OutCubic);

	area->viewport()->installEventFilter(this);
}

bool SmoothWheelScroll::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() != QEvent::Wheel)
		return QObject::eventFilter(watched, event);
	
	const auto* wheel = static_cast<QWheelEvent*>(event);
	const int delta = wheel->angleDelta().y();

	if (delta == 0)
		return false;

	const bool isAnimating = m_animation->state() == QAbstractAnimation::Running;
	const int start = isAnimating ? m_target : m_bar->value();

	m_target = std::clamp(start - delta * kPixelsPerNotch / kAngleDeltaPerNotch, m_bar->minimum(), m_bar->maximum());

	m_animation->stop();
	m_animation->setStartValue(m_bar->value());
	m_animation->setEndValue(m_target);
	m_animation->start();

	return true;
}