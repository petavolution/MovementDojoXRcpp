/**
 * openxr_stub.h - Minimal OpenXR type stubs for headless builds
 *
 * This provides the type definitions needed by Engine.h when OpenXR
 * is not available (pure headless/CLI testing mode).
 */

#pragma once

#ifndef NO_OPENXR
// This header should only be included when NO_OPENXR is defined
#error "openxr_stub.h should only be included when NO_OPENXR is defined"
#endif

#include <cstdint>

// Avoid polluting namespace - use explicit struct tags

// Handle types
typedef struct XrInstance_T* XrInstance;
typedef struct XrSession_T* XrSession;
typedef struct XrSpace_T* XrSpace;
typedef struct XrSwapchain_T* XrSwapchain;
typedef struct XrAction_T* XrAction;
typedef struct XrActionSet_T* XrActionSet;

typedef uint64_t XrSystemId;
typedef uint64_t XrPath;
typedef int64_t XrTime;
typedef int64_t XrDuration;

// Null handles
#define XR_NULL_HANDLE nullptr
#define XR_NULL_SYSTEM_ID 0
#define XR_NULL_PATH 0

// Result type
typedef int32_t XrResult;
#define XR_SUCCESS 0
#define XR_FAILED(result) ((result) < 0)
#define XR_SUCCEEDED(result) ((result) >= 0)

// Duration constants
#define XR_INFINITE_DURATION 0x7fffffffffffffffLL
#define XR_FREQUENCY_UNSPECIFIED 0

// Session states
typedef enum XrSessionState {
    XR_SESSION_STATE_UNKNOWN = 0,
    XR_SESSION_STATE_IDLE = 1,
    XR_SESSION_STATE_READY = 2,
    XR_SESSION_STATE_SYNCHRONIZED = 3,
    XR_SESSION_STATE_VISIBLE = 4,
    XR_SESSION_STATE_FOCUSED = 5,
    XR_SESSION_STATE_STOPPING = 6,
    XR_SESSION_STATE_LOSS_PENDING = 7,
    XR_SESSION_STATE_EXITING = 8,
} XrSessionState;

// Common sub-structures (needed for assignment compatibility)
struct XrQuaternionf {
    float x, y, z, w;
};

struct XrVector3f {
    float x, y, z;
};

struct XrPosef {
    XrQuaternionf orientation;
    XrVector3f position;
};

struct XrFovf {
    float angleLeft;
    float angleRight;
    float angleUp;
    float angleDown;
};

// Structures (minimal stubs)
struct XrFrameState {
    int type;
    void* next;
    XrTime predictedDisplayTime;
    XrDuration predictedDisplayPeriod;
    int shouldRender;
};

struct XrView {
    int type;
    void* next;
    XrPosef pose;
    XrFovf fov;
};

struct XrSwapchainImageWaitInfo {
    int type;
    void* next;
    XrDuration timeout;
};

struct XrSwapchainImageReleaseInfo {
    int type;
    void* next;
};

struct XrSwapchainImageAcquireInfo {
    int type;
    void* next;
};

struct XrViewState {
    int type;
    void* next;
    uint64_t viewStateFlags;
};

struct XrViewLocateInfo {
    int type;
    void* next;
    int viewConfigurationType;
    XrTime displayTime;
    XrSpace space;
};

struct XrActiveActionSet {
    XrActionSet actionSet;
    XrPath subactionPath;
};

struct XrActionsSyncInfo {
    int type;
    void* next;
    uint32_t countActiveActionSets;
    const XrActiveActionSet* activeActionSets;
};

struct XrSpaceLocation {
    int type;
    void* next;
    uint64_t locationFlags;
    XrPosef pose;
};

struct XrActionStateFloat {
    int type;
    void* next;
    float currentState;
    int changedSinceLastSync;
    XrTime lastChangeTime;
    int isActive;
};

struct XrActionStateBoolean {
    int type;
    void* next;
    int currentState;
    int changedSinceLastSync;
    XrTime lastChangeTime;
    int isActive;
};

struct XrActionStateGetInfo {
    int type;
    void* next;
    XrAction action;
    XrPath subactionPath;
};

