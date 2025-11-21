/**
 * SessionManager Implementation
 *
 * Central coordinator for all Movement Dojo systems.
 */

#include "SessionManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace lst {

SessionManager::SessionManager() {
    m_stats = SessionStats{};
}

SessionManager::~SessionManager() {
    if (m_sessionActive) {
        endSession();
    }
    shutdown();
}

bool SessionManager::initialize(const SessionConfig& config) {
    m_config = config;

    // Initialize MovementAnalytics
    m_analytics = std::make_unique<MovementAnalytics>();
    MovementAnalyticsConfig analyticsConfig;
    analyticsConfig.recordFullPose = true;
    analyticsConfig.voxelResolution = 0.1f;  // 10cm voxels
    analyticsConfig.historyDuration = 3600.0;  // Keep 1 hour of data
    m_analytics->initialize(analyticsConfig);

    // Initialize ProgressionSystem
    m_progression = std::make_unique<ProgressionSystem>();
    if (!m_progression->loadProfile(config.profilePath)) {
        m_progression->createNewProfile(config.playerName);
    }

    // Initialize OverlaySystem
    m_overlay = std::make_unique<OverlaySystem>();
    m_overlay->initialize(m_analytics.get());
    m_overlay->applyPreset(config.overlayPreset);
    m_overlay->setVisible(config.overlayVisible);

    // Setup progression callbacks
    setupCallbacks();

    // Create all training modes
    m_freeExploration = std::make_unique<FreeExplorationMode>(m_analytics.get());
    m_guidedStretch = std::make_unique<GuidedStretchMode>(m_analytics.get());
    m_breathingSync = std::make_unique<BreathingSyncMode>(m_analytics.get());
    m_mirror = std::make_unique<MirrorMode>(m_analytics.get());
    m_meditation = std::make_unique<MovementMeditationMode>(m_analytics.get());
    m_flowState = std::make_unique<FlowStateMode>(m_analytics.get());

    m_initialized = true;
    return true;
}

void SessionManager::shutdown() {
    if (!m_initialized) return;

    if (m_config.exportOnExit && m_analytics) {
        exportMovementData(m_config.exportPath);
    }

    // Save profile
    if (m_progression) {
        m_progression->saveProfile(m_config.profilePath);
    }

    m_overlay.reset();
    m_progression.reset();
    m_analytics.reset();

    m_freeExploration.reset();
    m_guidedStretch.reset();
    m_breathingSync.reset();
    m_mirror.reset();
    m_meditation.reset();
    m_flowState.reset();

    m_initialized = false;
}

void SessionManager::startSession() {
    if (m_sessionActive) return;

    m_sessionActive = true;
    m_paused = false;
    m_sessionStart = std::chrono::high_resolution_clock::now();

    // Reset session stats
    m_stats = SessionStats{};
    m_stats.currentCoverage = m_analytics->getMovementSpaceCoverage();
    m_lastCoverage = m_stats.currentCoverage;

    // Start analytics recording
    m_analytics->startRecording();

    // Start progression session
    m_progression->startSession();

    // Start the selected training mode
    TrainingModule* mode = getCurrentTrainingModule();
    if (mode) {
        mode->start();
    }
}

void SessionManager::endSession() {
    if (!m_sessionActive) return;

    // Calculate final session stats
    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.duration = std::chrono::duration<float>(endTime - m_sessionStart).count();
    m_stats.coverageGained = m_stats.currentCoverage - m_lastCoverage;

    // Stop analytics
    m_analytics->stopRecording();

    // End progression session
    m_progression->endSession();

    // Stop training mode
    TrainingModule* mode = getCurrentTrainingModule();
    if (mode) {
        mode->stop();
    }

    // Export session summary
    exportSessionSummary(m_config.exportPath + "_summary.json");

    m_sessionActive = false;
}

void SessionManager::pauseSession() {
    m_paused = true;
}

void SessionManager::resumeSession() {
    m_paused = false;
}

