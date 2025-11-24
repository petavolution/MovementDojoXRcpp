#pragma once
/**
 * OceanPlatformEnvironment.h - Ocean Platform Training Environment
 *
 * A calm, meditative environment with:
 * - Raised platform for the player
 * - Infinite ocean surrounding the platform
 * - Simple sky dome
 */

#include "Environment.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include <cmath>

namespace lst {

class OceanPlatformEnvironment : public Environment {
public:
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------
    static constexpr float PLATFORM_RADIUS = 3.0f;      // Platform radius
    static constexpr float PLATFORM_HEIGHT = 0.3f;      // Platform thickness
    static constexpr float PLATFORM_ELEVATION = 0.5f;   // Height above water
    static constexpr float RAILING_HEIGHT = 0.8f;       // Railing height
    static constexpr float RAILING_THICKNESS = 0.05f;   // Railing thickness
    static constexpr float OCEAN_SIZE = 100.0f;         // Ocean plane size
    static constexpr float WATER_LEVEL = 0.0f;          // Water surface Y position
    static constexpr float SKY_RADIUS = 50.0f;          // Sky dome radius
    static constexpr int OCEAN_SUBDIVISIONS = 20;       // Ocean grid subdivisions

    // -------------------------------------------------------------------------
    // Environment Interface
    // -------------------------------------------------------------------------

    const char* getName() const override {
        return "Ocean Platform";
    }

    const char* getDescription() const override {
        return "A raised platform surrounded by calm ocean waters";
    }

    bool setup(Engine* engine) override {
        if (!engine) {
            LOG_ERROR(LOG_TAG_ENV) << "OceanPlatform: Engine is null";
            return false;
        }

        LOG_INFO(LOG_TAG_ENV) << "Building Ocean Platform environment...";

        m_engine = engine;
        m_time = 0.0f;

        // Build the environment
        buildPlatform();
        buildRailing();
        buildOcean();
        buildSkyDome();

        LOG_INFO(LOG_TAG_ENV) << "Ocean Platform environment built: "
                               << m_objectNames.size() << " objects";
        return true;
    }

    void cleanup(Engine* engine) override {
        LOG_INFO(LOG_TAG_ENV) << "Cleaning up Ocean Platform environment...";
        removeAllTrackedObjects(engine);
        m_engine = nullptr;
    }

    void update(float deltaTime) override {
        m_time += deltaTime;

        // Animate water (simple vertical oscillation for now)
        if (m_engine) {
            if (auto* ocean = m_engine->getSceneObject("OceanPlatform_Ocean")) {
                // Subtle wave motion
                float wave = std::sin(m_time * 0.5f) * 0.02f;
                ocean->transform.position.y = WATER_LEVEL + wave;
            }
        }
    }

private:
    Engine* m_engine = nullptr;
    float m_time = 0.0f;

    // -------------------------------------------------------------------------
    // Build Functions
    // -------------------------------------------------------------------------

    void buildPlatform() {
        // Main platform (circular approximation using octagon)
        // For simplicity, use a scaled cube as base
        BoxParams platformParams;
        platformParams.width = PLATFORM_RADIUS * 2.0f;
        platformParams.height = PLATFORM_HEIGHT;
        platformParams.depth = PLATFORM_RADIUS * 2.0f;
        platformParams.center = Vec3(0, PLATFORM_ELEVATION, 0);

        SceneObject platform = SceneObjectBuilder("OceanPlatform_Platform")
            .mesh(ProceduralMesh::createBox(platformParams))
            .material(ProceduralMaterial::wood())
            .build();
        platform.material.baseColor = Color(0.4f, 0.3f, 0.2f);  // Darker wood

        addTrackedObject(m_engine, platform);

        // Platform edge accent
        BoxParams edgeParams;
        edgeParams.width = PLATFORM_RADIUS * 2.0f + 0.1f;
        edgeParams.height = 0.05f;
        edgeParams.depth = PLATFORM_RADIUS * 2.0f + 0.1f;
        edgeParams.center = Vec3(0, PLATFORM_ELEVATION + PLATFORM_HEIGHT / 2 + 0.025f, 0);

        SceneObject edge = SceneObjectBuilder("OceanPlatform_Edge")
            .mesh(ProceduralMesh::createBox(edgeParams))
            .material(ProceduralMaterial::metal())
            .build();

        addTrackedObject(m_engine, edge);

        LOG_DEBUG(LOG_TAG_ENV) << "  Platform built: " << PLATFORM_RADIUS * 2 << "m diameter";
    }

