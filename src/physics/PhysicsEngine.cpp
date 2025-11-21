#include "PhysicsEngine.h"
#include <iostream>
#include <algorithm>
#include <cmath>

#if HAS_BULLET
#include <btBulletDynamicsCommon.h>
#endif

namespace lst {

PhysicsEngine::PhysicsEngine() = default;

PhysicsEngine::~PhysicsEngine() {
    shutdown();
}

bool PhysicsEngine::initialize() {
    if (m_initialized) return true;

#if HAS_BULLET
    // Initialize Bullet Physics
    btDefaultCollisionConfiguration* collisionConfig = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfig);
    btBroadphaseInterface* broadphase = new btDbvtBroadphase();
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver();

    btDiscreteDynamicsWorld* world = new btDiscreteDynamicsWorld(
        dispatcher, broadphase, solver, collisionConfig);
    world->setGravity(btVector3(m_gravity.x, m_gravity.y, m_gravity.z));

    m_bulletWorld = world;
    std::cout << "Physics engine initialized (Bullet)" << std::endl;
#else
    std::cout << "Physics engine initialized (simplified)" << std::endl;
#endif

    m_initialized = true;
    return true;
}

void PhysicsEngine::shutdown() {
#if HAS_BULLET
    if (m_bulletWorld) {
        btDiscreteDynamicsWorld* world = static_cast<btDiscreteDynamicsWorld*>(m_bulletWorld);

        // Remove and delete all rigid bodies
        for (int i = world->getNumCollisionObjects() - 1; i >= 0; i--) {
            btCollisionObject* obj = world->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState()) {
                delete body->getMotionState();
            }
            world->removeCollisionObject(obj);
            delete obj;
        }

        delete world->getConstraintSolver();
        delete world->getBroadphase();
        delete world->getDispatcher();
        delete static_cast<btDefaultCollisionConfiguration*>(
            static_cast<btCollisionDispatcher*>(world->getDispatcher())->getCollisionConfiguration());
        delete world;
        m_bulletWorld = nullptr;
    }
#endif

    m_bodies.clear();
    m_initialized = false;
    std::cout << "Physics engine shutdown" << std::endl;
}

void PhysicsEngine::step(float deltaTime) {
    if (!m_initialized) return;

#if HAS_BULLET
    btDiscreteDynamicsWorld* world = static_cast<btDiscreteDynamicsWorld*>(m_bulletWorld);
    world->stepSimulation(deltaTime, 10, m_fixedTimeStep);

    // Update our body list from Bullet
    // ... (sync transforms)
#else
    // Simplified physics step
    m_accumulator += deltaTime;

    while (m_accumulator >= m_fixedTimeStep) {
        // Update dynamic bodies
        for (auto& body : m_bodies) {
            if (!body.active) continue;

            if (body.config.bodyType == BodyType::Dynamic) {
                // Apply gravity
                body.linearVelocity = body.linearVelocity + m_gravity * m_fixedTimeStep;

                // Update position
                body.transform.position = body.transform.position + body.linearVelocity * m_fixedTimeStep;

                // Simple ground collision
                float groundY = body.config.shapeSize.y;
                if (body.transform.position.y < groundY) {
                    body.transform.position.y = groundY;
                    body.linearVelocity.y = -body.linearVelocity.y * body.config.restitution;

                    // Apply friction
                    body.linearVelocity.x *= (1.0f - body.config.friction * m_fixedTimeStep);
                    body.linearVelocity.z *= (1.0f - body.config.friction * m_fixedTimeStep);
                }
            } else if (body.config.bodyType == BodyType::Kinematic) {
                // Interpolate towards target
                float t = 0.3f;
                body.transform.position = body.transform.position + (body.targetTransform.position - body.transform.position) * t;
                // TODO: Interpolate orientation using slerp
            }
        }

        // Detect collisions
        detectCollisions();

        m_accumulator -= m_fixedTimeStep;
    }
#endif
}

