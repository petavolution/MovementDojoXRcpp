/**
 * Systems.h - Optional System Plugins for Engine
 *
 * Wraps existing subsystems to work with the unified Engine plugin architecture.
 * Each system can be attached independently as needed.
 *
 * Usage:
 *   engine.addSystem<PhysicsSystem>();
 *   engine.addSystem<AnalyticsSystem>();
 *   engine.addSystem<HapticsSystem>();
 *   engine.addSystem<ProgressionSystem>();
 */

#pragma once

#include "../core/Engine.h"
#include "../physics/PhysicsEngine.h"
#include "../analytics/MovementAnalytics.h"
#include "../haptics/HapticManager.h"
#include "../progression/ProgressionSystem.h"

namespace lst {

// =============================================================================
// Physics System Plugin
// =============================================================================

class PhysicsSystem : public System {
public:
    const char* getName() const override { return "Physics"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;
        return m_physics.initialize();
    }

    void onDetach() override {
        m_physics.shutdown();
    }

    void onUpdate(const FrameContext& ctx) override {
        m_physics.step(static_cast<float>(ctx.deltaTime));
    }

    // Direct access to physics engine
    PhysicsEngine& getPhysics() { return m_physics; }

    // Convenience methods
    void addStaticBody(const std::string& name, const Vec3& position, float radius) {
        m_physics.addStaticSphere(name, position, radius);
    }

    void addDynamicBody(const std::string& name, const Vec3& position, float mass) {
        m_physics.addDynamicSphere(name, position, 0.05f, mass);
    }

    bool raycast(const Vec3& origin, const Vec3& direction, float maxDist, Vec3& hitPoint) {
        return m_physics.raycast(origin, direction, maxDist, hitPoint);
    }

private:
    PhysicsEngine m_physics;
};

// =============================================================================
// Analytics System Plugin
// =============================================================================

class AnalyticsSystem : public System {
public:
    const char* getName() const override { return "Analytics"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;
        m_analytics.initialize();
        return true;
    }

    void onDetach() override {
        m_analytics.shutdown();
    }

    void onUpdate(const FrameContext& ctx) override {
        m_analytics.recordPose(
            ctx.leftController.position,
            ctx.rightController.position,
            ctx.headPose.position,
            ctx.totalTime
        );
    }

    void onSessionStart() override {
        m_analytics.startSession();
    }

    void onSessionEnd() override {
        m_analytics.endSession();
    }

    // Analytics queries
    float getCoverage() const { return m_analytics.getCoverage(); }
    int getUncommonAreasFound() const { return m_analytics.getUncommonAreasFound(); }

    void exportData(const std::string& path) {
        m_analytics.exportToFile(path);
    }

private:
    MovementAnalytics m_analytics;
};

// =============================================================================
// Haptics System Plugin
// =============================================================================

class HapticsSystem : public System {
public:
    const char* getName() const override { return "Haptics"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;
        // HapticManager is initialized without Input dependency in new architecture
        m_initialized = true;
        return true;
    }

    void onUpdate(const FrameContext& ctx) override {
        if (!m_initialized) return;

        // Process queued haptic events
        m_haptics.update(ctx.deltaTime);
    }

    // Haptic patterns
    void pulse(Hand hand, float intensity, float duration) {
        if (m_engine) {
            m_engine->triggerHaptic(hand, intensity, duration);
        }
    }

    void playPattern(const std::string& pattern, Hand hand) {
        m_haptics.playPattern(pattern, hand);
    }

    void playCollisionFeedback(float impactStrength) {
        float intensity = std::min(1.0f, impactStrength);
        pulse(Hand::Left, intensity, 0.1f);
        pulse(Hand::Right, intensity, 0.1f);
    }

private:
    HapticManager m_haptics;
    bool m_initialized = false;
};

// =============================================================================
// Progression System Plugin
// =============================================================================

class ProgressionSystemPlugin : public System {
public:
    const char* getName() const override { return "Progression"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;
        return true;
    }

    void onSessionStart() override {
        m_progression.startSession(m_playerName);
    }

    void onSessionEnd() override {
        m_progression.endSession();
    }

    void onUpdate(const FrameContext& ctx) override {
        // Could trigger XP for exploration time
        m_sessionTime += ctx.deltaTime;

        // Award XP periodically for active sessions
        if (m_sessionTime - m_lastXPTime >= 60.0) {
            m_progression.awardXP(10, "Active exploration");
            m_lastXPTime = m_sessionTime;
        }
    }

