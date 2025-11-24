#pragma once
/**
 * Level1SequenceConfig.h - Level 1 Training Sequence Definition
 *
 * Defines the "Level 1 - Fundamentals" training sequence:
 * - Phase 1: Intro (welcome, controls overview)
 * - Phase 2: Saber Basics (blocking projectiles)
 * - Phase 3: Ranged Basics (shooting targets)
 * - Phase 4: Mixed Basics (combined mechanics)
 * - Phase 5: Summary (performance review)
 *
 * This is an example of a data-driven sequence configuration.
 * In the future, these could be loaded from JSON/config files.
 */

#include "../TrainingSequence.h"

namespace lst {
namespace sequences {

/**
 * Create the Level 1 Fundamentals training sequence.
 *
 * This sequence teaches basic combat mechanics:
 * - Blocking with saber
 * - Shooting with blaster
 * - Combined defensive and offensive play
 */
inline TrainingSequenceConfig createLevel1Fundamentals() {
    TrainingSequenceConfig seq;

    // ==========================================================================
    // Sequence Metadata
    // ==========================================================================
    seq.withId("level1_fundamentals")
       .withName("Level 1 - Fundamentals")
       .withDescription("Learn the basics of combat: blocking and shooting")
       .withDifficulty(SequenceDifficulty::TUTORIAL)
       .withEnvironment("dojo")
       .withMinScore(40.0f);  // 40% to pass

    // ==========================================================================
    // Phase 1: INTRO
    // ==========================================================================
    {
        TrainingPhaseConfig intro;
        intro.withName("Welcome")
             .withType(PhaseType::INTRO)
             .withLearningGoal("Get familiar with your equipment")
             .withIntro("Welcome to the Dojo!\n\nLook at your hands:\n- Right hand: Saber (block incoming fire)\n- Left hand: Blaster (shoot targets)\n\nLet's begin!", 8.0f)
             .withOutro("Training begins now...", 2.0f);

        // Intro has one "empty" wave just to give the player time
        TrainingWaveConfig introWave;
        introWave.withName("Equipment Check")
                 .withEndCondition(WaveEndCondition::TIME_ELAPSED, 5.0f)
                 .withPrepTime(0.0f);
        // No spawns - just a pause

        intro.addWave(introWave);
        seq.addPhase(intro);
    }

    // ==========================================================================
    // Phase 2: SABER BASICS
    // ==========================================================================
    {
        TrainingPhaseConfig saberPhase;
        saberPhase.withName("Saber Basics")
                  .withType(PhaseType::DRILL)
                  .withLearningGoal("Block incoming projectiles with your saber")
                  .withIntro("SABER TRAINING\n\nDrones will fire slow projectiles.\nSwing your saber to block them!", 4.0f)
                  .withOutro("Saber training complete!", 2.0f)
                  .addHint("Move your saber into the path of incoming projectiles")
                  .addHint("Watch for the red glow - that's your target");

        // Wave 1: Single stationary drone
        {
            TrainingWaveConfig wave;
            wave.withName("Single Drone")
                .withEndCondition(WaveEndCondition::TIME_ELAPSED, 15.0f)
                .withPrepTime(2.0f);

            EnemySpawnDef drone;
            drone.type = EnemyType::FLYING_DRONE;
            drone.behavior = EnemyBehavior::STATIONARY;
            drone.count = 1;
            drone.fireRateMultiplier = 0.5f;  // Slow fire rate
            drone.tag = "tutorial_drone";

            wave.addSpawn(drone);
            saberPhase.addWave(wave);
        }

        // Wave 2: Two stationary drones
        {
            TrainingWaveConfig wave;
            wave.withName("Dual Drones")
                .withEndCondition(WaveEndCondition::TIME_ELAPSED, 15.0f)
                .withPrepTime(2.0f);

            EnemySpawnDef drone1;
            drone1.type = EnemyType::FLYING_DRONE;
            drone1.behavior = EnemyBehavior::STATIONARY;
            drone1.count = 1;
            drone1.fireRateMultiplier = 0.6f;
            drone1.tag = "left_drone";

            EnemySpawnDef drone2;
            drone2.type = EnemyType::FLYING_DRONE;
            drone2.behavior = EnemyBehavior::STATIONARY;
            drone2.count = 1;
            drone2.fireRateMultiplier = 0.7f;
            drone2.tag = "right_drone";

            wave.addSpawn(drone1);
            wave.addSpawn(drone2);
            saberPhase.addWave(wave);
        }

        // Wave 3: Orbiting drone (more challenging)
        {
            TrainingWaveConfig wave;
            wave.withName("Moving Target")
                .withEndCondition(WaveEndCondition::TIME_ELAPSED, 15.0f)
                .withPrepTime(2.0f);

            EnemySpawnDef orbiter;
            orbiter.type = EnemyType::FLYING_DRONE;
            orbiter.behavior = EnemyBehavior::ORBIT_PLAYER;
            orbiter.count = 1;
            orbiter.speedMultiplier = 0.5f;  // Slow orbit
            orbiter.fireRateMultiplier = 0.5f;
            orbiter.tag = "orbit_drone";

            wave.addSpawn(orbiter);
            saberPhase.addWave(wave);
        }

        seq.addPhase(saberPhase);
    }

    // ==========================================================================
    // Phase 3: RANGED BASICS
    // ==========================================================================
    {
        TrainingPhaseConfig rangedPhase;
        rangedPhase.withName("Ranged Basics")
                   .withType(PhaseType::DRILL)
                   .withLearningGoal("Shoot targets with your blaster")
                   .withIntro("BLASTER TRAINING\n\nShoot the training dummies!\nAim carefully and fire.", 4.0f)
                   .withOutro("Ranged training complete!", 2.0f)
                   .addHint("Point your left hand at targets")
                   .addHint("Pull the trigger to fire");

        // Wave 1: Stationary targets
        {
            TrainingWaveConfig wave;
            wave.withName("Still Targets")
                .withEndCondition(WaveEndCondition::TIME_ELAPSED, 15.0f)
                .withPrepTime(2.0f);

            EnemySpawnDef dummy1;
            dummy1.type = EnemyType::TRAINING_DUMMY;
            dummy1.behavior = EnemyBehavior::STATIONARY;
            dummy1.count = 2;
            dummy1.tag = "stationary_target";

            wave.addSpawn(dummy1);
            rangedPhase.addWave(wave);
        }

        // Wave 2: Moving targets
        {
            TrainingWaveConfig wave;
            wave.withName("Moving Targets")
                .withEndCondition(WaveEndCondition::TIME_ELAPSED, 15.0f)
                .withPrepTime(2.0f);

            EnemySpawnDef patrol;
            patrol.type = EnemyType::TRAINING_DUMMY;
            patrol.behavior = EnemyBehavior::PATROL;
            patrol.count = 2;
            patrol.speedMultiplier = 0.4f;  // Slow patrol
            patrol.tag = "patrol_target";

            wave.addSpawn(patrol);
            rangedPhase.addWave(wave);
        }

        // Wave 3: Mixed stationary and moving
        {
            TrainingWaveConfig wave;
            wave.withName("Mixed Targets")
                .withEndCondition(WaveEndCondition::TIME_ELAPSED, 15.0f)
                .withPrepTime(2.0f);

            EnemySpawnDef stationary;
            stationary.type = EnemyType::TRAINING_DUMMY;
            stationary.behavior = EnemyBehavior::STATIONARY;
            stationary.count = 1;

            EnemySpawnDef orbit;
            orbit.type = EnemyType::TRAINING_DUMMY;
            orbit.behavior = EnemyBehavior::ORBIT_PLAYER;
            orbit.count = 1;
            orbit.speedMultiplier = 0.3f;

            wave.addSpawn(stationary);
            wave.addSpawn(orbit);
            rangedPhase.addWave(wave);
        }

        seq.addPhase(rangedPhase);
    }

    // ==========================================================================
    // Phase 4: MIXED BASICS
    // ==========================================================================
    {
        TrainingPhaseConfig mixedPhase;
        mixedPhase.withName("Mixed Combat")
                  .withType(PhaseType::MIXED)
                  .withLearningGoal("Combine blocking and shooting")
                  .withIntro("COMBINED TRAINING\n\nNow put it all together!\nBlock projectiles AND shoot targets.", 4.0f)
                  .withOutro("Combat training complete!", 2.0f)
                  .addHint("Prioritize blocking - defense first!")
                  .addHint("Shoot during gaps between projectiles");

        // Wave 1: One shooter, one target
        {
            TrainingWaveConfig wave;
            wave.withName("Basic Combo")
                .withEndCondition(WaveEndCondition::TIME_ELAPSED, 20.0f)
                .withPrepTime(3.0f);

            EnemySpawnDef shooter;
            shooter.type = EnemyType::FLYING_DRONE;
            shooter.behavior = EnemyBehavior::STATIONARY;
            shooter.count = 1;
            shooter.fireRateMultiplier = 0.5f;

            EnemySpawnDef target;
            target.type = EnemyType::TRAINING_DUMMY;
            target.behavior = EnemyBehavior::STATIONARY;
            target.count = 1;

            wave.addSpawn(shooter);
            wave.addSpawn(target);
            mixedPhase.addWave(wave);
        }

        // Wave 2: Multiple threats
        {
            TrainingWaveConfig wave;
            wave.withName("Multi-Threat")
                .withEndCondition(WaveEndCondition::TIME_ELAPSED, 25.0f)
                .withPrepTime(3.0f);

            EnemySpawnDef shooters;
            shooters.type = EnemyType::FLYING_DRONE;
            shooters.behavior = EnemyBehavior::STATIONARY;
            shooters.count = 2;
            shooters.fireRateMultiplier = 0.6f;

            EnemySpawnDef targets;
            targets.type = EnemyType::TRAINING_DUMMY;
            targets.behavior = EnemyBehavior::PATROL;
            targets.count = 1;
            targets.speedMultiplier = 0.3f;

            wave.addSpawn(shooters);
            wave.addSpawn(targets);
            mixedPhase.addWave(wave);
        }

        // Wave 3: Final challenge with dive attack
        {
            TrainingWaveConfig wave;
            wave.withName("Final Challenge")
                .withEndCondition(WaveEndCondition::TIME_ELAPSED, 30.0f)
                .withPrepTime(3.0f)
                .withMinDuration(20.0f);  // Ensure enough time for dive attack

            EnemySpawnDef orbiter;
            orbiter.type = EnemyType::FLYING_DRONE;
            orbiter.behavior = EnemyBehavior::ORBIT_PLAYER;
            orbiter.count = 1;
            orbiter.fireRateMultiplier = 0.7f;

            EnemySpawnDef diver;
            diver.type = EnemyType::FLYING_DRONE;
            diver.behavior = EnemyBehavior::DIVE_ATTACK;
            diver.count = 1;
            diver.spawnDelay = 10.0f;  // Spawn after 10 seconds
            diver.tag = "dive_drone";

            EnemySpawnDef target;
            target.type = EnemyType::TRAINING_DUMMY;
            target.behavior = EnemyBehavior::STATIONARY;
            target.count = 1;

            wave.addSpawn(orbiter);
            wave.addSpawn(diver);
            wave.addSpawn(target);
            mixedPhase.addWave(wave);
        }

        seq.addPhase(mixedPhase);
    }

    // ==========================================================================
    // Phase 5: SUMMARY
    // ==========================================================================
    {
        TrainingPhaseConfig summary;
        summary.withName("Training Complete")
               .withType(PhaseType::SUMMARY)
               .withLearningGoal("Review your performance")
               .withIntro("TRAINING COMPLETE!\n\nLet's review your performance...", 3.0f)
               .withOutro("Well done, trainee!\n\nYou've completed Level 1.\nKeep practicing to improve!", 5.0f);

        // Empty wave for summary display
        TrainingWaveConfig summaryWave;
        summaryWave.withName("Performance Review")
                   .withEndCondition(WaveEndCondition::TIME_ELAPSED, 8.0f)
                   .withPrepTime(0.0f);

        summary.addWave(summaryWave);
        seq.addPhase(summary);
    }

    return seq;
}

/**
 * Registry of all available training sequences.
 * Call getSequence() to retrieve a sequence by ID.
 */
class SequenceRegistry {
public:
    /**
     * Get a sequence by ID.
     * Returns a default-constructed config if not found.
     */
    static TrainingSequenceConfig getSequence(const std::string& id) {
        if (id == "level1_fundamentals" || id == "level1") {
            return createLevel1Fundamentals();
        }

        // Unknown sequence - return empty with warning
        TrainingSequenceConfig empty;
        empty.id = "unknown";
        empty.name = "Unknown Sequence";
        return empty;
    }

    /**
     * Get list of available sequence IDs.
     */
    static std::vector<std::string> getAvailableSequences() {
        return {
            "level1_fundamentals"
            // Add more sequences here as they're created
        };
    }

    /**
     * Check if a sequence ID exists.
     */
    static bool hasSequence(const std::string& id) {
        auto available = getAvailableSequences();
        for (const auto& s : available) {
            if (s == id) return true;
        }
        return false;
    }
};

} // namespace sequences
} // namespace lst
