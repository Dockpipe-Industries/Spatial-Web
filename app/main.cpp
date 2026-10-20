#include <SpatialWeb/Browser/BrowserWidget.h>
#ifdef SPATIAL_WEB_SHELL_PROJECTION
#include <SpatialWeb/Projection/SpatialWebBrowser.h>
#endif
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication application(argc, argv);
    application.setApplicationName("Spatial-Web");
    application.setOrganizationName("Spatial-Web");
    QMainWindow window;
    window.setWindowTitle("Spatial-Web Browser");
    auto *tabs = new QTabWidget(&window);
    auto *browser = new BrowserWidget(tabs);
    tabs->addTab(browser, "Browser");
#ifdef SPATIAL_WEB_SHELL_PROJECTION
    auto *spatial = new SpatialWebBrowser(true, tabs);
    tabs->addTab(spatial, "Spatial Web");
    QObject::connect(spatial, &SpatialWebBrowser::exitRequested, tabs, [tabs]() {
        tabs->setCurrentIndex(0);
    });
#endif
    window.setCentralWidget(tabs);
    window.resize(1200, 800);
    const auto arguments = application.arguments();
    for (int index = 1; index < arguments.size(); ++index) {
#ifdef SPATIAL_WEB_SHELL_PROJECTION
        if (arguments.at(index) == "--spatial") {
            tabs->setCurrentIndex(1);
            continue;
        }
        if (tabs->currentIndex() == 1) {
            spatial->navigate(BrowserWidget::addressUrl(arguments.at(index)));
            continue;
        }
#endif
        browser->navigate(BrowserWidget::addressUrl(arguments.at(index)));
    }
    window.show();
    return application.exec();
}
