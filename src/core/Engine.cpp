/**
 * Engine.cpp - Unified VR Engine Implementation
 *
 * Combines OpenXR session, rendering, and input into a single
 * coherent implementation with clear startup sequence.
 */

#include "Engine.h"
#include "Logger.h"
#include "../environments/Environment.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace lst {

// =============================================================================
// Construction / Destruction
// =============================================================================

Engine::Engine() {
    // Initialize controller states with correct hand assignment
    m_leftController = ControllerState();
    m_leftController.hand = Hand::Left;

    m_rightController = ControllerState();
    m_rightController.hand = Hand::Right;
}

Engine::~Engine() {
    shutdown();
}

// =============================================================================
// Lifecycle
// =============================================================================

bool Engine::initialize(const EngineConfig& config) {
    if (m_initialized) {
        LOG_WARN("Engine") << "Engine already initialized";
        log("Engine already initialized");
        return true;
    }

    // Initialize logger if not already done
    if (!Logger::isInitialized()) {
        Logger::init("debug-log.txt");
    }

    m_config = config;
    LOG_INFO("Engine") << "Initializing engine: " << config.appName;
    LOG_INFO("Engine") << "  Headless mode: " << (config.headlessMode ? "yes" : "no");
    LOG_INFO("Engine") << "  Mock tracking: " << (config.mockTracking ? "yes" : "no");
    log("Initializing engine: " + config.appName);

    // Initialize OpenXR (skip in headless mode)
    if (!config.headlessMode) {
        if (!initializeXR()) {
            LOG_ERROR("Engine") << "Failed to initialize OpenXR - check HMD connection";
            LOG_INFO("Engine") << "Tip: Use headlessMode=true for CLI testing without VR hardware";
            log("Failed to initialize OpenXR");
            return false;
        }
    } else {
        LOG_INFO("Engine") << "Headless mode - skipping XR initialization";
        m_xrReady = false;  // No XR in headless mode

        // Setup default view configuration for headless
        m_views.resize(2);  // Stereo views
        for (auto& view : m_views) {
            view.width = 1920;
            view.height = 1080;
            view.fov.angleLeft = -0.8f;
            view.fov.angleRight = 0.8f;
            view.fov.angleUp = 0.9f;
            view.fov.angleDown = -0.9f;
        }
    }

    m_initialized = true;
    m_running = true;
    LOG_INFO("Engine") << "Engine initialized successfully";
    log("Engine initialized successfully");
    return true;
}

void Engine::shutdown() {
    if (!m_initialized) return;

    LOG_INFO(LOG_TAG_ENGINE) << "Shutdown initiated";
    LOG_DEBUG(LOG_TAG_ENGINE) << "Shutting down engine";
    log("Shutting down engine");

    // End active session
    if (m_sessionActive) {
        LOG_DEBUG(LOG_TAG_ENGINE) << "Ending active session...";
        endSession();
    }

    // Unload environment
    if (m_environment) {
        LOG_DEBUG(LOG_TAG_ENGINE) << "Unloading environment...";
        unloadEnvironment();
    }

    // Detach all systems
    if (!m_systems.empty()) {
        LOG_DEBUG(LOG_TAG_ENGINE) << "Detaching " << m_systems.size() << " systems...";
        for (auto& sys : m_systems) {
            LOG_TRACE(LOG_TAG_ENGINE) << "  Detaching system: " << sys->getName();
            sys->onDetach();
        }
        m_systems.clear();
        LOG_DEBUG(LOG_TAG_ENGINE) << "Systems detached";
    }

    // Cleanup XR resources (in reverse order of creation)
    LOG_DEBUG(LOG_TAG_ENGINE) << "Cleaning up OpenXR resources...";

    // Input resources
    for (int i = 0; i < 2; i++) {
        if (m_handSpaces[i] != XR_NULL_HANDLE) {
            LOG_TRACE(LOG_TAG_XR) << "  Destroying hand space " << i;
            xrDestroySpace(m_handSpaces[i]);
            m_handSpaces[i] = XR_NULL_HANDLE;
        }
    }

    if (m_actionSet != XR_NULL_HANDLE) {
        LOG_TRACE(LOG_TAG_XR) << "  Destroying action set";
        xrDestroyActionSet(m_actionSet);
        m_actionSet = XR_NULL_HANDLE;
    }

    // Swapchains
    if (!m_swapchains.empty()) {
        LOG_TRACE(LOG_TAG_XR) << "  Destroying " << m_swapchains.size() << " swapchains";
        for (auto& sw : m_swapchains) {
            if (sw.handle != XR_NULL_HANDLE) {
                xrDestroySwapchain(sw.handle);
            }
        }
        m_swapchains.clear();
    }

    // Reference spaces
    if (m_viewSpace != XR_NULL_HANDLE) {
        LOG_TRACE(LOG_TAG_XR) << "  Destroying view space";
        xrDestroySpace(m_viewSpace);
        m_viewSpace = XR_NULL_HANDLE;
    }
    if (m_localSpace != XR_NULL_HANDLE) {
        LOG_TRACE(LOG_TAG_XR) << "  Destroying local space";
        xrDestroySpace(m_localSpace);
        m_localSpace = XR_NULL_HANDLE;
    }
    if (m_stageSpace != XR_NULL_HANDLE) {
        LOG_TRACE(LOG_TAG_XR) << "  Destroying stage space";
        xrDestroySpace(m_stageSpace);
        m_stageSpace = XR_NULL_HANDLE;
    }

    // Session
    if (m_xrSession != XR_NULL_HANDLE) {
        LOG_DEBUG(LOG_TAG_XR) << "  Destroying XR session";
        xrDestroySession(m_xrSession);
        m_xrSession = XR_NULL_HANDLE;
    }

    // Instance (must be last)
    if (m_xrInstance != XR_NULL_HANDLE) {
        LOG_DEBUG(LOG_TAG_XR) << "  Destroying XR instance";
        xrDestroyInstance(m_xrInstance);
        m_xrInstance = XR_NULL_HANDLE;
    }

    LOG_DEBUG(LOG_TAG_ENGINE) << "OpenXR resources cleaned up";

    m_initialized = false;
    m_running = false;
    m_xrReady = false;

    LOG_INFO(LOG_TAG_ENGINE) << "Engine shutdown complete";
    log("Engine shutdown complete");
}

// =============================================================================
// Main Loop
// =============================================================================

