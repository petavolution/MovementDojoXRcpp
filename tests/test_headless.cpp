/**
 * test_headless.cpp - Headless System Tests
 *
 * Tests core systems (Math, Analytics, Progression) without OpenXR
 * or any graphics dependencies. Used for CI/CD and CLI testing.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <string>

// Include Types.h for Vec3, Quat, etc.
#include "../include/Types.h"

// Include the systems we're testing
#include "../src/analytics/MovementAnalytics.h"
#include "../src/progression/ProgressionSystem.h"

namespace lst {

// =============================================================================
// Math Tests
// =============================================================================

bool testVec3Operations() {
    std::cout << "  Testing Vec3 operations... ";

    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);

    // Addition
    Vec3 sum = a + b;
    if (std::abs(sum.x - 5.0f) > 0.001f ||
        std::abs(sum.y - 7.0f) > 0.001f ||
        std::abs(sum.z - 9.0f) > 0.001f) {
        std::cout << "FAILED (addition)" << std::endl;
        return false;
    }

    // Subtraction
    Vec3 diff = b - a;
    if (std::abs(diff.x - 3.0f) > 0.001f ||
        std::abs(diff.y - 3.0f) > 0.001f ||
        std::abs(diff.z - 3.0f) > 0.001f) {
        std::cout << "FAILED (subtraction)" << std::endl;
        return false;
    }

    // Scalar multiplication
    Vec3 scaled = a * 2.0f;
    if (std::abs(scaled.x - 2.0f) > 0.001f ||
        std::abs(scaled.y - 4.0f) > 0.001f ||
        std::abs(scaled.z - 6.0f) > 0.001f) {
        std::cout << "FAILED (scalar mult)" << std::endl;
        return false;
    }

    // Dot product
    float dot = Vec3::dot(a, b);
    if (std::abs(dot - 32.0f) > 0.001f) {
        std::cout << "FAILED (dot product)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool testQuatOperations() {
    std::cout << "  Testing Quat operations... ";

    // Identity quaternion
    Quat identity = Quat::identity();

    // Rotate a vector by identity (should be unchanged)
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = identity.rotate(v);

    if (std::abs(rotated.x - 1.0f) > 0.001f ||
        std::abs(rotated.y) > 0.001f ||
        std::abs(rotated.z) > 0.001f) {
        std::cout << "FAILED (identity rotation)" << std::endl;
        return false;
    }

    // 90 degree rotation around Z axis
    Quat rotZ = Quat::fromAxisAngle(Vec3(0, 0, 1), 3.14159265359f / 2.0f);
    Vec3 rotatedZ = rotZ.rotate(v);

    // (1, 0, 0) rotated 90deg around Z should give (0, 1, 0)
    if (std::abs(rotatedZ.x) > 0.01f ||
        std::abs(rotatedZ.y - 1.0f) > 0.01f ||
        std::abs(rotatedZ.z) > 0.01f) {
        std::cout << "FAILED (90deg Z rotation)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool testTransform() {
    std::cout << "  Testing Transform... ";

    Transform t;
    t.position = Vec3(1.0f, 2.0f, 3.0f);
    t.orientation = Quat::identity();
    t.scale = Vec3(2.0f, 2.0f, 2.0f);

    Vec3 point(1.0f, 0.0f, 0.0f);
    Vec3 transformed = t.transformPoint(point);

    // Should be scaled (2) + position offset (1, 2, 3) = (3, 2, 3)
    if (std::abs(transformed.x - 3.0f) > 0.01f ||
        std::abs(transformed.y - 2.0f) > 0.01f ||
        std::abs(transformed.z - 3.0f) > 0.01f) {
        std::cout << "FAILED" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// =============================================================================
// Analytics Tests
// =============================================================================

bool testAnalyticsBasic() {
    std::cout << "  Testing MovementAnalytics basic... ";

    MovementAnalytics analytics;
    MovementAnalyticsConfig config;
    analytics.initialize(config);

    analytics.startSession();

    // Record some samples
    Transform headPose;
    headPose.position = Vec3(0, 1.6f, 0);
    ControllerState left, right;
    left.hand = Hand::Left;
    left.isTracked = true;
    left.position = Vec3(-0.3f, 1.0f, -0.4f);
    right.hand = Hand::Right;
    right.isTracked = true;
    right.position = Vec3(0.3f, 1.0f, -0.4f);

    for (int i = 0; i < 10; i++) {
        analytics.recordSample(0.016, headPose, left, right);
        // Move controllers slightly
        left.position.x += 0.01f;
        right.position.x -= 0.01f;
    }

    // Check session is recording
    if (!analytics.isRecording()) {
        std::cout << "FAILED (not recording)" << std::endl;
        return false;
    }

    analytics.endSession();

    // Check we got a summary
    const SessionSummary& summary = analytics.getSessionSummary();
    if (summary.totalSamples < 10) {
        std::cout << "FAILED (sample count)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool testAnalyticsIntensity() {
    std::cout << "  Testing movement intensity... ";

    MovementAnalytics analytics;
    MovementAnalyticsConfig config;
    analytics.initialize(config);
    analytics.startSession();

    Transform headPose;
    headPose.position = Vec3(0, 1.6f, 0);
    ControllerState left, right;
    left.hand = Hand::Left;
    left.isTracked = true;
    right.hand = Hand::Right;
    right.isTracked = true;

    // Simulate high-speed movement
    float t = 0;
    for (int i = 0; i < 30; i++) {
        left.position = Vec3(-0.3f + std::sin(t * 3.0f) * 0.5f, 1.0f, -0.4f);
        right.position = Vec3(0.3f + std::cos(t * 3.0f) * 0.5f, 1.0f, -0.4f);
        left.velocity = Vec3(std::cos(t * 3.0f) * 1.5f, 0, 0);
        right.velocity = Vec3(-std::sin(t * 3.0f) * 1.5f, 0, 0);

        analytics.recordSample(0.016, headPose, left, right);
        t += 0.016f;
    }

    float intensity = analytics.getCurrentIntensity();
    // Should have some intensity from the movement
    if (intensity < 0.0f) {
        std::cout << "FAILED (negative intensity)" << std::endl;
        return false;
    }

    analytics.endSession();
    std::cout << "PASSED (intensity=" << intensity << ")" << std::endl;
    return true;
}

// =============================================================================
// Progression Tests
// =============================================================================

bool testProgressionXP() {
    std::cout << "  Testing ProgressionSystem XP... ";

    ProgressionSystem progression;
    progression.createNewProfile("TestPlayer");

    int initialLevel = progression.getCurrentLevel();
    int initialXP = progression.getCurrentXP();

    // Add XP
    progression.addXP(100);

    int newXP = progression.getCurrentXP();
    if (newXP <= initialXP) {
        std::cout << "FAILED (XP not added)" << std::endl;
        return false;
    }

    std::cout << "PASSED (XP: " << initialXP << " -> " << newXP << ")" << std::endl;
    return true;
}

bool testProgressionLevelUp() {
    std::cout << "  Testing level progression... ";

    ProgressionSystem progression;
    progression.createNewProfile("TestPlayer");

    int initialLevel = progression.getCurrentLevel();

    // Add enough XP for several levels
    for (int i = 0; i < 100; i++) {
        progression.addXP(100);
    }

    int newLevel = progression.getCurrentLevel();
    if (newLevel <= initialLevel) {
        std::cout << "FAILED (no level up)" << std::endl;
        return false;
    }

    std::cout << "PASSED (Level: " << initialLevel << " -> " << newLevel << ")" << std::endl;
    return true;
}

bool testProgressionSession() {
    std::cout << "  Testing session tracking... ";

    ProgressionSystem progression;
    progression.createNewProfile("TestPlayer");

    progression.startSession();
    progression.addXP(50);
    progression.endSession();

    const PlayerStats& stats = progression.getStats();
    if (stats.totalSessions < 1) {
        std::cout << "FAILED (session not counted)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

} // namespace lst

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Movement Dojo - Headless System Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    int passed = 0;
    int failed = 0;

    // Math tests
    std::cout << "Math Tests:" << std::endl;
    if (lst::testVec3Operations()) passed++; else failed++;
    if (lst::testQuatOperations()) passed++; else failed++;
    if (lst::testTransform()) passed++; else failed++;
    std::cout << std::endl;

    // Analytics tests
    std::cout << "Analytics Tests:" << std::endl;
    if (lst::testAnalyticsBasic()) passed++; else failed++;
    if (lst::testAnalyticsIntensity()) passed++; else failed++;
    std::cout << std::endl;

    // Progression tests
    std::cout << "Progression Tests:" << std::endl;
    if (lst::testProgressionXP()) passed++; else failed++;
    if (lst::testProgressionLevelUp()) passed++; else failed++;
    if (lst::testProgressionSession()) passed++; else failed++;
    std::cout << std::endl;

    // Summary
    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return failed > 0 ? 1 : 0;
}