    void buildRailing() {
        // Build railing posts at corners
        float postOffset = PLATFORM_RADIUS - 0.1f;
        float postY = PLATFORM_ELEVATION + PLATFORM_HEIGHT / 2 + RAILING_HEIGHT / 2;

        const Vec3 postPositions[] = {
            Vec3(-postOffset, postY, -postOffset),
            Vec3( postOffset, postY, -postOffset),
            Vec3( postOffset, postY,  postOffset),
            Vec3(-postOffset, postY,  postOffset)
        };

        for (int i = 0; i < 4; i++) {
            BoxParams postParams;
            postParams.width = RAILING_THICKNESS;
            postParams.height = RAILING_HEIGHT;
            postParams.depth = RAILING_THICKNESS;
            postParams.center = postPositions[i];

            SceneObject post = SceneObjectBuilder("OceanPlatform_Post" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(postParams))
                .material(ProceduralMaterial::metal())
                .build();

            addTrackedObject(m_engine, post);
        }

        // Build horizontal rails
        float railY = PLATFORM_ELEVATION + PLATFORM_HEIGHT / 2 + RAILING_HEIGHT - 0.1f;
        float railLength = PLATFORM_RADIUS * 2 - 0.2f;

        // Front and back rails (along X)
        for (int i = 0; i < 2; i++) {
            float z = (i == 0) ? -postOffset : postOffset;
            BoxParams railParams;
            railParams.width = railLength;
            railParams.height = RAILING_THICKNESS;
            railParams.depth = RAILING_THICKNESS;
            railParams.center = Vec3(0, railY, z);

            SceneObject rail = SceneObjectBuilder("OceanPlatform_RailX" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(railParams))
                .material(ProceduralMaterial::metal())
                .build();

            addTrackedObject(m_engine, rail);
        }

        // Left and right rails (along Z)
        for (int i = 0; i < 2; i++) {
            float x = (i == 0) ? -postOffset : postOffset;
            BoxParams railParams;
            railParams.width = RAILING_THICKNESS;
            railParams.height = RAILING_THICKNESS;
            railParams.depth = railLength;
            railParams.center = Vec3(x, railY, 0);

            SceneObject rail = SceneObjectBuilder("OceanPlatform_RailZ" + std::to_string(i))
                .mesh(ProceduralMesh::createBox(railParams))
                .material(ProceduralMaterial::metal())
                .build();

            addTrackedObject(m_engine, rail);
        }

        LOG_DEBUG(LOG_TAG_ENV) << "  Railing built: 4 posts, 4 rails";
    }

    void buildOcean() {
        // Large subdivided plane for ocean
        GridPlaneParams oceanParams;
        oceanParams.width = OCEAN_SIZE;
        oceanParams.depth = OCEAN_SIZE;
        oceanParams.subdivisionsX = OCEAN_SUBDIVISIONS;
        oceanParams.subdivisionsZ = OCEAN_SUBDIVISIONS;
        oceanParams.center = Vec3(0, WATER_LEVEL, 0);
        oceanParams.normal = Vec3(0, 1, 0);

        SceneObject ocean = SceneObjectBuilder("OceanPlatform_Ocean")
            .mesh(ProceduralMesh::createGridPlane(oceanParams))
            .material(ProceduralMaterial::water())
            .build();

        // Adjust water color
        ocean.material.baseColor = Color(0.1f, 0.25f, 0.4f, 0.9f);
        ocean.material.emissive = 0.1f;

        addTrackedObject(m_engine, ocean);

        LOG_DEBUG(LOG_TAG_ENV) << "  Ocean built: " << OCEAN_SIZE << "x" << OCEAN_SIZE
                               << "m with " << OCEAN_SUBDIVISIONS << "x" << OCEAN_SUBDIVISIONS
                               << " subdivisions";
    }

    void buildSkyDome() {
        // Simple sky sphere (large inverted sphere)
        // Since we don't have an inverted sphere, use a large plane as horizon
        // and a sky color box surrounding everything

        // Horizon plane (very large, at water level, looking up)
        GridPlaneParams horizonParams;
        horizonParams.width = OCEAN_SIZE * 3;
        horizonParams.depth = OCEAN_SIZE * 3;
        horizonParams.subdivisionsX = 1;
        horizonParams.subdivisionsZ = 1;
        horizonParams.center = Vec3(0, -1.0f, 0);  // Below water
        horizonParams.normal = Vec3(0, 1, 0);

        SceneObject horizon = SceneObjectBuilder("OceanPlatform_Horizon")
            .mesh(ProceduralMesh::createGridPlane(horizonParams))
            .material(ProceduralMaterial::sky())
            .build();

        // Deep ocean color for horizon
        horizon.material.baseColor = Color(0.05f, 0.15f, 0.3f);
        horizon.material.emissive = 0.05f;

        addTrackedObject(m_engine, horizon);

        // Sky backdrop (large box above)
        BoxParams skyParams;
        skyParams.width = OCEAN_SIZE * 2;
        skyParams.height = 0.1f;
        skyParams.depth = OCEAN_SIZE * 2;
        skyParams.center = Vec3(0, SKY_RADIUS, 0);

        SceneObject sky = SceneObjectBuilder("OceanPlatform_Sky")
            .mesh(ProceduralMesh::createBox(skyParams))
            .material(ProceduralMaterial::sky())
            .build();

        sky.material.baseColor = Color(0.5f, 0.7f, 0.95f);
        sky.material.emissive = 0.2f;

        addTrackedObject(m_engine, sky);

        LOG_DEBUG(LOG_TAG_ENV) << "  Sky dome built";
    }
};

} // namespace lst