bool Engine::tick() {
    if (!m_initialized || !m_running) {
        return false;
    }

    // Calculate delta time
    auto now = std::chrono::high_resolution_clock::now();
    double currentTime = std::chrono::duration<double>(now.time_since_epoch()).count();

    if (m_lastFrameTime > 0) {
        m_deltaTime = currentTime - m_lastFrameTime;
    } else {
        m_deltaTime = 1.0 / 90.0;
    }
    m_lastFrameTime = currentTime;
    m_totalTime += m_deltaTime;

    // Headless mode: generate mock data and skip XR
    if (m_config.headlessMode) {
        if (m_config.mockTracking) {
            updateMockTracking();
        }
        m_shouldRender = false;  // No rendering in headless mode
    } else {
        // Poll XR events
        pollXREvents();

        if (!m_running) {
            return false;
        }

        // Sync input if ready
        if (m_xrReady) {
            syncActions();
        }
    }

    // Build frame context
    FrameContext ctx;
    ctx.deltaTime = m_deltaTime;
    ctx.totalTime = m_totalTime;
    ctx.sessionReady = m_xrReady || m_config.headlessMode;
    ctx.shouldRender = m_shouldRender;
    ctx.predictedDisplayTime = m_predictedDisplayTime;
    ctx.headPose = m_headPose;
    ctx.leftController = m_leftController;
    ctx.rightController = m_rightController;
    ctx.views = &m_views;

    // Pre-update systems
    for (auto& sys : m_systems) {
        sys->onPreUpdate(ctx);
    }

    // Update systems
    for (auto& sys : m_systems) {
        sys->onUpdate(ctx);
    }

    // Post-update systems
    for (auto& sys : m_systems) {
        sys->onPostUpdate(ctx);
    }

    // Update environment (animations, effects)
    if (m_environment) {
        m_environment->update(static_cast<float>(m_deltaTime));
    }

    // Render frame
    if (m_xrReady && beginXRFrame()) {
        if (m_shouldRender) {
            locateViews();
            render();

            // Let systems render
            for (auto& sys : m_systems) {
                sys->onRender(ctx);
            }
        }
        endXRFrame();
    }

    // Clear draw commands after frame
    m_drawCommands.clear();

    // Sleep if not rendering
    if (!m_xrReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return m_running;
}

void Engine::run() {
    while (tick()) {
        // Main loop continues
    }
}

// =============================================================================
// Session Control
// =============================================================================

void Engine::startSession() {
    if (m_sessionActive) return;

    m_sessionActive = true;
    m_paused = false;

    for (auto& sys : m_systems) {
        sys->onSessionStart();
    }

    log("Session started");
}

void Engine::endSession() {
    if (!m_sessionActive) return;

    LOG_DEBUG(LOG_TAG_ENGINE) << "Session ending - notifying " << m_systems.size() << " systems";
    for (auto& sys : m_systems) {
        sys->onSessionEnd();
    }

    m_sessionActive = false;
    LOG_DEBUG(LOG_TAG_ENGINE) << "Session ended";
    log("Session ended");
}

void Engine::pauseSession() {
    if (!m_sessionActive || m_paused) return;

    m_paused = true;
    for (auto& sys : m_systems) {
        sys->onSessionPause();
    }
}

void Engine::resumeSession() {
    if (!m_sessionActive || !m_paused) return;

    m_paused = false;
    for (auto& sys : m_systems) {
        sys->onSessionResume();
    }
}

// =============================================================================
// System Management
// =============================================================================

void Engine::removeSystem(System* system) {
    auto it = std::find_if(m_systems.begin(), m_systems.end(),
        [system](const std::unique_ptr<System>& s) { return s.get() == system; });

    if (it != m_systems.end()) {
        (*it)->onDetach();
        m_systems.erase(it);
    }
}

// =============================================================================
// Scene Management
// =============================================================================

void Engine::addSceneObject(const SceneObject& obj) {
    m_sceneObjects[obj.name] = obj;
}

void Engine::removeSceneObject(const std::string& name) {
    m_sceneObjects.erase(name);
}

SceneObject* Engine::getSceneObject(const std::string& name) {
    auto it = m_sceneObjects.find(name);
    return it != m_sceneObjects.end() ? &it->second : nullptr;
}

void Engine::clearScene() {
    m_sceneObjects.clear();
}

// =============================================================================
// Debug Drawing
// =============================================================================

void Engine::drawLine(const Vec3& start, const Vec3& end, const Color& color, float width) {
    DrawCommand cmd;
    cmd.type = DrawCommand::Line;
    cmd.start = start;
    cmd.end = end;
    cmd.color = color;
    cmd.size = width;
    m_drawCommands.push_back(cmd);
}

void Engine::drawSphere(const Vec3& position, float radius, const Color& color) {
    DrawCommand cmd;
    cmd.type = DrawCommand::Sphere;
    cmd.start = position;
    cmd.size = radius;
    cmd.color = color;
    m_drawCommands.push_back(cmd);
}

void Engine::drawCube(const Transform& transform, const Color& color) {
    DrawCommand cmd;
    cmd.type = DrawCommand::Cube;
    cmd.start = transform.position;
    cmd.size = transform.scale.x;
    cmd.color = color;
    m_drawCommands.push_back(cmd);
}

void Engine::drawText(const Vec3& position, const std::string& text, const Color& color) {
    DrawCommand cmd;
    cmd.type = DrawCommand::Text;
    cmd.start = position;
    cmd.text = text;
    cmd.color = color;
    m_drawCommands.push_back(cmd);
}

// =============================================================================
// Input
// =============================================================================

void Engine::triggerHaptic(Hand hand, float intensity, float duration) {
    if (m_hapticAction == XR_NULL_HANDLE) return;

    XrHapticVibration vibration = {XR_TYPE_HAPTIC_VIBRATION};
    vibration.amplitude = intensity;
    vibration.duration = static_cast<XrDuration>(duration * 1e9);
    vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

    XrHapticActionInfo hapticInfo = {XR_TYPE_HAPTIC_ACTION_INFO};
    hapticInfo.action = m_hapticAction;
    hapticInfo.subactionPath = m_handPaths[hand == Hand::Left ? 0 : 1];

    xrApplyHapticFeedback(m_xrSession, &hapticInfo,
                          reinterpret_cast<XrHapticBaseHeader*>(&vibration));
}

// =============================================================================
// OpenXR Initialization
// =============================================================================

bool Engine::initializeXR() {
    LOG_INFO(LOG_TAG_XR) << "========================================";
    LOG_INFO(LOG_TAG_XR) << "OpenXR Startup Sequence";
    LOG_INFO(LOG_TAG_XR) << "========================================";

    // Step 1: Create instance and query runtime
    LOG_INFO(LOG_TAG_XR) << "Step 1/5: Creating OpenXR instance...";
    if (!createXRInstance()) {
        LOG_ERROR(LOG_TAG_XR) << "Step 1/5 FAILED - Cannot create OpenXR instance";
        return false;
    }
    LOG_INFO(LOG_TAG_XR) << "Step 1/5: Instance created successfully";

    // Step 2: Create session with graphics binding
    LOG_INFO(LOG_TAG_XR) << "Step 2/5: Creating XR session...";
    if (!createXRSession()) {
        LOG_ERROR(LOG_TAG_XR) << "Step 2/5 FAILED - Cannot create XR session";
        return false;
    }
    LOG_INFO(LOG_TAG_XR) << "Step 2/5: Session created successfully";

    // Step 3: Create reference spaces
    LOG_INFO(LOG_TAG_XR) << "Step 3/5: Creating reference spaces...";
    if (!createXRSpaces()) {
        LOG_ERROR(LOG_TAG_XR) << "Step 3/5 FAILED - Cannot create reference spaces";
        return false;
    }
    LOG_INFO(LOG_TAG_XR) << "Step 3/5: Reference spaces created";

    // Step 4: Create swapchains for rendering
    LOG_INFO(LOG_TAG_XR) << "Step 4/5: Creating swapchains...";
    if (!createXRSwapchains()) {
        LOG_ERROR(LOG_TAG_XR) << "Step 4/5 FAILED - Cannot create swapchains";
        return false;
    }
    LOG_INFO(LOG_TAG_XR) << "Step 4/5: Swapchains created";

    // Step 5: Create input actions
    LOG_INFO(LOG_TAG_XR) << "Step 5/5: Setting up input actions...";
    if (!createXRActions()) {
        LOG_ERROR(LOG_TAG_XR) << "Step 5/5 FAILED - Cannot create input actions";
        return false;
    }
    LOG_INFO(LOG_TAG_XR) << "Step 5/5: Input actions configured";

    // Print summary
    LOG_INFO(LOG_TAG_XR) << "========================================";
    LOG_INFO(LOG_TAG_XR) << "OpenXR Startup Complete";
    LOG_INFO(LOG_TAG_XR) << "  Runtime: " << m_runtimeName;
    LOG_INFO(LOG_TAG_XR) << "  System: " << m_systemName;
    LOG_INFO(LOG_TAG_XR) << "  Views: " << m_views.size();
    if (!m_views.empty()) {
        LOG_INFO(LOG_TAG_XR) << "  Resolution: " << m_views[0].width << "x" << m_views[0].height << " per eye";
    }
    LOG_INFO(LOG_TAG_XR) << "  Graphics API: " << (m_usingVulkan ? "Vulkan" : (m_usingOpenGL ? "OpenGL" : "None"));
    LOG_INFO(LOG_TAG_XR) << "  Overlay mode: " << (m_overlayActive ? "active" : "inactive");
    LOG_INFO(LOG_TAG_XR) << "========================================";

    m_xrReady = true;
    return true;
}

bool Engine::createXRInstance() {
    LOG_INFO("XR") << "Enumerating OpenXR extensions...";

    // Check available extensions
    uint32_t extensionCount = 0;
    XrResult result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
    if (XR_FAILED(result)) {
        LOG_ERROR("XR") << "Failed to enumerate extensions: " << xrResultToString(result);
        return false;
    }

    LOG_DEBUG("XR") << "Found " << extensionCount << " available extensions";

    std::vector<XrExtensionProperties> extensions(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensions.data());

    // Required extensions
    std::vector<const char*> enabledExtensions;

    // Check for Vulkan (preferred) or OpenGL
    bool hasVulkan = false;
    bool hasOpenGL = false;
    bool hasOverlay = false;
    bool hasHandTracking = false;

    for (const auto& ext : extensions) {
        LOG_DEBUG("XR") << "  Extension: " << ext.extensionName << " v" << ext.extensionVersion;

        if (strcmp(ext.extensionName, "XR_KHR_vulkan_enable") == 0) {
            hasVulkan = true;
        }
        if (strcmp(ext.extensionName, "XR_KHR_opengl_enable") == 0) {
            hasOpenGL = true;
        }
        if (strcmp(ext.extensionName, "XR_EXTX_overlay") == 0) {
            hasOverlay = true;
        }
        if (strcmp(ext.extensionName, "XR_EXT_hand_tracking") == 0) {
            hasHandTracking = true;
        }
    }

    LOG_INFO("XR") << "Graphics APIs: Vulkan=" << (hasVulkan ? "yes" : "no")
                   << ", OpenGL=" << (hasOpenGL ? "yes" : "no");
    LOG_INFO("XR") << "Features: Overlay=" << (hasOverlay ? "yes" : "no")
                   << ", HandTracking=" << (hasHandTracking ? "yes" : "no");

    if (hasVulkan) {
        enabledExtensions.push_back("XR_KHR_vulkan_enable");
        LOG_INFO("XR") << "Using Vulkan graphics API";
    } else if (hasOpenGL) {
        enabledExtensions.push_back("XR_KHR_opengl_enable");
        LOG_INFO("XR") << "Using OpenGL graphics API";
    } else {
        LOG_WARN("XR") << "No graphics API extension found - session may fail";
    }

    if (hasOverlay && m_config.requestOverlay) {
        enabledExtensions.push_back("XR_EXTX_overlay");
        m_overlayActive = true;
        LOG_INFO("XR") << "Overlay mode enabled";
    }

    // Create instance
    XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
    strncpy(createInfo.applicationInfo.applicationName, m_config.appName.c_str(), XR_MAX_APPLICATION_NAME_SIZE - 1);
    createInfo.applicationInfo.applicationVersion = m_config.appVersion;
    strncpy(createInfo.applicationInfo.engineName, "Movement Dojo Engine", XR_MAX_ENGINE_NAME_SIZE - 1);
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.enabledExtensionNames = enabledExtensions.data();

    LOG_INFO("XR") << "Creating XR instance with " << enabledExtensions.size() << " extensions...";

    result = xrCreateInstance(&createInfo, &m_xrInstance);
    if (XR_FAILED(result)) {
        LOG_ERROR("XR") << "Failed to create XR instance: " << xrResultToString(result);
        log("Failed to create XR instance");
        return false;
    }

    LOG_INFO("XR") << "XR instance created successfully";

    // Get runtime properties for diagnostics
    XrInstanceProperties instanceProps = {XR_TYPE_INSTANCE_PROPERTIES};
    result = xrGetInstanceProperties(m_xrInstance, &instanceProps);
    if (XR_SUCCEEDED(result)) {
        m_runtimeName = instanceProps.runtimeName;
        uint32_t major = XR_VERSION_MAJOR(instanceProps.runtimeVersion);
        uint32_t minor = XR_VERSION_MINOR(instanceProps.runtimeVersion);
        uint32_t patch = XR_VERSION_PATCH(instanceProps.runtimeVersion);
        LOG_INFO("XR") << "OpenXR Runtime: " << m_runtimeName
                       << " v" << major << "." << minor << "." << patch;
        log("OpenXR Runtime: " + m_runtimeName);
    } else {
        LOG_WARN("XR") << "Could not get runtime properties: " << xrResultToString(result);
        m_runtimeName = "Unknown";
    }

    // Get system
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    LOG_INFO("XR") << "Getting HMD system...";
    result = xrGetSystem(m_xrInstance, &systemInfo, &m_xrSystemId);
    if (XR_FAILED(result)) {
        LOG_ERROR("XR") << "Failed to get XR system: " << xrResultToString(result);
        LOG_ERROR("XR") << "Is an HMD connected and the runtime active?";
        LOG_ERROR("XR") << "  For Quest 3 via Virtual Desktop: Ensure SteamVR is running";
        LOG_ERROR("XR") << "  For Quest 3 via Link: Ensure Oculus app is running";
        log("Failed to get XR system - is HMD connected?");
        return false;
    }

    // Get system properties
    XrSystemProperties systemProps = {XR_TYPE_SYSTEM_PROPERTIES};
    result = xrGetSystemProperties(m_xrInstance, m_xrSystemId, &systemProps);
    if (XR_SUCCEEDED(result)) {
        m_systemName = systemProps.systemName;
        LOG_INFO("XR") << "XR System: " << m_systemName;
        LOG_INFO("XR") << "  System ID: " << m_xrSystemId;
        LOG_INFO("XR") << "  Vendor ID: " << systemProps.vendorId;
        LOG_INFO("XR") << "  Max layers: " << systemProps.graphicsProperties.maxLayerCount;
        LOG_INFO("XR") << "  Max swapchain size: " << systemProps.graphicsProperties.maxSwapchainImageWidth
                       << "x" << systemProps.graphicsProperties.maxSwapchainImageHeight;
        LOG_INFO("XR") << "  Orientation tracking: " << (systemProps.trackingProperties.orientationTracking ? "yes" : "no");
        LOG_INFO("XR") << "  Position tracking: " << (systemProps.trackingProperties.positionTracking ? "yes" : "no");

        log("XR System: " + m_systemName);
    } else {
        LOG_WARN("XR") << "Could not get system properties: " << xrResultToString(result);
        m_systemName = "Unknown";
    }

    return true;
}

bool Engine::createXRSession() {
    LOG_DEBUG(LOG_TAG_XR) << "Creating XR session for system " << m_xrSystemId;

    // For now, create a headless session (graphics binding handled separately)
    // In production, would create proper Vulkan/OpenGL binding
    XrSessionCreateInfo sessionInfo = {XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.systemId = m_xrSystemId;
    sessionInfo.next = nullptr;  // Graphics binding would go here

    XrResult result = xrCreateSession(m_xrInstance, &sessionInfo, &m_xrSession);
    if (XR_FAILED(result)) {
        LOG_ERROR(LOG_TAG_XR) << "Failed to create XR session: " << xrResultToString(result);
        LOG_ERROR(LOG_TAG_XR) << "  System ID: " << m_xrSystemId;
        LOG_ERROR(LOG_TAG_XR) << "  Possible causes:";
        LOG_ERROR(LOG_TAG_XR) << "    - HMD disconnected after system query";
        LOG_ERROR(LOG_TAG_XR) << "    - Graphics binding mismatch";
        LOG_ERROR(LOG_TAG_XR) << "    - Runtime configuration issue";
        return false;
    }
    LOG_INFO(LOG_TAG_XR) << "XR session created successfully";

    // Query available view configurations
    uint32_t viewConfigCount = 0;
    result = xrEnumerateViewConfigurations(m_xrInstance, m_xrSystemId, 0, &viewConfigCount, nullptr);
    if (XR_SUCCEEDED(result) && viewConfigCount > 0) {
        std::vector<XrViewConfigurationType> viewConfigs(viewConfigCount);
        xrEnumerateViewConfigurations(m_xrInstance, m_xrSystemId, viewConfigCount, &viewConfigCount, viewConfigs.data());
        LOG_DEBUG(LOG_TAG_XR) << "Available view configurations: " << viewConfigCount;
        for (uint32_t i = 0; i < viewConfigCount; i++) {
            const char* configName = "UNKNOWN";
            switch (viewConfigs[i]) {
                case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO: configName = "PRIMARY_MONO"; break;
                case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO: configName = "PRIMARY_STEREO"; break;
                case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO: configName = "PRIMARY_QUAD_VARJO"; break;
                default: break;
            }
            LOG_DEBUG(LOG_TAG_XR) << "  [" << i << "] " << configName;
        }
    }

    // Get view configuration views for stereo
    uint32_t viewCount = 0;
    result = xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
    if (XR_FAILED(result)) {
        LOG_ERROR(LOG_TAG_XR) << "Failed to enumerate view config views: " << xrResultToString(result);
        return false;
    }

    if (viewCount == 0) {
        LOG_ERROR(LOG_TAG_XR) << "No stereo views available - HMD may not support stereo rendering";
        return false;
    }

    m_viewConfigViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    result = xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, m_viewConfigViews.data());
    if (XR_FAILED(result)) {
        LOG_ERROR(LOG_TAG_XR) << "Failed to get view config details: " << xrResultToString(result);
        return false;
    }

    LOG_INFO(LOG_TAG_XR) << "View configuration: " << viewCount << " views (stereo)";
    for (uint32_t i = 0; i < viewCount; i++) {
        const auto& v = m_viewConfigViews[i];
        LOG_DEBUG(LOG_TAG_XR) << "  View " << i << ": "
                              << v.recommendedImageRectWidth << "x" << v.recommendedImageRectHeight
                              << " (max " << v.maxImageRectWidth << "x" << v.maxImageRectHeight << ")"
                              << " samples=" << v.recommendedSwapchainSampleCount;

        // Store resolution in our view structs
        if (i < m_views.size()) {
            m_views[i].width = v.recommendedImageRectWidth;
            m_views[i].height = v.recommendedImageRectHeight;
        }
    }

    m_xrViews.resize(viewCount, {XR_TYPE_VIEW});
    m_views.resize(viewCount);

    // Update view resolution from config
    for (size_t i = 0; i < m_views.size() && i < m_viewConfigViews.size(); i++) {
        m_views[i].width = m_viewConfigViews[i].recommendedImageRectWidth;
        m_views[i].height = m_viewConfigViews[i].recommendedImageRectHeight;
    }

    LOG_INFO(LOG_TAG_XR) << "Session created with " << viewCount << " views";
    return true;
}

bool Engine::createXRSpaces() {
    LOG_DEBUG(LOG_TAG_XR) << "Creating reference spaces...";

    // First, enumerate available reference space types
    uint32_t spaceCount = 0;
    XrResult result = xrEnumerateReferenceSpaces(m_xrSession, 0, &spaceCount, nullptr);
    if (XR_SUCCEEDED(result) && spaceCount > 0) {
        std::vector<XrReferenceSpaceType> spaces(spaceCount);
        xrEnumerateReferenceSpaces(m_xrSession, spaceCount, &spaceCount, spaces.data());

        LOG_INFO(LOG_TAG_XR) << "Available reference spaces: " << spaceCount;
        bool hasStage = false, hasLocal = false, hasView = false;
        for (uint32_t i = 0; i < spaceCount; i++) {
            const char* spaceName = "UNKNOWN";
            switch (spaces[i]) {
                case XR_REFERENCE_SPACE_TYPE_VIEW: spaceName = "VIEW"; hasView = true; break;
                case XR_REFERENCE_SPACE_TYPE_LOCAL: spaceName = "LOCAL"; hasLocal = true; break;
                case XR_REFERENCE_SPACE_TYPE_STAGE: spaceName = "STAGE"; hasStage = true; break;
                case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT: spaceName = "UNBOUNDED_MSFT"; break;
                case XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO: spaceName = "COMBINED_EYE_VARJO"; break;
                default: break;
            }
            LOG_DEBUG(LOG_TAG_XR) << "  [" << i << "] " << spaceName;
        }

        if (!hasStage) {
            LOG_WARN(LOG_TAG_XR) << "STAGE space not available - room-scale tracking may be limited";
        }
        if (!hasLocal) {
            LOG_ERROR(LOG_TAG_XR) << "LOCAL space not available - this is required for VR";
            return false;
        }
    }

    XrReferenceSpaceCreateInfo spaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};  // Identity pose

    // Stage space (room-scale) - preferred for dojo combat
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    result = xrCreateReferenceSpace(m_xrSession, &spaceInfo, &m_stageSpace);
    if (XR_SUCCEEDED(result)) {
        LOG_INFO(LOG_TAG_XR) << "Created STAGE space (room-scale tracking)";
    } else {
        LOG_WARN(LOG_TAG_XR) << "Failed to create STAGE space: " << xrResultToString(result);
        LOG_WARN(LOG_TAG_XR) << "  Falling back to LOCAL space as primary tracking space";

        // Fall back to local space
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        result = xrCreateReferenceSpace(m_xrSession, &spaceInfo, &m_stageSpace);
        if (XR_FAILED(result)) {
            LOG_ERROR(LOG_TAG_XR) << "Failed to create LOCAL space as fallback: " << xrResultToString(result);
            LOG_ERROR(LOG_TAG_XR) << "  Cannot establish tracking origin - aborting";
            return false;
        }
        LOG_INFO(LOG_TAG_XR) << "Using LOCAL space as primary (seated/standing mode)";
    }

    // Local space (for seated/standing reference)
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    result = xrCreateReferenceSpace(m_xrSession, &spaceInfo, &m_localSpace);
    if (XR_FAILED(result)) {
        LOG_WARN(LOG_TAG_XR) << "Failed to create separate LOCAL space: " << xrResultToString(result);
        // Not fatal - we can use stage space
    } else {
        LOG_DEBUG(LOG_TAG_XR) << "Created LOCAL space";
    }

    // View space (head-locked content)
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    result = xrCreateReferenceSpace(m_xrSession, &spaceInfo, &m_viewSpace);
    if (XR_FAILED(result)) {
        LOG_WARN(LOG_TAG_XR) << "Failed to create VIEW space: " << xrResultToString(result);
        // Not fatal - head-locked overlays won't work
    } else {
        LOG_DEBUG(LOG_TAG_XR) << "Created VIEW space (for head-locked UI)";
    }

    LOG_INFO(LOG_TAG_XR) << "Reference spaces initialized successfully";
    return true;
}

