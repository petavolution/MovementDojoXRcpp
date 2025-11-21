/**
 * Flow/Meditation Training Modes Implementation
 *
 * Implements mindful movement exploration modes inspired by yoga, qi gong,
 * and meditation practices.
 */

#include "FlowModes.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace lst {

// =============================================================================
// FreeExplorationMode
// =============================================================================

FreeExplorationMode::FreeExplorationMode(MovementAnalytics* analytics)
    : m_analytics(analytics) {
    m_name = "Free Exploration";
    m_description = "Freely explore your movement space with visual feedback";
}

void FreeExplorationMode::start() {
    m_isActive = true;
    m_lastCoverage = m_analytics ? m_analytics->getMovementSpaceCoverage() : 0;
    m_uncommonAreasDiscovered = 0;
}

void FreeExplorationMode::update(double deltaTime, const ControllerState& left,
                                  const ControllerState& right) {
    if (!m_analytics || !m_isActive) return;

    // Track coverage progress
    float currentCoverage = m_analytics->getMovementSpaceCoverage();
    if (currentCoverage > m_lastCoverage + 0.1f) {
        // Gained coverage - could trigger haptic feedback
        m_lastCoverage = currentCoverage;
    }

    // Track uncommon area discovery
    if (m_analytics->isInUncommonPosition()) {
        m_uncommonAreasDiscovered++;
    }
}

FreeExplorationMode::VisualizationData FreeExplorationMode::getVisualization() const {
    VisualizationData data;

    if (m_analytics) {
        // Get exploration targets (unexplored areas)
        if (m_showTargets && m_targetUncommon) {
            data.explorationTargets = m_analytics->getUnexploredAreas();
        }

        data.currentCoverage = m_analytics->getMovementSpaceCoverage();
        data.isInUncommonPosition = m_analytics->isInUncommonPosition();

        // Generate context-aware hints
        if (data.isInUncommonPosition) {
            data.currentHint = "Exploring new territory! Stay a moment...";
        } else if (data.currentCoverage < 20) {
            data.currentHint = "Try reaching in different directions";
        } else if (data.currentCoverage < 50) {
            data.currentHint = "Good progress! Explore areas behind you";
        } else {
            data.currentHint = "Excellent coverage! Find the hidden corners";
        }
    }

    return data;
}

// =============================================================================
// GuidedStretchMode
// =============================================================================

GuidedStretchMode::GuidedStretchMode(MovementAnalytics* analytics)
    : m_analytics(analytics) {
    m_name = "Guided Stretch";
    m_description = "Follow guided stretch sequences";
    m_sequence = getUpperBodySequence();  // Default sequence
}

void GuidedStretchMode::start() {
    m_isActive = true;
    m_currentPoseIndex = 0;
    m_poseProgress = 0;
    m_holdTimer = 0;
    m_isHolding = false;
    m_isTransitioning = true;
}

void GuidedStretchMode::update(double deltaTime, const ControllerState& left,
                                const ControllerState& right) {
    if (!m_isActive || m_sequence.empty()) return;

    const StretchPose& currentPose = m_sequence[m_currentPoseIndex];

    // Get head-relative hand positions from MovementAnalytics samples
    // The analytics system already calculates head-relative positions
    Vec3 leftRelative = left.pose.position;
    Vec3 rightRelative = right.pose.position;

    // If we have analytics with recent samples, use the head-relative positions
    if (m_analytics) {
        const auto& samples = m_analytics->getSamples();
        if (!samples.empty()) {
            leftRelative = samples.back().leftHandRelative;
            rightRelative = samples.back().rightHandRelative;
        }
    }

    // Check if hands are at target positions (in head-relative space)
    float leftDist = (leftRelative - currentPose.leftHandTarget).length();
    float rightDist = (rightRelative - currentPose.rightHandTarget).length();

    bool leftInPosition = leftDist < currentPose.toleranceMeters;
    bool rightInPosition = rightDist < currentPose.toleranceMeters;
    bool inPosition = leftInPosition && rightInPosition;

    if (m_isTransitioning) {
        // Transitioning to pose
        m_poseProgress += static_cast<float>(deltaTime) / currentPose.transitionDuration;
        if (m_poseProgress >= 1.0f || inPosition) {
            m_isTransitioning = false;
            m_poseProgress = 0;
            m_holdTimer = 0;
        }
    } else {
        // Holding pose
        if (inPosition) {
            m_isHolding = true;
            m_holdTimer += static_cast<float>(deltaTime);
            m_poseProgress = m_holdTimer / currentPose.holdDuration;

            if (m_holdTimer >= currentPose.holdDuration) {
                // Pose complete - move to next
                m_currentPoseIndex++;
                if (m_currentPoseIndex >= static_cast<int>(m_sequence.size())) {
                    m_currentPoseIndex = 0;  // Loop
                    m_isComplete = true;
                }
                m_isTransitioning = true;
                m_poseProgress = 0;
            }
        } else {
            m_isHolding = false;
            // Could reduce timer if out of position too long
        }
    }
}

