#include "MovementAnalytics.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace lst {

// =============================================================================
// MovementAnalytics
// =============================================================================

MovementAnalytics::MovementAnalytics() {
    initializeMovementSpace();
}

MovementAnalytics::~MovementAnalytics() {
    if (m_isRecording) {
        endSession();
    }
}

void MovementAnalytics::initializeMovementSpace() {
    // Create a 3D grid of voxels covering the movement space
    // Grid covers -SPACE_RADIUS to +SPACE_RADIUS in each dimension
    m_voxelGridSize = static_cast<int>(std::ceil(2.0f * SPACE_RADIUS / m_voxelSize));
    int totalVoxels = m_voxelGridSize * m_voxelGridSize * m_voxelGridSize;

    m_movementSpace.clear();
    m_movementSpace.resize(totalVoxels);

    for (int x = 0; x < m_voxelGridSize; x++) {
        for (int y = 0; y < m_voxelGridSize; y++) {
            for (int z = 0; z < m_voxelGridSize; z++) {
                int index = x + y * m_voxelGridSize + z * m_voxelGridSize * m_voxelGridSize;

                Vec3 center(
                    -SPACE_RADIUS + (x + 0.5f) * m_voxelSize,
                    -SPACE_RADIUS + (y + 0.5f) * m_voxelSize,
                    -SPACE_RADIUS + (z + 0.5f) * m_voxelSize
                );

                m_movementSpace[index].center = center;
                m_movementSpace[index].visitCount = 0;
                m_movementSpace[index].totalTimeSpent = 0;
                m_movementSpace[index].lastVisitTime = 0;

                // Mark positions that are uncommon in daily life
                // Above head, behind back, very low, etc.
                bool uncommon = false;
                if (center.y > 0.5f) uncommon = true;           // Above head level
                if (center.y < -0.8f) uncommon = true;          // Very low (near floor)
                if (center.z > 0.3f) uncommon = true;           // Behind body
                if (std::abs(center.x) > 0.8f && center.y > 0) uncommon = true; // High and wide

                m_movementSpace[index].isUncommon = uncommon;
            }
        }
    }

    std::cout << "Movement space initialized: " << totalVoxels << " voxels ("
              << m_voxelGridSize << "^3)" << std::endl;
}

void MovementAnalytics::startSession() {
    m_isRecording = true;
    m_isPaused = false;
    m_samples.clear();

    auto now = std::chrono::high_resolution_clock::now();
    m_sessionStartTime = std::chrono::duration<double>(now.time_since_epoch()).count();
    m_lastSampleTime = m_sessionStartTime;

    // Reset statistics
    m_sessionSummary = SessionSummary();
    m_hasPreviousSample = false;

    // Reset movement space visit counts (but keep uncommon markers)
    for (auto& voxel : m_movementSpace) {
        voxel.visitCount = 0;
        voxel.totalTimeSpent = 0;
        voxel.lastVisitTime = 0;
    }

    std::cout << "Movement analytics session started" << std::endl;

    if (m_callback) {
        m_callback("session_start", "{}");
    }
}

