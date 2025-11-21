#pragma once

#include "Types.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace lst {

/**
 * PhysicsEngine - Handles collision detection and physics simulation
 *
 * For Stage 1-2, this is a simplified implementation.
 * When HAS_BULLET is defined, it wraps Bullet Physics.
 *
 * Handles:
 * - Static and dynamic rigid bodies
 * - Collision shapes (box, sphere, capsule)
 * - Kinematic bodies for controller-driven objects
 * - Collision detection and callbacks
 * - Ray casting
 */

// Collision shape types
enum class ShapeType {
    Box,
    Sphere,
    Capsule,
    Mesh
};

// Body type
enum class BodyType {
    Static,
    Dynamic,
    Kinematic
};

// Collision info
struct CollisionInfo {
    std::string bodyA;
    std::string bodyB;
    Vec3 contactPoint;
    Vec3 contactNormal;
    float penetrationDepth;
};

// Physics body configuration
struct PhysicsBodyConfig {
    std::string name;
    BodyType bodyType = BodyType::Static;
    ShapeType shapeType = ShapeType::Box;
    Vec3 shapeSize = Vec3(1, 1, 1);  // For box: half-extents, sphere: radius in x, capsule: radius/height
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    Transform initialTransform;
    int collisionGroup = 1;
    int collisionMask = -1;  // Collide with all groups
};

// Collision callback
using CollisionCallback = std::function<void(const CollisionInfo& info)>;

class PhysicsEngine {
public:
    PhysicsEngine();
    ~PhysicsEngine();

    // Initialization
    bool initialize();
    void shutdown();

    // Simulation
    void step(float deltaTime);

    // Body management
    bool addBody(const PhysicsBodyConfig& config);
    bool removeBody(const std::string& name);
    void clearBodies();

    // Body state
    bool setBodyTransform(const std::string& name, const Transform& transform);
    bool getBodyTransform(const std::string& name, Transform& transform) const;
    bool setBodyVelocity(const std::string& name, const Vec3& linear, const Vec3& angular);
    bool getBodyVelocity(const std::string& name, Vec3& linear, Vec3& angular) const;

    // Kinematic body update (for controller-driven objects)
    bool updateKinematicBody(const std::string& name, const Transform& target);

    // Ray casting
    struct RayHit {
        std::string bodyName;
        Vec3 hitPoint;
        Vec3 hitNormal;
        float distance;
    };
    bool raycast(const Vec3& origin, const Vec3& direction, float maxDistance, RayHit& hit) const;
    std::vector<RayHit> raycastAll(const Vec3& origin, const Vec3& direction, float maxDistance) const;

    // Collision callbacks
    void setCollisionCallback(CollisionCallback callback) { m_collisionCallback = callback; }

    // Gravity
    void setGravity(const Vec3& gravity) { m_gravity = gravity; }
    Vec3 getGravity() const { return m_gravity; }

    // Debug
    bool isInitialized() const { return m_initialized; }
    int getBodyCount() const { return static_cast<int>(m_bodies.size()); }

private:
    struct PhysicsBody {
        PhysicsBodyConfig config;
        Transform transform;
        Vec3 linearVelocity;
        Vec3 angularVelocity;
        bool active;

        // For kinematic interpolation
        Transform previousTransform;
        Transform targetTransform;
    };

    void detectCollisions();
    bool checkCollision(const PhysicsBody& a, const PhysicsBody& b, CollisionInfo& info) const;
    bool checkBoxBox(const PhysicsBody& a, const PhysicsBody& b, CollisionInfo& info) const;
    bool checkSphereSphere(const PhysicsBody& a, const PhysicsBody& b, CollisionInfo& info) const;
    bool checkBoxSphere(const PhysicsBody& box, const PhysicsBody& sphere, CollisionInfo& info) const;
    bool checkCapsuleSphere(const PhysicsBody& capsule, const PhysicsBody& sphere, CollisionInfo& info) const;

    bool m_initialized = false;
    Vec3 m_gravity = Vec3(0, -9.81f, 0);
    float m_fixedTimeStep = 1.0f / 90.0f;
    float m_accumulator = 0.0f;

    std::vector<PhysicsBody> m_bodies;
    CollisionCallback m_collisionCallback;

    // Bullet Physics handles (only used when HAS_BULLET=1)
    void* m_bulletWorld = nullptr;
};

} // namespace lst