bool Engine::createXRSwapchains() {
    LOG_INFO("XR") << "Creating swapchains for " << m_viewConfigViews.size() << " views";

    // Create swapchain for each view
    for (size_t i = 0; i < m_viewConfigViews.size(); i++) {
        const auto& viewConfig = m_viewConfigViews[i];
        SwapchainData swapchain;
        swapchain.width = viewConfig.recommendedImageRectWidth;
        swapchain.height = viewConfig.recommendedImageRectHeight;
        swapchain.acquiredIndex = 0;
        swapchain.imageAcquired = false;

        LOG_DEBUG("XR") << "  View " << i << ": " << swapchain.width << "x" << swapchain.height;

        XrSwapchainCreateInfo swapchainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                                   XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        swapchainInfo.format = 0;  // Graphics binding sets actual format
        swapchainInfo.sampleCount = viewConfig.recommendedSwapchainSampleCount;
        swapchainInfo.width = swapchain.width;
        swapchainInfo.height = swapchain.height;
        swapchainInfo.faceCount = 1;
        swapchainInfo.arraySize = 1;
        swapchainInfo.mipCount = 1;

        XrResult result = xrCreateSwapchain(m_xrSession, &swapchainInfo, &swapchain.handle);
        if (XR_FAILED(result)) {
            LOG_WARN("XR") << "Failed to create swapchain " << i << ": " << xrResultToString(result);
            swapchain.handle = XR_NULL_HANDLE;
        } else {
            // Enumerate swapchain images
            uint32_t imageCount = 0;
            xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr);
            LOG_DEBUG("XR") << "  Swapchain " << i << " has " << imageCount << " images";
        }

        m_swapchains.push_back(swapchain);
    }

    // Initialize projection views array
    m_projectionViews.resize(m_viewConfigViews.size(), {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});

    LOG_INFO("XR") << "Created " << m_swapchains.size() << " swapchains";
    return true;
}

