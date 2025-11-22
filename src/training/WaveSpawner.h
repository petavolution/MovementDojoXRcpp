/**
 * WaveSpawner.h - Wave-Based Enemy Spawning System
 *
 * Bridges data-driven TrainingWaveConfig to concrete entity spawning:
 * - Converts EnemySpawnDef to DroneConfig
 * - Tracks active enemies per wave
 * - Detects wave completion conditions
 * - Reports metrics (spawned, killed, hits taken)
 *
 * This system is decoupled from the sequence controller and can be
 * used independently for any wave-based gameplay.
 */

#pragma once

#include "TrainingSequence.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../entities/Drone.h"
#include "../entities/Projectile.h"
#include <vector>
#include <memory>
#include <functional>
#include <random>

namespace lst {

#define LOG_TAG_SPAWNER "WaveSpawner"

// =============================================================================
// Wave Metrics - Tracking per-wave performance
// =============================================================================

/**
 * Metrics tracked during a wave.
 */
struct WaveMetrics {
    // Enemy tracking
    int enemiesSpawned = 0;
    int enemiesKilled = 0;
    int enemiesRemaining = 0;

    // Combat stats
    int projectilesFired = 0;
    int projectilesBlocked = 0;
    int projectilesMissed = 0;
    int hitsTaken = 0;       // Projectiles that hit the player

    // Scoring
    float score = 0.0f;

    // Timing
    float duration = 0.0f;

    // Calculated
    float blockAccuracy() const {
        int total = projectilesBlocked + hitsTaken;
        return total > 0 ? (float)projectilesBlocked / total : 1.0f;
    }

    float killRate() const {
        return enemiesSpawned > 0 ? (float)enemiesKilled / enemiesSpawned : 0.0f;
    }

    void reset() {
        enemiesSpawned = 0;
        enemiesKilled = 0;
        enemiesRemaining = 0;
        projectilesFired = 0;
        projectilesBlocked = 0;
        projectilesMissed = 0;
        hitsTaken = 0;
        score = 0.0f;
        duration = 0.0f;
    }
};

// =============================================================================
// Spawn Position Generator
// =============================================================================

/**
 * Generates spawn positions around the player.
 * Uses a configurable pattern for Level 1 "chill" training.
 */
struct SpawnPositionConfig {
    float minDistance = 2.5f;   // Minimum distance from player
    float maxDistance = 4.0f;   // Maximum distance from player
    float minHeight = 1.2f;     // Minimum spawn height
    float maxHeight = 2.0f;     // Maximum spawn height
    float arcStart = -1.0f;     // Radians from forward (-1.0 = ~60 deg left)
    float arcEnd = 1.0f;        // Radians from forward (1.0 = ~60 deg right)
};

// =============================================================================
// Wave Spawner Callbacks
// =============================================================================

// Called when all enemies in wave are defeated
using OnWaveEnemiesClearedCallback = std::function<void(const WaveMetrics&)>;

// Called when player takes damage
using OnPlayerHitCallback = std::function<void(int damage)>;

// Called when enemy is killed
using OnEnemyKilledCallback = std::function<void(int droneId)>;

// =============================================================================
// Wave Spawner System
// =============================================================================

/**
 * WaveSpawner - Manages enemy spawning and tracking for wave-based gameplay.
 *
 * Usage:
 *   auto* spawner = engine.addSystem<WaveSpawner>();
 *   spawner->startWave(waveConfig);
 *   // ... updates automatically via onUpdate() ...
 *   if (spawner->isWaveComplete()) {
 *       auto metrics = spawner->getMetrics();
 *   }
 */
class WaveSpawner : public System {
public:
    // -------------------------------------------------------------------------
    // System Interface
    // -------------------------------------------------------------------------

    const char* getName() const override { return "WaveSpawner"; }

    bool onAttach(Engine* engine) override;
    void onDetach() override;
    void onUpdate(const FrameContext& ctx) override;

    // -------------------------------------------------------------------------
    // Wave Control
    // -------------------------------------------------------------------------

    /**
     * Start spawning enemies for a wave.
     * Clears any existing entities first.
     */
    bool startWave(const TrainingWaveConfig& config);

    /**
     * Stop the current wave and clean up all entities.
     */
    void stopWave();

    /**
     * Check if the wave is complete (all enemies defeated or time elapsed).
     */
    bool isWaveComplete() const { return m_waveComplete; }

    /**
     * Check if the wave is currently active.
     */
    bool isWaveActive() const { return m_waveActive; }

