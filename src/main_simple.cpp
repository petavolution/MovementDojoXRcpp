/**
 * Movement Dojo - Simplified Entry Point
 *
 * Demonstrates the unified Engine architecture with clean startup sequence.
 * All optional systems are attached as plugins after initialization.
 *
 * Usage:
 *   ./movement_dojo_simple [--overlay] [--scene path.usda]
 */

#include "core/Engine.h"
#include "core/Logger.h"
#include <iostream>
#include <csignal>
#include <cstring>
#include <chrono>
#include <cmath>

using namespace lst;

// Signal handling
static Engine* g_engine = nullptr;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOG_INFO(LOG_TAG_ENGINE) << "Shutdown requested (signal " << signal << ")";
        if (g_engine) {
            g_engine->requestExit();
        }
    }
}

// =============================================================================
// Example System Plugins
// =============================================================================

/**
 * Simple controller visualization system
 */
class ControllerVisualizerSystem : public System {
public:
    const char* getName() const override { return "ControllerVisualizer"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;

        // Create controller visualizations
        SceneObject left;
        left.name = "LeftController";
        left.mesh = Mesh::createCube(0.03f);
        left.material.baseColor = Color(0.2f, 0.8f, 0.2f);
        engine->addSceneObject(left);

        SceneObject right;
        right.name = "RightController";
        right.mesh = Mesh::createCube(0.03f);
        right.material.baseColor = Color(0.2f, 0.2f, 0.8f);
        engine->addSceneObject(right);

        return true;
    }

    void onUpdate(const FrameContext& ctx) override {
        // Update controller positions
        if (ctx.leftController.isTracked) {
            if (auto* obj = m_engine->getSceneObject("LeftController")) {
                obj->transform = ctx.leftController.pose;
            }
        }
        if (ctx.rightController.isTracked) {
            if (auto* obj = m_engine->getSceneObject("RightController")) {
                obj->transform = ctx.rightController.pose;
            }
        }
    }

    void onRender(const FrameContext& ctx) override {
        // Draw velocity vectors for debugging
        if (ctx.leftController.isTracked) {
            Vec3 end = ctx.leftController.position + ctx.leftController.velocity * 0.1f;
            m_engine->drawLine(ctx.leftController.position, end, Color::green(), 2.0f);
        }
        if (ctx.rightController.isTracked) {
            Vec3 end = ctx.rightController.position + ctx.rightController.velocity * 0.1f;
            m_engine->drawLine(ctx.rightController.position, end, Color::blue(), 2.0f);
        }
    }
};

/**
 * Simple statistics display system
 */
class StatsDisplaySystem : public System {
public:
    const char* getName() const override { return "StatsDisplay"; }

    void onUpdate(const FrameContext& ctx) override {
        m_displayTimer += ctx.deltaTime;

        if (m_displayTimer >= 10.0) {
            LOG_INFO(LOG_TAG_PERF) << "Time: " << static_cast<int>(ctx.totalTime) << "s"
                                   << " | Head Y: " << ctx.headPose.position.y << "m";
            m_displayTimer = 0;
        }
    }

private:
    double m_displayTimer = 0;
};

/**
 * Haptic feedback system for controller proximity
 */
class ProximityHapticsSystem : public System {
public:
    const char* getName() const override { return "ProximityHaptics"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;
        return true;
    }

    void onUpdate(const FrameContext& ctx) override {
        if (!ctx.leftController.isTracked || !ctx.rightController.isTracked) {
            return;
        }

        // Calculate distance between controllers
        Vec3 diff = ctx.leftController.position - ctx.rightController.position;
        float distance = diff.length();

        // Trigger haptic when controllers are close
        if (distance < 0.1f && !m_hapticTriggered) {
            m_engine->triggerHaptic(Hand::Left, 0.3f, 0.1f);
            m_engine->triggerHaptic(Hand::Right, 0.3f, 0.1f);
            m_hapticTriggered = true;
        } else if (distance > 0.15f) {
            m_hapticTriggered = false;
        }
    }

private:
    bool m_hapticTriggered = false;
};

