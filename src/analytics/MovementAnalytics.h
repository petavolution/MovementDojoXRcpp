#pragma once

#include "Types.h"
#include <vector>
#include <deque>
#include <string>
#include <functional>
#include <chrono>
#include <map>
#include <array>

namespace lst {

/**
 * MovementAnalytics - Records and analyzes ALL movement data
 *
 * Core philosophy: Track every conceivable movement of head, hands, and body
 * to help users discover and explore movements they NEVER make in daily life.
 *
 * Features:
 * - Continuous recording of all tracked points (HMD, controllers)
 * - Movement pattern analysis (velocity, acceleration, range of motion)
 * - "Movement space" coverage tracking - what movements haven't been explored
 * - Session statistics and historical data
 * - Export/visualization data generation
 */

// Single movement sample
struct MovementSample {
    double timestamp;  // Seconds since session start

    // Head/HMD tracking
    Vec3 headPosition;
    Quat headOrientation;
    Vec3 headVelocity;
    Vec3 headAngularVelocity;

    // Left hand/controller
    Vec3 leftHandPosition;
    Quat leftHandOrientation;
    Vec3 leftHandVelocity;
    Vec3 leftHandAngularVelocity;

    // Right hand/controller
    Vec3 rightHandPosition;
    Quat rightHandOrientation;
    Vec3 rightHandVelocity;
    Vec3 rightHandAngularVelocity;

    // Derived: hand positions relative to head (body-centric)
    Vec3 leftHandRelative;   // Left hand position in head-space
    Vec3 rightHandRelative;  // Right hand position in head-space

    // Input state (for correlating with actions)
    float leftGrip;
    float rightGrip;
    float leftTrigger;
    float rightTrigger;
};

// Movement statistics for a tracked point
struct PointStatistics {
    Vec3 minPosition;
    Vec3 maxPosition;
    Vec3 averagePosition;
    float maxSpeed;
    float averageSpeed;
    float totalDistance;
    int sampleCount;

    // Range of motion (in head-relative space)
    float reachUp;      // Max height above head
    float reachDown;    // Max depth below head
    float reachForward; // Max forward reach
    float reachBack;    // Max backward reach
    float reachLeft;    // Max left reach
    float reachRight;   // Max right reach
};

// Movement space voxel for coverage tracking
struct MovementVoxel {
    Vec3 center;
    int visitCount;
    float totalTimeSpent;
    float lastVisitTime;
    bool isUncommon;  // Rarely visited in daily life
};

// Session summary
struct SessionSummary {
    double duration;
    int totalSamples;

    PointStatistics headStats;
    PointStatistics leftHandStats;
    PointStatistics rightHandStats;

    // Movement coverage
    float movementSpaceCoverage;  // 0-100%
    int uniqueVoxelsVisited;
    int uncommonMovementsExplored;

    // Activity breakdown
    float timeStationary;
    float timeMoving;
    float timeFastMovement;

    // Quality metrics
    float movementVariety;     // How diverse were the movements
    float symmetryScore;       // Left/right balance
    float verticalExploration; // Up/down range usage
    float reachExploration;    // Full arm extension usage
};

// Movement pattern types
enum class MovementPattern {
    Stationary,
    Slow,
    Moderate,
    Fast,
    Circular,
    Linear,
    Exploratory,    // Reaching into new areas
    Repetitive,     // Same pattern repeated
    Asymmetric,     // One hand much more active
    FullBody        // Head and both hands coordinated
};

// Callback for real-time analysis events
using AnalyticsCallback = std::function<void(const std::string& event, const std::string& data)>;

class MovementAnalytics {
public:
    MovementAnalytics();
    ~MovementAnalytics();

    // Session management
    void startSession();
    void endSession();
    void pauseSession();
    void resumeSession();
    bool isRecording() const { return m_isRecording; }

    // Data recording (call every frame)
    void recordSample(const Transform& head,
                      const ControllerState& leftController,
                      const ControllerState& rightController);

    // Real-time analysis
    MovementPattern getCurrentPattern() const;
    float getCurrentIntensity() const;  // 0-1 activity level
    Vec3 getSuggestedExplorationDirection() const;  // Where to move next