void MovementAnalytics::endSession() {
    m_isRecording = false;

    // Finalize statistics
    auto now = std::chrono::high_resolution_clock::now();
    double endTime = std::chrono::duration<double>(now.time_since_epoch()).count();
    m_sessionSummary.duration = endTime - m_sessionStartTime;
    m_sessionSummary.totalSamples = static_cast<int>(m_samples.size());

    // Calculate final coverage
    int visitedVoxels = 0;
    int uncommonVisited = 0;
    for (const auto& voxel : m_movementSpace) {
        if (voxel.visitCount > 0) {
            visitedVoxels++;
            if (voxel.isUncommon) {
                uncommonVisited++;
            }
        }
    }
    m_sessionSummary.uniqueVoxelsVisited = visitedVoxels;
    m_sessionSummary.uncommonMovementsExplored = uncommonVisited;
    m_sessionSummary.movementSpaceCoverage =
        100.0f * static_cast<float>(visitedVoxels) / m_movementSpace.size();

    std::cout << "Movement analytics session ended" << std::endl;
    std::cout << "  Duration: " << m_sessionSummary.duration << "s" << std::endl;
    std::cout << "  Samples: " << m_sessionSummary.totalSamples << std::endl;
    std::cout << "  Coverage: " << m_sessionSummary.movementSpaceCoverage << "%" << std::endl;
    std::cout << "  Uncommon movements: " << uncommonVisited << std::endl;

    if (m_callback) {
        std::stringstream ss;
        ss << "{\"duration\":" << m_sessionSummary.duration
           << ",\"coverage\":" << m_sessionSummary.movementSpaceCoverage
           << ",\"uncommon\":" << uncommonVisited << "}";
        m_callback("session_end", ss.str());
    }
}

void MovementAnalytics::pauseSession() {
    m_isPaused = true;
}

void MovementAnalytics::resumeSession() {
    m_isPaused = false;
}

void MovementAnalytics::recordSample(const Transform& head,
                                     const ControllerState& leftController,
                                     const ControllerState& rightController) {
    if (!m_isRecording || m_isPaused) return;

    auto now = std::chrono::high_resolution_clock::now();
    double currentTime = std::chrono::duration<double>(now.time_since_epoch()).count();

    // Rate limiting
    double timeSinceLastSample = currentTime - m_lastSampleTime;
    double targetInterval = 1.0 / m_targetSampleRate;
    if (timeSinceLastSample < targetInterval * 0.9) {
        return;  // Too soon for another sample
    }

    MovementSample sample;
    sample.timestamp = currentTime - m_sessionStartTime;

    // Head tracking
    sample.headPosition = head.position;
    sample.headOrientation = head.orientation;

    // Controller tracking
    sample.leftHandPosition = leftController.pose.position;
    sample.leftHandOrientation = leftController.pose.orientation;
    sample.rightHandPosition = rightController.pose.position;
    sample.rightHandOrientation = rightController.pose.orientation;

    // Input state
    sample.leftGrip = leftController.gripValue;
    sample.rightGrip = rightController.gripValue;
    sample.leftTrigger = leftController.triggerValue;
    sample.rightTrigger = rightController.triggerValue;

    // Calculate velocities
    if (m_hasPreviousSample) {
        float dt = static_cast<float>(timeSinceLastSample);
        if (dt > 0.001f) {
            sample.headVelocity = (sample.headPosition - m_previousSample.headPosition) * (1.0f / dt);
            sample.leftHandVelocity = (sample.leftHandPosition - m_previousSample.leftHandPosition) * (1.0f / dt);
            sample.rightHandVelocity = (sample.rightHandPosition - m_previousSample.rightHandPosition) * (1.0f / dt);
        }
    }

    // Calculate head-relative positions
    calculateDerivedValues(sample);

    // Store sample
    m_samples.push_back(sample);
    if (m_samples.size() > MAX_SAMPLES) {
        m_samples.pop_front();
    }

    // Update recent buffer
    m_recentBuffer[m_recentBufferIndex] = sample;
    m_recentBufferIndex = (m_recentBufferIndex + 1) % m_recentBuffer.size();

    // Update analysis
    updateStatistics(sample);
    updateMovementSpace(sample);
    detectPatterns();

    m_previousSample = sample;
    m_hasPreviousSample = true;
    m_lastSampleTime = currentTime;
}

void MovementAnalytics::recordSample(double deltaTime, const Transform& head,
                                     const ControllerState& leftController,
                                     const ControllerState& rightController) {
    // Just forward to the main implementation - deltaTime is calculated internally
    (void)deltaTime;
    recordSample(head, leftController, rightController);
}