bool PhysicsEngine::addBody(const PhysicsBodyConfig& config) {
    // Check for duplicate
    for (const auto& body : m_bodies) {
        if (body.config.name == config.name) {
            std::cerr << "Physics body already exists: " << config.name << std::endl;
            return false;
        }
    }

#if HAS_BULLET
    btDiscreteDynamicsWorld* world = static_cast<btDiscreteDynamicsWorld*>(m_bulletWorld);

    // Create collision shape
    btCollisionShape* shape = nullptr;
    switch (config.shapeType) {
        case ShapeType::Box:
            shape = new btBoxShape(btVector3(
                config.shapeSize.x, config.shapeSize.y, config.shapeSize.z));
            break;
        case ShapeType::Sphere:
            shape = new btSphereShape(config.shapeSize.x);
            break;
        case ShapeType::Capsule:
            shape = new btCapsuleShape(config.shapeSize.x, config.shapeSize.y);
            break;
        default:
            shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
            break;
    }

    // Create motion state
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(
        config.initialTransform.position.x,
        config.initialTransform.position.y,
        config.initialTransform.position.z));
    startTransform.setRotation(btQuaternion(
        config.initialTransform.orientation.x,
        config.initialTransform.orientation.y,
        config.initialTransform.orientation.z,
        config.initialTransform.orientation.w));

    btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);

    // Calculate inertia for dynamic bodies
    btVector3 localInertia(0, 0, 0);
    float mass = (config.bodyType == BodyType::Dynamic) ? config.mass : 0.0f;
    if (mass > 0) {
        shape->calculateLocalInertia(mass, localInertia);
    }

    // Create rigid body
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
    rbInfo.m_friction = config.friction;
    rbInfo.m_restitution = config.restitution;

    btRigidBody* body = new btRigidBody(rbInfo);

    if (config.bodyType == BodyType::Kinematic) {
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
    }

    world->addRigidBody(body, config.collisionGroup, config.collisionMask);
#endif

    // Add to our list
    PhysicsBody physBody;
    physBody.config = config;
    physBody.transform = config.initialTransform;
    physBody.targetTransform = config.initialTransform;
    physBody.previousTransform = config.initialTransform;
    physBody.active = true;

    m_bodies.push_back(physBody);
    std::cout << "Added physics body: " << config.name << std::endl;

    return true;
}

bool PhysicsEngine::removeBody(const std::string& name) {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [&name](const PhysicsBody& b) { return b.config.name == name; });

    if (it == m_bodies.end()) return false;

#if HAS_BULLET
    // Remove from Bullet world
    // ... (would need to track Bullet body pointers)
#endif

    m_bodies.erase(it);
    std::cout << "Removed physics body: " << name << std::endl;
    return true;
}

void PhysicsEngine::clearBodies() {
    m_bodies.clear();
#if HAS_BULLET
    // Clear Bullet world
    // ...
#endif
}

bool PhysicsEngine::setBodyTransform(const std::string& name, const Transform& transform) {
    for (auto& body : m_bodies) {
        if (body.config.name == name) {
            body.transform = transform;
            return true;
        }
    }
    return false;
}

bool PhysicsEngine::getBodyTransform(const std::string& name, Transform& transform) const {
    for (const auto& body : m_bodies) {
        if (body.config.name == name) {
            transform = body.transform;
            return true;
        }
    }
    return false;
}

bool PhysicsEngine::setBodyVelocity(const std::string& name, const Vec3& linear, const Vec3& angular) {
    for (auto& body : m_bodies) {
        if (body.config.name == name) {
            body.linearVelocity = linear;
            body.angularVelocity = angular;
            return true;
        }
    }
    return false;
}

bool PhysicsEngine::getBodyVelocity(const std::string& name, Vec3& linear, Vec3& angular) const {
    for (const auto& body : m_bodies) {
        if (body.config.name == name) {
            linear = body.linearVelocity;
            angular = body.angularVelocity;
            return true;
        }
    }
    return false;
}

bool PhysicsEngine::updateKinematicBody(const std::string& name, const Transform& target) {
    for (auto& body : m_bodies) {
        if (body.config.name == name && body.config.bodyType == BodyType::Kinematic) {
            body.previousTransform = body.transform;
            body.targetTransform = target;
            return true;
        }
    }
    return false;
}