struct XrViewConfigurationView {
    int type;
    void* next;
    uint32_t recommendedImageRectWidth;
    uint32_t recommendedImageRectHeight;
    uint32_t maxImageRectWidth;
    uint32_t maxImageRectHeight;
    uint32_t recommendedSwapchainSampleCount;
    uint32_t maxSwapchainSampleCount;
};

struct XrRect2Di {
    struct { int32_t x, y; } offset;
    struct { int32_t width, height; } extent;
};

struct XrSwapchainSubImage {
    XrSwapchain swapchain;
    XrRect2Di imageRect;
    uint32_t imageArrayIndex;
};

struct XrCompositionLayerProjectionView {
    int type;
    void* next;
    XrPosef pose;
    XrFovf fov;
    XrSwapchainSubImage subImage;
};

// Action binding structures (for input configuration)
struct XrActionSuggestedBinding {
    XrAction action;
    XrPath binding;
};

struct XrInteractionProfileSuggestedBinding {
    int type;
    void* next;
    XrPath interactionProfile;
    uint32_t countSuggestedBindings;
    const XrActionSuggestedBinding* suggestedBindings;
};

struct XrSessionActionSetsAttachInfo {
    int type;
    void* next;
    uint32_t countActionSets;
    const XrActionSet* actionSets;
};

// Event structures
struct XrEventDataBuffer {
    int type;
    void* next;
    uint8_t varying[4000];
};

struct XrEventDataSessionStateChanged {
    int type;
    void* next;
    XrSession session;
    XrSessionState state;
    XrTime time;
};

// Session begin info
struct XrSessionBeginInfo {
    int type;
    void* next;
    int primaryViewConfigurationType;
};

// Haptic feedback structures
struct XrHapticActionInfo {
    int type;
    void* next;
    XrAction action;
    XrPath subactionPath;
};

struct XrHapticVibration {
    int type;
    void* next;
    XrDuration duration;
    float frequency;
    float amplitude;
};

// Haptic base header for polymorphic haptic types
struct XrHapticBaseHeader {
    int type;
    void* next;
};

// Extension properties
struct XrExtensionProperties {
    int type;
    void* next;
    char extensionName[128];
    uint32_t extensionVersion;
};

// System properties
struct XrSystemProperties {
    int type;
    void* next;
    XrSystemId systemId;
    uint32_t vendorId;
    char systemName[256];
    struct {
        uint32_t maxSwapchainImageHeight;
        uint32_t maxSwapchainImageWidth;
        uint32_t maxLayerCount;
    } graphicsProperties;
    struct {
        int orientationTracking;
        int positionTracking;
    } trackingProperties;
};

// Instance properties (runtime info)
struct XrInstanceProperties {
    int type;
    void* next;
    uint64_t runtimeVersion;
    char runtimeName[128];
};

// View configuration type enum
typedef enum XrViewConfigurationType {
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO = 1,
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO_ENUM = 2,  // Avoid redefinition
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO = 1000037000,
} XrViewConfigurationType;

// Reference space type enum
typedef enum XrReferenceSpaceType {
    XR_REFERENCE_SPACE_TYPE_VIEW_ENUM = 1,   // Avoid redefinition
    XR_REFERENCE_SPACE_TYPE_LOCAL_ENUM = 2,
    XR_REFERENCE_SPACE_TYPE_STAGE_ENUM = 3,
    XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT = 1000038000,
    XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO = 1000121000,
} XrReferenceSpaceType;

// Version macros
#define XR_VERSION_MAJOR(version) (uint32_t)(((uint64_t)(version) >> 48) & 0xFFFFULL)
#define XR_VERSION_MINOR(version) (uint32_t)(((uint64_t)(version) >> 32) & 0xFFFFULL)
#define XR_VERSION_PATCH(version) (uint32_t)((uint64_t)(version) & 0xFFFFFFFFULL)

// Instance and system creation
struct XrApplicationInfo {
    char applicationName[128];
    uint32_t applicationVersion;
    char engineName[128];
    uint32_t engineVersion;
    uint32_t apiVersion;
};

struct XrInstanceCreateInfo {
    int type;
    void* next;
    uint64_t createFlags;
    XrApplicationInfo applicationInfo;
    uint32_t enabledApiLayerCount;
    const char* const* enabledApiLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* enabledExtensionNames;
};

struct XrSystemGetInfo {
    int type;
    void* next;
    int formFactor;
};