void MovementAnalytics::initialize(const MovementAnalyticsConfig& config) {
    m_voxelSize = config.voxelResolution;
    m_targetSampleRate = config.sampleRate;
    initializeMovementSpace();
}

void MovementAnalytics::calculateDerivedValues(MovementSample& sample) {
    // Transform hand positions to head-relative space
    sample.leftHandRelative = worldToHeadSpace(sample.leftHandPosition,
        Transform(sample.headPosition, sample.headOrientation));
    sample.rightHandRelative = worldToHeadSpace(sample.rightHandPosition,
        Transform(sample.headPosition, sample.headOrientation));
}

Vec3 MovementAnalytics::worldToHeadSpace(const Vec3& worldPos, const Transform& head) const {
    // Get position relative to head
    Vec3 relativePos = worldPos - head.position;

    // Rotate into head space (inverse rotation)
    Quat invRot = head.orientation.conjugate();
    return invRot.rotate(relativePos);
}

void MovementAnalytics::updateStatistics(const MovementSample& sample) {
    // Update position bounds
    auto updatePointStats = [](PointStatistics& stats, const Vec3& pos, const Vec3& vel, const Vec3& relPos) {
        if (stats.sampleCount == 0) {
            stats.minPosition = pos;
            stats.maxPosition = pos;
            stats.averagePosition = pos;
        } else {
            stats.minPosition.x = std::min(stats.minPosition.x, pos.x);
            stats.minPosition.y = std::min(stats.minPosition.y, pos.y);
            stats.minPosition.z = std::min(stats.minPosition.z, pos.z);
            stats.maxPosition.x = std::max(stats.maxPosition.x, pos.x);
            stats.maxPosition.y = std::max(stats.maxPosition.y, pos.y);
            stats.maxPosition.z = std::max(stats.maxPosition.z, pos.z);

            // Running average
            float n = static_cast<float>(stats.sampleCount);
            stats.averagePosition = stats.averagePosition * (n / (n + 1)) + pos * (1.0f / (n + 1));
        }

        float speed = vel.length();
        stats.maxSpeed = std::max(stats.maxSpeed, speed);
        stats.averageSpeed = (stats.averageSpeed * stats.sampleCount + speed) / (stats.sampleCount + 1);

        // Update reach stats (from head-relative position)
        stats.reachUp = std::max(stats.reachUp, relPos.y);
        stats.reachDown = std::max(stats.reachDown, -relPos.y);
        stats.reachForward = std::max(stats.reachForward, -relPos.z);
        stats.reachBack = std::max(stats.reachBack, relPos.z);
        stats.reachLeft = std::max(stats.reachLeft, -relPos.x);
        stats.reachRight = std::max(stats.reachRight, relPos.x);

        stats.sampleCount++;
    };

    updatePointStats(m_sessionSummary.headStats, sample.headPosition,
                     sample.headVelocity, Vec3(0, 0, 0));
    updatePointStats(m_sessionSummary.leftHandStats, sample.leftHandPosition,
                     sample.leftHandVelocity, sample.leftHandRelative);
    updatePointStats(m_sessionSummary.rightHandStats, sample.rightHandPosition,
                     sample.rightHandVelocity, sample.rightHandRelative);
}

int MovementAnalytics::positionToVoxelIndex(const Vec3& relativePos) const {
    // Convert head-relative position to voxel index
    int x = static_cast<int>((relativePos.x + SPACE_RADIUS) / m_voxelSize);
    int y = static_cast<int>((relativePos.y + SPACE_RADIUS) / m_voxelSize);
    int z = static_cast<int>((relativePos.z + SPACE_RADIUS) / m_voxelSize);

    // Clamp to grid bounds
    x = std::max(0, std::min(m_voxelGridSize - 1, x));
    y = std::max(0, std::min(m_voxelGridSize - 1, y));
    z = std::max(0, std::min(m_voxelGridSize - 1, z));

    return x + y * m_voxelGridSize + z * m_voxelGridSize * m_voxelGridSize;
}

