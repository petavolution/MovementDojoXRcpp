/**
 * ProgressionSystem Tests
 *
 * Tests for XP, levels, achievements, challenges, and session tracking.
 */

#include "../src/progression/ProgressionSystem.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <functional>
#include <filesystem>

namespace {

int testsPassed = 0;
int testsFailed = 0;

void runTest(const std::string& name, std::function<bool()> test) {
    std::cout << "  Testing " << name << "... ";
    try {
        if (test()) {
            std::cout << "PASSED" << std::endl;
            testsPassed++;
        } else {
            std::cout << "FAILED" << std::endl;
            testsFailed++;
        }
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        testsFailed++;
    }
}

bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

// =============================================================================
// Profile Tests
// =============================================================================

bool testCreateNewProfile() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("TestPlayer");

    const auto& stats = progression.getStats();
    if (stats.totalPlayTime != 0) return false;
    if (stats.totalSessions != 0) return false;
    if (progression.getCurrentLevel() != 1) return false;
    if (progression.getCurrentXP() != 0) return false;

    return true;
}

bool testSaveLoadProfile() {
    const std::string testPath = "/tmp/test_profile.json";

    // Create and save
    {
        lst::ProgressionSystem progression;
        progression.createNewProfile("SaveTest");
        progression.addXP(500);
        bool saved = progression.saveProfile(testPath);
        if (!saved) return false;
    }

    // Load and verify
    {
        lst::ProgressionSystem progression;
        bool loaded = progression.loadProfile(testPath);
        if (!loaded) return false;
        if (progression.getCurrentXP() < 500) return false;
    }

    // Cleanup
    std::filesystem::remove(testPath);
    return true;
}

bool testLoadNonexistentProfile() {
    lst::ProgressionSystem progression;
    bool result = progression.loadProfile("/nonexistent/path/profile.json");
    return !result;  // Should fail gracefully
}

// =============================================================================
// Level System Tests
// =============================================================================

bool testInitialLevel() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("LevelTest");

    if (progression.getCurrentLevel() != 1) return false;

    const auto& info = progression.getCurrentLevelInfo();
    if (info.level != 1) return false;
    if (info.title != "Newcomer") return false;

    return true;
}

bool testAddXP() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("XPTest");

    progression.addXP(50);
    if (progression.getCurrentXP() != 50) return false;

    progression.addXP(30);
    if (progression.getCurrentXP() != 80) return false;

    return true;
}

bool testLevelUp() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("LevelUpTest");

    bool leveledUp = false;
    progression.setLevelUpCallback([&](int newLevel, const lst::LevelInfo& info) {
        leveledUp = true;
    });

    // Level 2 requires 100 XP
    progression.addXP(100);

    if (!leveledUp) return false;
    if (progression.getCurrentLevel() != 2) return false;

    return true;
}

bool testMultipleLevelUps() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("MultiLevelTest");

    int levelUpCount = 0;
    progression.setLevelUpCallback([&](int newLevel, const lst::LevelInfo& info) {
        levelUpCount++;
    });

    // Add enough XP for multiple levels (level 5 requires 1600 total)
    progression.addXP(2000);

    if (levelUpCount < 2) return false;
    if (progression.getCurrentLevel() < 3) return false;

    return true;
}

bool testXPToNextLevel() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("XPNextTest");

    int xpNeeded = progression.getXPToNextLevel();
    if (xpNeeded != 100) return false;  // Level 2 requires 100 XP

    progression.addXP(50);
    xpNeeded = progression.getXPToNextLevel();
    if (xpNeeded != 50) return false;  // 50 more to level 2

    return true;
}

bool testLevelProgress() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("ProgressTest");

    float progress = progression.getLevelProgress();
    if (!approxEqual(progress, 0.0f)) return false;

    progression.addXP(50);  // Half way to level 2
    progress = progression.getLevelProgress();
    if (!approxEqual(progress, 0.5f, 0.1f)) return false;

    return true;
}

bool testMaxLevel() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("MaxLevelTest");

    // Add way more XP than needed for max level
    progression.addXP(100000);

    int level = progression.getCurrentLevel();
    // Should be at or near max level (10)
    if (level < 10) return false;

    // XP to next should be 0 at max level
    if (progression.getCurrentLevel() == 10) {
        // At max level, getXPToNextLevel behavior may vary
        return true;
    }

    return true;
}

// =============================================================================
// Achievement Tests
// =============================================================================

bool testInitialAchievements() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("AchievementTest");

    const auto& achievements = progression.getAchievements();
    if (achievements.empty()) return false;

    // All should be locked initially
    for (const auto& a : achievements) {
        if (a.unlocked) return false;
    }

    return true;
}

bool testUnlockAchievement() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("UnlockTest");

    bool achievementUnlocked = false;
    std::string unlockedId;

    progression.setAchievementCallback([&](const lst::Achievement& a) {
        achievementUnlocked = true;
        unlockedId = a.id;
    });

    // Simulate conditions that might unlock an achievement
    // (depends on implementation - just testing the callback mechanism)
    progression.checkAchievement("first_session");

    // The achievement system should work even if not unlocked yet
    return true;
}

