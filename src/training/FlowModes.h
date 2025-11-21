#pragma once

#include "Types.h"
#include "TrainingModule.h"
#include "analytics/MovementAnalytics.h"
#include <vector>
#include <string>
#include <memory>
#include <random>

namespace lst {

/**
 * Flow/Meditation Training Modes
 *
 * These modes focus on mindful movement exploration rather than combat.
 * Inspired by yoga, qi gong, and meditation practices.
 *
 * Core concept: Guide users to explore their FULL range of motion,
 * especially movements they NEVER make in daily life.
 */

// =============================================================================
// FreeExplorationMode - Visualize and encourage full movement space exploration
// =============================================================================

class FreeExplorationMode : public TrainingModule {
public:
    FreeExplorationMode(MovementAnalytics* analytics);

    void start() override;
    void update(double deltaTime, const ControllerState& left, const ControllerState& right) override;

    // Get visualization data
    struct VisualizationData {
        std::vector<Vec3> trails;           // Recent movement trails
        std::vector<Vec3> explorationTargets; // Suggested areas to explore
        float currentCoverage;              // 0-100%
        bool isInUncommonPosition;
        std::string currentHint;
    };
    VisualizationData getVisualization() const;

    // Configuration
    void setShowTrails(bool show) { m_showTrails = show; }
    void setShowTargets(bool show) { m_showTargets = show; }
    void setTargetUncommonAreas(bool target) { m_targetUncommon = target; }

private:
    MovementAnalytics* m_analytics;
    bool m_showTrails = true;
    bool m_showTargets = true;
    bool m_targetUncommon = true;

    float m_lastCoverage = 0;
    int m_uncommonAreasDiscovered = 0;
};

// =============================================================================
// GuidedStretchMode - Follow guided stretch/yoga-like sequences
// =============================================================================

struct StretchPose {
    std::string name;
    std::string instruction;   // e.g., "Reach up with both hands"

    // Target positions in head-relative space
    Vec3 leftHandTarget;
    Vec3 rightHandTarget;

    float holdDuration;        // Seconds to hold
    float transitionDuration;  // Seconds to transition from previous
    float toleranceMeters;     // How close to target counts as "correct"
};

class GuidedStretchMode : public TrainingModule {
public:
    GuidedStretchMode(MovementAnalytics* analytics);

    void start() override;
    void update(double deltaTime, const ControllerState& left, const ControllerState& right) override;

    // Load a stretch sequence
    void loadSequence(const std::string& name);

    // Pre-defined sequences
    static std::vector<StretchPose> getUpperBodySequence();
    static std::vector<StretchPose> getFullRangeSequence();
    static std::vector<StretchPose> getQiGongSequence();

    // Current state
    int getCurrentPoseIndex() const { return m_currentPoseIndex; }
    const StretchPose& getCurrentPose() const { return m_sequence[m_currentPoseIndex]; }
    float getPoseProgress() const { return m_poseProgress; }
    bool isHoldingCorrectly() const { return m_isHolding; }

private:
    MovementAnalytics* m_analytics;
    std::vector<StretchPose> m_sequence;
    int m_currentPoseIndex = 0;
    float m_poseProgress = 0;  // 0-1 for current pose completion
    float m_holdTimer = 0;
    bool m_isHolding = false;
    bool m_isTransitioning = false;
};

// =============================================================================
// BreathingSyncMode - Synchronize movement with breathing rhythm
// =============================================================================

class BreathingSyncMode : public TrainingModule {
public:
    BreathingSyncMode(MovementAnalytics* analytics);

    void start() override;
    void update(double deltaTime, const ControllerState& left, const ControllerState& right) override;

    // Breathing parameters
    void setBreathCycle(float inhaleSeconds, float exhaleSeconds, float holdSeconds = 0);

    // Current breathing state
    enum class BreathPhase { Inhale, Hold, Exhale, Pause };
    BreathPhase getCurrentPhase() const { return m_currentPhase; }
    float getPhaseProgress() const { return m_phaseProgress; }

    // Movement guidance
    Vec3 getSuggestedLeftHandPosition() const;
    Vec3 getSuggestedRightHandPosition() const;
    float getSuggestedMovementSpeed() const;

private:
    MovementAnalytics* m_analytics;