void MovementAnalytics::updateMovementSpace(const MovementSample& sample) {
    float dt = 1.0f / m_targetSampleRate;

    // Update voxels for both hands
    auto updateVoxel = [this, &sample, dt](const Vec3& relPos) {
        int idx = positionToVoxelIndex(relPos);
        if (idx >= 0 && idx < static_cast<int>(m_movementSpace.size())) {
            m_movementSpace[idx].visitCount++;
            m_movementSpace[idx].totalTimeSpent += dt;
            m_movementSpace[idx].lastVisitTime = static_cast<float>(sample.timestamp);

            // Notify if entering uncommon area for first time
            if (m_movementSpace[idx].isUncommon && m_movementSpace[idx].visitCount == 1) {
                if (m_callback) {
                    std::stringstream ss;
                    ss << "{\"position\":[" << relPos.x << "," << relPos.y << "," << relPos.z << "]}";
                    m_callback("uncommon_area_entered", ss.str());
                }
            }
        }
    };

    updateVoxel(sample.leftHandRelative);
    updateVoxel(sample.rightHandRelative);
}

void MovementAnalytics::detectPatterns() {
    // Analyze recent samples to detect movement patterns
    if (m_samples.size() < 30) {
        m_currentPattern = MovementPattern::Stationary;
        return;
    }

    // Calculate average velocity over recent samples
    float avgLeftSpeed = 0, avgRightSpeed = 0;
    int count = std::min(30, static_cast<int>(m_samples.size()));

    auto it = m_samples.rbegin();
    for (int i = 0; i < count && it != m_samples.rend(); i++, ++it) {
        avgLeftSpeed += it->leftHandVelocity.length();
        avgRightSpeed += it->rightHandVelocity.length();
    }
    avgLeftSpeed /= count;
    avgRightSpeed /= count;

    float totalSpeed = avgLeftSpeed + avgRightSpeed;

    // Classify pattern
    if (totalSpeed < 0.1f) {
        m_currentPattern = MovementPattern::Stationary;
    } else if (totalSpeed < 0.5f) {
        m_currentPattern = MovementPattern::Slow;
    } else if (totalSpeed < 1.5f) {
        m_currentPattern = MovementPattern::Moderate;
    } else {
        m_currentPattern = MovementPattern::Fast;
    }

    // Check for asymmetry
    if (avgLeftSpeed > 0.1f || avgRightSpeed > 0.1f) {
        float asymmetry = std::abs(avgLeftSpeed - avgRightSpeed) / (avgLeftSpeed + avgRightSpeed + 0.001f);
        if (asymmetry > 0.6f) {
            m_currentPattern = MovementPattern::Asymmetric;
        }
    }
}

MovementPattern MovementAnalytics::getCurrentPattern() const {
    return m_currentPattern;
}

float MovementAnalytics::getCurrentIntensity() const {
    if (m_samples.empty()) return 0;

    const auto& latest = m_samples.back();
    float leftSpeed = latest.leftHandVelocity.length();
    float rightSpeed = latest.rightHandVelocity.length();

    // Normalize to 0-1 range (assuming max reasonable speed is ~5 m/s)
    return std::min(1.0f, (leftSpeed + rightSpeed) / 10.0f);
}

float MovementAnalytics::getMovementSpaceCoverage() const {
    int visited = 0;
    for (const auto& voxel : m_movementSpace) {
        if (voxel.visitCount > 0) visited++;
    }
    return 100.0f * static_cast<float>(visited) / m_movementSpace.size();
}

std::vector<Vec3> MovementAnalytics::getUnexploredAreas() const {
    std::vector<Vec3> unexplored;

    for (const auto& voxel : m_movementSpace) {
        if (voxel.visitCount == 0) {
            // Only include reachable areas (within arm's reach)
            float dist = voxel.center.length();
            if (dist < 0.9f) {  // ~90cm reach
                unexplored.push_back(voxel.center);
            }
        }
    }

    return unexplored;
}

