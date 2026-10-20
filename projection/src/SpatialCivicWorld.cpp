#include "SpatialCivicWorld.h"
#include "SpatialCivicBuilder.h"
#include <spatialpipe/scene/FurnishingCatalog.h>
#include <algorithm>
#include <cmath>

namespace {
using Builder = SpatialCivicBuilder;
}

SpatialInterior SpatialCivicWorld::cinema(const SpatialWebDocument &document)
{
    Builder builder;
    builder.room.spawn = {0, -650};
    builder.street();
    builder.shell("GRAND CINEMA · LOBBY");
    builder.box({0, 45, 4050}, {2800, 90, 350}, "#4d2b40");
    for (float side : {-1.f, 1.f}) {
        builder.box({side * 1350, 800, 4160}, {220, 1500, 160}, "#8e3450");
        builder.box({side * 1420, 350, 2200}, {20, 32, 3600}, "#edbd77", {}, false);
        for (float z : {1500.f, 3000.f}) {
            builder.box({side * 1460, 750, z}, {30, 260, 160}, "#b99a68");
            builder.box({side * 1435, 750, z}, {20, 180, 80}, "#ffe3a3", {}, false);
        }
    }
    for (float z : {1500.f, 2900.f}) {
        builder.panel({{-700, 1560, z - 200}, {700, 1560, z - 200},
            {700, 1560, z + 200}, {-700, 1560, z + 200}}, "#d1b78b");
        builder.panel({{-600, 1558, z - 130}, {600, 1558, z - 130},
            {600, 1558, z + 130}, {-600, 1558, z + 130}}, "#ffe5af");
    }
    // Two Art Deco ceiling ribs: fixed twelve-segment arches, cached with the
    // other architectural meshes and still within the 512-vertex world budget.
    for (float z : {900.f, 3200.f}) {
        for (int segment = 0; segment < 12; ++segment) {
            const float a = segment * 3.14159265f / 12;
            const float b = (segment + 1) * 3.14159265f / 12;
            const QVector3D left(1440 * std::cos(a), 940 + 570 * std::sin(a), z);
            const QVector3D right(1440 * std::cos(b), 940 + 570 * std::sin(b), z);
            builder.panel({left, right, right + QVector3D(0, 35, 0), left + QVector3D(0, 35, 0)}, "#d1b78b");
        }
    }
    builder.box({0, 1510, 4160}, {2800, 60, 160}, "#d1aa68");
    SpatialDisplay screen;
    screen.id = "cinema-screen";
    screen.application.name = "Grand screen · Select a film from the wall";
    screen.freestanding = true;
    screen.center = {0, 810, 4000};
    screen.size = {2400, 1350};
    builder.room.displays.append(screen);
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 4; ++column) {
            const float x = (column - 1.5f) * 340 + (column < 2 ? -180 : 180);
            builder.seat(QString("stalls-%1-%2").arg(row).arg(column), x, 1100 + row * 560);
        }
    }
    // A real raised floor, supported by the walking collider. Lifts avoid steep
    // stairs and give keyboard/controller users the same accessible route.
    builder.box({0, 420, 480}, {2900, 40, 850}, "#394454");
    builder.room.walkSurfaces.append({QRectF(-1450, 55, 2900, 850), 440});
    builder.box({0, 500, 900}, {2900, 120, 30}, "#b89965");
    for (int column = 0; column < 4; ++column)
        builder.seat(QString("balcony-%1").arg(column), (column - 1.5f) * 420, 580, 440);
    builder.box({-1260, 180, -110}, {160, 360, 200}, "#697581", "BALCONY LIFT");
    builder.action("balcony-up", "Balcony lift · E / A", {-1260, 170, -260}, SpatialActionKind::Transfer, {-1100, 600, 300});
    builder.box({-1260, 760, 90}, {220, 80, 40}, "#b89965", "LOBBY LIFT", false);
    builder.action("balcony-down", "Lobby lift · E / A", {-1260, 610, 220}, SpatialActionKind::Transfer, {-1100, 160, -250});
    for (int index = 0; index < qMin(11, int(document.items.size())); ++index) {
        const float side = index % 2 == 0 ? -1 : 1;
        SpatialDisplay poster;
        poster.id = document.items[index].id;
        poster.application.name = document.items[index].title;
        poster.freestanding = true;
        poster.center = {side * 1460, 240, float(-760 + index / 2 * 560)};
        poster.size = {340, 212};
        poster.yaw = -side * 90;
        builder.room.displays.append(poster);
    }
    builder.gateway("search", "CITY DIRECTORY", 2800, -1100, "search");
    return builder.room;
}

SpatialInterior SpatialCivicWorld::city(const SpatialWebDocument &document)
{
    Builder builder;
    // Arrive facing the first result, rather than the empty gap between shops.
    builder.room.spawn = {-4000, -2700};
    builder.street();
    // Stable slots keep the search ranking legible. Every result has its source
    // title and poster at street level, plus a real doorway into that website.
    for (int index = 0; index < qMin(12, int(document.items.size())); ++index) {
        const auto &item = document.items[index];
        const float x = -4000 + (index % 6) * 1600;
        const float z = index < 6 ? -1000 : 1500;
        builder.box({x - 430, 320, z}, {360, 640, 460}, "#4f5266");
        builder.box({x + 430, 320, z}, {360, 640, 460}, "#4f5266");
        builder.box({x, 600, z}, {1220, 100, 460}, "#bb9b69", item.detail.isEmpty() ? item.url.host() : item.detail);
        const float height = 700 + (index % 3) * 260;
        builder.box({x, 650 + height / 2, z + 200}, {1300, height, 900},
            index % 2 ? "#66514c" : "#394858");
        builder.windows(x, z - 260, 1100, 720, 1);
        builder.frontage(x, 520, z - 245, 1220, 22, "#ffe5af");
        builder.frontage(x, 650 + height - 40, z - 260, 1320, 45, "#d1b78b");
        builder.frontage(x - 430, 60, z - 245, 300, 400, "#31465a");
        SpatialDisplay display;
        display.id = item.id;
        display.application.name = item.title;
        display.freestanding = true;
        display.center = {x + 430, 240, z - 245};
        display.size = {320, 200};
        display.portal = !item.url.isEmpty();
        display.portalCenter = {x, 160, z - 260};
        display.portalApproachDistance = 160;
        builder.room.displays.append(display);
    }
    builder.action("search", "Search the web · E / A", {0, 160, -2900}, SpatialActionKind::Destination, {}, "search");
    return builder.room;
}