// Session creation
struct XrSessionCreateInfo {
    int type;
    void* next;
    uint64_t createFlags;
    XrSystemId systemId;
};

// Space creation
struct XrReferenceSpaceCreateInfo {
    int type;
    void* next;
    int referenceSpaceType;
    XrPosef poseInReferenceSpace;
};

// Swapchain creation
struct XrSwapchainCreateInfo {
    int type;
    void* next;
    uint64_t createFlags;
    uint64_t usageFlags;
    int64_t format;
    uint32_t sampleCount;
    uint32_t width;
    uint32_t height;
    uint32_t faceCount;
    uint32_t arraySize;
    uint32_t mipCount;
};

// Action creation
struct XrActionSetCreateInfo {
    int type;
    void* next;
    char actionSetName[64];
    char localizedActionSetName[128];
    uint32_t priority;
};

struct XrActionCreateInfo {
    int type;
    void* next;
    char actionName[64];
    int actionType;
    uint32_t countSubactionPaths;
    const XrPath* subactionPaths;
    char localizedActionName[128];
};

struct XrActionSpaceCreateInfo {
    int type;
    void* next;
    XrAction action;
    XrPath subactionPath;
    XrPosef poseInActionSpace;
};

// Frame end info
struct XrCompositionLayerBaseHeader {
    int type;
    void* next;
    uint64_t layerFlags;
    XrSpace space;
};

struct XrFrameEndInfo {
    int type;
    void* next;
    XrTime displayTime;
    int environmentBlendMode;
    uint32_t layerCount;
    const XrCompositionLayerBaseHeader* const* layers;
};

// Composition layer projection
struct XrCompositionLayerProjection {
    int type;
    void* next;
    uint64_t layerFlags;
    XrSpace space;
    uint32_t viewCount;
    const XrCompositionLayerProjectionView* views;
};

// Stub function declarations (do nothing in headless mode)
inline XrResult xrCreateInstance(void*, XrInstance*) { return -1; }
inline XrResult xrDestroyInstance(XrInstance) { return 0; }
inline XrResult xrGetSystem(XrInstance, void*, XrSystemId*) { return -1; }
inline XrResult xrCreateSession(XrInstance, void*, XrSession*) { return -1; }
inline XrResult xrDestroySession(XrSession) { return 0; }
inline XrResult xrCreateReferenceSpace(XrSession, void*, XrSpace*) { return -1; }
inline XrResult xrDestroySpace(XrSpace) { return 0; }
inline XrResult xrCreateSwapchain(XrSession, void*, XrSwapchain*) { return -1; }
inline XrResult xrDestroySwapchain(XrSwapchain) { return 0; }
inline XrResult xrEnumerateSwapchainImages(XrSwapchain, uint32_t, uint32_t*, void*) { return -1; }
inline XrResult xrAcquireSwapchainImage(XrSwapchain, void*, uint32_t*) { return -1; }
inline XrResult xrWaitSwapchainImage(XrSwapchain, void*) { return -1; }
inline XrResult xrReleaseSwapchainImage(XrSwapchain, void*) { return -1; }
inline XrResult xrCreateActionSet(XrInstance, void*, XrActionSet*) { return -1; }
inline XrResult xrDestroyActionSet(XrActionSet) { return 0; }
inline XrResult xrCreateAction(XrActionSet, void*, XrAction*) { return -1; }
inline XrResult xrCreateActionSpace(XrSession, void*, XrSpace*) { return -1; }
inline XrResult xrStringToPath(XrInstance, const char*, XrPath*) { return -1; }
inline XrResult xrSuggestInteractionProfileBindings(XrInstance, void*) { return -1; }
inline XrResult xrAttachSessionActionSets(XrSession, void*) { return -1; }
inline XrResult xrSyncActions(XrSession, void*) { return -1; }
inline XrResult xrGetActionStateFloat(XrSession, void*, void*) { return -1; }
inline XrResult xrGetActionStateBoolean(XrSession, void*, void*) { return -1; }
inline XrResult xrLocateSpace(XrSpace, XrSpace, XrTime, void*) { return -1; }
inline XrResult xrLocateViews(XrSession, void*, void*, uint32_t, uint32_t*, XrView*) { return -1; }
inline XrResult xrWaitFrame(XrSession, void*, XrFrameState*) { return -1; }
inline XrResult xrBeginFrame(XrSession, void*) { return -1; }
inline XrResult xrEndFrame(XrSession, void*) { return -1; }
inline XrResult xrBeginSession(XrSession, void*) { return -1; }
inline XrResult xrEndSession(XrSession) { return 0; }
inline XrResult xrPollEvent(XrInstance, void*) { return -1; }
inline XrResult xrApplyHapticFeedback(XrSession, void*, void*) { return -1; }
inline XrResult xrEnumerateInstanceExtensionProperties(const char*, uint32_t, uint32_t*, void*) { return -1; }
inline XrResult xrGetSystemProperties(XrInstance, XrSystemId, void*) { return -1; }
inline XrResult xrEnumerateViewConfigurationViews(XrInstance, XrSystemId, int, uint32_t, uint32_t*, void*) { return -1; }
inline XrResult xrGetInstanceProperties(XrInstance, XrInstanceProperties*) { return -1; }
inline XrResult xrEnumerateViewConfigurations(XrInstance, XrSystemId, uint32_t, uint32_t*, XrViewConfigurationType*) { return -1; }
inline XrResult xrEnumerateReferenceSpaces(XrSession, uint32_t, uint32_t*, XrReferenceSpaceType*) { return -1; }