void GuidedStretchMode::loadSequence(const std::string& name) {
    if (name == "upper_body") {
        m_sequence = getUpperBodySequence();
    } else if (name == "full_range") {
        m_sequence = getFullRangeSequence();
    } else if (name == "qi_gong") {
        m_sequence = getQiGongSequence();
    }
    m_currentPoseIndex = 0;
}

std::vector<StretchPose> GuidedStretchMode::getUpperBodySequence() {
    return {
        {"Arms Up", "Reach both hands above your head",
         Vec3(-0.2f, 0.6f, -0.1f), Vec3(0.2f, 0.6f, -0.1f),
         5.0f, 3.0f, 0.15f},
        {"Wide Stretch", "Stretch arms out to the sides",
         Vec3(-0.7f, 0.1f, -0.1f), Vec3(0.7f, 0.1f, -0.1f),
         5.0f, 3.0f, 0.15f},
        {"Forward Reach", "Reach both hands forward",
         Vec3(-0.2f, 0.0f, -0.6f), Vec3(0.2f, 0.0f, -0.6f),
         5.0f, 3.0f, 0.15f},
        {"Cross Body Left", "Reach right hand across to left",
         Vec3(-0.4f, 0.1f, -0.3f), Vec3(-0.5f, 0.0f, -0.2f),
         5.0f, 3.0f, 0.15f},
        {"Cross Body Right", "Reach left hand across to right",
         Vec3(0.5f, 0.0f, -0.2f), Vec3(0.4f, 0.1f, -0.3f),
         5.0f, 3.0f, 0.15f},
    };
}

std::vector<StretchPose> GuidedStretchMode::getFullRangeSequence() {
    return {
        {"Sky Reach", "Reach as high as you can",
         Vec3(-0.1f, 0.8f, -0.1f), Vec3(0.1f, 0.8f, -0.1f),
         6.0f, 4.0f, 0.15f},
        {"Ground Touch", "Bend down, reach toward the floor",
         Vec3(-0.2f, -0.8f, -0.3f), Vec3(0.2f, -0.8f, -0.3f),
         6.0f, 4.0f, 0.2f},
        {"Left Reach", "Lean and reach to your left",
         Vec3(-0.8f, 0.2f, -0.1f), Vec3(-0.4f, 0.4f, -0.1f),
         5.0f, 3.0f, 0.15f},
        {"Right Reach", "Lean and reach to your right",
         Vec3(0.4f, 0.4f, -0.1f), Vec3(0.8f, 0.2f, -0.1f),
         5.0f, 3.0f, 0.15f},
        {"Behind Reach", "Carefully reach behind your back",
         Vec3(-0.2f, -0.2f, 0.3f), Vec3(0.2f, -0.2f, 0.3f),
         4.0f, 4.0f, 0.2f},
    };
}

std::vector<StretchPose> GuidedStretchMode::getQiGongSequence() {
    return {
        {"Holding the Sky", "Slowly raise hands above head, palms up",
         Vec3(-0.15f, 0.7f, -0.1f), Vec3(0.15f, 0.7f, -0.1f),
         8.0f, 5.0f, 0.15f},
        {"Gathering Energy", "Bring hands to chest level, palms facing",
         Vec3(-0.2f, 0.0f, -0.25f), Vec3(0.2f, 0.0f, -0.25f),
         6.0f, 4.0f, 0.12f},
        {"Pushing Mountains", "Push hands forward slowly",
         Vec3(-0.3f, 0.0f, -0.6f), Vec3(0.3f, 0.0f, -0.6f),
         6.0f, 4.0f, 0.15f},
        {"Parting Clouds", "Sweep arms out to sides",
         Vec3(-0.7f, 0.15f, -0.2f), Vec3(0.7f, 0.15f, -0.2f),
         6.0f, 4.0f, 0.15f},
        {"Drawing the Bow", "Draw an imaginary bow to the left",
         Vec3(-0.6f, 0.1f, -0.2f), Vec3(0.1f, 0.1f, -0.4f),
         6.0f, 4.0f, 0.15f},
        {"Drawing the Bow", "Draw an imaginary bow to the right",
         Vec3(-0.1f, 0.1f, -0.4f), Vec3(0.6f, 0.1f, -0.2f),
         6.0f, 4.0f, 0.15f},
        {"Returning to Center", "Bring hands back to rest at sides",
         Vec3(-0.25f, -0.3f, -0.1f), Vec3(0.25f, -0.3f, -0.1f),
         5.0f, 4.0f, 0.15f},
    };
}

