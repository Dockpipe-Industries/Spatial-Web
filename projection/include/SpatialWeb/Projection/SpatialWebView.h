#pragma once

#include <spatialpipe/view/SpatialWorldView.h>
#include <functional>

// Browser presentation over the shared scene viewport. Rendering, navigation and
// terrain remain owned by SpatialPipe::Scene; this adapter owns only web HUD/fade.
class SpatialWebView final : public SpatialWorldView {
public:
    explicit SpatialWebView(QWidget *parent = nullptr);
    void setFlightHardwareAcceleration(bool enabled);
    void setFlightStatus(const QString &text);
    void setInteriorPhase(qreal opacity);
    void setTransitionActive(bool active);
    QString activeVehicleId() const;

    std::function<void(const QString &, bool)> destinationActivated;
    std::function<void()> orbitRequested;
    std::function<QString()> vehicleSessionId;

private:
    QString m_status;
    qreal m_phase = 0;
};
