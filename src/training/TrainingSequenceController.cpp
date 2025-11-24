/**
 * TrainingSequenceController.cpp - Training Sequence Controller Implementation
 *
 * Implements the state machine for executing training sequences.
 */

#include "TrainingSequenceController.h"
#include "../core/Logger.h"

namespace lst {

// Log tag for training sequence system
#define LOG_TAG_TRAINING "Training"

// =============================================================================
// System Interface
// =============================================================================

bool TrainingSequenceController::onAttach(Engine* engine) {
    m_engine = engine;
    LOG_INFO(LOG_TAG_TRAINING) << "TrainingSequenceController attached";
    return true;
}

void TrainingSequenceController::onDetach() {
    if (m_state == SequenceControllerState::RUNNING) {
        LOG_WARN(LOG_TAG_TRAINING) << "Controller detached while sequence running - stopping";
        stopSequence();
    }
    m_engine = nullptr;
    LOG_INFO(LOG_TAG_TRAINING) << "TrainingSequenceController detached";
}

void TrainingSequenceController::onUpdate(const FrameContext& ctx) {
    if (m_state != SequenceControllerState::RUNNING) {
        return;
    }

    updateRunning(static_cast<float>(ctx.deltaTime));
}

// =============================================================================
// Sequence Management
// =============================================================================

bool TrainingSequenceController::loadSequence(const TrainingSequenceConfig& config) {
    if (m_state == SequenceControllerState::RUNNING) {
        LOG_ERROR(LOG_TAG_TRAINING) << "Cannot load sequence while another is running";
        return false;
    }

    // Validate sequence
    if (config.id.empty()) {
        LOG_ERROR(LOG_TAG_TRAINING) << "Cannot load sequence: empty ID";
        return false;
    }

    if (!config.hasPhases()) {
        LOG_WARN(LOG_TAG_TRAINING) << "Loading sequence with no phases: " << config.id;
    }

    m_sequence = config;
    m_state = SequenceControllerState::LOADING;

    LOG_INFO(LOG_TAG_TRAINING) << "Loaded sequence: \"" << config.name << "\" (id=" << config.id << ")";
    LOG_DEBUG(LOG_TAG_TRAINING) << "  Phases: " << config.getPhaseCount();
    LOG_DEBUG(LOG_TAG_TRAINING) << "  Total waves: " << config.getTotalWaveCount();
    LOG_DEBUG(LOG_TAG_TRAINING) << "  Difficulty: " << sequenceDifficultyToString(config.difficulty);
    LOG_DEBUG(LOG_TAG_TRAINING) << "  Est. duration: " << config.getEstimatedDuration() << "s";

    return true;
}

bool TrainingSequenceController::startSequence() {
    if (!m_sequence) {
        LOG_ERROR(LOG_TAG_TRAINING) << "Cannot start: no sequence loaded";
        return false;
    }

    if (m_state == SequenceControllerState::RUNNING) {
        LOG_ERROR(LOG_TAG_TRAINING) << "Cannot start: sequence already running";
        return false;
    }

    // Initialize state
    m_state = SequenceControllerState::RUNNING;
    m_currentPhaseIndex = -1;
    m_currentWaveIndex = -1;
    m_sequenceTimer = 0.0f;
    m_phaseTimer = 0.0f;
    m_waveTimer = 0.0f;

    // Initialize results
    m_sequenceResult = SequenceResult{};
    m_sequenceResult.sequenceId = m_sequence->id;
    m_sequenceResult.sequenceName = m_sequence->name;

    logSequenceStart();

    // Notify listeners
    if (m_onSequenceStart) {
        m_onSequenceStart(*m_sequence);
    }

    // Start first phase
    if (m_sequence->hasPhases()) {
        enterPhase(0);
    } else {
        LOG_WARN(LOG_TAG_TRAINING) << "Sequence has no phases - completing immediately";
        completeSequence(true);
    }

    return true;
}

void TrainingSequenceController::pauseSequence() {
    if (m_state != SequenceControllerState::RUNNING) {
        LOG_WARN(LOG_TAG_TRAINING) << "Cannot pause: sequence not running";
        return;
    }

    m_state = SequenceControllerState::PAUSED;
    LOG_INFO(LOG_TAG_TRAINING) << "Sequence paused at phase " << m_currentPhaseIndex
                                << ", wave " << m_currentWaveIndex;
}

void TrainingSequenceController::resumeSequence() {
    if (m_state != SequenceControllerState::PAUSED) {
        LOG_WARN(LOG_TAG_TRAINING) << "Cannot resume: sequence not paused";
        return;
    }

    m_state = SequenceControllerState::RUNNING;
    LOG_INFO(LOG_TAG_TRAINING) << "Sequence resumed";
}

void TrainingSequenceController::stopSequence() {
    if (m_state != SequenceControllerState::RUNNING &&
        m_state != SequenceControllerState::PAUSED) {
        return;
    }

    LOG_INFO(LOG_TAG_TRAINING) << "Sequence stopped by request";
    completeSequence(false);
}

void TrainingSequenceController::reset() {
    m_state = SequenceControllerState::IDLE;
    m_sequence.reset();
    m_currentPhaseIndex = -1;
    m_currentWaveIndex = -1;
    m_phaseState = PhaseState::NOT_STARTED;
    m_waveState = WaveState::NOT_STARTED;
    m_sequenceTimer = 0.0f;
    m_phaseTimer = 0.0f;
    m_waveTimer = 0.0f;
    m_sequenceResult = SequenceResult{};

    LOG_DEBUG(LOG_TAG_TRAINING) << "Controller reset to idle state";
}

// =============================================================================
// State Queries
// =============================================================================

const TrainingSequenceConfig* TrainingSequenceController::getSequenceConfig() const {
    return m_sequence ? &(*m_sequence) : nullptr;
}

const TrainingPhaseConfig* TrainingSequenceController::getCurrentPhase() const {
    if (!m_sequence || m_currentPhaseIndex < 0 ||
        m_currentPhaseIndex >= static_cast<int>(m_sequence->phases.size())) {
        return nullptr;
    }
    return &m_sequence->phases[m_currentPhaseIndex];
}

const TrainingWaveConfig* TrainingSequenceController::getCurrentWave() const {
    const auto* phase = getCurrentPhase();
    if (!phase || m_currentWaveIndex < 0 ||
        m_currentWaveIndex >= static_cast<int>(phase->waves.size())) {
        return nullptr;
    }
    return &phase->waves[m_currentWaveIndex];
}

float TrainingSequenceController::getSequenceProgress() const {
    if (!m_sequence || m_sequence->phases.empty()) return 0.0f;

    int totalWaves = m_sequence->getTotalWaveCount();
    if (totalWaves == 0) {
        // Progress by phase if no waves
        return static_cast<float>(m_currentPhaseIndex + 1) / m_sequence->phases.size();
    }

    int completedWaves = 0;
    for (int i = 0; i < m_currentPhaseIndex; i++) {
        completedWaves += m_sequence->phases[i].getWaveCount();
    }
    if (m_currentPhaseIndex >= 0 && m_currentWaveIndex >= 0) {
        completedWaves += m_currentWaveIndex;
    }

    return static_cast<float>(completedWaves) / totalWaves;
}

float TrainingSequenceController::getPhaseProgress() const {
    const auto* phase = getCurrentPhase();
    if (!phase || phase->waves.empty()) return 0.0f;

    return static_cast<float>(m_currentWaveIndex + 1) / phase->waves.size();
}

float TrainingSequenceController::getWaveProgress() const {
    const auto* wave = getCurrentWave();
    if (!wave) return 0.0f;

    if (wave->endCondition == WaveEndCondition::TIME_ELAPSED && wave->endValue > 0) {
        return std::min(1.0f, m_waveTimer / wave->endValue);
    }

    if (wave->endCondition == WaveEndCondition::ALL_ENEMIES_DEFEATED && m_waveEnemiesSpawned > 0) {
        return static_cast<float>(m_waveEnemiesDefeated) / m_waveEnemiesSpawned;
    }

    return 0.0f;
}

// =============================================================================
// Wave Control
// =============================================================================

void TrainingSequenceController::notifyWaveConditionMet() {
    if (m_waveState == WaveState::ACTIVE) {
        m_waveConditionMet = true;
        LOG_DEBUG(LOG_TAG_TRAINING) << "Wave condition met notification received";
    }
}

void TrainingSequenceController::notifyWaveFailed(const std::string& reason) {
    if (m_waveState == WaveState::ACTIVE) {
        m_currentWaveResult.failureReason = reason;
        exitWave(WaveResultType::FAILED);
    }
}

void TrainingSequenceController::addScore(float score) {
    m_waveScore += score;
    m_currentWaveResult.score = m_waveScore;
}

void TrainingSequenceController::updateAccuracy(int hits, int total) {
    m_waveHits = hits;
    m_waveTotalShots = total;
    if (total > 0) {
        m_currentWaveResult.accuracy = static_cast<float>(hits) / total;
    }
}

void TrainingSequenceController::notifyEnemyKilled() {
    if (m_waveState == WaveState::ACTIVE || m_waveState == WaveState::SPAWNING) {
        m_waveEnemiesDefeated++;
        m_currentWaveResult.enemiesDefeated = m_waveEnemiesDefeated;
        LOG_DEBUG(LOG_TAG_TRAINING) << "Enemy killed: " << m_waveEnemiesDefeated
                                     << "/" << m_waveEnemiesSpawned << " defeated";

        // Check if all enemies defeated
        if (m_waveEnemiesSpawned > 0 && m_waveEnemiesDefeated >= m_waveEnemiesSpawned) {
            LOG_INFO(LOG_TAG_TRAINING) << "All enemies defeated!";
            m_waveConditionMet = true;
        }
    }
}

void TrainingSequenceController::notifyPlayerHit(int damage) {
    (void)damage;  // Could be used for health tracking in future
    m_waveTotalShots++;  // Count as a "missed block" for accuracy
    LOG_DEBUG(LOG_TAG_TRAINING) << "Player hit! (tracked for accuracy)";
}

void TrainingSequenceController::skipCurrentWave() {
    if (m_waveState == WaveState::ACTIVE || m_waveState == WaveState::PREPARING) {
        LOG_INFO(LOG_TAG_TRAINING) << "Skipping current wave (debug)";
        exitWave(WaveResultType::SKIPPED);
    }
}

void TrainingSequenceController::skipToNextPhase() {
    if (m_state == SequenceControllerState::RUNNING) {
        LOG_INFO(LOG_TAG_TRAINING) << "Skipping to next phase (debug)";
        exitPhase();
        advanceToNextPhase();
    }
}

// =============================================================================
// Internal Update
// =============================================================================

void TrainingSequenceController::updateRunning(float deltaTime) {
    m_sequenceTimer += deltaTime;

    switch (m_phaseState) {
        case PhaseState::INTRO:
            updatePhaseIntro(deltaTime);
            break;

        case PhaseState::RUNNING_WAVES:
            updatePhaseWaves(deltaTime);
            break;

        case PhaseState::OUTRO:
            updatePhaseOutro(deltaTime);
            break;

        default:
            break;
    }
}

void TrainingSequenceController::updatePhaseIntro(float deltaTime) {
    m_stateTimer += deltaTime;
    m_phaseTimer += deltaTime;

    const auto* phase = getCurrentPhase();
    if (!phase) return;

    if (m_stateTimer >= phase->introDuration) {
        LOG_DEBUG(LOG_TAG_TRAINING) << "Phase intro complete, starting waves";

        if (phase->hasWaves()) {
            m_phaseState = PhaseState::RUNNING_WAVES;
            enterWave(0);
        } else {
            // Phase with no waves - go straight to outro
            m_phaseState = PhaseState::OUTRO;
            m_stateTimer = 0.0f;
            if (m_onPrompt && !phase->outroPrompt.empty()) {
                m_onPrompt(phase->outroPrompt, phase->outroDuration);
            }
        }
    }
}

void TrainingSequenceController::updatePhaseWaves(float deltaTime) {
    m_phaseTimer += deltaTime;
    updateWave(deltaTime);
}

void TrainingSequenceController::updatePhaseOutro(float deltaTime) {
    m_stateTimer += deltaTime;
    m_phaseTimer += deltaTime;

    const auto* phase = getCurrentPhase();
    if (!phase) return;

    if (m_stateTimer >= phase->outroDuration) {
        exitPhase();

        if (!advanceToNextPhase()) {
            completeSequence(true);
        }
    }
}

void TrainingSequenceController::updateWave(float deltaTime) {
    m_waveTimer += deltaTime;

    const auto* wave = getCurrentWave();
    if (!wave) return;

    switch (m_waveState) {
        case WaveState::PREPARING:
            if (m_waveTimer >= wave->preparationTime) {
                m_waveState = WaveState::SPAWNING;
                m_waveTimer = 0.0f;

                // Trigger spawns
                for (const auto& spawn : wave->spawns) {
                    if (m_onWaveSpawn) {
                        m_onWaveSpawn(*wave, spawn);
                    }
                    m_waveEnemiesSpawned += spawn.count;
                }
                m_currentWaveResult.enemiesSpawned = m_waveEnemiesSpawned;

                LOG_DEBUG(LOG_TAG_TRAINING) << "Wave spawning complete: "
                                             << m_waveEnemiesSpawned << " enemies";
                m_waveState = WaveState::ACTIVE;
            }
            break;

        case WaveState::ACTIVE:
            // Check end condition
            if (checkWaveEndCondition()) {
                exitWave(WaveResultType::COMPLETED);
            }

            // Check max duration timeout
            if (wave->maxDuration > 0 && m_waveTimer >= wave->maxDuration) {
                LOG_WARN(LOG_TAG_TRAINING) << "Wave timed out after " << wave->maxDuration << "s";
                exitWave(WaveResultType::TIMED_OUT);
            }
            break;

        default:
            break;
    }
}

// =============================================================================
// State Transitions
// =============================================================================

void TrainingSequenceController::enterPhase(int phaseIndex) {
    if (!m_sequence || phaseIndex < 0 ||
        phaseIndex >= static_cast<int>(m_sequence->phases.size())) {
        LOG_ERROR(LOG_TAG_TRAINING) << "Invalid phase index: " << phaseIndex;
        return;
    }

    m_currentPhaseIndex = phaseIndex;
    m_currentWaveIndex = -1;
    m_phaseState = PhaseState::INTRO;
    m_waveState = WaveState::NOT_STARTED;
    m_phaseTimer = 0.0f;
    m_stateTimer = 0.0f;

    // Initialize phase result
    m_currentPhaseResult = PhaseResult{};
    m_currentPhaseResult.phaseId = phaseIndex;
    m_currentPhaseResult.phaseName = m_sequence->phases[phaseIndex].name;

    logPhaseStart();

    const auto* phase = getCurrentPhase();
    if (m_onPhaseStart && phase) {
        m_onPhaseStart(*phase);
    }

    // Show intro prompt
    if (m_onPrompt && phase && !phase->introPrompt.empty()) {
        m_onPrompt(phase->introPrompt, phase->introDuration);
    }
}

void TrainingSequenceController::exitPhase() {
    const auto* phase = getCurrentPhase();
    if (!phase) return;

    m_currentPhaseResult.duration = m_phaseTimer;

    // Calculate phase score
    float totalScore = 0.0f;
    for (const auto& wr : m_currentPhaseResult.waveResults) {
        totalScore += wr.score;
    }
    m_currentPhaseResult.score = totalScore;

    logPhaseEnd();

    if (m_onPhaseEnd) {
        m_onPhaseEnd(*phase, m_currentPhaseResult);
    }

    // Add to sequence results
    m_sequenceResult.phaseResults.push_back(m_currentPhaseResult);
    m_sequenceResult.phasesCompleted++;

    m_phaseState = PhaseState::COMPLETED;
}

void TrainingSequenceController::enterWave(int waveIndex) {
    const auto* phase = getCurrentPhase();
    if (!phase || waveIndex < 0 ||
        waveIndex >= static_cast<int>(phase->waves.size())) {
        LOG_ERROR(LOG_TAG_TRAINING) << "Invalid wave index: " << waveIndex;
        return;
    }

    m_currentWaveIndex = waveIndex;
    m_waveState = WaveState::PREPARING;
    m_waveTimer = 0.0f;
    m_waveConditionMet = false;
    m_waveEnemiesSpawned = 0;
    m_waveEnemiesDefeated = 0;
    m_waveScore = 0.0f;
    m_waveHits = 0;
    m_waveTotalShots = 0;

    // Initialize wave result
    m_currentWaveResult = WaveResult{};

    logWaveStart();

    const auto* wave = getCurrentWave();
    if (m_onWaveStart && wave) {
        m_onWaveStart(*wave);
    }

    // Check for empty wave (no enemies) - complete immediately after prep
    if (wave && wave->isEmpty()) {
        LOG_DEBUG(LOG_TAG_TRAINING) << "Empty wave (no enemies) - will auto-complete";
        m_waveConditionMet = true;
    }
}

void TrainingSequenceController::exitWave(WaveResultType result) {
    const auto* wave = getCurrentWave();
    if (!wave) return;

    m_currentWaveResult.type = result;
    m_currentWaveResult.duration = m_waveTimer;
    m_currentWaveResult.enemiesDefeated = m_waveEnemiesDefeated;
    m_currentWaveResult.enemiesSpawned = m_waveEnemiesSpawned;
    m_currentWaveResult.score = m_waveScore;
    if (m_waveTotalShots > 0) {
        m_currentWaveResult.accuracy = static_cast<float>(m_waveHits) / m_waveTotalShots;
    }

    logWaveEnd();

    if (m_onWaveEnd) {
        m_onWaveEnd(*wave, m_currentWaveResult);
    }

    // Add to phase results
    m_currentPhaseResult.waveResults.push_back(m_currentWaveResult);
    if (result == WaveResultType::COMPLETED || result == WaveResultType::SKIPPED) {
        m_currentPhaseResult.wavesCompleted++;
        m_sequenceResult.totalWavesCompleted++;
    } else {
        m_currentPhaseResult.wavesFailed++;
        m_sequenceResult.totalWavesFailed++;
    }

    m_waveState = WaveState::COMPLETED;

    // Advance to next wave or phase
    if (!advanceToNextWave()) {
        // No more waves in this phase
        m_phaseState = PhaseState::OUTRO;
        m_stateTimer = 0.0f;

        const auto* phase = getCurrentPhase();
        if (m_onPrompt && phase && !phase->outroPrompt.empty()) {
            m_onPrompt(phase->outroPrompt, phase->outroDuration);
        }
    }
}

bool TrainingSequenceController::advanceToNextWave() {
    const auto* phase = getCurrentPhase();
    if (!phase) return false;

    int nextWave = m_currentWaveIndex + 1;
    if (nextWave >= static_cast<int>(phase->waves.size())) {
        return false;  // No more waves
    }

    enterWave(nextWave);
    return true;
}

bool TrainingSequenceController::advanceToNextPhase() {
    if (!m_sequence) return false;

    int nextPhase = m_currentPhaseIndex + 1;
    if (nextPhase >= static_cast<int>(m_sequence->phases.size())) {
        return false;  // No more phases
    }

    enterPhase(nextPhase);
    return true;
}

void TrainingSequenceController::completeSequence(bool success) {
    m_sequenceResult.totalDuration = m_sequenceTimer;
    m_sequenceResult.passed = success;

    // Calculate total score
    float totalScore = 0.0f;
    for (const auto& pr : m_sequenceResult.phaseResults) {
        totalScore += pr.score;
    }
    m_sequenceResult.totalScore = totalScore;

    // Calculate score percentage (simplified for now)
    // In a real implementation, this would consider max possible score
    m_sequenceResult.scorePercentage = m_sequenceResult.totalWavesCompleted > 0
        ? (static_cast<float>(m_sequenceResult.totalWavesCompleted) /
           (m_sequenceResult.totalWavesCompleted + m_sequenceResult.totalWavesFailed)) * 100.0f
        : 0.0f;

    logSequenceEnd();

    if (m_onSequenceEnd && m_sequence) {
        m_onSequenceEnd(*m_sequence, m_sequenceResult);
    }

    m_state = success ? SequenceControllerState::COMPLETED : SequenceControllerState::FAILED;
}

// =============================================================================
// Wave Condition Checking
// =============================================================================

bool TrainingSequenceController::checkWaveEndCondition() {
    const auto* wave = getCurrentWave();
    if (!wave) return false;

    // Check minimum duration first
    if (wave->minDuration > 0 && m_waveTimer < wave->minDuration) {
        return false;
    }

    switch (wave->endCondition) {
        case WaveEndCondition::ALL_ENEMIES_DEFEATED:
            // External notification sets m_waveConditionMet, or check our counter
            return m_waveConditionMet ||
                   (m_waveEnemiesSpawned > 0 && m_waveEnemiesDefeated >= m_waveEnemiesSpawned);

        case WaveEndCondition::TIME_ELAPSED:
            return m_waveTimer >= wave->endValue;

        case WaveEndCondition::SCORE_REACHED:
            return m_waveScore >= wave->endValue;

        case WaveEndCondition::PLAYER_ACTION:
            // Requires external notification
            return m_waveConditionMet;

        case WaveEndCondition::MANUAL:
            // Requires external call to notifyWaveConditionMet()
            return m_waveConditionMet;

        default:
            return false;
    }
}

// =============================================================================
// Logging
// =============================================================================

void TrainingSequenceController::logSequenceStart() {
    if (!m_sequence) return;

    LOG_INFO(LOG_TAG_TRAINING) << "════════════════════════════════════════════════════════════";
    LOG_INFO(LOG_TAG_TRAINING) << "SEQUENCE START: " << m_sequence->name;
    LOG_INFO(LOG_TAG_TRAINING) << "════════════════════════════════════════════════════════════";
    LOG_INFO(LOG_TAG_TRAINING) << "  ID: " << m_sequence->id;
    LOG_INFO(LOG_TAG_TRAINING) << "  Phases: " << m_sequence->getPhaseCount();
    LOG_INFO(LOG_TAG_TRAINING) << "  Total Waves: " << m_sequence->getTotalWaveCount();
    LOG_INFO(LOG_TAG_TRAINING) << "  Difficulty: " << sequenceDifficultyToString(m_sequence->difficulty);
}

void TrainingSequenceController::logSequenceEnd() {
    LOG_INFO(LOG_TAG_TRAINING) << "════════════════════════════════════════════════════════════";
    LOG_INFO(LOG_TAG_TRAINING) << "SEQUENCE " << (m_sequenceResult.passed ? "COMPLETE" : "ENDED");
    LOG_INFO(LOG_TAG_TRAINING) << "════════════════════════════════════════════════════════════";
    LOG_INFO(LOG_TAG_TRAINING) << "  Duration: " << m_sequenceResult.totalDuration << "s";
    LOG_INFO(LOG_TAG_TRAINING) << "  Phases completed: " << m_sequenceResult.phasesCompleted;
    LOG_INFO(LOG_TAG_TRAINING) << "  Waves completed: " << m_sequenceResult.totalWavesCompleted;
    LOG_INFO(LOG_TAG_TRAINING) << "  Waves failed: " << m_sequenceResult.totalWavesFailed;
    LOG_INFO(LOG_TAG_TRAINING) << "  Score: " << m_sequenceResult.totalScore
                                << " (" << m_sequenceResult.scorePercentage << "%)";
}

void TrainingSequenceController::logPhaseStart() {
    const auto* phase = getCurrentPhase();
    if (!phase) return;

    LOG_INFO(LOG_TAG_TRAINING) << "────────────────────────────────────────";
    LOG_INFO(LOG_TAG_TRAINING) << "PHASE " << (m_currentPhaseIndex + 1) << "/" << m_sequence->getPhaseCount()
                                << ": " << phase->name;
    LOG_INFO(LOG_TAG_TRAINING) << "────────────────────────────────────────";
    LOG_INFO(LOG_TAG_TRAINING) << "  Type: " << phaseTypeToString(phase->type);
    LOG_INFO(LOG_TAG_TRAINING) << "  Waves: " << phase->getWaveCount();
    if (!phase->learningGoal.empty()) {
        LOG_INFO(LOG_TAG_TRAINING) << "  Goal: " << phase->learningGoal;
    }
}

void TrainingSequenceController::logPhaseEnd() {
    const auto* phase = getCurrentPhase();
    if (!phase) return;

    LOG_INFO(LOG_TAG_TRAINING) << "Phase \"" << phase->name << "\" complete";
    LOG_INFO(LOG_TAG_TRAINING) << "  Duration: " << m_currentPhaseResult.duration << "s";
    LOG_INFO(LOG_TAG_TRAINING) << "  Waves: " << m_currentPhaseResult.wavesCompleted << " completed, "
                                << m_currentPhaseResult.wavesFailed << " failed";
    LOG_INFO(LOG_TAG_TRAINING) << "  Score: " << m_currentPhaseResult.score;
}

void TrainingSequenceController::logWaveStart() {
    const auto* wave = getCurrentWave();
    const auto* phase = getCurrentPhase();
    if (!wave || !phase) return;

    LOG_DEBUG(LOG_TAG_TRAINING) << "Wave " << (m_currentWaveIndex + 1) << "/" << phase->getWaveCount()
                                 << ": " << (wave->name.empty() ? "(unnamed)" : wave->name);
    LOG_DEBUG(LOG_TAG_TRAINING) << "  Enemies: " << wave->getTotalEnemyCount();
    LOG_DEBUG(LOG_TAG_TRAINING) << "  End condition: " << waveEndConditionToString(wave->endCondition);
    if (wave->endCondition == WaveEndCondition::TIME_ELAPSED) {
        LOG_DEBUG(LOG_TAG_TRAINING) << "  Duration: " << wave->endValue << "s";
    }
}

void TrainingSequenceController::logWaveEnd() {
    const auto* wave = getCurrentWave();
    if (!wave) return;

    LOG_DEBUG(LOG_TAG_TRAINING) << "Wave \"" << wave->name << "\" "
                                 << waveResultTypeToString(m_currentWaveResult.type);
    LOG_DEBUG(LOG_TAG_TRAINING) << "  Duration: " << m_currentWaveResult.duration << "s";
    LOG_DEBUG(LOG_TAG_TRAINING) << "  Enemies: " << m_currentWaveResult.enemiesDefeated << "/"
                                 << m_currentWaveResult.enemiesSpawned << " defeated";
    LOG_DEBUG(LOG_TAG_TRAINING) << "  Score: " << m_currentWaveResult.score;
}

} // namespace lst
