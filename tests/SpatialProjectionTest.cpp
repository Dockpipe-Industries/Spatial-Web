#include <spatialpipe/scene/SpatialVehicle.h>
#include "SpatialWebBrowser.h"
#include "SpatialWebMedia.h"
#include "SpatialWebWorld.h"
#include "SpatialWebPassage.h"
#include "SpatialSiteExplorer.h"
#include <QComboBox>
#include <QFile>
#include <QBuffer>
#include <QPainter>
#include <QToolButton>
#include <QMenu>
#include "SpatialNeuralService.h"
#include "SpatialAnimalModel.h"
#include <limits>
#include "SpatialWebView.h"
#include <spatialpipe/scene/SpatialInteriorGeometry.h>
#ifdef DOCKPIPE_SPATIAL_WEB
#include "BrowserWidget.h"
#include <QTabBar>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#endif
#include <QApplication>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QLineEdit>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScreen>
#include <QStackedWidget>
#include <QOpenGLWidget>
#include <QPushButton>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QSharedPointer>
#include <cstdio>
#include <algorithm>
#include <cmath>

#define VERIFY_WEB(value) do { if (!(value)) { std::fprintf(stderr, "Spatial web assertion at %d: %s\n", __LINE__, #value); return 1; } } while (false)

namespace {
bool waitForWeb(const std::function<bool()> &ready, int milliseconds = 15000)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (!ready() && elapsed.elapsed() < milliseconds) {
        QApplication::processEvents();
        QThread::msleep(5);
    }
    return ready();
}
}