bool Engine::createXRActions() {
    LOG_INFO(LOG_TAG_INPUT) << "Setting up input actions for dojo combat...";

    // Create action set
    XrActionSetCreateInfo actionSetInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy(actionSetInfo.actionSetName, "dojo_combat");
    strcpy(actionSetInfo.localizedActionSetName, "Dojo Combat");

    XrResult result = xrCreateActionSet(m_xrInstance, &actionSetInfo, &m_actionSet);
    if (XR_FAILED(result)) {
        LOG_XR_RESULT(LOG_TAG_INPUT, result, "Create action set 'dojo_combat'");
        return false;
    }
    LOG_INFO(LOG_TAG_INPUT) << "Created action set: dojo_combat";

    // Create hand paths
    xrStringToPath(m_xrInstance, "/user/hand/left", &m_handPaths[0]);
    xrStringToPath(m_xrInstance, "/user/hand/right", &m_handPaths[1]);

    // Create pose action for both hands
    XrActionCreateInfo actionInfo = {XR_TYPE_ACTION_CREATE_INFO};
    actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strcpy(actionInfo.actionName, "hand_pose");
    strcpy(actionInfo.localizedActionName, "Hand Pose");
    actionInfo.countSubactionPaths = 2;
    actionInfo.subactionPaths = m_handPaths;
    result = xrCreateAction(m_actionSet, &actionInfo, &m_poseAction);
    LOG_INFO(LOG_TAG_INPUT) << "  Action: hand_pose (POSE) - LeftHand=blaster, RightHand=saber";

    // Create trigger action (fire blaster / activate saber)
    actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
    strcpy(actionInfo.actionName, "trigger");
    strcpy(actionInfo.localizedActionName, "Trigger");
    result = xrCreateAction(m_actionSet, &actionInfo, &m_triggerAction);
    LOG_INFO(LOG_TAG_INPUT) << "  Action: trigger (FLOAT) - Left=fire blaster, Right=saber swing";

    // Create grip action (grip weapon)
    strcpy(actionInfo.actionName, "grip");
    strcpy(actionInfo.localizedActionName, "Grip");
    result = xrCreateAction(m_actionSet, &actionInfo, &m_gripAction);
    LOG_INFO(LOG_TAG_INPUT) << "  Action: grip (FLOAT) - Weapon grip strength";

    // Create haptic action (force feedback)
    actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
    strcpy(actionInfo.actionName, "haptic");
    strcpy(actionInfo.localizedActionName, "Haptic");
    result = xrCreateAction(m_actionSet, &actionInfo, &m_hapticAction);
    LOG_INFO(LOG_TAG_INPUT) << "  Action: haptic (VIBRATION) - Impact feedback";

    // Create hand spaces for tracking
    for (int i = 0; i < 2; i++) {
        XrActionSpaceCreateInfo spaceInfo = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
        spaceInfo.action = m_poseAction;
        spaceInfo.subactionPath = m_handPaths[i];
        spaceInfo.poseInActionSpace = {{0, 0, 0, 1}, {0, 0, 0}};
        result = xrCreateActionSpace(m_xrSession, &spaceInfo, &m_handSpaces[i]);
        if (XR_FAILED(result)) {
            LOG_WARN(LOG_TAG_INPUT) << "Failed to create hand space " << i;
        }
    }
    LOG_DEBUG(LOG_TAG_INPUT) << "Created hand spaces for pose tracking";

    // Helper to add bindings and log them
    auto suggestProfileBindings = [&](const char* profilePath, const char* profileName) {
        std::vector<XrActionSuggestedBinding> bindings;

        auto addBinding = [&](XrAction action, const char* path, const char* desc) {
            XrPath xrPath;
            if (XR_SUCCEEDED(xrStringToPath(m_xrInstance, path, &xrPath))) {
                bindings.push_back({action, xrPath});
                LOG_TRACE(LOG_TAG_INPUT) << "    Binding: " << path << " -> " << desc;
            }
        };

        LOG_INFO(LOG_TAG_INPUT) << "Suggesting bindings for: " << profileName;

        // Pose bindings (grip pose for weapon handling)
        addBinding(m_poseAction, "/user/hand/left/input/grip/pose", "LeftHandPose (blaster)");
        addBinding(m_poseAction, "/user/hand/right/input/grip/pose", "RightHandPose (saber)");

        // Trigger bindings
        addBinding(m_triggerAction, "/user/hand/left/input/trigger/value", "LeftTrigger (fire)");
        addBinding(m_triggerAction, "/user/hand/right/input/trigger/value", "RightTrigger (swing)");

        // Grip bindings
        addBinding(m_gripAction, "/user/hand/left/input/squeeze/value", "LeftGrip");
        addBinding(m_gripAction, "/user/hand/right/input/squeeze/value", "RightGrip");

        // Haptic bindings
        addBinding(m_hapticAction, "/user/hand/left/output/haptic", "LeftHaptic");
        addBinding(m_hapticAction, "/user/hand/right/output/haptic", "RightHaptic");

        XrPath interactionProfile;
        if (XR_FAILED(xrStringToPath(m_xrInstance, profilePath, &interactionProfile))) {
            LOG_WARN(LOG_TAG_INPUT) << "  Profile path not found: " << profilePath;
            return;
        }

        XrInteractionProfileSuggestedBinding suggestedBindings = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestedBindings.interactionProfile = interactionProfile;
        suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        suggestedBindings.suggestedBindings = bindings.data();

        result = xrSuggestInteractionProfileBindings(m_xrInstance, &suggestedBindings);
        if (XR_SUCCEEDED(result)) {
            LOG_INFO(LOG_TAG_INPUT) << "  Suggested " << bindings.size() << " bindings for " << profileName;
        } else {
            LOG_WARN(LOG_TAG_INPUT) << "  Failed to suggest bindings for " << profileName
                                    << ": " << xrResultToString(result);
        }
    };

    // Suggest bindings for multiple controller profiles
    // Quest 3 / Quest 2 controllers (via Virtual Desktop / Link)
    suggestProfileBindings("/interaction_profiles/oculus/touch_controller",
                          "Oculus Touch (Quest 2/3)");

    // Valve Index controllers
    suggestProfileBindings("/interaction_profiles/valve/index_controller",
                          "Valve Index");

    // Generic simple controller (fallback)
    suggestProfileBindings("/interaction_profiles/khr/simple_controller",
                          "Simple Controller (fallback)");

    // Attach action set to session
    XrSessionActionSetsAttachInfo attachInfo = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;
    result = xrAttachSessionActionSets(m_xrSession, &attachInfo);
    if (XR_FAILED(result)) {
        LOG_XR_RESULT(LOG_TAG_INPUT, result, "Attach action sets");
        return false;
    }

    LOG_INFO(LOG_TAG_INPUT) << "Input actions configured successfully";
    LOG_INFO(LOG_TAG_INPUT) << "  Left hand: Blaster (trigger=fire, grip=hold)";
    LOG_INFO(LOG_TAG_INPUT) << "  Right hand: Saber (trigger=swing, grip=hold)";

    return true;
}