void SessionManager::update(double deltaTime, const ControllerState& left,
                             const ControllerState& right, const Transform& headPose) {
    if (!m_initialized || !m_sessionActive || m_paused) return;

    // Update analytics with new samples
    m_analytics->recordSample(deltaTime, headPose, left, right);

    // Update current training mode
    TrainingModule* mode = getCurrentTrainingModule();
    if (mode) {
        mode->update(deltaTime, left, right);
    }

    // Update progression from analytics
    m_progression->updateFromAnalytics(*m_analytics);

    // Update overlay
    m_overlay->update(deltaTime, headPose);

    // Update session stats
    updateStats(deltaTime);

    // Check for events
    checkEvents();
}

void SessionManager::setTrainingMode(SessionConfig::TrainingMode mode) {
    // Stop current mode
    TrainingModule* current = getCurrentTrainingModule();
    if (current) {
        current->stop();
    }

    m_config.trainingMode = mode;

    // Start new mode if session is active
    if (m_sessionActive) {
        TrainingModule* newMode = getCurrentTrainingModule();
        if (newMode) {
            newMode->start();
        }
    }
}

TrainingModule* SessionManager::getCurrentTrainingModule() {
    switch (m_config.trainingMode) {
        case SessionConfig::TrainingMode::FreeExploration:
            return m_freeExploration.get();
        case SessionConfig::TrainingMode::GuidedStretch:
            return m_guidedStretch.get();
        case SessionConfig::TrainingMode::BreathingSync:
            return m_breathingSync.get();
        case SessionConfig::TrainingMode::Mirror:
            return m_mirror.get();
        case SessionConfig::TrainingMode::Meditation:
            return m_meditation.get();
        case SessionConfig::TrainingMode::FlowState:
            return m_flowState.get();
    }
    return nullptr;
}

const OverlayRenderData& SessionManager::getOverlayRenderData() const {
    return m_overlay->getRenderData();
}

void SessionManager::updateStats(double deltaTime) {
    // Update timing
    auto now = std::chrono::high_resolution_clock::now();
    m_stats.duration = std::chrono::duration<float>(now - m_sessionStart).count();

    // Update coverage
    m_stats.currentCoverage = m_analytics->getMovementSpaceCoverage();
    m_stats.coverageGained = m_stats.currentCoverage - m_lastCoverage;

    // Update uncommon position status
    m_stats.isInUncommonPosition = m_analytics->isInUncommonPosition();
    if (m_stats.isInUncommonPosition) {
        m_stats.uncommonAreasFound++;
    }

    // Update flow score from FlowStateMode if active
    if (m_config.trainingMode == SessionConfig::TrainingMode::FlowState) {
        m_stats.currentFlowScore = m_flowState->getFlowScore();
        if (m_flowState->isInFlow()) {
            m_stats.flowTimeAchieved += static_cast<float>(deltaTime);
        }
    }

    // Cache last hand positions
    const auto& samples = m_analytics->getSamples();
    if (!samples.empty()) {
        m_stats.lastLeftHandPos = samples.back().leftHandPosition;
        m_stats.lastRightHandPos = samples.back().rightHandPosition;
    }
}

void SessionManager::checkEvents() {
    // Check for coverage change
    float coverage = m_stats.currentCoverage;
    if (std::abs(coverage - m_lastCoverage) > 0.5f) {
        if (m_callbacks.onCoverageUpdate) {
            m_callbacks.onCoverageUpdate(coverage);
        }
        m_lastCoverage = coverage;
    }

    // Check for uncommon position change
    bool inUncommon = m_stats.isInUncommonPosition;
    if (inUncommon != m_lastInUncommon) {
        if (m_callbacks.onUncommonPositionChange) {
            m_callbacks.onUncommonPositionChange(inUncommon);
        }
        m_lastInUncommon = inUncommon;
    }

    // Check for flow score change
    float flowScore = m_stats.currentFlowScore;
    if (std::abs(flowScore - m_lastFlowScore) > 0.1f) {
        if (m_callbacks.onFlowScoreChange) {
            m_callbacks.onFlowScoreChange(flowScore);
        }
        m_lastFlowScore = flowScore;
    }
}

