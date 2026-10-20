#pragma once

#include "SpatialWebBrowserExport.h"

#include <QString>
#include <QUrl>

class QWidget;
class QWebEngineProfile;
class QWebEngineView;

// Shared ordinary-web setup used by the desktop Browser and spatial projection.
// The caller owns the profile and must destroy the view before the profile.
namespace BrowserRuntime {
SPATIAL_WEB_BROWSER_EXPORT bool allowsUrl(const QUrl &url);
SPATIAL_WEB_BROWSER_EXPORT bool allowsNavigation(const QUrl &url, QWebEngineProfile *profile);
SPATIAL_WEB_BROWSER_EXPORT QString rendererError();
SPATIAL_WEB_BROWSER_EXPORT QWebEngineView *createView(QWebEngineProfile *profile, QWidget *parent = nullptr);
}
