#pragma once

#include "Types.h"
#include <openxr/openxr.h>
#include <array>
#include <vector>

namespace lst {

class XRSession;

/**
 * Input - Manages OpenXR input actions and controller tracking
 *
 * Handles:
 * - Action set and action creation
 * - Controller pose tracking
 * - Button/trigger/grip state
 * - Interaction profile binding
 * - Haptic feedback output
 */
class Input {
public:
    Input();
    ~Input();

    // Initialization
    bool initialize(XRSession* session);
    void shutdown();

    // Action management
    bool createActions();
    bool suggestBindings();
    bool attachActionSets();

    // Per-frame update
    bool syncActions();

    // Controller state access
    const ControllerState& getLeftController() const { return m_controllers[0]; }
    const ControllerState& getRightController() const { return m_controllers[1]; }
    ControllerState& getLeftController() { return m_controllers[0]; }
    ControllerState& getRightController() { return m_controllers[1]; }

    // Haptic feedback
    bool triggerHaptic(Hand hand, float intensity, float duration, float frequency = 0.0f);

    // Action handles (for advanced use)
    XrAction getPoseAction() const { return m_poseAction; }
    XrAction getTriggerAction() const { return m_triggerAction; }
    XrAction getGripAction() const { return m_gripAction; }

    // Controller spaces
    XrSpace getHandSpace(Hand hand) const;

private:
    bool createActionSpace(Hand hand);
    bool locateController(Hand hand);

    XRSession* m_session = nullptr;

    // Action set
    XrActionSet m_actionSet = XR_NULL_HANDLE;

    // Actions
    XrAction m_poseAction = XR_NULL_HANDLE;
    XrAction m_triggerAction = XR_NULL_HANDLE;
    XrAction m_triggerClickAction = XR_NULL_HANDLE;
    XrAction m_gripAction = XR_NULL_HANDLE;
    XrAction m_gripClickAction = XR_NULL_HANDLE;
    XrAction m_primaryButtonAction = XR_NULL_HANDLE;
    XrAction m_secondaryButtonAction = XR_NULL_HANDLE;
    XrAction m_hapticAction = XR_NULL_HANDLE;

    // Controller spaces
    std::array<XrSpace, 2> m_handSpaces = {XR_NULL_HANDLE, XR_NULL_HANDLE};

    // Subaction paths (left/right hand)
    std::array<XrPath, 2> m_handPaths = {XR_NULL_PATH, XR_NULL_PATH};

    // Controller state
    std::array<ControllerState, 2> m_controllers;

    // Velocity tracking
    std::array<Transform, 2> m_previousPoses;
    double m_previousTime = 0;
};

} // namespace lst
