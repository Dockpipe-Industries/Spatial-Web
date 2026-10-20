#include "BrowserHostServices.h"
#include "BrowserPreferences.h"

#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class BrowserHostServicesTest final : public QObject {
    Q_OBJECT
private slots:
    void hostPersistenceAndFailure()
    {
        QJsonObject stored;
        BrowserHostServices::Adapter adapter;
        adapter.loadPreferences = [&stored](QJsonObject &value) { value = stored; return true; };
        adapter.savePreferences = [&stored](const QJsonObject &value) { stored = value; return true; };
        QVERIFY(BrowserHostServices::configure(adapter));
        BrowserSettings settings = BrowserPreferences::instance()->settings();
        settings.showFullUrl = true;
        QVERIFY(BrowserPreferences::instance()->save(settings));
        QVERIFY(stored.value("showFullUrl").toBool());
        QVERIFY(BrowserPreferences::instance()->settings().showFullUrl);
        adapter.savePreferences = [](const QJsonObject &) { return false; };
        QVERIFY(BrowserHostServices::configure(adapter));
        QVERIFY(!BrowserPreferences::instance()->save(settings));
        QVERIFY(BrowserHostServices::configure({}));
    }
    void partialAdapterRejected()
    {
        BrowserHostServices::Adapter adapter;
        adapter.loadPreferences = [](QJsonObject &) { return true; };
        QString error;
        QVERIFY(!BrowserHostServices::configure(adapter, &error));
        QVERIFY(!error.isEmpty());
    }
    void standalonePersistenceAndIcons()
    {
        QVERIFY(BrowserHostServices::configure({}));
        const QJsonObject expected{{"showFullUrl", true}};
        QVERIFY(BrowserHostServices::savePreferences(expected));
        QJsonObject actual;
        QVERIFY(BrowserHostServices::loadPreferences(actual));
        QCOMPARE(actual, expected);
        QVERIFY(!BrowserHostServices::icon(BrowserHostServices::Icon::Back).pixmap(24, 24).isNull());
        QFile corrupt(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/browser.json");
        QVERIFY(corrupt.open(QIODevice::WriteOnly));
        QVERIFY(corrupt.write("invalid") > 0);
        corrupt.close();
        QVERIFY(!BrowserHostServices::loadPreferences(actual));
    }
};
int main(int argc, char **argv)
{
    QTemporaryDir storage;
    if (!storage.isValid())
        return 1;
    qputenv("XDG_CONFIG_HOME", storage.path().toUtf8());
    QApplication app(argc, argv);
    BrowserHostServicesTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "BrowserHostServicesTest.moc"