// =============================================================================
// BreathingSyncMode
// =============================================================================

BreathingSyncMode::BreathingSyncMode(MovementAnalytics* analytics)
    : m_analytics(analytics) {
    m_name = "Breathing Sync";
    m_description = "Synchronize movement with breathing rhythm";
}

void BreathingSyncMode::start() {
    m_isActive = true;
    m_currentPhase = BreathPhase::Inhale;
    m_phaseTimer = 0;
    m_phaseProgress = 0;
    m_cycleCount = 0;
}

void BreathingSyncMode::update(double deltaTime, const ControllerState& left,
                                const ControllerState& right) {
    if (!m_isActive) return;

    m_phaseTimer += static_cast<float>(deltaTime);

    // Get current phase duration
    float phaseDuration = 0;
    switch (m_currentPhase) {
        case BreathPhase::Inhale: phaseDuration = m_inhaleTime; break;
        case BreathPhase::Hold: phaseDuration = m_holdTime; break;
        case BreathPhase::Exhale: phaseDuration = m_exhaleTime; break;
        case BreathPhase::Pause: phaseDuration = m_pauseTime; break;
    }

    m_phaseProgress = m_phaseTimer / phaseDuration;

    // Transition to next phase
    if (m_phaseTimer >= phaseDuration) {
        m_phaseTimer = 0;
        switch (m_currentPhase) {
            case BreathPhase::Inhale:
                m_currentPhase = (m_holdTime > 0) ? BreathPhase::Hold : BreathPhase::Exhale;
                break;
            case BreathPhase::Hold:
                m_currentPhase = BreathPhase::Exhale;
                break;
            case BreathPhase::Exhale:
                m_currentPhase = (m_pauseTime > 0) ? BreathPhase::Pause : BreathPhase::Inhale;
                break;
            case BreathPhase::Pause:
                m_currentPhase = BreathPhase::Inhale;
                m_cycleCount++;
                break;
        }
    }
}

void BreathingSyncMode::setBreathCycle(float inhaleSeconds, float exhaleSeconds,
                                        float holdSeconds) {
    m_inhaleTime = inhaleSeconds;
    m_exhaleTime = exhaleSeconds;
    m_holdTime = holdSeconds;
}

Vec3 BreathingSyncMode::getSuggestedLeftHandPosition() const {
    // During inhale: hands rise
    // During exhale: hands lower
    float heightOffset = 0;
    switch (m_currentPhase) {
        case BreathPhase::Inhale:
            heightOffset = m_phaseProgress * 0.4f;
            break;
        case BreathPhase::Hold:
            heightOffset = 0.4f;
            break;
        case BreathPhase::Exhale:
            heightOffset = (1.0f - m_phaseProgress) * 0.4f;
            break;
        case BreathPhase::Pause:
            heightOffset = 0;
            break;
    }
    return Vec3(-0.3f, heightOffset, -0.3f);
}

Vec3 BreathingSyncMode::getSuggestedRightHandPosition() const {
    Vec3 left = getSuggestedLeftHandPosition();
    return Vec3(-left.x, left.y, left.z);  // Mirror X
}

float BreathingSyncMode::getSuggestedMovementSpeed() const {
    // Slower during hold phases
    if (m_currentPhase == BreathPhase::Hold || m_currentPhase == BreathPhase::Pause) {
        return 0.05f;
    }
    return 0.15f;  // 15cm/s during active phases
}

// =============================================================================
// MirrorMode
// =============================================================================

MirrorMode::MirrorMode(MovementAnalytics* analytics)
    : m_analytics(analytics) {
    m_name = "Mirror Mode";
    m_description = "Follow a ghost guide through movements";
}

void MirrorMode::start() {
    m_isActive = true;
    m_playbackTime = 0;
    m_isPaused = false;
    m_accuracyScore = 0;
}

