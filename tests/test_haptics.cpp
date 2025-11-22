/**
 * HapticManager Tests
 *
 * Tests haptic feedback patterns, queuing, and timing.
 * Uses mock Input to verify haptic requests without hardware.
 */

#include "../src/haptics/HapticManager.h"
#include "../src/core/Input.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <functional>
#include <vector>

namespace {

int testsPassed = 0;
int testsFailed = 0;

// Mock Input class for testing haptics without hardware
class MockInput : public lst::Input {
public:
    struct HapticRequest {
        lst::Hand hand;
        float intensity;
        float duration;
        double timestamp;
    };

    std::vector<HapticRequest> hapticRequests;
    double currentTime = 0.0;

    void clearRequests() { hapticRequests.clear(); }

    // Override triggerHaptic to record requests
    void triggerHaptic(lst::Hand hand, float intensity, float duration) override {
        hapticRequests.push_back({hand, intensity, duration, currentTime});
    }

    void advanceTime(double dt) { currentTime += dt; }
};

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
// Initialization Tests
// =============================================================================

bool testInitialization() {
    lst::HapticManager haptics;
    MockInput mockInput;

    bool result = haptics.initialize(&mockInput);
    if (!result) return false;

    haptics.shutdown();
    return true;
}

bool testInitializeWithNull() {
    lst::HapticManager haptics;
    bool result = haptics.initialize(nullptr);
    // Should handle null gracefully (either fail or work without haptics)
    return true;  // Just checking it doesn't crash
}

// =============================================================================
// Basic Haptic Triggers
// =============================================================================

bool testPulse() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.pulse(lst::Hand::Left, 0.5f, 0.1f);
    haptics.update(0.01);

    if (mockInput.hapticRequests.empty()) return false;

    auto& req = mockInput.hapticRequests[0];
    if (req.hand != lst::Hand::Left) return false;
    if (!approxEqual(req.intensity, 0.5f)) return false;

    haptics.shutdown();
    return true;
}

bool testBuzz() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.buzz(lst::Hand::Right, 0.3f, 0.05f);
    haptics.update(0.01);

    if (mockInput.hapticRequests.empty()) return false;

    auto& req = mockInput.hapticRequests[0];
    if (req.hand != lst::Hand::Right) return false;
    if (!approxEqual(req.intensity, 0.3f)) return false;

    haptics.shutdown();
    return true;
}

bool testRamp() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.ramp(lst::Hand::Left, 0.0f, 1.0f, 0.5f);

    // Update multiple times to see ramp effect
    for (int i = 0; i < 10; i++) {
        haptics.update(0.05);
        mockInput.advanceTime(0.05);
    }

    // Should have multiple requests with increasing intensity
    if (mockInput.hapticRequests.size() < 2) return false;

    // First request should be lower intensity than later ones
    float firstIntensity = mockInput.hapticRequests.front().intensity;
    float lastIntensity = mockInput.hapticRequests.back().intensity;

    haptics.shutdown();
    return lastIntensity >= firstIntensity;
}

// =============================================================================
// Predefined Pattern Tests
// =============================================================================

bool testSuccessPattern() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.playSuccessPattern(lst::Hand::Right);

    // Update enough to play full pattern
    for (int i = 0; i < 50; i++) {
        haptics.update(0.02);
        mockInput.advanceTime(0.02);
    }

    // Should have generated multiple haptic events
    haptics.shutdown();
    return mockInput.hapticRequests.size() >= 2;
}

bool testFailurePattern() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.playFailurePattern(lst::Hand::Left);

    for (int i = 0; i < 50; i++) {
        haptics.update(0.02);
    }

    haptics.shutdown();
    return mockInput.hapticRequests.size() >= 1;
}

bool testCollisionPattern() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    // Test various impact strengths
    haptics.playCollisionPattern(lst::Hand::Right, 0.2f);  // Light
    haptics.update(0.1);
    mockInput.clearRequests();

    haptics.playCollisionPattern(lst::Hand::Right, 0.8f);  // Heavy
    haptics.update(0.1);

    if (mockInput.hapticRequests.empty()) return false;

    // Heavy impact should have higher intensity
    float heavyIntensity = mockInput.hapticRequests[0].intensity;

    haptics.shutdown();
    return heavyIntensity > 0.5f;
}

