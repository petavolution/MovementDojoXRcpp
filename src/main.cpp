/**
 * Movement Dojo: XR Proprioception & Movement Exploration
 *
 * Main entry point for the VR movement training application.
 * Integrates all systems: Analytics, Progression, Overlay, Training Modes.
 */

#include "core/XRSession.h"
#include "core/Renderer.h"
#include "core/Input.h"
#include "core/SessionManager.h"
#include "visualization/MovementVisualizer.h"
#include "usd/USDLoader.h"
#include "haptics/HapticManager.h"
#include "physics/PhysicsEngine.h"

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include <csignal>

using namespace lst;

// Global flag for signal handling
volatile bool g_running = true;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutdown requested..." << std::endl;
        g_running = false;
    }
}

struct CommandLineConfig {
    std::string scenePath;
    std::string profilePath = "profile.dat";
    std::string playerName = "Player";
    bool validateOnly = false;
    bool overlayMode = false;
    std::string overlayPreset = "standard";
    std::string trainingMode = "free";
    bool exportData = true;
    std::string exportPath = "movement_data";
};

void printUsage(const char* programName) {
    std::cout << "Movement Dojo: XR Proprioception & Movement Exploration" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --scene <path>       Path to USD scene file (.usda)" << std::endl;
    std::cout << "  --validate-only      Validate scene and exit (no XR)" << std::endl;
    std::cout << "  --overlay            Run as overlay on other VR apps" << std::endl;
    std::cout << "  --overlay-preset <p> Overlay preset: minimal, standard, exploration," << std::endl;
    std::cout << "                       meditation, full (default: standard)" << std::endl;
    std::cout << "  --mode <mode>        Training mode: free, stretch, breathing, mirror," << std::endl;
    std::cout << "                       meditation, flow (default: free)" << std::endl;
    std::cout << "  --profile <path>     Path to player profile (default: profile.dat)" << std::endl;
    std::cout << "  --player <name>      Player name for new profile" << std::endl;
    std::cout << "  --export <path>      Export movement data path (default: movement_data)" << std::endl;
    std::cout << "  --no-export          Disable movement data export" << std::endl;
    std::cout << "  --help               Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " --scene scenes/dojo_basic.usda" << std::endl;
    std::cout << "  " << programName << " --overlay --overlay-preset minimal" << std::endl;
    std::cout << "  " << programName << " --mode breathing --scene scenes/dojo_full.usda" << std::endl;
}

bool parseArgs(int argc, char* argv[], CommandLineConfig& config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return false;
        } else if (strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            config.scenePath = argv[++i];
        } else if (strcmp(argv[i], "--validate-only") == 0) {
            config.validateOnly = true;
        } else if (strcmp(argv[i], "--overlay") == 0) {
            config.overlayMode = true;
        } else if (strcmp(argv[i], "--overlay-preset") == 0 && i + 1 < argc) {
            config.overlayPreset = argv[++i];
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            config.trainingMode = argv[++i];
        } else if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc) {
            config.profilePath = argv[++i];
        } else if (strcmp(argv[i], "--player") == 0 && i + 1 < argc) {
            config.playerName = argv[++i];
        } else if (strcmp(argv[i], "--export") == 0 && i + 1 < argc) {
            config.exportPath = argv[++i];
        } else if (strcmp(argv[i], "--no-export") == 0) {
            config.exportData = false;
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            printUsage(argv[0]);
            return false;
        }
    }
    return true;
}

SessionConfig::TrainingMode parseTrainingMode(const std::string& mode) {
    if (mode == "stretch") return SessionConfig::TrainingMode::GuidedStretch;
    if (mode == "breathing") return SessionConfig::TrainingMode::BreathingSync;
    if (mode == "mirror") return SessionConfig::TrainingMode::Mirror;
    if (mode == "meditation") return SessionConfig::TrainingMode::Meditation;
    if (mode == "flow") return SessionConfig::TrainingMode::FlowState;
    return SessionConfig::TrainingMode::FreeExploration;
}

int runValidationMode(const CommandLineConfig& config) {
    std::cout << "=== USD Scene Validation Mode ===" << std::endl;
    std::cout << "Scene: " << config.scenePath << std::endl;

    USDLoader loader;
    if (!loader.loadStage(config.scenePath)) {
        std::cerr << "FAILED: Could not load scene" << std::endl;
        return 1;
    }

    std::string errorMessage;
    if (!loader.validate(errorMessage)) {
        std::cerr << "FAILED: " << errorMessage << std::endl;
        return 1;
    }

    std::cout << "Scene loaded successfully:" << std::endl;
    std::cout << "  Default prim: " << loader.getDefaultPrimName() << std::endl;
    std::cout << "  Up axis: " << loader.getUpAxis() << std::endl;
    std::cout << "  Objects: " << loader.getSceneObjects().size() << std::endl;

    for (const auto& obj : loader.getSceneObjects()) {
        std::cout << "  - " << obj.name << " (" << obj.mesh.vertices.size()
                  << " vertices)" << std::endl;
    }

    std::cout << std::endl << "PASSED: Scene validation successful" << std::endl;
    return 0;
}

