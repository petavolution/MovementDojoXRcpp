#pragma once
/**
 * DojoEnvironment.h - Kung-Fu Dojo Training Environment
 *
 * A traditional martial arts dojo with:
 * - Tatami mat floor (grid of quads)
 * - Wooden walls surrounding the player
 * - Wooden pillars and beams
 * - Weapon rack with decorative elements
 * - Floating holographic training screens
 */

#include "Environment.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include <cmath>

namespace lst {

class DojoEnvironment : public Environment {
public:
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------
    static constexpr float ROOM_WIDTH = 12.0f;        // Room width (X)
    static constexpr float ROOM_DEPTH = 12.0f;        // Room depth (Z)
    static constexpr float WALL_HEIGHT = 4.0f;        // Wall height
    static constexpr float WALL_THICKNESS = 0.15f;    // Wall thickness
    static constexpr float TATAMI_SIZE = 1.0f;        // Size of each tatami mat
    static constexpr float TATAMI_GAP = 0.02f;        // Gap between mats
    static constexpr float PILLAR_WIDTH = 0.3f;       // Pillar cross-section
    static constexpr float BEAM_WIDTH = 0.2f;         // Beam cross-section
    static constexpr float SCREEN_WIDTH = 1.5f;       // Holographic screen width
    static constexpr float SCREEN_HEIGHT = 1.0f;      // Holographic screen height
    static constexpr int NUM_SCREENS = 3;             // Number of floating screens

    // -------------------------------------------------------------------------
    // Environment Interface
    // -------------------------------------------------------------------------

    const char* getName() const override {
        return "Kung-Fu Dojo";
    }

    const char* getDescription() const override {
        return "A traditional martial arts training dojo";
    }

    bool setup(Engine* engine) override {
        if (!engine) {
            LOG_ERROR(LOG_TAG_ENV) << "Dojo: Engine is null";
            return false;
        }

        LOG_INFO(LOG_TAG_ENV) << "Building Dojo environment...";

        m_engine = engine;
        m_time = 0.0f;

        // Build the environment
        buildFloor();
        buildWalls();
        buildPillars();
        buildBeams();
        buildWeaponRack();
        buildHolographicScreens();

        LOG_INFO(LOG_TAG_ENV) << "Dojo environment built: "
                               << m_objectNames.size() << " objects";
        return true;
    }

    void cleanup(Engine* engine) override {
        LOG_INFO(LOG_TAG_ENV) << "Cleaning up Dojo environment...";
        removeAllTrackedObjects(engine);
        m_engine = nullptr;
    }

    void update(float deltaTime) override {
        m_time += deltaTime;

        // Animate holographic screens (gentle bob and pulse)
        if (m_engine) {
            for (int i = 0; i < NUM_SCREENS; i++) {
                std::string screenName = "Dojo_Screen" + std::to_string(i);
                if (auto* screen = m_engine->getSceneObject(screenName)) {
                    // Gentle vertical bob
                    float phase = m_time * 0.8f + i * 2.094f;  // 120 degree offset
                    float bob = std::sin(phase) * 0.05f;
                    screen->transform.position.y = m_screenBaseY[i] + bob;
                }
            }
        }
    }

private:
    Engine* m_engine = nullptr;
    float m_time = 0.0f;
    float m_screenBaseY[NUM_SCREENS] = {0};

    // -------------------------------------------------------------------------
    // Build Functions
    // -------------------------------------------------------------------------

