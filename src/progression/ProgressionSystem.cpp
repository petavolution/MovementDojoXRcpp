/**
 * Progression System Implementation
 *
 * Tracks long-term progress in movement exploration with gamification elements.
 */

#include "ProgressionSystem.h"
#include <fstream>
#include <algorithm>
#include <ctime>

namespace lst {

// =============================================================================
// ProgressionSystem
// =============================================================================

ProgressionSystem::ProgressionSystem() {
    initializeAchievements();
    initializeMovementZones();
    initializeLevelSystem();
}

ProgressionSystem::~ProgressionSystem() {
    if (m_inSession) {
        endSession();
    }
}

void ProgressionSystem::initializeAchievements() {
    m_achievements = {
        // Exploration achievements
        {"first_steps", "First Steps", "Complete your first session",
         "badge_start", Achievement::Category::Exploration, false, 0, 0},
        {"explorer_10", "Curious Explorer", "Reach 10% movement space coverage",
         "badge_explore", Achievement::Category::Exploration, false, 0, 0},
        {"explorer_25", "Space Pioneer", "Reach 25% movement space coverage",
         "badge_explore_silver", Achievement::Category::Exploration, false, 0, 0},
        {"explorer_50", "Movement Adventurer", "Reach 50% movement space coverage",
         "badge_explore_gold", Achievement::Category::Exploration, false, 0, 0},
        {"explorer_75", "Territory Master", "Reach 75% movement space coverage",
         "badge_explore_platinum", Achievement::Category::Exploration, false, 0, 0},

        // Discovery achievements
        {"uncommon_1", "Off the Beaten Path", "Discover your first uncommon position",
         "badge_discover", Achievement::Category::Discovery, false, 0, 0},
        {"uncommon_10", "Hidden Corners", "Discover 10 uncommon positions",
         "badge_discover_silver", Achievement::Category::Discovery, false, 0, 0},
        {"uncommon_50", "Movement Archaeologist", "Discover 50 uncommon positions",
         "badge_discover_gold", Achievement::Category::Discovery, false, 0, 0},
        {"behind_master", "Behind You!", "Fully explore the space behind your back",
         "badge_behind", Achievement::Category::Discovery, false, 0, 0},
        {"overhead_master", "Sky Dancer", "Fully explore the overhead space",
         "badge_overhead", Achievement::Category::Discovery, false, 0, 0},
        {"floor_master", "Ground Sweeper", "Fully explore low positions near the floor",
         "badge_floor", Achievement::Category::Discovery, false, 0, 0},

        // Flow achievements
        {"flow_30", "Finding Flow", "Maintain flow state for 30 seconds",
         "badge_flow", Achievement::Category::Flow, false, 0, 0},
        {"flow_60", "In the Zone", "Maintain flow state for 1 minute",
         "badge_flow_silver", Achievement::Category::Flow, false, 0, 0},
        {"flow_300", "Flow Master", "Maintain flow state for 5 minutes",
         "badge_flow_gold", Achievement::Category::Flow, false, 0, 0},

        // Consistency achievements
        {"streak_3", "Getting Started", "Practice 3 days in a row",
         "badge_streak", Achievement::Category::Consistency, false, 0, 0},
        {"streak_7", "Weekly Warrior", "Practice 7 days in a row",
         "badge_streak_silver", Achievement::Category::Consistency, false, 0, 0},
        {"streak_30", "Dedicated Practitioner", "Practice 30 days in a row",
         "badge_streak_gold", Achievement::Category::Consistency, false, 0, 0},

        // Milestone achievements
        {"time_1h", "First Hour", "Accumulate 1 hour of practice time",
         "badge_time", Achievement::Category::Milestone, false, 0, 0},
        {"time_10h", "Committed", "Accumulate 10 hours of practice time",
         "badge_time_silver", Achievement::Category::Milestone, false, 0, 0},
        {"time_100h", "Master", "Accumulate 100 hours of practice time",
         "badge_time_gold", Achievement::Category::Milestone, false, 0, 0},
        {"sessions_10", "Regular", "Complete 10 sessions",
         "badge_sessions", Achievement::Category::Milestone, false, 0, 0},
        {"sessions_100", "Veteran", "Complete 100 sessions",
         "badge_sessions_silver", Achievement::Category::Milestone, false, 0, 0},

        // Mastery achievements
        {"symmetry_90", "Balanced Being", "Achieve 90% body symmetry score",
         "badge_symmetry", Achievement::Category::Mastery, false, 0, 0},
        {"variety_high", "Movement Polyglot", "Achieve high movement variety score",
         "badge_variety", Achievement::Category::Mastery, false, 0, 0},
    };
}

void ProgressionSystem::initializeMovementZones() {
    using namespace MovementZones;
    m_movementZones = {
        OVERHEAD, FRONT_HIGH, FRONT_MID, FRONT_LOW,
        LEFT_HIGH, LEFT_MID, RIGHT_HIGH, RIGHT_MID,
        BEHIND_HIGH, BEHIND_LOW, FLOOR_FRONT, FLOOR_SIDE
    };
}

void ProgressionSystem::initializeLevelSystem() {
    m_levelDefinitions = Levels::DEFINITIONS;
}

bool ProgressionSystem::loadProfile(const std::string& profilePath) {
    std::ifstream file(profilePath, std::ios::binary);
    if (!file.is_open()) return false;

    // Read player name
    size_t nameLen;
    file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    m_playerName.resize(nameLen);
    file.read(&m_playerName[0], nameLen);

    // Read stats
    file.read(reinterpret_cast<char*>(&m_stats), sizeof(PlayerStats));

    // Read level info
    file.read(reinterpret_cast<char*>(&m_currentLevel), sizeof(m_currentLevel));
    file.read(reinterpret_cast<char*>(&m_currentXP), sizeof(m_currentXP));

    // Read achievement unlock status
    size_t numAchievements;
    file.read(reinterpret_cast<char*>(&numAchievements), sizeof(numAchievements));
    for (size_t i = 0; i < numAchievements && i < m_achievements.size(); i++) {
        file.read(reinterpret_cast<char*>(&m_achievements[i].unlocked), sizeof(bool));
        file.read(reinterpret_cast<char*>(&m_achievements[i].unlockedTime), sizeof(double));
        file.read(reinterpret_cast<char*>(&m_achievements[i].progress), sizeof(float));
    }

    // Read movement zone data
    size_t numZones;
    file.read(reinterpret_cast<char*>(&numZones), sizeof(numZones));
    for (size_t i = 0; i < numZones && i < m_movementZones.size(); i++) {
        file.read(reinterpret_cast<char*>(&m_movementZones[i].explorationLevel), sizeof(float));
        file.read(reinterpret_cast<char*>(&m_movementZones[i].masteryLevel), sizeof(float));
        file.read(reinterpret_cast<char*>(&m_movementZones[i].visitCount), sizeof(int));
        file.read(reinterpret_cast<char*>(&m_movementZones[i].totalTime), sizeof(float));
    }

    return true;
}

bool ProgressionSystem::saveProfile(const std::string& profilePath) {
    std::ofstream file(profilePath, std::ios::binary);
    if (!file.is_open()) return false;

    // Write player name
    size_t nameLen = m_playerName.size();
    file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    file.write(m_playerName.c_str(), nameLen);

    // Write stats
    file.write(reinterpret_cast<const char*>(&m_stats), sizeof(PlayerStats));

    // Write level info
    file.write(reinterpret_cast<const char*>(&m_currentLevel), sizeof(m_currentLevel));
    file.write(reinterpret_cast<const char*>(&m_currentXP), sizeof(m_currentXP));

    // Write achievement status
    size_t numAchievements = m_achievements.size();
    file.write(reinterpret_cast<const char*>(&numAchievements), sizeof(numAchievements));
    for (const auto& achievement : m_achievements) {
        file.write(reinterpret_cast<const char*>(&achievement.unlocked), sizeof(bool));
        file.write(reinterpret_cast<const char*>(&achievement.unlockedTime), sizeof(double));
        file.write(reinterpret_cast<const char*>(&achievement.progress), sizeof(float));
    }

    // Write movement zone data
    size_t numZones = m_movementZones.size();
    file.write(reinterpret_cast<const char*>(&numZones), sizeof(numZones));
    for (const auto& zone : m_movementZones) {
        file.write(reinterpret_cast<const char*>(&zone.explorationLevel), sizeof(float));
        file.write(reinterpret_cast<const char*>(&zone.masteryLevel), sizeof(float));
        file.write(reinterpret_cast<const char*>(&zone.visitCount), sizeof(int));
        file.write(reinterpret_cast<const char*>(&zone.totalTime), sizeof(float));
    }

    return true;
}

void ProgressionSystem::createNewProfile(const std::string& playerName) {
    m_playerName = playerName;
    m_stats = PlayerStats{};
    m_currentLevel = 1;
    m_currentXP = 0;

    // Reset achievements
    for (auto& achievement : m_achievements) {
        achievement.unlocked = false;
        achievement.unlockedTime = 0;
        achievement.progress = 0;
    }

    // Reset zones
    for (auto& zone : m_movementZones) {
        zone.explorationLevel = 0;
        zone.masteryLevel = 0;
        zone.visitCount = 0;
        zone.totalTime = 0;
    }

    m_sessionHistory.clear();
    refreshDailyChallenges();
    refreshWeeklyChallenges();
}

void ProgressionSystem::startSession() {
    m_inSession = true;
    m_currentSession = SessionRecord{};
    m_currentSession.startTime = std::chrono::system_clock::now();
}

void ProgressionSystem::endSession() {
    if (!m_inSession) return;

    auto endTime = std::chrono::system_clock::now();
    m_currentSession.duration = std::chrono::duration<double>(
        endTime - m_currentSession.startTime).count();

    // Update stats
    m_stats.totalPlayTime += m_currentSession.duration;
    m_stats.totalSessions++;

    // Update session bests
    if (m_currentSession.coverageGained > m_stats.bestCoverageInSession) {
        m_stats.bestCoverageInSession = m_currentSession.coverageGained;
    }
    if (m_currentSession.flowTimeAchieved > m_stats.bestFlowDuration) {
        m_stats.bestFlowDuration = m_currentSession.flowTimeAchieved;
    }
    if (m_currentSession.uncommonAreasFound > m_stats.mostUncommonInSession) {
        m_stats.mostUncommonInSession = m_currentSession.uncommonAreasFound;
    }

    // Add session XP
    int sessionXP = static_cast<int>(m_currentSession.duration / 60.0) * 10;  // 10 XP per minute
    sessionXP += m_currentSession.uncommonAreasFound * 5;  // 5 XP per uncommon area
    sessionXP += static_cast<int>(m_currentSession.flowTimeAchieved / 10.0) * 2;  // 2 XP per 10s of flow
    addXP(sessionXP);

    // Check for first session achievement
    if (m_stats.totalSessions == 1) {
        checkAchievement("first_steps");
    }

    // Save session history
    m_sessionHistory.push_back(m_currentSession);

    // Check all achievements
    checkAllAchievements();

    m_inSession = false;
}

void ProgressionSystem::updateFromAnalytics(const MovementAnalytics& analytics) {
    if (!m_inSession) return;

    // Update coverage stats
    float currentCoverage = analytics.getMovementSpaceCoverage();
    m_stats.movementSpaceCoverage = std::max(m_stats.movementSpaceCoverage, currentCoverage);
    m_currentSession.coverageGained = currentCoverage;

    // Track uncommon areas
    if (analytics.isInUncommonPosition()) {
        m_currentSession.uncommonAreasFound++;
    }

    // Update movement zones from analytics data
    const auto& samples = analytics.getSamples();
    if (!samples.empty()) {
        const auto& latest = samples.back();

        // Check which zones the hands are in
        for (auto& zone : m_movementZones) {
            float leftDist = (latest.leftHandRelative - zone.center).length();
            float rightDist = (latest.rightHandRelative - zone.center).length();

            if (leftDist < zone.radius || rightDist < zone.radius) {
                zone.visitCount++;
                zone.totalTime += 1.0f / 60.0f;  // Assuming 60fps
                zone.explorationLevel = std::min(1.0f, zone.explorationLevel + 0.001f);
            }
        }
    }

    // Update challenge progress
    updateChallengeProgress("coverage", currentCoverage);
    updateChallengeProgress("uncommon_areas", static_cast<float>(m_currentSession.uncommonAreasFound));
}

const LevelInfo& ProgressionSystem::getCurrentLevelInfo() const {
    if (m_currentLevel > 0 && m_currentLevel <= static_cast<int>(m_levelDefinitions.size())) {
        return m_levelDefinitions[m_currentLevel - 1];
    }
    return m_levelDefinitions[0];
}

int ProgressionSystem::getXPToNextLevel() const {
    const auto& info = getCurrentLevelInfo();
    return info.xpForNext - (m_currentXP - info.xpRequired);
}

float ProgressionSystem::getLevelProgress() const {
    const auto& info = getCurrentLevelInfo();
    if (info.xpForNext == 0) return 1.0f;  // Max level
    int xpInLevel = m_currentXP - info.xpRequired;
    return static_cast<float>(xpInLevel) / static_cast<float>(info.xpForNext);
}

void ProgressionSystem::addXP(int amount) {
    m_currentXP += amount;

    // Check for level up
    while (m_currentLevel < static_cast<int>(m_levelDefinitions.size())) {
        const auto& nextLevel = m_levelDefinitions[m_currentLevel];
        if (m_currentXP >= nextLevel.xpRequired) {
            m_currentLevel++;
            if (m_levelUpCallback) {
                m_levelUpCallback(m_currentLevel, getCurrentLevelInfo());
            }
        } else {
            break;
        }
    }
}

std::vector<Achievement> ProgressionSystem::getUnlockedAchievements() const {
    std::vector<Achievement> result;
    for (const auto& a : m_achievements) {
        if (a.unlocked) result.push_back(a);
    }
    return result;
}

std::vector<Achievement> ProgressionSystem::getInProgressAchievements() const {
    std::vector<Achievement> result;
    for (const auto& a : m_achievements) {
        if (!a.unlocked && a.progress > 0) result.push_back(a);
    }
    return result;
}

bool ProgressionSystem::checkAchievement(const std::string& id) {
    for (auto& achievement : m_achievements) {
        if (achievement.id == id && !achievement.unlocked) {
            achievement.unlocked = true;
            achievement.unlockedTime = static_cast<double>(std::time(nullptr));
            achievement.progress = 1.0f;
            m_stats.achievementsUnlocked++;

            // Give achievement XP bonus
            addXP(50);

            if (m_achievementCallback) {
                m_achievementCallback(achievement);
            }
            return true;
        }
    }
    return false;
}

MovementZone* ProgressionSystem::getZoneAt(const Vec3& headRelativePos) {
    for (auto& zone : m_movementZones) {
        float dist = (headRelativePos - zone.center).length();
        if (dist < zone.radius) {
            return &zone;
        }
    }
    return nullptr;
}

float ProgressionSystem::getZoneMastery(const std::string& zoneName) const {
    for (const auto& zone : m_movementZones) {
        if (zone.name == zoneName) {
            return zone.masteryLevel;
        }
    }
    return 0.0f;
}

void ProgressionSystem::refreshDailyChallenges() {
    // Remove expired daily challenges
    auto now = std::chrono::system_clock::now();
    m_activeChallenges.erase(
        std::remove_if(m_activeChallenges.begin(), m_activeChallenges.end(),
            [now](const Challenge& c) {
                return c.type == Challenge::Type::Daily && c.expiresAt < now;
            }),
        m_activeChallenges.end());

    // Generate new daily challenge
    generateDailyChallenge();
    m_lastDailyChallengeDate = now;
}

void ProgressionSystem::refreshWeeklyChallenges() {
    auto now = std::chrono::system_clock::now();
    m_activeChallenges.erase(
        std::remove_if(m_activeChallenges.begin(), m_activeChallenges.end(),
            [now](const Challenge& c) {
                return c.type == Challenge::Type::Weekly && c.expiresAt < now;
            }),
        m_activeChallenges.end());

    generateWeeklyChallenge();
    m_lastWeeklyChallengeDate = now;
}

void ProgressionSystem::updateChallengeProgress(const std::string& metric, float value) {
    for (auto& challenge : m_activeChallenges) {
        if (challenge.goalMetric == metric && !challenge.completed) {
            challenge.currentProgress = std::max(challenge.currentProgress, value);

            if (challenge.currentProgress >= challenge.targetValue) {
                challenge.completed = true;
                addXP(challenge.xpReward);

                if (!challenge.achievementReward.empty()) {
                    checkAchievement(challenge.achievementReward);
                }

                if (m_challengeCompleteCallback) {
                    m_challengeCompleteCallback(challenge);
                }
            }
        }
    }
}

void ProgressionSystem::generateDailyChallenge() {
    Challenge daily;
    daily.id = "daily_" + std::to_string(std::time(nullptr));
    daily.type = Challenge::Type::Daily;

    // Random challenge type
    int type = std::rand() % 3;
    switch (type) {
        case 0:
            daily.name = "Coverage Explorer";
            daily.description = "Increase your movement space coverage by 5%";
            daily.goalMetric = "coverage";
            daily.targetValue = m_stats.movementSpaceCoverage + 5.0f;
            break;
        case 1:
            daily.name = "Uncommon Seeker";
            daily.description = "Discover 3 uncommon positions";
            daily.goalMetric = "uncommon_areas";
            daily.targetValue = 3.0f;
            break;
        case 2:
            daily.name = "Flow Finder";
            daily.description = "Maintain flow state for 30 seconds";
            daily.goalMetric = "flow_time";
            daily.targetValue = 30.0f;
            break;
    }

    daily.xpReward = 25;
    daily.currentProgress = 0;
    daily.completed = false;
    daily.expiresAt = std::chrono::system_clock::now() + std::chrono::hours(24);

    m_activeChallenges.push_back(daily);
}

void ProgressionSystem::generateWeeklyChallenge() {
    Challenge weekly;
    weekly.id = "weekly_" + std::to_string(std::time(nullptr));
    weekly.type = Challenge::Type::Weekly;

    weekly.name = "Week of Movement";
    weekly.description = "Reach 25% overall coverage this week";
    weekly.goalMetric = "coverage";
    weekly.targetValue = 25.0f;
    weekly.xpReward = 100;
    weekly.currentProgress = 0;
    weekly.completed = false;
    weekly.expiresAt = std::chrono::system_clock::now() + std::chrono::hours(24 * 7);

    m_activeChallenges.push_back(weekly);
}

void ProgressionSystem::checkAllAchievements() {
    // Time-based achievements
    if (m_stats.totalPlayTime >= 3600) checkAchievement("time_1h");
    if (m_stats.totalPlayTime >= 36000) checkAchievement("time_10h");
    if (m_stats.totalPlayTime >= 360000) checkAchievement("time_100h");

    // Session achievements
    if (m_stats.totalSessions >= 10) checkAchievement("sessions_10");
    if (m_stats.totalSessions >= 100) checkAchievement("sessions_100");

    // Coverage achievements
    if (m_stats.movementSpaceCoverage >= 10) checkAchievement("explorer_10");
    if (m_stats.movementSpaceCoverage >= 25) checkAchievement("explorer_25");
    if (m_stats.movementSpaceCoverage >= 50) checkAchievement("explorer_50");
    if (m_stats.movementSpaceCoverage >= 75) checkAchievement("explorer_75");

    // Uncommon discoveries
    if (m_stats.uncommonAreasDiscovered >= 1) checkAchievement("uncommon_1");
    if (m_stats.uncommonAreasDiscovered >= 10) checkAchievement("uncommon_10");
    if (m_stats.uncommonAreasDiscovered >= 50) checkAchievement("uncommon_50");

    // Streak achievements
    if (m_stats.currentStreak >= 3) checkAchievement("streak_3");
    if (m_stats.currentStreak >= 7) checkAchievement("streak_7");
    if (m_stats.currentStreak >= 30) checkAchievement("streak_30");

    // Symmetry achievement
    if (m_stats.bodySymmetryScore >= 90) checkAchievement("symmetry_90");

    // Flow achievements
    if (m_stats.bestFlowDuration >= 30) checkAchievement("flow_30");
    if (m_stats.bestFlowDuration >= 60) checkAchievement("flow_60");
    if (m_stats.bestFlowDuration >= 300) checkAchievement("flow_300");

    // Zone-specific achievements
    for (const auto& zone : m_movementZones) {
        if (zone.name == "Behind High" || zone.name == "Behind Low") {
            if (zone.explorationLevel >= 0.9f) {
                checkAchievement("behind_master");
            }
        }
        if (zone.name == "Overhead") {
            if (zone.explorationLevel >= 0.9f) {
                checkAchievement("overhead_master");
            }
        }
        if (zone.name == "Floor Front" || zone.name == "Floor Side") {
            if (zone.explorationLevel >= 0.9f) {
                checkAchievement("floor_master");
            }
        }
    }
}

} // namespace lst
