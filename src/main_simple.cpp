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
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    // Parse minimal command line
    bool overlayMode = false;
    bool headlessMode = false;
    bool mockTracking = false;
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
    logConfig.enableColors = !quiet;  // No colors in quiet mode
    Logger::init(logConfig);

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
