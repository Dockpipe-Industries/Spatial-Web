#include <SpatialWeb/Composition/SpatialWebDocument.h>
#include <SpatialWeb/Composition/SpatialWebGraph.h>
#include <SpatialWeb/Composition/SpatialSitePolicy.h>
#include <SpatialWeb/Composition/SpatialSiteWorld.h>
#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

class SpatialCompositionTest final : public QObject {
    Q_OBJECT
private slots:
    void boundedDocuments()
    {
        SpatialWebDocument document;
        QString error;
        const QJsonObject item{{"id", "0"}, {"title", "First"}, {"href", "https://example.org/first"}};
        QJsonObject value{{"url", "https://example.org"}, {"title", "Example"},
            {"items", QJsonArray{item}}};
        QVERIFY(SpatialWebDocument::parse(QJsonDocument(value).toJson(), document, error));
        QCOMPARE(document.items.size(), 1);
        auto graph = SpatialWebGraph::fromDocument(document);
        QVERIFY(graph.validate(error));
        QVERIFY(!SpatialWebDocument::parse(QByteArray(SpatialWebDocument::maximumBytes + 1, 'x'), document, error));
        value["items"] = QJsonArray{item, item};
        QVERIFY(!SpatialWebDocument::parse(QJsonDocument(value).toJson(), document, error));
        for (const auto &url : {"file:///etc/passwd", "javascript:alert(1)", "https://user:pass@example.org"})
            QVERIFY(!SpatialWebDocument::allowedUrl(QUrl(url)));
    }
    void discoveryBoundary()
    {
        QVERIFY(!SpatialSitePolicy::publicAddress(QHostAddress("127.0.0.1")));
        QVERIFY(!SpatialSitePolicy::publicAddress(QHostAddress("192.168.1.1")));
        QVERIFY(SpatialSitePolicy::sameOrigin(QUrl("https://example.org/a"), QUrl("https://example.org/b")));
        QVERIFY(!SpatialSitePolicy::sameOrigin(QUrl("https://example.org"), QUrl("https://other.org")));
        SpatialSiteWorld world;
        world.origin = QUrl("https://example.org");
        QVERIFY(!world.discover({QUrl("https://other.org"), "Other", {}, false}, 0));
    }
};

int main(int argc, char **argv)
{
    QTemporaryDir storage;
    if (!storage.isValid())
        return 1;
    qputenv("XDG_DATA_HOME", storage.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", storage.path().toUtf8());
    qputenv("XDG_CACHE_HOME", storage.path().toUtf8());
    QApplication application(argc, argv);
    SpatialCompositionTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "SpatialCompositionTest.moc"
