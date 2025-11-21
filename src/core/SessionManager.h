#pragma once

#include "Types.h"
#include "analytics/MovementAnalytics.h"
#include "training/FlowModes.h"
#include "progression/ProgressionSystem.h"
#include "overlay/OverlaySystem.h"
#include <memory>
#include <functional>
#include <string>
#include <chrono>

namespace lst {

/**
 * SessionManager - Central coordinator for the Movement Dojo
 *
 * Manages the lifecycle and integration of all major systems:
 * - MovementAnalytics: Recording all movement data
 * - FlowModes: Training mode selection and execution
 * - ProgressionSystem: XP, levels, achievements
 * - OverlaySystem: Visualization overlay
 *
 * This is the single point of integration for the main loop.
 */

// Session configuration
struct SessionConfig {
    std::string profilePath = "profile.dat";
    std::string playerName = "Player";

    // Mode selection
    enum class Mode {
        Standalone,  // Full app with scene
        Overlay      // Overlay on other VR apps
    };
    Mode mode = Mode::Standalone;

    // Training mode
    enum class TrainingMode {
        FreeExploration,
        GuidedStretch,
        BreathingSync,
        Mirror,
        Meditation,
        FlowState
    };
    TrainingMode trainingMode = TrainingMode::FreeExploration;

    // Overlay settings
    std::string overlayPreset = "standard";
    bool overlayVisible = true;

    // Data recording
    bool recordMovement = true;
    bool exportOnExit = true;
    std::string exportPath = "movement_data";

    // Visualization
    bool showTrails = true;
    bool showCoverage = true;
    bool showHUD = true;
};

// Session statistics (for UI display)
struct SessionStats {
    float duration;
    float coverageGained;
    float currentCoverage;
    int uncommonAreasFound;
    float flowTimeAchieved;
    int xpEarned;
    std::vector<std::string> achievementsUnlocked;

    // Real-time stats
    float currentFlowScore;
    bool isInUncommonPosition;
    Vec3 lastLeftHandPos;
    Vec3 lastRightHandPos;
};

// Event callbacks
struct SessionCallbacks {
    std::function<void(int level, const std::string& title)> onLevelUp;
    std::function<void(const std::string& name, const std::string& desc)> onAchievement;
    std::function<void(const std::string& name)> onChallengeComplete;
    std::function<void(float coverage)> onCoverageUpdate;
    std::function<void(bool inUncommon)> onUncommonPositionChange;
    std::function<void(float score)> onFlowScoreChange;
};

class SessionManager {
public:
    SessionManager();
    ~SessionManager();

    // Lifecycle
    bool initialize(const SessionConfig& config);
    void shutdown();

    // Session control
    void startSession();
    void endSession();
    void pauseSession();
    void resumeSession();
    bool isSessionActive() const { return m_sessionActive; }
    bool isPaused() const { return m_paused; }

    // Main update (call every frame)
    void update(double deltaTime, const ControllerState& left,
                const ControllerState& right, const Transform& headPose);

    // Mode switching
    void setTrainingMode(SessionConfig::TrainingMode mode);
    SessionConfig::TrainingMode getTrainingMode() const { return m_config.trainingMode; }
    TrainingModule* getCurrentTrainingModule();

    // Stats access
    const SessionStats& getStats() const { return m_stats; }
    const ProgressionSystem& getProgression() const { return *m_progression; }
    const MovementAnalytics& getAnalytics() const { return *m_analytics; }

    // Overlay access
    OverlaySystem& getOverlay() { return *m_overlay; }
    const OverlayRenderData& getOverlayRenderData() const;

    // Callbacks
    void setCallbacks(const SessionCallbacks& callbacks) { m_callbacks = callbacks; }

    // Data export
    bool exportMovementData(const std::string& path);
    bool exportSessionSummary(const std::string& path);

    // Profile management
    bool saveProfile();
    bool loadProfile();

private:
    void updateStats(double deltaTime);
    void checkEvents();
    void setupCallbacks();

    SessionConfig m_config;
    SessionStats m_stats;
    SessionCallbacks m_callbacks;

    // Core systems
    std::unique_ptr<MovementAnalytics> m_analytics;
    std::unique_ptr<ProgressionSystem> m_progression;
    std::unique_ptr<OverlaySystem> m_overlay;

    // Training modes (created on demand)
    std::unique_ptr<FreeExplorationMode> m_freeExploration;
    std::unique_ptr<GuidedStretchMode> m_guidedStretch;
    std::unique_ptr<BreathingSyncMode> m_breathingSync;
    std::unique_ptr<MirrorMode> m_mirror;
    std::unique_ptr<MovementMeditationMode> m_meditation;
    std::unique_ptr<FlowStateMode> m_flowState;

    // State
    bool m_initialized = false;
    bool m_sessionActive = false;
    bool m_paused = false;
    std::chrono::high_resolution_clock::time_point m_sessionStart;

    // Cached state for change detection
    float m_lastCoverage = 0;
    bool m_lastInUncommon = false;
    float m_lastFlowScore = 0;
};

} // namespace lst
