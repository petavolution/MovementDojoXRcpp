/**
 * SessionManager Integration Tests
 *
 * Tests the central coordinator for all Movement Dojo systems.
 */

#include "core/SessionManager.h"
#include "core/Configuration.h"
#include <iostream>
#include <cassert>
#include <cmath>

#if USE_GTEST
#include <gtest/gtest.h>
#endif

using namespace lst;

namespace {

const float EPSILON = 0.0001f;

bool approxEqual(float a, float b) {
    return std::abs(a - b) < EPSILON;
}

// =============================================================================
// SessionConfig Tests
// =============================================================================

void testSessionConfigDefaults() {
    SessionConfig config;

    assert(config.profilePath == "profile.dat");
    assert(config.playerName == "Player");
    assert(config.mode == SessionConfig::Mode::Standalone);
    assert(config.trainingMode == SessionConfig::TrainingMode::FreeExploration);
    assert(config.overlayPreset == "standard");
    assert(config.overlayVisible == true);
    assert(config.recordMovement == true);
    assert(config.exportOnExit == true);
    assert(config.exportPath == "movement_data");

    std::cout << "  SessionConfig defaults: PASS" << std::endl;
}

void testSessionConfigTrainingModes() {
    SessionConfig config;

    // Test all training mode values
    config.trainingMode = SessionConfig::TrainingMode::FreeExploration;
    assert(config.trainingMode == SessionConfig::TrainingMode::FreeExploration);

    config.trainingMode = SessionConfig::TrainingMode::GuidedStretch;
    assert(config.trainingMode == SessionConfig::TrainingMode::GuidedStretch);

    config.trainingMode = SessionConfig::TrainingMode::BreathingSync;
    assert(config.trainingMode == SessionConfig::TrainingMode::BreathingSync);

    config.trainingMode = SessionConfig::TrainingMode::Mirror;
    assert(config.trainingMode == SessionConfig::TrainingMode::Mirror);

    config.trainingMode = SessionConfig::TrainingMode::Meditation;
    assert(config.trainingMode == SessionConfig::TrainingMode::Meditation);

    config.trainingMode = SessionConfig::TrainingMode::FlowState;
    assert(config.trainingMode == SessionConfig::TrainingMode::FlowState);

    std::cout << "  SessionConfig training modes: PASS" << std::endl;
}

// =============================================================================
// SessionStats Tests
// =============================================================================

void testSessionStatsDefaults() {
    SessionStats stats{};

    assert(stats.duration == 0.0f);
    assert(stats.coverageGained == 0.0f);
    assert(stats.currentCoverage == 0.0f);
    assert(stats.uncommonAreasFound == 0);
    assert(stats.flowTimeAchieved == 0.0f);
    assert(stats.xpEarned == 0);
    assert(stats.achievementsUnlocked.empty());
    assert(stats.currentFlowScore == 0.0f);
    assert(stats.isInUncommonPosition == false);

    std::cout << "  SessionStats defaults: PASS" << std::endl;
}

// =============================================================================
// SessionManager Lifecycle Tests
// =============================================================================

void testSessionManagerConstruction() {
    SessionManager manager;

    assert(manager.isSessionActive() == false);
    assert(manager.isPaused() == false);

    std::cout << "  SessionManager construction: PASS" << std::endl;
}

void testSessionManagerInitialization() {
    SessionManager manager;
    SessionConfig config;
    config.playerName = "TestPlayer";
    config.profilePath = "test_profile.dat";
    config.exportOnExit = false;  // Don't export during tests

    bool initialized = manager.initialize(config);
    assert(initialized == true);

    // Verify training mode getter
    assert(manager.getTrainingMode() == SessionConfig::TrainingMode::FreeExploration);

    manager.shutdown();

    std::cout << "  SessionManager initialization: PASS" << std::endl;
}

void testSessionManagerSessionLifecycle() {
    SessionManager manager;
    SessionConfig config;
    config.exportOnExit = false;

    bool initialized = manager.initialize(config);
    assert(initialized);

    // Should not be active initially
    assert(manager.isSessionActive() == false);

    // Start session
    manager.startSession();
    assert(manager.isSessionActive() == true);
    assert(manager.isPaused() == false);

    // Pause
    manager.pauseSession();
    assert(manager.isSessionActive() == true);
    assert(manager.isPaused() == true);

    // Resume
    manager.resumeSession();
    assert(manager.isSessionActive() == true);
    assert(manager.isPaused() == false);

    // End session
    manager.endSession();
    assert(manager.isSessionActive() == false);

    manager.shutdown();

    std::cout << "  SessionManager session lifecycle: PASS" << std::endl;
}

void testSessionManagerTrainingModeSwitch() {
    SessionManager manager;
    SessionConfig config;
    config.exportOnExit = false;
    config.trainingMode = SessionConfig::TrainingMode::FreeExploration;

    manager.initialize(config);
    manager.startSession();

    // Initial mode
    assert(manager.getTrainingMode() == SessionConfig::TrainingMode::FreeExploration);

    // Switch modes
    manager.setTrainingMode(SessionConfig::TrainingMode::BreathingSync);
    assert(manager.getTrainingMode() == SessionConfig::TrainingMode::BreathingSync);

    manager.setTrainingMode(SessionConfig::TrainingMode::FlowState);
    assert(manager.getTrainingMode() == SessionConfig::TrainingMode::FlowState);

    // Get current module
    TrainingModule* module = manager.getCurrentTrainingModule();
    assert(module != nullptr);

    manager.endSession();
    manager.shutdown();

    std::cout << "  SessionManager training mode switch: PASS" << std::endl;
}

void testSessionManagerUpdate() {
    SessionManager manager;
    SessionConfig config;
    config.exportOnExit = false;

    manager.initialize(config);
    manager.startSession();

    // Create mock controller states
    ControllerState leftController;
    leftController.hand = Hand::Left;
    leftController.isTracked = true;
    leftController.pose.position = Vec3(-0.3f, 1.0f, -0.5f);
    leftController.pose.orientation = Quat::identity();

    ControllerState rightController;
    rightController.hand = Hand::Right;
    rightController.isTracked = true;
    rightController.pose.position = Vec3(0.3f, 1.0f, -0.5f);
    rightController.pose.orientation = Quat::identity();

    Transform headPose;
    headPose.position = Vec3(0.0f, 1.6f, 0.0f);
    headPose.orientation = Quat::identity();

    // Run a few update cycles
    for (int i = 0; i < 10; i++) {
        manager.update(0.011, leftController, rightController, headPose);

        // Simulate movement
        leftController.pose.position.x -= 0.01f;
        rightController.pose.position.x += 0.01f;
    }

    // Get stats
    const SessionStats& stats = manager.getStats();
    assert(stats.duration > 0.0f);

    manager.endSession();
    manager.shutdown();

    std::cout << "  SessionManager update: PASS" << std::endl;
}

void testSessionManagerCallbacks() {
    SessionManager manager;
    SessionConfig config;
    config.exportOnExit = false;

    manager.initialize(config);

    // Track callback invocations
    bool coverageCallbackCalled = false;

    SessionCallbacks callbacks;
    callbacks.onCoverageUpdate = [&coverageCallbackCalled](float coverage) {
        coverageCallbackCalled = true;
    };

    manager.setCallbacks(callbacks);
    manager.startSession();

    // Callbacks exist and can be set
    // Note: Actually triggering callbacks requires more movement simulation

    manager.endSession();
    manager.shutdown();

    std::cout << "  SessionManager callbacks: PASS" << std::endl;
}

// =============================================================================
// Configuration Tests
// =============================================================================

void testConfigurationDefaults() {
    Configuration config = Configuration::createDefault();

    assert(config.getString(Configuration::Keys::APP_NAME) == "Movement Dojo");
    assert(config.getString(Configuration::Keys::PLAYER_NAME) == "Player");
    assert(config.getBool(Configuration::Keys::OVERLAY_ENABLED) == true);
    assert(config.getBool(Configuration::Keys::HAPTICS_ENABLED) == true);
    assert(config.getInt(Configuration::Keys::SAMPLE_RATE) == 90);

    std::cout << "  Configuration defaults: PASS" << std::endl;
}

void testConfigurationGetSet() {
    Configuration config;

    // String
    config.setString("test.string", "hello");
    assert(config.getString("test.string") == "hello");
    assert(config.getString("nonexistent", "default") == "default");

    // Int
    config.setInt("test.int", 42);
    assert(config.getInt("test.int") == 42);
    assert(config.getInt("nonexistent", 100) == 100);

    // Float
    config.setFloat("test.float", 3.14f);
    assert(approxEqual(config.getFloat("test.float"), 3.14f));
    assert(approxEqual(config.getFloat("nonexistent", 1.0f), 1.0f));

    // Bool
    config.setBool("test.bool", true);
    assert(config.getBool("test.bool") == true);
    config.setBool("test.bool2", false);
    assert(config.getBool("test.bool2") == false);
    assert(config.getBool("nonexistent", true) == true);

    std::cout << "  Configuration get/set: PASS" << std::endl;
}

void testConfigurationHasKey() {
    Configuration config;

    assert(config.hasKey("test.key") == false);

    config.setString("test.key", "value");
    assert(config.hasKey("test.key") == true);

    config.remove("test.key");
    assert(config.hasKey("test.key") == false);

    std::cout << "  Configuration hasKey: PASS" << std::endl;
}

void testConfigurationClear() {
    Configuration config;

    config.setString("a", "1");
    config.setString("b", "2");
    config.setString("c", "3");

    assert(config.getKeys().size() == 3);

    config.clear();

    assert(config.getKeys().size() == 0);

    std::cout << "  Configuration clear: PASS" << std::endl;
}

void testConfigurationGetKeys() {
    Configuration config;

    config.setString("alpha", "a");
    config.setInt("beta", 2);
    config.setFloat("gamma", 3.0f);

    auto keys = config.getKeys();
    assert(keys.size() == 3);

    // Keys should be present (order may vary due to map)
    bool hasAlpha = false, hasBeta = false, hasGamma = false;
    for (const auto& key : keys) {
        if (key == "alpha") hasAlpha = true;
        if (key == "beta") hasBeta = true;
        if (key == "gamma") hasGamma = true;
    }
    assert(hasAlpha && hasBeta && hasGamma);

    std::cout << "  Configuration getKeys: PASS" << std::endl;
}

} // anonymous namespace