// =============================================================================
// Dojo Smoke Test System
// =============================================================================

/**
 * DojoSmokeTestSystem - Minimal scene to verify VR setup visually
 *
 * Creates:
 * - Floor plane
 * - Simple walls/pillars
 * - Saber placeholder on right hand
 * - Blaster placeholder on left hand
 * - Static drone sphere placeholder
 */
class DojoSmokeTestSystem : public System {
public:
    const char* getName() const override { return "DojoSmokeTest"; }

    bool onAttach(Engine* engine) override {
        m_engine = engine;
        LOG_INFO(LOG_TAG_DIAG) << "DojoSmokeTest: Creating test scene...";

        // Floor plane (large flat cube)
        SceneObject floor;
        floor.name = "Floor";
        floor.mesh = Mesh::createCube(1.0f);
        floor.transform.position = Vec3(0, -0.01f, 0);
        floor.transform.scale = Vec3(5.0f, 0.02f, 5.0f);
        floor.material.baseColor = Color(0.2f, 0.2f, 0.25f);  // Dark gray
        engine->addSceneObject(floor);
        LOG_DEBUG(LOG_TAG_DIAG) << "  Created: Floor (5m x 5m)";

        // Corner pillars
        const float pillarHeight = 2.5f;
        const float pillarSize = 0.15f;
        const float arenaRadius = 2.0f;
        Color pillarColor(0.4f, 0.1f, 0.1f);  // Dark red

        for (int i = 0; i < 4; i++) {
            float angle = (i * 90.0f + 45.0f) * 3.14159f / 180.0f;
            float x = arenaRadius * std::cos(angle);
            float z = arenaRadius * std::sin(angle);

            SceneObject pillar;
            pillar.name = "Pillar" + std::to_string(i);
            pillar.mesh = Mesh::createCube(1.0f);
            pillar.transform.position = Vec3(x, pillarHeight / 2, z);
            pillar.transform.scale = Vec3(pillarSize, pillarHeight, pillarSize);
            pillar.material.baseColor = pillarColor;
            engine->addSceneObject(pillar);
        }
        LOG_DEBUG(LOG_TAG_DIAG) << "  Created: 4 corner pillars";

        // Right hand: Saber placeholder (elongated cyan cylinder-like shape)
        SceneObject saber;
        saber.name = "Saber";
        saber.mesh = Mesh::createCube(1.0f);
        saber.transform.scale = Vec3(0.025f, 0.025f, 0.8f);  // Thin and long
        saber.material.baseColor = Color(0.2f, 0.9f, 1.0f);  // Cyan/blue glow
        saber.material.emissive = 0.6f;  // Glow intensity
        engine->addSceneObject(saber);
        LOG_DEBUG(LOG_TAG_DIAG) << "  Created: Saber (right hand)";

        // Left hand: Blaster placeholder (compact box)
        SceneObject blaster;
        blaster.name = "Blaster";
        blaster.mesh = Mesh::createCube(1.0f);
        blaster.transform.scale = Vec3(0.04f, 0.08f, 0.15f);  // Compact gun shape
        blaster.material.baseColor = Color(0.3f, 0.3f, 0.35f);  // Metallic gray
        engine->addSceneObject(blaster);
        LOG_DEBUG(LOG_TAG_DIAG) << "  Created: Blaster (left hand)";

        // Static drone placeholder (floating sphere)
        SceneObject drone;
        drone.name = "DroneDummy";
        drone.mesh = Mesh::createSphere(0.15f);
        drone.transform.position = Vec3(0, 1.5f, -1.5f);  // Floating in front
        drone.material.baseColor = Color(0.8f, 0.2f, 0.2f);  // Red enemy
        drone.material.emissive = 0.3f;  // Glow intensity
        engine->addSceneObject(drone);
        LOG_DEBUG(LOG_TAG_DIAG) << "  Created: Dummy drone at (0, 1.5, -1.5)";

        LOG_INFO(LOG_TAG_DIAG) << "DojoSmokeTest: Scene created with 8 objects";
        return true;
    }

