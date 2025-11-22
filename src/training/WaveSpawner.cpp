/**
 * WaveSpawner.cpp - Wave-Based Enemy Spawning Implementation
 */

#include "WaveSpawner.h"
#include <algorithm>
#include <cmath>

namespace lst {

// =============================================================================
// System Interface
// =============================================================================

bool WaveSpawner::onAttach(Engine* engine) {
    m_engine = engine;
    m_rng.seed(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));

    LOG_INFO(LOG_TAG_SPAWNER) << "WaveSpawner attached";
    LOG_DEBUG(LOG_TAG_SPAWNER) << "  Max drones: " << MAX_DRONES;
    LOG_DEBUG(LOG_TAG_SPAWNER) << "  Max projectiles: " << MAX_PROJECTILES;

    return true;
}

void WaveSpawner::onDetach() {
    LOG_INFO(LOG_TAG_SPAWNER) << "WaveSpawner detaching, cleaning up...";
    stopWave();
    m_engine = nullptr;
}

void WaveSpawner::onUpdate(const FrameContext& ctx) {
    if (!m_waveActive || m_waveComplete) {
        return;
    }

    float dt = ctx.deltaTime;
    m_waveTimer += dt;
    m_metrics.duration = m_waveTimer;

    // Process pending spawns (delayed spawning)
    for (auto& pending : m_pendingSpawns) {
        if (pending.remaining > 0) {
            pending.delay -= dt;
            if (pending.delay <= 0) {
                // Spawn one enemy
                spawnFromDef(pending.def);
                pending.remaining--;

                // Reset delay for next spawn in this group
                if (pending.remaining > 0) {
                    pending.delay = pending.def.spawnInterval;
                }
            }
        }
    }

    // Remove completed pending spawns
    m_pendingSpawns.erase(
        std::remove_if(m_pendingSpawns.begin(), m_pendingSpawns.end(),
            [](const PendingSpawn& p) { return p.remaining <= 0; }),
        m_pendingSpawns.end());

    // Update entities
    updateDrones(dt);
    checkDronesFiring(dt);
    updateProjectiles(dt);
    cleanupDeadEntities();

    // Check completion
    checkWaveCompletion();
}

// =============================================================================
// Wave Control
// =============================================================================

bool WaveSpawner::startWave(const TrainingWaveConfig& config) {
    if (!m_engine) {
        LOG_ERROR(LOG_TAG_SPAWNER) << "Cannot start wave: Engine is null";
        return false;
    }

    // Clean up any existing entities
    stopWave();

    m_currentWave = config;
    m_waveActive = true;
    m_waveComplete = false;
    m_waveTimer = 0.0f;
    m_metrics.reset();

    LOG_INFO(LOG_TAG_SPAWNER) << "════════════════════════════════════════";
    LOG_INFO(LOG_TAG_SPAWNER) << "WAVE START: " << config.name;
    LOG_INFO(LOG_TAG_SPAWNER) << "════════════════════════════════════════";
    LOG_INFO(LOG_TAG_SPAWNER) << "  Spawn definitions: " << config.spawns.size();
    LOG_INFO(LOG_TAG_SPAWNER) << "  Total enemies: " << config.getTotalEnemyCount();
    LOG_INFO(LOG_TAG_SPAWNER) << "  End condition: " << waveEndConditionToString(config.endCondition);

    if (config.endCondition == WaveEndCondition::TIME_ELAPSED) {
        LOG_INFO(LOG_TAG_SPAWNER) << "  Duration: " << config.endValue << "s";
    }

    // Queue all spawns
    for (const auto& spawnDef : config.spawns) {
        if (spawnDef.count <= 0) continue;

        if (spawnDef.spawnDelay > 0 || spawnDef.spawnInterval > 0) {
            // Delayed/staggered spawning
            PendingSpawn pending;
            pending.def = spawnDef;
            pending.delay = spawnDef.spawnDelay;
            pending.remaining = spawnDef.count;
            m_pendingSpawns.push_back(pending);

            LOG_DEBUG(LOG_TAG_SPAWNER) << "  Queued delayed spawn: "
                                        << spawnDef.count << "x " << enemyTypeToString(spawnDef.type)
                                        << " (delay=" << spawnDef.spawnDelay << "s)";
        } else {
            // Immediate spawning
            for (int i = 0; i < spawnDef.count; i++) {
                spawnFromDef(spawnDef);
            }
        }
    }

    LOG_INFO(LOG_TAG_SPAWNER) << "  Initial spawns: " << m_metrics.enemiesSpawned;
    LOG_INFO(LOG_TAG_SPAWNER) << "  Pending spawns: " << m_pendingSpawns.size();

    return true;
}