void MirrorMode::update(double deltaTime, const ControllerState& left,
                         const ControllerState& right) {
    if (!m_isActive || m_isPaused || m_ghostSequence.empty()) return;

    m_playbackTime += static_cast<float>(deltaTime) * m_playbackSpeed;

    // Get ghost positions at current time
    Transform ghostLeft = getGhostLeftHand();
    Transform ghostRight = getGhostRightHand();

    // Calculate accuracy
    float leftError = (left.position - ghostLeft.position).length();
    float rightError = (right.position - ghostRight.position).length();
    float avgError = (leftError + rightError) / 2.0f;

    // Convert to 0-1 score (0.5m error = 0 score)
    m_accuracyScore = std::max(0.0f, 1.0f - avgError / 0.5f);

    // Loop if reached end
    if (m_playbackTime >= m_ghostSequence.back().timestamp) {
        m_playbackTime = 0;
    }
}

void MirrorMode::loadRecordedSequence(const std::vector<MovementSample>& samples) {
    m_ghostSequence = samples;
    m_playbackTime = 0;
}

void MirrorMode::generateProceduralSequence(float duration, float complexity) {
    m_ghostSequence.clear();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

    float sampleRate = 60.0f;  // 60 samples per second
    int numSamples = static_cast<int>(duration * sampleRate);

    // Generate smooth procedural movement using sine waves
    float freq1 = 0.3f * (1.0f + complexity);
    float freq2 = 0.5f * (1.0f + complexity);
    float amp = 0.3f * (0.5f + complexity * 0.5f);

    for (int i = 0; i < numSamples; i++) {
        MovementSample sample;
        sample.timestamp = i / sampleRate;

        float t = sample.timestamp;

        // Generate smooth figure-8 like patterns
        sample.leftHandRelative = Vec3(
            -0.3f + amp * std::sin(t * freq1),
            0.1f + amp * 0.5f * std::sin(t * freq2),
            -0.3f + amp * 0.3f * std::cos(t * freq1)
        );

        sample.rightHandRelative = Vec3(
            0.3f + amp * std::sin(t * freq1 + 3.14159f),
            0.1f + amp * 0.5f * std::sin(t * freq2 + 3.14159f),
            -0.3f + amp * 0.3f * std::cos(t * freq1 + 3.14159f)
        );

        m_ghostSequence.push_back(sample);
    }
}

Transform MirrorMode::getGhostLeftHand() const {
    if (m_ghostSequence.empty()) return Transform();

    // Find samples around current time
    size_t idx = 0;
    for (size_t i = 0; i < m_ghostSequence.size() - 1; i++) {
        if (m_ghostSequence[i + 1].timestamp > m_playbackTime) {
            idx = i;
            break;
        }
    }

    // Linear interpolation
    if (idx + 1 < m_ghostSequence.size()) {
        const MovementSample& a = m_ghostSequence[idx];
        const MovementSample& b = m_ghostSequence[idx + 1];
        float t = (m_playbackTime - static_cast<float>(a.timestamp)) /
                  static_cast<float>(b.timestamp - a.timestamp);
        t = std::clamp(t, 0.0f, 1.0f);

        Transform result;
        result.position = a.leftHandRelative * (1.0f - t) + b.leftHandRelative * t;
        result.orientation = Quat::identity();
        result.scale = Vec3(1, 1, 1);
        return result;
    }

    Transform result;
    result.position = m_ghostSequence[idx].leftHandRelative;
    result.orientation = Quat::identity();
    result.scale = Vec3(1, 1, 1);
    return result;
}

Transform MirrorMode::getGhostRightHand() const {
    if (m_ghostSequence.empty()) return Transform();

    size_t idx = 0;
    for (size_t i = 0; i < m_ghostSequence.size() - 1; i++) {
        if (m_ghostSequence[i + 1].timestamp > m_playbackTime) {
            idx = i;
            break;
        }
    }

    if (idx + 1 < m_ghostSequence.size()) {
        const MovementSample& a = m_ghostSequence[idx];
        const MovementSample& b = m_ghostSequence[idx + 1];
        float t = (m_playbackTime - static_cast<float>(a.timestamp)) /
                  static_cast<float>(b.timestamp - a.timestamp);
        t = std::clamp(t, 0.0f, 1.0f);

        Transform result;
        result.position = a.rightHandRelative * (1.0f - t) + b.rightHandRelative * t;
        result.orientation = Quat::identity();
        result.scale = Vec3(1, 1, 1);
        return result;
    }

    Transform result;
    result.position = m_ghostSequence[idx].rightHandRelative;
    result.orientation = Quat::identity();
    result.scale = Vec3(1, 1, 1);
    return result;
}

