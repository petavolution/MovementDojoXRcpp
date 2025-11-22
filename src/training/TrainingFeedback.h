/**
 * TrainingFeedback.h - Training UX Feedback System
 *
 * Provides user feedback during training sequences:
 * - Phase/wave transition messages
 * - Performance ratings (star system)
 * - Wave summaries
 * - Sequence completion summary
 *
 * Can output to:
 * - Log system (always)
 * - In-world 3D text (when HUD system available)
 * - Console/stdout (debug mode)
 */

#pragma once

#include "TrainingSequence.h"
#include "WaveSpawner.h"
#include "../core/Logger.h"
#include <functional>
#include <string>
#include <vector>

namespace lst {

#define LOG_TAG_FEEDBACK "Feedback"

// =============================================================================
// Wave Rating System
// =============================================================================

/**
 * Star rating for wave performance.
 */
enum class WaveRating {
    NONE = 0,       // Wave not completed
    BRONZE = 1,     // Completed with many hits
    SILVER = 2,     // Completed with few hits
    GOLD = 3        // Completed with no hits (perfect)
};

inline const char* waveRatingToString(WaveRating rating) {
    switch (rating) {
        case WaveRating::GOLD:   return "GOLD ★★★";
        case WaveRating::SILVER: return "SILVER ★★☆";
        case WaveRating::BRONZE: return "BRONZE ★☆☆";
        case WaveRating::NONE:   return "NONE ☆☆☆";
        default:                 return "???";
    }
}

inline const char* waveRatingToStars(WaveRating rating) {
    switch (rating) {
        case WaveRating::GOLD:   return "★★★";
        case WaveRating::SILVER: return "★★☆";
        case WaveRating::BRONZE: return "★☆☆";
        case WaveRating::NONE:   return "☆☆☆";
        default:                 return "???";
    }
}

/**
 * Configuration for wave rating thresholds.
 */
struct WaveRatingConfig {
    int goldMaxHits = 0;        // Max hits taken for gold (perfect)
    int silverMaxHits = 2;      // Max hits taken for silver
    // Anything above silverMaxHits = bronze (if completed)
};

/**
 * Calculate wave rating based on metrics.
 */
inline WaveRating calculateWaveRating(const WaveMetrics& metrics, const WaveRatingConfig& config = {}) {
    if (metrics.hitsTaken <= config.goldMaxHits) {
        return WaveRating::GOLD;
    } else if (metrics.hitsTaken <= config.silverMaxHits) {
        return WaveRating::SILVER;
    } else {
        return WaveRating::BRONZE;
    }
}

// =============================================================================
// Sequence Summary
// =============================================================================

/**
 * Summary of entire sequence performance.
 */
struct SequenceSummary {
    std::string sequenceId;
    std::string sequenceName;

    // Phase tracking
    int totalPhases = 0;
    int phasesCompleted = 0;

    // Wave tracking
    int totalWaves = 0;
    int wavesCompleted = 0;
    int wavesFailed = 0;

    // Ratings
    int goldWaves = 0;
    int silverWaves = 0;
    int bronzeWaves = 0;
    int totalStars = 0;         // Sum of all wave ratings (1-3 per wave)
    int maxPossibleStars = 0;   // Max possible stars (3 per wave)

    // Combat stats
    int totalEnemiesKilled = 0;
    int totalProjectilesBlocked = 0;
    int totalHitsTaken = 0;
    float totalScore = 0.0f;

    // Timing
    float totalDuration = 0.0f;

    // Calculated
    float starPercentage() const {
        return maxPossibleStars > 0 ? (float)totalStars / maxPossibleStars * 100.0f : 0.0f;
    }