    // Movement space coverage
    float getMovementSpaceCoverage() const;
    std::vector<Vec3> getUnexploredAreas() const;
    std::vector<Vec3> getFrequentAreas() const;
    bool isInUncommonPosition() const;

    // Statistics
    const SessionSummary& getSessionSummary() const { return m_sessionSummary; }
    PointStatistics getRealtimeStats(int trackedPoint) const;  // 0=head, 1=left, 2=right

    // Historical data
    const std::deque<MovementSample>& getRecentSamples(int count = 1000) const;
    std::vector<Vec3> getMovementTrail(int trackedPoint, float duration) const;

    // Visualization data generation
    struct HeatmapData {
        std::vector<Vec3> positions;
        std::vector<float> intensities;
        Vec3 boundsMin;
        Vec3 boundsMax;
    };
    HeatmapData generateHeatmap(int trackedPoint, float resolution = 0.1f) const;

    // Export
    bool exportSession(const std::string& filepath) const;
    bool exportToCSV(const std::string& filepath) const;

    // Callbacks
    void setAnalyticsCallback(AnalyticsCallback callback) { m_callback = callback; }

    // Configuration
    void setMovementSpaceResolution(float meters) { m_voxelSize = meters; }
    void setRecordingRate(int samplesPerSecond) { m_targetSampleRate = samplesPerSecond; }

private:
    void updateStatistics(const MovementSample& sample);
    void updateMovementSpace(const MovementSample& sample);
    void detectPatterns();
    void calculateDerivedValues(MovementSample& sample);
    Vec3 worldToHeadSpace(const Vec3& worldPos, const Transform& head) const;
    int positionToVoxelIndex(const Vec3& relativePos) const;
    void initializeMovementSpace();

    bool m_isRecording = false;
    bool m_isPaused = false;
    double m_sessionStartTime = 0;
    double m_lastSampleTime = 0;
    int m_targetSampleRate = 90;

    // Movement samples (ring buffer behavior)
    std::deque<MovementSample> m_samples;
    static constexpr size_t MAX_SAMPLES = 90 * 60 * 30;  // 30 minutes at 90Hz

    // Movement space voxels (3D grid in head-relative space)
    // Covers a sphere around the user's head
    float m_voxelSize = 0.1f;  // 10cm resolution
    static constexpr float SPACE_RADIUS = 1.5f;  // 1.5m reach sphere
    std::vector<MovementVoxel> m_movementSpace;
    int m_voxelGridSize = 0;

    // Running statistics
    SessionSummary m_sessionSummary;
    MovementPattern m_currentPattern = MovementPattern::Stationary;

    // Recent samples for smoothing/analysis
    std::array<MovementSample, 30> m_recentBuffer;
    int m_recentBufferIndex = 0;

    // Callbacks
    AnalyticsCallback m_callback;

    // Previous sample for velocity calculation
    MovementSample m_previousSample;
    bool m_hasPreviousSample = false;
};

/**
 * MovementSpaceVisualizer - Generates visualization data for movement patterns
 */
class MovementSpaceVisualizer {
public:
    MovementSpaceVisualizer(MovementAnalytics* analytics);

    // Trail visualization
    struct TrailSegment {
        Vec3 start;
        Vec3 end;
        Color color;
        float width;
    };
    std::vector<TrailSegment> generateTrail(int trackedPoint,
                                            float duration,
                                            bool colorBySpeed = true) const;

    // Sphere coverage visualization (like a "movement fingerprint")
    struct CoveragePoint {
        Vec3 direction;  // Normalized direction from center
        float coverage;  // 0-1 how much this direction has been explored
        Color color;
    };
    std::vector<CoveragePoint> generateCoverageSphere(int segments = 32) const;

    // Exploration suggestions
    struct ExplorationTarget {
        Vec3 position;       // Target position in head-space
        float priority;      // How important to explore (0-1)
        std::string hint;    // Text hint like "Reach up and back"
        Color guideColor;
    };
    std::vector<ExplorationTarget> getExplorationTargets(int maxTargets = 5) const;

    // Movement history playback data
    struct PlaybackFrame {
        double timestamp;
        Transform head;
        Transform leftHand;
        Transform rightHand;
    };
    std::vector<PlaybackFrame> getPlaybackData(double startTime, double endTime) const;

private:
    MovementAnalytics* m_analytics;
};

} // namespace lst
