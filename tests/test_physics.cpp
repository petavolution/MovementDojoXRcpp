/**
 * Physics Engine Tests
 *
 * Comprehensive tests for collision detection, rigid body simulation,
 * and kinematic body handling.
 */

#include "../src/physics/PhysicsEngine.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <functional>

namespace {

int testsPassed = 0;
int testsFailed = 0;

bool approxEqual(float a, float b, float epsilon = 0.0001f) {
    return std::abs(a - b) < epsilon;
}

bool approxEqual(const lst::Vec3& a, const lst::Vec3& b, float epsilon = 0.0001f) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

void runTest(const std::string& name, std::function<bool()> test) {
    std::cout << "  Testing " << name << "... ";
    try {
        if (test()) {
            std::cout << "PASSED" << std::endl;
            testsPassed++;
        } else {
            std::cout << "FAILED" << std::endl;
            testsFailed++;
        }
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        testsFailed++;
    }
}

// =============================================================================
// Initialization Tests
// =============================================================================

bool testInitialization() {
    lst::PhysicsEngine physics;
    if (physics.isInitialized()) return false;

    bool result = physics.initialize();
    if (!result) return false;
    if (!physics.isInitialized()) return false;

    physics.shutdown();
    if (physics.isInitialized()) return false;

    return true;
}

bool testDefaultGravity() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::Vec3 gravity = physics.getGravity();
    if (!approxEqual(gravity.y, -9.81f, 0.01f)) return false;

    physics.shutdown();
    return true;
}

bool testSetGravity() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::Vec3 newGravity(0, -15.0f, 0);
    physics.setGravity(newGravity);

    lst::Vec3 gravity = physics.getGravity();
    if (!approxEqual(gravity.y, -15.0f)) return false;

    physics.shutdown();
    return true;
}

// =============================================================================
// Body Management Tests
// =============================================================================

bool testAddStaticBody() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::PhysicsBodyConfig config;
    config.name = "floor";
    config.bodyType = lst::BodyType::Static;
    config.shapeType = lst::ShapeType::Box;
    config.shapeSize = lst::Vec3(10, 0.1f, 10);
    config.initialTransform.position = lst::Vec3(0, 0, 0);

    bool result = physics.addBody(config);
    if (!result) return false;
    if (physics.getBodyCount() != 1) return false;

    physics.shutdown();
    return true;
}

bool testAddDynamicBody() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::PhysicsBodyConfig config;
    config.name = "ball";
    config.bodyType = lst::BodyType::Dynamic;
    config.shapeType = lst::ShapeType::Sphere;
    config.shapeSize = lst::Vec3(0.1f, 0, 0);  // radius
    config.mass = 1.0f;
    config.initialTransform.position = lst::Vec3(0, 2, 0);

    bool result = physics.addBody(config);
    if (!result) return false;
    if (physics.getBodyCount() != 1) return false;

    physics.shutdown();
    return true;
}

bool testAddKinematicBody() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::PhysicsBodyConfig config;
    config.name = "controller";
    config.bodyType = lst::BodyType::Kinematic;
    config.shapeType = lst::ShapeType::Sphere;
    config.shapeSize = lst::Vec3(0.05f, 0, 0);
    config.enableCCD = true;
    config.ccdMotionThreshold = 0.001f;

    bool result = physics.addBody(config);
    if (!result) return false;

    physics.shutdown();
    return true;
}

bool testRemoveBody() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::PhysicsBodyConfig config;
    config.name = "temp";
    config.bodyType = lst::BodyType::Static;

    physics.addBody(config);
    if (physics.getBodyCount() != 1) return false;

    bool result = physics.removeBody("temp");
    if (!result) return false;
    if (physics.getBodyCount() != 0) return false;

    // Removing non-existent body should fail
    result = physics.removeBody("nonexistent");
    if (result) return false;

    physics.shutdown();
    return true;
}

bool testClearBodies() {
    lst::PhysicsEngine physics;
    physics.initialize();

    for (int i = 0; i < 10; i++) {
        lst::PhysicsBodyConfig config;
        config.name = "body_" + std::to_string(i);
        physics.addBody(config);
    }

    if (physics.getBodyCount() != 10) return false;

    physics.clearBodies();
    if (physics.getBodyCount() != 0) return false;

    physics.shutdown();
    return true;
}

// =============================================================================
// Transform Tests
// =============================================================================