// =============================================================================
// XR Event Handling
// =============================================================================

void Engine::pollXREvents() {
    XrEventDataBuffer event = {XR_TYPE_EVENT_DATA_BUFFER};

    while (xrPollEvent(m_xrInstance, &event) == XR_SUCCESS) {
        switch (event.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                auto* stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                handleSessionStateChange(stateEvent->state);
                break;
            }
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                m_running = false;
                break;
            default:
                break;
        }
        event = {XR_TYPE_EVENT_DATA_BUFFER};
    }
}

void Engine::handleSessionStateChange(XrSessionState newState) {
    // Convert state to readable string for logging
    auto stateToString = [](XrSessionState state) -> const char* {
        switch (state) {
            case XR_SESSION_STATE_UNKNOWN:        return "UNKNOWN";
            case XR_SESSION_STATE_IDLE:           return "IDLE";
            case XR_SESSION_STATE_READY:          return "READY";
            case XR_SESSION_STATE_SYNCHRONIZED:   return "SYNCHRONIZED";
            case XR_SESSION_STATE_VISIBLE:        return "VISIBLE";
            case XR_SESSION_STATE_FOCUSED:        return "FOCUSED";
            case XR_SESSION_STATE_STOPPING:       return "STOPPING";
            case XR_SESSION_STATE_LOSS_PENDING:   return "LOSS_PENDING";
            case XR_SESSION_STATE_EXITING:        return "EXITING";
            default:                              return "INVALID";
        }
    };

    XrSessionState oldState = m_xrSessionState;
    m_xrSessionState = newState;

    // Log all state transitions with context
    LOG_INFO(LOG_TAG_XR) << "Session state: " << stateToString(oldState)
                         << " -> " << stateToString(newState);

    switch (newState) {
        case XR_SESSION_STATE_IDLE:
            LOG_DEBUG(LOG_TAG_XR) << "  Session created, waiting for runtime";
            break;

        case XR_SESSION_STATE_READY: {
            LOG_INFO(LOG_TAG_XR) << "  Session ready - beginning XR session...";
            XrSessionBeginInfo beginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
            beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

            XrResult result = xrBeginSession(m_xrSession, &beginInfo);
            if (XR_SUCCEEDED(result)) {
                m_xrReady = true;
                LOG_INFO(LOG_TAG_XR) << "  XR session begun successfully - rendering enabled";
            } else {
                LOG_ERROR(LOG_TAG_XR) << "  Failed to begin XR session: " << xrResultToString(result);
                LOG_ERROR(LOG_TAG_XR) << "  This may indicate a runtime or HMD configuration issue";
                m_xrReady = false;
            }
            break;
        }

        case XR_SESSION_STATE_SYNCHRONIZED:
            LOG_INFO(LOG_TAG_XR) << "  Session synchronized with runtime";
            break;

        case XR_SESSION_STATE_VISIBLE:
            LOG_INFO(LOG_TAG_XR) << "  Session visible - HMD displaying content";
            break;

        case XR_SESSION_STATE_FOCUSED:
            LOG_INFO(LOG_TAG_XR) << "  Session focused - full input available";
            if (m_sessionActive) {
                resumeSession();
            }
            break;

        case XR_SESSION_STATE_STOPPING:
            LOG_INFO(LOG_TAG_XR) << "  Session stopping - ending XR session...";
            {
                XrResult result = xrEndSession(m_xrSession);
                if (XR_SUCCEEDED(result)) {
                    LOG_INFO(LOG_TAG_XR) << "  XR session ended cleanly";
                } else {
                    LOG_WARN(LOG_TAG_XR) << "  xrEndSession returned: " << xrResultToString(result);
                }
            }
            m_xrReady = false;
            break;

        case XR_SESSION_STATE_EXITING:
            LOG_WARN(LOG_TAG_XR) << "  Session exiting - user requested quit or runtime closing";
            m_running = false;
            break;

        case XR_SESSION_STATE_LOSS_PENDING:
            LOG_ERROR(LOG_TAG_XR) << "  Session loss pending - runtime disconnecting!";
            LOG_ERROR(LOG_TAG_XR) << "  Check HMD connection and Virtual Desktop/SteamVR status";
            m_running = false;
            break;

        default:
            LOG_WARN(LOG_TAG_XR) << "  Unhandled session state: " << static_cast<int>(newState);
            break;
    }
}

