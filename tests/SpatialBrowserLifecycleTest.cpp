#include <SpatialWeb/Browser/BrowserWidget.h>
#include <SpatialWeb/Projection/SpatialWebBrowser.h>
#include <SpatialWeb/Projection/SpatialWebView.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <cstdio>
#include <functional>

namespace {
bool waitFor(const std::function<bool()> &condition, int milliseconds = 15000)
{
    QElapsedTimer clock;
    clock.start();
    while (!condition() && clock.elapsed() < milliseconds) {
        QApplication::processEvents();
        QThread::msleep(5);
    }
    return condition();
}
}

#define CHECK_BROWSER(condition) \
    do { \
        if (!(condition)) { \
            std::fprintf(stderr, "Spatial browser lifecycle assertion %d: %s\n", __LINE__, #condition); \
            return 1; \
        } \
    } while (false)

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QTemporaryDir storage;
    CHECK_BROWSER(storage.isValid());
    qputenv("XDG_CONFIG_HOME", storage.path().toUtf8());
    qputenv("XDG_DATA_HOME", storage.path().toUtf8());
    qputenv("XDG_CACHE_HOME", storage.path().toUtf8());
    QApplication application(argc, argv);
    QTcpServer server;
    CHECK_BROWSER(server.listen(QHostAddress::LocalHost));
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&]() {
        while (auto *socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
                const auto request = socket->property("request").toByteArray() + socket->readAll();
                socket->setProperty("request", request);
                if (!request.contains("\r\n\r\n"))
                    return;
                const QByteArray body = "<html><head><title>Public Gallery Fixture</title></head>"
                    "<body><main><h1>Gallery of public source exhibits</h1>"
                    "<article><a href='/first'><h2>First public exhibit</h2></a>"
                    "<p>A bounded source description for the first gallery exhibit.</p></article>"
                    "<article><a href='/second'><h2>Second public exhibit</h2></a>"
                    "<p>A second bounded description for readers to explore.</p></article>"
                    "</main></body></html>";
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\nContent-Length: "
                    + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    BrowserWidget ordinary;
    SpatialWebBrowser spatial(false);
    spatial.resize(1100, 760);
    spatial.show();
    auto *embedded = spatial.findChild<BrowserWidget *>();
    CHECK_BROWSER(embedded && embedded->activeView());
    CHECK_BROWSER(ordinary.activeView()->page()->profile() != embedded->activeView()->page()->profile());
    CHECK_BROWSER(embedded->activeView()->page()->profile()->persistentCookiesPolicy() == QWebEngineProfile::NoPersistentCookies);
    int publications = 0;
    QObject::connect(&spatial, &SpatialWebBrowser::spaceReady, [&]() { ++publications; });
    const QUrl url(QString("http://127.0.0.1:%1/").arg(server.serverPort()));
    spatial.navigate(url, false);
    CHECK_BROWSER(waitFor([&]() { return embedded->activeView()->title() == "Public Gallery Fixture"; }));
    spatial.buildSpace();
    CHECK_BROWSER(waitFor([&]() { return publications > 0 && !spatial.building(); }));
    CHECK_BROWSER(spatial.spatial());
    CHECK_BROWSER(spatial.document().url == url);
    CHECK_BROWSER(!spatial.document().items.isEmpty());
    const auto camera = spatial.scene()->cameraPosition();
    const auto environment = spatial.document().environmentKey();
    const QString proofDirectory = qEnvironmentVariable("SPATIAL_WEB_PROOF_DIR");
    if (!proofDirectory.isEmpty()) {
        CHECK_BROWSER(QDir().mkpath(proofDirectory));
        spatial.scene()->requestFrame();
        QApplication::processEvents();
        CHECK_BROWSER(spatial.grab().save(proofDirectory + "/spatial.png"));
    }
    spatial.showPage();
    if (!proofDirectory.isEmpty()) {
        QApplication::processEvents();
        CHECK_BROWSER(spatial.grab().save(proofDirectory + "/browser.png"));
    }
    spatial.showSpace();
    CHECK_BROWSER(spatial.document().environmentKey() == environment);
    CHECK_BROWSER(spatial.scene()->cameraPosition() == camera);
    spatial.showStartPage();
    CHECK_BROWSER(!spatial.spatial());
    spatial.hide();
    std::puts("Shared browser profiles, DOM projection, live/world preservation and neutral start-page lifecycle passed.");
    return 0;
}
