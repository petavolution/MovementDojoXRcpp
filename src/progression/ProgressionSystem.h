#pragma once

#include "Types.h"
#include "analytics/MovementAnalytics.h"
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <chrono>

namespace lst {

/**
 * Serious Game Progression System
 *
 * Tracks long-term progress in movement exploration and body awareness.
 * Gamifies the process of exploring the full human movement space.
 *
 * Key metrics:
 * - Movement Space Mastery: % of reachable space explored
 * - Movement Variety: diversity of movements performed
 * - Body Symmetry: left/right balance
 * - Uncommon Movement Discovery: exploration of rarely-used positions
 * - Consistency: regular practice tracking
 */

// =============================================================================
// Achievement System
// =============================================================================

struct Achievement {
    std::string id;
    std::string name;
    std::string description;
    std::string iconName;

    enum class Category {
        Exploration,    // Discovering new movement areas
        Mastery,       // Perfecting specific movements
        Consistency,   // Regular practice
        Discovery,     // Finding uncommon positions
        Flow,          // Achieving flow states
        Milestone      // Total time/sessions milestones
    };
    Category category;

    bool unlocked;
    double unlockedTime;
    float progress;  // 0-1 for achievements with progress
};

// =============================================================================
// Movement Space Mastery
// =============================================================================

struct MovementZone {
    std::string name;
    std::string description;

    // Zone bounds in head-relative space
    Vec3 center;
    float radius;

    // Mastery levels
    float explorationLevel;  // How much of this zone has been visited
    float masteryLevel;      // How fluent are movements in this zone
    int visitCount;
    float totalTime;

    // Difficulty rating
    float difficultyRating;  // How challenging is this zone (0-1)
    bool isUncommon;         // Is this a rarely-used daily position
};

// =============================================================================
// Player Stats & Profile
// =============================================================================

struct PlayerStats {
    // Lifetime stats
    double totalPlayTime;           // Seconds
    int totalSessions;
    int totalMovementSamples;

    // Movement mastery
    float overallMastery;           // 0-100%
    float movementSpaceCoverage;    // 0-100%
    float movementVarietyScore;     // 0-100%
    float bodySymmetryScore;        // 0-100%

    // Discovery
    int uncommonAreasDiscovered;
    int achievementsUnlocked;
    int zonesFullyExplored;

    // Streaks
    int currentStreak;              // Days in a row
    int longestStreak;
    int sessionsThisWeek;

    // Session bests
    float bestFlowDuration;         // Seconds
    float bestCoverageInSession;    // %
    int mostUncommonInSession;
};

struct SessionRecord {
    std::chrono::system_clock::time_point startTime;
    double duration;

    float coverageGained;
    int uncommonAreasFound;
    float flowTimeAchieved;
    float averageIntensity;

    std::vector<std::string> achievementsUnlocked;
    std::string dominantMode;  // Which training mode was used most
};

// =============================================================================
// Level System
// =============================================================================

struct LevelInfo {
    int level;
    std::string title;        // e.g., "Novice Explorer", "Movement Master"
    int xpRequired;
    int xpForNext;

    // Unlocks at this level
    std::vector<std::string> unlockedModes;
    std::vector<std::string> unlockedFeatures;
};

// =============================================================================
// Daily/Weekly Challenges
// =============================================================================

struct Challenge {
    std::string id;
    std::string name;
    std::string description;

    enum class Type {
        Daily,
        Weekly,
        Special
    };
    Type type;

    // Goal
    std::string goalMetric;   // e.g., "coverage", "uncommon_areas", "flow_time"
    float targetValue;
    float currentProgress;

    // Reward
    int xpReward;
    std::string achievementReward;  // Optional achievement to unlock

    // Timing
    std::chrono::system_clock::time_point expiresAt;
    bool completed;
};

// =============================================================================
// Main Progression System
// =============================================================================

class ProgressionSystem {
public:
    ProgressionSystem();
    ~ProgressionSystem();

    // Profile management
    bool loadProfile(const std::string& profilePath);
    bool saveProfile(const std::string& profilePath);
    void createNewProfile(const std::string& playerName);

    // Session tracking
    void startSession();
    void endSession();
    void updateFromAnalytics(const MovementAnalytics& analytics);

    // Stats access
    const PlayerStats& getStats() const { return m_stats; }
    const std::vector<SessionRecord>& getHistory() const { return m_sessionHistory; }

    // Level system
    int getCurrentLevel() const { return m_currentLevel; }
    const LevelInfo& getCurrentLevelInfo() const;
    int getCurrentXP() const { return m_currentXP; }
    int getXPToNextLevel() const;
    float getLevelProgress() const;  // 0-1

    void addXP(int amount);

    // Achievements
    const std::vector<Achievement>& getAchievements() const { return m_achievements; }
    std::vector<Achievement> getUnlockedAchievements() const;
    std::vector<Achievement> getInProgressAchievements() const;
    bool checkAchievement(const std::string& id);

    // Movement zones
    const std::vector<MovementZone>& getMovementZones() const { return m_movementZones; }
    MovementZone* getZoneAt(const Vec3& headRelativePos);
    float getZoneMastery(const std::string& zoneName) const;

    // Challenges
    const std::vector<Challenge>& getActiveChallenges() const { return m_activeChallenges; }
    void refreshDailyChallenges();
    void refreshWeeklyChallenges();
    void updateChallengeProgress(const std::string& metric, float value);