bool testSaberActivatePattern() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.playSaberActivatePattern(lst::Hand::Right);

    for (int i = 0; i < 30; i++) {
        haptics.update(0.02);
    }

    haptics.shutdown();
    return mockInput.hapticRequests.size() >= 1;
}

bool testSaberDeactivatePattern() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.playSaberDeactivatePattern(lst::Hand::Left);

    for (int i = 0; i < 30; i++) {
        haptics.update(0.02);
    }

    haptics.shutdown();
    return mockInput.hapticRequests.size() >= 1;
}

bool testGuidanceNudge() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.playGuidanceNudge(lst::Hand::Left, 0.3f);
    haptics.update(0.1);

    if (mockInput.hapticRequests.empty()) return false;

    // Guidance nudge should be subtle (low intensity)
    float intensity = mockInput.hapticRequests[0].intensity;

    haptics.shutdown();
    return intensity <= 0.5f;
}

// =============================================================================
// Custom Pattern Tests
// =============================================================================

bool testCustomPattern() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    std::vector<lst::HapticManager::HapticStep> pattern = {
        {0.3f, 0.1f, 0.05f},  // Low pulse
        {0.6f, 0.1f, 0.05f},  // Medium pulse
        {1.0f, 0.2f, 0.0f},   // Strong pulse
    };

    haptics.playPattern(lst::Hand::Right, pattern);

    // Play through the pattern
    for (int i = 0; i < 100; i++) {
        haptics.update(0.01);
        mockInput.advanceTime(0.01);
    }

    // Should have at least 3 different haptic events
    haptics.shutdown();
    return mockInput.hapticRequests.size() >= 3;
}

bool testEmptyPattern() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    std::vector<lst::HapticManager::HapticStep> emptyPattern;
    haptics.playPattern(lst::Hand::Left, emptyPattern);
    haptics.update(0.1);

    // Should handle empty pattern gracefully
    haptics.shutdown();
    return true;
}

// =============================================================================
// Queue Management Tests
// =============================================================================

bool testQueueMultipleHaptics() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    // Queue multiple haptics
    haptics.pulse(lst::Hand::Left, 0.5f, 0.1f);
    haptics.pulse(lst::Hand::Left, 0.7f, 0.1f);
    haptics.pulse(lst::Hand::Left, 0.9f, 0.1f);

    // Process all queued haptics
    for (int i = 0; i < 50; i++) {
        haptics.update(0.02);
    }

    // All should have been played
    haptics.shutdown();
    return mockInput.hapticRequests.size() >= 3;
}

bool testBothHandsIndependent() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.pulse(lst::Hand::Left, 0.5f, 0.1f);
    haptics.pulse(lst::Hand::Right, 0.8f, 0.1f);
    haptics.update(0.01);

    int leftCount = 0;
    int rightCount = 0;

    for (const auto& req : mockInput.hapticRequests) {
        if (req.hand == lst::Hand::Left) leftCount++;
        if (req.hand == lst::Hand::Right) rightCount++;
    }

    haptics.shutdown();
    return leftCount >= 1 && rightCount >= 1;
}

bool testStopAll() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    // Start long patterns
    haptics.ramp(lst::Hand::Left, 0.0f, 1.0f, 2.0f);
    haptics.ramp(lst::Hand::Right, 0.0f, 1.0f, 2.0f);

    haptics.update(0.1);
    int beforeStop = mockInput.hapticRequests.size();
    mockInput.clearRequests();

    haptics.stopAll();
    haptics.update(0.1);
    int afterStop = mockInput.hapticRequests.size();

    haptics.shutdown();
    // After stopAll, no new haptics should be generated
    return afterStop == 0 || afterStop < beforeStop;
}

// =============================================================================
// Timing Tests
// =============================================================================

bool testDurationRespected() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.pulse(lst::Hand::Left, 0.5f, 0.5f);  // 0.5 second pulse

    // Count active updates
    int activeFrames = 0;
    for (int i = 0; i < 100; i++) {
        mockInput.clearRequests();
        haptics.update(0.01);
        if (!mockInput.hapticRequests.empty()) {
            activeFrames++;
        }
    }

    haptics.shutdown();
    // Should be active for roughly 50 frames (0.5s at 100Hz update)
    return activeFrames > 30 && activeFrames < 70;
}

bool testRapidUpdates() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    // Simulate rapid updates like in VR (90Hz)
    for (int i = 0; i < 90; i++) {
        haptics.pulse(lst::Hand::Right, 0.5f, 0.05f);
        haptics.update(1.0 / 90.0);
    }

    // Should handle rapid updates without crashing
    haptics.shutdown();
    return mockInput.hapticRequests.size() > 0;
}

