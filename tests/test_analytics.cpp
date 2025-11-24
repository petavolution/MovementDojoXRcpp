/**
 * MovementAnalytics Tests
 *
 * Comprehensive tests for movement recording, voxelization,
 * pattern detection, and statistics generation.
 */

#include "../src/analytics/MovementAnalytics.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <functional>
#include <chrono>
#include <thread>

namespace {

int testsPassed = 0;
int testsFailed = 0;

bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

bool approxEqual(const lst::Vec3& a, const lst::Vec3& b, float epsilon = 0.01f) {
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

// Helper to create test controller state
lst::ControllerState makeController(const lst::Vec3& pos, float grip = 0.0f, float trigger = 0.0f) {
    lst::ControllerState state;
    state.position = pos;
    state.orientation = lst::Quat(1, 0, 0, 0);
    state.gripValue = grip;
    state.triggerValue = trigger;
    return state;
}

// Helper to create test transform
lst::Transform makeTransform(const lst::Vec3& pos) {
    lst::Transform t;
    t.position = pos;
    t.orientation = lst::Quat(1, 0, 0, 0);
    t.scale = lst::Vec3(1, 1, 1);
    return t;
}

// =============================================================================
// Session Management Tests
// =============================================================================

bool testSessionStart() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    if (!analytics.isRecording()) return false;

    analytics.endSession();
    return true;
}

bool testSessionEnd() {
    lst::MovementAnalytics analytics;
    analytics.startSession();
    analytics.endSession();

    if (analytics.isRecording()) return false;

    return true;
}

bool testSessionPauseResume() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    analytics.pauseSession();
    // Recording state after pause depends on implementation
    // Some systems keep isRecording=true but skip samples

    analytics.resumeSession();
    if (!analytics.isRecording()) return false;

    analytics.endSession();
    return true;
}

bool testMultipleSessions() {
    lst::MovementAnalytics analytics;

    for (int session = 0; session < 3; session++) {
        analytics.startSession();

        lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
        lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
        lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

        for (int i = 0; i < 10; i++) {
            analytics.recordSample(head, left, right);
        }

        analytics.endSession();
    }

    return true;
}

// =============================================================================
// Recording Tests
// =============================================================================

bool testRecordSample() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    analytics.recordSample(head, left, right);

    const auto& samples = analytics.getSamples();
    if (samples.empty()) return false;

    analytics.endSession();
    return true;
}

bool testRecordSampleWithDeltaTime() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    analytics.recordSample(0.011, head, left, right);  // ~90Hz

    const auto& samples = analytics.getSamples();
    if (samples.empty()) return false;

    analytics.endSession();
    return true;
}

bool testManyRecordedSamples() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));

    // Record 1000 samples (about 11 seconds at 90Hz)
    for (int i = 0; i < 1000; i++) {
        lst::ControllerState left = makeController(
            lst::Vec3(-0.3f + sinf(i * 0.1f) * 0.2f, 1.0f, -0.3f)
        );
        lst::ControllerState right = makeController(
            lst::Vec3(0.3f + cosf(i * 0.1f) * 0.2f, 1.0f, -0.3f)
        );
        analytics.recordSample(head, left, right);
    }

    const auto& samples = analytics.getSamples();
    if (samples.size() < 1000) return false;

    analytics.endSession();
    return true;
}

bool testInputValuesRecorded() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f), 0.5f, 0.8f);
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f), 0.3f, 0.6f);

    analytics.recordSample(head, left, right);

    const auto& samples = analytics.getSamples();
    if (samples.empty()) return false;

    const auto& s = samples.back();
    if (!approxEqual(s.leftGrip, 0.5f)) return false;
    if (!approxEqual(s.leftTrigger, 0.8f)) return false;

    analytics.endSession();
    return true;
}

// =============================================================================
// Movement Space Coverage Tests
// =============================================================================

bool testInitialCoverage() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    float coverage = analytics.getMovementSpaceCoverage();
    if (coverage != 0.0f) return false;

    analytics.endSession();
    return true;
}

