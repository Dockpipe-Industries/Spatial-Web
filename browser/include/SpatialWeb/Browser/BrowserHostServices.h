#pragma once

#include "SpatialWebBrowserExport.h"

#include <QIcon>
#include <QJsonObject>
#include <QString>
#include <functional>

// Trusted application integration only. No page or extension receives this API.
class SPATIAL_WEB_BROWSER_EXPORT BrowserHostServices final {
public:
    enum class Icon {
        Add, Minus, Preview, Back, Forward, Refresh, Close, Rename,
        Properties, Extensions, Download, More, Globe, ChevronDown, Check
    };
    struct Adapter {
        std::function<QIcon(Icon)> icon;
        std::function<bool(QJsonObject &)> loadPreferences;
        std::function<bool(const QJsonObject &)> savePreferences;
    };

    // Configure on the application thread before creating browser widgets.
    // Captured host resources must outlive their configured callbacks.
    static bool configure(Adapter adapter, QString *error = nullptr);
    static QIcon icon(Icon icon);
    static bool loadPreferences(QJsonObject &preferences);
    static bool savePreferences(const QJsonObject &preferences);
};