bool testGetSetTransform() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::PhysicsBodyConfig config;
    config.name = "box";
    config.bodyType = lst::BodyType::Dynamic;
    config.initialTransform.position = lst::Vec3(1, 2, 3);
    physics.addBody(config);

    lst::Transform transform;
    bool result = physics.getBodyTransform("box", transform);
    if (!result) return false;
    if (!approxEqual(transform.position, lst::Vec3(1, 2, 3))) return false;

    lst::Transform newTransform;
    newTransform.position = lst::Vec3(5, 6, 7);
    result = physics.setBodyTransform("box", newTransform);
    if (!result) return false;

    result = physics.getBodyTransform("box", transform);
    if (!result) return false;
    if (!approxEqual(transform.position, lst::Vec3(5, 6, 7))) return false;

    physics.shutdown();
    return true;
}

bool testKinematicUpdate() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::PhysicsBodyConfig config;
    config.name = "saber";
    config.bodyType = lst::BodyType::Kinematic;
    config.shapeType = lst::ShapeType::Capsule;
    config.shapeSize = lst::Vec3(0.02f, 1.0f, 0);  // radius, height
    config.enableCCD = true;
    physics.addBody(config);

    // Simulate controller movement
    for (int i = 0; i < 10; i++) {
        lst::Transform target;
        target.position = lst::Vec3(0, 1.0f + i * 0.1f, -0.5f);
        physics.updateKinematicBody("saber", target);
        physics.step(1.0f / 90.0f);
    }

    lst::Transform finalTransform;
    physics.getBodyTransform("saber", finalTransform);
    // Kinematic body should have moved toward targets
    if (finalTransform.position.y < 1.5f) return false;

    physics.shutdown();
    return true;
}

// =============================================================================
// Simulation Tests
// =============================================================================

bool testGravitySimulation() {
    lst::PhysicsEngine physics;
    physics.initialize();
    physics.setGravity(lst::Vec3(0, -10.0f, 0));

    lst::PhysicsBodyConfig config;
    config.name = "falling_ball";
    config.bodyType = lst::BodyType::Dynamic;
    config.shapeType = lst::ShapeType::Sphere;
    config.shapeSize = lst::Vec3(0.1f, 0, 0);
    config.mass = 1.0f;
    config.initialTransform.position = lst::Vec3(0, 5, 0);
    physics.addBody(config);

    // Simulate 1 second at 90Hz
    for (int i = 0; i < 90; i++) {
        physics.step(1.0f / 90.0f);
    }

    lst::Transform transform;
    physics.getBodyTransform("falling_ball", transform);

    // Ball should have fallen (y should be lower than 5)
    if (transform.position.y >= 5.0f) return false;
    // Approximate free fall: y = y0 - 0.5*g*t^2 = 5 - 0.5*10*1 = 0
    // With simple integration it won't be exact
    if (transform.position.y > 2.0f) return false;

    physics.shutdown();
    return true;
}

bool testVelocityApplication() {
    lst::PhysicsEngine physics;
    physics.initialize();
    physics.setGravity(lst::Vec3(0, 0, 0));  // Disable gravity

    lst::PhysicsBodyConfig config;
    config.name = "projectile";
    config.bodyType = lst::BodyType::Dynamic;
    config.mass = 1.0f;
    config.initialTransform.position = lst::Vec3(0, 0, 0);
    physics.addBody(config);

    physics.setBodyVelocity("projectile", lst::Vec3(1, 0, 0), lst::Vec3(0, 0, 0));

    // Simulate 1 second
    for (int i = 0; i < 90; i++) {
        physics.step(1.0f / 90.0f);
    }

    lst::Transform transform;
    physics.getBodyTransform("projectile", transform);

    // Should have moved ~1 unit in x direction
    if (transform.position.x < 0.5f) return false;
    if (std::abs(transform.position.y) > 0.1f) return false;
    if (std::abs(transform.position.z) > 0.1f) return false;

    physics.shutdown();
    return true;
}

// =============================================================================
// Collision Detection Tests
// =============================================================================