void SessionManager::setupCallbacks() {
    // Wire up progression callbacks to our callbacks
    m_progression->setLevelUpCallback([this](int level, const LevelInfo& info) {
        m_stats.xpEarned = m_progression->getCurrentXP();
        if (m_callbacks.onLevelUp) {
            m_callbacks.onLevelUp(level, info.title);
        }
    });

    m_progression->setAchievementCallback([this](const Achievement& achievement) {
        m_stats.achievementsUnlocked.push_back(achievement.name);
        if (m_callbacks.onAchievement) {
            m_callbacks.onAchievement(achievement.name, achievement.description);
        }
    });

    m_progression->setChallengeCompleteCallback([this](const Challenge& challenge) {
        if (m_callbacks.onChallengeComplete) {
            m_callbacks.onChallengeComplete(challenge.name);
        }
    });
}

bool SessionManager::exportMovementData(const std::string& path) {
    if (!m_analytics) return false;

    const auto& samples = m_analytics->getSamples();
    if (samples.empty()) return false;

    // Generate timestamp for filename
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << path << "_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".json";

    std::ofstream file(ss.str());
    if (!file.is_open()) return false;

    // Write JSON header
    file << "{\n";
    file << "  \"version\": 1,\n";
    file << "  \"sampleCount\": " << samples.size() << ",\n";
    file << "  \"duration\": " << m_stats.duration << ",\n";
    file << "  \"coverage\": " << m_stats.currentCoverage << ",\n";
    file << "  \"samples\": [\n";

    // Write samples
    for (size_t i = 0; i < samples.size(); i++) {
        const auto& s = samples[i];
        file << "    {\n";
        file << "      \"t\": " << s.timestamp << ",\n";
        file << "      \"head\": [" << s.headPosition.x << "," << s.headPosition.y << "," << s.headPosition.z << "],\n";
        file << "      \"leftHand\": [" << s.leftHandPosition.x << "," << s.leftHandPosition.y << "," << s.leftHandPosition.z << "],\n";
        file << "      \"rightHand\": [" << s.rightHandPosition.x << "," << s.rightHandPosition.y << "," << s.rightHandPosition.z << "],\n";
        file << "      \"leftRel\": [" << s.leftHandRelative.x << "," << s.leftHandRelative.y << "," << s.leftHandRelative.z << "],\n";
        file << "      \"rightRel\": [" << s.rightHandRelative.x << "," << s.rightHandRelative.y << "," << s.rightHandRelative.z << "]\n";
        file << "    }" << (i < samples.size() - 1 ? "," : "") << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    return true;
}

bool SessionManager::exportSessionSummary(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    const auto& stats = m_progression->getStats();

    file << "{\n";
    file << "  \"session\": {\n";
    file << "    \"duration\": " << m_stats.duration << ",\n";
    file << "    \"coverageGained\": " << m_stats.coverageGained << ",\n";
    file << "    \"uncommonAreasFound\": " << m_stats.uncommonAreasFound << ",\n";
    file << "    \"flowTimeAchieved\": " << m_stats.flowTimeAchieved << ",\n";
    file << "    \"xpEarned\": " << m_stats.xpEarned << "\n";
    file << "  },\n";
    file << "  \"player\": {\n";
    file << "    \"level\": " << m_progression->getCurrentLevel() << ",\n";
    file << "    \"totalXP\": " << m_progression->getCurrentXP() << ",\n";
    file << "    \"totalPlayTime\": " << stats.totalPlayTime << ",\n";
    file << "    \"totalSessions\": " << stats.totalSessions << ",\n";
    file << "    \"overallCoverage\": " << stats.movementSpaceCoverage << ",\n";
    file << "    \"achievementsUnlocked\": " << stats.achievementsUnlocked << "\n";
    file << "  }\n";
    file << "}\n";

    return true;
}

bool SessionManager::saveProfile() {
    return m_progression->saveProfile(m_config.profilePath);
}

bool SessionManager::loadProfile() {
    return m_progression->loadProfile(m_config.profilePath);
}

} // namespace lst