    float completionPercentage() const {
        return totalWaves > 0 ? (float)wavesCompleted / totalWaves * 100.0f : 0.0f;
    }
};

// =============================================================================
// Feedback Callbacks
// =============================================================================

// Called when feedback text should be displayed to user
using OnFeedbackTextCallback = std::function<void(const std::string& text, float duration, bool important)>;

// =============================================================================
// Training Feedback System
// =============================================================================

/**
 * TrainingFeedback - Manages user feedback during training.
 *
 * Usage:
 *   TrainingFeedback feedback;
 *   feedback.setVerboseMode(true);  // For debug
 *   feedback.onPhaseStart(phase);
 *   feedback.onWaveStart(wave, waveNum, totalWaves);
 *   feedback.onWaveEnd(wave, metrics, result);
 *   auto summary = feedback.getSequenceSummary();
 */
class TrainingFeedback {
public:
    TrainingFeedback() = default;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * Enable verbose mode (extra logging to console).
     */
    void setVerboseMode(bool enabled) { m_verbose = enabled; }

    /**
     * Set rating thresholds.
     */
    void setRatingConfig(const WaveRatingConfig& config) { m_ratingConfig = config; }

    /**
     * Set callback for displaying feedback text.
     */
    void setOnFeedbackText(OnFeedbackTextCallback cb) { m_onFeedbackText = std::move(cb); }

    // -------------------------------------------------------------------------
    // Sequence Events
    // -------------------------------------------------------------------------

    /**
     * Called when sequence starts.
     */
    void onSequenceStart(const TrainingSequenceConfig& sequence) {
        m_summary = SequenceSummary{};
        m_summary.sequenceId = sequence.id;
        m_summary.sequenceName = sequence.name;
        m_summary.totalPhases = sequence.getPhaseCount();
        m_summary.totalWaves = sequence.getTotalWaveCount();
        m_summary.maxPossibleStars = m_summary.totalWaves * 3;

        LOG_INFO(LOG_TAG_FEEDBACK) << "╔════════════════════════════════════════════════════════════╗";
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  TRAINING SEQUENCE: " << sequence.name;
        LOG_INFO(LOG_TAG_FEEDBACK) << "╠════════════════════════════════════════════════════════════╣";
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  " << sequence.description;
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  Phases: " << sequence.getPhaseCount()
                                    << "  |  Waves: " << sequence.getTotalWaveCount();
        LOG_INFO(LOG_TAG_FEEDBACK) << "╚════════════════════════════════════════════════════════════╝";

        showFeedback("Starting: " + sequence.name, 3.0f, true);
    }

    /**
     * Called when sequence ends.
     */
    void onSequenceEnd(const TrainingSequenceConfig& sequence, bool completed, float duration) {
        (void)sequence;
        m_summary.totalDuration = duration;

        LOG_INFO(LOG_TAG_FEEDBACK) << "";
        LOG_INFO(LOG_TAG_FEEDBACK) << "╔════════════════════════════════════════════════════════════╗";
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  SEQUENCE " << (completed ? "COMPLETE!" : "ENDED");
        LOG_INFO(LOG_TAG_FEEDBACK) << "╠════════════════════════════════════════════════════════════╣";
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  Duration: " << formatTime(duration);
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  Phases: " << m_summary.phasesCompleted << "/" << m_summary.totalPhases;
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  Waves: " << m_summary.wavesCompleted << "/" << m_summary.totalWaves
                                    << " (" << m_summary.wavesFailed << " failed)";
        LOG_INFO(LOG_TAG_FEEDBACK) << "╠════════════════════════════════════════════════════════════╣";
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  RATINGS: " << m_summary.goldWaves << " Gold, "
                                    << m_summary.silverWaves << " Silver, "
                                    << m_summary.bronzeWaves << " Bronze";
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  Stars: " << m_summary.totalStars << "/" << m_summary.maxPossibleStars
                                    << " (" << static_cast<int>(m_summary.starPercentage()) << "%)";
        LOG_INFO(LOG_TAG_FEEDBACK) << "╠════════════════════════════════════════════════════════════╣";
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  Enemies killed: " << m_summary.totalEnemiesKilled;
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  Projectiles blocked: " << m_summary.totalProjectilesBlocked;
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  Hits taken: " << m_summary.totalHitsTaken;
        LOG_INFO(LOG_TAG_FEEDBACK) << "║  Total score: " << static_cast<int>(m_summary.totalScore);
        LOG_INFO(LOG_TAG_FEEDBACK) << "╚════════════════════════════════════════════════════════════╝";

        std::string summaryText = completed ? "Training Complete!" : "Training Ended";
        summaryText += "\nStars: " + std::to_string(m_summary.totalStars) + "/" + std::to_string(m_summary.maxPossibleStars);
        showFeedback(summaryText, 5.0f, true);
    }