Vec3 MovementAnalytics::getSuggestedExplorationDirection() const {
    // Find the direction with least exploration
    Vec3 suggestion(0, 0.5f, -0.5f);  // Default: up and forward

    float minCoverage = 1.0f;

    // Check 6 main directions
    std::array<Vec3, 6> directions = {{
        Vec3(0, 1, 0),   // Up
        Vec3(0, -1, 0),  // Down
        Vec3(1, 0, 0),   // Right
        Vec3(-1, 0, 0),  // Left
        Vec3(0, 0, -1),  // Forward
        Vec3(0, 0, 1)    // Back
    }};

    for (const auto& dir : directions) {
        int visited = 0, total = 0;

        for (const auto& voxel : m_movementSpace) {
            if (Vec3::dot(voxel.center.normalized(), dir) > 0.5f) {
                total++;
                if (voxel.visitCount > 0) visited++;
            }
        }

        float coverage = (total > 0) ? static_cast<float>(visited) / total : 1.0f;
        if (coverage < minCoverage) {
            minCoverage = coverage;
            suggestion = dir;
        }
    }

    return suggestion;
}

bool MovementAnalytics::isInUncommonPosition() const {
    if (m_samples.empty()) return false;

    const auto& latest = m_samples.back();

    int leftIdx = positionToVoxelIndex(latest.leftHandRelative);
    int rightIdx = positionToVoxelIndex(latest.rightHandRelative);

    bool leftUncommon = (leftIdx >= 0 && leftIdx < static_cast<int>(m_movementSpace.size()))
                        && m_movementSpace[leftIdx].isUncommon;
    bool rightUncommon = (rightIdx >= 0 && rightIdx < static_cast<int>(m_movementSpace.size()))
                         && m_movementSpace[rightIdx].isUncommon;

    return leftUncommon || rightUncommon;
}

const std::deque<MovementSample>& MovementAnalytics::getRecentSamples(int count) const {
    return m_samples;
}

std::vector<Vec3> MovementAnalytics::getMovementTrail(int trackedPoint, float duration) const {
    std::vector<Vec3> trail;

    if (m_samples.empty()) return trail;

    double currentTime = m_samples.back().timestamp;
    double startTime = currentTime - duration;

    for (auto it = m_samples.rbegin(); it != m_samples.rend(); ++it) {
        if (it->timestamp < startTime) break;

        switch (trackedPoint) {
            case 0: trail.push_back(it->headPosition); break;
            case 1: trail.push_back(it->leftHandPosition); break;
            case 2: trail.push_back(it->rightHandPosition); break;
        }
    }

    std::reverse(trail.begin(), trail.end());
    return trail;
}

MovementAnalytics::HeatmapData MovementAnalytics::generateHeatmap(int trackedPoint, float resolution) const {
    HeatmapData data;

    // Simplified heatmap generation
    for (const auto& voxel : m_movementSpace) {
        if (voxel.visitCount > 0) {
            data.positions.push_back(voxel.center);
            data.intensities.push_back(std::min(1.0f, voxel.visitCount / 100.0f));
        }
    }

    data.boundsMin = Vec3(-SPACE_RADIUS, -SPACE_RADIUS, -SPACE_RADIUS);
    data.boundsMax = Vec3(SPACE_RADIUS, SPACE_RADIUS, SPACE_RADIUS);

    return data;
}

bool MovementAnalytics::exportSession(const std::string& filepath) const {
    // JSON export
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"duration\": " << m_sessionSummary.duration << ",\n";
    file << "  \"samples\": " << m_sessionSummary.totalSamples << ",\n";
    file << "  \"coverage\": " << m_sessionSummary.movementSpaceCoverage << ",\n";
    file << "  \"uncommonExplored\": " << m_sessionSummary.uncommonMovementsExplored << "\n";
    file << "}\n";

    file.close();
    return true;
}