    /**
     * Get current wave metrics.
     */
    const WaveMetrics& getMetrics() const { return m_metrics; }

    /**
     * Get number of active enemies.
     */
    int getActiveEnemyCount() const { return static_cast<int>(m_drones.size()); }

    /**
     * Get number of active projectiles.
     */
    int getActiveProjectileCount() const { return static_cast<int>(m_projectiles.size()); }

    // -------------------------------------------------------------------------
    // External Events (call these from combat systems)
    // -------------------------------------------------------------------------

    /**
     * Notify that a drone was hit by player attack.
     */
    void notifyDroneHit(int droneId, int damage = 1);

    /**
     * Notify that player was hit by a projectile.
     */
    void notifyPlayerHit(int damage = 1);

    /**
     * Add score for the current wave.
     */
    void addScore(float points);

    // -------------------------------------------------------------------------
    // Player State (for AI targeting)
    // -------------------------------------------------------------------------

    /**
     * Update player position (called each frame by main loop).
     */
    void setPlayerPosition(const Vec3& pos) { m_playerPosition = pos; }
    void setPlayerSaberPosition(const Vec3& pos) { m_saberPosition = pos; }
    void setPlayerSaberRadius(float radius) { m_saberRadius = radius; }

    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------

    void setOnWaveEnemiesCleared(OnWaveEnemiesClearedCallback cb) {
        m_onWaveEnemiesCleared = std::move(cb);
    }
    void setOnPlayerHit(OnPlayerHitCallback cb) {
        m_onPlayerHit = std::move(cb);
    }
    void setOnEnemyKilled(OnEnemyKilledCallback cb) {
        m_onEnemyKilled = std::move(cb);
    }

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * Set spawn position configuration.
     */
    void setSpawnConfig(const SpawnPositionConfig& config) { m_spawnConfig = config; }

    /**
     * Entity limits (safety).
     */
    static constexpr int MAX_DRONES = 6;
    static constexpr int MAX_PROJECTILES = 30;

private:
    // -------------------------------------------------------------------------
    // Spawning Implementation
    // -------------------------------------------------------------------------

    /**
     * Spawn enemies based on spawn definition.
     */
    void spawnFromDef(const EnemySpawnDef& def);

    /**
     * Spawn a single drone with given config.
     */
    void spawnDrone(const DroneConfig& config);

    /**
     * Convert EnemySpawnDef to DroneConfig.
     */
    DroneConfig createDroneConfig(const EnemySpawnDef& def, int index);

    /**
     * Generate spawn position.
     */
    Vec3 generateSpawnPosition(int index, int total);

    /**
     * Map EnemyBehavior to DroneBehavior.
     */
    DroneBehavior mapBehavior(EnemyBehavior behavior);

    // -------------------------------------------------------------------------
    // Update Helpers
    // -------------------------------------------------------------------------

    void updateDrones(float deltaTime);
    void updateProjectiles(float deltaTime);
    void checkDronesFiring(float deltaTime);
    void cleanupDeadEntities();
    void checkWaveCompletion();

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    Engine* m_engine = nullptr;

    // Wave state
    bool m_waveActive = false;
    bool m_waveComplete = false;
    TrainingWaveConfig m_currentWave;
    WaveMetrics m_metrics;
    float m_waveTimer = 0.0f;

    // Pending spawns (for delayed spawning)
    struct PendingSpawn {
        EnemySpawnDef def;
        float delay;
        int remaining;
    };
    std::vector<PendingSpawn> m_pendingSpawns;

    // Active entities
    std::vector<std::unique_ptr<Drone>> m_drones;
    std::vector<std::unique_ptr<Projectile>> m_projectiles;
    std::vector<std::string> m_entityNames;  // For cleanup

    // ID counters
    int m_nextDroneId = 0;
    int m_nextProjectileId = 0;

    // Player state (for AI)
    Vec3 m_playerPosition = Vec3(0, 1.5f, 0);
    Vec3 m_saberPosition = Vec3(0.3f, 1.2f, -0.3f);
    float m_saberRadius = 0.15f;

    // Configuration
    SpawnPositionConfig m_spawnConfig;

    // RNG for spawn positions
    std::mt19937 m_rng;

    // Callbacks
    OnWaveEnemiesClearedCallback m_onWaveEnemiesCleared;
    OnPlayerHitCallback m_onPlayerHit;
    OnEnemyKilledCallback m_onEnemyKilled;
};

} // namespace lst