    void buildFloor() {
        // Tatami mat floor - grid of individual mats
        int matsX = static_cast<int>(ROOM_WIDTH / (TATAMI_SIZE + TATAMI_GAP));
        int matsZ = static_cast<int>(ROOM_DEPTH / (TATAMI_SIZE + TATAMI_GAP));

        // Use a simpler approach - one large floor with tatami texture simulation
        // (Individual mats would create too many objects)
        GridPlaneParams floorParams;
        floorParams.width = ROOM_WIDTH - 0.5f;
        floorParams.depth = ROOM_DEPTH - 0.5f;
        floorParams.subdivisionsX = matsX;
        floorParams.subdivisionsZ = matsZ;
        floorParams.center = Vec3(0, 0, 0);
        floorParams.normal = Vec3(0, 1, 0);

        SceneObject floor = SceneObjectBuilder("Dojo_Floor")
            .mesh(ProceduralMesh::createGridPlane(floorParams))
            .material(ProceduralMaterial::wood())
            .build();

        // Tatami color (greenish-beige)
        floor.material.baseColor = Color(0.6f, 0.55f, 0.35f);
        floor.material.roughness = 0.9f;

        addTrackedObject(m_engine, floor);

        // Floor border/edge
        BoxParams borderParams;
        borderParams.width = ROOM_WIDTH;
        borderParams.height = 0.05f;
        borderParams.depth = ROOM_DEPTH;
        borderParams.center = Vec3(0, -0.025f, 0);

        SceneObject border = SceneObjectBuilder("Dojo_FloorBorder")
            .mesh(ProceduralMesh::createBox(borderParams))
            .material(ProceduralMaterial::wood())
            .build();

        border.material.baseColor = Color(0.3f, 0.2f, 0.1f);  // Dark wood border

        addTrackedObject(m_engine, border);

        LOG_DEBUG(LOG_TAG_ENV) << "  Floor built: " << ROOM_WIDTH << "x" << ROOM_DEPTH << "m";
    }

    void buildWalls() {
        float halfWidth = ROOM_WIDTH / 2.0f;
        float halfDepth = ROOM_DEPTH / 2.0f;
        float wallY = WALL_HEIGHT / 2.0f;

        // Wall configurations: position, width, depth
        struct WallConfig {
            const char* name;
            Vec3 position;
            float w, h, d;
        };

        WallConfig walls[] = {
            // Back wall (negative Z)
            {"Dojo_WallBack", Vec3(0, wallY, -halfDepth), ROOM_WIDTH, WALL_HEIGHT, WALL_THICKNESS},
            // Front wall (positive Z) - with gap for entrance
            {"Dojo_WallFrontL", Vec3(-halfWidth/2 - 1.0f, wallY, halfDepth), halfWidth - 2.0f, WALL_HEIGHT, WALL_THICKNESS},
            {"Dojo_WallFrontR", Vec3(halfWidth/2 + 1.0f, wallY, halfDepth), halfWidth - 2.0f, WALL_HEIGHT, WALL_THICKNESS},
            // Left wall
            {"Dojo_WallLeft", Vec3(-halfWidth, wallY, 0), WALL_THICKNESS, WALL_HEIGHT, ROOM_DEPTH},
            // Right wall
            {"Dojo_WallRight", Vec3(halfWidth, wallY, 0), WALL_THICKNESS, WALL_HEIGHT, ROOM_DEPTH}
        };

        for (const auto& cfg : walls) {
            BoxParams params;
            params.width = cfg.w;
            params.height = cfg.h;
            params.depth = cfg.d;
            params.center = cfg.position;

            SceneObject wall = SceneObjectBuilder(cfg.name)
                .mesh(ProceduralMesh::createBox(params))
                .material(ProceduralMaterial::wood())
                .build();

            // Wooden wall color
            wall.material.baseColor = Color(0.5f, 0.35f, 0.2f);
            wall.material.roughness = 0.7f;

            addTrackedObject(m_engine, wall);
        }

        // Entrance header (above the gap)
        BoxParams headerParams;
        headerParams.width = 4.0f;
        headerParams.height = 0.3f;
        headerParams.depth = WALL_THICKNESS;
        headerParams.center = Vec3(0, WALL_HEIGHT - 0.15f, halfDepth);

        SceneObject header = SceneObjectBuilder("Dojo_EntranceHeader")
            .mesh(ProceduralMesh::createBox(headerParams))
            .material(ProceduralMaterial::wood())
            .build();

        header.material.baseColor = Color(0.4f, 0.25f, 0.15f);

        addTrackedObject(m_engine, header);

        LOG_DEBUG(LOG_TAG_ENV) << "  Walls built: 4 walls with entrance";
    }