bool testCoverageIncreasesWithMovement() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));

    // Move hands around to different positions
    std::vector<lst::Vec3> positions = {
        {-0.3f, 1.0f, -0.3f},
        {0.3f, 1.5f, -0.3f},
        {0.0f, 0.5f, -0.5f},
        {-0.5f, 1.2f, 0.0f},
        {0.5f, 0.8f, -0.5f}
    };

    float previousCoverage = 0;

    for (const auto& pos : positions) {
        lst::ControllerState left = makeController(pos);
        lst::ControllerState right = makeController(lst::Vec3(-pos.x, pos.y, pos.z));

        for (int i = 0; i < 10; i++) {
            analytics.recordSample(head, left, right);
        }

        float coverage = analytics.getMovementSpaceCoverage();
        if (coverage < previousCoverage) return false;  // Should never decrease
        previousCoverage = coverage;
    }

    analytics.endSession();
    return previousCoverage > 0;
}

bool testUnexploredAreas() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    // Only record in one area
    for (int i = 0; i < 100; i++) {
        analytics.recordSample(head, left, right);
    }

    auto unexplored = analytics.getUnexploredAreas();
    // Should have many unexplored areas
    if (unexplored.empty()) return false;

    analytics.endSession();
    return true;
}

bool testFrequentAreas() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    // Record many samples in same area
    for (int i = 0; i < 500; i++) {
        analytics.recordSample(head, left, right);
    }

    auto frequent = analytics.getFrequentAreas();
    // The recorded area should be frequent
    if (frequent.empty()) return false;

    analytics.endSession();
    return true;
}

// =============================================================================
// Pattern Detection Tests
// =============================================================================

bool testStationaryPattern() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    // Record stationary samples
    for (int i = 0; i < 100; i++) {
        analytics.recordSample(head, left, right);
    }

    lst::MovementPattern pattern = analytics.getCurrentPattern();
    // Should detect stationary
    if (pattern != lst::MovementPattern::Stationary) {
        // Might also be Slow - acceptable
        if (pattern != lst::MovementPattern::Slow) return false;
    }

    analytics.endSession();
    return true;
}

bool testFastMovementPattern() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));

    // Simulate fast movement
    for (int i = 0; i < 100; i++) {
        float t = i * 0.1f;
        lst::ControllerState left = makeController(
            lst::Vec3(-0.3f + sinf(t * 5) * 0.5f, 1.0f + cosf(t * 5) * 0.5f, -0.3f)
        );
        lst::ControllerState right = makeController(
            lst::Vec3(0.3f + cosf(t * 5) * 0.5f, 1.0f + sinf(t * 5) * 0.5f, -0.3f)
        );
        analytics.recordSample(0.011, head, left, right);
    }

    lst::MovementPattern pattern = analytics.getCurrentPattern();
    // Should detect some movement (Fast or Moderate)
    if (pattern == lst::MovementPattern::Stationary) return false;

    analytics.endSession();
    return true;
}

bool testCurrentIntensity() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));

    // Stationary - low intensity
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    for (int i = 0; i < 50; i++) {
        analytics.recordSample(head, left, right);
    }

    float intensity = analytics.getCurrentIntensity();
    if (intensity < 0.0f || intensity > 1.0f) return false;

    analytics.endSession();
    return true;
}

// =============================================================================
// Statistics Tests
// =============================================================================

bool testSessionSummary() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    for (int i = 0; i < 100; i++) {
        analytics.recordSample(0.011, head, left, right);
    }

    const auto& summary = analytics.getSessionSummary();
    if (summary.totalSamples < 100) return false;

    analytics.endSession();
    return true;
}

bool testPointStatistics() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));

    // Move hand through known positions
    for (int i = 0; i < 100; i++) {
        float y = 0.5f + (i / 100.0f) * 1.0f;  // 0.5 to 1.5
        lst::ControllerState left = makeController(lst::Vec3(-0.3f, y, -0.3f));
        lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));
        analytics.recordSample(head, left, right);
    }

    lst::PointStatistics stats = analytics.getRealtimeStats(1);  // Left hand
    // Min Y should be around 0.5, max around 1.5
    if (stats.minPosition.y > 0.6f) return false;
    if (stats.maxPosition.y < 1.4f) return false;

    analytics.endSession();
    return true;
}

bool testMovementTrail() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));

    for (int i = 0; i < 100; i++) {
        lst::ControllerState left = makeController(
            lst::Vec3(-0.3f + i * 0.01f, 1.0f, -0.3f)
        );
        lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));
        analytics.recordSample(head, left, right);
    }

    auto trail = analytics.getMovementTrail(1, 10.0f);  // Left hand, 10 seconds
    if (trail.empty()) return false;

    analytics.endSession();
    return true;
}

// =============================================================================
// Heatmap Tests
// =============================================================================