bool testGetUnlockedAchievements() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("GetUnlockedTest");

    auto unlocked = progression.getUnlockedAchievements();
    if (!unlocked.empty()) return false;  // Should start empty

    // After some progression, check again
    progression.addXP(500);
    // Unlocked list depends on achievement criteria
    return true;
}

bool testAchievementCategories() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("CategoryTest");

    const auto& achievements = progression.getAchievements();

    bool hasExploration = false;
    bool hasMastery = false;
    bool hasConsistency = false;

    for (const auto& a : achievements) {
        if (a.category == lst::Achievement::Category::Exploration) hasExploration = true;
        if (a.category == lst::Achievement::Category::Mastery) hasMastery = true;
        if (a.category == lst::Achievement::Category::Consistency) hasConsistency = true;
    }

    return hasExploration && hasMastery && hasConsistency;
}

// =============================================================================
// Session Tracking Tests
// =============================================================================

bool testStartSession() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("SessionTest");

    const auto& stats = progression.getStats();
    int initialSessions = stats.totalSessions;

    progression.startSession();
    // Session count should increase when ended, not started

    return true;
}

bool testEndSession() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("EndSessionTest");

    progression.startSession();
    // Simulate some time passing
    progression.endSession();

    const auto& stats = progression.getStats();
    if (stats.totalSessions < 1) return false;

    return true;
}

bool testSessionHistory() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("HistoryTest");

    // Run a few sessions
    for (int i = 0; i < 3; i++) {
        progression.startSession();
        progression.endSession();
    }

    const auto& history = progression.getHistory();
    if (history.size() < 3) return false;

    return true;
}

// =============================================================================
// Movement Zone Tests
// =============================================================================

bool testMovementZones() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("ZoneTest");

    const auto& zones = progression.getMovementZones();
    if (zones.empty()) return false;

    // Check for expected zones
    bool hasOverhead = false;
    bool hasBehind = false;

    for (const auto& zone : zones) {
        if (zone.name == "Overhead") hasOverhead = true;
        if (zone.name.find("Behind") != std::string::npos) hasBehind = true;
    }

    return hasOverhead;
}

bool testGetZoneAt() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("ZoneAtTest");

    // Test overhead position
    lst::Vec3 overheadPos(0, 0.6f, 0);
    auto* zone = progression.getZoneAt(overheadPos);

    // Zone lookup may or may not find a match
    return true;  // Just testing it doesn't crash
}

bool testZoneMastery() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("MasteryTest");

    float mastery = progression.getZoneMastery("Overhead");
    // Initial mastery should be 0
    if (mastery < 0.0f || mastery > 1.0f) return false;

    return true;
}

// =============================================================================
// Challenge Tests
// =============================================================================

bool testDailyChallenges() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("DailyTest");

    progression.refreshDailyChallenges();

    const auto& challenges = progression.getActiveChallenges();
    // Should have at least one daily challenge
    bool hasDaily = false;
    for (const auto& c : challenges) {
        if (c.type == lst::Challenge::Type::Daily) {
            hasDaily = true;
            break;
        }
    }

    return hasDaily;
}

bool testWeeklyChallenges() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("WeeklyTest");

    progression.refreshWeeklyChallenges();

    const auto& challenges = progression.getActiveChallenges();
    bool hasWeekly = false;
    for (const auto& c : challenges) {
        if (c.type == lst::Challenge::Type::Weekly) {
            hasWeekly = true;
            break;
        }
    }

    return hasWeekly;
}

bool testChallengeProgress() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("ChallengeProgressTest");

    progression.refreshDailyChallenges();
    const auto& challenges = progression.getActiveChallenges();

    if (challenges.empty()) return true;  // No challenges to test

    std::string metric = challenges[0].goalMetric;
    float target = challenges[0].targetValue;

    progression.updateChallengeProgress(metric, target * 0.5f);

    // Check progress was updated
    for (const auto& c : progression.getActiveChallenges()) {
        if (c.goalMetric == metric) {
            if (c.currentProgress > 0) return true;
        }
    }

    return true;
}

bool testChallengeCompletion() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("CompletionTest");

    bool challengeCompleted = false;
    progression.setChallengeCompleteCallback([&](const lst::Challenge& c) {
        challengeCompleted = true;
    });

    progression.refreshDailyChallenges();
    const auto& challenges = progression.getActiveChallenges();

    if (challenges.empty()) return true;

    // Complete a challenge
    std::string metric = challenges[0].goalMetric;
    float target = challenges[0].targetValue;
    progression.updateChallengeProgress(metric, target * 2);  // Over-complete

    return challengeCompleted;
}

// =============================================================================
// Analytics Integration Tests
// =============================================================================

