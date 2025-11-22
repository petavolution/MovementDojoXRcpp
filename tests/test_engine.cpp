/**
 * test_engine.cpp - Unit tests for unified Engine architecture
 *
 * Tests Engine initialization, System plugin lifecycle, and core functionality.
 * These tests run without XR hardware using mock/headless mode.
 */

#include "../src/core/Engine.h"
#include "../src/core/Logger.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace lst;

// Test counters
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) \
    std::cout << "  Testing: " << name << "... "; \
    LOG_INFO("Test") << "Running: " << name;

#define PASS() \
    std::cout << "PASSED" << std::endl; \
    g_testsPassed++;

#define FAIL(msg) \
    std::cout << "FAILED: " << msg << std::endl; \
    LOG_ERROR("Test") << "FAILED: " << msg; \
    g_testsFailed++;

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { FAIL(msg); return false; }

#define ASSERT_FALSE(cond, msg) \
    if (cond) { FAIL(msg); return false; }

#define ASSERT_EQ(a, b, msg) \
    if ((a) != (b)) { FAIL(msg); return false; }

// =============================================================================
// Mock System for Testing
// =============================================================================

class MockSystem : public System {
public:
    const char* getName() const override { return "MockSystem"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;
        attachCalled = true;
        return shouldAttachSucceed;
    }

    void onDetach() override {
        detachCalled = true;
    }

    void onPreUpdate(const FrameContext& ctx) override {
        preUpdateCount++;
        lastDeltaTime = ctx.deltaTime;
    }

    void onUpdate(const FrameContext& ctx) override {
        updateCount++;
        (void)ctx;
    }

    void onPostUpdate(const FrameContext& ctx) override {
        postUpdateCount++;
        (void)ctx;
    }

    void onRender(const FrameContext& ctx) override {
        renderCount++;
        (void)ctx;
    }

    void onSessionStart() override {
        sessionStartCount++;
    }

    void onSessionEnd() override {
        sessionEndCount++;
    }

    // Test state
    bool shouldAttachSucceed = true;
    bool attachCalled = false;
    bool detachCalled = false;
    int preUpdateCount = 0;
    int updateCount = 0;
    int postUpdateCount = 0;
    int renderCount = 0;
    int sessionStartCount = 0;
    int sessionEndCount = 0;
    double lastDeltaTime = 0;
};

// =============================================================================
// Logger Tests
// =============================================================================

bool testLoggerInitialization() {
    TEST("Logger initialization");

    // Logger should initialize
    bool result = Logger::init("test-debug-log.txt");
    ASSERT_TRUE(result, "Logger::init() should succeed");
    ASSERT_TRUE(Logger::isInitialized(), "Logger should be initialized");

    // Test all log levels
    LOG_DEBUG("TestLogger") << "Debug message test";
    LOG_INFO("TestLogger") << "Info message test";
    LOG_WARN("TestLogger") << "Warning message test";
    LOG_ERROR("TestLogger") << "Error message test";

    Logger::shutdown();
    ASSERT_FALSE(Logger::isInitialized(), "Logger should be shutdown");

    PASS();
    return true;
}

bool testLoggerLevels() {
    TEST("Logger level filtering");

    Logger::init("test-debug-log.txt");

    // Test level setting
    Logger::setLevel(LogLevel::WARN);

    // Debug and Info should be filtered (but we can't easily verify without file parsing)
    LOG_DEBUG("TestLogger") << "Should be filtered";
    LOG_INFO("TestLogger") << "Should be filtered";
    LOG_WARN("TestLogger") << "Should appear";
    LOG_ERROR("TestLogger") << "Should appear";

    // Reset to DEBUG level
    Logger::setLevel(LogLevel::DEBUG);

    Logger::shutdown();
    PASS();
    return true;
}

// =============================================================================
// Engine Config Tests
// =============================================================================

bool testEngineConfigDefaults() {
    TEST("EngineConfig defaults");

    EngineConfig config;

    ASSERT_EQ(config.appName, "Movement Dojo", "Default app name");
    ASSERT_EQ(config.appVersion, 1u, "Default app version");
    ASSERT_FALSE(config.requestOverlay, "Overlay should be off by default");
    ASSERT_TRUE(config.showGround, "Ground should be shown by default");

    PASS();
    return true;
}