// =============================================================================
// Edge Case Tests
// =============================================================================

bool testZeroIntensity() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.pulse(lst::Hand::Left, 0.0f, 0.1f);
    haptics.update(0.1);

    haptics.shutdown();
    // Should handle zero intensity (might skip or send zero)
    return true;
}

bool testMaxIntensity() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.pulse(lst::Hand::Right, 1.0f, 0.1f);
    haptics.update(0.1);

    if (mockInput.hapticRequests.empty()) return false;

    // Intensity should be clamped to 1.0
    float intensity = mockInput.hapticRequests[0].intensity;

    haptics.shutdown();
    return intensity <= 1.0f;
}

bool testOverIntensity() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.pulse(lst::Hand::Left, 2.0f, 0.1f);  // Over max
    haptics.update(0.1);

    if (mockInput.hapticRequests.empty()) return true;  // OK if skipped

    // Should be clamped
    float intensity = mockInput.hapticRequests[0].intensity;

    haptics.shutdown();
    return intensity <= 1.0f;
}

bool testNegativeDuration() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    haptics.pulse(lst::Hand::Right, 0.5f, -0.1f);  // Negative duration
    haptics.update(0.1);

    // Should handle gracefully (skip or treat as zero)
    haptics.shutdown();
    return true;
}

// =============================================================================
// Integration-like Tests
// =============================================================================

bool testTypicalVRUsage() {
    lst::HapticManager haptics;
    MockInput mockInput;
    haptics.initialize(&mockInput);

    // Simulate typical VR session
    // 1. Saber activation
    haptics.playSaberActivatePattern(lst::Hand::Right);
    for (int i = 0; i < 30; i++) haptics.update(1.0/90.0);

    mockInput.clearRequests();

    // 2. Multiple hits during combat
    for (int hit = 0; hit < 5; hit++) {
        haptics.playCollisionPattern(lst::Hand::Right, 0.5f + hit * 0.1f);
        for (int i = 0; i < 10; i++) haptics.update(1.0/90.0);
    }

    int hitRequests = mockInput.hapticRequests.size();
    mockInput.clearRequests();

    // 3. Guidance during training
    haptics.playGuidanceNudge(lst::Hand::Left, 0.3f);
    for (int i = 0; i < 20; i++) haptics.update(1.0/90.0);

    // 4. Success at end
    haptics.playSuccessPattern(lst::Hand::Right);
    for (int i = 0; i < 50; i++) haptics.update(1.0/90.0);

    haptics.shutdown();
    return hitRequests >= 5;
}

}  // namespace

int runHapticsTests() {
    std::cout << "HapticManager Tests" << std::endl;
    std::cout << "-------------------" << std::endl;

    testsPassed = 0;
    testsFailed = 0;

    // Initialization
    runTest("initialization", testInitialization);
    runTest("initialize with null", testInitializeWithNull);

    // Basic triggers
    runTest("pulse", testPulse);
    runTest("buzz", testBuzz);
    runTest("ramp", testRamp);

    // Predefined patterns
    runTest("success pattern", testSuccessPattern);
    runTest("failure pattern", testFailurePattern);
    runTest("collision pattern", testCollisionPattern);
    runTest("saber activate pattern", testSaberActivatePattern);
    runTest("saber deactivate pattern", testSaberDeactivatePattern);
    runTest("guidance nudge", testGuidanceNudge);

    // Custom patterns
    runTest("custom pattern", testCustomPattern);
    runTest("empty pattern", testEmptyPattern);

    // Queue management
    runTest("queue multiple haptics", testQueueMultipleHaptics);
    runTest("both hands independent", testBothHandsIndependent);
    runTest("stop all", testStopAll);

    // Timing
    runTest("duration respected", testDurationRespected);
    runTest("rapid updates", testRapidUpdates);

    // Edge cases
    runTest("zero intensity", testZeroIntensity);
    runTest("max intensity", testMaxIntensity);
    runTest("over intensity", testOverIntensity);
    runTest("negative duration", testNegativeDuration);

    // Integration
    runTest("typical VR usage", testTypicalVRUsage);

    std::cout << std::endl;
    std::cout << "Haptics Tests: " << testsPassed << " passed, " << testsFailed << " failed" << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