    void onUpdate(const FrameContext& ctx) override {
        // Update weapon positions to follow controller poses
        if (ctx.rightController.isTracked) {
            if (auto* saber = m_engine->getSceneObject("Saber")) {
                // Offset saber forward from grip
                Transform saberPose = ctx.rightController.pose;
                Vec3 forward = saberPose.orientation.rotate(Vec3(0, 0, -0.4f));
                saber->transform.position = saberPose.position + forward;
                saber->transform.orientation = saberPose.orientation;
            }
        }

        if (ctx.leftController.isTracked) {
            if (auto* blaster = m_engine->getSceneObject("Blaster")) {
                // Offset blaster forward from grip
                Transform blasterPose = ctx.leftController.pose;
                Vec3 forward = blasterPose.orientation.rotate(Vec3(0, 0, -0.08f));
                blaster->transform.position = blasterPose.position + forward;
                blaster->transform.orientation = blasterPose.orientation;
            }
        }

        // Animate drone (slow hover)
        if (auto* drone = m_engine->getSceneObject("DroneDummy")) {
            float hover = std::sin(ctx.totalTime * 2.0f) * 0.05f;
            drone->transform.position.y = 1.5f + hover;
        }

        // Periodic status logging
        m_statusTimer += ctx.deltaTime;
        if (m_statusTimer >= 1.0) {
            m_statusTimer = 0;
            LOG_DEBUG(LOG_TAG_DIAG) << "SmokeTest status: "
                << "HMD=" << (ctx.headPose.position.y > 0.1f ? "OK" : "low")
                << " L=" << (ctx.leftController.isTracked ? "tracked" : "lost")
                << " R=" << (ctx.rightController.isTracked ? "tracked" : "lost");
        }
    }

    void onRender(const FrameContext& ctx) override {
        // Draw tracking status indicators
        if (ctx.rightController.isTracked) {
            // Draw saber trail hint
            Vec3 tip = ctx.rightController.position +
                ctx.rightController.pose.orientation.rotate(Vec3(0, 0, -0.8f));
            m_engine->drawLine(ctx.rightController.position, tip, Color(0.2f, 0.9f, 1.0f), 2.0f);
        }

        if (ctx.leftController.isTracked) {
            // Draw blaster aim line
            Vec3 aim = ctx.leftController.position +
                ctx.leftController.pose.orientation.rotate(Vec3(0, 0, -2.0f));
            m_engine->drawLine(ctx.leftController.position, aim, Color(1.0f, 0.3f, 0.3f), 1.0f);
        }
    }

private:
    double m_statusTimer = 0;
};

// =============================================================================
// VR Smoke Test Mode
// =============================================================================

/**
 * RunVrSmokeTest - Visual VR test with minimal dojo scene
 *
 * Tests visual rendering with:
 * - Floor and pillars
 * - Saber on right hand
 * - Blaster on left hand
 * - Dummy drone target
 *
 * Returns 0 on completion
 */
