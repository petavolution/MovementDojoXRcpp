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

    // Convenience methods using proper PhysicsBodyConfig API
    bool addStaticSphere(const std::string& name, const Vec3& position, float radius) {
        PhysicsBodyConfig config;
        config.name = name;
        config.bodyType = BodyType::Static;
        config.shapeType = ShapeType::Sphere;
        config.shapeSize = Vec3(radius, radius, radius);
        config.initialTransform.position = position;
        return m_physics.addBody(config);
    }

    bool addDynamicSphere(const std::string& name, const Vec3& position, float radius, float mass) {
        PhysicsBodyConfig config;
        config.name = name;
        config.bodyType = BodyType::Dynamic;
        config.shapeType = ShapeType::Sphere;
        config.shapeSize = Vec3(radius, radius, radius);
        config.mass = mass;
        config.initialTransform.position = position;
        config.enableCCD = true;  // Enable CCD for VR controller objects
        return m_physics.addBody(config);
    }

    bool addKinematicController(const std::string& name, const Vec3& position, float radius) {
        PhysicsBodyConfig config;
        config.name = name;
        config.bodyType = BodyType::Kinematic;
        config.shapeType = ShapeType::Sphere;
        config.shapeSize = Vec3(radius, radius, radius);
        config.initialTransform.position = position;
        config.enableCCD = true;
        return m_physics.addBody(config);
    }

    bool raycast(const Vec3& origin, const Vec3& direction, float maxDist,
                 std::string& hitBodyName, Vec3& hitPoint, Vec3& hitNormal) {
        PhysicsEngine::RayHit hit;
        if (m_physics.raycast(origin, direction, maxDist, hit)) {
            hitBodyName = hit.bodyName;
            hitPoint = hit.hitPoint;
            hitNormal = hit.hitNormal;
            return true;
        }
        return false;
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
        // Initialize with default config
        MovementAnalyticsConfig config;
        m_analytics.initialize(config);
        return true;
    }

    void onDetach() override {
        // End session if active
        if (m_analytics.isRecording()) {
            m_analytics.endSession();
        }
    }

    void onUpdate(const FrameContext& ctx) override {
        if (!m_analytics.isRecording()) return;

        // Record sample using proper API
        m_analytics.recordSample(ctx.deltaTime, ctx.headPose,
                                  ctx.leftController, ctx.rightController);
    }

    void onSessionStart() override {
        m_analytics.startSession();
    }

    void onSessionEnd() override {
        m_analytics.endSession();
    }

    // Analytics queries - using proper API names
    float getCoverage() const { return m_analytics.getMovementSpaceCoverage(); }

    bool isInUncommonPosition() const { return m_analytics.isInUncommonPosition(); }

    const SessionSummary& getSessionSummary() const {
        return m_analytics.getSessionSummary();
    }

    MovementPattern getCurrentPattern() const {
        return m_analytics.getCurrentPattern();
    }

    float getCurrentIntensity() const {
        return m_analytics.getCurrentIntensity();
    }

    Vec3 getSuggestedExplorationDirection() const {
        return m_analytics.getSuggestedExplorationDirection();
    }

    std::vector<Vec3> getUnexploredAreas() const {
        return m_analytics.getUnexploredAreas();
    }

    bool exportData(const std::string& path) {
        return m_analytics.exportSession(path);
    }

    bool exportCSV(const std::string& path) {
        return m_analytics.exportToCSV(path);
    }

    // Direct access for advanced usage
    MovementAnalytics& getAnalytics() { return m_analytics; }

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
        // Note: HapticManager requires Input* which we don't have in new architecture
        // We use Engine::triggerHaptic() directly instead
        return true;
    }

    void onUpdate(const FrameContext& ctx) override {
        (void)ctx;  // No update needed when using Engine directly
    }

    // Simple haptic triggers via Engine
    void pulse(Hand hand, float intensity = 0.5f, float duration = 0.1f) {
        if (m_engine) {
            m_engine->triggerHaptic(hand, intensity, duration);
        }
    }

    void buzz(Hand hand, float intensity = 0.3f) {
        pulse(hand, intensity, 0.05f);
    }

    // Predefined patterns using Engine haptics
    void playSuccessPattern(Hand hand) {
        pulse(hand, 0.6f, 0.1f);  // Quick pulse
    }

    void playFailurePattern(Hand hand) {
        pulse(hand, 0.3f, 0.2f);  // Longer buzz
    }

    void playCollisionFeedback(Hand hand, float impactStrength) {
        float intensity = std::min(1.0f, impactStrength);
        pulse(hand, intensity, 0.1f);
    }

    void playCollisionFeedbackBoth(float impactStrength) {
        float intensity = std::min(1.0f, impactStrength);
        pulse(Hand::Left, intensity, 0.1f);
        pulse(Hand::Right, intensity, 0.1f);
    }

    void playGuidanceNudge(Hand hand, float strength = 0.3f) {
        pulse(hand, strength, 0.15f);
    }
};

// =============================================================================
// Progression System Plugin
// =============================================================================

class ProgressionSystemPlugin : public System {
public:
    const char* getName() const override { return "Progression"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;
        // Create default profile if none loaded
        if (m_playerName.empty()) {
            m_playerName = "Player";
        }
        m_progression.createNewProfile(m_playerName);
        return true;
    }

    void onSessionStart() override {
        m_progression.startSession();
        m_sessionTime = 0;
        m_lastXPTime = 0;
    }

    void onSessionEnd() override {
        m_progression.endSession();
    }

    void onUpdate(const FrameContext& ctx) override {
        m_sessionTime += ctx.deltaTime;

        // Award XP periodically for active sessions (every 60 seconds)
        if (m_sessionTime - m_lastXPTime >= 60.0) {
            m_progression.addXP(10);  // Active exploration bonus
            m_lastXPTime = m_sessionTime;
        }
    }

    // Profile management
    void setPlayerName(const std::string& name) {
        m_playerName = name;
        m_progression.createNewProfile(name);
    }

    bool loadProfile(const std::string& path) {
        return m_progression.loadProfile(path);
    }

    bool saveProfile(const std::string& path) {
        return m_progression.saveProfile(path);
    }

    // XP and Level
    void addXP(int amount) {
        m_progression.addXP(amount);
    }

    int getLevel() const { return m_progression.getCurrentLevel(); }
    int getCurrentXP() const { return m_progression.getCurrentXP(); }
    float getLevelProgress() const { return m_progression.getLevelProgress(); }

    const LevelInfo& getCurrentLevelInfo() const {
        return m_progression.getCurrentLevelInfo();
    }

    // Stats access
    const PlayerStats& getStats() const { return m_progression.getStats(); }

    // Achievements
    const std::vector<Achievement>& getAchievements() const {
        return m_progression.getAchievements();
    }

    std::vector<Achievement> getUnlockedAchievements() const {
        return m_progression.getUnlockedAchievements();
    }

    // Challenges
    const std::vector<Challenge>& getActiveChallenges() const {
        return m_progression.getActiveChallenges();
    }

    // Callbacks - using correct signatures from ProgressionSystem.h
    void setLevelUpCallback(ProgressionSystem::LevelUpCallback callback) {
        m_progression.setLevelUpCallback(callback);
    }

    void setAchievementCallback(ProgressionSystem::AchievementCallback callback) {
        m_progression.setAchievementCallback(callback);
    }

    void setChallengeCompleteCallback(ProgressionSystem::ChallengeCompleteCallback callback) {
        m_progression.setChallengeCompleteCallback(callback);
    }

    // Direct access for advanced usage
    ProgressionSystem& getProgression() { return m_progression; }

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
