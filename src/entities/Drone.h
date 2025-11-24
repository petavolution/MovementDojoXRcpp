/**
 * Drone.h - Level 1 Training Drone Entity
 *
 * Simple drone implementation for beginner training:
 * - HOVER: Stationary target for blaster practice
 * - SLOW_ORBIT: Slow circular movement around player
 * - SLOW_DIVE: Telegraphed dive attack (slow, predictable)
 *
 * Design: Safe, beginner-friendly AI with comprehensive logging.
 */

#pragma once

#include "../core/Engine.h"
#include "../core/Logger.h"
#include <string>
#include <cmath>

namespace lst {

#define LOG_TAG_DRONE "Drone"

/**
 * Drone behavior types
 */
enum class DroneBehavior {
    HOVER,          // Stationary, face player
    SLOW_ORBIT,     // Slow circular orbit around player
    SLOW_DIVE       // Telegraphed dive attack
};

/**
 * Drone state for dive attacks
 */
enum class DroneState {
    IDLE,           // Normal operation
    TELEGRAPH,      // Warning phase before dive
    DIVING,         // Active dive
    RECOVERING      // Post-dive recovery
};

/**
 * Convert behavior enum to string
 */
inline const char* droneBehaviorToString(DroneBehavior behavior) {
    switch (behavior) {
        case DroneBehavior::HOVER:      return "HOVER";
        case DroneBehavior::SLOW_ORBIT: return "SLOW_ORBIT";
        case DroneBehavior::SLOW_DIVE:  return "SLOW_DIVE";
        default:                        return "UNKNOWN";
    }
}

inline const char* droneStateToString(DroneState state) {
    switch (state) {
        case DroneState::IDLE:       return "IDLE";
        case DroneState::TELEGRAPH:  return "TELEGRAPH";
        case DroneState::DIVING:     return "DIVING";
        case DroneState::RECOVERING: return "RECOVERING";
        default:                     return "UNKNOWN";
    }
}

/**
 * Drone configuration
 */
struct DroneConfig {
    DroneBehavior behavior = DroneBehavior::HOVER;
    Vec3 spawnPosition = Vec3(0, 1.5f, -2.0f);
    float orbitRadius = 2.0f;
    float orbitSpeed = 0.3f;       // radians per second (very slow)
    float fireInterval = 3.0f;    // seconds between shots (slow)
    float projectileSpeed = 2.0f; // m/s (very slow, easy to block)
    int maxHealth = 1;            // One-shot kill for beginners
    bool canFire = true;
};

/**
 * Drone entity
 */
class Drone {
public:
    Drone(int id, const DroneConfig& config);
    ~Drone() = default;

    // Update
    void update(float deltaTime, const Vec3& playerPosition);

    // State queries
    int getId() const { return m_id; }
    const Vec3& getPosition() const { return m_position; }
    const Quat& getOrientation() const { return m_orientation; }
    DroneBehavior getBehavior() const { return m_config.behavior; }
    DroneState getState() const { return m_state; }
    bool isAlive() const { return m_health > 0; }
    bool canFire() const { return m_config.canFire && m_fireTimer <= 0; }
    float getFireTimer() const { return m_fireTimer; }
    std::string getSceneObjectName() const { return "Drone_" + std::to_string(m_id); }

    // Combat
    void takeDamage(int damage);
    void resetFireTimer();

    // Dive attack control
    void startDiveAttack(const Vec3& targetPosition);
    bool isDiving() const { return m_state == DroneState::DIVING; }
    bool isDiveComplete() const { return m_state == DroneState::RECOVERING && m_diveTimer <= 0; }

    // Scene object creation
    SceneObject createSceneObject() const;

private:
    void updateHover(float deltaTime, const Vec3& playerPosition);
    void updateOrbit(float deltaTime, const Vec3& playerPosition);
    void updateDive(float deltaTime, const Vec3& playerPosition);
    void facePlayer(const Vec3& playerPosition);

private:
    int m_id;
    DroneConfig m_config;

    // Position and orientation
    Vec3 m_position;
    Quat m_orientation;

    // State
    DroneState m_state = DroneState::IDLE;
    int m_health;
    float m_fireTimer = 0.0f;

    // Orbit state
    float m_orbitAngle = 0.0f;
    float m_hoverHeight;

    // Dive attack state
    Vec3 m_diveTarget;
    Vec3 m_diveStartPosition;
    float m_diveTimer = 0.0f;
    float m_telegraphTimer = 0.0f;