// =============================================================================
// MovementMeditationMode
// =============================================================================

MovementMeditationMode::MovementMeditationMode(MovementAnalytics* analytics)
    : m_analytics(analytics) {
    m_name = "Movement Meditation";
    m_description = "Slow, mindful movement with awareness cues";
}

void MovementMeditationMode::start() {
    m_isActive = true;
    m_awarenessTimer = 0;
    m_cueIndex = 0;
    m_currentCue = m_awarenessCues[0];
}

void MovementMeditationMode::update(double deltaTime, const ControllerState& left,
                                     const ControllerState& right) {
    if (!m_isActive) return;

    // Calculate current movement speed from controller velocities
    float leftSpeed = left.linearVelocity.length();
    float rightSpeed = right.linearVelocity.length();
    m_currentSpeed = (leftSpeed + rightSpeed) / 2.0f;

    // Update awareness timer
    m_awarenessTimer += static_cast<float>(deltaTime);
    if (m_awarenessTimer >= m_awarenessInterval) {
        m_awarenessTimer = 0;
        m_cueIndex = (m_cueIndex + 1) % m_awarenessCues.size();
        m_currentCue = m_awarenessCues[m_cueIndex];
    }
}

// =============================================================================
// FlowStateMode
// =============================================================================

FlowStateMode::FlowStateMode(MovementAnalytics* analytics)
    : m_analytics(analytics) {
    m_name = "Flow State";
    m_description = "Encourage flow state through continuous movement";
}

void FlowStateMode::start() {
    m_isActive = true;
    m_flowScore = 0;
    m_flowDuration = 0;
    m_flowTargets.clear();

    // Initialize some starting targets
    m_flowTargets = {
        Vec3(-0.4f, 0.2f, -0.4f),
        Vec3(0.4f, 0.2f, -0.4f),
        Vec3(0.0f, 0.5f, -0.3f)
    };
}

void FlowStateMode::update(double deltaTime, const ControllerState& left,
                            const ControllerState& right) {
    if (!m_isActive) return;

    float dt = static_cast<float>(deltaTime);

    // Calculate movement metrics
    float leftSpeed = left.linearVelocity.length();
    float rightSpeed = right.linearVelocity.length();
    float avgSpeed = (leftSpeed + rightSpeed) / 2.0f;

    // Continuity: are we moving consistently?
    if (avgSpeed > 0.05f && avgSpeed < 1.0f) {
        m_movementContinuity = std::min(1.0f, m_movementContinuity + dt * 0.5f);
    } else {
        m_movementContinuity = std::max(0.0f, m_movementContinuity - dt * 0.3f);
    }

    // Variety: track position changes over time
    if (m_analytics) {
        // Higher variety score if exploring different areas
        float coverage = m_analytics->getMovementSpaceCoverage();
        m_movementVariety = coverage / 100.0f;
    }

    // Calculate overall flow score
    m_flowScore = (m_movementContinuity * 0.5f + m_movementVariety * 0.3f +
                   m_rhythmScore * 0.2f);

    // Track flow duration
    if (m_flowScore > 0.7f) {
        m_flowDuration += dt;
    } else {
        m_flowDuration = std::max(0.0f, m_flowDuration - dt * 0.5f);
    }

    // Update dynamic targets
    if (m_analytics) {
        const auto& samples = m_analytics->getSamples();
        if (!samples.empty()) {
            updateFlowTargets(samples.back());
        }
    }
}

void FlowStateMode::updateFlowTargets(const MovementSample& sample) {
    // Move targets dynamically based on where the user is moving
    // Targets should "dance" ahead of the user's movement

    for (auto& target : m_flowTargets) {
        // Get direction from current hand positions to target
        Vec3 leftDir = target - sample.leftHandRelative;
        Vec3 rightDir = target - sample.rightHandRelative;

        // If either hand is close to a target, move the target
        if (leftDir.length() < 0.15f || rightDir.length() < 0.15f) {
            // Generate new position in a semi-random direction
            // but biased toward unexplored areas
            std::uniform_real_distribution<float> dist(-0.6f, 0.6f);

            target = Vec3(dist(m_rng), 0.1f + dist(m_rng) * 0.4f, -0.3f + dist(m_rng) * 0.3f);
        }
    }
}

} // namespace lst