// =============================================================================
// Test Runner
// =============================================================================

int runSessionManagerTests() {
    std::cout << "Running SessionManager Tests..." << std::endl;

    // SessionConfig tests
    testSessionConfigDefaults();
    testSessionConfigTrainingModes();

    // SessionStats tests
    testSessionStatsDefaults();

    // SessionManager tests
    testSessionManagerConstruction();
    testSessionManagerInitialization();
    testSessionManagerSessionLifecycle();
    testSessionManagerTrainingModeSwitch();
    testSessionManagerUpdate();
    testSessionManagerCallbacks();

    // Configuration tests
    testConfigurationDefaults();
    testConfigurationGetSet();
    testConfigurationHasKey();
    testConfigurationClear();
    testConfigurationGetKeys();

    std::cout << "All SessionManager Tests PASSED!" << std::endl;
    return 0;
}

#if USE_GTEST
TEST(SessionManagerTest, SessionConfigDefaults) { testSessionConfigDefaults(); }
TEST(SessionManagerTest, SessionConfigTrainingModes) { testSessionConfigTrainingModes(); }
TEST(SessionManagerTest, SessionStatsDefaults) { testSessionStatsDefaults(); }
TEST(SessionManagerTest, Construction) { testSessionManagerConstruction(); }
TEST(SessionManagerTest, Initialization) { testSessionManagerInitialization(); }
TEST(SessionManagerTest, SessionLifecycle) { testSessionManagerSessionLifecycle(); }
TEST(SessionManagerTest, TrainingModeSwitch) { testSessionManagerTrainingModeSwitch(); }
TEST(SessionManagerTest, Update) { testSessionManagerUpdate(); }
TEST(SessionManagerTest, Callbacks) { testSessionManagerCallbacks(); }
TEST(ConfigurationTest, Defaults) { testConfigurationDefaults(); }
TEST(ConfigurationTest, GetSet) { testConfigurationGetSet(); }
TEST(ConfigurationTest, HasKey) { testConfigurationHasKey(); }
TEST(ConfigurationTest, Clear) { testConfigurationClear(); }
TEST(ConfigurationTest, GetKeys) { testConfigurationGetKeys(); }
#endif

// Note: main() is provided by test_main.cpp when not using GTest
