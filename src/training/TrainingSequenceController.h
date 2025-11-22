#pragma once
/**
 * TrainingSequenceController.h - Runtime Training Sequence Execution
 *
 * Manages the execution of a TrainingSequenceConfig:
 * - Steps through phases and waves
 * - Exposes hooks for spawning/cleanup
 * - Tracks completion state and statistics
 * - Logs all important transitions
 *
 * The controller is decoupled from concrete enemy spawning; it provides
 * abstract callbacks that can be wired to any spawner system.
 */

#include "TrainingSequence.h"
#include "../core/Engine.h"
#include <functional>
#include <chrono>

namespace lst {

// =============================================================================
// Controller State
// =============================================================================

/**
 * Current state of the sequence controller.
 */
enum class SequenceControllerState {
    IDLE,               // No sequence loaded or sequence finished
    LOADING,            // Loading sequence, preparing to start
    RUNNING,            // Actively running a sequence
    PAUSED,             // Sequence paused
    COMPLETED,          // Sequence completed successfully
    FAILED              // Sequence failed
};

inline const char* controllerStateToString(SequenceControllerState state) {
    switch (state) {
        case SequenceControllerState::IDLE:      return "Idle";
        case SequenceControllerState::LOADING:   return "Loading";
        case SequenceControllerState::RUNNING:   return "Running";
        case SequenceControllerState::PAUSED:    return "Paused";
        case SequenceControllerState::COMPLETED: return "Completed";
        case SequenceControllerState::FAILED:    return "Failed";
        default:                                 return "Unknown";
    }
}

/**
 * Current state within a wave.
 */
enum class WaveState {
    NOT_STARTED,        // Wave has not begun
    PREPARING,          // Countdown before wave starts
    SPAWNING,           // Enemies are being spawned
    ACTIVE,             // Combat in progress
    ENDING,             // Wave is ending (cleanup)
    COMPLETED,          // Wave finished successfully
    FAILED              // Wave failed
};

inline const char* waveStateToString(WaveState state) {
    switch (state) {
        case WaveState::NOT_STARTED: return "NotStarted";
        case WaveState::PREPARING:   return "Preparing";
        case WaveState::SPAWNING:    return "Spawning";
        case WaveState::ACTIVE:      return "Active";
        case WaveState::ENDING:      return "Ending";
        case WaveState::COMPLETED:   return "Completed";
        case WaveState::FAILED:      return "Failed";
        default:                     return "Unknown";
    }
}

/**
 * Current state within a phase.
 */
enum class PhaseState {
    NOT_STARTED,        // Phase has not begun
    INTRO,              // Showing intro prompt
    RUNNING_WAVES,      // Executing waves
    OUTRO,              // Showing outro prompt
    COMPLETED,          // Phase finished
    FAILED              // Phase failed
};

inline const char* phaseStateToString(PhaseState state) {
    switch (state) {
        case PhaseState::NOT_STARTED:   return "NotStarted";
        case PhaseState::INTRO:         return "Intro";
        case PhaseState::RUNNING_WAVES: return "RunningWaves";
        case PhaseState::OUTRO:         return "Outro";
        case PhaseState::COMPLETED:     return "Completed";
        case PhaseState::FAILED:        return "Failed";
        default:                        return "Unknown";
    }
}

// =============================================================================
// Callback Types (Abstract Hooks)
// =============================================================================

/**
 * Callback signatures for sequence events.
 * These are called by the controller to notify external systems.
 */
using OnSequenceStartCallback = std::function<void(const TrainingSequenceConfig&)>;
using OnSequenceEndCallback = std::function<void(const TrainingSequenceConfig&, const SequenceResult&)>;
using OnPhaseStartCallback = std::function<void(const TrainingPhaseConfig&)>;
using OnPhaseEndCallback = std::function<void(const TrainingPhaseConfig&, const PhaseResult&)>;
using OnWaveStartCallback = std::function<void(const TrainingWaveConfig&)>;
using OnWaveEndCallback = std::function<void(const TrainingWaveConfig&, const WaveResult&)>;
using OnWaveSpawnCallback = std::function<void(const TrainingWaveConfig&, const EnemySpawnDef&)>;
using OnPromptCallback = std::function<void(const std::string& text, float duration)>;

// =============================================================================
// Training Sequence Controller
// =============================================================================

/**
 * TrainingSequenceController
 *
 * Manages the execution of training sequences. This is a System that can be
 * attached to the Engine and receives update calls each frame.
 *
 * Usage:
 *   auto* controller = engine.addSystem<TrainingSequenceController>();
 *   controller->loadSequence(mySequenceConfig);
 *   controller->startSequence();
 *   // Controller automatically updates via onUpdate()
 */
class TrainingSequenceController : public System {
public:
    // -------------------------------------------------------------------------
    // System Interface
    // -------------------------------------------------------------------------

    const char* getName() const override { return "TrainingSequenceController"; }

    bool onAttach(Engine* engine) override;
    void onDetach() override;
    void onUpdate(const FrameContext& ctx) override;

    // -------------------------------------------------------------------------
    // Sequence Management
    // -------------------------------------------------------------------------

    /**
     * Load a sequence configuration.
     * Does not start execution; call startSequence() to begin.
     */
    bool loadSequence(const TrainingSequenceConfig& config);

    /**
     * Start executing the loaded sequence.
     * Returns false if no sequence is loaded or already running.
     */
    bool startSequence();

    /**
     * Pause the current sequence.
     */
    void pauseSequence();

    /**
     * Resume a paused sequence.
     */
    void resumeSequence();

    /**
     * Stop the current sequence entirely.
     */
    void stopSequence();

    /**
     * Reset the controller to idle state.
     */
    void reset();

