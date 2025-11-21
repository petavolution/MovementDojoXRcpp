#pragma once

#include "Types.h"
#include <queue>
#include <vector>

namespace lst {

class Input;

/**
 * HapticManager - Manages haptic feedback patterns and events
 *
 * Handles:
 * - Queuing haptic events
 * - Pattern playback (pulses, ramps, etc.)
 * - Per-hand haptic state
 * - Integration with OpenXR haptics API via Input
 */
class HapticManager {
public:
    HapticManager();
    ~HapticManager();

    // Initialize with input system for haptic output
    bool initialize(Input* input);
    void shutdown();

    // Simple haptic triggers
    void pulse(Hand hand, float intensity = 0.5f, float duration = 0.1f);
    void buzz(Hand hand, float intensity = 0.3f, float duration = 0.05f);
    void ramp(Hand hand, float startIntensity, float endIntensity, float duration);

    // Predefined patterns
    void playSuccessPattern(Hand hand);
    void playFailurePattern(Hand hand);
    void playCollisionPattern(Hand hand, float impactStrength);
    void playSaberActivatePattern(Hand hand);
    void playSaberDeactivatePattern(Hand hand);
    void playGuidanceNudge(Hand hand, float strength);

    // Custom pattern playback
    struct HapticStep {
        float intensity;
        float duration;
        float pauseAfter;
    };
    void playPattern(Hand hand, const std::vector<HapticStep>& pattern);

    // Update (processes queued haptics)
    void update(double deltaTime);

    // Stop all haptics
    void stopAll();

private:
    struct QueuedHaptic {
        Hand hand;
        float intensity;
        float duration;
        float delay;
        float timeRemaining;
    };

    Input* m_input = nullptr;

    // Haptic queues (per hand)
    std::queue<QueuedHaptic> m_leftQueue;
    std::queue<QueuedHaptic> m_rightQueue;

    // Current haptic state
    float m_leftRemainingTime = 0.0f;
    float m_rightRemainingTime = 0.0f;

    // Pattern definitions
    std::vector<HapticStep> m_successPattern;
    std::vector<HapticStep> m_failurePattern;
    std::vector<HapticStep> m_activatePattern;
    std::vector<HapticStep> m_deactivatePattern;
};

} // namespace lst
