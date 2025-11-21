#include "XRSession.h"
#include <cstring>
#include <iostream>
#include <chrono>

// Vulkan headers for graphics binding
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

// OpenXR Vulkan extension
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr_platform.h>

namespace lst {

// Helper macro for OpenXR error checking
#define XR_CHECK(result, msg) \
    do { \
        XrResult _result = (result); \
        if (XR_FAILED(_result)) { \
            std::cerr << "OpenXR Error: " << msg << " (result=" << _result << ")" << std::endl; \
            return false; \
        } \
    } while(0)

XRSession::XRSession() = default;

XRSession::~XRSession() {
    shutdown();
}

bool XRSession::initialize(const AppConfig& config) {
    std::cout << "Initializing XR Session..." << std::endl;

    if (!enumerateExtensions()) {
        std::cerr << "Failed to enumerate extensions" << std::endl;
        return false;
    }

    if (!createInstance(config)) {
        std::cerr << "Failed to create XR instance" << std::endl;
        return false;
    }

    if (!getSystem()) {
        std::cerr << "Failed to get XR system" << std::endl;
        return false;
    }

    // Note: In a full implementation, we'd create the graphics device here
    // For now, we'll stub this out
    std::cout << "XR Session initialized successfully" << std::endl;
    std::cout << "System: " << m_systemName << std::endl;
    std::cout << "Max resolution: " << m_maxSwapchainWidth << "x" << m_maxSwapchainHeight << std::endl;

    m_isRunning = true;
    return true;
}

void XRSession::shutdown() {
    std::cout << "Shutting down XR Session..." << std::endl;

    // Destroy spaces
    if (m_stageSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_stageSpace);
        m_stageSpace = XR_NULL_HANDLE;
    }
    if (m_localSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_localSpace);
        m_localSpace = XR_NULL_HANDLE;
    }
    if (m_viewSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_viewSpace);
        m_viewSpace = XR_NULL_HANDLE;
    }

    // Destroy swapchains
    for (auto& swapchain : m_swapchains) {
        if (swapchain.swapchain != XR_NULL_HANDLE) {
            xrDestroySwapchain(swapchain.swapchain);
        }
    }
    m_swapchains.clear();

    // End and destroy session
    if (m_session != XR_NULL_HANDLE) {
        if (m_sessionBegun) {
            xrEndSession(m_session);
            m_sessionBegun = false;
        }
        xrDestroySession(m_session);
        m_session = XR_NULL_HANDLE;
    }

    // Destroy instance
    if (m_instance != XR_NULL_HANDLE) {
        xrDestroyInstance(m_instance);
        m_instance = XR_NULL_HANDLE;
    }

    m_isRunning = false;
    m_sessionReady = false;
    std::cout << "XR Session shutdown complete" << std::endl;
}

bool XRSession::enumerateExtensions() {
    uint32_t extensionCount = 0;
    XR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
             "Failed to get extension count");

    std::vector<XrExtensionProperties> extensions(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
    XR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensions.data()),
             "Failed to enumerate extensions");

    std::cout << "Available OpenXR extensions:" << std::endl;
    bool hasVulkan = false;
    bool hasOpenGL = false;

    for (const auto& ext : extensions) {
        std::cout << "  - " << ext.extensionName << " (v" << ext.extensionVersion << ")" << std::endl;
        if (strcmp(ext.extensionName, XR_KHR_VULKAN_ENABLE_EXTENSION_NAME) == 0) {
            hasVulkan = true;
        }
        if (strcmp(ext.extensionName, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) == 0) {
            hasOpenGL = true;
        }
    }

    // Select graphics API extension
    if (hasVulkan) {
        m_enabledExtensions.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
        std::cout << "Using Vulkan graphics API" << std::endl;
    } else if (hasOpenGL) {
        m_enabledExtensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
        std::cout << "Using OpenGL graphics API" << std::endl;
    } else {
        std::cerr << "No supported graphics API extension found!" << std::endl;
        return false;
    }

    return true;
}

