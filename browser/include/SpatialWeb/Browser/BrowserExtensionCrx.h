#pragma once

#include "SpatialWebBrowserExport.h"

#include <QByteArray>
#include <QString>

class SPATIAL_WEB_BROWSER_EXPORT BrowserExtensionCrx final {
public:
    // Extracts identity metadata only. This does not verify the CRX signature.
    // The caller must authenticate the source and validate the archive separately.
    static QByteArray extractPublicKey(const QByteArray &crx, const QString &expectedId,
                                       QString *error);
};