void WaveSpawner::stopWave() {
    if (!m_engine) return;

    LOG_DEBUG(LOG_TAG_SPAWNER) << "Stopping wave, cleaning up " << m_drones.size()
                                << " drones and " << m_projectiles.size() << " projectiles";

    // Remove all scene objects
    for (const auto& name : m_entityNames) {
        m_engine->removeSceneObject(name);
    }
    m_entityNames.clear();

    // Clear entity containers
    m_drones.clear();
    m_projectiles.clear();
    m_pendingSpawns.clear();

    m_waveActive = false;
}

// =============================================================================
// External Events
// =============================================================================

void WaveSpawner::notifyDroneHit(int droneId, int damage) {
    for (auto& drone : m_drones) {
        if (drone && drone->getId() == droneId) {
            drone->takeDamage(damage);

            if (!drone->isAlive()) {
                m_metrics.enemiesKilled++;
                m_metrics.enemiesRemaining--;

                // Score for kill
                addScore(100.0f);

                LOG_INFO(LOG_TAG_SPAWNER) << "Drone " << droneId << " destroyed!"
                                           << " (killed=" << m_metrics.enemiesKilled
                                           << ", remaining=" << m_metrics.enemiesRemaining << ")";

                if (m_onEnemyKilled) {
                    m_onEnemyKilled(droneId);
                }
            }
            return;
        }
    }
    LOG_WARN(LOG_TAG_SPAWNER) << "notifyDroneHit: Drone " << droneId << " not found";
}

void WaveSpawner::notifyPlayerHit(int damage) {
    m_metrics.hitsTaken += damage;
    LOG_INFO(LOG_TAG_SPAWNER) << "Player hit! (total hits: " << m_metrics.hitsTaken << ")";

    if (m_onPlayerHit) {
        m_onPlayerHit(damage);
    }
}

void WaveSpawner::addScore(float points) {
    m_metrics.score += points;
}

// =============================================================================
// Spawning Implementation
// =============================================================================

void WaveSpawner::spawnFromDef(const EnemySpawnDef& def) {
    // Currently only support FLYING_DRONE
    if (def.type != EnemyType::FLYING_DRONE && def.type != EnemyType::TRAINING_DUMMY) {
        LOG_WARN(LOG_TAG_SPAWNER) << "Unsupported enemy type: " << enemyTypeToString(def.type)
                                   << " - spawning as drone instead";
    }

    // Check limit
    if (static_cast<int>(m_drones.size()) >= MAX_DRONES) {
        LOG_WARN(LOG_TAG_SPAWNER) << "Drone limit reached (" << MAX_DRONES
                                   << "), skipping spawn";
        return;
    }

    // Create drone config
    DroneConfig config = createDroneConfig(def, m_metrics.enemiesSpawned);
    spawnDrone(config);
}

void WaveSpawner::spawnDrone(const DroneConfig& config) {
    if (!m_engine) return;

    int id = m_nextDroneId++;
    auto drone = std::make_unique<Drone>(id, config);

    // Add to scene
    SceneObject obj = drone->createSceneObject();
    m_engine->addSceneObject(obj);
    m_entityNames.push_back(obj.name);

    LOG_INFO(LOG_TAG_SPAWNER) << "Spawned drone " << id << " at ("
                               << config.spawnPosition.x << ", "
                               << config.spawnPosition.y << ", "
                               << config.spawnPosition.z << ")"
                               << " behavior=" << droneBehaviorToString(config.behavior)
                               << " canFire=" << (config.canFire ? "yes" : "no");

    m_drones.push_back(std::move(drone));
    m_metrics.enemiesSpawned++;
    m_metrics.enemiesRemaining++;
}

DroneConfig WaveSpawner::createDroneConfig(const EnemySpawnDef& def, int index) {
    DroneConfig config;

    // Map behavior
    config.behavior = mapBehavior(def.behavior);

    // Generate position
    config.spawnPosition = generateSpawnPosition(index, def.count);

    // Apply multipliers for "chill" Level 1 training
    // Base values are already slow; multipliers make them even slower
    config.fireInterval = 3.0f / def.fireRateMultiplier;  // Slower fire = higher interval
    config.projectileSpeed = 2.0f * def.speedMultiplier;
    config.orbitSpeed = 0.3f * def.speedMultiplier;
    config.maxHealth = static_cast<int>(1 * def.healthMultiplier);

    // Training dummies don't fire
    if (def.type == EnemyType::TRAINING_DUMMY) {
        config.canFire = false;
    } else {
        config.canFire = (def.behavior != EnemyBehavior::DIVE_ATTACK);  // Dive drones don't fire
    }

    return config;
}