bool XRSession::createInstance(const AppConfig& config) {
    // Prepare extension names
    std::vector<const char*> extensionNames;
    for (const auto& ext : m_enabledExtensions) {
        extensionNames.push_back(ext.c_str());
    }

    // Application info
    XrApplicationInfo appInfo = {};
    strncpy(appInfo.applicationName, config.appName.c_str(), XR_MAX_APPLICATION_NAME_SIZE - 1);
    appInfo.applicationVersion = config.appVersion;
    strncpy(appInfo.engineName, "LightsaberEngine", XR_MAX_ENGINE_NAME_SIZE - 1);
    appInfo.engineVersion = 1;
    appInfo.apiVersion = XR_CURRENT_API_VERSION;

    // Create instance
    XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo = appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
    createInfo.enabledExtensionNames = extensionNames.data();

    XR_CHECK(xrCreateInstance(&createInfo, &m_instance), "Failed to create XR instance");

    // Get instance properties
    XrInstanceProperties instanceProps = {XR_TYPE_INSTANCE_PROPERTIES};
    XR_CHECK(xrGetInstanceProperties(m_instance, &instanceProps), "Failed to get instance properties");

    std::cout << "OpenXR Runtime: " << instanceProps.runtimeName << std::endl;
    std::cout << "Runtime Version: "
              << XR_VERSION_MAJOR(instanceProps.runtimeVersion) << "."
              << XR_VERSION_MINOR(instanceProps.runtimeVersion) << "."
              << XR_VERSION_PATCH(instanceProps.runtimeVersion) << std::endl;

    return true;
}

bool XRSession::getSystem() {
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XR_CHECK(xrGetSystem(m_instance, &systemInfo, &m_systemId), "Failed to get XR system");

    // Get system properties
    XrSystemProperties systemProps = {XR_TYPE_SYSTEM_PROPERTIES};
    XR_CHECK(xrGetSystemProperties(m_instance, m_systemId, &systemProps), "Failed to get system properties");

    m_systemName = systemProps.systemName;
    m_maxSwapchainWidth = systemProps.graphicsProperties.maxSwapchainImageWidth;
    m_maxSwapchainHeight = systemProps.graphicsProperties.maxSwapchainImageHeight;

    std::cout << "XR System: " << m_systemName << std::endl;
    std::cout << "Vendor ID: " << systemProps.vendorId << std::endl;
    std::cout << "Max Layers: " << systemProps.graphicsProperties.maxLayerCount << std::endl;

    // Enumerate view configurations
    uint32_t viewConfigCount = 0;
    XR_CHECK(xrEnumerateViewConfigurations(m_instance, m_systemId, 0, &viewConfigCount, nullptr),
             "Failed to get view config count");

    std::vector<XrViewConfigurationType> viewConfigs(viewConfigCount);
    XR_CHECK(xrEnumerateViewConfigurations(m_instance, m_systemId, viewConfigCount, &viewConfigCount, viewConfigs.data()),
             "Failed to enumerate view configurations");

    // Find stereo view configuration
    bool foundStereo = false;
    for (auto config : viewConfigs) {
        if (config == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
            m_viewConfigType = config;
            foundStereo = true;
            break;
        }
    }

    if (!foundStereo) {
        std::cerr << "Stereo view configuration not supported!" << std::endl;
        return false;
    }

    // Get view configuration views (per-eye settings)
    uint32_t viewCount = 0;
    XR_CHECK(xrEnumerateViewConfigurationViews(m_instance, m_systemId, m_viewConfigType, 0, &viewCount, nullptr),
             "Failed to get view count");

    m_viewConfigViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    XR_CHECK(xrEnumerateViewConfigurationViews(m_instance, m_systemId, m_viewConfigType, viewCount, &viewCount, m_viewConfigViews.data()),
             "Failed to enumerate view configuration views");

    std::cout << "View count: " << viewCount << std::endl;
    for (size_t i = 0; i < m_viewConfigViews.size(); i++) {
        std::cout << "View " << i << ": "
                  << m_viewConfigViews[i].recommendedImageRectWidth << "x"
                  << m_viewConfigViews[i].recommendedImageRectHeight << std::endl;
    }

    // Initialize XR views
    m_xrViews.resize(viewCount, {XR_TYPE_VIEW});
    m_views.resize(viewCount);

    return true;
}

bool XRSession::createSession() {
    // Note: In a real implementation, this would require a valid graphics binding
    // For now, we'll create a minimal session for demonstration

    // This would need proper graphics binding setup
    // XrGraphicsBindingVulkanKHR vulkanBinding = {...};

    XrSessionCreateInfo sessionCreateInfo = {XR_TYPE_SESSION_CREATE_INFO};
    sessionCreateInfo.systemId = m_systemId;
    // sessionCreateInfo.next = &vulkanBinding; // Would point to graphics binding

    // For validation mode, we skip actual session creation
    std::cout << "Session creation would happen here with graphics binding" << std::endl;

    return true;
}