    // Progression API
    void setPlayerName(const std::string& name) { m_playerName = name; }

    void awardXP(int amount, const std::string& reason) {
        m_progression.awardXP(amount, reason);
    }

    int getLevel() const { return m_progression.getLevel(); }
    int getTotalXP() const { return m_progression.getTotalXP(); }

    void setLevelUpCallback(std::function<void(int, const std::string&)> callback) {
        m_progression.setLevelUpCallback(callback);
    }

    void setAchievementCallback(std::function<void(const std::string&, const std::string&)> callback) {
        m_progression.setAchievementCallback(callback);
    }

    bool saveProfile(const std::string& path) {
        return m_progression.saveProfile(path);
    }

    bool loadProfile(const std::string& path) {
        return m_progression.loadProfile(path);
    }

private:
    ProgressionSystem m_progression;
    std::string m_playerName = "Player";
    double m_sessionTime = 0;
    double m_lastXPTime = 0;
};

// =============================================================================
// Ground Plane System Plugin
// =============================================================================

class GroundPlaneSystem : public System {
public:
    const char* getName() const override { return "GroundPlane"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;

        SceneObject ground;
        ground.name = "Ground";
        ground.mesh = Mesh::createPlane(10.0f, 10.0f);
        ground.material.baseColor = Color(0.3f, 0.3f, 0.35f);
        ground.material.roughness = 0.8f;
        ground.isStatic = true;
        engine->addSceneObject(ground);

        return true;
    }

    void onRender(const FrameContext& ctx) override {
        // Draw grid lines on ground
        const float gridSize = 10.0f;
        const float gridStep = 1.0f;
        Color gridColor(0.4f, 0.4f, 0.45f, 0.5f);

        for (float x = -gridSize/2; x <= gridSize/2; x += gridStep) {
            m_engine->drawLine(
                Vec3(x, 0.001f, -gridSize/2),
                Vec3(x, 0.001f, gridSize/2),
                gridColor, 1.0f
            );
        }
        for (float z = -gridSize/2; z <= gridSize/2; z += gridStep) {
            m_engine->drawLine(
                Vec3(-gridSize/2, 0.001f, z),
                Vec3(gridSize/2, 0.001f, z),
                gridColor, 1.0f
            );
        }
    }
};

// =============================================================================
// Movement Trail System Plugin
// =============================================================================

class MovementTrailSystem : public System {
public:
    const char* getName() const override { return "MovementTrail"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;
        return true;
    }

    void onUpdate(const FrameContext& ctx) override {
        // Record trail points
        if (ctx.leftController.isTracked) {
            addTrailPoint(m_leftTrail, ctx.leftController.position, ctx.totalTime);
        }
        if (ctx.rightController.isTracked) {
            addTrailPoint(m_rightTrail, ctx.rightController.position, ctx.totalTime);
        }
    }

    void onRender(const FrameContext& ctx) override {
        drawTrail(m_leftTrail, Color(0.2f, 0.8f, 0.2f, 0.6f), ctx.totalTime);
        drawTrail(m_rightTrail, Color(0.2f, 0.2f, 0.8f, 0.6f), ctx.totalTime);
    }

    void setTrailDuration(float seconds) { m_trailDuration = seconds; }

private:
    struct TrailPoint {
        Vec3 position;
        double timestamp;
    };

    std::vector<TrailPoint> m_leftTrail;
    std::vector<TrailPoint> m_rightTrail;
    float m_trailDuration = 2.0f;

    void addTrailPoint(std::vector<TrailPoint>& trail, const Vec3& pos, double time) {
        trail.push_back({pos, time});

        // Remove old points
        while (!trail.empty() && time - trail.front().timestamp > m_trailDuration) {
            trail.erase(trail.begin());
        }
    }

    void drawTrail(const std::vector<TrailPoint>& trail, const Color& baseColor, double currentTime) {
        for (size_t i = 1; i < trail.size(); i++) {
            float age = static_cast<float>(currentTime - trail[i].timestamp);
            float alpha = 1.0f - (age / m_trailDuration);
            Color color = baseColor;
            color.a = alpha * baseColor.a;

            m_engine->drawLine(trail[i-1].position, trail[i].position, color, 3.0f);
        }
    }
};

} // namespace lst
