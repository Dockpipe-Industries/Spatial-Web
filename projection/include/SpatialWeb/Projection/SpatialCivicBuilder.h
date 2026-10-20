#pragma once

#include <spatialpipe/SpatialWorkspace.h>
#include <spatialpipe/scene/FurnishingCatalog.h>
#include <algorithm>
#include <cmath>

// Neutral bounded scene composition shared by civic spaces and native consumers.
class SpatialCivicBuilder {
public:
    SpatialInterior room;

    SpatialCivicBuilder()
    {
        room.size = {10000, 9200};
        room.height = 4200;
        room.outdoors = true;
        room.naturalGround = true;
        room.floorColor = QColor("#252b38");
        room.skyColor = QColor("#54677e");
        room.exit = {0, -4500};
    }

    void box(QVector3D position, QVector3D size, const char *color, QString label = {}, bool collision = true)
    {
        room.solids.append({position, size, QColor(color), collision, SpatialSolidShape::Box, label});
    }

    void action(QString id, QString title, QVector3D center, SpatialActionKind kind,
        QVector3D destination = {}, QString link = {})
    {
        room.actions.append({id, title, center, {120, 150, 120}, kind, destination, 0, link});
    }

    void seat(QString id, float x, float z, float floor = 0)
    {
        box({x, floor + 45, z}, {140, 90, 140}, "#693048");
        box({x, floor + 110, z - 60}, {140, 110, 30}, "#943f59");
        action(id, "Sit · E / A", {x, floor + 190, z}, SpatialActionKind::Seat, {x, floor + 190, z});
    }

    // Architectural detail shares one mesh per material. It costs no collision
    // objects, textures, light passes or work after the room is constructed.
    void panel(QVector<QVector3D> corners, const char *color)
    {
        const QColor material(color);
        auto mesh = std::find_if(room.meshes.begin(), room.meshes.end(),
            [&](const SpatialMesh &value) { return value.color == material; });
        if (mesh == room.meshes.end()) {
            room.meshes.append({{}, {}, material});
            mesh = room.meshes.end() - 1;
        }
        const int first = mesh->vertices.size();
        mesh->vertices += corners;
        mesh->faces.append({first, first + 1, first + 2, first + 3});
    }

    void frontage(float x, float y, float z, float width, float height, const char *color)
    {
        panel({{x - width / 2, y, z}, {x + width / 2, y, z},
            {x + width / 2, y + height, z}, {x - width / 2, y + height, z}}, color);
    }

    void gateway(QString id, QString title, float x, float z, QString link)
    {
        // A street-sized building with an open central entrance, layered cornice,
        // illuminated marquee and tall glazing, rather than a freestanding gate.
        box({x - 480, 550, z + 220}, {500, 1100, 620}, "#8b7b70");
        box({x + 480, 550, z + 220}, {500, 1100, 620}, "#8b7b70");
        box({x, 950, z + 220}, {1460, 300, 620}, "#4c5363");
        box({x, 590, z - 160}, {1500, 140, 330}, "#b89559", title);
        frontage(x, 1060, z - 95, 1500, 45, "#d1b78b");
        frontage(x, 520, z - 330, 1460, 22, "#ffe5af");
        frontage(x, 645, z - 330, 1460, 16, "#ffe5af");
        for (float side : {-1.f, 1.f}) {
            frontage(x + side * 480, 60, z - 95, 360, 400, "#d1b78b");
            frontage(x + side * 480, 90, z - 98, 300, 330, "#31465a");
            frontage(x + side * 675, 680, z - 95, 30, 350, "#ffe5af");
        }
        windows(x, z - 95, 1000, 740, 1);
        panel({{x - 680, 11, z - 800}, {x + 680, 11, z - 800},
            {x + 500, 11, z - 250}, {x - 500, 11, z - 250}}, "#675347");
        frontage(x, 0, z + 500, 460, 800, "#31465a");
        frontage(x, 80, z + 495, 360, 580, "#d1b78b");
        action(id, title + " · Enter", {x, 170, z - 100}, SpatialActionKind::Destination, {}, link);
    }

