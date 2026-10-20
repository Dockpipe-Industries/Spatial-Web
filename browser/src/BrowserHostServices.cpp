#include "BrowserHostServices.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIconEngine>
#include <QJsonDocument>
#include <QLockFile>
#include <QPainter>
#include <QPalette>
#include <QPainterPath>
#include <QSaveFile>
#include <QStandardPaths>
#include <utility>

namespace {
constexpr qint64 maximumPreferencesBytes = 1024 * 1024;
BrowserHostServices::Adapter hostAdapter;

QString preferencesPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/browser.json";
}

class BrowserIconEngine final : public QIconEngine {
public:
    explicit BrowserIconEngine(BrowserHostServices::Icon icon) : m_icon(icon) {}
    QIconEngine *clone() const override { return new BrowserIconEngine(m_icon); }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        QPixmap result(size);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        paint(&painter, QRect(QPoint(), size), mode, state);
        return result;
    }
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        painter->save();
        const qreal extent = qMin(rect.width(), rect.height());
        painter->translate(rect.center().x() - extent / 2, rect.center().y() - extent / 2);
        painter->scale(extent / 24, extent / 24);
        painter->setRenderHint(QPainter::Antialiasing);
        const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
        QColor color(dark ? "#c2cbd7" : "#526078");
        if (mode == QIcon::Disabled)
            color = QColor(dark ? "#566070" : "#a7b0bf");
        else if (state == QIcon::On)
            color = QColor(dark ? "#91dbc6" : "#16745c");
        painter->setPen(QPen(color, 1.65, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        const auto line = [painter](qreal x1, qreal y1, qreal x2, qreal y2) {
            painter->drawLine(QPointF(x1, y1), QPointF(x2, y2));
        };
        const auto path = [painter](std::initializer_list<QPointF> points) {
            QPainterPath shape;
            bool first = true;
            for (const auto &point : points) {
                if (first)
                    shape.moveTo(point);
                else
                    shape.lineTo(point);
                first = false;
            }
            painter->drawPath(shape);
        };
        using Icon = BrowserHostServices::Icon;
        switch (m_icon) {
        case Icon::Back:
        case Icon::Forward:
            line(4, 12, 20, 12);
            if (m_icon == Icon::Back)
                path({{10, 6}, {4, 12}, {10, 18}});
            else
                path({{14, 6}, {20, 12}, {14, 18}});
            break;
        case Icon::Globe:
            painter->drawEllipse(QRectF(3, 3, 18, 18));
            painter->drawEllipse(QRectF(8, 3, 8, 18));
            line(3, 12, 21, 12);
            break;
        case Icon::Extensions:
            path({{4, 5}, {9, 5}, {9, 3}, {11, 2}, {13, 3}, {13, 5}, {19, 5},
                  {19, 10}, {21, 10}, {22, 12}, {21, 14}, {19, 14}, {19, 20},
                  {13, 20}, {13, 18}, {11, 17}, {9, 18}, {9, 20}, {4, 20}, {4, 5}});
            break;
        case Icon::Refresh:
            painter->drawArc(QRectF(4, 4, 16, 16), 35 * 16, 280 * 16);
            path({{16, 3}, {21, 7}, {16, 9}});
            break;
        case Icon::Download:
            line(12, 4, 12, 17);
            path({{7, 12}, {12, 17}, {17, 12}});
            path({{4, 17}, {4, 21}, {20, 21}, {20, 17}});
            break;
        case Icon::Rename:
            path({{4, 16}, {16, 4}, {20, 8}, {8, 20}, {3, 21}, {4, 16}});
            line(14, 6, 18, 10);
            break;
        case Icon::ChevronDown:
            path({{5, 9}, {12, 16}, {19, 9}});
            break;
        case Icon::Add:
            line(12, 5, 12, 19);
            line(5, 12, 19, 12);
            break;
        case Icon::Minus:
            line(5, 12, 19, 12);
            break;
        case Icon::Properties:
            line(6, 3, 6, 8);
            line(6, 12, 6, 21);
            line(18, 3, 18, 14);
            line(18, 18, 18, 21);
            painter->drawRoundedRect(QRectF(3, 8, 6, 4), 1, 1);
            painter->drawRoundedRect(QRectF(15, 14, 6, 4), 1, 1);
            break;
        case Icon::More:
            painter->setBrush(color);
            for (int x : {5, 12, 19})
                painter->drawEllipse(QPointF(x, 12), 0.9, 0.9);
            break;
        case Icon::Preview: {
            QPainterPath outline;
            outline.moveTo(2, 12);
            outline.cubicTo(7, 3, 17, 3, 22, 12);
            outline.cubicTo(17, 21, 7, 21, 2, 12);
            painter->drawPath(outline);
            painter->drawEllipse(QPointF(12, 12), 3, 3);
            break;
        }
        case Icon::Close:
            line(6, 6, 18, 18);
            line(18, 6, 6, 18);
            break;
        case Icon::Check:
            path({{4, 12}, {9, 17}, {20, 6}});
            break;
        }
        painter->restore();
    }
private:
    BrowserHostServices::Icon m_icon;
};
}

bool BrowserHostServices::configure(Adapter adapter, QString *error)
{
    if (bool(adapter.loadPreferences) != bool(adapter.savePreferences)) {
        if (error)
            *error = QStringLiteral("Browser preferences require both a loader and a saver.");
        return false;
    }
    hostAdapter = std::move(adapter);
    return true;
}

QIcon BrowserHostServices::icon(Icon icon)
{
    if (hostAdapter.icon) {
        const QIcon result = hostAdapter.icon(icon);
        if (!result.isNull())
            return result;
    }
    return QIcon(new BrowserIconEngine(icon));
}

bool BrowserHostServices::loadPreferences(QJsonObject &preferences)
{
    if (hostAdapter.loadPreferences)
        return hostAdapter.loadPreferences(preferences);
    QFile file(preferencesPath());
    if (!file.exists()) {
        preferences = QJsonObject();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly) || file.size() > maximumPreferencesBytes)
        return false;
    const QByteArray contents = file.read(maximumPreferencesBytes + 1);
    const QJsonDocument document = QJsonDocument::fromJson(contents);
    if (contents.size() > maximumPreferencesBytes || !document.isObject())
        return false;
    preferences = document.object();
    return true;
}

bool BrowserHostServices::savePreferences(const QJsonObject &preferences)
{
    if (hostAdapter.savePreferences)
        return hostAdapter.savePreferences(preferences);
    const QByteArray contents = QJsonDocument(preferences).toJson(QJsonDocument::Indented);
    if (contents.size() > maximumPreferencesBytes
        || !QDir().mkpath(QFileInfo(preferencesPath()).absolutePath()))
        return false;
    QLockFile lock(preferencesPath() + ".lock");
    if (!lock.tryLock(0))
        return false;
    QSaveFile file(preferencesPath());
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size() && file.commit();
}