// Type codes (minimal set)
#define XR_TYPE_FRAME_STATE 0
#define XR_TYPE_VIEW 0
#define XR_TYPE_VIEW_CONFIGURATION_VIEW 0
#define XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW 0
#define XR_TYPE_COMPOSITION_LAYER_PROJECTION 0
#define XR_TYPE_INSTANCE_CREATE_INFO 0
#define XR_TYPE_SESSION_CREATE_INFO 0
#define XR_TYPE_REFERENCE_SPACE_CREATE_INFO 0
#define XR_TYPE_SWAPCHAIN_CREATE_INFO 0
#define XR_TYPE_ACTION_SET_CREATE_INFO 0
#define XR_TYPE_ACTION_CREATE_INFO 0
#define XR_TYPE_ACTION_SPACE_CREATE_INFO 0
#define XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING 0
#define XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO 0
#define XR_TYPE_ACTIONS_SYNC_INFO 0
#define XR_TYPE_ACTION_STATE_FLOAT 0
#define XR_TYPE_ACTION_STATE_BOOLEAN 0
#define XR_TYPE_SPACE_LOCATION 0
#define XR_TYPE_VIEW_LOCATE_INFO 0
#define XR_TYPE_VIEW_STATE 0
#define XR_TYPE_FRAME_END_INFO 0
#define XR_TYPE_SESSION_BEGIN_INFO 0
#define XR_TYPE_EVENT_DATA_BUFFER 0
#define XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED 1
#define XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING 2
#define XR_TYPE_HAPTIC_VIBRATION 0
#define XR_TYPE_HAPTIC_ACTION_INFO 0
#define XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO 0
#define XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO 0
#define XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO 0
#define XR_TYPE_EXTENSION_PROPERTIES 0
#define XR_TYPE_SYSTEM_GET_INFO 0
#define XR_TYPE_SYSTEM_PROPERTIES 0
#define XR_TYPE_ACTION_STATE_GET_INFO 0
#define XR_TYPE_INSTANCE_PROPERTIES 0

// Constants
#define XR_MAX_APPLICATION_NAME_SIZE 128
#define XR_MAX_ENGINE_NAME_SIZE 128
#define XR_CURRENT_API_VERSION 0
#define XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY 1
#define XR_REFERENCE_SPACE_TYPE_STAGE 3
#define XR_REFERENCE_SPACE_TYPE_LOCAL 1
#define XR_REFERENCE_SPACE_TYPE_VIEW 2
#define XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO 2
#define XR_ACTION_TYPE_POSE_INPUT 0
#define XR_ACTION_TYPE_FLOAT_INPUT 1
#define XR_ACTION_TYPE_VIBRATION_OUTPUT 2
#define XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT 1
#define XR_SWAPCHAIN_USAGE_SAMPLED_BIT 2
#define XR_SPACE_LOCATION_POSITION_VALID_BIT 1
#define XR_ENVIRONMENT_BLEND_MODE_OPAQUE 1