int spatialWebModelTest()
{
    VERIFY_WEB(SpatialWebDocument::allowedUrl(QUrl("https://example.org/gallery")));
    for (const QString &url : {"file:///etc/passwd", "javascript:alert(1)", "https://user:pass@example.org", "data:text/html,test"})
        VERIFY_WEB(!SpatialWebDocument::allowedUrl(QUrl(url)));
    SpatialWebDocument document;
    QString error;
    VERIFY_WEB(!SpatialWebDocument::parse("not json", document, error));
    QJsonObject value{{"url", "https://example.org"}, {"title", "Gallery"}, {"venue", "gallery"},
        {"items", QJsonArray{QJsonObject{{"id", "0"}, {"title", "First exhibit"}, {"href", "https://example.org/first"}}}}};
    VERIFY_WEB(SpatialWebDocument::parse(QJsonDocument(value).toJson(), document, error));
    VERIFY_WEB(document.items.size() == 1 && document.room().displays.size() == 1);
    VERIFY_WEB(SpatialWebDocument::poster(document.items[0], {}, 0).size() == QSize(512, 320));
    auto items = value.value("items").toArray();
    items.append(items[0]);
    value["items"] = items;
    VERIFY_WEB(!SpatialWebDocument::parse(QJsonDocument(value).toJson(), document, error));
    VERIFY_WEB(!SpatialWebDocument::parse(QByteArray(SpatialWebDocument::maximumBytes + 1, 'x'), document, error));
    QJsonArray tooMany;
    for (int index = 0; index <= SpatialWebDocument::maximumItems; ++index)
        tooMany.append(QJsonObject{{"id", QString::number(index)}, {"title", "Exhibit"}});
    value["items"] = tooMany;
    VERIFY_WEB(!SpatialWebDocument::parse(QJsonDocument(value).toJson(), document, error));
    VERIFY_WEB(SpatialWebDocument::searchResultsUrl(QUrl("https://www.google.com/search?q=rainforest")));
    VERIFY_WEB(SpatialWebDocument::searchResultsUrl(QUrl("https://www.bing.com/search?q=rainforest")));
    VERIFY_WEB(SpatialWebDocument::searchResultsUrl(QUrl("https://duckduckgo.com/?q=rainforest")));
    VERIFY_WEB(!SpatialWebDocument::searchResultsUrl(QUrl("https://example.org/search?q=rainforest")));
    VERIFY_WEB(!SpatialWebDocument::searchResultsUrl(QUrl("https://google.com.evil.test/search?q=rainforest")));
    VERIFY_WEB(!SpatialWebDocument::searchResultsUrl(QUrl("https://google.com/search?q=")));
    SpatialWebDocument capacity;
    for (int index = 0; index < SpatialWebDocument::maximumItems; ++index) {
        SpatialWebItem item;
        item.id = QString::number(index);
        item.title = "Repository " + item.id;
        item.url = QUrl("https://example.org/" + item.id);
        capacity.items.append(item);
    }
    for (const QString &venue : {"repositories", "gallery", "store", "cinema"}) {
        capacity.venue = venue;
        const auto room = capacity.room();
        VERIFY_WEB(room.displays.size() == SpatialWebDocument::maximumItems);
        VERIFY_WEB(!room.solids.isEmpty() && room.solids.size() <= SpatialInterior::maximumSolids);
        VERIFY_WEB(!SpatialInteriorGeometry::blocked(room, {float(room.spawn.x()), 160, float(room.spawn.y())}));
        VERIFY_WEB(!SpatialInteriorGeometry::blocked(room, {0, 160, 0}));
        for (const auto &display : room.displays)
            VERIFY_WEB(display.freestanding);
        if (venue == "repositories" || venue == "gallery") {
            for (const auto &surface : room.displays) {
                const float radians = surface.yaw * 3.14159265f / 180;
                const QVector3D outward(std::sin(radians), 0, -std::cos(radians));
                const QVector3D approach = surface.portalCenter + outward * 100;
                const QVector3D previous = surface.portalCenter + outward * 120;
                VERIFY_WEB(!SpatialInteriorGeometry::blocked(room, approach));
                VERIFY_WEB(SpatialInteriorGeometry::approachedPortal(room, previous, approach, -outward) == surface.id);
                VERIFY_WEB(SpatialInteriorGeometry::approachedPortal(room, approach, previous, -outward).isEmpty());
                VERIFY_WEB(SpatialInteriorGeometry::approachedPortal(room, previous, approach, outward).isEmpty());
                VERIFY_WEB(SpatialInteriorGeometry::approachedPortal(room, approach, approach, -outward).isEmpty());
            }
        }
        if (venue == "cinema") {
            VERIFY_WEB(room.displays[0].id == "cinema-screen");
            for (int index = 1; index < room.displays.size(); ++index) {
                const auto &surface = room.displays[index];
                const auto corners = SpatialInteriorGeometry::surface(room, surface);
                const auto normal = QVector3D::crossProduct(corners[1] - corners[0], corners[3] - corners[0]);
                VERIFY_WEB(QVector3D::dotProduct(normal, QVector3D(0, 260, surface.center.z()) - surface.center) > 0);
            }
        }
    }
    capacity.venue = "mall";
    capacity.url = QUrl("https://example.org");
    capacity.title = "Shared content";
    auto animals = capacity;
    animals.venue = "park";
    animals.title = "Invertebrate animals";
    animals.evidence = "Animal habitats and wildlife";
    for (const QString &form : {"spider", "crab", "insect", "worm", "butterfly", "fish", "bird", "quadruped", "cephalopod"}) {
        for (auto &item : animals.items) {
            item.kind = "animal";
            item.animalForm = form;
        }
        const auto plan = SpatialWebWorld::plan(animals, SpatialWebGraph::fromDocument(animals), SpatialWebWorld::optionsFor(animals));
        const auto result = SpatialWebWorld::compile(plan);
        if (!result.success)
            std::fprintf(stderr, "Animal %s construction: %s\n", qPrintable(form), qPrintable(result.diagnostic));
        VERIFY_WEB(result.success);
        VERIFY_WEB(std::none_of(result.interior.displays.cbegin(), result.interior.displays.cend(), [](const auto &display) { return display.portal; }));
        VERIFY_WEB(std::count_if(result.interior.solids.cbegin(), result.interior.solids.cend(), [](const auto &solid) {
            return solid.label.startsWith("Illustrative model");
        }) == 4);
        const auto model = SpatialAnimalModel::assemble(form);
        VERIFY_WEB(model.vertices == SpatialAnimalModel::assemble(form).vertices);
        VERIFY_WEB(model.vertices.size() <= 64 && model.faces.size() <= 40);
    }
    VERIFY_WEB(SpatialAnimalModel::assemble("unknown").vertices.isEmpty());
    SpatialWebItem spider;
    spider.kind = "link";
    spider.title = "Black Widow";
    SpatialAnimalModel::interpret(spider, "Movies and cinema");
    VERIFY_WEB(spider.animalForm.isEmpty());
    SpatialAnimalModel::interpret(spider, "Invertebrates and animals");
    VERIFY_WEB(spider.kind == "animal" && spider.animalForm == "spider");
    auto selfLink = capacity;
    selfLink.items.first().url = QUrl("https://example.org/#section");
    VERIFY_WEB(SpatialWebGraph::fromDocument(selfLink).nodes[2].action == SpatialWebNode::Action::None);
    for (int index = 0; index < capacity.items.size(); ++index)
        capacity.items[index].category = "Group " + QString::number(index / 3);
    auto departmental = capacity;
    const auto departmentOptions = SpatialWebWorld::optionsFor(departmental);
    VERIFY_WEB(departmentOptions.topology == SpatialTopology::Branches);
    auto catalogue = departmental;
    catalogue.accent = QColor("#bd4f89");
    for (auto &item : catalogue.items) {
        item.category.clear();
        item.hasImage = false;
    }
    const auto catalogueOptions = SpatialWebWorld::optionsFor(catalogue);
    VERIFY_WEB(catalogueOptions.topology == SpatialTopology::Spine);
    VERIFY_WEB(catalogueOptions.style.accent == catalogue.accent);
    auto naturalArticle = catalogue;
    naturalArticle.venue = "park";
    naturalArticle.evidence = "Wildlife habitat facts";
    const auto naturalOptions = SpatialWebWorld::optionsFor(naturalArticle);
    VERIFY_WEB(naturalOptions.topology == SpatialTopology::Spine);
    const auto naturalPlan = SpatialWebWorld::plan(naturalArticle, SpatialWebGraph::fromDocument(naturalArticle), naturalOptions);
    VERIFY_WEB(SpatialWebWorld::compile(naturalPlan).success);
    VERIFY_WEB(!naturalPlan.meshes.isEmpty());
    for (const auto &hill : naturalPlan.meshes) {
        // Scenery here consists of five-vertex hills; paths are added by compile.
        VERIFY_WEB(hill.vertices.size() == 5);
        const auto peak = hill.vertices.last();
        VERIFY_WEB(std::abs(peak.x() - naturalPlan.spawn.x()) >= 480
            || std::abs(peak.z() - naturalPlan.spawn.z()) >= 480);
    }
    auto illustrated = catalogue;
    for (auto &item : illustrated.items) {
        item.hasImage = true;
        item.visualAspect = 2.3f;
    }
    const auto illustratedOptions = SpatialWebWorld::optionsFor(illustrated);
    VERIFY_WEB(illustratedOptions.topology == SpatialTopology::Loop);
    const auto illustratedGraph = SpatialWebGraph::fromDocument(illustrated);
    const auto widePlan = SpatialWebWorld::plan(illustrated, illustratedGraph, illustratedOptions);
    if (!SpatialWebWorld::compile(widePlan).success)
        std::fprintf(stderr, "Illustrated construction: %s\n", qPrintable(SpatialWebWorld::compile(widePlan).diagnostic));
    VERIFY_WEB(SpatialWebWorld::compile(widePlan).success);
    const auto wideRoom = SpatialWebWorld::compile(widePlan).interior;
    for (const auto &entrance : wideRoom.displays) {
        SpatialWebPassage passage;
        VERIFY_WEB(passage.begin(wideRoom, entrance));
        VERIFY_WEB(passage.stage(wideRoom));
        const float radians = entrance.yaw * 3.14159265f / 180;
        const QVector3D inward(-std::sin(radians), 0, std::cos(radians));
        const auto joined = passage.room();
        for (int distance = 0; distance <= 750; distance += 25)
            VERIFY_WEB(!SpatialInteriorGeometry::blocked(joined, entrance.portalCenter + inward * distance));
    }
    for (auto &item : illustrated.items)
        item.visualAspect = 0.6f;
    const auto narrowPlan = SpatialWebWorld::plan(illustrated, illustratedGraph, illustratedOptions);
    VERIFY_WEB(SpatialWebWorld::compile(narrowPlan).success);
    VERIFY_WEB(widePlan.displays.first().size.width() > narrowPlan.displays.first().size.width());
    VERIFY_WEB(qAbs(widePlan.displays.first().size.width() / widePlan.displays.first().size.height() - 1.6) < 0.001);
    // Artwork adapts to source aspect; the walk-through doorway keeps its clearance.
    const auto graph = SpatialWebGraph::fromDocument(capacity);
    VERIFY_WEB(graph.validate(error));
    auto rainforest = capacity;
    rainforest.venue = "park";
    rainforest.evidence = "Rainforest habitats and wildlife";
    for (int index = 0; index < 8; ++index)
        rainforest.items[index].kind = "excerpt";
    const auto habitat = SpatialWebWorld::construct(rainforest, SpatialWebWorld::optionsFor(rainforest));
    VERIFY_WEB(habitat.success && !habitat.fallback && habitat.interior.naturalGround);
    VERIFY_WEB(habitat.interior.solids.size() <= SpatialInterior::maximumSolids);
    VERIFY_WEB(!habitat.interior.displays.first().portal);
    VERIFY_WEB(habitat.interior.displays.last().portalApproachDistance == 240);
    QSet<QString> layouts;
    for (auto topology : {SpatialTopology::Branches, SpatialTopology::Loop, SpatialTopology::Spine, SpatialTopology::Layers}) {
        SpatialWorldOptions options;
        options.topology = topology;
        const auto plan = SpatialWebWorld::plan(capacity, graph, options);
        const auto world = SpatialWebWorld::compile(plan);
        if (!world.success)
            std::fprintf(stderr, "Composition rejected: %s\n", qPrintable(world.diagnostic));
        VERIFY_WEB(world.success && !world.fallback);
        VERIFY_WEB(world.interior.displays.size() == 12);
        VERIFY_WEB(world.interior.solids.size() <= SpatialInterior::maximumSolids);
        VERIFY_WEB(world.interior.meshes.first().vertices.size() <= SpatialInterior::maximumMeshVertices);
        QString geometry;
        for (const auto &anchor : plan.anchors)
            geometry += QString("%1,%2,%3;").arg(anchor.position.x()).arg(anchor.position.y()).arg(anchor.position.z());
        for (const auto &edge : plan.connections)
            geometry += QString("%1>%2;").arg(edge.from).arg(edge.to);
        layouts.insert(geometry);
        const auto repeat = SpatialWebWorld::plan(capacity, graph, options);
        VERIFY_WEB(repeat.anchors.size() == plan.anchors.size());
        for (int index = 0; index < plan.anchors.size(); ++index)
            VERIFY_WEB(repeat.anchors[index].position == plan.anchors[index].position);
        for (const auto &surface : world.interior.displays) {
            const auto node = std::find_if(graph.nodes.cbegin(), graph.nodes.cend(), [&](const SpatialWebNode &candidate) {
                return candidate.id == surface.id;
            });
            VERIFY_WEB(node != graph.nodes.cend());
            VERIFY_WEB(node->url == capacity.items[node->itemIndex].url);
            if (world.interior.locomotion == "walk") {
                SpatialWebPassage passage;
                VERIFY_WEB(passage.begin(world.interior, surface));
                VERIFY_WEB(passage.stage(world.interior));
                const float angle = surface.yaw * 3.14159265358979323846f / 180;
                const QVector3D direction(-std::sin(angle), 0, std::cos(angle));
                const auto connected = passage.room();
                VERIFY_WEB(connected.solids.size() <= SpatialWebPassage::solidBudget);
                VERIFY_WEB(connected.displays.size() <= SpatialWorkspace::maximumDisplays);
                for (int distance = 0; distance <= 950; distance += 25) {
                    const auto position = surface.portalCenter + direction * distance;
                    if (SpatialInteriorGeometry::blocked(connected, position))
                        std::fprintf(stderr, "Blocked passage: topology=%d item=%s distance=%d\n", int(topology), qPrintable(surface.id), distance);
                    VERIFY_WEB(!SpatialInteriorGeometry::blocked(connected, position));
                }
            }
        }
        auto extended = plan;
        SpatialMesh landmark;
        landmark.color = Qt::yellow;
        landmark.vertices = {{-100, 800, 0}, {100, 800, 0}, {0, 880, 0}};
        landmark.faces = {{0, 1, 2}};
        extended.meshes.append(landmark);
        const int nested = extended.transforms.size();
        extended.transforms.append({0, {0, 850, 0}, 35, 0.5f});
        extended.parts.append({nested, {}, {200, 100, 200}, Qt::cyan, SpatialSolidShape::Octahedron, false});
        VERIFY_WEB(SpatialWebWorld::compile(extended).success);
        extended.meshes[0].faces[0][0] = 20;
        VERIFY_WEB(!SpatialWebWorld::compile(extended).success);
        auto hidden = plan;
        const auto surface = hidden.displays.first();
        const auto corners = SpatialInteriorGeometry::surface(world.interior, surface);
        SpatialMesh curtain;
        curtain.color = Qt::black;
        const auto towardsAnchor = (surface.portalCenter - surface.center).normalized() * 20;
        for (const auto &corner : corners)
            curtain.vertices.append(corner + towardsAnchor);
        curtain.faces = {{0, 1, 2, 3}};
        hidden.meshes.append(curtain);
        VERIFY_WEB(!SpatialWebWorld::compile(hidden).success);
        auto obstructed = plan;
        obstructed.parts.append({0, plan.spawn, {200, 360, 200}, Qt::red});
        VERIFY_WEB(!SpatialWebWorld::compile(obstructed).success);
        auto disconnected = plan;
        disconnected.connections.clear();
        VERIFY_WEB(!SpatialWebWorld::compile(disconnected).success);
        auto cyclic = plan;
        cyclic.transforms[0].parent = 0;
        VERIFY_WEB(!SpatialWebWorld::compile(cyclic).success);
        auto invalid = plan;
        invalid.displays[0].center.setX(std::numeric_limits<float>::quiet_NaN());
        VERIFY_WEB(!SpatialWebWorld::compile(invalid).success);
        auto lost = plan;
        lost.displays.removeLast();
        VERIFY_WEB(!SpatialWebWorld::compile(lost).success);
        auto excessive = plan;
        excessive.parts.resize(SpatialInterior::maximumSolids + 1);
        VERIFY_WEB(!SpatialWebWorld::compile(excessive).success);
    }
    VERIFY_WEB(layouts.size() == 4);
    SpatialWorldOptions invalidOptions;
    invalidOptions.spacing = 100;
    const auto fallback = SpatialWebWorld::construct(capacity, invalidOptions);
    VERIFY_WEB(fallback.success && fallback.fallback && !fallback.diagnostic.isEmpty());
    VERIFY_WEB(fallback.interior.displays.size() == capacity.items.size());
    auto unsafe = capacity;
    unsafe.items[0].url = QUrl("javascript:alert(1)");
    VERIFY_WEB(!SpatialWebWorld::construct(unsafe, {}).success);
    for (int count : {0, 1}) {
        auto sparse = capacity;
        sparse.items.resize(count);
        VERIFY_WEB(SpatialWebWorld::construct(sparse, {}).success);
    }
    // Appearance evidence cannot rewrite semantic containment or URLs.
    auto themed = capacity;
    themed.evidence = "interstellar forest castle";
    const auto themedGraph = SpatialWebGraph::fromDocument(themed);
    VERIFY_WEB(themedGraph.nodes.size() == graph.nodes.size());
    for (int index = 0; index < graph.nodes.size(); ++index) {
        VERIFY_WEB(themedGraph.nodes[index].parent == graph.nodes[index].parent);
        VERIFY_WEB(themedGraph.nodes[index].url == graph.nodes[index].url);
    }

    // Slow pages can extend only four sections. Every orientation has a real
    // collision door, a clear walking lane, and a reversible coordinate rebase.
    for (float yaw : {0.0f, 90.0f, 180.0f, -90.0f}) {
        SpatialInterior source;
        source.size = {6000, 6000};
        SpatialDisplay entrance;
        entrance.id = "entrance";
        entrance.freestanding = true;
        entrance.portal = true;
        entrance.yaw = yaw;
        source.displays.append(entrance);
        SpatialWebPassage passage;
        VERIFY_WEB(passage.begin(source, entrance));
        const float angle = yaw * 3.14159265358979323846f / 180;
        const auto point = [angle](float distance) {
            return QVector3D(-std::sin(angle) * distance, 160, std::cos(angle) * distance);
        };
        VERIFY_WEB(passage.segments() == 2);
        VERIFY_WEB(SpatialInteriorGeometry::blocked(passage.room(), point(800)));
        VERIFY_WEB(passage.advance(point(450)) && passage.segments() == 3);
        VERIFY_WEB(passage.advance(point(850)) && passage.segments() == 4);
        VERIFY_WEB(!passage.advance(point(100000)) && passage.segments() == 4);
        VERIFY_WEB(SpatialInteriorGeometry::blocked(passage.room(), point(1600)));
        VERIFY_WEB(SpatialInteriorGeometry::blocked(passage.room(), point(1100)));
        for (int distance = 0; distance < 1030; distance += 20)
            VERIFY_WEB(!SpatialInteriorGeometry::blocked(passage.room(), point(distance)));
        SpatialInterior destination;
        destination.spawn = {0, -350};
        destination.displays.append(entrance);
        VERIFY_WEB(passage.stage(destination));
        VERIFY_WEB(!SpatialInteriorGeometry::blocked(passage.room(), point(1600)));
        VERIFY_WEB(!SpatialInteriorGeometry::blocked(passage.room(), point(1100)));
        VERIFY_WEB(passage.room().displays.last().id == "arrival:entrance");
        VERIFY_WEB(passage.arrived(point(1760)));
        VERIFY_WEB((passage.destinationPosition(point(1760)) - QVector3D(0, 160, -350)).length() < 0.01);
        VERIFY_WEB(passage.room().solids.size() <= SpatialWebPassage::solidBudget);
        VERIFY_WEB(passage.begin(source, entrance));
        passage.fail();
        VERIFY_WEB(!passage.stage(destination));
        VERIFY_WEB(!passage.advance(point(450)));
        VERIFY_WEB(SpatialInteriorGeometry::blocked(passage.room(), point(800)));
    }

    SpatialWebView scene;
    scene.setHardwareAcceleration(false);
    scene.show();
    SpatialInterior controls;
    controls.size = {6000, 6000};
    controls.height = 2000;
    controls.locomotion = "drive";
    controls.spawnHeight = 110;
    scene.setInterior(controls, "Controls fixture");
    const auto driveStart = scene.cameraPosition();
    scene.setPilotInput({0, 0, 1}, {}, false);
    VERIFY_WEB(waitForWeb([&]() { return scene.cameraPosition().z() > driveStart.z() + 40; }, 1500));
    VERIFY_WEB(scene.cameraPosition().y() == 110);
    const auto steerStart = scene.cameraPosition();
    scene.setPilotInput({1, 0, 1}, {}, false);
    VERIFY_WEB(waitForWeb([&]() { return scene.cameraPosition().x() > steerStart.x() + 5; }, 1000));
    scene.setPilotInput({}, {}, false);
    VERIFY_WEB(waitForWeb([&]() { return !scene.moving(); }, 3000));
    scene.setInteriorLocomotion("fly");
    const auto flightStart = scene.cameraPosition();
    scene.setPilotInput({0, 1, 0}, {}, false);
    VERIFY_WEB(waitForWeb([&]() { return scene.cameraPosition().y() > flightStart.y() + 50; }, 1000));
    scene.setPilotInput({}, {}, false);
    VERIFY_WEB(waitForWeb([&]() { return !scene.moving(); }, 3000));
    scene.setInteriorLocomotion("walk");
    VERIFY_WEB(scene.cameraPosition().y() == 160);
    scene.hide();
    scene.setInterior(document.room(), "Fixture");
    VERIFY_WEB(!scene.setInteriorImage("missing", QImage(16, 16, QImage::Format_RGB32)));
    VERIFY_WEB(!scene.setInteriorImage("0", QImage(513, 320, QImage::Format_RGB32)));
#if defined(DOCKPIPE_SPATIAL_WEB) && defined(Q_OS_LINUX)
    const QByteArray renderer = qgetenv("QTWEBENGINEPROCESS_PATH");
    const bool hadRenderer = qEnvironmentVariableIsSet("QTWEBENGINEPROCESS_PATH");
    qputenv("QTWEBENGINEPROCESS_PATH", "/nonexistent/dockpipe-web-test/QtWebEngineProcess");
    {
        SpatialWebBrowser missing(false);
        missing.show();
        QApplication::processEvents();
        VERIFY_WEB(missing.status().contains("renderer is missing"));
        missing.navigate(QUrl("https://example.org"));
        missing.buildSpace();
        missing.showPage();
        missing.showSpace();
        missing.activateItem("0");
        missing.hide();
        VERIFY_WEB(!missing.building());
    }
    if (hadRenderer)
        qputenv("QTWEBENGINEPROCESS_PATH", renderer);
    else
        qunsetenv("QTWEBENGINEPROCESS_PATH");
#endif
    std::puts("Spatial document URL, bounds, duplicate identity, room, image and missing-runtime contracts passed.");
    return 0;
}


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
    return spatialWebModelTest();
}