    // -------------------------------------------------------------------------
    // Phase Events
    // -------------------------------------------------------------------------

    /**
     * Called when phase starts.
     */
    void onPhaseStart(const TrainingPhaseConfig& phase, int phaseNum, int totalPhases) {
        m_currentPhase = phaseNum;
        m_totalPhases = totalPhases;

        LOG_INFO(LOG_TAG_FEEDBACK) << "";
        LOG_INFO(LOG_TAG_FEEDBACK) << "┌────────────────────────────────────────┐";
        LOG_INFO(LOG_TAG_FEEDBACK) << "│  PHASE " << phaseNum << "/" << totalPhases << ": " << phase.name;
        LOG_INFO(LOG_TAG_FEEDBACK) << "├────────────────────────────────────────┤";
        LOG_INFO(LOG_TAG_FEEDBACK) << "│  Type: " << phaseTypeToString(phase.type);
        LOG_INFO(LOG_TAG_FEEDBACK) << "│  Waves: " << phase.getWaveCount();
        if (!phase.learningGoal.empty()) {
            LOG_INFO(LOG_TAG_FEEDBACK) << "│  Goal: " << phase.learningGoal;
        }
        LOG_INFO(LOG_TAG_FEEDBACK) << "└────────────────────────────────────────┘";

        // Show hints
        for (size_t i = 0; i < phase.hints.size(); i++) {
            LOG_INFO(LOG_TAG_FEEDBACK) << "  💡 Hint " << (i+1) << ": " << phase.hints[i];
        }

        std::string text = "Phase " + std::to_string(phaseNum) + ": " + phase.name;
        if (!phase.learningGoal.empty()) {
            text += "\n" + phase.learningGoal;
        }
        showFeedback(text, 4.0f, true);
    }

    /**
     * Called when phase ends.
     */
    void onPhaseEnd(const TrainingPhaseConfig& phase, float duration, int wavesCompleted, int wavesFailed) {
        m_summary.phasesCompleted++;

        LOG_INFO(LOG_TAG_FEEDBACK) << "  Phase \"" << phase.name << "\" complete";
        LOG_INFO(LOG_TAG_FEEDBACK) << "    Duration: " << formatTime(duration);
        LOG_INFO(LOG_TAG_FEEDBACK) << "    Waves: " << wavesCompleted << " completed, " << wavesFailed << " failed";

        std::string text = phase.name + " Complete!";
        showFeedback(text, 2.0f, false);
    }

    // -------------------------------------------------------------------------
    // Wave Events
    // -------------------------------------------------------------------------

    /**
     * Called when wave starts.
     */
    void onWaveStart(const TrainingWaveConfig& wave, int waveNum, int totalWaves) {
        m_currentWave = waveNum;
        m_totalWaves = totalWaves;

        LOG_INFO(LOG_TAG_FEEDBACK) << "  ▶ Wave " << waveNum << "/" << totalWaves << ": " << wave.name;
        LOG_INFO(LOG_TAG_FEEDBACK) << "    Enemies: " << wave.getTotalEnemyCount();

        std::string text = "Wave " + std::to_string(waveNum) + "/" + std::to_string(totalWaves);
        if (!wave.name.empty() && wave.name != "unnamed") {
            text += ": " + wave.name;
        }
        showFeedback(text, 2.0f, false);
    }