bool PhysicsEngine::raycast(const Vec3& origin, const Vec3& direction, float maxDistance, RayHit& hit) const {
    Vec3 end = origin + direction.normalized() * maxDistance;

#if HAS_BULLET
    btDiscreteDynamicsWorld* world = static_cast<btDiscreteDynamicsWorld*>(m_bulletWorld);
    btVector3 from(origin.x, origin.y, origin.z);
    btVector3 to(end.x, end.y, end.z);

    btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
    world->rayTest(from, to, rayCallback);

    if (rayCallback.hasHit()) {
        hit.hitPoint = Vec3(
            rayCallback.m_hitPointWorld.x(),
            rayCallback.m_hitPointWorld.y(),
            rayCallback.m_hitPointWorld.z());
        hit.hitNormal = Vec3(
            rayCallback.m_hitNormalWorld.x(),
            rayCallback.m_hitNormalWorld.y(),
            rayCallback.m_hitNormalWorld.z());
        hit.distance = (hit.hitPoint - origin).length();
        // hit.bodyName would need to be looked up from the collision object
        return true;
    }
    return false;
#else
    // Simplified raycast against all bodies
    float closestDist = maxDistance;
    bool found = false;

    for (const auto& body : m_bodies) {
        if (!body.active) continue;

        // Simple sphere intersection test (approximate all shapes as spheres)
        Vec3 toCenter = body.transform.position - origin;
        float tca = Vec3::dot(toCenter, direction.normalized());

        if (tca < 0) continue;

        float d2 = Vec3::dot(toCenter, toCenter) - tca * tca;
        float radius = std::max({body.config.shapeSize.x, body.config.shapeSize.y, body.config.shapeSize.z});

        if (d2 > radius * radius) continue;

        float thc = std::sqrt(radius * radius - d2);
        float t = tca - thc;

        if (t < closestDist && t > 0) {
            closestDist = t;
            hit.bodyName = body.config.name;
            hit.hitPoint = origin + direction.normalized() * t;
            hit.hitNormal = (hit.hitPoint - body.transform.position).normalized();
            hit.distance = t;
            found = true;
        }
    }

    return found;
#endif
}

std::vector<PhysicsEngine::RayHit> PhysicsEngine::raycastAll(const Vec3& origin, const Vec3& direction, float maxDistance) const {
    std::vector<RayHit> hits;
    RayHit hit;
    if (raycast(origin, direction, maxDistance, hit)) {
        hits.push_back(hit);
    }
    return hits;
}

void PhysicsEngine::detectCollisions() {
    if (!m_collisionCallback) return;

    for (size_t i = 0; i < m_bodies.size(); i++) {
        for (size_t j = i + 1; j < m_bodies.size(); j++) {
            if (!m_bodies[i].active || !m_bodies[j].active) continue;

            // Check collision masks
            if ((m_bodies[i].config.collisionMask & m_bodies[j].config.collisionGroup) == 0) continue;
            if ((m_bodies[j].config.collisionMask & m_bodies[i].config.collisionGroup) == 0) continue;

            CollisionInfo info;
            if (checkCollision(m_bodies[i], m_bodies[j], info)) {
                m_collisionCallback(info);
            }
        }
    }
}

bool PhysicsEngine::checkCollision(const PhysicsBody& a, const PhysicsBody& b, CollisionInfo& info) const {
    info.bodyA = a.config.name;
    info.bodyB = b.config.name;

    // Dispatch based on shape types
    if (a.config.shapeType == ShapeType::Box && b.config.shapeType == ShapeType::Box) {
        return checkBoxBox(a, b, info);
    } else if (a.config.shapeType == ShapeType::Sphere && b.config.shapeType == ShapeType::Sphere) {
        return checkSphereSphere(a, b, info);
    } else if (a.config.shapeType == ShapeType::Box && b.config.shapeType == ShapeType::Sphere) {
        return checkBoxSphere(a, b, info);
    } else if (a.config.shapeType == ShapeType::Sphere && b.config.shapeType == ShapeType::Box) {
        bool result = checkBoxSphere(b, a, info);
        std::swap(info.bodyA, info.bodyB);
        return result;
    } else if (a.config.shapeType == ShapeType::Capsule && b.config.shapeType == ShapeType::Sphere) {
        return checkCapsuleSphere(a, b, info);
    } else if (a.config.shapeType == ShapeType::Sphere && b.config.shapeType == ShapeType::Capsule) {
        bool result = checkCapsuleSphere(b, a, info);
        std::swap(info.bodyA, info.bodyB);
        return result;
    }

    return false;
}

