/**
 * Projectile.h - Level 1 Training Projectile Entity
 *
 * Simple slow-moving projectile for beginner training:
 * - Slow velocity (2-3 m/s) - easy to track and block
 * - Visible glow for clarity
 * - Auto-despawn after timeout or hit
 * - Clear collision detection with saber
 *
 * Design: Safe, beginner-friendly with comprehensive logging.
 */

#pragma once

#include "../core/Engine.h"
#include "../core/Logger.h"
#include <string>
#include <cmath>

namespace lst {

#define LOG_TAG_PROJECTILE "Projectile"

/**
 * Projectile state
 */
enum class ProjectileState {
    ACTIVE,     // Flying toward target
    BLOCKED,    // Hit by saber
    MISSED,     // Passed player without being blocked
    DESPAWNED   // Removed (timeout)
};

inline const char* projectileStateToString(ProjectileState state) {
    switch (state) {
        case ProjectileState::ACTIVE:    return "ACTIVE";
        case ProjectileState::BLOCKED:   return "BLOCKED";
        case ProjectileState::MISSED:    return "MISSED";
        case ProjectileState::DESPAWNED: return "DESPAWNED";
        default:                         return "UNKNOWN";
    }
}

/**
 * Projectile configuration
 */
struct ProjectileConfig {
    Vec3 spawnPosition;
    Vec3 direction;              // Will be normalized
    float speed = 2.0f;          // m/s (very slow)
    float radius = 0.08f;        // Collision radius
    float lifetime = 5.0f;       // Auto-despawn after this time
    float maxDistance = 10.0f;   // Auto-despawn after traveling this far
    Color color = Color(1.0f, 0.3f, 0.3f);  // Default red glow
};

/**
 * Projectile entity
 */
class Projectile {
public:
    Projectile(int id, const ProjectileConfig& config);
    ~Projectile() = default;

    // Update - returns false if projectile should be removed
    bool update(float deltaTime, const Vec3& saberPosition, float saberRadius);

    // State queries
    int getId() const { return m_id; }
    const Vec3& getPosition() const { return m_position; }
    const Vec3& getVelocity() const { return m_velocity; }
    ProjectileState getState() const { return m_state; }
    bool isActive() const { return m_state == ProjectileState::ACTIVE; }
    bool wasBlocked() const { return m_state == ProjectileState::BLOCKED; }
    bool wasMissed() const { return m_state == ProjectileState::MISSED; }
    std::string getSceneObjectName() const { return "Projectile_" + std::to_string(m_id); }

    // Manual state changes
    void markBlocked();
    void markMissed();

    // Scene object creation
    SceneObject createSceneObject() const;

    // Check collision with a sphere
    bool checkCollision(const Vec3& center, float radius) const;

private:
    int m_id;
    ProjectileConfig m_config;

    // Position and movement
    Vec3 m_position;
    Vec3 m_velocity;
    Vec3 m_spawnPosition;

    // State
    ProjectileState m_state = ProjectileState::ACTIVE;
    float m_lifetime;
    float m_distanceTraveled = 0.0f;
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline Projectile::Projectile(int id, const ProjectileConfig& config)
    : m_id(id)
    , m_config(config)
    , m_position(config.spawnPosition)
    , m_spawnPosition(config.spawnPosition)
    , m_lifetime(config.lifetime)
{
    // Normalize direction and apply speed
    Vec3 dir = config.direction;
    float len = dir.length();
    if (len > 0.001f) {
        dir = dir * (1.0f / len);
    } else {
        dir = Vec3(0, 0, 1);  // Default forward
    }
    m_velocity = dir * config.speed;

    LOG_DEBUG(LOG_TAG_PROJECTILE) << "Projectile " << m_id << " spawned at ("
                                   << m_position.x << ", " << m_position.y << ", " << m_position.z << ")"
                                   << " velocity=(" << m_velocity.x << ", " << m_velocity.y << ", " << m_velocity.z << ")";
}

inline bool Projectile::update(float deltaTime, const Vec3& saberPosition, float saberRadius) {
    if (!isActive()) {
        return false;  // Should be removed
    }

    // Update position
    Vec3 movement = m_velocity * deltaTime;
    m_position = m_position + movement;
    m_distanceTraveled += movement.length();

    // Check lifetime
    m_lifetime -= deltaTime;
    if (m_lifetime <= 0) {
        LOG_DEBUG(LOG_TAG_PROJECTILE) << "Projectile " << m_id << " timed out";
        m_state = ProjectileState::DESPAWNED;
        return false;
    }

    // Check max distance
    if (m_distanceTraveled > m_config.maxDistance) {
        LOG_DEBUG(LOG_TAG_PROJECTILE) << "Projectile " << m_id << " exceeded max distance";
        m_state = ProjectileState::MISSED;
        return false;
    }

    // Check if passed the player (z > player z + threshold)
    // In our coordinate system, player is at origin, projectiles move toward positive z
    if (m_position.z > 1.0f) {  // Passed player
        LOG_DEBUG(LOG_TAG_PROJECTILE) << "Projectile " << m_id << " passed player - MISSED";
        m_state = ProjectileState::MISSED;
        return false;
    }

    // Check saber collision
    if (saberRadius > 0 && checkCollision(saberPosition, saberRadius)) {
        LOG_INFO(LOG_TAG_PROJECTILE) << "Projectile " << m_id << " BLOCKED by saber!";
        m_state = ProjectileState::BLOCKED;
        return false;
    }

    return true;  // Still active
}

inline bool Projectile::checkCollision(const Vec3& center, float radius) const {
    Vec3 diff = m_position - center;
    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    float combinedRadius = m_config.radius + radius;
    return distSq < (combinedRadius * combinedRadius);
}

inline void Projectile::markBlocked() {
    if (isActive()) {
        LOG_INFO(LOG_TAG_PROJECTILE) << "Projectile " << m_id << " marked as BLOCKED";
        m_state = ProjectileState::BLOCKED;
    }
}

inline void Projectile::markMissed() {
    if (isActive()) {
        LOG_DEBUG(LOG_TAG_PROJECTILE) << "Projectile " << m_id << " marked as MISSED";
        m_state = ProjectileState::MISSED;
    }
}

inline SceneObject Projectile::createSceneObject() const {
    SceneObject obj;
    obj.name = getSceneObjectName();
    obj.mesh = Mesh::createSphere(m_config.radius, 8);
    obj.transform.position = m_position;
    obj.material.baseColor = m_config.color;
    obj.material.emissive = 0.8f;  // Bright glow for visibility
    return obj;
}

} // namespace lst