    float m_inhaleTime = 4.0f;
    float m_exhaleTime = 4.0f;
    float m_holdTime = 2.0f;
    float m_pauseTime = 1.0f;

    BreathPhase m_currentPhase = BreathPhase::Inhale;
    float m_phaseTimer = 0;
    float m_phaseProgress = 0;
    int m_cycleCount = 0;
};

// =============================================================================
// MirrorMode - Follow a ghost/mirror guide through movements
// =============================================================================

class MirrorMode : public TrainingModule {
public:
    MirrorMode(MovementAnalytics* analytics);

    void start() override;
    void update(double deltaTime, const ControllerState& left, const ControllerState& right) override;

    // Load a movement sequence (from recording or procedural)
    void loadRecordedSequence(const std::vector<MovementSample>& samples);
    void generateProceduralSequence(float duration, float complexity);

    // Ghost visualization positions
    Transform getGhostLeftHand() const;
    Transform getGhostRightHand() const;
    float getAccuracyScore() const { return m_accuracyScore; }

    // Playback control
    void setPlaybackSpeed(float speed) { m_playbackSpeed = speed; }
    void pause() { m_isPaused = true; }
    void resume() { m_isPaused = false; }
    void rewind() { m_playbackTime = 0; }

private:
    MovementAnalytics* m_analytics;
    std::vector<MovementSample> m_ghostSequence;

    float m_playbackTime = 0;
    float m_playbackSpeed = 1.0f;
    bool m_isPaused = false;
    float m_accuracyScore = 0;
};

// =============================================================================
// MovementMeditationMode - Slow, mindful movement with awareness cues
// =============================================================================

class MovementMeditationMode : public TrainingModule {
public:
    MovementMeditationMode(MovementAnalytics* analytics);

    void start() override;
    void update(double deltaTime, const ControllerState& left, const ControllerState& right) override;

    // Meditation settings
    void setTargetMovementSpeed(float metersPerSecond) { m_targetSpeed = metersPerSecond; }
    void setAwarenessInterval(float seconds) { m_awarenessInterval = seconds; }

    // Feedback
    bool isMovingTooFast() const { return m_currentSpeed > m_targetSpeed * 1.5f; }
    bool isStationary() const { return m_currentSpeed < 0.01f; }
    std::string getAwarenessCue() const { return m_currentCue; }

private:
    MovementAnalytics* m_analytics;

    float m_targetSpeed = 0.2f;  // 20cm per second - very slow
    float m_currentSpeed = 0;
    float m_awarenessInterval = 10.0f;
    float m_awarenessTimer = 0;
    std::string m_currentCue;

    std::vector<std::string> m_awarenessCues = {
        "Notice the sensation in your fingertips",
        "Feel the weight of your arms",
        "Observe your breath",
        "Notice any tension in your shoulders",
        "Feel the space around you",
        "Move with intention",
        "Let go of any rushing",
        "Be present in this moment"
    };
    int m_cueIndex = 0;
};

// =============================================================================
// FlowStateMode - Encourage "flow state" through continuous movement
// =============================================================================

class FlowStateMode : public TrainingModule {
public:
    FlowStateMode(MovementAnalytics* analytics);

    void start() override;
    void update(double deltaTime, const ControllerState& left, const ControllerState& right) override;

    // Flow detection
    float getFlowScore() const { return m_flowScore; }  // 0-1
    bool isInFlow() const { return m_flowScore > 0.7f; }
    float getFlowDuration() const { return m_flowDuration; }

    // Dynamic targets that respond to movement
    std::vector<Vec3> getFlowTargets() const { return m_flowTargets; }

private:
    MovementAnalytics* m_analytics;

    float m_flowScore = 0;
    float m_flowDuration = 0;

    // Flow state tracking
    float m_movementContinuity = 0;  // How continuous is the movement
    float m_movementVariety = 0;     // How varied are the movements
    float m_rhythmScore = 0;         // Is there a rhythm to the movement

    // Dynamic targets that move based on user's movement
    std::vector<Vec3> m_flowTargets;
    void updateFlowTargets(const MovementSample& sample);

    // Random generator (initialized once for performance)
    std::mt19937 m_rng{std::random_device{}()};
};

} // namespace lst