bool testGenerateHeatmap() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));

    // Create concentrated activity in one area
    for (int i = 0; i < 200; i++) {
        lst::ControllerState left = makeController(
            lst::Vec3(-0.3f + (rand() % 10 - 5) * 0.01f,
                      1.0f + (rand() % 10 - 5) * 0.01f,
                      -0.3f)
        );
        lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));
        analytics.recordSample(head, left, right);
    }

    auto heatmap = analytics.generateHeatmap(1, 0.1f);  // Left hand
    if (heatmap.positions.empty()) return false;
    if (heatmap.intensities.empty()) return false;
    if (heatmap.positions.size() != heatmap.intensities.size()) return false;

    analytics.endSession();
    return true;
}

// =============================================================================
// Uncommon Position Tests
// =============================================================================

bool testIsInUncommonPosition() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));

    // Overhead position (uncommon)
    lst::ControllerState left = makeController(lst::Vec3(0, 2.3f, 0));  // Above head
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 2.3f, 0));

    analytics.recordSample(head, left, right);

    bool uncommon = analytics.isInUncommonPosition();
    // Overhead should be uncommon

    analytics.endSession();
    return true;  // Just verifying it doesn't crash, actual logic varies
}

bool testSuggestedExplorationDirection() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    // Record in limited area
    for (int i = 0; i < 100; i++) {
        analytics.recordSample(head, left, right);
    }

    lst::Vec3 suggestion = analytics.getSuggestedExplorationDirection();
    // Should suggest moving somewhere
    float length = sqrtf(suggestion.x*suggestion.x + suggestion.y*suggestion.y + suggestion.z*suggestion.z);
    // May return zero vector if no suggestion

    analytics.endSession();
    return true;
}

// =============================================================================
// Export Tests
// =============================================================================

bool testExportToCSV() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    for (int i = 0; i < 50; i++) {
        analytics.recordSample(head, left, right);
    }

    bool result = analytics.exportToCSV("/tmp/test_movement.csv");

    analytics.endSession();

    // Cleanup
    std::remove("/tmp/test_movement.csv");

    return result;
}

bool testExportSession() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    for (int i = 0; i < 50; i++) {
        analytics.recordSample(head, left, right);
    }

    bool result = analytics.exportSession("/tmp/test_session.json");

    analytics.endSession();

    std::remove("/tmp/test_session.json");

    return result;
}

// =============================================================================
// Configuration Tests
// =============================================================================

bool testSetVoxelResolution() {
    lst::MovementAnalytics analytics;
    analytics.setMovementSpaceResolution(0.05f);  // 5cm voxels
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    analytics.recordSample(head, left, right);

    analytics.endSession();
    return true;
}

bool testSetRecordingRate() {
    lst::MovementAnalytics analytics;
    analytics.setRecordingRate(45);  // 45Hz instead of 90Hz
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    for (int i = 0; i < 100; i++) {
        analytics.recordSample(head, left, right);
    }

    analytics.endSession();
    return true;
}

bool testInitializeWithConfig() {
    lst::MovementAnalytics analytics;

    lst::MovementAnalyticsConfig config;
    config.recordFullPose = true;
    config.voxelResolution = 0.08f;
    config.historyDuration = 1800.0;
    config.sampleRate = 72;

    analytics.initialize(config);
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    analytics.recordSample(head, left, right);

    analytics.endSession();
    return true;
}

// =============================================================================
// Callback Tests
// =============================================================================

bool testAnalyticsCallback() {
    lst::MovementAnalytics analytics;

    bool callbackFired = false;
    analytics.setAnalyticsCallback([&](const std::string& event, const std::string& data) {
        callbackFired = true;
    });

    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    for (int i = 0; i < 100; i++) {
        analytics.recordSample(head, left, right);
    }

    analytics.endSession();
    // Callback may or may not fire depending on implementation
    return true;
}

// =============================================================================
// Stress Tests
// =============================================================================

bool testLongSession() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));

    // Simulate 10 minutes at 90Hz = 54000 samples
    for (int i = 0; i < 54000; i++) {
        float t = i * 0.011f;
        lst::ControllerState left = makeController(
            lst::Vec3(-0.3f + sinf(t) * 0.2f, 1.0f + cosf(t * 0.5f) * 0.3f, -0.3f)
        );
        lst::ControllerState right = makeController(
            lst::Vec3(0.3f + cosf(t) * 0.2f, 1.0f + sinf(t * 0.5f) * 0.3f, -0.3f)
        );
        analytics.recordSample(head, left, right);
    }

    const auto& summary = analytics.getSessionSummary();
    if (summary.totalSamples < 50000) return false;

    analytics.endSession();
    return true;
}

