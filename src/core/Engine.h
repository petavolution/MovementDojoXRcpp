/**
 * Engine.h - Unified VR Engine Core
 *
 * Single header combining essential VR functionality:
 * - OpenXR session management
 * - Rendering pipeline
 * - Input handling
 *
 * Design principles:
 * - Minimal dependencies
 * - Clear ownership
 * - Simple startup sequence
 * - Plugin-based extensibility
 */

#pragma once

#include "../../include/Types.h"

// Include OpenXR or stub headers depending on build
#ifdef NO_OPENXR
#include "../../include/openxr_stub.h"
#else
#include <openxr/openxr.h>
#endif

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

namespace lst {

// =============================================================================
// Forward Declarations
// =============================================================================

class Engine;
class System;

// =============================================================================
// Engine Configuration
// =============================================================================

struct EngineConfig {
    std::string appName = "Movement Dojo";
    uint32_t appVersion = 1;

    // XR options
    bool requestOverlay = false;
    bool headlessMode = false;    // Run without XR hardware (for testing)
    bool mockTracking = false;    // Generate mock tracking data in headless mode

    // Render options
    Color clearColor = Color(0.1f, 0.1f, 0.15f, 1.0f);
    bool showGround = true;

    // Callbacks
    std::function<void(const std::string&)> logCallback;
};

// =============================================================================
// Frame Context - Data passed to systems each frame
// =============================================================================

struct FrameContext {
    double deltaTime;
    double totalTime;

    // XR state
    bool sessionReady;
    bool shouldRender;
    XrTime predictedDisplayTime;

    // Tracking
    Transform headPose;
    ControllerState leftController;
    ControllerState rightController;

    // Views for rendering
    const std::vector<XRView>* views;
};

// =============================================================================
// System Interface - Base for all optional systems
// =============================================================================

class System {
public:
    virtual ~System() = default;

    virtual const char* getName() const = 0;

    // Lifecycle
    virtual bool onAttach(Engine* engine) { return true; }
    virtual void onDetach() {}

    // Frame updates (called in order)
    virtual void onPreUpdate(const FrameContext& ctx) {}
    virtual void onUpdate(const FrameContext& ctx) {}
    virtual void onPostUpdate(const FrameContext& ctx) {}

    // Rendering (called during render pass)
    virtual void onRender(const FrameContext& ctx) {}

    // Session events
    virtual void onSessionStart() {}
    virtual void onSessionEnd() {}
    virtual void onSessionPause() {}
    virtual void onSessionResume() {}

protected:
    Engine* m_engine = nullptr;
};

// =============================================================================
// Engine - Unified VR Core
// =============================================================================

class Engine {
public:
    Engine();
    ~Engine();

    // Prevent copying
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    bool initialize(const EngineConfig& config);
    void shutdown();
    bool isRunning() const { return m_running; }

    // -------------------------------------------------------------------------
    // Main Loop
    // -------------------------------------------------------------------------

    // Call this in your main loop
    bool tick();

    // Or use the built-in loop
    void run();
    void requestExit() { m_running = false; }

    // -------------------------------------------------------------------------
    // System Management
    // -------------------------------------------------------------------------

    template<typename T, typename... Args>
    T* addSystem(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = system.get();
        if (system->onAttach(this)) {
            m_systems.push_back(std::move(system));
            return ptr;
        }
        return nullptr;
    }

    template<typename T>
    T* getSystem() {
        for (auto& sys : m_systems) {
            if (T* typed = dynamic_cast<T*>(sys.get())) {
                return typed;
            }
        }
        return nullptr;
    }

    void removeSystem(System* system);

    // -------------------------------------------------------------------------
    // Session Control
    // -------------------------------------------------------------------------

    void startSession();
    void endSession();
    void pauseSession();
    void resumeSession();
    bool isSessionActive() const { return m_sessionActive; }

    // -------------------------------------------------------------------------
    // Rendering API
    // -------------------------------------------------------------------------

    void addSceneObject(const SceneObject& obj);
    void removeSceneObject(const std::string& name);
    SceneObject* getSceneObject(const std::string& name);
    void clearScene();

    // Debug drawing (only during render)
    void drawLine(const Vec3& start, const Vec3& end, const Color& color, float width = 1.0f);
    void drawSphere(const Vec3& position, float radius, const Color& color);
    void drawCube(const Transform& transform, const Color& color);
    void drawText(const Vec3& position, const std::string& text, const Color& color);

    // -------------------------------------------------------------------------
    // Input API
    // -------------------------------------------------------------------------

