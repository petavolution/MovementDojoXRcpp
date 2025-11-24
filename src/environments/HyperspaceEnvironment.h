#pragma once
/**
 * HyperspaceEnvironment.h - Spaceship Hyperspace Training Environment
 *
 * A sci-fi environment with:
 * - Small cockpit platform for the player
 * - Animated hyperspace streaks rushing past
 * - Dark space backdrop
 * - Cockpit console elements
 */

#include "Environment.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include <cmath>
#include <random>

namespace lst {

class HyperspaceEnvironment : public Environment {
public:
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------
    static constexpr float PLATFORM_RADIUS = 2.0f;      // Cockpit platform radius
    static constexpr float PLATFORM_HEIGHT = 0.1f;      // Platform thickness
    static constexpr int NUM_STREAKS = 50;              // Number of hyperspace streaks
    static constexpr float STREAK_SPAWN_RADIUS = 15.0f; // Spawn distance from center
    static constexpr float STREAK_LENGTH = 3.0f;        // Base streak length
    static constexpr float STREAK_SPEED = 20.0f;        // Streak movement speed
    static constexpr float STREAK_SPAWN_Z = 40.0f;      // Far spawn distance
    static constexpr float STREAK_DESPAWN_Z = -10.0f;   // Despawn distance behind player

    // -------------------------------------------------------------------------
    // Environment Interface
    // -------------------------------------------------------------------------

    const char* getName() const override {
        return "Hyperspace";
    }

    const char* getDescription() const override {
        return "A spaceship cockpit traveling through hyperspace";
    }

    bool setup(Engine* engine) override {
        if (!engine) {
            LOG_ERROR(LOG_TAG_ENV) << "Hyperspace: Engine is null";
            return false;
        }

        LOG_INFO(LOG_TAG_ENV) << "Building Hyperspace environment...";

        m_engine = engine;
        m_time = 0.0f;

        // Initialize random generator
        m_rng.seed(42);  // Fixed seed for reproducibility

        // Build the environment
        buildCockpit();
        buildConsole();
        buildSpaceBackdrop();
        buildHyperspaceStreaks();

        LOG_INFO(LOG_TAG_ENV) << "Hyperspace environment built: "
                               << m_objectNames.size() << " objects";
        return true;
    }

    void cleanup(Engine* engine) override {
        LOG_INFO(LOG_TAG_ENV) << "Cleaning up Hyperspace environment...";
        removeAllTrackedObjects(engine);
        m_engine = nullptr;
    }

    void update(float deltaTime) override {
        m_time += deltaTime;

        if (!m_engine) return;

        // Animate hyperspace streaks
        for (int i = 0; i < NUM_STREAKS; i++) {
            std::string streakName = "Hyperspace_Streak" + std::to_string(i);
            if (auto* streak = m_engine->getSceneObject(streakName)) {
                // Move streak toward player
                streak->transform.position.z -= STREAK_SPEED * deltaTime;

                // Respawn if past player
                if (streak->transform.position.z < STREAK_DESPAWN_Z) {
                    respawnStreak(streak, i);
                }

                // Stretch based on speed (motion blur effect)
                float stretch = 1.0f + (STREAK_SPEED * deltaTime) * 2.0f;
                streak->transform.scale.z = m_streakBaseLength[i] * stretch;
            }
        }

        // Subtle console glow pulsing
        if (auto* console = m_engine->getSceneObject("Hyperspace_ConsoleGlow")) {
            float pulse = 0.5f + 0.3f * std::sin(m_time * 2.0f);
            console->material.emissive = pulse;
        }
    }

private:
    Engine* m_engine = nullptr;
    float m_time = 0.0f;
    std::mt19937 m_rng;
    float m_streakBaseLength[NUM_STREAKS] = {0};

    // -------------------------------------------------------------------------
    // Build Functions
    // -------------------------------------------------------------------------