    void windows(float x, float z, float width, float firstHeight, int rows = 2)
    {
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < 3; ++column)
                frontage(x + (column - 1) * width / 3, firstHeight + row * 260,
                    z, 120, 160, "#d1b78b");
        }
    }

    void furnishing(const QString &type, QVector3D position, float yaw = 0)
    {
        FurnishingObject object;
        object.type = type;
        object.position = position;
        object.yaw = yaw;
        room.solids += FurnishingCatalog::parts(object);
    }

    void street()
    {
        // The boulevard crosses the lobby doors. Pavements and markings are
        // shallow non-blocking geometry; distant buildings are six reusable boxes.
        box({0, 4, -2000}, {9800, 8, 1400}, "#171d29", {}, false);
        box({0, 10, -1200}, {9800, 20, 200}, "#89909b", {}, false);
        box({0, 10, -2800}, {9800, 20, 200}, "#89909b", {}, false);
        for (int index = 0; index < 6; ++index) {
            const float x = -4000 + index * 1600;
            const float height = 1200 + (index % 3) * 460;
            box({x, height / 2, -3650}, {1300, height, 900}, index % 2 ? "#354359" : "#4c4049");
            windows(x, -3190, 1000, 600);
            frontage(x, height - 60, -3190, 1320, 55, "#d1b78b");
            frontage(x, 60, -3190, 950, 300, "#31465a");
            box({x, 12, -2000}, {600, 5, 18}, "#d9b874", {}, false);
        }
        for (const float x : {-1900.f, 1900.f}) {
            box({x, 240, -1220}, {24, 480, 24}, "#111b29");
            box({x, 480, -1220}, {100, 35, 100}, "#ffe6ae", {}, false);
        }
        box({1800, 24, -2000}, {420, 48, 420}, "#263b50", "DRONE PAD");
        box({1800, 95, -2000}, {190, 90, 160}, "#84c4ce");
        action("drone", "Board drone · E / A", {1800, 190, -2000}, SpatialActionKind::Drone, {1800, 320, -2000});
    }

    void shell(const QString &title, bool darkLobby = false)
    {
        // Auditorium and lobby share a wide, uninterrupted centre aisle.
        const float depth = darkLobby ? 4100 : 5400;
        const float center = -1100 + depth / 2;
        const char *wall = darkLobby ? "#22292f" : "#352532";
        box({-1500, 800, center}, {60, 1600, depth}, wall);
        box({1500, 800, center}, {60, 1600, depth}, wall);
        box({0, 800, -1100 + depth}, {3000, 1600, 60}, darkLobby ? "#111519" : "#241c2a");
        box({0, 1620, center}, {3060, 40, depth}, "#17202e");
        for (float side : {-1.f, 1.f}) {
            box({side * 950, 330, -1100}, {1100, 660, 70}, "#354459");
            box({side * 950, 300, 0}, {1100, 600, 60}, wall);
            box({side * 1100, 60, -640}, {450, 120, 180}, "#425568");
            box({side * 1100, 150, -720}, {450, 180, 40}, "#617083");
        }
        box({0, 730, -1200}, {3060, 140, 400}, "#b89559", title);
        frontage(0, 650, -1405, 3060, 24, "#ffe5af");
        frontage(0, 790, -1405, 3060, 18, "#ffe5af");
        frontage(0, 1510, -1135, 3060, 65, "#d1b78b");
        for (float side : {-1.f, 1.f}) {
            frontage(side * 950, 90, -1138, 950, 480, "#31465a");
            frontage(side * 950, 950, -1135, 900, 360, "#d1b78b");
            frontage(side * 1450, 70, -1140, 35, 1400, "#ffe5af");
            // Gold-edged wall bays and luminous coves frame the long interior.
            panel({{side * 1460, 1370, 100}, {side * 1460, 1370, 3900},
                {side * 1460, 1400, 3900}, {side * 1460, 1400, 100}}, "#ffe5af");
            for (float z : {1300.f, 2700.f}) {
                panel({{side * 1460, 450, z}, {side * 1460, 450, z + 650},
                    {side * 1460, 1250, z + 650}, {side * 1460, 1250, z}}, "#675347");
            }
        }
        panel({{-350, 13, -1400}, {350, 13, -1400}, {350, 13, 3900}, {-350, 13, 3900}}, "#675347");
        box({0, 4, -550}, {2950, 8, 1050}, "#737b87", {}, false);
        box({0, 12, 1500}, {340, 6, 3000}, darkLobby ? "#213a31" : "#783a47", {}, false);
        box({0, 1060, -550}, {520, 60, 380}, darkLobby ? "#a4eccd" : "#ffdf9c", {}, false);
    }
};
