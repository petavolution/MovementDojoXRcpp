#pragma once
/**
 * TrainingSequence.h - Data-Driven Training Sequence Definitions
 *
 * Provides the data structures for defining training sequences:
 * - TrainingSequenceConfig: Top-level container for a full training program
 * - TrainingPhaseConfig: A named phase with a learning goal and waves
 * - TrainingWaveConfig: A concrete encounter definition
 *
 * Design principles:
 * - Data-driven: Sequences defined as data, not hardcoded logic
 * - Extensible: New enemy types/conditions can be added without changing core
 * - Serialization-ready: Structured for future JSON/file loading
 */

#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace lst {

// =============================================================================
// Enemy Types (Generic, Mechanics-Agnostic)
// =============================================================================

/**
 * Generic enemy types for training encounters.
 * Mapped to concrete entity classes by the spawner system.
 */
enum class EnemyType {
    FLYING_DRONE,       // Airborne projectile-firing enemy
    MELEE_BOT,          // Close-range attacking enemy
    RANGED_TROOP,       // Ground-based ranged enemy
    BOSS_DRONE,         // Larger, tougher flying enemy
    TRAINING_DUMMY      // Non-hostile target for practice
};

/**
 * Convert EnemyType to human-readable string
 */
inline const char* enemyTypeToString(EnemyType type) {
    switch (type) {
        case EnemyType::FLYING_DRONE:   return "FlyingDrone";
        case EnemyType::MELEE_BOT:      return "MeleeBot";
        case EnemyType::RANGED_TROOP:   return "RangedTroop";
        case EnemyType::BOSS_DRONE:     return "BossDrone";
        case EnemyType::TRAINING_DUMMY: return "TrainingDummy";
        default:                        return "Unknown";
    }
}

// =============================================================================
// Enemy Behavior Hints
// =============================================================================

/**
 * Behavioral hints for spawned enemies.
 * The spawner interprets these; the sequence system just specifies intent.
 */
enum class EnemyBehavior {
    STATIONARY,         // Stay in place
    PATROL,             // Move along a path
    ORBIT_PLAYER,       // Circle around the player
    APPROACH_PLAYER,    // Move toward the player
    DIVE_ATTACK,        // Perform telegraphed dive attacks
    AGGRESSIVE          // Actively pursue and attack
};

inline const char* enemyBehaviorToString(EnemyBehavior behavior) {
    switch (behavior) {
        case EnemyBehavior::STATIONARY:      return "Stationary";
        case EnemyBehavior::PATROL:          return "Patrol";
        case EnemyBehavior::ORBIT_PLAYER:    return "OrbitPlayer";
        case EnemyBehavior::APPROACH_PLAYER: return "ApproachPlayer";
        case EnemyBehavior::DIVE_ATTACK:     return "DiveAttack";
        case EnemyBehavior::AGGRESSIVE:      return "Aggressive";
        default:                             return "Unknown";
    }
}

// =============================================================================
// Wave Completion Conditions
// =============================================================================

/**
 * Conditions that determine when a wave ends.
 */
enum class WaveEndCondition {
    ALL_ENEMIES_DEFEATED,   // Wave ends when all spawned enemies are dead
    TIME_ELAPSED,           // Wave ends after specified duration
    SCORE_REACHED,          // Wave ends when score threshold is met
    PLAYER_ACTION,          // Wave ends on specific player action (e.g., press button)
    MANUAL                  // Wave must be advanced manually (for intros/summaries)
};

inline const char* waveEndConditionToString(WaveEndCondition cond) {
    switch (cond) {
        case WaveEndCondition::ALL_ENEMIES_DEFEATED: return "AllEnemiesDefeated";
        case WaveEndCondition::TIME_ELAPSED:         return "TimeElapsed";
        case WaveEndCondition::SCORE_REACHED:        return "ScoreReached";
        case WaveEndCondition::PLAYER_ACTION:        return "PlayerAction";
        case WaveEndCondition::MANUAL:               return "Manual";
        default:                                     return "Unknown";
    }
}

// =============================================================================
// Enemy Spawn Definition
// =============================================================================

/**
 * Definition for a single enemy spawn within a wave.
 */
struct EnemySpawnDef {
    EnemyType type = EnemyType::FLYING_DRONE;
    EnemyBehavior behavior = EnemyBehavior::STATIONARY;