    void buildCockpit() {
        // Main platform (hexagonal approximation)
        BoxParams platformParams;
        platformParams.width = PLATFORM_RADIUS * 2.0f;
        platformParams.height = PLATFORM_HEIGHT;
        platformParams.depth = PLATFORM_RADIUS * 2.0f;
        platformParams.center = Vec3(0, -0.05f, 0);

        SceneObject platform = SceneObjectBuilder("Hyperspace_Platform")
            .mesh(ProceduralMesh::createBox(platformParams))
            .material(ProceduralMaterial::metal())
            .build();

        platform.material.baseColor = Color(0.15f, 0.15f, 0.18f);
        platform.material.metallic = 0.7f;
        platform.material.roughness = 0.4f;

        addTrackedObject(m_engine, platform);

        // Platform edge lighting (emissive ring)
        RingParams edgeParams;
        edgeParams.innerRadius = PLATFORM_RADIUS - 0.1f;
        edgeParams.outerRadius = PLATFORM_RADIUS;
        edgeParams.segments = 32;
        edgeParams.center = Vec3(0, 0.01f, 0);

        SceneObject edge = SceneObjectBuilder("Hyperspace_PlatformEdge")
            .mesh(ProceduralMesh::createRing(edgeParams))
            .material(ProceduralMaterial::createEmissive(
                Color(0.3f, 0.5f, 1.0f), 0.8f))  // Blue glow
            .build();

        addTrackedObject(m_engine, edge);

        // Cockpit frame posts (4 corners)
        float postOffset = PLATFORM_RADIUS - 0.3f;
        float postHeight = 2.5f;
        Vec3 postPositions[] = {
            Vec3(-postOffset, postHeight / 2, -postOffset),
            Vec3( postOffset, postHeight / 2, -postOffset),
            Vec3(-postOffset, postHeight / 2,  postOffset * 0.3f),
            Vec3( postOffset, postHeight / 2,  postOffset * 0.3f)
        };

        for (int i = 0; i < 4; i++) {
            BoxParams postParams;
            postParams.width = 0.08f;
            postParams.height = postHeight;
            postParams.depth = 0.08f;
            postParams.center = postPositions[i];

            SceneObject post = SceneObjectBuilder("Hyperspace_Post" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(postParams))
                .material(ProceduralMaterial::metal())
                .build();

            post.material.baseColor = Color(0.2f, 0.2f, 0.25f);

            addTrackedObject(m_engine, post);
        }

        LOG_DEBUG(LOG_TAG_ENV) << "  Cockpit built";
    }

    void buildConsole() {
        // Main console in front of player
        float consoleZ = -PLATFORM_RADIUS + 0.5f;

        // Console base
        BoxParams consoleParams;
        consoleParams.width = 2.5f;
        consoleParams.height = 0.8f;
        consoleParams.depth = 0.6f;
        consoleParams.center = Vec3(0, 0.4f, consoleZ);

        SceneObject console = SceneObjectBuilder("Hyperspace_Console")
            .mesh(ProceduralMesh::createBox(consoleParams))
            .material(ProceduralMaterial::metal())
            .build();

        console.material.baseColor = Color(0.12f, 0.12f, 0.15f);

        addTrackedObject(m_engine, console);

        // Console screen
        BoxParams screenParams;
        screenParams.width = 2.0f;
        screenParams.height = 0.6f;
        screenParams.depth = 0.02f;
        screenParams.center = Vec3(0, 0.9f, consoleZ - 0.2f);

        SceneObject screen = SceneObjectBuilder("Hyperspace_Screen")
            .mesh(ProceduralMesh::createBox(screenParams))
            .material(ProceduralMaterial::createHolographic(
                Color(0.1f, 0.3f, 0.8f), 0.5f))
            .build();

        addTrackedObject(m_engine, screen);

        // Glowing console elements
        BoxParams glowParams;
        glowParams.width = 1.8f;
        glowParams.height = 0.05f;
        glowParams.depth = 0.4f;
        glowParams.center = Vec3(0, 0.85f, consoleZ + 0.1f);

        SceneObject glow = SceneObjectBuilder("Hyperspace_ConsoleGlow")
            .mesh(ProceduralMesh::createBox(glowParams))
            .material(ProceduralMaterial::createEmissive(
                Color(0.2f, 0.6f, 1.0f), 0.5f))
            .build();

        addTrackedObject(m_engine, glow);

        // Side panels
        for (int i = 0; i < 2; i++) {
            float x = (i == 0) ? -1.0f : 1.0f;

            BoxParams panelParams;
            panelParams.width = 0.5f;
            panelParams.height = 1.0f;
            panelParams.depth = 0.3f;
            panelParams.center = Vec3(x, 0.5f, consoleZ + 0.3f);

            SceneObject panel = SceneObjectBuilder("Hyperspace_SidePanel" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(panelParams))
                .material(ProceduralMaterial::metal())
                .build();

            panel.material.baseColor = Color(0.15f, 0.15f, 0.18f);

            addTrackedObject(m_engine, panel);
        }

        LOG_DEBUG(LOG_TAG_ENV) << "  Console built";
    }