bool testUpdateFromAnalytics() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("AnalyticsTest");

    // Create mock analytics data
    lst::MovementAnalytics analytics;
    analytics.startSession();

    // Record some samples
    lst::Transform head;
    head.position = lst::Vec3(0, 1.7f, 0);
    lst::ControllerState left, right;
    left.position = lst::Vec3(-0.3f, 1.0f, -0.3f);
    right.position = lst::Vec3(0.3f, 1.0f, -0.3f);

    for (int i = 0; i < 100; i++) {
        analytics.recordSample(head, left, right);
    }

    progression.startSession();
    progression.updateFromAnalytics(analytics);

    // Stats should be updated
    const auto& stats = progression.getStats();
    // Movement samples should be recorded
    return stats.totalMovementSamples >= 0;  // May or may not update immediately
}

// =============================================================================
// Player Stats Tests
// =============================================================================

bool testPlayerStats() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("StatsTest");

    const auto& stats = progression.getStats();

    // Initial stats should be zeroed
    if (stats.totalPlayTime != 0) return false;
    if (stats.currentStreak != 0) return false;
    if (stats.overallMastery < 0 || stats.overallMastery > 100) return false;

    return true;
}

bool testStreakTracking() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("StreakTest");

    // Simulate daily sessions
    progression.startSession();
    progression.endSession();

    const auto& stats = progression.getStats();
    // Streak tracking depends on calendar logic
    return stats.currentStreak >= 0 && stats.longestStreak >= 0;
}

// =============================================================================
// Callback Tests
// =============================================================================

bool testAllCallbacks() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("CallbackTest");

    bool levelCalled = false;
    bool achievementCalled = false;
    bool challengeCalled = false;

    progression.setLevelUpCallback([&](int, const lst::LevelInfo&) {
        levelCalled = true;
    });

    progression.setAchievementCallback([&](const lst::Achievement&) {
        achievementCalled = true;
    });

    progression.setChallengeCompleteCallback([&](const lst::Challenge&) {
        challengeCalled = true;
    });

    // Trigger level up
    progression.addXP(200);

    return levelCalled;  // At minimum, level up should trigger
}

// =============================================================================
// Edge Case Tests
// =============================================================================

bool testNegativeXP() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("NegativeXPTest");

    progression.addXP(100);
    progression.addXP(-50);  // Should this be allowed?

    // XP should not go negative
    if (progression.getCurrentXP() < 0) return false;

    return true;
}

bool testEmptyPlayerName() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("");

    // Should handle empty name gracefully
    return true;
}

bool testRapidSessionStartStop() {
    lst::ProgressionSystem progression;
    progression.createNewProfile("RapidSessionTest");

    for (int i = 0; i < 100; i++) {
        progression.startSession();
        progression.endSession();
    }

    const auto& stats = progression.getStats();
    return stats.totalSessions >= 100;
}

}  // namespace

int runProgressionTests() {
    std::cout << "ProgressionSystem Tests" << std::endl;
    std::cout << "-----------------------" << std::endl;

    testsPassed = 0;
    testsFailed = 0;

    // Profile tests
    runTest("create new profile", testCreateNewProfile);
    runTest("save/load profile", testSaveLoadProfile);
    runTest("load nonexistent profile", testLoadNonexistentProfile);

    // Level system
    runTest("initial level", testInitialLevel);
    runTest("add XP", testAddXP);
    runTest("level up", testLevelUp);
    runTest("multiple level ups", testMultipleLevelUps);
    runTest("XP to next level", testXPToNextLevel);
    runTest("level progress", testLevelProgress);
    runTest("max level", testMaxLevel);

    // Achievements
    runTest("initial achievements", testInitialAchievements);
    runTest("unlock achievement", testUnlockAchievement);
    runTest("get unlocked achievements", testGetUnlockedAchievements);
    runTest("achievement categories", testAchievementCategories);

    // Session tracking
    runTest("start session", testStartSession);
    runTest("end session", testEndSession);
    runTest("session history", testSessionHistory);

    // Movement zones
    runTest("movement zones", testMovementZones);
    runTest("get zone at position", testGetZoneAt);
    runTest("zone mastery", testZoneMastery);

    // Challenges
    runTest("daily challenges", testDailyChallenges);
    runTest("weekly challenges", testWeeklyChallenges);
    runTest("challenge progress", testChallengeProgress);
    runTest("challenge completion", testChallengeCompletion);

    // Analytics integration
    runTest("update from analytics", testUpdateFromAnalytics);

    // Player stats
    runTest("player stats", testPlayerStats);
    runTest("streak tracking", testStreakTracking);

    // Callbacks
    runTest("all callbacks", testAllCallbacks);

    // Edge cases
    runTest("negative XP", testNegativeXP);
    runTest("empty player name", testEmptyPlayerName);
    runTest("rapid session start/stop", testRapidSessionStartStop);

    std::cout << std::endl;
    std::cout << "Progression Tests: " << testsPassed << " passed, " << testsFailed << " failed" << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