    /**
     * Called when wave ends.
     */
    void onWaveEnd(const TrainingWaveConfig& wave, const WaveMetrics& metrics, bool success) {
        WaveRating rating = WaveRating::NONE;

        if (success) {
            rating = calculateWaveRating(metrics, m_ratingConfig);
            m_summary.wavesCompleted++;
            m_summary.totalStars += static_cast<int>(rating);

            switch (rating) {
                case WaveRating::GOLD:   m_summary.goldWaves++; break;
                case WaveRating::SILVER: m_summary.silverWaves++; break;
                case WaveRating::BRONZE: m_summary.bronzeWaves++; break;
                default: break;
            }
        } else {
            m_summary.wavesFailed++;
        }

        // Accumulate stats
        m_summary.totalEnemiesKilled += metrics.enemiesKilled;
        m_summary.totalProjectilesBlocked += metrics.projectilesBlocked;
        m_summary.totalHitsTaken += metrics.hitsTaken;
        m_summary.totalScore += metrics.score;

        LOG_INFO(LOG_TAG_FEEDBACK) << "  ✓ Wave \"" << wave.name << "\" " << (success ? "CLEARED" : "FAILED");
        LOG_INFO(LOG_TAG_FEEDBACK) << "    Rating: " << waveRatingToString(rating);
        LOG_INFO(LOG_TAG_FEEDBACK) << "    Kills: " << metrics.enemiesKilled << "/" << metrics.enemiesSpawned;
        LOG_INFO(LOG_TAG_FEEDBACK) << "    Blocked: " << metrics.projectilesBlocked
                                    << " | Hits: " << metrics.hitsTaken;
        LOG_INFO(LOG_TAG_FEEDBACK) << "    Score: +" << static_cast<int>(metrics.score);

        std::string text;
        if (success) {
            text = "Wave Cleared! " + std::string(waveRatingToStars(rating));
            text += "\nKills: " + std::to_string(metrics.enemiesKilled);
            text += " | Hits: " + std::to_string(metrics.hitsTaken);
        } else {
            text = "Wave Failed!";
        }
        showFeedback(text, 3.0f, success);
    }

    // -------------------------------------------------------------------------
    // Debug Events
    // -------------------------------------------------------------------------

    /**
     * Log debug action (skip, restart, etc.)
     */
    void onDebugAction(const std::string& action) {
        LOG_WARN(LOG_TAG_FEEDBACK) << "🔧 DEBUG: " << action;
        showFeedback("[DEBUG] " + action, 2.0f, false);
    }

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    const SequenceSummary& getSequenceSummary() const { return m_summary; }

    int getCurrentPhase() const { return m_currentPhase; }
    int getTotalPhases() const { return m_totalPhases; }
    int getCurrentWave() const { return m_currentWave; }
    int getTotalWavesInPhase() const { return m_totalWaves; }

private:
    void showFeedback(const std::string& text, float duration, bool important) {
        if (m_onFeedbackText) {
            m_onFeedbackText(text, duration, important);
        }

        if (m_verbose) {
            std::cout << "\n[FEEDBACK] " << text << "\n" << std::endl;
        }
    }

    std::string formatTime(float seconds) const {
        int mins = static_cast<int>(seconds) / 60;
        int secs = static_cast<int>(seconds) % 60;
        return std::to_string(mins) + "m " + std::to_string(secs) + "s";
    }

private:
    bool m_verbose = false;
    WaveRatingConfig m_ratingConfig;
    SequenceSummary m_summary;
    OnFeedbackTextCallback m_onFeedbackText;

    // Current state for display
    int m_currentPhase = 0;
    int m_totalPhases = 0;
    int m_currentWave = 0;
    int m_totalWaves = 0;
};

} // namespace lst