    void buildPillars() {
        float halfWidth = ROOM_WIDTH / 2.0f - 0.5f;
        float halfDepth = ROOM_DEPTH / 2.0f - 0.5f;
        float pillarY = WALL_HEIGHT / 2.0f;

        // Corner pillars
        Vec3 pillarPositions[] = {
            Vec3(-halfWidth, pillarY, -halfDepth),
            Vec3( halfWidth, pillarY, -halfDepth),
            Vec3(-halfWidth, pillarY,  halfDepth),
            Vec3( halfWidth, pillarY,  halfDepth)
        };

        for (int i = 0; i < 4; i++) {
            BoxParams params;
            params.width = PILLAR_WIDTH;
            params.height = WALL_HEIGHT;
            params.depth = PILLAR_WIDTH;
            params.center = pillarPositions[i];

            SceneObject pillar = SceneObjectBuilder("Dojo_Pillar" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(params))
                .material(ProceduralMaterial::wood())
                .build();

            // Darker wood for pillars
            pillar.material.baseColor = Color(0.35f, 0.22f, 0.12f);

            addTrackedObject(m_engine, pillar);
        }

        LOG_DEBUG(LOG_TAG_ENV) << "  Pillars built: 4 corner pillars";
    }

    void buildBeams() {
        float beamY = WALL_HEIGHT - BEAM_WIDTH / 2.0f;
        float halfWidth = ROOM_WIDTH / 2.0f - 0.5f;
        float halfDepth = ROOM_DEPTH / 2.0f - 0.5f;

        // Horizontal beams along X (front and back)
        for (int i = 0; i < 2; i++) {
            float z = (i == 0) ? -halfDepth : halfDepth;
            BoxParams params;
            params.width = ROOM_WIDTH - 1.0f;
            params.height = BEAM_WIDTH;
            params.depth = BEAM_WIDTH;
            params.center = Vec3(0, beamY, z);

            SceneObject beam = SceneObjectBuilder("Dojo_BeamX" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(params))
                .material(ProceduralMaterial::wood())
                .build();

            beam.material.baseColor = Color(0.4f, 0.28f, 0.15f);

            addTrackedObject(m_engine, beam);
        }

        // Horizontal beams along Z (left and right)
        for (int i = 0; i < 2; i++) {
            float x = (i == 0) ? -halfWidth : halfWidth;
            BoxParams params;
            params.width = BEAM_WIDTH;
            params.height = BEAM_WIDTH;
            params.depth = ROOM_DEPTH - 1.0f;
            params.center = Vec3(x, beamY, 0);

            SceneObject beam = SceneObjectBuilder("Dojo_BeamZ" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(params))
                .material(ProceduralMaterial::wood())
                .build();

            beam.material.baseColor = Color(0.4f, 0.28f, 0.15f);

            addTrackedObject(m_engine, beam);
        }

        // Center cross beam
        BoxParams crossParams;
        crossParams.width = ROOM_WIDTH - 1.0f;
        crossParams.height = BEAM_WIDTH * 0.8f;
        crossParams.depth = BEAM_WIDTH * 0.8f;
        crossParams.center = Vec3(0, beamY - BEAM_WIDTH * 0.5f, 0);

        SceneObject crossBeam = SceneObjectBuilder("Dojo_BeamCenter")
            .mesh(ProceduralMesh::createBox(crossParams))
            .material(ProceduralMaterial::wood())
            .build();

        crossBeam.material.baseColor = Color(0.4f, 0.28f, 0.15f);

        addTrackedObject(m_engine, crossBeam);

        LOG_DEBUG(LOG_TAG_ENV) << "  Beams built: 5 ceiling beams";
    }