    void buildSpaceBackdrop() {
        // Large dark sphere/box surrounding the scene
        BoxParams backdropParams;
        backdropParams.width = 100.0f;
        backdropParams.height = 100.0f;
        backdropParams.depth = 100.0f;
        backdropParams.center = Vec3(0, 0, 20.0f);

        SceneObject backdrop = SceneObjectBuilder("Hyperspace_Backdrop")
            .mesh(ProceduralMesh::createBox(backdropParams))
            .material(ProceduralMaterial::createUnlitColor(
                Color(0.02f, 0.02f, 0.05f)))  // Very dark blue-black
            .build();

        addTrackedObject(m_engine, backdrop);

        // "Horizon" glow plane far ahead
        GridPlaneParams horizonParams;
        horizonParams.width = 80.0f;
        horizonParams.depth = 40.0f;
        horizonParams.subdivisionsX = 1;
        horizonParams.subdivisionsZ = 1;
        horizonParams.center = Vec3(0, 0, 50.0f);
        horizonParams.normal = Vec3(0, 0, -1);

        SceneObject horizon = SceneObjectBuilder("Hyperspace_Horizon")
            .mesh(ProceduralMesh::createGridPlane(horizonParams))
            .material(ProceduralMaterial::createEmissive(
                Color(0.4f, 0.6f, 1.0f), 0.3f))  // Blue hyperspace glow
            .build();

        addTrackedObject(m_engine, horizon);

        LOG_DEBUG(LOG_TAG_ENV) << "  Space backdrop built";
    }

    void buildHyperspaceStreaks() {
        std::uniform_real_distribution<float> distRadius(2.0f, STREAK_SPAWN_RADIUS);
        std::uniform_real_distribution<float> distAngle(0.0f, 6.283185f);
        std::uniform_real_distribution<float> distZ(0.0f, STREAK_SPAWN_Z);
        std::uniform_real_distribution<float> distLength(STREAK_LENGTH * 0.5f, STREAK_LENGTH * 1.5f);
        std::uniform_real_distribution<float> distBrightness(0.5f, 1.0f);

        for (int i = 0; i < NUM_STREAKS; i++) {
            float radius = distRadius(m_rng);
            float angle = distAngle(m_rng);
            float z = distZ(m_rng);
            float length = distLength(m_rng);
            float brightness = distBrightness(m_rng);

            m_streakBaseLength[i] = length;

            float x = std::cos(angle) * radius;
            float y = std::sin(angle) * radius;

            LineStreakParams streakParams;
            streakParams.length = length;
            streakParams.width = 0.03f + brightness * 0.02f;
            streakParams.start = Vec3(x, y, z);
            streakParams.direction = Vec3(0, 0, -1);

            SceneObject streak = SceneObjectBuilder("Hyperspace_Streak" + std::to_string(i))
                .mesh(ProceduralMesh::createLineStreak(streakParams))
                .material(ProceduralMaterial::hyperspaceStreak())
                .build();

            // Vary color slightly
            float hue = 0.55f + (i % 10) * 0.02f;  // Blue to cyan range
            streak.material.baseColor = Color(
                0.7f + (1.0f - hue) * 0.3f,
                0.8f + hue * 0.2f,
                1.0f
            );
            streak.material.baseColor.a = brightness;
            streak.material.emissive = 0.8f + brightness * 0.4f;

            addTrackedObject(m_engine, streak);
        }

        LOG_DEBUG(LOG_TAG_ENV) << "  Hyperspace streaks built: " << NUM_STREAKS;
    }

    void respawnStreak(SceneObject* streak, int /*index*/) {
        std::uniform_real_distribution<float> distRadius(2.0f, STREAK_SPAWN_RADIUS);
        std::uniform_real_distribution<float> distAngle(0.0f, 6.283185f);

        float radius = distRadius(m_rng);
        float angle = distAngle(m_rng);

        streak->transform.position.x = std::cos(angle) * radius;
        streak->transform.position.y = std::sin(angle) * radius;
        streak->transform.position.z = STREAK_SPAWN_Z;
    }
};

} // namespace lst
