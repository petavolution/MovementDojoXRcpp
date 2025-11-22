/**
 * Engine.cpp - Unified VR Engine Implementation
 *
 * Combines OpenXR session, rendering, and input into a single
 * coherent implementation with clear startup sequence.
 */

#include "Engine.h"
#include "Logger.h"
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

    log("Shutting down engine");

    // End active session
    if (m_sessionActive) {
        endSession();
    }

    // Detach all systems
    for (auto& sys : m_systems) {
        sys->onDetach();
    }
    m_systems.clear();

    // Cleanup XR resources
    for (int i = 0; i < 2; i++) {
        if (m_handSpaces[i] != XR_NULL_HANDLE) {
            xrDestroySpace(m_handSpaces[i]);
            m_handSpaces[i] = XR_NULL_HANDLE;
        }
    }

    if (m_actionSet != XR_NULL_HANDLE) {
        xrDestroyActionSet(m_actionSet);
        m_actionSet = XR_NULL_HANDLE;
    }

    for (auto& sw : m_swapchains) {
        if (sw.handle != XR_NULL_HANDLE) {
            xrDestroySwapchain(sw.handle);
        }
    }
    m_swapchains.clear();

    if (m_viewSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_viewSpace);
        m_viewSpace = XR_NULL_HANDLE;
    }
    if (m_localSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_localSpace);
        m_localSpace = XR_NULL_HANDLE;
    }
    if (m_stageSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_stageSpace);
        m_stageSpace = XR_NULL_HANDLE;
    }

    if (m_xrSession != XR_NULL_HANDLE) {
        xrDestroySession(m_xrSession);
        m_xrSession = XR_NULL_HANDLE;
    }

    if (m_xrInstance != XR_NULL_HANDLE) {
        xrDestroyInstance(m_xrInstance);
        m_xrInstance = XR_NULL_HANDLE;
    }

    m_initialized = false;
    m_running = false;
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

    for (auto& sys : m_systems) {
        sys->onSessionEnd();
    }

    m_sessionActive = false;
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
    if (!createXRInstance()) return false;
    if (!createXRSession()) return false;
    if (!createXRSpaces()) return false;
    if (!createXRSwapchains()) return false;
    if (!createXRActions()) return false;

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

    // Get system
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    LOG_INFO("XR") << "Getting HMD system...";
    result = xrGetSystem(m_xrInstance, &systemInfo, &m_xrSystemId);
    if (XR_FAILED(result)) {
        LOG_ERROR("XR") << "Failed to get XR system: " << xrResultToString(result);
        LOG_ERROR("XR") << "Is an HMD connected and the runtime active?";
        log("Failed to get XR system - is HMD connected?");
        return false;
    }

    // Get system properties
    XrSystemProperties systemProps = {XR_TYPE_SYSTEM_PROPERTIES};
    xrGetSystemProperties(m_xrInstance, m_xrSystemId, &systemProps);

    LOG_INFO("XR") << "XR System: " << systemProps.systemName;
    LOG_INFO("XR") << "  Vendor ID: " << systemProps.vendorId;
    LOG_INFO("XR") << "  Max layers: " << systemProps.graphicsProperties.maxLayerCount;
    LOG_INFO("XR") << "  Max swapchain size: " << systemProps.graphicsProperties.maxSwapchainImageWidth
                   << "x" << systemProps.graphicsProperties.maxSwapchainImageHeight;

    log("XR System: " + std::string(systemProps.systemName));

    return true;
}

bool Engine::createXRSession() {
    // For now, create a headless session (graphics binding handled separately)
    // In production, would create proper Vulkan/OpenGL binding

    XrSessionCreateInfo sessionInfo = {XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.systemId = m_xrSystemId;
    sessionInfo.next = nullptr;  // Graphics binding would go here

    XrResult result = xrCreateSession(m_xrInstance, &sessionInfo, &m_xrSession);
    if (XR_FAILED(result)) {
        log("Failed to create XR session");
        return false;
    }

    // Get view configuration
    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);

    m_viewConfigViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, m_viewConfigViews.data());

    m_xrViews.resize(viewCount, {XR_TYPE_VIEW});
    m_views.resize(viewCount);

    return true;
}

