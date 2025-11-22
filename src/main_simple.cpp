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
#include <iostream>
#include <csignal>
#include <cstring>

using namespace lst;

// Signal handling
static Engine* g_engine = nullptr;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutdown requested..." << std::endl;
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
            std::cout << "[Stats] Time: " << static_cast<int>(ctx.totalTime) << "s"
                      << " | Head Y: " << ctx.headPose.position.y << "m"
                      << std::endl;
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
    std::string scenePath;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            std::cout << "Movement Dojo - Simplified Entry Point\n\n"
                      << "Usage: " << argv[0] << " [options]\n\n"
                      << "Options:\n"
                      << "  --overlay         Run as VR overlay\n"
                      << "  --scene <path>    Load USD scene file\n"
                      << "  --help            Show this help\n";
            return 0;
        } else if (strcmp(argv[i], "--overlay") == 0) {
            overlayMode = true;
        } else if (strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            scenePath = argv[++i];
        }
    }

    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::cout << "=== Movement Dojo ===" << std::endl;
    std::cout << "Mode: " << (overlayMode ? "Overlay" : "Standalone") << std::endl;

    // Create and configure engine
    Engine engine;
    g_engine = &engine;

    EngineConfig config;
    config.appName = "Movement Dojo";
    config.requestOverlay = overlayMode;
    config.logCallback = [](const std::string& msg) {
        std::cout << "[Engine] " << msg << std::endl;
    };

    // Initialize engine (handles XR, rendering, input)
    if (!engine.initialize(config)) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return 1;
    }

    // Attach optional systems as plugins
    engine.addSystem<ControllerVisualizerSystem>();
    engine.addSystem<StatsDisplaySystem>();
    engine.addSystem<ProximityHapticsSystem>();

    // In a full implementation, would also add:
    // engine.addSystem<PhysicsSystem>();
    // engine.addSystem<AnalyticsSystem>();
    // engine.addSystem<ProgressionSystem>();
    // engine.addSystem<TrainingSystem>();

    std::cout << "Engine initialized. Press Ctrl+C to exit." << std::endl;

    // Start the session
    engine.startSession();

    // Run the main loop
    engine.run();

    // Cleanup
    engine.endSession();
    engine.shutdown();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}
