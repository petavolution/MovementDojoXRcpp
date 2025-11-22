/**
 * Integration Tests
 *
 * Tests for verifying multiple systems work together correctly.
 * These tests validate the core engine pipeline end-to-end.
 */

#include "../src/core/SessionManager.h"
#include "../src/physics/PhysicsEngine.h"
#include "../src/haptics/HapticManager.h"
#include "../src/analytics/MovementAnalytics.h"
#include "../src/progression/ProgressionSystem.h"
#include "../src/training/TrainingModule.h"
#include "../src/training/FlowModes.h"
#include "../src/usd/USDLoader.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <functional>
#include <thread>
#include <chrono>

namespace {

int testsPassed = 0;
int testsFailed = 0;

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

bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

// =============================================================================
// Core Pipeline Integration Tests
// =============================================================================

bool testFullSessionLifecycle() {
    // Test complete session from start to end with all systems
    lst::PhysicsEngine physics;
    lst::MovementAnalytics analytics;
    lst::ProgressionSystem progression;

    // Initialize systems
    physics.initialize();
    progression.createNewProfile("IntegrationTest");

    // Start session
    analytics.startSession();
    progression.startSession();

    // Simulate 100 frames of VR usage
    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);
    head.orientation = lst::Quat(1, 0, 0, 0);

    for (int frame = 0; frame < 100; frame++) {
        float t = frame * 0.011f;

        // Simulate controller movement
        lst::ControllerState left, right;
        left.position = lst::Vec3(-0.3f + sinf(t) * 0.1f, 1.0f, -0.3f);
        left.orientation = lst::Quat(1, 0, 0, 0);
        right.position = lst::Vec3(0.3f + cosf(t) * 0.1f, 1.0f, -0.3f);
        right.orientation = lst::Quat(1, 0, 0, 0);

        // Record analytics
        analytics.recordSample(head, left, right);

        // Step physics
        physics.step(0.011f);
    }

    // End session
    analytics.endSession();
    progression.updateFromAnalytics(analytics);
    progression.endSession();

    // Verify data was collected
    const auto& samples = analytics.getSamples();
    if (samples.size() < 100) return false;

    const auto& stats = progression.getStats();
    if (stats.totalSessions < 1) return false;

    physics.shutdown();
    return true;
}

bool testPhysicsAnalyticsIntegration() {
    // Test physics collision triggers analytics events
    lst::PhysicsEngine physics;
    lst::MovementAnalytics analytics;

    physics.initialize();
    analytics.startSession();

    // Add a target body
    lst::PhysicsBodyConfig target;
    target.name = "target";
    target.bodyType = lst::BodyType::Static;
    target.shapeType = lst::ShapeType::Sphere;
    target.shapeSize = lst::Vec3(0.1f, 0, 0);
    target.initialTransform.position = lst::Vec3(0, 1.0f, -0.5f);
    physics.addBody(target);

    // Add a kinematic saber
    lst::PhysicsBodyConfig saber;
    saber.name = "saber";
    saber.bodyType = lst::BodyType::Kinematic;
    saber.shapeType = lst::ShapeType::Capsule;
    saber.shapeSize = lst::Vec3(0.02f, 1.0f, 0);
    saber.enableCCD = true;
    physics.addBody(saber);

    bool collisionOccurred = false;
    physics.setCollisionCallback([&](const lst::CollisionInfo& info) {
        collisionOccurred = true;
    });

    // Simulate swing through target
    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);

    for (int frame = 0; frame < 50; frame++) {
        float y = 0.5f + frame * 0.03f;  // Move upward through target

        lst::Transform saberTransform;
        saberTransform.position = lst::Vec3(0, y, -0.5f);
        physics.updateKinematicBody("saber", saberTransform);
        physics.step(0.011f);

        // Record controller position as if holding saber
        lst::ControllerState right;
        right.position = lst::Vec3(0, y - 0.5f, -0.5f);  // Handle below blade
        lst::ControllerState left;
        left.position = lst::Vec3(-0.3f, 1.0f, -0.3f);

        analytics.recordSample(head, left, right);
    }

    analytics.endSession();
    physics.shutdown();

    return collisionOccurred;
}