bool Engine::beginXRFrame() {
    m_xrFrameState = {XR_TYPE_FRAME_STATE};
    XrResult result = xrWaitFrame(m_xrSession, nullptr, &m_xrFrameState);
    if (XR_FAILED(result)) return false;

    m_predictedDisplayTime = m_xrFrameState.predictedDisplayTime;
    m_shouldRender = m_xrFrameState.shouldRender;

    result = xrBeginFrame(m_xrSession, nullptr);
    return XR_SUCCEEDED(result);
}

void Engine::endXRFrame() {
    XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = m_predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

    // Build projection layer if we should render
    XrCompositionLayerProjection projectionLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    const XrCompositionLayerBaseHeader* layers[1] = {nullptr};

    if (m_shouldRender && !m_projectionViews.empty() && !m_swapchains.empty()) {
        // Verify we have valid swapchains
        bool hasValidSwapchains = true;
        for (const auto& sc : m_swapchains) {
            if (sc.handle == XR_NULL_HANDLE) {
                hasValidSwapchains = false;
                break;
            }
        }

        if (hasValidSwapchains) {
            projectionLayer.space = m_stageSpace;
            projectionLayer.viewCount = static_cast<uint32_t>(m_projectionViews.size());
            projectionLayer.views = m_projectionViews.data();

            layers[0] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer);
            endInfo.layerCount = 1;
            endInfo.layers = layers;
        }
    }

    // If not rendering (e.g., HMD not visible), submit empty frame
    if (endInfo.layerCount == 0) {
        endInfo.layers = nullptr;
    }

    XrResult result = xrEndFrame(m_xrSession, &endInfo);
    if (XR_FAILED(result)) {
        LOG_WARN("XR") << "xrEndFrame failed: " << xrResultToString(result);
    }
}