bool PhysicsEngine::checkBoxBox(const PhysicsBody& a, const PhysicsBody& b, CollisionInfo& info) const {
    // Simple AABB test (ignores rotation)
    Vec3 aMin = a.transform.position - a.config.shapeSize;
    Vec3 aMax = a.transform.position + a.config.shapeSize;
    Vec3 bMin = b.transform.position - b.config.shapeSize;
    Vec3 bMax = b.transform.position + b.config.shapeSize;

    if (aMax.x < bMin.x || aMin.x > bMax.x) return false;
    if (aMax.y < bMin.y || aMin.y > bMax.y) return false;
    if (aMax.z < bMin.z || aMin.z > bMax.z) return false;

    // Calculate contact point and penetration
    info.contactPoint = (a.transform.position + b.transform.position) * 0.5f;
    info.contactNormal = (b.transform.position - a.transform.position).normalized();
    info.penetrationDepth = 0.01f; // Simplified

    return true;
}

bool PhysicsEngine::checkSphereSphere(const PhysicsBody& a, const PhysicsBody& b, CollisionInfo& info) const {
    Vec3 diff = b.transform.position - a.transform.position;
    float dist = diff.length();
    float radiusSum = a.config.shapeSize.x + b.config.shapeSize.x;

    if (dist > radiusSum) return false;

    info.contactNormal = diff.normalized();
    info.contactPoint = a.transform.position + info.contactNormal * a.config.shapeSize.x;
    info.penetrationDepth = radiusSum - dist;

    return true;
}

bool PhysicsEngine::checkBoxSphere(const PhysicsBody& box, const PhysicsBody& sphere, CollisionInfo& info) const {
    // Find closest point on box to sphere center
    Vec3 closestPoint;
    closestPoint.x = std::max(box.transform.position.x - box.config.shapeSize.x,
                              std::min(sphere.transform.position.x,
                                       box.transform.position.x + box.config.shapeSize.x));
    closestPoint.y = std::max(box.transform.position.y - box.config.shapeSize.y,
                              std::min(sphere.transform.position.y,
                                       box.transform.position.y + box.config.shapeSize.y));
    closestPoint.z = std::max(box.transform.position.z - box.config.shapeSize.z,
                              std::min(sphere.transform.position.z,
                                       box.transform.position.z + box.config.shapeSize.z));

    Vec3 diff = sphere.transform.position - closestPoint;
    float dist = diff.length();

    if (dist > sphere.config.shapeSize.x) return false;

    info.contactPoint = closestPoint;
    info.contactNormal = diff.normalized();
    info.penetrationDepth = sphere.config.shapeSize.x - dist;

    return true;
}

bool PhysicsEngine::checkCapsuleSphere(const PhysicsBody& capsule, const PhysicsBody& sphere, CollisionInfo& info) const {
    // Simplified: treat capsule as a line segment
    Vec3 capsuleDir = capsule.transform.orientation.rotate(Vec3(0, 1, 0));
    float halfHeight = capsule.config.shapeSize.y * 0.5f;

    Vec3 capsuleStart = capsule.transform.position - capsuleDir * halfHeight;
    Vec3 capsuleEnd = capsule.transform.position + capsuleDir * halfHeight;

    // Find closest point on capsule line to sphere center
    Vec3 lineDir = capsuleEnd - capsuleStart;
    float lineLen = lineDir.length();
    lineDir = lineDir * (1.0f / lineLen);

    float t = Vec3::dot(sphere.transform.position - capsuleStart, lineDir);
    t = std::max(0.0f, std::min(lineLen, t));

    Vec3 closestPoint = capsuleStart + lineDir * t;
    Vec3 diff = sphere.transform.position - closestPoint;
    float dist = diff.length();
    float radiusSum = capsule.config.shapeSize.x + sphere.config.shapeSize.x;

    if (dist > radiusSum) return false;

    info.contactNormal = diff.normalized();
    info.contactPoint = closestPoint + info.contactNormal * capsule.config.shapeSize.x;
    info.penetrationDepth = radiusSum - dist;

    return true;
}

} // namespace lst