    void buildWeaponRack() {
        // Position against back wall
        float rackZ = -ROOM_DEPTH / 2.0f + 0.5f;
        float rackX = -3.0f;  // Left side of back wall

        // Rack frame (vertical posts)
        for (int i = 0; i < 2; i++) {
            float x = rackX + i * 1.5f;
            BoxParams postParams;
            postParams.width = 0.08f;
            postParams.height = 2.0f;
            postParams.depth = 0.08f;
            postParams.center = Vec3(x, 1.0f, rackZ);

            SceneObject post = SceneObjectBuilder("Dojo_RackPost" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(postParams))
                .material(ProceduralMaterial::wood())
                .build();

            post.material.baseColor = Color(0.25f, 0.15f, 0.08f);

            addTrackedObject(m_engine, post);
        }

        // Horizontal rack bars
        float barYPositions[] = {0.6f, 1.2f, 1.8f};
        for (int i = 0; i < 3; i++) {
            BoxParams barParams;
            barParams.width = 1.5f;
            barParams.height = 0.05f;
            barParams.depth = 0.1f;
            barParams.center = Vec3(rackX + 0.75f, barYPositions[i], rackZ + 0.1f);

            SceneObject bar = SceneObjectBuilder("Dojo_RackBar" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(barParams))
                .material(ProceduralMaterial::wood())
                .build();

            bar.material.baseColor = Color(0.3f, 0.18f, 0.1f);

            addTrackedObject(m_engine, bar);
        }

        // Decorative weapon placeholders (simple cylinders representing staffs/swords)
        for (int i = 0; i < 3; i++) {
            float x = rackX + 0.3f + i * 0.5f;

            CylinderParams weaponParams;
            weaponParams.radius = 0.02f;
            weaponParams.height = 1.6f;
            weaponParams.segments = 8;
            weaponParams.center = Vec3(x, 1.0f, rackZ + 0.15f);
            weaponParams.axis = Vec3(0.1f, 1, 0);  // Slight tilt

            SceneObject weapon = SceneObjectBuilder("Dojo_Weapon" + std::to_string(i))
                .mesh(ProceduralMesh::createCylinder(weaponParams))
                .material(ProceduralMaterial::wood())
                .build();

            // Alternate colors for variety
            if (i == 1) {
                weapon.material.baseColor = Color(0.6f, 0.6f, 0.65f);  // Metal
                weapon.material.metallic = 0.8f;
            } else {
                weapon.material.baseColor = Color(0.5f, 0.35f, 0.2f);  // Wood
            }

            addTrackedObject(m_engine, weapon);
        }

        LOG_DEBUG(LOG_TAG_ENV) << "  Weapon rack built";
    }

    void buildHolographicScreens() {
        // Floating holographic training screens
        float screenY = 1.8f;  // Eye level
        float screenZ = 2.5f;  // In front of player

        // Screen positions in an arc
        float angles[] = {-30.0f, 0.0f, 30.0f};  // Degrees

        for (int i = 0; i < NUM_SCREENS; i++) {
            float angle = angles[i] * 3.14159f / 180.0f;
            float x = std::sin(angle) * screenZ;
            float z = -std::cos(angle) * screenZ;

            m_screenBaseY[i] = screenY + i * 0.1f;  // Slight height variation

            // Screen panel
            BoxParams screenParams;
            screenParams.width = SCREEN_WIDTH;
            screenParams.height = SCREEN_HEIGHT;
            screenParams.depth = 0.02f;
            screenParams.center = Vec3(x, m_screenBaseY[i], z);

            SceneObject screen = SceneObjectBuilder("Dojo_Screen" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(screenParams))
                .material(ProceduralMaterial::createHolographic(
                    Color(0.2f, 0.8f, 1.0f), 0.6f))  // Cyan holographic
                .build();

            addTrackedObject(m_engine, screen);

            // Screen frame/border
            BoxParams frameParams;
            frameParams.width = SCREEN_WIDTH + 0.1f;
            frameParams.height = SCREEN_HEIGHT + 0.1f;
            frameParams.depth = 0.01f;
            frameParams.center = Vec3(x, m_screenBaseY[i], z + 0.015f);

            SceneObject frame = SceneObjectBuilder("Dojo_ScreenFrame" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(frameParams))
                .material(ProceduralMaterial::createEmissive(
                    Color(0.1f, 0.4f, 0.5f), 0.3f))
                .build();

            addTrackedObject(m_engine, frame);
        }

        LOG_DEBUG(LOG_TAG_ENV) << "  Holographic screens built: " << NUM_SCREENS;
    }
};

} // namespace lst
