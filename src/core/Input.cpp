#include "Input.h"
#include "XRSession.h"
#include <iostream>
#include <cstring>

namespace lst {

#define XR_CHECK(result, msg) \
    do { \
        XrResult _result = (result); \
        if (XR_FAILED(_result)) { \
            std::cerr << "OpenXR Input Error: " << msg << " (result=" << _result << ")" << std::endl; \
            return false; \
        } \
    } while(0)

Input::Input() {
    m_controllers[0].hand = Hand::Left;
    m_controllers[1].hand = Hand::Right;
}

Input::~Input() {
    shutdown();
}

bool Input::initialize(XRSession* session) {
    if (!session) return false;
    m_session = session;

    XrInstance instance = session->getInstance();
    if (instance == XR_NULL_HANDLE) return false;

    // Get hand paths
    XR_CHECK(xrStringToPath(instance, "/user/hand/left", &m_handPaths[0]),
             "Failed to get left hand path");
    XR_CHECK(xrStringToPath(instance, "/user/hand/right", &m_handPaths[1]),
             "Failed to get right hand path");

    if (!createActions()) {
        std::cerr << "Failed to create actions" << std::endl;
        return false;
    }

    if (!suggestBindings()) {
        std::cerr << "Failed to suggest bindings" << std::endl;
        return false;
    }

    std::cout << "Input system initialized" << std::endl;
    return true;
}

void Input::shutdown() {
    // Destroy action spaces
    for (auto& space : m_handSpaces) {
        if (space != XR_NULL_HANDLE) {
            xrDestroySpace(space);
            space = XR_NULL_HANDLE;
        }
    }

    // Destroy actions (destroyed with action set)

    // Destroy action set
    if (m_actionSet != XR_NULL_HANDLE) {
        xrDestroyActionSet(m_actionSet);
        m_actionSet = XR_NULL_HANDLE;
    }

    m_session = nullptr;
}

bool Input::createActions() {
    XrInstance instance = m_session->getInstance();

    // Create action set
    XrActionSetCreateInfo actionSetInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy(actionSetInfo.actionSetName, "gameplay");
    strcpy(actionSetInfo.localizedActionSetName, "Gameplay Actions");
    actionSetInfo.priority = 0;

    XR_CHECK(xrCreateActionSet(instance, &actionSetInfo, &m_actionSet),
             "Failed to create action set");

    // Pose action (controller position/orientation)
    XrActionCreateInfo poseActionInfo = {XR_TYPE_ACTION_CREATE_INFO};
    poseActionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strcpy(poseActionInfo.actionName, "hand_pose");
    strcpy(poseActionInfo.localizedActionName, "Hand Pose");
    poseActionInfo.countSubactionPaths = 2;
    poseActionInfo.subactionPaths = m_handPaths.data();

    XR_CHECK(xrCreateAction(m_actionSet, &poseActionInfo, &m_poseAction),
             "Failed to create pose action");

    // Trigger value action (float 0-1)
    XrActionCreateInfo triggerActionInfo = {XR_TYPE_ACTION_CREATE_INFO};
    triggerActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
    strcpy(triggerActionInfo.actionName, "trigger");
    strcpy(triggerActionInfo.localizedActionName, "Trigger");
    triggerActionInfo.countSubactionPaths = 2;
    triggerActionInfo.subactionPaths = m_handPaths.data();

    XR_CHECK(xrCreateAction(m_actionSet, &triggerActionInfo, &m_triggerAction),
             "Failed to create trigger action");

    // Trigger click action (boolean)
    XrActionCreateInfo triggerClickInfo = {XR_TYPE_ACTION_CREATE_INFO};
    triggerClickInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strcpy(triggerClickInfo.actionName, "trigger_click");
    strcpy(triggerClickInfo.localizedActionName, "Trigger Click");
    triggerClickInfo.countSubactionPaths = 2;
    triggerClickInfo.subactionPaths = m_handPaths.data();

    XR_CHECK(xrCreateAction(m_actionSet, &triggerClickInfo, &m_triggerClickAction),
             "Failed to create trigger click action");

    // Grip value action (float 0-1)
    XrActionCreateInfo gripActionInfo = {XR_TYPE_ACTION_CREATE_INFO};
    gripActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
    strcpy(gripActionInfo.actionName, "grip");
    strcpy(gripActionInfo.localizedActionName, "Grip");
    gripActionInfo.countSubactionPaths = 2;
    gripActionInfo.subactionPaths = m_handPaths.data();

    XR_CHECK(xrCreateAction(m_actionSet, &gripActionInfo, &m_gripAction),
             "Failed to create grip action");

    // Grip click action (boolean)
    XrActionCreateInfo gripClickInfo = {XR_TYPE_ACTION_CREATE_INFO};
    gripClickInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strcpy(gripClickInfo.actionName, "grip_click");
    strcpy(gripClickInfo.localizedActionName, "Grip Click");
    gripClickInfo.countSubactionPaths = 2;
    gripClickInfo.subactionPaths = m_handPaths.data();

    XR_CHECK(xrCreateAction(m_actionSet, &gripClickInfo, &m_gripClickAction),
             "Failed to create grip click action");

    // Primary button (A/X)
    XrActionCreateInfo primaryButtonInfo = {XR_TYPE_ACTION_CREATE_INFO};
    primaryButtonInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strcpy(primaryButtonInfo.actionName, "primary_button");
    strcpy(primaryButtonInfo.localizedActionName, "Primary Button");
    primaryButtonInfo.countSubactionPaths = 2;
    primaryButtonInfo.subactionPaths = m_handPaths.data();

    XR_CHECK(xrCreateAction(m_actionSet, &primaryButtonInfo, &m_primaryButtonAction),
             "Failed to create primary button action");

    // Secondary button (B/Y)
    XrActionCreateInfo secondaryButtonInfo = {XR_TYPE_ACTION_CREATE_INFO};
    secondaryButtonInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strcpy(secondaryButtonInfo.actionName, "secondary_button");
    strcpy(secondaryButtonInfo.localizedActionName, "Secondary Button");
    secondaryButtonInfo.countSubactionPaths = 2;
    secondaryButtonInfo.subactionPaths = m_handPaths.data();

    XR_CHECK(xrCreateAction(m_actionSet, &secondaryButtonInfo, &m_secondaryButtonAction),
             "Failed to create secondary button action");

    // Haptic output action
    XrActionCreateInfo hapticActionInfo = {XR_TYPE_ACTION_CREATE_INFO};
    hapticActionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
    strcpy(hapticActionInfo.actionName, "haptic");
    strcpy(hapticActionInfo.localizedActionName, "Haptic Feedback");
    hapticActionInfo.countSubactionPaths = 2;
    hapticActionInfo.subactionPaths = m_handPaths.data();

    XR_CHECK(xrCreateAction(m_actionSet, &hapticActionInfo, &m_hapticAction),
             "Failed to create haptic action");

    std::cout << "Created all input actions" << std::endl;
    return true;
}

bool Input::suggestBindings() {
    XrInstance instance = m_session->getInstance();

    // Helper to create path
    auto getPath = [instance](const char* pathStr) -> XrPath {
        XrPath path;
        xrStringToPath(instance, pathStr, &path);
        return path;
    };

    // Oculus Touch controller bindings
    {
        std::vector<XrActionSuggestedBinding> bindings;

        // Pose bindings
        bindings.push_back({m_poseAction, getPath("/user/hand/left/input/grip/pose")});
        bindings.push_back({m_poseAction, getPath("/user/hand/right/input/grip/pose")});

        // Trigger bindings
        bindings.push_back({m_triggerAction, getPath("/user/hand/left/input/trigger/value")});
        bindings.push_back({m_triggerAction, getPath("/user/hand/right/input/trigger/value")});
        bindings.push_back({m_triggerClickAction, getPath("/user/hand/left/input/trigger/value")});
        bindings.push_back({m_triggerClickAction, getPath("/user/hand/right/input/trigger/value")});

        // Grip bindings
        bindings.push_back({m_gripAction, getPath("/user/hand/left/input/squeeze/value")});
        bindings.push_back({m_gripAction, getPath("/user/hand/right/input/squeeze/value")});
        bindings.push_back({m_gripClickAction, getPath("/user/hand/left/input/squeeze/value")});
        bindings.push_back({m_gripClickAction, getPath("/user/hand/right/input/squeeze/value")});

        // Button bindings
        bindings.push_back({m_primaryButtonAction, getPath("/user/hand/left/input/x/click")});
        bindings.push_back({m_primaryButtonAction, getPath("/user/hand/right/input/a/click")});
        bindings.push_back({m_secondaryButtonAction, getPath("/user/hand/left/input/y/click")});
        bindings.push_back({m_secondaryButtonAction, getPath("/user/hand/right/input/b/click")});

        // Haptic bindings
        bindings.push_back({m_hapticAction, getPath("/user/hand/left/output/haptic")});
        bindings.push_back({m_hapticAction, getPath("/user/hand/right/output/haptic")});

        XrInteractionProfileSuggestedBinding suggestedBindings = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestedBindings.interactionProfile = getPath("/interaction_profiles/oculus/touch_controller");
        suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        suggestedBindings.suggestedBindings = bindings.data();

        XrResult result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
        if (XR_SUCCEEDED(result)) {
            std::cout << "Suggested Oculus Touch bindings" << std::endl;
        }
    }

    // Valve Index controller bindings
    {
        std::vector<XrActionSuggestedBinding> bindings;

        bindings.push_back({m_poseAction, getPath("/user/hand/left/input/grip/pose")});
        bindings.push_back({m_poseAction, getPath("/user/hand/right/input/grip/pose")});

        bindings.push_back({m_triggerAction, getPath("/user/hand/left/input/trigger/value")});
        bindings.push_back({m_triggerAction, getPath("/user/hand/right/input/trigger/value")});
        bindings.push_back({m_triggerClickAction, getPath("/user/hand/left/input/trigger/click")});
        bindings.push_back({m_triggerClickAction, getPath("/user/hand/right/input/trigger/click")});

        bindings.push_back({m_gripAction, getPath("/user/hand/left/input/squeeze/value")});
        bindings.push_back({m_gripAction, getPath("/user/hand/right/input/squeeze/value")});
        bindings.push_back({m_gripClickAction, getPath("/user/hand/left/input/squeeze/force")});
        bindings.push_back({m_gripClickAction, getPath("/user/hand/right/input/squeeze/force")});

        bindings.push_back({m_primaryButtonAction, getPath("/user/hand/left/input/a/click")});
        bindings.push_back({m_primaryButtonAction, getPath("/user/hand/right/input/a/click")});
        bindings.push_back({m_secondaryButtonAction, getPath("/user/hand/left/input/b/click")});
        bindings.push_back({m_secondaryButtonAction, getPath("/user/hand/right/input/b/click")});

        bindings.push_back({m_hapticAction, getPath("/user/hand/left/output/haptic")});
        bindings.push_back({m_hapticAction, getPath("/user/hand/right/output/haptic")});

        XrInteractionProfileSuggestedBinding suggestedBindings = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestedBindings.interactionProfile = getPath("/interaction_profiles/valve/index_controller");
        suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        suggestedBindings.suggestedBindings = bindings.data();

        XrResult result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
        if (XR_SUCCEEDED(result)) {
            std::cout << "Suggested Valve Index bindings" << std::endl;
        }
    }

    // Simple controller bindings (fallback)
    {
        std::vector<XrActionSuggestedBinding> bindings;

        bindings.push_back({m_poseAction, getPath("/user/hand/left/input/grip/pose")});
        bindings.push_back({m_poseAction, getPath("/user/hand/right/input/grip/pose")});

        bindings.push_back({m_primaryButtonAction, getPath("/user/hand/left/input/select/click")});
        bindings.push_back({m_primaryButtonAction, getPath("/user/hand/right/input/select/click")});

        bindings.push_back({m_hapticAction, getPath("/user/hand/left/output/haptic")});
        bindings.push_back({m_hapticAction, getPath("/user/hand/right/output/haptic")});

        XrInteractionProfileSuggestedBinding suggestedBindings = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestedBindings.interactionProfile = getPath("/interaction_profiles/khr/simple_controller");
        suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        suggestedBindings.suggestedBindings = bindings.data();

        XrResult result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
        if (XR_SUCCEEDED(result)) {
            std::cout << "Suggested simple controller bindings" << std::endl;
        }
    }

    return true;
}

bool Input::attachActionSets() {
    XrSession session = m_session->getSession();
    if (session == XR_NULL_HANDLE) return false;

    XrSessionActionSetsAttachInfo attachInfo = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;

    XR_CHECK(xrAttachSessionActionSets(session, &attachInfo), "Failed to attach action sets");

    // Create action spaces for controllers
    for (int i = 0; i < 2; i++) {
        if (!createActionSpace(static_cast<Hand>(i))) {
            return false;
        }
    }

    std::cout << "Action sets attached" << std::endl;
    return true;
}

bool Input::createActionSpace(Hand hand) {
    XrSession session = m_session->getSession();
    int idx = static_cast<int>(hand);

    XrActionSpaceCreateInfo spaceInfo = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
    spaceInfo.action = m_poseAction;
    spaceInfo.subactionPath = m_handPaths[idx];
    spaceInfo.poseInActionSpace.orientation = {0, 0, 0, 1};
    spaceInfo.poseInActionSpace.position = {0, 0, 0};

    XR_CHECK(xrCreateActionSpace(session, &spaceInfo, &m_handSpaces[idx]),
             "Failed to create hand space");

    return true;
}

bool Input::syncActions() {
    XrSession session = m_session->getSession();
    if (session == XR_NULL_HANDLE || !m_session->isSessionReady()) return false;

    XrActiveActionSet activeActionSet = {};
    activeActionSet.actionSet = m_actionSet;
    activeActionSet.subactionPath = XR_NULL_PATH;

    XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeActionSet;

    XrResult result = xrSyncActions(session, &syncInfo);
    if (XR_FAILED(result)) {
        return false;
    }

    // Update controller states
    for (int i = 0; i < 2; i++) {
        locateController(static_cast<Hand>(i));
    }

    return true;
}

bool Input::locateController(Hand hand) {
    XrSession session = m_session->getSession();
    int idx = static_cast<int>(hand);
    auto& controller = m_controllers[idx];

    // Store previous pose for velocity calculation
    Transform previousPose = controller.pose;

    // Get pose state
    XrActionStateGetInfo poseGetInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    poseGetInfo.action = m_poseAction;
    poseGetInfo.subactionPath = m_handPaths[idx];

    XrActionStatePose poseState = {XR_TYPE_ACTION_STATE_POSE};
    xrGetActionStatePose(session, &poseGetInfo, &poseState);
    controller.isTracked = poseState.isActive;

    if (controller.isTracked && m_handSpaces[idx] != XR_NULL_HANDLE) {
        XrSpaceLocation location = {XR_TYPE_SPACE_LOCATION};
        XrSpaceVelocity velocity = {XR_TYPE_SPACE_VELOCITY};
        location.next = &velocity;

        xrLocateSpace(m_handSpaces[idx], m_session->getStageSpace(),
                      m_session->getPredictedDisplayTime(), &location);

        if (location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
            controller.pose.position = Vec3(
                location.pose.position.x,
                location.pose.position.y,
                location.pose.position.z
            );
        }

        if (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
            controller.pose.orientation = Quat(
                location.pose.orientation.x,
                location.pose.orientation.y,
                location.pose.orientation.z,
                location.pose.orientation.w
            );
        }

        if (velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) {
            controller.velocity = Vec3(
                velocity.linearVelocity.x,
                velocity.linearVelocity.y,
                velocity.linearVelocity.z
            );
        }

        if (velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT) {
            controller.angularVelocity = Vec3(
                velocity.angularVelocity.x,
                velocity.angularVelocity.y,
                velocity.angularVelocity.z
            );
        }
    }

    // Get trigger state
    XrActionStateGetInfo triggerGetInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    triggerGetInfo.action = m_triggerAction;
    triggerGetInfo.subactionPath = m_handPaths[idx];

    XrActionStateFloat triggerState = {XR_TYPE_ACTION_STATE_FLOAT};
    xrGetActionStateFloat(session, &triggerGetInfo, &triggerState);
    controller.triggerValue = triggerState.currentState;
    controller.triggerPressed = controller.triggerValue > 0.5f;

    // Get grip state
    XrActionStateGetInfo gripGetInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    gripGetInfo.action = m_gripAction;
    gripGetInfo.subactionPath = m_handPaths[idx];

    XrActionStateFloat gripState = {XR_TYPE_ACTION_STATE_FLOAT};
    xrGetActionStateFloat(session, &gripGetInfo, &gripState);
    controller.gripValue = gripState.currentState;
    controller.gripPressed = controller.gripValue > 0.5f;

    // Get primary button state
    XrActionStateGetInfo primaryGetInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    primaryGetInfo.action = m_primaryButtonAction;
    primaryGetInfo.subactionPath = m_handPaths[idx];

    XrActionStateBoolean primaryState = {XR_TYPE_ACTION_STATE_BOOLEAN};
    xrGetActionStateBoolean(session, &primaryGetInfo, &primaryState);
    controller.primaryButtonPressed = primaryState.currentState;

    // Get secondary button state
    XrActionStateGetInfo secondaryGetInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    secondaryGetInfo.action = m_secondaryButtonAction;
    secondaryGetInfo.subactionPath = m_handPaths[idx];

    XrActionStateBoolean secondaryState = {XR_TYPE_ACTION_STATE_BOOLEAN};
    xrGetActionStateBoolean(session, &secondaryGetInfo, &secondaryState);
    controller.secondaryButtonPressed = secondaryState.currentState;

    return true;
}

bool Input::triggerHaptic(Hand hand, float intensity, float duration, float frequency) {
    XrSession session = m_session->getSession();
    if (session == XR_NULL_HANDLE) return false;

    int idx = static_cast<int>(hand);

    XrHapticVibration vibration = {XR_TYPE_HAPTIC_VIBRATION};
    vibration.amplitude = intensity;
    vibration.duration = static_cast<XrDuration>(duration * 1e9); // Convert to nanoseconds
    vibration.frequency = frequency > 0 ? frequency : XR_FREQUENCY_UNSPECIFIED;

    XrHapticActionInfo hapticInfo = {XR_TYPE_HAPTIC_ACTION_INFO};
    hapticInfo.action = m_hapticAction;
    hapticInfo.subactionPath = m_handPaths[idx];

    XrResult result = xrApplyHapticFeedback(session, &hapticInfo,
                                            reinterpret_cast<XrHapticBaseHeader*>(&vibration));
    return XR_SUCCEEDED(result);
}

XrSpace Input::getHandSpace(Hand hand) const {
    return m_handSpaces[static_cast<int>(hand)];
}

} // namespace lst