void Engine::locateViews() {
    XrViewState viewState = {XR_TYPE_VIEW_STATE};
    XrViewLocateInfo locateInfo = {XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    locateInfo.displayTime = m_predictedDisplayTime;
    locateInfo.space = m_stageSpace;

    uint32_t viewCount = static_cast<uint32_t>(m_xrViews.size());
    xrLocateViews(m_xrSession, &locateInfo, &viewState, viewCount, &viewCount, m_xrViews.data());

    // Convert to our view format
    for (size_t i = 0; i < m_xrViews.size(); i++) {
        const auto& xrView = m_xrViews[i];
        auto& view = m_views[i];

        view.pose.position = Vec3(
            xrView.pose.position.x,
            xrView.pose.position.y,
            xrView.pose.position.z
        );
        view.pose.orientation = Quat(
            xrView.pose.orientation.w,
            xrView.pose.orientation.x,
            xrView.pose.orientation.y,
            xrView.pose.orientation.z
        );
        view.fov.angleLeft = xrView.fov.angleLeft;
        view.fov.angleRight = xrView.fov.angleRight;
        view.fov.angleUp = xrView.fov.angleUp;
        view.fov.angleDown = xrView.fov.angleDown;
    }

    // Update head pose (average of both eyes)
    if (m_views.size() >= 2) {
        m_headPose.position = Vec3(
            (m_views[0].pose.position.x + m_views[1].pose.position.x) * 0.5f,
            (m_views[0].pose.position.y + m_views[1].pose.position.y) * 0.5f,
            (m_views[0].pose.position.z + m_views[1].pose.position.z) * 0.5f
        );
        m_headPose.orientation = m_views[0].pose.orientation;
    }
}

void Engine::syncActions() {
    XrActiveActionSet activeSet = {};
    activeSet.actionSet = m_actionSet;
    activeSet.subactionPath = XR_NULL_PATH;

    XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeSet;

    XrResult result = xrSyncActions(m_xrSession, &syncInfo);
    if (XR_FAILED(result) && result != -26) {  // -26 = SESSION_NOT_FOCUSED is normal
        LOG_DEBUG(LOG_TAG_INPUT) << "xrSyncActions: " << xrResultToString(result);
    }

    // Get controller poses
    for (int i = 0; i < 2; i++) {
        ControllerState& controller = (i == 0) ? m_leftController : m_rightController;
        int& lostFrames = (i == 0) ? m_leftHandLostFrames : m_rightHandLostFrames;
        bool& warnedLost = (i == 0) ? m_leftHandWarnedLost : m_rightHandWarnedLost;
        const char* handName = (i == 0) ? "LeftHand (blaster)" : "RightHand (saber)";

        XrSpaceLocation spaceLocation = {XR_TYPE_SPACE_LOCATION};
        xrLocateSpace(m_handSpaces[i], m_stageSpace, m_predictedDisplayTime, &spaceLocation);

        bool wasTracked = controller.isTracked;
        controller.isTracked = (spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;

        if (controller.isTracked) {
            // Tracking recovered
            if (warnedLost) {
                LOG_INFO(LOG_TAG_INPUT) << handName << " tracking recovered after " << lostFrames << " frames";
                warnedLost = false;
            }
            lostFrames = 0;

            controller.position = Vec3(
                spaceLocation.pose.position.x,
                spaceLocation.pose.position.y,
                spaceLocation.pose.position.z
            );
            controller.orientation = Quat(
                spaceLocation.pose.orientation.w,
                spaceLocation.pose.orientation.x,
                spaceLocation.pose.orientation.y,
                spaceLocation.pose.orientation.z
            );
            controller.pose.position = controller.position;
            controller.pose.orientation = controller.orientation;
        } else {
            // Tracking lost
            lostFrames++;
            if (lostFrames == 1 && wasTracked) {
                LOG_DEBUG(LOG_TAG_INPUT) << handName << " tracking lost";
            }
            if (lostFrames >= TRACKING_LOST_WARN_FRAMES && !warnedLost) {
                LOG_WARN(LOG_TAG_INPUT) << handName << " not tracked for >" << TRACKING_LOST_WARN_FRAMES
                                        << " frames (possible tracking/binding issue)";
                warnedLost = true;
            }
        }

        // Get trigger/grip values
        XrActionStateFloat floatState = {XR_TYPE_ACTION_STATE_FLOAT};
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.subactionPath = m_handPaths[i];

        getInfo.action = m_triggerAction;
        xrGetActionStateFloat(m_xrSession, &getInfo, &floatState);
        float prevTrigger = controller.triggerValue;
        controller.triggerValue = floatState.currentState;

        // Log first trigger activation
        bool& triggerBoundLogged = (i == 0) ? m_leftTriggerBoundLogged : m_rightTriggerBoundLogged;
        if (!triggerBoundLogged && floatState.isActive && controller.triggerValue > 0.1f) {
            LOG_INFO(LOG_TAG_INPUT) << handName << " trigger bound and active (value=" << controller.triggerValue << ")";
            triggerBoundLogged = true;
        }

        getInfo.action = m_gripAction;
        xrGetActionStateFloat(m_xrSession, &getInfo, &floatState);
        controller.gripValue = floatState.currentState;
    }
}

// =============================================================================
// Rendering
// =============================================================================

void Engine::render() {
    // Acquire swapchain images and render each view
    for (size_t viewIdx = 0; viewIdx < m_views.size() && viewIdx < m_swapchains.size(); viewIdx++) {
        auto& swapchain = m_swapchains[viewIdx];

        if (swapchain.handle == XR_NULL_HANDLE) {
            continue;
        }

        // Acquire swapchain image
        XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        XrResult result = xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &swapchain.acquiredIndex);
        if (XR_FAILED(result)) {
            LOG_WARN("Render") << "Failed to acquire swapchain image for view " << viewIdx;
            continue;
        }
        swapchain.imageAcquired = true;

        // Wait for swapchain image to be available
        XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        result = xrWaitSwapchainImage(swapchain.handle, &waitInfo);
        if (XR_FAILED(result)) {
            LOG_WARN("Render") << "Failed to wait for swapchain image for view " << viewIdx;
        }

        // ====================================================================
        // GPU RENDERING WOULD HAPPEN HERE
        // In a real implementation with Vulkan/OpenGL:
        // 1. Bind framebuffer with swapchain image as color attachment
        // 2. Clear to m_config.clearColor
        // 3. Set viewport to (0, 0, swapchain.width, swapchain.height)
        // 4. Calculate view and projection matrices from view.pose and view.fov
        // 5. Render scene objects
        // 6. Render debug draw commands
        // ====================================================================

        // Build projection view for this eye
        XrCompositionLayerProjectionView& projView = m_projectionViews[viewIdx];
        projView.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projView.pose = m_xrViews[viewIdx].pose;
        projView.fov = m_xrViews[viewIdx].fov;
        projView.subImage.swapchain = swapchain.handle;
        projView.subImage.imageArrayIndex = 0;
        projView.subImage.imageRect.offset = {0, 0};
        projView.subImage.imageRect.extent = {swapchain.width, swapchain.height};

        // Release swapchain image
        XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        result = xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
        if (XR_FAILED(result)) {
            LOG_WARN("Render") << "Failed to release swapchain image for view " << viewIdx;
        }
        swapchain.imageAcquired = false;
    }

    // Track rendered objects for statistics
    size_t objectCount = m_sceneObjects.size();
    size_t drawCommandCount = m_drawCommands.size();

    // Log render stats periodically
    static int frameCounter = 0;
    if (++frameCounter % 300 == 0) {
        LOG_DEBUG("Render") << "Frame " << frameCounter
                           << ": " << objectCount << " objects, "
                           << drawCommandCount << " draw commands";
    }
}

// =============================================================================
// Mock Tracking (for headless testing)
// =============================================================================

void Engine::updateMockTracking() {
    // Generate simulated tracking data for CLI testing
    // Controllers move in a figure-8 pattern, head stays mostly stationary

    float t = static_cast<float>(m_totalTime);

    // Head: slight natural movement
    m_headPose.position = Vec3(
        0.05f * std::sin(t * 0.3f),
        1.6f + 0.02f * std::sin(t * 0.5f),  // Standing height ~1.6m
        0.03f * std::cos(t * 0.4f)
    );
    m_headPose.orientation = Quat(1, 0, 0, 0);  // Looking forward

    // Left controller: figure-8 pattern on left side
    m_leftController.isTracked = true;
    m_leftController.hand = Hand::Left;
    m_leftController.position = Vec3(
        -0.3f + 0.2f * std::sin(t * 1.2f),
        1.0f + 0.3f * std::sin(t * 0.8f),
        -0.4f + 0.15f * std::cos(t * 1.2f)
    );
    m_leftController.pose.position = m_leftController.position;
    m_leftController.pose.orientation = Quat(1, 0, 0, 0);
    m_leftController.orientation = m_leftController.pose.orientation;

    // Calculate velocity from position change
    static Vec3 lastLeftPos = m_leftController.position;
    m_leftController.velocity = (m_leftController.position - lastLeftPos) * (1.0f / static_cast<float>(m_deltaTime));
    lastLeftPos = m_leftController.position;

    // Right controller: figure-8 pattern on right side
    m_rightController.isTracked = true;
    m_rightController.hand = Hand::Right;
    m_rightController.position = Vec3(
        0.3f + 0.2f * std::sin(t * 1.1f + 1.57f),
        1.0f + 0.3f * std::cos(t * 0.9f),
        -0.4f + 0.15f * std::sin(t * 1.1f)
    );
    m_rightController.pose.position = m_rightController.position;
    m_rightController.pose.orientation = Quat(1, 0, 0, 0);
    m_rightController.orientation = m_rightController.pose.orientation;

    // Calculate velocity
    static Vec3 lastRightPos = m_rightController.position;
    m_rightController.velocity = (m_rightController.position - lastRightPos) * (1.0f / static_cast<float>(m_deltaTime));
    lastRightPos = m_rightController.position;

    // Simulate occasional trigger/grip presses
    m_leftController.triggerValue = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;  // 0-0.3
    m_rightController.triggerValue = (std::cos(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
    m_leftController.gripValue = 0.0f;
    m_rightController.gripValue = 0.0f;
}

// =============================================================================
// Environment Management
// =============================================================================

bool Engine::loadEnvironment(EnvironmentType type) {
    LOG_INFO(LOG_TAG_ENGINE) << "Loading environment: " << environmentTypeToString(type);

    // Unload previous environment if exists
    if (m_environment) {
        LOG_DEBUG(LOG_TAG_ENGINE) << "Unloading previous environment...";
        unloadEnvironment();
    }

    // Create new environment
    try {
        m_environment = createEnvironment(type);
        if (!m_environment) {
            LOG_ERROR(LOG_TAG_ENGINE) << "Failed to create environment: " << environmentTypeToString(type);
            return false;
        }

        // Setup the environment
        if (!m_environment->setup(this)) {
            LOG_ERROR(LOG_TAG_ENGINE) << "Failed to setup environment: " << m_environment->getName();
            m_environment.reset();
            return false;
        }

        m_environmentType = type;
        LOG_INFO(LOG_TAG_ENGINE) << "Environment loaded successfully: " << m_environment->getName()
                                  << " - " << m_environment->getDescription();
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(LOG_TAG_ENGINE) << "Exception loading environment: " << e.what();
        m_environment.reset();
        return false;
    }
}

void Engine::unloadEnvironment() {
    if (m_environment) {
        LOG_INFO(LOG_TAG_ENGINE) << "Unloading environment: " << m_environment->getName();
        m_environment->cleanup(this);
        m_environment.reset();
        LOG_DEBUG(LOG_TAG_ENGINE) << "Environment unloaded";
    }
}

} // namespace lst