    // Spawn configuration
    int count = 1;                      // How many of this enemy type to spawn
    float spawnDelay = 0.0f;            // Delay before spawning (seconds)
    float spawnInterval = 0.0f;         // Interval between spawns if count > 1

    // Difficulty modifiers (1.0 = default)
    float healthMultiplier = 1.0f;
    float speedMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
    float fireRateMultiplier = 1.0f;

    // Optional metadata
    std::string tag;                    // For grouping/filtering (e.g., "tutorial_drone")

    // Builder-style setters for fluent configuration
    EnemySpawnDef& withCount(int n) { count = n; return *this; }
    EnemySpawnDef& withBehavior(EnemyBehavior b) { behavior = b; return *this; }
    EnemySpawnDef& withDelay(float d) { spawnDelay = d; return *this; }
    EnemySpawnDef& withInterval(float i) { spawnInterval = i; return *this; }
    EnemySpawnDef& withHealth(float h) { healthMultiplier = h; return *this; }
    EnemySpawnDef& withSpeed(float s) { speedMultiplier = s; return *this; }
    EnemySpawnDef& withFireRate(float f) { fireRateMultiplier = f; return *this; }
    EnemySpawnDef& withTag(const std::string& t) { tag = t; return *this; }
};

// =============================================================================
// Training Wave Configuration
// =============================================================================

/**
 * Configuration for a single training wave (encounter).
 *
 * A wave represents a discrete combat encounter with:
 * - A set of enemies to spawn
 * - A condition that ends the wave
 * - Optional metadata for display/logging
 */
struct TrainingWaveConfig {
    // Identity
    int id = 0;                         // Unique wave ID within phase
    std::string name;                   // Display name (e.g., "Opening Volley")
    std::string description;            // Optional description

    // Enemy composition
    std::vector<EnemySpawnDef> spawns;  // What enemies to spawn

    // Completion condition
    WaveEndCondition endCondition = WaveEndCondition::ALL_ENEMIES_DEFEATED;
    float endValue = 0.0f;              // Duration (TIME_ELAPSED) or score (SCORE_REACHED)

    // Timing
    float preparationTime = 2.0f;       // Countdown before wave starts
    float minDuration = 0.0f;           // Minimum wave duration even if condition met
    float maxDuration = 0.0f;           // Maximum duration (0 = unlimited)

    // Difficulty metadata
    int difficultyRating = 1;           // 1-5 difficulty scale
    std::vector<std::string> tags;      // Tags for filtering/categorization

    // Helper: Get total enemy count
    int getTotalEnemyCount() const {
        int total = 0;
        for (const auto& spawn : spawns) {
            total += spawn.count;
        }
        return total;
    }

    // Helper: Check if this is an "empty" wave (no enemies, for intros/summaries)
    bool isEmpty() const {
        return spawns.empty() || getTotalEnemyCount() == 0;
    }

    // Builder-style setters
    TrainingWaveConfig& withName(const std::string& n) { name = n; return *this; }
    TrainingWaveConfig& withDescription(const std::string& d) { description = d; return *this; }
    TrainingWaveConfig& withEndCondition(WaveEndCondition c, float v = 0.0f) {
        endCondition = c;
        endValue = v;
        return *this;
    }
    TrainingWaveConfig& withPrepTime(float t) { preparationTime = t; return *this; }
    TrainingWaveConfig& withMinDuration(float t) { minDuration = t; return *this; }
    TrainingWaveConfig& withMaxDuration(float t) { maxDuration = t; return *this; }
    TrainingWaveConfig& addSpawn(const EnemySpawnDef& spawn) {
        spawns.push_back(spawn);
        return *this;
    }
};

// =============================================================================
// Training Phase Configuration
// =============================================================================

/**
 * Type of training phase for categorization and UI treatment.
 */
enum class PhaseType {
    INTRO,              // Introductory phase (no combat, tutorial text)
    DRILL,              // Focused skill practice (one mechanic)
    MIXED,              // Combined mechanics
    CHALLENGE,          // Timed/scored challenge
    BOSS,               // Boss encounter
    SUMMARY             // Post-training summary (no combat)
};