bool MovementAnalytics::exportToCSV(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    // Header
    file << "timestamp,head_x,head_y,head_z,left_x,left_y,left_z,right_x,right_y,right_z\n";

    for (const auto& sample : m_samples) {
        file << sample.timestamp << ","
             << sample.headPosition.x << "," << sample.headPosition.y << "," << sample.headPosition.z << ","
             << sample.leftHandPosition.x << "," << sample.leftHandPosition.y << "," << sample.leftHandPosition.z << ","
             << sample.rightHandPosition.x << "," << sample.rightHandPosition.y << "," << sample.rightHandPosition.z
             << "\n";
    }

    file.close();
    return true;
}

// =============================================================================
// MovementSpaceVisualizer
// =============================================================================

MovementSpaceVisualizer::MovementSpaceVisualizer(MovementAnalytics* analytics)
    : m_analytics(analytics) {}

std::vector<MovementSpaceVisualizer::TrailSegment> MovementSpaceVisualizer::generateTrail(
    int trackedPoint, float duration, bool colorBySpeed) const {

    std::vector<TrailSegment> segments;
    auto trail = m_analytics->getMovementTrail(trackedPoint, duration);

    if (trail.size() < 2) return segments;

    for (size_t i = 1; i < trail.size(); i++) {
        TrailSegment seg;
        seg.start = trail[i - 1];
        seg.end = trail[i];

        if (colorBySpeed) {
            float speed = (seg.end - seg.start).length() * 90.0f;  // Approximate speed
            float t = std::min(1.0f, speed / 3.0f);
            // Blue (slow) -> Green (medium) -> Red (fast)
            if (t < 0.5f) {
                seg.color = Color(0, t * 2, 1 - t * 2);
            } else {
                seg.color = Color((t - 0.5f) * 2, 1 - (t - 0.5f) * 2, 0);
            }
        } else {
            seg.color = Color::cyan();
        }

        seg.width = 0.005f;
        segments.push_back(seg);
    }

    return segments;
}

std::vector<MovementSpaceVisualizer::CoveragePoint> MovementSpaceVisualizer::generateCoverageSphere(
    int segments) const {

    std::vector<CoveragePoint> points;
    // Generate points on a sphere and check coverage in each direction
    // Simplified implementation

    return points;
}

std::vector<MovementSpaceVisualizer::ExplorationTarget> MovementSpaceVisualizer::getExplorationTargets(
    int maxTargets) const {

    std::vector<ExplorationTarget> targets;

    auto unexplored = m_analytics->getUnexploredAreas();

    // Sort by "interestingness" (prefer uncommon areas)
    std::sort(unexplored.begin(), unexplored.end(), [](const Vec3& a, const Vec3& b) {
        // Prefer positions above head or behind body
        float scoreA = a.y + (a.z > 0 ? 0.5f : 0);
        float scoreB = b.y + (b.z > 0 ? 0.5f : 0);
        return scoreA > scoreB;
    });

    for (int i = 0; i < maxTargets && i < static_cast<int>(unexplored.size()); i++) {
        ExplorationTarget target;
        target.position = unexplored[i];
        target.priority = 1.0f - (static_cast<float>(i) / maxTargets);

        // Generate hint text
        if (target.position.y > 0.3f) {
            target.hint = "Reach upward";
            if (target.position.z > 0) target.hint += " and back";
        } else if (target.position.y < -0.5f) {
            target.hint = "Reach down low";
        } else if (target.position.z > 0) {
            target.hint = "Reach behind you";
        } else {
            target.hint = "Explore this area";
        }

        target.guideColor = Color(0.2f, 0.8f, 0.4f);  // Green guide
        targets.push_back(target);
    }

    return targets;
}

} // namespace lst
