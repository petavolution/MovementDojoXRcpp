#pragma once

#include "Types.h"
#include <openxr/openxr.h>
#include <vector>
#include <string>
#include <functional>

namespace lst {

/**
 * XRSession - Manages the OpenXR session lifecycle and frame loop
 *
 * Handles:
 * - XrInstance creation and destruction
 * - XrSession management
 * - XrSpace creation (stage, local, view)
 * - Swapchain management
 * - Frame loop (xrWaitFrame, xrBeginFrame, xrEndFrame)
 * - View location queries
 */
class XRSession {
public:
    XRSession();
    ~XRSession();

    // Prevent copying
    XRSession(const XRSession&) = delete;
    XRSession& operator=(const XRSession&) = delete;

    // Initialization
    bool initialize(const AppConfig& config);
    void shutdown();

    // Session state
    bool isRunning() const { return m_isRunning; }
    bool isSessionReady() const { return m_sessionReady; }
    XrSession getSession() const { return m_session; }
    XrInstance getInstance() const { return m_instance; }

    // Frame loop
    bool beginFrame();
    bool endFrame(const std::vector<XrCompositionLayerBaseHeader*>& layers);
    bool shouldRender() const { return m_shouldRender; }

    // Views
    const std::vector<XRView>& getViews() const { return m_views; }
    bool locateViews();

    // Spaces
    XrSpace getStageSpace() const { return m_stageSpace; }
    XrSpace getLocalSpace() const { return m_localSpace; }
    XrSpace getViewSpace() const { return m_viewSpace; }

    // Swapchain
    struct SwapchainInfo {
        XrSwapchain swapchain;
        int32_t width;
        int32_t height;
        std::vector<uint32_t> images; // Vulkan image handles
    };
    const std::vector<SwapchainInfo>& getSwapchains() const { return m_swapchains; }
    bool acquireSwapchainImage(int viewIndex, uint32_t& imageIndex);
    bool releaseSwapchainImage(int viewIndex);
    bool waitSwapchainImage(int viewIndex);

    // Frame timing
    XrTime getPredictedDisplayTime() const { return m_predictedDisplayTime; }
    double getFrameDeltaTime() const { return m_frameDeltaTime; }

    // System info
    std::string getSystemName() const { return m_systemName; }
    uint32_t getMaxSwapchainWidth() const { return m_maxSwapchainWidth; }
    uint32_t getMaxSwapchainHeight() const { return m_maxSwapchainHeight; }

    // Event polling
    void pollEvents();

    // Head pose (convenience method)
    Transform getHeadPose() const;

private:
    bool createInstance(const AppConfig& config);
    bool getSystem();
    bool createSession();
    bool createSpaces();
    bool createSwapchains();
    bool enumerateExtensions();

    void handleSessionStateChanged(XrSessionState newState);

    // OpenXR handles
    XrInstance m_instance = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
    XrSession m_session = XR_NULL_HANDLE;
    XrSpace m_stageSpace = XR_NULL_HANDLE;
    XrSpace m_localSpace = XR_NULL_HANDLE;
    XrSpace m_viewSpace = XR_NULL_HANDLE;

    // Swapchains (one per view/eye)
    std::vector<SwapchainInfo> m_swapchains;

    // View configuration
    XrViewConfigurationType m_viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    std::vector<XrViewConfigurationView> m_viewConfigViews;
    std::vector<XrView> m_xrViews;
    std::vector<XRView> m_views;

    // Frame state
    XrFrameState m_frameState = {};
    XrTime m_predictedDisplayTime = 0;
    double m_frameDeltaTime = 0;
    double m_lastFrameTime = 0;
    bool m_shouldRender = false;

    // Session state
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;
    bool m_isRunning = false;
    bool m_sessionReady = false;
    bool m_sessionBegun = false;

    // System info
    std::string m_systemName;
    uint32_t m_maxSwapchainWidth = 0;
    uint32_t m_maxSwapchainHeight = 0;

    // Extensions
    std::vector<std::string> m_enabledExtensions;

    // Graphics binding (Vulkan or OpenGL)
    void* m_graphicsBinding = nullptr;
};

} // namespace lst