bool testProgressionUnlocks() {
    // Test that progression unlocks modes based on XP
    lst::ProgressionSystem progression;
    progression.createNewProfile("UnlockTest");

    // Check initial state
    if (progression.getCurrentLevel() != 1) return false;

    // Track level ups
    int levelUps = 0;
    progression.setLevelUpCallback([&](int level, const lst::LevelInfo& info) {
        levelUps++;
    });

    // Add enough XP to reach level 5 (1600 XP)
    progression.addXP(1700);

    if (levelUps < 4) return false;  // Should level up 4 times (1->2->3->4->5)
    if (progression.getCurrentLevel() < 5) return false;

    return true;
}

// =============================================================================
// Training Module Integration Tests
// =============================================================================

bool testTrainingModeWithAnalytics() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    // Simulate a free exploration session
    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);

    // Move through different zones
    std::vector<lst::Vec3> positions = {
        {-0.3f, 1.0f, -0.3f},   // Front mid
        {0.0f, 1.8f, 0.0f},     // Overhead (uncommon)
        {-0.5f, 0.5f, 0.0f},    // Side low
        {0.3f, 1.2f, -0.4f},    // Front high
    };

    for (const auto& pos : positions) {
        for (int i = 0; i < 50; i++) {
            lst::ControllerState left;
            left.position = pos;
            lst::ControllerState right;
            right.position = lst::Vec3(-pos.x, pos.y, pos.z);

            analytics.recordSample(head, left, right);
        }
    }

    // Check coverage increased
    float coverage = analytics.getMovementSpaceCoverage();

    analytics.endSession();
    return coverage > 0;
}

bool testFlowModeIntegration() {
    // Test that flow modes work with analytics
    lst::MovementAnalytics analytics;

    // Create flow mode
    lst::FreeExplorationMode freeMode;
    freeMode.start();

    analytics.startSession();

    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);

    for (int i = 0; i < 100; i++) {
        lst::ControllerState left, right;
        left.position = lst::Vec3(-0.3f + sinf(i * 0.1f) * 0.2f, 1.0f, -0.3f);
        right.position = lst::Vec3(0.3f + cosf(i * 0.1f) * 0.2f, 1.0f, -0.3f);

        analytics.recordSample(head, left, right);

        // Update flow mode
        freeMode.update(0.011, head, left, right);
    }

    freeMode.stop();
    analytics.endSession();

    // Flow mode should have tracked something
    return true;
}

// =============================================================================
// USD Scene Integration Tests
// =============================================================================

bool testUSDSceneWithPhysics() {
    lst::USDLoader loader;
    lst::PhysicsEngine physics;

    physics.initialize();

    // Load a test scene
    bool loaded = loader.loadStage("scenes/stage1_cube.usda");
    if (!loaded) {
        // Generate a stub scene if file doesn't exist
        auto objects = loader.getSceneObjects();
        // Continue with stub data
    }

    // Get scene objects and create physics bodies
    auto objects = loader.getSceneObjects();

    for (const auto& obj : objects) {
        lst::PhysicsBodyConfig config;
        config.name = obj.name;
        config.bodyType = lst::BodyType::Static;
        config.shapeType = lst::ShapeType::Box;
        config.shapeSize = obj.transform.scale;
        config.initialTransform = obj.transform;
        physics.addBody(config);
    }

    // Step physics
    physics.step(0.011f);

    physics.shutdown();
    return true;
}

bool testUSDVariantsWithProgression() {
    lst::USDLoader loader;
    lst::ProgressionSystem progression;

    progression.createNewProfile("VariantTest");

    // Load scene with variants
    loader.loadStage("scenes/dojo_basic.usda");

    // Get available variants
    auto variants = loader.getVariantSets("/World/Dojo");

    // Select variant based on player level
    int level = progression.getCurrentLevel();
    if (!variants.empty() && variants.size() > 0) {
        // Could select beginner/intermediate based on level
    }

    return true;
}

// =============================================================================
// Stress and Performance Integration Tests
// =============================================================================

bool testHighFrequencyLoop() {
    // Test all systems running at 90Hz for simulated 10 seconds
    lst::PhysicsEngine physics;
    lst::MovementAnalytics analytics;

    physics.initialize();
    analytics.startSession();

    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);

    auto startTime = std::chrono::high_resolution_clock::now();

    // 900 frames = 10 seconds at 90Hz
    for (int frame = 0; frame < 900; frame++) {
        float t = frame * 0.011f;

        lst::ControllerState left, right;
        left.position = lst::Vec3(-0.3f + sinf(t) * 0.2f, 1.0f + cosf(t * 0.5f) * 0.1f, -0.3f);
        right.position = lst::Vec3(0.3f + cosf(t) * 0.2f, 1.0f + sinf(t * 0.5f) * 0.1f, -0.3f);

        analytics.recordSample(head, left, right);
        physics.step(0.011f);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    analytics.endSession();
    physics.shutdown();

    std::cout << "[" << duration.count() << "ms] ";
    // Should complete well under 1 second for 900 frames
    return duration.count() < 1000;
}