bool XRSession::createSpaces() {
    if (m_session == XR_NULL_HANDLE) return false;

    // Create stage space (room-scale)
    XrReferenceSpaceCreateInfo stageSpaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    stageSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    stageSpaceInfo.poseInReferenceSpace.orientation = {0, 0, 0, 1};
    stageSpaceInfo.poseInReferenceSpace.position = {0, 0, 0};

    XR_CHECK(xrCreateReferenceSpace(m_session, &stageSpaceInfo, &m_stageSpace),
             "Failed to create stage space");

    // Create local space (seated/standing)
    XrReferenceSpaceCreateInfo localSpaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    localSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    localSpaceInfo.poseInReferenceSpace.orientation = {0, 0, 0, 1};
    localSpaceInfo.poseInReferenceSpace.position = {0, 0, 0};

    XR_CHECK(xrCreateReferenceSpace(m_session, &localSpaceInfo, &m_localSpace),
             "Failed to create local space");

    // Create view space (head-relative)
    XrReferenceSpaceCreateInfo viewSpaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    viewSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    viewSpaceInfo.poseInReferenceSpace.orientation = {0, 0, 0, 1};
    viewSpaceInfo.poseInReferenceSpace.position = {0, 0, 0};

    XR_CHECK(xrCreateReferenceSpace(m_session, &viewSpaceInfo, &m_viewSpace),
             "Failed to create view space");

    return true;
}

bool XRSession::createSwapchains() {
    if (m_session == XR_NULL_HANDLE) return false;

    m_swapchains.resize(m_viewConfigViews.size());

    for (size_t i = 0; i < m_viewConfigViews.size(); i++) {
        const auto& viewConfig = m_viewConfigViews[i];

        // Query supported swapchain formats
        uint32_t formatCount = 0;
        XR_CHECK(xrEnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr),
                 "Failed to get swapchain format count");

        std::vector<int64_t> formats(formatCount);
        XR_CHECK(xrEnumerateSwapchainFormats(m_session, formatCount, &formatCount, formats.data()),
                 "Failed to enumerate swapchain formats");

        // Select format (prefer SRGB for color accuracy)
        int64_t selectedFormat = formats[0]; // Default to first available

        // Create swapchain
        XrSwapchainCreateInfo swapchainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        swapchainInfo.format = selectedFormat;
        swapchainInfo.sampleCount = viewConfig.recommendedSwapchainSampleCount;
        swapchainInfo.width = viewConfig.recommendedImageRectWidth;
        swapchainInfo.height = viewConfig.recommendedImageRectHeight;
        swapchainInfo.faceCount = 1;
        swapchainInfo.arraySize = 1;
        swapchainInfo.mipCount = 1;

        XR_CHECK(xrCreateSwapchain(m_session, &swapchainInfo, &m_swapchains[i].swapchain),
                 "Failed to create swapchain");

        m_swapchains[i].width = swapchainInfo.width;
        m_swapchains[i].height = swapchainInfo.height;

        // Enumerate swapchain images
        uint32_t imageCount = 0;
        XR_CHECK(xrEnumerateSwapchainImages(m_swapchains[i].swapchain, 0, &imageCount, nullptr),
                 "Failed to get swapchain image count");

        // Store image handles (would be XrSwapchainImageVulkanKHR or similar)
        m_swapchains[i].images.resize(imageCount);
    }

    return true;
}

void XRSession::pollEvents() {
    XrEventDataBuffer eventData = {XR_TYPE_EVENT_DATA_BUFFER};

    while (xrPollEvent(m_instance, &eventData) == XR_SUCCESS) {
        switch (eventData.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                auto* stateChanged = reinterpret_cast<XrEventDataSessionStateChanged*>(&eventData);
                handleSessionStateChanged(stateChanged->state);
                break;
            }
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                std::cout << "Instance loss pending, shutting down..." << std::endl;
                m_isRunning = false;
                break;
            }
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
                std::cout << "Interaction profile changed" << std::endl;
                break;
            }
            default:
                break;
        }
        eventData = {XR_TYPE_EVENT_DATA_BUFFER};
    }
}

void XRSession::handleSessionStateChanged(XrSessionState newState) {
    m_sessionState = newState;

    std::cout << "Session state changed to: " << newState << std::endl;

    switch (newState) {
        case XR_SESSION_STATE_READY:
            if (!m_sessionBegun && m_session != XR_NULL_HANDLE) {
                XrSessionBeginInfo beginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
                beginInfo.primaryViewConfigurationType = m_viewConfigType;
                if (XR_SUCCEEDED(xrBeginSession(m_session, &beginInfo))) {
                    m_sessionBegun = true;
                    m_sessionReady = true;
                    std::cout << "Session begun" << std::endl;
                }
            }
            break;
        case XR_SESSION_STATE_STOPPING:
            if (m_sessionBegun) {
                xrEndSession(m_session);
                m_sessionBegun = false;
                m_sessionReady = false;
            }
            break;
        case XR_SESSION_STATE_EXITING:
        case XR_SESSION_STATE_LOSS_PENDING:
            m_isRunning = false;
            break;
        default:
            break;
    }
}

