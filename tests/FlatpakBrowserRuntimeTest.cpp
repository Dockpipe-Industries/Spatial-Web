#include <SpatialWeb/Browser/BrowserRuntime.h>
#include <SpatialWeb/Browser/BrowserWidget.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QtWebEngineCore/qtwebenginecoreglobal.h>
#include <cstdio>
#include <functional>

namespace {
bool waitFor(const std::function<bool()> &condition)
{
    QElapsedTimer deadline;
    deadline.start();
    while (deadline.elapsed() < 15000) {
        QApplication::processEvents();
        if (condition())
            return true;
        QThread::msleep(5);
    }
    return false;
}
}

int main(int argc, char **argv)
{
    if (qEnvironmentVariableIsSet("QTWEBENGINE_DISABLE_SANDBOX")
        || qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS").contains("--no-sandbox")) {
        std::fprintf(stderr, "Installed runtime proof requires the default Chromium sandbox.\n");
        return 1;
    }
    QSettings instance("/.flatpak-info", QSettings::IniFormat);
    if (instance.value("Instance/app-commit").toString().isEmpty()) {
        std::fprintf(stderr, "Runtime proof requires an installed Flatpak app commit.\n");
        return 2;
    }
    QTemporaryDir state;
    if (!state.isValid())
        return 3;
    qputenv("XDG_CONFIG_HOME", state.path().toUtf8());
    qputenv("XDG_DATA_HOME", state.path().toUtf8());
    qputenv("XDG_CACHE_HOME", state.path().toUtf8());
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication application(argc, argv);
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost))
        return 4;
    QObject::connect(&server, &QTcpServer::newConnection, &application, [&] {
        while (server.hasPendingConnections()) {
            auto *socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                QByteArray request = socket->property("request").toByteArray();
                request += socket->readAll();
                if (request.size() > 16384) {
                    socket->abort();
                    return;
                }
                socket->setProperty("request", request);
                if (!request.contains("\r\n\r\n"))
                    return;
                const QByteArray body = "<!doctype html><title>Installed Flatpak Browser</title>"
                    "<body style='background:#173a57;color:white;font:24px sans-serif;padding:40px'>"
                    "Spatial-Web installed runtime<script>document.body.dataset.proof='real-dom'</script></body>";
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: "
                    + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    BrowserWidget browser;
    browser.resize(900, 600);
    browser.show();
    auto *view = browser.activeView();
    if (!view || view->page()->profile()->persistentCookiesPolicy() != QWebEngineProfile::NoPersistentCookies)
        return 5;
    bool loaded = false;
    QObject::connect(view, &QWebEngineView::loadFinished, &application, [&](bool success) { loaded = success; });
    browser.navigate(QUrl(QString("http://127.0.0.1:%1/").arg(server.serverPort())));
    if (!waitFor([&] { return loaded; }))
        return 6;
    QString dom;
    view->page()->runJavaScript("document.body.dataset.proof", [&](const QVariant &value) { dom = value.toString(); });
    if (!waitFor([&] { return dom == "real-dom"; }) || BrowserRuntime::allowsUrl(QUrl("file:///private")))
        return 7;
    const QJsonObject receipt{
        {"check", "flatpak-installed-runtime-dom"},
        {"Qt6Core", QString::fromLatin1(qVersion())},
        {"Qt6WebEngineCore", QString::fromLatin1(qWebEngineVersion())},
        {"runtime_commit", instance.value("Instance/runtime-commit").toString()},
        {"app_commit", instance.value("Instance/app-commit").toString()},
        {"dom", dom},
        {"sandbox", "default"},
    };
    std::puts(QJsonDocument(receipt).toJson(QJsonDocument::Compact).constData());
    return QString::fromLatin1(qVersion()) == "6.10.3"
        && QString::fromLatin1(qWebEngineVersion()) == "6.10.2" ? 0 : 8;
}