bool testEngineConfigCustomization() {
    TEST("EngineConfig customization");

    EngineConfig config;
    config.appName = "Custom App";
    config.appVersion = 42;
    config.requestOverlay = true;
    config.clearColor = Color(1.0f, 0.0f, 0.0f, 1.0f);

    std::string loggedMessage;
    config.logCallback = [&](const std::string& msg) {
        loggedMessage = msg;
    };

    config.logCallback("Test message");
    ASSERT_EQ(loggedMessage, "Test message", "Log callback should work");

    PASS();
    return true;
}

// =============================================================================
// FrameContext Tests
// =============================================================================

bool testFrameContextInitialization() {
    TEST("FrameContext initialization");

    FrameContext ctx;
    ctx.deltaTime = 0.011;
    ctx.totalTime = 5.0;
    ctx.sessionReady = true;
    ctx.shouldRender = true;

    ASSERT_TRUE(ctx.deltaTime > 0, "Delta time should be positive");
    ASSERT_TRUE(ctx.totalTime > 0, "Total time should be positive");
    ASSERT_TRUE(ctx.sessionReady, "Session ready flag");
    ASSERT_TRUE(ctx.shouldRender, "Should render flag");

    PASS();
    return true;
}

// =============================================================================
// System Interface Tests
// =============================================================================

bool testSystemLifecycle() {
    TEST("System lifecycle (attach/detach)");

    MockSystem system;

    ASSERT_FALSE(system.attachCalled, "Attach should not be called initially");
    ASSERT_FALSE(system.detachCalled, "Detach should not be called initially");

    // Simulate attach (normally done by Engine)
    system.onAttach(nullptr);
    ASSERT_TRUE(system.attachCalled, "Attach should be called");

    // Simulate detach
    system.onDetach();
    ASSERT_TRUE(system.detachCalled, "Detach should be called");

    PASS();
    return true;
}

bool testSystemUpdateCycle() {
    TEST("System update cycle");

    MockSystem system;

    FrameContext ctx;
    ctx.deltaTime = 0.016;
    ctx.totalTime = 1.0;

    // Simulate one frame
    system.onPreUpdate(ctx);
    system.onUpdate(ctx);
    system.onPostUpdate(ctx);

    ASSERT_EQ(system.preUpdateCount, 1, "PreUpdate should be called once");
    ASSERT_EQ(system.updateCount, 1, "Update should be called once");
    ASSERT_EQ(system.postUpdateCount, 1, "PostUpdate should be called once");
    ASSERT_TRUE(std::abs(system.lastDeltaTime - 0.016) < 0.001, "Delta time should be passed");

    // Simulate another frame
    system.onPreUpdate(ctx);
    system.onUpdate(ctx);
    system.onPostUpdate(ctx);

    ASSERT_EQ(system.preUpdateCount, 2, "PreUpdate count after second frame");
    ASSERT_EQ(system.updateCount, 2, "Update count after second frame");

    PASS();
    return true;
}

bool testSystemSessionEvents() {
    TEST("System session events");

    MockSystem system;

    ASSERT_EQ(system.sessionStartCount, 0, "Session start should not be called initially");
    ASSERT_EQ(system.sessionEndCount, 0, "Session end should not be called initially");

    system.onSessionStart();
    ASSERT_EQ(system.sessionStartCount, 1, "Session start should be called");

    system.onSessionEnd();
    ASSERT_EQ(system.sessionEndCount, 1, "Session end should be called");

    PASS();
    return true;
}

bool testSystemFailedAttach() {
    TEST("System failed attach");

    MockSystem system;
    system.shouldAttachSucceed = false;

    bool result = system.onAttach(nullptr);
    ASSERT_FALSE(result, "Attach should fail when shouldAttachSucceed is false");

    PASS();
    return true;
}

// =============================================================================
// Type Tests
// =============================================================================

bool testVec3Operations() {
    TEST("Vec3 operations");

    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);

    Vec3 sum = a + b;
    ASSERT_TRUE(std::abs(sum.x - 5.0f) < 0.001f, "Vec3 addition x");
    ASSERT_TRUE(std::abs(sum.y - 7.0f) < 0.001f, "Vec3 addition y");
    ASSERT_TRUE(std::abs(sum.z - 9.0f) < 0.001f, "Vec3 addition z");

    Vec3 diff = b - a;
    ASSERT_TRUE(std::abs(diff.x - 3.0f) < 0.001f, "Vec3 subtraction x");

    Vec3 scaled = a * 2.0f;
    ASSERT_TRUE(std::abs(scaled.x - 2.0f) < 0.001f, "Vec3 scale x");
    ASSERT_TRUE(std::abs(scaled.y - 4.0f) < 0.001f, "Vec3 scale y");

    PASS();
    return true;
}