inline const char* phaseTypeToString(PhaseType type) {
    switch (type) {
        case PhaseType::INTRO:     return "Intro";
        case PhaseType::DRILL:     return "Drill";
        case PhaseType::MIXED:     return "Mixed";
        case PhaseType::CHALLENGE: return "Challenge";
        case PhaseType::BOSS:      return "Boss";
        case PhaseType::SUMMARY:   return "Summary";
        default:                   return "Unknown";
    }
}

/**
 * Configuration for a training phase.
 *
 * A phase represents a logical section of training with:
 * - A specific learning goal
 * - One or more waves
 * - Tutorial/hint text
 */
struct TrainingPhaseConfig {
    // Identity
    int id = 0;                         // Unique phase ID within sequence
    std::string name;                   // Display name (e.g., "Saber Basics")
    std::string description;            // Description of the phase
    PhaseType type = PhaseType::DRILL;  // Type of phase

    // Learning content
    std::string learningGoal;           // What the player should learn
    std::string introPrompt;            // Text shown at phase start
    std::string outroPrompt;            // Text shown at phase end
    std::vector<std::string> hints;     // Tutorial hints shown during phase

    // Waves
    std::vector<TrainingWaveConfig> waves;

    // Timing
    float introDuration = 3.0f;         // How long to show intro prompt
    float outroDuration = 2.0f;         // How long to show outro prompt

    // Scoring
    bool trackScore = true;             // Whether to track score for this phase
    float scoreWeight = 1.0f;           // Weight in overall sequence score

    // Helpers
    int getWaveCount() const { return static_cast<int>(waves.size()); }
    bool hasWaves() const { return !waves.empty(); }

    int getTotalEnemyCount() const {
        int total = 0;
        for (const auto& wave : waves) {
            total += wave.getTotalEnemyCount();
        }
        return total;
    }

    // Builder-style setters
    TrainingPhaseConfig& withName(const std::string& n) { name = n; return *this; }
    TrainingPhaseConfig& withType(PhaseType t) { type = t; return *this; }
    TrainingPhaseConfig& withLearningGoal(const std::string& g) { learningGoal = g; return *this; }
    TrainingPhaseConfig& withIntro(const std::string& text, float duration = 3.0f) {
        introPrompt = text;
        introDuration = duration;
        return *this;
    }
    TrainingPhaseConfig& withOutro(const std::string& text, float duration = 2.0f) {
        outroPrompt = text;
        outroDuration = duration;
        return *this;
    }
    TrainingPhaseConfig& addHint(const std::string& hint) {
        hints.push_back(hint);
        return *this;
    }
    TrainingPhaseConfig& addWave(const TrainingWaveConfig& wave) {
        TrainingWaveConfig w = wave;
        w.id = static_cast<int>(waves.size());
        waves.push_back(w);
        return *this;
    }
};

// =============================================================================
// Training Sequence Configuration
// =============================================================================

/**
 * Difficulty level for the entire sequence.
 */
enum class SequenceDifficulty {
    TUTORIAL,           // Very easy, lots of guidance
    BEGINNER,           // Easy, forgiving
    INTERMEDIATE,       // Moderate challenge
    ADVANCED,           // Hard, requires skill
    EXPERT              // Very hard, for experienced players
};

inline const char* sequenceDifficultyToString(SequenceDifficulty diff) {
    switch (diff) {
        case SequenceDifficulty::TUTORIAL:     return "Tutorial";
        case SequenceDifficulty::BEGINNER:     return "Beginner";
        case SequenceDifficulty::INTERMEDIATE: return "Intermediate";
        case SequenceDifficulty::ADVANCED:     return "Advanced";
        case SequenceDifficulty::EXPERT:       return "Expert";
        default:                               return "Unknown";
    }
}

/**
 * Configuration for a complete training sequence.
 *
 * A sequence represents a full training program (e.g., "Level 1") with:
 * - Multiple phases
 * - Global metadata
 * - Completion criteria
 */
struct TrainingSequenceConfig {
    // Identity
    std::string id;                     // Unique identifier (e.g., "level1_fundamentals")
    std::string name;                   // Display name (e.g., "Level 1 - Fundamentals")
    std::string description;            // Full description
    int version = 1;                    // Config version for compatibility

    // Content
    std::vector<TrainingPhaseConfig> phases;

    // Metadata
    SequenceDifficulty difficulty = SequenceDifficulty::BEGINNER;
    float estimatedDuration = 0.0f;     // Estimated time in seconds (0 = auto-calculate)
    std::string recommendedEnvironment; // Suggested environment (e.g., "dojo")