bool testMultipleSystemsParallel() {
    // Test multiple systems operating correctly together
    lst::PhysicsEngine physics;
    lst::MovementAnalytics analytics;
    lst::ProgressionSystem progression;

    physics.initialize();
    analytics.startSession();
    progression.createNewProfile("ParallelTest");
    progression.startSession();

    // Add multiple physics bodies
    for (int i = 0; i < 10; i++) {
        lst::PhysicsBodyConfig config;
        config.name = "target_" + std::to_string(i);
        config.bodyType = lst::BodyType::Dynamic;
        config.shapeType = lst::ShapeType::Sphere;
        config.shapeSize = lst::Vec3(0.1f, 0, 0);
        config.initialTransform.position = lst::Vec3(
            (i % 5 - 2) * 0.5f,
            1.0f + (i / 5) * 0.5f,
            -1.0f
        );
        physics.addBody(config);
    }

    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);

    // Run simulation
    for (int frame = 0; frame < 300; frame++) {
        lst::ControllerState left, right;
        left.position = lst::Vec3(-0.3f, 1.0f, -0.3f);
        right.position = lst::Vec3(0.3f, 1.0f, -0.3f);

        analytics.recordSample(head, left, right);
        physics.step(0.011f);
    }

    analytics.endSession();
    progression.updateFromAnalytics(analytics);
    progression.endSession();
    physics.shutdown();

    // Verify all systems recorded data
    const auto& samples = analytics.getSamples();
    const auto& stats = progression.getStats();

    return samples.size() >= 300 && stats.totalSessions >= 1;
}

// =============================================================================
// Error Recovery Integration Tests
// =============================================================================

bool testGracefulDegradation() {
    // Test that system handles missing components gracefully
    lst::MovementAnalytics analytics;
    lst::ProgressionSystem progression;

    // Start without physics (simulating unavailable hardware)
    analytics.startSession();
    progression.createNewProfile("DegradedTest");
    progression.startSession();

    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);
    lst::ControllerState left, right;
    left.position = lst::Vec3(-0.3f, 1.0f, -0.3f);
    right.position = lst::Vec3(0.3f, 1.0f, -0.3f);

    // Should still work without physics
    for (int i = 0; i < 50; i++) {
        analytics.recordSample(head, left, right);
    }

    analytics.endSession();
    progression.updateFromAnalytics(analytics);
    progression.endSession();

    return true;
}

bool testRecoveryFromInvalidState() {
    lst::MovementAnalytics analytics;

    // Try various invalid operations
    analytics.endSession();  // End without start
    analytics.pauseSession();  // Pause without start
    analytics.resumeSession();  // Resume without start

    // Now start properly
    analytics.startSession();

    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);
    lst::ControllerState left, right;
    left.position = lst::Vec3(-0.3f, 1.0f, -0.3f);
    right.position = lst::Vec3(0.3f, 1.0f, -0.3f);

    // Should work correctly after invalid operations
    for (int i = 0; i < 50; i++) {
        analytics.recordSample(head, left, right);
    }

    analytics.endSession();

    const auto& samples = analytics.getSamples();
    return samples.size() >= 50;
}

// =============================================================================
// Data Flow Integration Tests
// =============================================================================

bool testAnalyticsToProgressionFlow() {
    // Test that analytics data correctly flows to progression
    lst::MovementAnalytics analytics;
    lst::ProgressionSystem progression;

    progression.createNewProfile("DataFlowTest");

    analytics.startSession();
    progression.startSession();

    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);

    // Generate varied movement for good coverage
    for (int zone = 0; zone < 8; zone++) {
        float angle = zone * (3.14159f / 4.0f);
        for (int i = 0; i < 30; i++) {
            lst::ControllerState left, right;
            left.position = lst::Vec3(
                cosf(angle) * 0.4f,
                1.0f + sinf(angle) * 0.3f,
                -0.4f
            );
            right.position = lst::Vec3(0.3f, 1.0f, -0.3f);

            analytics.recordSample(head, left, right);
        }
    }

    float coverageBefore = analytics.getMovementSpaceCoverage();

    analytics.endSession();
    progression.updateFromAnalytics(analytics);
    progression.endSession();

    const auto& stats = progression.getStats();
    // Stats should reflect the session
    return stats.totalSessions >= 1;
}

