/**
 * Level1Training.h - Dojo Level 1 Training System
 *
 * A gentle, tutorial-like training experience that demonstrates:
 * - Saber basics (blocking + striking)
 * - Blaster basics (aiming + shooting)
 * - Simple drone encounters
 *
 * State Machine:
 *   INTRO (10s) -> SABER_DRILL (45s) -> BLASTER_DRILL (45s) -> MIXED_DRILL (60s) -> SUMMARY (10s)
 *
 * Focus: Robust state transitions, comprehensive logging, graceful handling of missing subsystems.
 */

#pragma once

#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../entities/Drone.h"
#include "../entities/Projectile.h"
#include <string>
#include <chrono>
#include <memory>

namespace lst {

// Log tag for Level 1 training
#define LOG_TAG_LEVEL1 "Level1"

/**
 * Level 1 Training States
 */
enum class Level1State {
    NOT_STARTED,    // Initial state before level begins
    INTRO,          // Welcome, show controls, weapons visible
    SABER_DRILL,    // Practice blocking and striking
    BLASTER_DRILL,  // Practice aiming and shooting
    MIXED_DRILL,    // Combined combat scenario
    SUMMARY,        // Show stats, completion message
    COMPLETED       // Level finished
};

/**
 * Convert Level1State to string for logging
 */
inline const char* level1StateToString(Level1State state) {
    switch (state) {
        case Level1State::NOT_STARTED:   return "NOT_STARTED";
        case Level1State::INTRO:         return "INTRO";
        case Level1State::SABER_DRILL:   return "SABER_DRILL";
        case Level1State::BLASTER_DRILL: return "BLASTER_DRILL";
        case Level1State::MIXED_DRILL:   return "MIXED_DRILL";
        case Level1State::SUMMARY:       return "SUMMARY";
        case Level1State::COMPLETED:     return "COMPLETED";
        default:                         return "UNKNOWN";
    }
}

/**
 * Level 1 Statistics
 */
struct Level1Stats {
    int targetsHit = 0;
    int targetsMissed = 0;
    int projectilesBlocked = 0;
    int projectilesMissed = 0;
    int shotsHit = 0;
    int shotsFired = 0;
    int divesDodged = 0;
    int divesHit = 0;

    float totalTime = 0.0f;
    float introTime = 0.0f;
    float saberDrillTime = 0.0f;
    float blasterDrillTime = 0.0f;
    float mixedDrillTime = 0.0f;

    // Calculate accuracy percentages
    float getSaberAccuracy() const {
        int total = targetsHit + targetsMissed;
        return total > 0 ? (float)targetsHit / total * 100.0f : 0.0f;
    }

    float getBlockAccuracy() const {
        int total = projectilesBlocked + projectilesMissed;
        return total > 0 ? (float)projectilesBlocked / total * 100.0f : 0.0f;
    }

    float getBlasterAccuracy() const {
        return shotsFired > 0 ? (float)shotsHit / shotsFired * 100.0f : 0.0f;
    }
};

/**
 * Level 1 Training System
 *
 * Plugin-based system that manages the Level 1 training experience.
 * Integrates with the Engine's system architecture.
 */
class Level1TrainingSystem : public System {
public:
    Level1TrainingSystem();
    ~Level1TrainingSystem() override = default;

    // System interface
    const char* getName() const override { return "Level1Training"; }
    bool onAttach(Engine* engine) override;
    void onDetach() override;
    void onSessionStart() override;
    void onSessionEnd() override;
    void onUpdate(const FrameContext& ctx) override;
    void onRender(const FrameContext& ctx) override;

    // Level control
    void startLevel();
    void pauseLevel();
    void resumeLevel();
    void skipToState(Level1State state);

    // State queries
    Level1State getCurrentState() const { return m_currentState; }
    bool isCompleted() const { return m_currentState == Level1State::COMPLETED; }
    bool isPaused() const { return m_paused; }
    const Level1Stats& getStats() const { return m_stats; }
    float getStateTimeRemaining() const;
    float getStateDuration() const;

private:
    // State machine
    void transitionTo(Level1State newState);
    void updateCurrentState(float deltaTime);

    // State handlers
    void onEnterState(Level1State state);
    void onExitState(Level1State state);
    void updateIntro(float deltaTime);
    void updateSaberDrill(float deltaTime);
    void updateBlasterDrill(float deltaTime);
    void updateMixedDrill(float deltaTime);
    void updateSummary(float deltaTime);

    // Scene setup
    void setupDojoEnvironment();
    void setupWeapons();
    void clearDrillEntities();

    // Entity management
    void spawnDrone(const DroneConfig& config);
    void spawnProjectile(const ProjectileConfig& config);
    void updateDrones(float deltaTime, const Vec3& playerPosition);
    void updateProjectiles(float deltaTime, const Vec3& saberPosition, float saberRadius);
    void cleanupDeadEntities();
    void fireProjectileFromDrone(Drone& drone, const Vec3& targetPosition);
    void checkBlasterHits(const Vec3& blasterPosition, const Vec3& blasterDirection);

    // Phase-specific spawning
    void spawnSaberDrillEntities();
    void spawnBlasterDrillEntities();
    void spawnMixedDrillEntities();

    // Rendering helpers
    void renderStateHUD(const FrameContext& ctx);
    void renderIntroText(const FrameContext& ctx);
    void renderDrillHUD(const FrameContext& ctx);
    void renderSummary(const FrameContext& ctx);

    // Logging
    void logStateTransition(Level1State from, Level1State to);
    void logLevelSummary();

private:
    Engine* m_engine = nullptr;

    // State machine
    Level1State m_currentState = Level1State::NOT_STARTED;
    Level1State m_previousState = Level1State::NOT_STARTED;
    float m_stateTimer = 0.0f;
    bool m_paused = false;

    // Timing configuration (in seconds)
    static constexpr float INTRO_DURATION = 10.0f;
    static constexpr float SABER_DRILL_DURATION = 45.0f;
    static constexpr float BLASTER_DRILL_DURATION = 45.0f;
    static constexpr float MIXED_DRILL_DURATION = 60.0f;
    static constexpr float SUMMARY_DURATION = 10.0f;

    // Statistics
    Level1Stats m_stats;

    // Level timing
    std::chrono::steady_clock::time_point m_levelStartTime;
    float m_totalLevelTime = 0.0f;

    // Scene object names (for cleanup)
    std::vector<std::string> m_drillEntityNames;

    // Entity management
    std::vector<std::unique_ptr<Drone>> m_drones;
    std::vector<std::unique_ptr<Projectile>> m_projectiles;
    int m_nextDroneId = 0;
    int m_nextProjectileId = 0;

    // Entity limits (for safety)
    static constexpr int MAX_DRONES = 3;
    static constexpr int MAX_PROJECTILES = 20;

    // Saber collision parameters
    static constexpr float SABER_RADIUS = 0.1f;  // Collision radius for blocking
    static constexpr float BLASTER_RANGE = 20.0f;
    static constexpr float BLASTER_RADIUS = 0.15f;

    // Dive attack tracking
    bool m_diveAttackTriggered = false;
    float m_diveAttackTimer = 0.0f;
    static constexpr float DIVE_ATTACK_TIME = 30.0f;  // Trigger dive at 30s into mixed drill

    // Flags
    bool m_environmentSetup = false;
    bool m_weaponsSetup = false;
};

} // namespace lst