bool testSphereSphereCollision() {
    lst::PhysicsEngine physics;
    physics.initialize();
    physics.setGravity(lst::Vec3(0, 0, 0));

    bool collisionDetected = false;
    physics.setCollisionCallback([&](const lst::CollisionInfo& info) {
        collisionDetected = true;
    });

    // Two overlapping spheres
    lst::PhysicsBodyConfig sphere1;
    sphere1.name = "sphere1";
    sphere1.bodyType = lst::BodyType::Dynamic;
    sphere1.shapeType = lst::ShapeType::Sphere;
    sphere1.shapeSize = lst::Vec3(0.5f, 0, 0);
    sphere1.initialTransform.position = lst::Vec3(0, 0, 0);
    physics.addBody(sphere1);

    lst::PhysicsBodyConfig sphere2;
    sphere2.name = "sphere2";
    sphere2.bodyType = lst::BodyType::Dynamic;
    sphere2.shapeType = lst::ShapeType::Sphere;
    sphere2.shapeSize = lst::Vec3(0.5f, 0, 0);
    sphere2.initialTransform.position = lst::Vec3(0.5f, 0, 0);  // Overlapping
    physics.addBody(sphere2);

    physics.step(1.0f / 90.0f);

    physics.shutdown();
    return collisionDetected;
}

bool testBoxBoxCollision() {
    lst::PhysicsEngine physics;
    physics.initialize();
    physics.setGravity(lst::Vec3(0, 0, 0));

    bool collisionDetected = false;
    physics.setCollisionCallback([&](const lst::CollisionInfo& info) {
        collisionDetected = true;
    });

    lst::PhysicsBodyConfig box1;
    box1.name = "box1";
    box1.bodyType = lst::BodyType::Static;
    box1.shapeType = lst::ShapeType::Box;
    box1.shapeSize = lst::Vec3(1, 1, 1);
    box1.initialTransform.position = lst::Vec3(0, 0, 0);
    physics.addBody(box1);

    lst::PhysicsBodyConfig box2;
    box2.name = "box2";
    box2.bodyType = lst::BodyType::Dynamic;
    box2.shapeType = lst::ShapeType::Box;
    box2.shapeSize = lst::Vec3(0.5f, 0.5f, 0.5f);
    box2.initialTransform.position = lst::Vec3(0.8f, 0, 0);  // Overlapping
    physics.addBody(box2);

    physics.step(1.0f / 90.0f);

    physics.shutdown();
    return collisionDetected;
}

bool testNoCollisionWhenSeparated() {
    lst::PhysicsEngine physics;
    physics.initialize();
    physics.setGravity(lst::Vec3(0, 0, 0));

    bool collisionDetected = false;
    physics.setCollisionCallback([&](const lst::CollisionInfo& info) {
        collisionDetected = true;
    });

    lst::PhysicsBodyConfig sphere1;
    sphere1.name = "sphere1";
    sphere1.shapeType = lst::ShapeType::Sphere;
    sphere1.shapeSize = lst::Vec3(0.5f, 0, 0);
    sphere1.initialTransform.position = lst::Vec3(0, 0, 0);
    physics.addBody(sphere1);

    lst::PhysicsBodyConfig sphere2;
    sphere2.name = "sphere2";
    sphere2.shapeType = lst::ShapeType::Sphere;
    sphere2.shapeSize = lst::Vec3(0.5f, 0, 0);
    sphere2.initialTransform.position = lst::Vec3(5, 0, 0);  // Far apart
    physics.addBody(sphere2);

    physics.step(1.0f / 90.0f);

    physics.shutdown();
    return !collisionDetected;
}

// =============================================================================
// Raycast Tests
// =============================================================================

bool testRaycastHit() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::PhysicsBodyConfig config;
    config.name = "target";
    config.shapeType = lst::ShapeType::Sphere;
    config.shapeSize = lst::Vec3(0.5f, 0, 0);
    config.initialTransform.position = lst::Vec3(0, 0, -2);
    physics.addBody(config);

    lst::PhysicsEngine::RayHit hit;
    bool result = physics.raycast(
        lst::Vec3(0, 0, 0),      // origin
        lst::Vec3(0, 0, -1),     // direction
        10.0f,                    // max distance
        hit
    );

    physics.shutdown();

    if (!result) return false;
    if (hit.bodyName != "target") return false;
    if (hit.distance < 1.0f || hit.distance > 3.0f) return false;

    return true;
}

bool testRaycastMiss() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::PhysicsBodyConfig config;
    config.name = "target";
    config.shapeType = lst::ShapeType::Sphere;
    config.shapeSize = lst::Vec3(0.5f, 0, 0);
    config.initialTransform.position = lst::Vec3(0, 0, -2);
    physics.addBody(config);

    lst::PhysicsEngine::RayHit hit;
    bool result = physics.raycast(
        lst::Vec3(0, 0, 0),      // origin
        lst::Vec3(0, 1, 0),      // direction (wrong way)
        10.0f,
        hit
    );

    physics.shutdown();
    return !result;  // Should miss
}