bool XRSession::beginFrame() {
    if (m_session == XR_NULL_HANDLE || !m_sessionReady) return false;

    m_frameState = {XR_TYPE_FRAME_STATE};
    XR_CHECK(xrWaitFrame(m_session, nullptr, &m_frameState), "Failed to wait for frame");

    m_predictedDisplayTime = m_frameState.predictedDisplayTime;
    m_shouldRender = m_frameState.shouldRender;

    // Calculate delta time
    auto now = std::chrono::high_resolution_clock::now();
    double currentTime = std::chrono::duration<double>(now.time_since_epoch()).count();
    m_frameDeltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;

    XR_CHECK(xrBeginFrame(m_session, nullptr), "Failed to begin frame");

    return true;
}

bool XRSession::endFrame(const std::vector<XrCompositionLayerBaseHeader*>& layers) {
    if (m_session == XR_NULL_HANDLE) return false;

    XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = m_predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = static_cast<uint32_t>(layers.size());
    endInfo.layers = layers.data();

    XR_CHECK(xrEndFrame(m_session, &endInfo), "Failed to end frame");

    return true;
}

bool XRSession::locateViews() {
    if (m_session == XR_NULL_HANDLE || m_stageSpace == XR_NULL_HANDLE) return false;

    XrViewState viewState = {XR_TYPE_VIEW_STATE};
    XrViewLocateInfo locateInfo = {XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = m_viewConfigType;
    locateInfo.displayTime = m_predictedDisplayTime;
    locateInfo.space = m_stageSpace;

    uint32_t viewCount = static_cast<uint32_t>(m_xrViews.size());
    XR_CHECK(xrLocateViews(m_session, &locateInfo, &viewState, viewCount, &viewCount, m_xrViews.data()),
             "Failed to locate views");

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
            xrView.pose.orientation.x,
            xrView.pose.orientation.y,
            xrView.pose.orientation.z,
            xrView.pose.orientation.w
        );

        view.fov[0] = xrView.fov.angleLeft;
        view.fov[1] = xrView.fov.angleRight;
        view.fov[2] = xrView.fov.angleUp;
        view.fov[3] = xrView.fov.angleDown;

        view.width = m_viewConfigViews[i].recommendedImageRectWidth;
        view.height = m_viewConfigViews[i].recommendedImageRectHeight;
    }

    return true;
}

bool XRSession::acquireSwapchainImage(int viewIndex, uint32_t& imageIndex) {
    if (viewIndex < 0 || viewIndex >= static_cast<int>(m_swapchains.size())) return false;

    XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    XR_CHECK(xrAcquireSwapchainImage(m_swapchains[viewIndex].swapchain, &acquireInfo, &imageIndex),
             "Failed to acquire swapchain image");

    return true;
}

bool XRSession::waitSwapchainImage(int viewIndex) {
    if (viewIndex < 0 || viewIndex >= static_cast<int>(m_swapchains.size())) return false;

    XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    XR_CHECK(xrWaitSwapchainImage(m_swapchains[viewIndex].swapchain, &waitInfo),
             "Failed to wait for swapchain image");

    return true;
}

bool XRSession::releaseSwapchainImage(int viewIndex) {
    if (viewIndex < 0 || viewIndex >= static_cast<int>(m_swapchains.size())) return false;

    XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    XR_CHECK(xrReleaseSwapchainImage(m_swapchains[viewIndex].swapchain, &releaseInfo),
             "Failed to release swapchain image");

    return true;
}

Transform XRSession::getHeadPose() const {
    Transform result;
    result.position = Vec3(0, 0, 0);
    result.orientation = Quat::identity();
    result.scale = Vec3(1, 1, 1);

    // If we have views, the head pose is roughly the midpoint between the eyes
    if (m_views.size() >= 2) {
        Vec3 leftPos = m_views[0].pose.position;
        Vec3 rightPos = m_views[1].pose.position;
        result.position = (leftPos + rightPos) * 0.5f;
        // Use left eye orientation as head orientation (close enough)
        result.orientation = m_views[0].pose.orientation;
    } else if (m_views.size() == 1) {
        result.position = m_views[0].pose.position;
        result.orientation = m_views[0].pose.orientation;
    }

    return result;
}

} // namespace lst