bool Engine::createXRSpaces() {
    // Stage space (room-scale)
    XrReferenceSpaceCreateInfo spaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    spaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};

    XrResult result = xrCreateReferenceSpace(m_xrSession, &spaceInfo, &m_stageSpace);
    if (XR_FAILED(result)) {
        // Fall back to local space
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        result = xrCreateReferenceSpace(m_xrSession, &spaceInfo, &m_stageSpace);
        if (XR_FAILED(result)) {
            log("Failed to create reference space");
            return false;
        }
    }

    // Local space
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    xrCreateReferenceSpace(m_xrSession, &spaceInfo, &m_localSpace);

    // View space
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    xrCreateReferenceSpace(m_xrSession, &spaceInfo, &m_viewSpace);

    return true;
}

bool Engine::createXRSwapchains() {
    // Create swapchain for each view
    for (const auto& viewConfig : m_viewConfigViews) {
        SwapchainData swapchain;
        swapchain.width = viewConfig.recommendedImageRectWidth;
        swapchain.height = viewConfig.recommendedImageRectHeight;

        XrSwapchainCreateInfo swapchainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.format = 0;  // Would be set based on graphics API
        swapchainInfo.sampleCount = 1;
        swapchainInfo.width = swapchain.width;
        swapchainInfo.height = swapchain.height;
        swapchainInfo.faceCount = 1;
        swapchainInfo.arraySize = 1;
        swapchainInfo.mipCount = 1;

        XrResult result = xrCreateSwapchain(m_xrSession, &swapchainInfo, &swapchain.handle);
        if (XR_FAILED(result)) {
            swapchain.handle = XR_NULL_HANDLE;
        }

        m_swapchains.push_back(swapchain);
    }

    return true;
}