Vec3 WaveSpawner::generateSpawnPosition(int index, int total) {
    // Distribute spawns in an arc in front of the player
    std::uniform_real_distribution<float> distHeight(m_spawnConfig.minHeight, m_spawnConfig.maxHeight);
    std::uniform_real_distribution<float> distDist(m_spawnConfig.minDistance, m_spawnConfig.maxDistance);

    float arcRange = m_spawnConfig.arcEnd - m_spawnConfig.arcStart;
    float angle;

    if (total <= 1) {
        // Single enemy: center of arc (in front)
        angle = (m_spawnConfig.arcStart + m_spawnConfig.arcEnd) / 2.0f;
    } else {
        // Multiple enemies: spread across arc
        float t = static_cast<float>(index) / (total - 1);
        angle = m_spawnConfig.arcStart + t * arcRange;
    }

    float distance = distDist(m_rng);
    float height = distHeight(m_rng);

    // Convert to position (player at origin, looking toward -Z)
    Vec3 pos;
    pos.x = m_playerPosition.x + std::sin(angle) * distance;
    pos.y = height;
    pos.z = m_playerPosition.z - std::cos(angle) * distance;  // Negative Z is "in front"

    return pos;
}

DroneBehavior WaveSpawner::mapBehavior(EnemyBehavior behavior) {
    switch (behavior) {
        case EnemyBehavior::STATIONARY:
            return DroneBehavior::HOVER;
        case EnemyBehavior::PATROL:
        case EnemyBehavior::ORBIT_PLAYER:
            return DroneBehavior::SLOW_ORBIT;
        case EnemyBehavior::DIVE_ATTACK:
        case EnemyBehavior::APPROACH_PLAYER:
        case EnemyBehavior::AGGRESSIVE:
            return DroneBehavior::SLOW_DIVE;
        default:
            return DroneBehavior::HOVER;
    }
}

// =============================================================================
// Update Helpers
// =============================================================================

void WaveSpawner::updateDrones(float deltaTime) {
    for (auto& drone : m_drones) {
        if (drone && drone->isAlive()) {
            drone->update(deltaTime, m_playerPosition);

            // Update scene object position
            SceneObject* obj = m_engine->getSceneObject(drone->getSceneObjectName());
            if (obj) {
                obj->transform.position = drone->getPosition();
                obj->transform.orientation = drone->getOrientation();
            }

            // Trigger dive attacks for dive drones
            if (drone->getBehavior() == DroneBehavior::SLOW_DIVE &&
                drone->getState() == DroneState::IDLE) {
                // Random chance to start dive each frame (average once per 5 seconds)
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                if (dist(m_rng) < deltaTime / 5.0f) {
                    drone->startDiveAttack(m_playerPosition);
                }
            }
        }
    }
}

void WaveSpawner::checkDronesFiring(float /*deltaTime*/) {
    for (auto& drone : m_drones) {
        if (drone && drone->isAlive() && drone->canFire()) {
            // Check projectile limit
            if (static_cast<int>(m_projectiles.size()) >= MAX_PROJECTILES) {
                continue;
            }

            // Fire projectile toward player
            Vec3 dronePos = drone->getPosition();
            Vec3 direction = m_playerPosition - dronePos;

            ProjectileConfig projConfig;
            projConfig.spawnPosition = dronePos;
            projConfig.direction = direction;
            projConfig.speed = 2.0f;  // Slow, easy to block
            projConfig.color = Color(1.0f, 0.3f, 0.3f);  // Red glow

            int projId = m_nextProjectileId++;
            auto proj = std::make_unique<Projectile>(projId, projConfig);

            // Add to scene
            SceneObject obj = proj->createSceneObject();
            m_engine->addSceneObject(obj);
            m_entityNames.push_back(obj.name);

            m_projectiles.push_back(std::move(proj));
            m_metrics.projectilesFired++;

            drone->resetFireTimer();

            LOG_DEBUG(LOG_TAG_SPAWNER) << "Drone " << drone->getId() << " fired projectile " << projId;
        }
    }
}

void WaveSpawner::updateProjectiles(float deltaTime) {
    for (auto& proj : m_projectiles) {
        if (proj && proj->isActive()) {
            bool stillActive = proj->update(deltaTime, m_saberPosition, m_saberRadius);

            if (stillActive) {
                // Update scene object position
                SceneObject* obj = m_engine->getSceneObject(proj->getSceneObjectName());
                if (obj) {
                    obj->transform.position = proj->getPosition();
                }
            } else {
                // Handle result
                if (proj->wasBlocked()) {
                    m_metrics.projectilesBlocked++;
                    addScore(10.0f);  // Score for blocking
                    LOG_DEBUG(LOG_TAG_SPAWNER) << "Projectile blocked! (total: "
                                                << m_metrics.projectilesBlocked << ")";
                } else if (proj->wasMissed()) {
                    m_metrics.projectilesMissed++;
                    // Check if it would have hit the player (simplified collision)
                    // For now, assume missed projectiles that passed player count as hits
                    notifyPlayerHit(1);
                }
            }
        }
    }
}