bool testQuatConstruction() {
    TEST("Quat construction");

    Quat identity;
    ASSERT_TRUE(std::abs(identity.w - 1.0f) < 0.001f, "Identity quat w");
    ASSERT_TRUE(std::abs(identity.x - 0.0f) < 0.001f, "Identity quat x");
    ASSERT_TRUE(std::abs(identity.y - 0.0f) < 0.001f, "Identity quat y");
    ASSERT_TRUE(std::abs(identity.z - 0.0f) < 0.001f, "Identity quat z");

    Quat custom(0.707f, 0.0f, 0.707f, 0.0f);  // w, x, y, z
    ASSERT_TRUE(std::abs(custom.w - 0.707f) < 0.001f, "Custom quat w");
    ASSERT_TRUE(std::abs(custom.y - 0.707f) < 0.001f, "Custom quat y");

    PASS();
    return true;
}

bool testControllerStateInit() {
    TEST("ControllerState initialization");

    ControllerState state;

    ASSERT_EQ(static_cast<int>(state.hand), static_cast<int>(Hand::Left), "Default hand");
    ASSERT_FALSE(state.isTracked, "Should not be tracked by default");
    ASSERT_TRUE(std::abs(state.triggerValue) < 0.001f, "Trigger should be 0");
    ASSERT_TRUE(std::abs(state.gripValue) < 0.001f, "Grip should be 0");
    ASSERT_FALSE(state.triggerPressed, "Trigger not pressed");
    ASSERT_FALSE(state.gripPressed, "Grip not pressed");

    PASS();
    return true;
}

bool testColorPresets() {
    TEST("Color presets");

    Color white = Color::white();
    ASSERT_TRUE(std::abs(white.r - 1.0f) < 0.001f, "White r");
    ASSERT_TRUE(std::abs(white.g - 1.0f) < 0.001f, "White g");
    ASSERT_TRUE(std::abs(white.b - 1.0f) < 0.001f, "White b");

    Color red = Color::red();
    ASSERT_TRUE(std::abs(red.r - 1.0f) < 0.001f, "Red r");
    ASSERT_TRUE(std::abs(red.g - 0.0f) < 0.001f, "Red g");

    PASS();
    return true;
}

// =============================================================================
// XRView Tests
// =============================================================================

bool testXRViewFov() {
    TEST("XRView FOV structure");

    XRView view;
    view.fov.angleLeft = -0.8f;
    view.fov.angleRight = 0.8f;
    view.fov.angleUp = 0.9f;
    view.fov.angleDown = -0.9f;
    view.width = 1920;
    view.height = 1080;

    ASSERT_TRUE(view.fov.angleLeft < 0, "Left angle should be negative");
    ASSERT_TRUE(view.fov.angleRight > 0, "Right angle should be positive");
    ASSERT_TRUE(view.fov.angleUp > 0, "Up angle should be positive");
    ASSERT_TRUE(view.fov.angleDown < 0, "Down angle should be negative");
    ASSERT_EQ(view.width, 1920, "View width");
    ASSERT_EQ(view.height, 1080, "View height");

    PASS();
    return true;
}

// =============================================================================
// Headless Engine Startup Tests (Critical Path Validation)
// =============================================================================

bool testEngineHeadlessStartup() {
    TEST("Engine headless initialization");

    Engine engine;

    EngineConfig config;
    config.appName = "Test App";
    config.headlessMode = true;
    config.mockTracking = false;

    bool initResult = engine.initialize(config);
    ASSERT_TRUE(initResult, "Engine should initialize in headless mode");

    engine.shutdown();

    PASS();
    return true;
}

bool testEngineHeadlessWithMockTracking() {
    TEST("Engine headless with mock tracking");

    Engine engine;

    EngineConfig config;
    config.appName = "Mock Test";
    config.headlessMode = true;
    config.mockTracking = true;

    bool initResult = engine.initialize(config);
    ASSERT_TRUE(initResult, "Engine should initialize with mock tracking");

    // Run a few frames
    for (int i = 0; i < 10; i++) {
        bool tickResult = engine.tick();
        ASSERT_TRUE(tickResult, "tick() should succeed in headless mode");
    }

    engine.shutdown();

    PASS();
    return true;
}