int runXRMode(const CommandLineConfig& cmdConfig) {
    std::cout << "=== Movement Dojo: XR Proprioception Trainer ===" << std::endl;
    std::cout << "Mode: " << (cmdConfig.overlayMode ? "Overlay" : "Standalone") << std::endl;
    std::cout << "Training: " << cmdConfig.trainingMode << std::endl;
    std::cout << "Initializing..." << std::endl;

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create app config
    AppConfig appConfig;
    appConfig.appName = "Movement Dojo";
    appConfig.appVersion = 1;
    appConfig.scenePath = cmdConfig.scenePath;

    // Initialize XR Session
    XRSession xrSession;

    // Request overlay mode if specified (must be before initialize)
    if (cmdConfig.overlayMode) {
        xrSession.requestOverlayMode(true);
    }

    if (!xrSession.initialize(appConfig)) {
        std::cerr << "Failed to initialize XR session" << std::endl;
        return 1;
    }

    // Report overlay status
    if (cmdConfig.overlayMode) {
        if (xrSession.isOverlayModeActive()) {
            std::cout << "Running in overlay mode (XR_EXTX_overlay active)" << std::endl;
        } else {
            std::cout << "Overlay extension not available - running in standalone mode" << std::endl;
        }
    }

    // Initialize Renderer
    Renderer renderer;
    if (!renderer.initialize(&xrSession)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return 1;
    }

    // Initialize Input
    Input input;
    if (!input.initialize(&xrSession)) {
        std::cerr << "Failed to initialize input" << std::endl;
        return 1;
    }

    // Initialize Physics
    PhysicsEngine physics;
    if (!physics.initialize()) {
        std::cerr << "Failed to initialize physics" << std::endl;
        return 1;
    }

    // Initialize Haptics
    HapticManager haptics;
    if (!haptics.initialize(&input)) {
        std::cerr << "Failed to initialize haptics" << std::endl;
        return 1;
    }

    // Initialize Session Manager
    SessionManager sessionManager;
    SessionConfig sessionConfig;
    sessionConfig.profilePath = cmdConfig.profilePath;
    sessionConfig.playerName = cmdConfig.playerName;
    sessionConfig.mode = cmdConfig.overlayMode ? SessionConfig::Mode::Overlay
                                               : SessionConfig::Mode::Standalone;
    sessionConfig.trainingMode = parseTrainingMode(cmdConfig.trainingMode);
    sessionConfig.overlayPreset = cmdConfig.overlayPreset;
    sessionConfig.exportOnExit = cmdConfig.exportData;
    sessionConfig.exportPath = cmdConfig.exportPath;

    if (!sessionManager.initialize(sessionConfig)) {
        std::cerr << "Failed to initialize session manager" << std::endl;
        return 1;
    }

    // Set up session callbacks
    SessionCallbacks callbacks;
    callbacks.onLevelUp = [&haptics](int level, const std::string& title) {
        std::cout << "LEVEL UP! Level " << level << " - " << title << std::endl;
        haptics.pulse(Hand::Left, 0.8f, 0.5f);
        haptics.pulse(Hand::Right, 0.8f, 0.5f);
    };
    callbacks.onAchievement = [&haptics](const std::string& name, const std::string& desc) {
        std::cout << "ACHIEVEMENT: " << name << " - " << desc << std::endl;
        haptics.pulse(Hand::Left, 0.5f, 0.3f);
        haptics.pulse(Hand::Right, 0.5f, 0.3f);
    };
    callbacks.onUncommonPositionChange = [&haptics](bool inUncommon) {
        if (inUncommon) {
            haptics.pulse(Hand::Left, 0.2f, 0.1f);
            haptics.pulse(Hand::Right, 0.2f, 0.1f);
        }
    };
    sessionManager.setCallbacks(callbacks);

    // Load USD scene
    USDLoader usdLoader;
    if (!cmdConfig.overlayMode && !cmdConfig.scenePath.empty()) {
        if (usdLoader.loadStage(cmdConfig.scenePath)) {
            for (const auto& obj : usdLoader.getSceneObjects()) {
                renderer.addObject(obj);
            }
            std::cout << "Loaded scene: " << cmdConfig.scenePath << std::endl;
        }
    }

    // Create controller visualizations
    SceneObject leftViz;
    leftViz.name = "LeftController";
    leftViz.mesh = Mesh::createCube(0.03f);
    leftViz.material.baseColor = Color(0.2f, 0.8f, 0.2f);
    renderer.addObject(leftViz);

    SceneObject rightViz;
    rightViz.name = "RightController";
    rightViz.mesh = Mesh::createCube(0.03f);
    rightViz.material.baseColor = Color(0.2f, 0.2f, 0.8f);
    renderer.addObject(rightViz);

    // Main loop timing
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    float statsDisplayTimer = 0;

    std::cout << std::endl;
    std::cout << "Session started! Press Ctrl+C to exit" << std::endl;
    std::cout << "Explore your movement space!" << std::endl;
    std::cout << std::endl;

    // Start session
    sessionManager.startSession();

    while (g_running && xrSession.isRunning()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        xrSession.pollEvents();

        if (xrSession.isSessionReady()) {
            input.syncActions();
        }

        const ControllerState& leftController = input.getLeftController();
        const ControllerState& rightController = input.getRightController();
        Transform headPose = xrSession.getHeadPose();

        // Update controller visualizations
        if (leftController.isTracked) {
            SceneObject* viz = renderer.getObject("LeftController");
            if (viz) viz->transform = leftController.pose;
        }
        if (rightController.isTracked) {
            SceneObject* viz = renderer.getObject("RightController");
            if (viz) viz->transform = rightController.pose;
        }

        // Update all systems
        sessionManager.update(deltaTime, leftController, rightController, headPose);
        haptics.update(deltaTime);
        physics.step(static_cast<float>(deltaTime));

        // Periodic stats display
        statsDisplayTimer += static_cast<float>(deltaTime);
        if (statsDisplayTimer >= 10.0f) {
            const auto& stats = sessionManager.getStats();
            std::cout << "[Stats] Coverage: " << static_cast<int>(stats.currentCoverage) << "% | "
                      << "Uncommon: " << stats.uncommonAreasFound << " | "
                      << "Flow: " << static_cast<int>(stats.currentFlowScore * 100) << "%" << std::endl;
            statsDisplayTimer = 0;
        }

        // Render frame
        if (xrSession.isSessionReady() && xrSession.beginFrame()) {
            if (xrSession.shouldRender()) {
                xrSession.locateViews();
                const auto& views = xrSession.getViews();

                renderer.beginFrame();

                for (size_t i = 0; i < views.size(); i++) {
                    renderer.renderView(static_cast<int>(i), views[i]);
                }

                // Render overlay elements
                const OverlayRenderData& overlayData = sessionManager.getOverlayRenderData();
                for (const auto& seg : overlayData.leftTrail) {
                    renderer.drawLine(seg.start, seg.end, seg.color, seg.width);
                }
                for (const auto& seg : overlayData.rightTrail) {
                    renderer.drawLine(seg.start, seg.end, seg.color, seg.width);
                }
                for (const auto& orb : overlayData.guideOrbs) {
                    renderer.drawSphere(orb.position, orb.radius, orb.color);
                }

                renderer.endFrame();
            }

            std::vector<XrCompositionLayerBaseHeader*> layers;
            xrSession.endFrame(layers);
        }

        if (!xrSession.isSessionReady()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << std::endl << "Ending session..." << std::endl;
    sessionManager.endSession();

    // Print final stats
    const auto& finalStats = sessionManager.getStats();
    std::cout << std::endl;
    std::cout << "=== Session Summary ===" << std::endl;
    std::cout << "Duration: " << static_cast<int>(finalStats.duration / 60) << " min" << std::endl;
    std::cout << "Coverage: " << finalStats.currentCoverage << "%" << std::endl;
    std::cout << "Uncommon areas: " << finalStats.uncommonAreasFound << std::endl;
    std::cout << "Flow time: " << static_cast<int>(finalStats.flowTimeAchieved) << "s" << std::endl;
    std::cout << "========================" << std::endl;

    // Cleanup
    sessionManager.shutdown();
    haptics.shutdown();
    physics.shutdown();
    input.shutdown();
    renderer.shutdown();
    xrSession.shutdown();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    CommandLineConfig config;

    if (!parseArgs(argc, argv, config)) {
        return 1;
    }

    if (config.validateOnly) {
        if (config.scenePath.empty()) {
            std::cerr << "Error: --validate-only requires --scene" << std::endl;
            return 1;
        }
        return runValidationMode(config);
    } else {
        return runXRMode(config);
    }
}