int RunVrSmokeTest(const std::string& logPath) {
    LOG_INFO(LOG_TAG_DIAG) << "========================================";
    LOG_INFO(LOG_TAG_DIAG) << "VR Smoke Test - Dojo Visual Check";
    LOG_INFO(LOG_TAG_DIAG) << "========================================";
    LOG_INFO(LOG_TAG_DIAG) << "Press Ctrl+C to exit when done viewing";
    LOG_INFO(LOG_TAG_DIAG) << "";

    // Create engine
    Engine engine;
    g_engine = &engine;

    EngineConfig config;
    config.appName = "Dojo Smoke Test";
    config.headlessMode = false;
    config.mockTracking = false;

    LOG_INFO(LOG_TAG_DIAG) << "Initializing VR...";
    if (!engine.initialize(config)) {
        LOG_ERROR(LOG_TAG_DIAG) << "Failed to initialize VR";
        std::cout << "\nVR Smoke Test: FAIL - Could not initialize VR\n";
        std::cout << "See log at: " << logPath << "\n";
        Logger::shutdown();
        return 1;
    }

    // Add only the smoke test system
    engine.addSystem<DojoSmokeTestSystem>();

    LOG_INFO(LOG_TAG_DIAG) << "Starting VR session...";
    LOG_INFO(LOG_TAG_DIAG) << "Look around - you should see:";
    LOG_INFO(LOG_TAG_DIAG) << "  - Gray floor";
    LOG_INFO(LOG_TAG_DIAG) << "  - 4 red corner pillars";
    LOG_INFO(LOG_TAG_DIAG) << "  - Cyan saber in right hand";
    LOG_INFO(LOG_TAG_DIAG) << "  - Gray blaster in left hand";
    LOG_INFO(LOG_TAG_DIAG) << "  - Red drone sphere floating ahead";

    engine.startSession();
    engine.run();
    engine.endSession();
    engine.shutdown();

    LOG_INFO(LOG_TAG_DIAG) << "VR Smoke Test completed";
    std::cout << "\nVR Smoke Test: Completed\n";

    Logger::shutdown();
    return 0;
}

// =============================================================================
// VR Diagnostics Mode
// =============================================================================

/**
 * RunVrDiagnostics - Check VR readiness without loading gameplay
 *
 * Tests:
 * - OpenXR runtime availability
 * - HMD detection and tracking
 * - Controller tracking (left/right)
 * - Basic frame loop stability
 *
 * Returns 0 on PASS, 1 on FAIL
 */