    // Timing constants
    static constexpr float TELEGRAPH_DURATION = 1.5f;  // Clear warning
    static constexpr float DIVE_DURATION = 1.0f;       // Slow dive
    static constexpr float RECOVERY_DURATION = 2.0f;   // Long recovery
    static constexpr float DIVE_SPEED = 3.0f;          // Slow, dodgeable
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline Drone::Drone(int id, const DroneConfig& config)
    : m_id(id)
    , m_config(config)
    , m_position(config.spawnPosition)
    , m_orientation(Quat::identity())
    , m_health(config.maxHealth)
    , m_hoverHeight(config.spawnPosition.y)
{
    LOG_DEBUG(LOG_TAG_DRONE) << "Drone " << m_id << " created at ("
                              << m_position.x << ", " << m_position.y << ", " << m_position.z << ")"
                              << " behavior=" << droneBehaviorToString(m_config.behavior);
}

inline void Drone::update(float deltaTime, const Vec3& playerPosition) {
    if (!isAlive()) return;

    // Update fire timer
    if (m_fireTimer > 0) {
        m_fireTimer -= deltaTime;
    }

    // Update based on behavior
    switch (m_config.behavior) {
        case DroneBehavior::HOVER:
            updateHover(deltaTime, playerPosition);
            break;
        case DroneBehavior::SLOW_ORBIT:
            updateOrbit(deltaTime, playerPosition);
            break;
        case DroneBehavior::SLOW_DIVE:
            updateDive(deltaTime, playerPosition);
            break;
    }
}

inline void Drone::updateHover(float deltaTime, const Vec3& playerPosition) {
    (void)deltaTime;  // Stationary
    facePlayer(playerPosition);

    // Gentle hover bob
    float time = static_cast<float>(std::fmod(m_orbitAngle, 6.28f));
    m_orbitAngle += deltaTime;
    m_position.y = m_hoverHeight + std::sin(time * 2.0f) * 0.05f;
}

inline void Drone::updateOrbit(float deltaTime, const Vec3& playerPosition) {
    // Update orbit angle (slow)
    m_orbitAngle += m_config.orbitSpeed * deltaTime;

    // Calculate position on orbit circle
    float x = playerPosition.x + m_config.orbitRadius * std::cos(m_orbitAngle);
    float z = playerPosition.z + m_config.orbitRadius * std::sin(m_orbitAngle);
    m_position = Vec3(x, m_hoverHeight, z);

    facePlayer(playerPosition);
}

inline void Drone::updateDive(float deltaTime, const Vec3& playerPosition) {
    switch (m_state) {
        case DroneState::IDLE:
            // Normal hover behavior until dive is triggered
            updateHover(deltaTime, playerPosition);
            break;

        case DroneState::TELEGRAPH: {
            // Warning phase - glow/shake to signal incoming attack
            m_telegraphTimer -= deltaTime;
            facePlayer(playerPosition);

            // Shake effect
            float shake = std::sin(m_telegraphTimer * 30.0f) * 0.02f;
            m_position.x += shake;

            if (m_telegraphTimer <= 0) {
                LOG_INFO(LOG_TAG_DRONE) << "Drone " << m_id << " starting dive attack";
                m_state = DroneState::DIVING;
                m_diveTimer = DIVE_DURATION;
                m_diveStartPosition = m_position;
            }
            break;
        }

        case DroneState::DIVING:
            m_diveTimer -= deltaTime;
            {
                // Lerp toward target
                float t = 1.0f - (m_diveTimer / DIVE_DURATION);
                t = std::min(1.0f, std::max(0.0f, t));
                m_position = m_diveStartPosition + (m_diveTarget - m_diveStartPosition) * t;
            }

            if (m_diveTimer <= 0) {
                LOG_DEBUG(LOG_TAG_DRONE) << "Drone " << m_id << " dive complete, recovering";
                m_state = DroneState::RECOVERING;
                m_diveTimer = RECOVERY_DURATION;
            }
            break;

        case DroneState::RECOVERING:
            m_diveTimer -= deltaTime;
            {
                // Return to hover position
                float t = 1.0f - (m_diveTimer / RECOVERY_DURATION);
                Vec3 hoverPos = Vec3(m_config.spawnPosition.x, m_hoverHeight, m_config.spawnPosition.z);
                m_position = m_diveTarget + (hoverPos - m_diveTarget) * t;
            }

            if (m_diveTimer <= 0) {
                LOG_DEBUG(LOG_TAG_DRONE) << "Drone " << m_id << " recovery complete, returning to idle";
                m_state = DroneState::IDLE;
            }
            break;
    }
}

inline void Drone::facePlayer(const Vec3& playerPosition) {
    Vec3 toPlayer = playerPosition - m_position;
    toPlayer.y = 0;  // Only rotate on Y axis
    float len = toPlayer.length();
    if (len > 0.01f) {
        float angle = std::atan2(toPlayer.x, toPlayer.z);
        m_orientation = Quat::fromAxisAngle(Vec3(0, 1, 0), angle);
    }
}

inline void Drone::takeDamage(int damage) {
    m_health -= damage;
    LOG_INFO(LOG_TAG_DRONE) << "Drone " << m_id << " took " << damage << " damage"
                            << ", health: " << m_health;
    if (m_health <= 0) {
        LOG_INFO(LOG_TAG_DRONE) << "Drone " << m_id << " destroyed!";
    }
}

inline void Drone::resetFireTimer() {
    m_fireTimer = m_config.fireInterval;
}

inline void Drone::startDiveAttack(const Vec3& targetPosition) {
    if (m_state != DroneState::IDLE) {
        LOG_WARN(LOG_TAG_DRONE) << "Drone " << m_id << " cannot start dive - not in IDLE state";
        return;
    }

    LOG_INFO(LOG_TAG_DRONE) << "Drone " << m_id << " telegraphing dive attack!";
    m_state = DroneState::TELEGRAPH;
    m_telegraphTimer = TELEGRAPH_DURATION;
    m_diveTarget = targetPosition;
}

inline SceneObject Drone::createSceneObject() const {
    SceneObject obj;
    obj.name = getSceneObjectName();
    obj.mesh = Mesh::createSphere(0.15f, 12);  // Small sphere drone
    obj.transform.position = m_position;
    obj.transform.orientation = m_orientation;
    obj.transform.scale = Vec3(1, 0.6f, 1);  // Slightly flattened

    // Color based on behavior
    switch (m_config.behavior) {
        case DroneBehavior::HOVER:
            obj.material.baseColor = Color(0.8f, 0.2f, 0.2f);  // Red
            break;
        case DroneBehavior::SLOW_ORBIT:
            obj.material.baseColor = Color(0.8f, 0.5f, 0.2f);  // Orange
            break;
        case DroneBehavior::SLOW_DIVE:
            obj.material.baseColor = Color(0.9f, 0.2f, 0.9f);  // Purple (danger)
            break;
    }

    obj.material.emissive = 0.3f;
    return obj;
}

} // namespace lst