void WaveSpawner::cleanupDeadEntities() {
    // Remove dead drones
    for (auto it = m_drones.begin(); it != m_drones.end();) {
        if (*it && !(*it)->isAlive()) {
            std::string name = (*it)->getSceneObjectName();
            m_engine->removeSceneObject(name);
            m_entityNames.erase(
                std::remove(m_entityNames.begin(), m_entityNames.end(), name),
                m_entityNames.end());
            it = m_drones.erase(it);
        } else {
            ++it;
        }
    }

    // Remove inactive projectiles
    for (auto it = m_projectiles.begin(); it != m_projectiles.end();) {
        if (*it && !(*it)->isActive()) {
            std::string name = (*it)->getSceneObjectName();
            m_engine->removeSceneObject(name);
            m_entityNames.erase(
                std::remove(m_entityNames.begin(), m_entityNames.end(), name),
                m_entityNames.end());
            it = m_projectiles.erase(it);
        } else {
            ++it;
        }
    }
}

void WaveSpawner::checkWaveCompletion() {
    if (m_waveComplete) return;

    bool completed = false;

    switch (m_currentWave.endCondition) {
        case WaveEndCondition::ALL_ENEMIES_DEFEATED:
            // Complete when all enemies spawned and all killed
            if (m_pendingSpawns.empty() && m_drones.empty()) {
                completed = true;
                LOG_INFO(LOG_TAG_SPAWNER) << "Wave complete: All enemies defeated";
            }
            break;

        case WaveEndCondition::TIME_ELAPSED:
            if (m_waveTimer >= m_currentWave.endValue) {
                completed = true;
                LOG_INFO(LOG_TAG_SPAWNER) << "Wave complete: Time elapsed ("
                                           << m_waveTimer << "s >= " << m_currentWave.endValue << "s)";
            }
            break;

        case WaveEndCondition::SCORE_REACHED:
            if (m_metrics.score >= m_currentWave.endValue) {
                completed = true;
                LOG_INFO(LOG_TAG_SPAWNER) << "Wave complete: Score reached ("
                                           << m_metrics.score << " >= " << m_currentWave.endValue << ")";
            }
            break;

        case WaveEndCondition::PLAYER_ACTION:
        case WaveEndCondition::MANUAL:
            // External control - don't auto-complete
            break;
    }

    // Also check minimum duration
    if (completed && m_currentWave.minDuration > 0 && m_waveTimer < m_currentWave.minDuration) {
        LOG_DEBUG(LOG_TAG_SPAWNER) << "Wave completion delayed: min duration not met ("
                                    << m_waveTimer << "s < " << m_currentWave.minDuration << "s)";
        completed = false;
    }

    if (completed) {
        m_waveComplete = true;

        LOG_INFO(LOG_TAG_SPAWNER) << "────────────────────────────────────────";
        LOG_INFO(LOG_TAG_SPAWNER) << "WAVE FINISHED: " << m_currentWave.name;
        LOG_INFO(LOG_TAG_SPAWNER) << "────────────────────────────────────────";
        LOG_INFO(LOG_TAG_SPAWNER) << "  Duration: " << m_metrics.duration << "s";
        LOG_INFO(LOG_TAG_SPAWNER) << "  Enemies: spawned=" << m_metrics.enemiesSpawned
                                   << ", killed=" << m_metrics.enemiesKilled;
        LOG_INFO(LOG_TAG_SPAWNER) << "  Projectiles: fired=" << m_metrics.projectilesFired
                                   << ", blocked=" << m_metrics.projectilesBlocked
                                   << ", missed=" << m_metrics.projectilesMissed;
        LOG_INFO(LOG_TAG_SPAWNER) << "  Hits taken: " << m_metrics.hitsTaken;
        LOG_INFO(LOG_TAG_SPAWNER) << "  Score: " << m_metrics.score;
        LOG_INFO(LOG_TAG_SPAWNER) << "  Block accuracy: " << (m_metrics.blockAccuracy() * 100.0f) << "%";

        if (m_onWaveEnemiesCleared) {
            m_onWaveEnemiesCleared(m_metrics);
        }
    }
}

} // namespace lst