int RunVrDiagnostics(const std::string& logPath) {
    LOG_INFO(LOG_TAG_DIAG) << "========================================";
    LOG_INFO(LOG_TAG_DIAG) << "VR Diagnostics - Dojo Readiness Check";
    LOG_INFO(LOG_TAG_DIAG) << "========================================";
    LOG_INFO(LOG_TAG_DIAG) << "Target: Quest 3 + Virtual Desktop + SteamVR";
    LOG_INFO(LOG_TAG_DIAG) << "";

    // Track what we've verified
    bool runtimeOk = false;
    bool hmdOk = false;
    bool leftControllerOk = false;
    bool rightControllerOk = false;
    bool frameLoopOk = false;
    std::string failReason;

    // Create engine for diagnostics
    Engine engine;

    EngineConfig config;
    config.appName = "VR Diagnostics";
    config.headlessMode = false;  // We need real VR
    config.mockTracking = false;

    LOG_INFO(LOG_TAG_DIAG) << "Test 1: OpenXR Initialization";
    LOG_INFO(LOG_TAG_DIAG) << "--------------------------------------------";

    if (!engine.initialize(config)) {
        failReason = "OpenXR initialization failed";
        LOG_ERROR(LOG_TAG_DIAG) << "FAIL: " << failReason;

        // Print final result
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "VR Diagnostics: FAIL\n";
        std::cout << "Reason: " << failReason << "\n";
        std::cout << "See log at: " << logPath << "\n";
        std::cout << "========================================\n";

        Logger::shutdown();
        return 1;
    }

    runtimeOk = engine.isXRReady();
    LOG_INFO(LOG_TAG_DIAG) << "Runtime/HMD: " << (runtimeOk ? "OK" : "FAIL");

    if (!runtimeOk) {
        failReason = "OpenXR runtime not ready";
        LOG_ERROR(LOG_TAG_DIAG) << "FAIL: " << failReason;
        engine.shutdown();

        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "VR Diagnostics: FAIL\n";
        std::cout << "Reason: " << failReason << "\n";
        std::cout << "See log at: " << logPath << "\n";
        std::cout << "========================================\n";

        Logger::shutdown();
        return 1;
    }

    LOG_INFO(LOG_TAG_DIAG) << "";
    LOG_INFO(LOG_TAG_DIAG) << "Test 2: Frame Loop & Tracking (3 seconds)";
    LOG_INFO(LOG_TAG_DIAG) << "--------------------------------------------";

    // Start session and run for a few seconds
    engine.startSession();

    int validHmdPoses = 0;
    int validLeftPoses = 0;
    int validRightPoses = 0;
    int totalFrames = 0;
    const int targetFrames = 270;  // ~3 seconds at 90Hz

    auto startTime = std::chrono::steady_clock::now();
    auto maxDuration = std::chrono::seconds(5);  // Safety timeout

    while (totalFrames < targetFrames) {
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (elapsed > maxDuration) {
            LOG_WARN(LOG_TAG_DIAG) << "Timeout waiting for frames";
            break;
        }

        if (!engine.tick()) {
            LOG_WARN(LOG_TAG_DIAG) << "Frame tick failed at frame " << totalFrames;
            break;
        }

        totalFrames++;

        // Check tracking status
        const auto& head = engine.getHeadPose();
        const auto& left = engine.getLeftController();
        const auto& right = engine.getRightController();

        // Simple validity check - position should not be exactly zero
        bool headValid = (head.position.x != 0 || head.position.y != 0 || head.position.z != 0);
        if (headValid) validHmdPoses++;
        if (left.isTracked) validLeftPoses++;
        if (right.isTracked) validRightPoses++;

        // Log progress every second
        if (totalFrames % 90 == 0) {
            LOG_DEBUG(LOG_TAG_DIAG) << "  Frame " << totalFrames << "/" << targetFrames
                                    << " - HMD:" << validHmdPoses << " L:" << validLeftPoses
                                    << " R:" << validRightPoses;
        }
    }

    engine.endSession();

    // Analyze results
    frameLoopOk = (totalFrames >= targetFrames / 2);  // At least half the frames
    hmdOk = (validHmdPoses > totalFrames / 2);
    leftControllerOk = (validLeftPoses > 0);
    rightControllerOk = (validRightPoses > 0);

    LOG_INFO(LOG_TAG_DIAG) << "";
    LOG_INFO(LOG_TAG_DIAG) << "Diagnostics Results:";
    LOG_INFO(LOG_TAG_DIAG) << "--------------------------------------------";
    LOG_INFO(LOG_TAG_DIAG) << "  Frames completed: " << totalFrames << "/" << targetFrames;
    LOG_INFO(LOG_TAG_DIAG) << "  Frame loop: " << (frameLoopOk ? "OK" : "FAIL");
    LOG_INFO(LOG_TAG_DIAG) << "  HMD tracking: " << validHmdPoses << "/" << totalFrames
                           << " valid poses - " << (hmdOk ? "OK" : "FAIL");
    LOG_INFO(LOG_TAG_DIAG) << "  Left controller: " << validLeftPoses << "/" << totalFrames
                           << " tracked - " << (leftControllerOk ? "OK" : "not detected");
    LOG_INFO(LOG_TAG_DIAG) << "  Right controller: " << validRightPoses << "/" << totalFrames
                           << " tracked - " << (rightControllerOk ? "OK" : "not detected");

    engine.shutdown();

    // Determine overall result
    bool overallPass = runtimeOk && frameLoopOk && hmdOk;
    // Note: Controllers are optional - dojo can work with head tracking only

    if (!overallPass) {
        if (!frameLoopOk) failReason = "Frame loop unstable";
        else if (!hmdOk) failReason = "HMD tracking invalid";
        else failReason = "Unknown failure";
    }

    LOG_INFO(LOG_TAG_DIAG) << "";
    LOG_INFO(LOG_TAG_DIAG) << "========================================";
    if (overallPass) {
        LOG_INFO(LOG_TAG_DIAG) << "VR Diagnostics: PASS";
        LOG_INFO(LOG_TAG_DIAG) << "Ready for dojo prototype!";
        if (!leftControllerOk || !rightControllerOk) {
            LOG_WARN(LOG_TAG_DIAG) << "Note: Controllers not detected - some features may be limited";
        }
    } else {
        LOG_ERROR(LOG_TAG_DIAG) << "VR Diagnostics: FAIL";
        LOG_ERROR(LOG_TAG_DIAG) << "Reason: " << failReason;
    }
    LOG_INFO(LOG_TAG_DIAG) << "========================================";

    // Print to stdout for easy CI/CD integration
    std::cout << "\n";
    std::cout << "========================================\n";
    if (overallPass) {
        std::cout << "VR Diagnostics: PASS - Ready for dojo prototype.\n";
    } else {
        std::cout << "VR Diagnostics: FAIL - Reason: " << failReason << "\n";
        std::cout << "See log at: " << logPath << "\n";
    }
    std::cout << "========================================\n";

    Logger::shutdown();
    return overallPass ? 0 : 1;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    // Parse minimal command line
    bool overlayMode = false;
    bool headlessMode = false;
    bool mockTracking = false;
    bool vrDiagnostics = false;
    bool vrSmokeTest = false;
    int maxFrames = 0;  // 0 = unlimited
    std::string scenePath;
    std::string logPath = "./logs/engine.log";
    LogLevel consoleLogLevel = LogLevel::INFO;
    LogLevel fileLogLevel = LogLevel::DEBUG;
    bool verbose = false;
    bool quiet = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            std::cout << "Movement Dojo - Simplified Entry Point\n\n"
                      << "Usage: " << argv[0] << " [options]\n\n"
                      << "Options:\n"
                      << "  --vr-diagnostics    Run VR readiness check and exit\n"
                      << "  --vr-smoke-test     Run visual VR test with dojo scene\n"
                      << "  --overlay           Run as VR overlay\n"
                      << "  --headless          Run without VR hardware (CLI testing)\n"
                      << "  --mock              Generate mock tracking data (with --headless)\n"
                      << "  --frames <n>        Run for n frames then exit (testing)\n"
                      << "  --scene <path>      Load USD scene file\n"
                      << "  --log <path>        Log file path (default: ./logs/engine.log)\n"
                      << "  --verbose, -v       Verbose console output (DEBUG level)\n"
                      << "  --trace             Very verbose console output (TRACE level)\n"
                      << "  --quiet, -q         Quiet mode (WARN+ only to console)\n"
                      << "  --help              Show this help\n";
            return 0;
        } else if (strcmp(argv[i], "--vr-diagnostics") == 0) {
            vrDiagnostics = true;
        } else if (strcmp(argv[i], "--vr-smoke-test") == 0) {
            vrSmokeTest = true;
        } else if (strcmp(argv[i], "--overlay") == 0) {
            overlayMode = true;
        } else if (strcmp(argv[i], "--headless") == 0) {
            headlessMode = true;
        } else if (strcmp(argv[i], "--mock") == 0) {
            mockTracking = true;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            maxFrames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            scenePath = argv[++i];
        } else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            logPath = argv[++i];
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
            consoleLogLevel = LogLevel::DEBUG;
        } else if (strcmp(argv[i], "--trace") == 0) {
            verbose = true;
            consoleLogLevel = LogLevel::TRACE;
            fileLogLevel = LogLevel::TRACE;
        } else if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0) {
            quiet = true;
            consoleLogLevel = LogLevel::WARN;
        }
    }

    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize logging system FIRST, before anything else
    LogConfig logConfig;
    logConfig.logFilePath = logPath;
    logConfig.consoleLogLevel = consoleLogLevel;
    logConfig.fileLogLevel = fileLogLevel;
    logConfig.appName = "Movement Dojo";
    logConfig.enableColors = !quiet;
    Logger::init(logConfig);

    // Handle VR diagnostics mode
    if (vrDiagnostics) {
        return RunVrDiagnostics(logPath);
    }

    // Handle VR smoke test mode
    if (vrSmokeTest) {
        return RunVrSmokeTest(logPath);
    }

    // Log startup
    LOG_INFO(LOG_TAG_ENGINE) << "Startup begin";
    LOG_INFO(LOG_TAG_ENGINE) << "Version: 0.1.0-dev";
    LOG_DEBUG(LOG_TAG_ENGINE) << "Log file: " << logPath;

    if (headlessMode) {
        LOG_INFO(LOG_TAG_ENGINE) << "Mode: Headless" << (mockTracking ? " (with mock tracking)" : "");
    } else {
        LOG_INFO(LOG_TAG_ENGINE) << "Mode: " << (overlayMode ? "Overlay" : "Standalone");
    }

    // Create and configure engine
    Engine engine;
    g_engine = &engine;

    EngineConfig config;
    config.appName = "Movement Dojo";
    config.requestOverlay = overlayMode;
    config.headlessMode = headlessMode;
    config.mockTracking = mockTracking;
    config.logCallback = [](const std::string& msg) {
        // Route engine callbacks through the logger
        LOG_DEBUG(LOG_TAG_ENGINE) << msg;
    };

    // Initialize engine (handles XR, rendering, input)
    LOG_DEBUG(LOG_TAG_ENGINE) << "Initializing engine...";
    if (!engine.initialize(config)) {
        LOG_ERROR(LOG_TAG_ENGINE) << "Failed to initialize engine";
        Logger::shutdown();
        return 1;
    }
    LOG_INFO(LOG_TAG_ENGINE) << "Engine initialized successfully";

    // Attach optional systems as plugins
    LOG_DEBUG(LOG_TAG_ENGINE) << "Attaching systems...";
    engine.addSystem<ControllerVisualizerSystem>();
    engine.addSystem<StatsDisplaySystem>();
    if (!headlessMode) {
        engine.addSystem<ProximityHapticsSystem>();
    }
    LOG_DEBUG(LOG_TAG_ENGINE) << "Systems attached";

    // In a full implementation, would also add:
    // engine.addSystem<PhysicsSystem>();
    // engine.addSystem<AnalyticsSystem>();
    // engine.addSystem<ProgressionSystem>();
    // engine.addSystem<TrainingSystem>();

    if (headlessMode && maxFrames > 0) {
        LOG_INFO(LOG_TAG_ENGINE) << "Running " << maxFrames << " frames in headless mode...";
    } else {
        LOG_INFO(LOG_TAG_ENGINE) << "Ready. Press Ctrl+C to exit.";
    }

    // Start the session
    LOG_DEBUG(LOG_TAG_ENGINE) << "Starting session...";
    engine.startSession();
    LOG_INFO(LOG_TAG_ENGINE) << "Session started";

    // Run the main loop
    if (maxFrames > 0) {
        // Frame-limited run for testing
        LOG_DEBUG(LOG_TAG_ENGINE) << "Entering frame-limited loop (" << maxFrames << " frames)";
        for (int frame = 0; frame < maxFrames && engine.tick(); frame++) {
            if (frame % 100 == 0) {
                LOG_DEBUG(LOG_TAG_ENGINE) << "Progress: Frame " << frame << "/" << maxFrames;
            }
        }
        LOG_DEBUG(LOG_TAG_ENGINE) << "Frame loop completed";
    } else {
        // Normal run
        LOG_DEBUG(LOG_TAG_ENGINE) << "Entering main loop";
        engine.run();
    }

    // Cleanup
    LOG_DEBUG(LOG_TAG_ENGINE) << "Ending session...";
    engine.endSession();
    LOG_DEBUG(LOG_TAG_ENGINE) << "Shutting down engine...";
    engine.shutdown();

    LOG_INFO(LOG_TAG_ENGINE) << "Shutdown complete";
    Logger::shutdown();
    return 0;
}
