/**
 * USD-Based OpenXR Lightsaber Proprioception Trainer
 *
 * Main entry point for the VR training application.
 * Handles initialization, main loop, and shutdown.
 */

#include "core/XRSession.h"
#include "core/Renderer.h"
#include "core/Input.h"
#include "usd/USDLoader.h"
#include "training/TrainingModule.h"
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

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --scene <path>      Path to USD scene file (.usda)" << std::endl;
    std::cout << "  --validate-only     Validate scene and exit (no XR)" << std::endl;
    std::cout << "  --help              Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " --scene scenes/stage1_cube.usda" << std::endl;
    std::cout << "  " << programName << " --scene scenes/dojo_basic.usda --validate-only" << std::endl;
}

bool parseArgs(int argc, char* argv[], AppConfig& config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return false;
        } else if (strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            config.scenePath = argv[++i];
        } else if (strcmp(argv[i], "--validate-only") == 0) {
            config.validateOnly = true;
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            printUsage(argv[0]);
            return false;
        }
    }
    return true;
}

int runValidationMode(const AppConfig& config) {
    std::cout << "=== USD Scene Validation Mode ===" << std::endl;
    std::cout << "Scene: " << config.scenePath << std::endl;
    std::cout << std::endl;

    // Load and validate USD scene
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
    std::cout << std::endl;

    // List objects
    for (const auto& obj : loader.getSceneObjects()) {
        std::cout << "  - " << obj.name << " (" << obj.mesh.vertices.size()
                  << " vertices, " << obj.mesh.indices.size() << " indices)" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "PASSED: Scene validation successful" << std::endl;
    return 0;
}

int runXRMode(const AppConfig& config) {
    std::cout << "=== USD-Based OpenXR Lightsaber Trainer ===" << std::endl;
    std::cout << "Initializing..." << std::endl;

    // Install signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize XR Session
    XRSession xrSession;
    if (!xrSession.initialize(config)) {
        std::cerr << "Failed to initialize XR session" << std::endl;
        return 1;
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

    // Load USD scene
    USDLoader usdLoader;
    if (!config.scenePath.empty()) {
        if (usdLoader.loadStage(config.scenePath)) {
            // Add loaded objects to renderer
            for (const auto& obj : usdLoader.getSceneObjects()) {
                renderer.addObject(obj);
            }
        }
    }

    // Initialize Training Manager
    TrainingManager training;

    // Set up haptic callback for training modules
    auto hapticCallback = [&haptics](Hand hand, float intensity, float duration) {
        haptics.pulse(hand, intensity, duration);
    };

    for (auto& module : training.getModules()) {
        module->setHapticCallback(hapticCallback);
    }

    // Create controller visualization objects
    SceneObject leftControllerViz;
    leftControllerViz.name = "LeftController";
    leftControllerViz.mesh = Mesh::createCube(0.05f);
    leftControllerViz.material.baseColor = Color(0.2f, 0.8f, 0.2f);
    renderer.addObject(leftControllerViz);

    SceneObject rightControllerViz;
    rightControllerViz.name = "RightController";
    rightControllerViz.mesh = Mesh::createCube(0.05f);
    rightControllerViz.material.baseColor = Color(0.2f, 0.2f, 0.8f);
    renderer.addObject(rightControllerViz);

    // Create lightsaber visualization
    SceneObject saberHandle;
    saberHandle.name = "SaberHandle";
    saberHandle.mesh = Mesh::createCylinder(0.02f, 0.25f, 16);
    saberHandle.material.baseColor = Color(0.3f, 0.3f, 0.3f);
    saberHandle.material.metallic = 0.8f;
    renderer.addObject(saberHandle);

    SceneObject saberBlade;
    saberBlade.name = "SaberBlade";
    saberBlade.mesh = Mesh::createCylinder(0.015f, 1.0f, 16);
    saberBlade.material.baseColor = Color(0.2f, 0.8f, 1.0f);
    saberBlade.material.emissive = 1.0f;
    saberBlade.visible = false;  // Initially off
    renderer.addObject(saberBlade);

    // Set up physics bodies for saber
    PhysicsBodyConfig saberHandlePhys;
    saberHandlePhys.name = "SaberHandle";
    saberHandlePhys.bodyType = BodyType::Kinematic;
    saberHandlePhys.shapeType = ShapeType::Capsule;
    saberHandlePhys.shapeSize = Vec3(0.02f, 0.25f, 0.02f);
    physics.addBody(saberHandlePhys);

    PhysicsBodyConfig saberBladePhys;
    saberBladePhys.name = "SaberBlade";
    saberBladePhys.bodyType = BodyType::Kinematic;
    saberBladePhys.shapeType = ShapeType::Capsule;
    saberBladePhys.shapeSize = Vec3(0.015f, 1.0f, 0.015f);
    saberBladePhys.collisionGroup = 2;
    physics.addBody(saberBladePhys);

    // Set up collision callback
    physics.setCollisionCallback([&haptics, &training](const CollisionInfo& info) {
        std::cout << "Collision: " << info.bodyA << " <-> " << info.bodyB << std::endl;

        // Trigger haptic feedback
        if (info.bodyA == "SaberBlade" || info.bodyB == "SaberBlade") {
            haptics.playCollisionPattern(Hand::Right, info.penetrationDepth * 10.0f);

            // Notify training module
            if (training.getCurrentExercise()) {
                training.getCurrentExercise()->onSaberCollision(info.contactPoint, info.bodyB);
            }
        }
    });

    // Saber state
    bool saberActivated = false;
    bool previousTrigger = false;

    // Main loop timing
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    std::cout << "Entering main loop..." << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;

    while (g_running && xrSession.isRunning()) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        // Poll XR events
        xrSession.pollEvents();

        // Sync input actions
        if (xrSession.isSessionReady()) {
            input.syncActions();
        }

        // Get controller states
        const ControllerState& leftController = input.getLeftController();
        const ControllerState& rightController = input.getRightController();

        // Update controller visualizations
        if (leftController.isTracked) {
            SceneObject* leftViz = renderer.getObject("LeftController");
            if (leftViz) {
                leftViz->transform = leftController.pose;
            }
        }

        if (rightController.isTracked) {
            SceneObject* rightViz = renderer.getObject("RightController");
            if (rightViz) {
                rightViz->transform = rightController.pose;
            }

            // Update saber position
            SceneObject* handleViz = renderer.getObject("SaberHandle");
            if (handleViz) {
                handleViz->transform = rightController.pose;
            }

            // Saber blade position (offset from handle)
            SceneObject* bladeViz = renderer.getObject("SaberBlade");
            if (bladeViz) {
                Transform bladeTransform = rightController.pose;
                bladeTransform.position = rightController.pose.transformPoint(Vec3(0, 0.5f, 0));
                bladeViz->transform = bladeTransform;
            }

            // Update physics bodies
            physics.updateKinematicBody("SaberHandle", rightController.pose);
            Transform bladePhysTransform = rightController.pose;
            bladePhysTransform.position = rightController.pose.transformPoint(Vec3(0, 0.5f, 0));
            physics.updateKinematicBody("SaberBlade", bladePhysTransform);

            // Toggle saber on trigger press
            if (rightController.triggerPressed && !previousTrigger) {
                saberActivated = !saberActivated;
                if (bladeViz) {
                    bladeViz->visible = saberActivated;
                }

                if (saberActivated) {
                    haptics.playSaberActivatePattern(Hand::Right);
                    std::cout << "Saber activated!" << std::endl;
                } else {
                    haptics.playSaberDeactivatePattern(Hand::Right);
                    std::cout << "Saber deactivated!" << std::endl;
                }
            }
            previousTrigger = rightController.triggerPressed;
        }

        // Update physics
        physics.step(static_cast<float>(deltaTime));

        // Update haptics
        haptics.update(deltaTime);

        // Update training module
        training.update(deltaTime, leftController, rightController);

        // Begin frame
        if (xrSession.isSessionReady() && xrSession.beginFrame()) {
            // Only render if we should
            if (xrSession.shouldRender()) {
                // Locate views
                xrSession.locateViews();
                const auto& views = xrSession.getViews();

                // Begin rendering
                renderer.beginFrame();

                // Render each view (left/right eye)
                for (size_t i = 0; i < views.size(); i++) {
                    renderer.renderView(static_cast<int>(i), views[i]);
                }

                // End rendering
                renderer.endFrame();
            }

            // End frame (submit to compositor)
            std::vector<XrCompositionLayerBaseHeader*> layers;
            // In a real implementation, we'd populate layers with swapchain images
            xrSession.endFrame(layers);
        }

        // Rate limit when not rendering to avoid spinning
        if (!xrSession.isSessionReady()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "Shutting down..." << std::endl;

    // Cleanup
    haptics.shutdown();
    physics.shutdown();
    input.shutdown();
    renderer.shutdown();
    xrSession.shutdown();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    AppConfig config;
    config.appName = "Lightsaber Trainer";
    config.appVersion = 1;

    if (!parseArgs(argc, argv, config)) {
        return 1;
    }

    // Run in appropriate mode
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