bool testEngineSystemAttachment() {
    TEST("Engine system attachment in headless mode");

    Engine engine;

    EngineConfig config;
    config.headlessMode = true;
    config.mockTracking = true;

    engine.initialize(config);

    // Attach a mock system
    MockSystem* mockSystem = engine.addSystem<MockSystem>();
    ASSERT_TRUE(mockSystem != nullptr, "addSystem should return valid pointer");
    ASSERT_TRUE(mockSystem->attachCalled, "System onAttach should be called");

    // Run a frame to trigger update
    engine.tick();
    ASSERT_TRUE(mockSystem->updateCount > 0, "System onUpdate should be called");

    engine.shutdown();
    ASSERT_TRUE(mockSystem->detachCalled, "System onDetach should be called on shutdown");

    PASS();
    return true;
}

bool testEngineSessionLifecycle() {
    TEST("Engine session lifecycle");

    Engine engine;

    EngineConfig config;
    config.headlessMode = true;
    config.mockTracking = true;

    engine.initialize(config);

    MockSystem* mockSystem = engine.addSystem<MockSystem>();

    // Start session
    engine.startSession();
    ASSERT_EQ(mockSystem->sessionStartCount, 1, "Session start should be called");

    // Run frames
    for (int i = 0; i < 5; i++) {
        engine.tick();
    }

    // End session
    engine.endSession();
    ASSERT_EQ(mockSystem->sessionEndCount, 1, "Session end should be called");

    engine.shutdown();

    PASS();
    return true;
}

bool testEngineFrameContextData() {
    TEST("Engine provides valid frame context data");

    Engine engine;

    EngineConfig config;
    config.headlessMode = true;
    config.mockTracking = true;

    engine.initialize(config);

    // Custom system to capture frame context
    class ContextCapture : public System {
    public:
        const char* getName() const override { return "ContextCapture"; }
        FrameContext lastContext;
        bool captured = false;

        void onUpdate(const FrameContext& ctx) override {
            lastContext = ctx;
            captured = true;
        }
    };

    ContextCapture* capture = engine.addSystem<ContextCapture>();

    // Run a few frames
    for (int i = 0; i < 5; i++) {
        engine.tick();
    }

    ASSERT_TRUE(capture->captured, "Context should be captured");
    ASSERT_TRUE(capture->lastContext.deltaTime > 0, "Delta time should be positive");
    ASSERT_TRUE(capture->lastContext.totalTime > 0, "Total time should be positive");
    ASSERT_TRUE(capture->lastContext.sessionReady, "Session should be ready in headless mode");

    // With mock tracking, controllers should be tracked
    ASSERT_TRUE(capture->lastContext.leftController.isTracked, "Left controller should be tracked with mock");
    ASSERT_TRUE(capture->lastContext.rightController.isTracked, "Right controller should be tracked with mock");

    engine.shutdown();

    PASS();
    return true;
}

// =============================================================================
// Test Runner
// =============================================================================

int runEngineTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Engine Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Initialize logger for tests
    Logger::init("test-engine-debug.txt");
    LOG_INFO("Test") << "Starting Engine tests";

    // Logger tests
    std::cout << "[Logger Tests]" << std::endl;
    testLoggerLevels();

    // Config tests
    std::cout << "\n[Config Tests]" << std::endl;
    testEngineConfigDefaults();
    testEngineConfigCustomization();

    // FrameContext tests
    std::cout << "\n[FrameContext Tests]" << std::endl;
    testFrameContextInitialization();

    // System tests
    std::cout << "\n[System Interface Tests]" << std::endl;
    testSystemLifecycle();
    testSystemUpdateCycle();
    testSystemSessionEvents();
    testSystemFailedAttach();

    // Type tests
    std::cout << "\n[Type Tests]" << std::endl;
    testVec3Operations();
    testQuatConstruction();
    testControllerStateInit();
    testColorPresets();
    testXRViewFov();

    // Headless startup tests (critical path validation)
    std::cout << "\n[Headless Startup Tests]" << std::endl;
    testEngineHeadlessStartup();
    testEngineHeadlessWithMockTracking();
    testEngineSystemAttachment();
    testEngineSessionLifecycle();
    testEngineFrameContextData();

    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Engine Tests Complete" << std::endl;
    std::cout << "Passed: " << g_testsPassed << std::endl;
    std::cout << "Failed: " << g_testsFailed << std::endl;
    std::cout << "========================================\n" << std::endl;

    LOG_INFO("Test") << "Engine tests complete: " << g_testsPassed << " passed, " << g_testsFailed << " failed";
    Logger::shutdown();

    return g_testsFailed > 0 ? 1 : 0;
}

// Make runEngineTests available to test_main.cpp
extern "C" int runEngineTests();