    // Callbacks for UI
    using LevelUpCallback = std::function<void(int newLevel, const LevelInfo& info)>;
    using AchievementCallback = std::function<void(const Achievement& achievement)>;
    using ChallengeCompleteCallback = std::function<void(const Challenge& challenge)>;

    void setLevelUpCallback(LevelUpCallback cb) { m_levelUpCallback = cb; }
    void setAchievementCallback(AchievementCallback cb) { m_achievementCallback = cb; }
    void setChallengeCompleteCallback(ChallengeCompleteCallback cb) { m_challengeCompleteCallback = cb; }

private:
    void initializeAchievements();
    void initializeMovementZones();
    void initializeLevelSystem();
    void checkAllAchievements();
    void generateDailyChallenge();
    void generateWeeklyChallenge();

    std::string m_playerName;
    PlayerStats m_stats;
    std::vector<SessionRecord> m_sessionHistory;

    // Current session
    bool m_inSession = false;
    SessionRecord m_currentSession;

    // Level system
    int m_currentLevel = 1;
    int m_currentXP = 0;
    std::vector<LevelInfo> m_levelDefinitions;

    // Achievements
    std::vector<Achievement> m_achievements;

    // Movement zones
    std::vector<MovementZone> m_movementZones;

    // Challenges
    std::vector<Challenge> m_activeChallenges;
    std::chrono::system_clock::time_point m_lastDailyChallengeDate;
    std::chrono::system_clock::time_point m_lastWeeklyChallengeDate;

    // Callbacks
    LevelUpCallback m_levelUpCallback;
    AchievementCallback m_achievementCallback;
    ChallengeCompleteCallback m_challengeCompleteCallback;
};

// =============================================================================
// Movement Zone Definitions
// =============================================================================

namespace MovementZones {
    // Upper body zones
    const MovementZone OVERHEAD {"Overhead", "Above your head", Vec3(0, 0.6f, 0), 0.3f, 0, 0, 0, 0, 0.7f, true};
    const MovementZone FRONT_HIGH {"Front High", "In front, above shoulders", Vec3(0, 0.2f, -0.5f), 0.3f, 0, 0, 0, 0, 0.3f, false};
    const MovementZone FRONT_MID {"Front Mid", "In front, chest level", Vec3(0, 0, -0.5f), 0.3f, 0, 0, 0, 0, 0.1f, false};
    const MovementZone FRONT_LOW {"Front Low", "In front, below waist", Vec3(0, -0.4f, -0.5f), 0.3f, 0, 0, 0, 0, 0.4f, false};

    // Side zones
    const MovementZone LEFT_HIGH {"Left High", "Left side, high", Vec3(-0.6f, 0.2f, -0.2f), 0.3f, 0, 0, 0, 0, 0.5f, true};
    const MovementZone LEFT_MID {"Left Mid", "Left side, mid", Vec3(-0.6f, 0, -0.2f), 0.3f, 0, 0, 0, 0, 0.3f, false};
    const MovementZone RIGHT_HIGH {"Right High", "Right side, high", Vec3(0.6f, 0.2f, -0.2f), 0.3f, 0, 0, 0, 0, 0.5f, true};
    const MovementZone RIGHT_MID {"Right Mid", "Right side, mid", Vec3(0.6f, 0, -0.2f), 0.3f, 0, 0, 0, 0, 0.3f, false};

    // Behind zones (uncommon)
    const MovementZone BEHIND_HIGH {"Behind High", "Behind, shoulder level", Vec3(0, 0.1f, 0.3f), 0.3f, 0, 0, 0, 0, 0.9f, true};
    const MovementZone BEHIND_LOW {"Behind Low", "Behind, lower back", Vec3(0, -0.3f, 0.3f), 0.3f, 0, 0, 0, 0, 0.8f, true};

    // Low zones (uncommon)
    const MovementZone FLOOR_FRONT {"Floor Front", "Near floor, in front", Vec3(0, -0.8f, -0.4f), 0.3f, 0, 0, 0, 0, 0.85f, true};
    const MovementZone FLOOR_SIDE {"Floor Side", "Near floor, to sides", Vec3(0.4f, -0.8f, 0), 0.4f, 0, 0, 0, 0, 0.9f, true};
}

// =============================================================================
// Level Definitions
// =============================================================================

namespace Levels {
    const std::vector<LevelInfo> DEFINITIONS = {
        {1, "Newcomer", 0, 100, {}, {}},
        {2, "Explorer", 100, 250, {"free_exploration"}, {}},
        {3, "Seeker", 350, 500, {"guided_stretch"}, {"trail_colors"}},
        {4, "Practitioner", 850, 750, {"breathing_sync"}, {"heatmap_view"}},
        {5, "Adept", 1600, 1000, {"mirror_mode"}, {"ghost_playback"}},
        {6, "Journeyman", 2600, 1500, {"meditation_mode"}, {"custom_sequences"}},
        {7, "Expert", 4100, 2000, {"flow_state"}, {"movement_analysis"}},
        {8, "Master", 6100, 3000, {}, {"advanced_stats"}},
        {9, "Grand Master", 9100, 5000, {}, {"mentor_mode"}},
        {10, "Movement Sage", 14100, 0, {}, {"all_features"}}
    };
}

} // namespace lst