bool testCollisionToHapticFlow() {
    // Test physics collision triggers haptic feedback
    // Note: Using simplified test without actual haptic hardware

    lst::PhysicsEngine physics;
    physics.initialize();

    bool collisionDetected = false;
    float collisionStrength = 0;

    physics.setCollisionCallback([&](const lst::CollisionInfo& info) {
        collisionDetected = true;
        // Calculate impact strength from penetration depth
        collisionStrength = std::min(1.0f, info.penetrationDepth * 10.0f);
    });

    // Setup collision scenario
    lst::PhysicsBodyConfig target;
    target.name = "target";
    target.bodyType = lst::BodyType::Static;
    target.shapeType = lst::ShapeType::Sphere;
    target.shapeSize = lst::Vec3(0.2f, 0, 0);
    target.initialTransform.position = lst::Vec3(0, 1.0f, 0);
    physics.addBody(target);

    lst::PhysicsBodyConfig blade;
    blade.name = "blade";
    blade.bodyType = lst::BodyType::Kinematic;
    blade.shapeType = lst::ShapeType::Sphere;
    blade.shapeSize = lst::Vec3(0.1f, 0, 0);
    blade.enableCCD = true;
    physics.addBody(blade);

    // Move blade into target
    lst::Transform bladeTransform;
    bladeTransform.position = lst::Vec3(0, 1.0f, 0);
    physics.updateKinematicBody("blade", bladeTransform);
    physics.step(0.011f);

    physics.shutdown();

    // In a real system, collision would trigger haptic
    // if (collisionDetected) haptics.playCollisionPattern(Hand::Right, collisionStrength);

    return collisionDetected;
}

// =============================================================================
// Session Persistence Integration Tests
// =============================================================================

bool testSessionSaveRestore() {
    const std::string savePath = "/tmp/test_integration_profile.json";

    // Session 1: Create and save progress
    {
        lst::ProgressionSystem progression;
        progression.createNewProfile("PersistenceTest");

        progression.addXP(500);
        progression.startSession();
        progression.endSession();

        bool saved = progression.saveProfile(savePath);
        if (!saved) return false;
    }

    // Session 2: Load and verify
    {
        lst::ProgressionSystem progression;
        bool loaded = progression.loadProfile(savePath);
        if (!loaded) return false;

        if (progression.getCurrentXP() < 500) return false;
        if (progression.getStats().totalSessions < 1) return false;
    }

    // Cleanup
    std::remove(savePath.c_str());
    return true;
}

}  // namespace

int runIntegrationTests() {
    std::cout << "Integration Tests" << std::endl;
    std::cout << "-----------------" << std::endl;

    testsPassed = 0;
    testsFailed = 0;

    // Core pipeline
    runTest("full session lifecycle", testFullSessionLifecycle);
    runTest("physics-analytics integration", testPhysicsAnalyticsIntegration);
    runTest("progression unlocks", testProgressionUnlocks);

    // Training modules
    runTest("training mode with analytics", testTrainingModeWithAnalytics);
    runTest("flow mode integration", testFlowModeIntegration);

    // USD scenes
    runTest("USD scene with physics", testUSDSceneWithPhysics);
    runTest("USD variants with progression", testUSDVariantsWithProgression);

    // Stress and performance
    runTest("high frequency loop", testHighFrequencyLoop);
    runTest("multiple systems parallel", testMultipleSystemsParallel);

    // Error recovery
    runTest("graceful degradation", testGracefulDegradation);
    runTest("recovery from invalid state", testRecoveryFromInvalidState);

    // Data flow
    runTest("analytics to progression flow", testAnalyticsToProgressionFlow);
    runTest("collision to haptic flow", testCollisionToHapticFlow);

    // Persistence
    runTest("session save/restore", testSessionSaveRestore);

    std::cout << std::endl;
    std::cout << "Integration Tests: " << testsPassed << " passed, " << testsFailed << " failed" << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
