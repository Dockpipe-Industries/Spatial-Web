#include <SpatialWeb/Browser/BrowserHostServices.h>
#include <SpatialWeb/Browser/BrowserRuntime.h>
#include <SpatialWeb/Browser/BrowserWidget.h>

#ifdef SPATIAL_WEB_TEST_COMPOSITION
#include <SpatialWeb/Composition/SpatialWebGraph.h>
#endif

#ifdef SPATIAL_WEB_TEST_PROJECTION
#include <SpatialWeb/Projection/SpatialWebBrowser.h>
#endif

#include <QApplication>
#include <QPointer>
#include <QTemporaryDir>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <memory>

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QTemporaryDir storage;
    if (!storage.isValid())
        return 1;
    qputenv("XDG_DATA_HOME", storage.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", storage.path().toUtf8());
    qputenv("XDG_CACHE_HOME", storage.path().toUtf8());
    QApplication application(argc, argv);
    QJsonObject preferences;
    BrowserHostServices::Adapter host;
    host.loadPreferences = [&preferences](QJsonObject &value) { value = preferences; return true; };
    host.savePreferences = [&preferences](const QJsonObject &value) { preferences = value; return true; };
    if (!BrowserHostServices::configure(std::move(host)))
        return 2;
    if (!BrowserRuntime::allowsUrl(QUrl("https://example.invalid"))
        || BrowserRuntime::allowsUrl(QUrl("file:///private")))
        return 3;
#ifdef SPATIAL_WEB_TEST_COMPOSITION
    SpatialWebDocument document;
    document.url = QUrl("https://example.org");
    document.title = "Installed model";
    auto graph = SpatialWebGraph::fromDocument(document);
    QString error;
    if (!graph.validate(error))
        return 8;
#endif
#ifdef SPATIAL_WEB_TEST_PROJECTION
    SpatialWebDocument projectionDocument;
    projectionDocument.url = QUrl("https://example.org");
    if (projectionDocument.room().size.isEmpty())
        return 9;
    SpatialWebBrowser projection(false);
    projection.showStartPage();
    if (projection.spatial())
        return 10;
#endif
    auto first = std::make_unique<BrowserWidget>();
    auto second = std::make_unique<BrowserWidget>();
    first->setCanvasPresentationEnabled(true);
    second->setCanvasPresentationEnabled(true);
    auto *view = first->activeView();
    if (!view || view->page()->profile()->persistentCookiesPolicy() != QWebEngineProfile::NoPersistentCookies)
        return 4;
    QPointer<QWebEngineProfile> profile(view->page()->profile());
    if (!first->moveTabTo(view, second.get()))
        return 5;
    first.reset();
    if (!profile || view->page()->profile() != profile)
        return 6;
    second.reset();
    return profile.isNull() ? 0 : 7;
}