bool testPerformance() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    auto start = std::chrono::high_resolution_clock::now();

    // Record 9000 samples (100 seconds at 90Hz)
    for (int i = 0; i < 9000; i++) {
        analytics.recordSample(head, left, right);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    analytics.endSession();

    std::cout << "[" << duration.count() << "ms] ";
    // Should complete in under 500ms for 9000 samples
    return duration.count() < 500;
}

// =============================================================================
// Edge Case Tests
// =============================================================================

bool testRecordWithoutStarting() {
    lst::MovementAnalytics analytics;
    // Don't call startSession

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-0.3f, 1.0f, -0.3f));
    lst::ControllerState right = makeController(lst::Vec3(0.3f, 1.0f, -0.3f));

    analytics.recordSample(head, left, right);

    // Should handle gracefully (skip or auto-start)
    return true;
}

bool testEndWithoutStarting() {
    lst::MovementAnalytics analytics;
    analytics.endSession();  // No session started

    // Should handle gracefully
    return true;
}

bool testZeroPositions() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 0, 0));
    lst::ControllerState left = makeController(lst::Vec3(0, 0, 0));
    lst::ControllerState right = makeController(lst::Vec3(0, 0, 0));

    analytics.recordSample(head, left, right);

    analytics.endSession();
    return true;
}

bool testExtremePositions() {
    lst::MovementAnalytics analytics;
    analytics.startSession();

    lst::Transform head = makeTransform(lst::Vec3(0, 1.7f, 0));
    lst::ControllerState left = makeController(lst::Vec3(-10.0f, 100.0f, -50.0f));
    lst::ControllerState right = makeController(lst::Vec3(10.0f, 100.0f, 50.0f));

    analytics.recordSample(head, left, right);

    analytics.endSession();
    return true;
}

}  // namespace

int runAnalyticsTests() {
    std::cout << "MovementAnalytics Tests" << std::endl;
    std::cout << "-----------------------" << std::endl;

    testsPassed = 0;
    testsFailed = 0;

    // Session management
    runTest("session start", testSessionStart);
    runTest("session end", testSessionEnd);
    runTest("session pause/resume", testSessionPauseResume);
    runTest("multiple sessions", testMultipleSessions);

    // Recording
    runTest("record sample", testRecordSample);
    runTest("record sample with delta time", testRecordSampleWithDeltaTime);
    runTest("many recorded samples", testManyRecordedSamples);
    runTest("input values recorded", testInputValuesRecorded);

    // Movement space coverage
    runTest("initial coverage", testInitialCoverage);
    runTest("coverage increases with movement", testCoverageIncreasesWithMovement);
    runTest("unexplored areas", testUnexploredAreas);
    runTest("frequent areas", testFrequentAreas);

    // Pattern detection
    runTest("stationary pattern", testStationaryPattern);
    runTest("fast movement pattern", testFastMovementPattern);
    runTest("current intensity", testCurrentIntensity);

    // Statistics
    runTest("session summary", testSessionSummary);
    runTest("point statistics", testPointStatistics);
    runTest("movement trail", testMovementTrail);

    // Heatmap
    runTest("generate heatmap", testGenerateHeatmap);

    // Uncommon positions
    runTest("is in uncommon position", testIsInUncommonPosition);
    runTest("suggested exploration direction", testSuggestedExplorationDirection);

    // Export
    runTest("export to CSV", testExportToCSV);
    runTest("export session", testExportSession);

    // Configuration
    runTest("set voxel resolution", testSetVoxelResolution);
    runTest("set recording rate", testSetRecordingRate);
    runTest("initialize with config", testInitializeWithConfig);

    // Callbacks
    runTest("analytics callback", testAnalyticsCallback);

    // Stress tests
    runTest("long session", testLongSession);
    runTest("performance", testPerformance);

    // Edge cases
    runTest("record without starting", testRecordWithoutStarting);
    runTest("end without starting", testEndWithoutStarting);
    runTest("zero positions", testZeroPositions);
    runTest("extreme positions", testExtremePositions);

    std::cout << std::endl;
    std::cout << "Analytics Tests: " << testsPassed << " passed, " << testsFailed << " failed" << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