    // -------------------------------------------------------------------------
    // State Queries
    // -------------------------------------------------------------------------

    SequenceControllerState getState() const { return m_state; }
    bool isRunning() const { return m_state == SequenceControllerState::RUNNING; }
    bool isCompleted() const { return m_state == SequenceControllerState::COMPLETED; }
    bool isPaused() const { return m_state == SequenceControllerState::PAUSED; }

    // Current position in sequence
    int getCurrentPhaseIndex() const { return m_currentPhaseIndex; }
    int getCurrentWaveIndex() const { return m_currentWaveIndex; }
    PhaseState getPhaseState() const { return m_phaseState; }
    WaveState getWaveState() const { return m_waveState; }

    // Get current configs (may be null if not running)
    const TrainingSequenceConfig* getSequenceConfig() const;
    const TrainingPhaseConfig* getCurrentPhase() const;
    const TrainingWaveConfig* getCurrentWave() const;

    // Progress tracking
    float getSequenceProgress() const;  // 0.0 - 1.0
    float getPhaseProgress() const;     // 0.0 - 1.0
    float getWaveProgress() const;      // 0.0 - 1.0
    float getSequenceElapsedTime() const { return m_sequenceTimer; }
    float getPhaseElapsedTime() const { return m_phaseTimer; }
    float getWaveElapsedTime() const { return m_waveTimer; }

    // Results
    const SequenceResult& getSequenceResult() const { return m_sequenceResult; }

    // -------------------------------------------------------------------------
    // Wave Control (for external systems)
    // -------------------------------------------------------------------------

    /**
     * Notify controller that a wave condition has been met.
     * Called by combat/spawner systems when enemies are defeated, etc.
     */
    void notifyWaveConditionMet();

    /**
     * Notify controller that player has failed current wave.
     */
    void notifyWaveFailed(const std::string& reason = "");

    /**
     * Notify controller of score earned.
     */
    void addScore(float score);

    /**
     * Notify controller of accuracy update.
     */
    void updateAccuracy(int hits, int total);

    /**
     * Skip current wave (for debugging/testing).
     */
    void skipCurrentWave();

    /**
     * Skip to next phase (for debugging/testing).
     */
    void skipToNextPhase();

    // -------------------------------------------------------------------------
    // Callbacks (Register External Handlers)
    // -------------------------------------------------------------------------

    void setOnSequenceStart(OnSequenceStartCallback cb) { m_onSequenceStart = std::move(cb); }
    void setOnSequenceEnd(OnSequenceEndCallback cb) { m_onSequenceEnd = std::move(cb); }
    void setOnPhaseStart(OnPhaseStartCallback cb) { m_onPhaseStart = std::move(cb); }
    void setOnPhaseEnd(OnPhaseEndCallback cb) { m_onPhaseEnd = std::move(cb); }
    void setOnWaveStart(OnWaveStartCallback cb) { m_onWaveStart = std::move(cb); }
    void setOnWaveEnd(OnWaveEndCallback cb) { m_onWaveEnd = std::move(cb); }
    void setOnWaveSpawn(OnWaveSpawnCallback cb) { m_onWaveSpawn = std::move(cb); }
    void setOnPrompt(OnPromptCallback cb) { m_onPrompt = std::move(cb); }

private:
    // -------------------------------------------------------------------------
    // Internal State Machine
    // -------------------------------------------------------------------------

    void updateRunning(float deltaTime);
    void updatePhaseIntro(float deltaTime);
    void updatePhaseWaves(float deltaTime);
    void updatePhaseOutro(float deltaTime);
    void updateWave(float deltaTime);

    // State transitions
    void enterPhase(int phaseIndex);
    void exitPhase();
    void enterWave(int waveIndex);
    void exitWave(WaveResultType result);

    bool advanceToNextWave();
    bool advanceToNextPhase();
    void completeSequence(bool success);

    // Wave condition checking
    bool checkWaveEndCondition();

    // Logging helpers
    void logSequenceStart();
    void logSequenceEnd();
    void logPhaseStart();
    void logPhaseEnd();
    void logWaveStart();
    void logWaveEnd();

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    Engine* m_engine = nullptr;
    SequenceControllerState m_state = SequenceControllerState::IDLE;

    // Loaded sequence
    std::optional<TrainingSequenceConfig> m_sequence;

    // Current position
    int m_currentPhaseIndex = -1;
    int m_currentWaveIndex = -1;
    PhaseState m_phaseState = PhaseState::NOT_STARTED;
    WaveState m_waveState = WaveState::NOT_STARTED;

    // Timers
    float m_sequenceTimer = 0.0f;
    float m_phaseTimer = 0.0f;
    float m_waveTimer = 0.0f;
    float m_stateTimer = 0.0f;  // Timer for current sub-state (intro, outro, prep)

    // Wave tracking
    bool m_waveConditionMet = false;
    int m_waveEnemiesSpawned = 0;
    int m_waveEnemiesDefeated = 0;
    float m_waveScore = 0.0f;
    int m_waveHits = 0;
    int m_waveTotalShots = 0;

    // Results
    SequenceResult m_sequenceResult;
    PhaseResult m_currentPhaseResult;
    WaveResult m_currentWaveResult;

    // Callbacks
    OnSequenceStartCallback m_onSequenceStart;
    OnSequenceEndCallback m_onSequenceEnd;
    OnPhaseStartCallback m_onPhaseStart;
    OnPhaseEndCallback m_onPhaseEnd;
    OnWaveStartCallback m_onWaveStart;
    OnWaveEndCallback m_onWaveEnd;
    OnWaveSpawnCallback m_onWaveSpawn;
    OnPromptCallback m_onPrompt;
};

} // namespace lst