bool testRaycastAll() {
    lst::PhysicsEngine physics;
    physics.initialize();

    for (int i = 1; i <= 3; i++) {
        lst::PhysicsBodyConfig config;
        config.name = "target_" + std::to_string(i);
        config.shapeType = lst::ShapeType::Sphere;
        config.shapeSize = lst::Vec3(0.3f, 0, 0);
        config.initialTransform.position = lst::Vec3(0, 0, -i * 2.0f);
        physics.addBody(config);
    }

    auto hits = physics.raycastAll(
        lst::Vec3(0, 0, 0),
        lst::Vec3(0, 0, -1),
        20.0f
    );

    physics.shutdown();
    return hits.size() == 3;
}

// =============================================================================
// CCD Tests
// =============================================================================

bool testCCDConfiguration() {
    lst::PhysicsEngine physics;
    physics.initialize();

    lst::PhysicsBodyConfig config;
    config.name = "fast_saber";
    config.bodyType = lst::BodyType::Kinematic;
    config.shapeType = lst::ShapeType::Capsule;
    config.shapeSize = lst::Vec3(0.02f, 1.0f, 0);
    config.enableCCD = true;
    config.ccdMotionThreshold = 0.001f;
    config.ccdSweptSphereRadius = 0.05f;

    bool result = physics.addBody(config);
    physics.shutdown();
    return result;
}

bool testFastMovingCollision() {
    lst::PhysicsEngine physics;
    physics.initialize();
    physics.setGravity(lst::Vec3(0, 0, 0));

    bool collisionDetected = false;
    physics.setCollisionCallback([&](const lst::CollisionInfo& info) {
        collisionDetected = true;
    });

    // Static target
    lst::PhysicsBodyConfig target;
    target.name = "target";
    target.bodyType = lst::BodyType::Static;
    target.shapeType = lst::ShapeType::Sphere;
    target.shapeSize = lst::Vec3(0.1f, 0, 0);
    target.initialTransform.position = lst::Vec3(0, 1, -0.5f);
    physics.addBody(target);

    // Fast-moving saber blade
    lst::PhysicsBodyConfig blade;
    blade.name = "blade";
    blade.bodyType = lst::BodyType::Kinematic;
    blade.shapeType = lst::ShapeType::Capsule;
    blade.shapeSize = lst::Vec3(0.02f, 1.0f, 0);
    blade.enableCCD = true;
    blade.initialTransform.position = lst::Vec3(0, 0, -0.5f);
    physics.addBody(blade);

    // Simulate fast swing through target
    for (int i = 0; i < 10; i++) {
        lst::Transform target_tf;
        target_tf.position = lst::Vec3(0, i * 0.3f, -0.5f);
        physics.updateKinematicBody("blade", target_tf);
        physics.step(1.0f / 90.0f);
    }

    physics.shutdown();
    return collisionDetected;
}

// =============================================================================
// Collision Layer Tests
// =============================================================================

bool testCollisionGroups() {
    lst::PhysicsEngine physics;
    physics.initialize();

    bool playerEnemyCollision = false;
    bool playerEnvironmentCollision = false;

    physics.setCollisionCallback([&](const lst::CollisionInfo& info) {
        if ((info.bodyA == "player" && info.bodyB == "enemy") ||
            (info.bodyA == "enemy" && info.bodyB == "player")) {
            playerEnemyCollision = true;
        }
        if ((info.bodyA == "player" && info.bodyB == "wall") ||
            (info.bodyA == "wall" && info.bodyB == "player")) {
            playerEnvironmentCollision = true;
        }
    });

    // Player - group 1, collides with groups 2 and 4
    lst::PhysicsBodyConfig player;
    player.name = "player";
    player.collisionGroup = 1;
    player.collisionMask = 2 | 4;
    player.shapeType = lst::ShapeType::Sphere;
    player.shapeSize = lst::Vec3(0.5f, 0, 0);
    player.initialTransform.position = lst::Vec3(0, 0, 0);
    physics.addBody(player);

    // Enemy - group 2
    lst::PhysicsBodyConfig enemy;
    enemy.name = "enemy";
    enemy.collisionGroup = 2;
    enemy.shapeType = lst::ShapeType::Sphere;
    enemy.shapeSize = lst::Vec3(0.5f, 0, 0);
    enemy.initialTransform.position = lst::Vec3(0.5f, 0, 0);
    physics.addBody(enemy);

    // Wall - group 4
    lst::PhysicsBodyConfig wall;
    wall.name = "wall";
    wall.collisionGroup = 4;
    wall.shapeType = lst::ShapeType::Box;
    wall.shapeSize = lst::Vec3(1, 1, 1);
    wall.initialTransform.position = lst::Vec3(-0.5f, 0, 0);
    physics.addBody(wall);

    physics.step(1.0f / 90.0f);

    physics.shutdown();
    return playerEnemyCollision && playerEnvironmentCollision;
}