    const ControllerState& getLeftController() const { return m_leftController; }
    const ControllerState& getRightController() const { return m_rightController; }
    const Transform& getHeadPose() const { return m_headPose; }

    void triggerHaptic(Hand hand, float intensity, float duration);

    // -------------------------------------------------------------------------
    // XR State
    // -------------------------------------------------------------------------

    bool isXRReady() const { return m_xrReady; }
    bool isOverlayMode() const { return m_overlayActive; }

    XrInstance getXRInstance() const { return m_xrInstance; }
    XrSession getXRSession() const { return m_xrSession; }
    XrSpace getStageSpace() const { return m_stageSpace; }

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    double getTime() const { return m_totalTime; }
    double getDeltaTime() const { return m_deltaTime; }
    void log(const std::string& message);

private:
    // XR Initialization
    bool initializeXR();
    bool createXRInstance();
    bool createXRSession();
    bool createXRSpaces();
    bool createXRSwapchains();
    bool createXRActions();

    // XR Frame handling
    void pollXREvents();
    void handleSessionStateChange(XrSessionState newState);
    bool beginXRFrame();
    void endXRFrame();
    void locateViews();
    void syncActions();

    // Rendering
    void render();

    // Mock tracking for headless mode
    void updateMockTracking();

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------
    EngineConfig m_config;

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------
    bool m_initialized = false;
    bool m_running = false;
    bool m_sessionActive = false;
    bool m_paused = false;
    bool m_xrReady = false;
    bool m_overlayActive = false;
    bool m_shouldRender = false;

    double m_totalTime = 0;
    double m_deltaTime = 0;
    double m_lastFrameTime = 0;

    // -------------------------------------------------------------------------
    // OpenXR Handles
    // -------------------------------------------------------------------------
    XrInstance m_xrInstance = XR_NULL_HANDLE;
    XrSystemId m_xrSystemId = XR_NULL_SYSTEM_ID;
    XrSession m_xrSession = XR_NULL_HANDLE;
    XrSessionState m_xrSessionState = XR_SESSION_STATE_UNKNOWN;

    // Spaces
    XrSpace m_stageSpace = XR_NULL_HANDLE;
    XrSpace m_localSpace = XR_NULL_HANDLE;
    XrSpace m_viewSpace = XR_NULL_HANDLE;

    // Frame state
    XrFrameState m_xrFrameState = {};
    XrTime m_predictedDisplayTime = 0;

    // Swapchains
    struct SwapchainData {
        XrSwapchain handle;
        int32_t width;
        int32_t height;
        std::vector<uint32_t> images;
        uint32_t acquiredIndex;  // Currently acquired image index
        bool imageAcquired;      // Whether an image is currently acquired
    };
    std::vector<SwapchainData> m_swapchains;

    // Projection layers for frame submission
    std::vector<XrCompositionLayerProjectionView> m_projectionViews;

    // Views
    std::vector<XRView> m_views;
    std::vector<XrView> m_xrViews;
    std::vector<XrViewConfigurationView> m_viewConfigViews;

    // -------------------------------------------------------------------------
    // Input
    // -------------------------------------------------------------------------
    XrActionSet m_actionSet = XR_NULL_HANDLE;
    XrAction m_poseAction = XR_NULL_HANDLE;
    XrAction m_triggerAction = XR_NULL_HANDLE;
    XrAction m_gripAction = XR_NULL_HANDLE;
    XrAction m_hapticAction = XR_NULL_HANDLE;
    XrSpace m_handSpaces[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrPath m_handPaths[2] = {XR_NULL_PATH, XR_NULL_PATH};

    ControllerState m_leftController;
    ControllerState m_rightController;
    Transform m_headPose;

    // -------------------------------------------------------------------------
    // Scene
    // -------------------------------------------------------------------------
    std::unordered_map<std::string, SceneObject> m_sceneObjects;

    // Debug draw commands
    struct DrawCommand {
        enum Type { Line, Sphere, Cube, Text };
        Type type;
        Vec3 start, end;
        float size;
        Color color;
        std::string text;
    };
    std::vector<DrawCommand> m_drawCommands;

    // -------------------------------------------------------------------------
    // Systems
    // -------------------------------------------------------------------------
    std::vector<std::unique_ptr<System>> m_systems;
};

// =============================================================================
// Inline Implementations
// =============================================================================

inline void Engine::log(const std::string& message) {
    if (m_config.logCallback) {
        m_config.logCallback(message);
    }
}

} // namespace lst