    // Requirements (for future use)
    int requiredLevel = 0;              // Player level required
    std::vector<std::string> prerequisites;  // Sequences that must be completed first

    // Completion criteria
    float minimumScoreToPass = 0.0f;    // Minimum score percentage (0-100)
    bool allowRetry = true;             // Can player retry failed waves?
    int maxRetries = 3;                 // Max retries per wave (0 = unlimited)

    // Helpers
    int getPhaseCount() const { return static_cast<int>(phases.size()); }
    bool hasPhases() const { return !phases.empty(); }

    int getTotalWaveCount() const {
        int total = 0;
        for (const auto& phase : phases) {
            total += phase.getWaveCount();
        }
        return total;
    }

    int getTotalEnemyCount() const {
        int total = 0;
        for (const auto& phase : phases) {
            total += phase.getTotalEnemyCount();
        }
        return total;
    }

    float getEstimatedDuration() const {
        if (estimatedDuration > 0) return estimatedDuration;
        // Auto-calculate from phases/waves
        float total = 0;
        for (const auto& phase : phases) {
            total += phase.introDuration + phase.outroDuration;
            for (const auto& wave : phase.waves) {
                total += wave.preparationTime;
                if (wave.endCondition == WaveEndCondition::TIME_ELAPSED) {
                    total += wave.endValue;
                } else {
                    total += 30.0f; // Estimate 30s per combat wave
                }
            }
        }
        return total;
    }

    // Builder-style setters
    TrainingSequenceConfig& withId(const std::string& i) { id = i; return *this; }
    TrainingSequenceConfig& withName(const std::string& n) { name = n; return *this; }
    TrainingSequenceConfig& withDescription(const std::string& d) { description = d; return *this; }
    TrainingSequenceConfig& withDifficulty(SequenceDifficulty d) { difficulty = d; return *this; }
    TrainingSequenceConfig& withEnvironment(const std::string& e) { recommendedEnvironment = e; return *this; }
    TrainingSequenceConfig& withMinScore(float s) { minimumScoreToPass = s; return *this; }
    TrainingSequenceConfig& addPhase(const TrainingPhaseConfig& phase) {
        TrainingPhaseConfig p = phase;
        p.id = static_cast<int>(phases.size());
        phases.push_back(p);
        return *this;
    }
};

// =============================================================================
// Wave Result (Runtime)
// =============================================================================

/**
 * Result of a completed wave.
 */
enum class WaveResultType {
    COMPLETED,          // Wave finished successfully
    FAILED,             // Player failed the wave
    SKIPPED,            // Wave was skipped
    TIMED_OUT           // Wave exceeded max duration
};

inline const char* waveResultTypeToString(WaveResultType result) {
    switch (result) {
        case WaveResultType::COMPLETED: return "Completed";
        case WaveResultType::FAILED:    return "Failed";
        case WaveResultType::SKIPPED:   return "Skipped";
        case WaveResultType::TIMED_OUT: return "TimedOut";
        default:                        return "Unknown";
    }
}

/**
 * Runtime result data for a completed wave.
 */
struct WaveResult {
    WaveResultType type = WaveResultType::COMPLETED;
    float duration = 0.0f;              // How long the wave took
    int enemiesDefeated = 0;            // How many enemies were killed
    int enemiesSpawned = 0;             // How many enemies were spawned
    float score = 0.0f;                 // Score earned during wave
    float accuracy = 0.0f;              // Hit accuracy (0-1)
    std::string failureReason;          // If failed, why
};

/**
 * Runtime result data for a completed phase.
 */
struct PhaseResult {
    int phaseId = 0;
    std::string phaseName;
    float duration = 0.0f;
    float score = 0.0f;
    int wavesCompleted = 0;
    int wavesFailed = 0;
    std::vector<WaveResult> waveResults;
};

/**
 * Runtime result data for a completed sequence.
 */
struct SequenceResult {
    std::string sequenceId;
    std::string sequenceName;
    float totalDuration = 0.0f;
    float totalScore = 0.0f;
    float scorePercentage = 0.0f;       // 0-100
    bool passed = false;
    int phasesCompleted = 0;
    int totalWavesCompleted = 0;
    int totalWavesFailed = 0;
    std::vector<PhaseResult> phaseResults;
};

} // namespace lst