// =============================================================================
// Stress Tests
// =============================================================================

bool testManyBodies() {
    lst::PhysicsEngine physics;
    physics.initialize();

    // Add 100 bodies
    for (int i = 0; i < 100; i++) {
        lst::PhysicsBodyConfig config;
        config.name = "body_" + std::to_string(i);
        config.bodyType = lst::BodyType::Dynamic;
        config.shapeType = lst::ShapeType::Sphere;
        config.shapeSize = lst::Vec3(0.1f, 0, 0);
        config.mass = 1.0f;
        config.initialTransform.position = lst::Vec3(
            (i % 10) * 0.5f,
            (i / 10) * 0.5f,
            0
        );
        physics.addBody(config);
    }

    if (physics.getBodyCount() != 100) return false;

    // Simulate for 1 second
    for (int i = 0; i < 90; i++) {
        physics.step(1.0f / 90.0f);
    }

    physics.shutdown();
    return true;
}

bool testPerformance() {
    lst::PhysicsEngine physics;
    physics.initialize();

    // Add bodies
    for (int i = 0; i < 50; i++) {
        lst::PhysicsBodyConfig config;
        config.name = "body_" + std::to_string(i);
        config.bodyType = lst::BodyType::Dynamic;
        config.shapeType = lst::ShapeType::Sphere;
        config.shapeSize = lst::Vec3(0.1f, 0, 0);
        config.initialTransform.position = lst::Vec3(
            (float)(rand() % 100) / 10.0f,
            (float)(rand() % 100) / 10.0f,
            (float)(rand() % 100) / 10.0f
        );
        physics.addBody(config);
    }

    // Measure time for 900 steps (10 seconds at 90Hz)
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 900; i++) {
        physics.step(1.0f / 90.0f);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    physics.shutdown();

    // Should complete in under 1 second for 50 bodies over 10 simulated seconds
    std::cout << "[" << duration.count() << "ms] ";
    return duration.count() < 1000;
}

}  // namespace

int runPhysicsTests() {
    std::cout << "Physics Engine Tests" << std::endl;
    std::cout << "--------------------" << std::endl;

    testsPassed = 0;
    testsFailed = 0;

    // Initialization
    runTest("initialization", testInitialization);
    runTest("default gravity", testDefaultGravity);
    runTest("set gravity", testSetGravity);

    // Body management
    runTest("add static body", testAddStaticBody);
    runTest("add dynamic body", testAddDynamicBody);
    runTest("add kinematic body", testAddKinematicBody);
    runTest("remove body", testRemoveBody);
    runTest("clear bodies", testClearBodies);

    // Transforms
    runTest("get/set transform", testGetSetTransform);
    runTest("kinematic update", testKinematicUpdate);

    // Simulation
    runTest("gravity simulation", testGravitySimulation);
    runTest("velocity application", testVelocityApplication);

    // Collision detection
    runTest("sphere-sphere collision", testSphereSphereCollision);
    runTest("box-box collision", testBoxBoxCollision);
    runTest("no collision when separated", testNoCollisionWhenSeparated);

    // Raycasting
    runTest("raycast hit", testRaycastHit);
    runTest("raycast miss", testRaycastMiss);
    runTest("raycast all", testRaycastAll);

    // CCD
    runTest("CCD configuration", testCCDConfiguration);
    runTest("fast moving collision", testFastMovingCollision);

    // Collision groups
    runTest("collision groups", testCollisionGroups);

    // Stress tests
    runTest("many bodies", testManyBodies);
    runTest("performance", testPerformance);

    std::cout << std::endl;
    std::cout << "Physics Tests: " << testsPassed << " passed, " << testsFailed << " failed" << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
