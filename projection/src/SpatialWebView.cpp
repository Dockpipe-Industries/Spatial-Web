#include "SpatialWebView.h"
#include <QPainter>

SpatialWebView::SpatialWebView(QWidget *parent) : SpatialWorldView(parent)
{
    displayActivated = [this](const QString &id) {
        if (destinationActivated)
            destinationActivated("display:" + id, false);
    };
    actionActivated = [this](const QString &destination) {
        if (destinationActivated)
            destinationActivated("civic:" + destination, false);
    };
    returnRequested = [this]() {
        if (orbitRequested)
            orbitRequested();
    };
    paintOverlay = [this](QPainter *painter) {
        if (m_phase > 0)
            painter->fillRect(viewport()->rect(), QColor(8, 14, 22, qRound(m_phase * 255)));
        if (!m_status.isEmpty()) {
            const QRect bounds = viewport()->rect().adjusted(12, 12, -12, -12);
            const QRect textArea(bounds.left(), bounds.bottom() - 72, bounds.width(), 72);
            painter->fillRect(textArea, QColor(8, 14, 22, 190));
            painter->setPen(Qt::white);
            painter->drawText(textArea.adjusted(10, 4, -10, -4), Qt::AlignVCenter | Qt::TextWordWrap, m_status);
        }
    };
}

void SpatialWebView::setFlightHardwareAcceleration(bool enabled)
{
    setHardwareAcceleration(enabled, !isVisible());
}

void SpatialWebView::setFlightStatus(const QString &text)
{
    m_status = text.left(1200);
    requestFrame();
}

void SpatialWebView::setInteriorPhase(qreal opacity)
{
    m_phase = qBound(qreal(0), opacity, qreal(1));
    requestFrame();
}

void SpatialWebView::setTransitionActive(bool active)
{
    setNavigationEnabled(!active);
}

QString SpatialWebView::activeVehicleId() const
{
    return vehicleSessionId ? vehicleSessionId() : QString();
}