bool Engine::createXRActions() {
    // Create action set
    XrActionSetCreateInfo actionSetInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy(actionSetInfo.actionSetName, "gameplay");
    strcpy(actionSetInfo.localizedActionSetName, "Gameplay");

    xrCreateActionSet(m_xrInstance, &actionSetInfo, &m_actionSet);

    // Create hand paths
    xrStringToPath(m_xrInstance, "/user/hand/left", &m_handPaths[0]);
    xrStringToPath(m_xrInstance, "/user/hand/right", &m_handPaths[1]);

    // Create pose action
    XrActionCreateInfo actionInfo = {XR_TYPE_ACTION_CREATE_INFO};
    actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strcpy(actionInfo.actionName, "hand_pose");
    strcpy(actionInfo.localizedActionName, "Hand Pose");
    actionInfo.countSubactionPaths = 2;
    actionInfo.subactionPaths = m_handPaths;
    xrCreateAction(m_actionSet, &actionInfo, &m_poseAction);

    // Create trigger action
    actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
    strcpy(actionInfo.actionName, "trigger");
    strcpy(actionInfo.localizedActionName, "Trigger");
    xrCreateAction(m_actionSet, &actionInfo, &m_triggerAction);

    // Create grip action
    strcpy(actionInfo.actionName, "grip");
    strcpy(actionInfo.localizedActionName, "Grip");
    xrCreateAction(m_actionSet, &actionInfo, &m_gripAction);

    // Create haptic action
    actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
    strcpy(actionInfo.actionName, "haptic");
    strcpy(actionInfo.localizedActionName, "Haptic");
    xrCreateAction(m_actionSet, &actionInfo, &m_hapticAction);

    // Create hand spaces
    for (int i = 0; i < 2; i++) {
        XrActionSpaceCreateInfo spaceInfo = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
        spaceInfo.action = m_poseAction;
        spaceInfo.subactionPath = m_handPaths[i];
        spaceInfo.poseInActionSpace = {{0, 0, 0, 1}, {0, 0, 0}};
        xrCreateActionSpace(m_xrSession, &spaceInfo, &m_handSpaces[i]);
    }

    // Suggest bindings for common controllers
    std::vector<XrActionSuggestedBinding> bindings;

    auto addBinding = [&](XrAction action, const char* path) {
        XrPath xrPath;
        if (XR_SUCCEEDED(xrStringToPath(m_xrInstance, path, &xrPath))) {
            bindings.push_back({action, xrPath});
        }
    };

    // Simple controller bindings
    addBinding(m_poseAction, "/user/hand/left/input/grip/pose");
    addBinding(m_poseAction, "/user/hand/right/input/grip/pose");
    addBinding(m_triggerAction, "/user/hand/left/input/trigger/value");
    addBinding(m_triggerAction, "/user/hand/right/input/trigger/value");
    addBinding(m_gripAction, "/user/hand/left/input/squeeze/value");
    addBinding(m_gripAction, "/user/hand/right/input/squeeze/value");
    addBinding(m_hapticAction, "/user/hand/left/output/haptic");
    addBinding(m_hapticAction, "/user/hand/right/output/haptic");

    XrPath interactionProfile;
    xrStringToPath(m_xrInstance, "/interaction_profiles/khr/simple_controller", &interactionProfile);

    XrInteractionProfileSuggestedBinding suggestedBindings = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggestedBindings.interactionProfile = interactionProfile;
    suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
    suggestedBindings.suggestedBindings = bindings.data();

    xrSuggestInteractionProfileBindings(m_xrInstance, &suggestedBindings);

    // Attach action set
    XrSessionActionSetsAttachInfo attachInfo = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;
    xrAttachSessionActionSets(m_xrSession, &attachInfo);

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
    m_xrSessionState = newState;

    switch (newState) {
        case XR_SESSION_STATE_READY: {
            XrSessionBeginInfo beginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
            beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            xrBeginSession(m_xrSession, &beginInfo);
            m_xrReady = true;
            break;
        }
        case XR_SESSION_STATE_STOPPING:
            xrEndSession(m_xrSession);
            m_xrReady = false;
            break;
        case XR_SESSION_STATE_EXITING:
        case XR_SESSION_STATE_LOSS_PENDING:
            m_running = false;
            break;
        default:
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
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;

    xrEndFrame(m_xrSession, &endInfo);
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

    xrSyncActions(m_xrSession, &syncInfo);

    // Get controller poses
    for (int i = 0; i < 2; i++) {
        ControllerState& controller = (i == 0) ? m_leftController : m_rightController;

        XrSpaceLocation spaceLocation = {XR_TYPE_SPACE_LOCATION};
        xrLocateSpace(m_handSpaces[i], m_stageSpace, m_predictedDisplayTime, &spaceLocation);

        controller.isTracked = (spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;

        if (controller.isTracked) {
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
        }

        // Get trigger/grip values
        XrActionStateFloat floatState = {XR_TYPE_ACTION_STATE_FLOAT};
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.subactionPath = m_handPaths[i];

        getInfo.action = m_triggerAction;
        xrGetActionStateFloat(m_xrSession, &getInfo, &floatState);
        controller.triggerValue = floatState.currentState;

        getInfo.action = m_gripAction;
        xrGetActionStateFloat(m_xrSession, &getInfo, &floatState);
        controller.gripValue = floatState.currentState;
    }
}

// =============================================================================
// Rendering
// =============================================================================

void Engine::render() {
    // This is where actual GPU rendering would happen
    // For now, just process draw commands conceptually

    for (const auto& obj : m_sceneObjects) {
        // Would render mesh with material
        (void)obj;
    }

    for (const auto& cmd : m_drawCommands) {
        // Would draw debug primitives
        (void)cmd;
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

} // namespace lst
