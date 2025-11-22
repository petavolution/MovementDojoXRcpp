/**
 * Level1Training.cpp - Dojo Level 1 Training Implementation
 */

#include "Level1Training.h"
#include <cmath>

namespace lst {

// =============================================================================
// Construction
// =============================================================================

Level1TrainingSystem::Level1TrainingSystem() {
    LOG_DEBUG(LOG_TAG_LEVEL1) << "Level1TrainingSystem created";
}

// =============================================================================
// System Interface
// =============================================================================

bool Level1TrainingSystem::onAttach(Engine* engine) {
    m_engine = engine;
    LOG_INFO(LOG_TAG_LEVEL1) << "Level 1 Training System attached";
    LOG_INFO(LOG_TAG_LEVEL1) << "========================================";
    LOG_INFO(LOG_TAG_LEVEL1) << "DOJO LEVEL 1 - Basic Training";
    LOG_INFO(LOG_TAG_LEVEL1) << "========================================";
    LOG_INFO(LOG_TAG_LEVEL1) << "Duration: ~3 minutes";
    LOG_INFO(LOG_TAG_LEVEL1) << "Phases: INTRO -> SABER -> BLASTER -> MIXED -> SUMMARY";
    LOG_INFO(LOG_TAG_LEVEL1) << "";

    // Setup the dojo environment
    setupDojoEnvironment();
    setupWeapons();

    return true;
}

void Level1TrainingSystem::onDetach() {
    LOG_INFO(LOG_TAG_LEVEL1) << "Level 1 Training System detaching";

    // Clean up any drill entities
    clearDrillEntities();

    // Log final summary if level was in progress
    if (m_currentState != Level1State::NOT_STARTED &&
        m_currentState != Level1State::COMPLETED) {
        LOG_WARN(LOG_TAG_LEVEL1) << "Level interrupted before completion";
        logLevelSummary();
    }

    m_engine = nullptr;
}

void Level1TrainingSystem::onSessionStart() {
    LOG_DEBUG(LOG_TAG_LEVEL1) << "Session started - beginning Level 1";
    startLevel();
}

void Level1TrainingSystem::onSessionEnd() {
    LOG_DEBUG(LOG_TAG_LEVEL1) << "Session ending";
    if (m_currentState != Level1State::COMPLETED) {
        LOG_INFO(LOG_TAG_LEVEL1) << "Level 1 session ended early";
    }
}

void Level1TrainingSystem::onUpdate(const FrameContext& ctx) {
    if (m_paused || m_currentState == Level1State::COMPLETED) {
        return;
    }

    // Track total level time
    m_totalLevelTime += static_cast<float>(ctx.deltaTime);

    // Update current state
    updateCurrentState(static_cast<float>(ctx.deltaTime));
}

void Level1TrainingSystem::onRender(const FrameContext& ctx) {
    if (m_currentState == Level1State::NOT_STARTED ||
        m_currentState == Level1State::COMPLETED) {
        return;
    }

    // Render state-appropriate HUD
    renderStateHUD(ctx);
}

// =============================================================================
// Level Control
// =============================================================================

void Level1TrainingSystem::startLevel() {
    if (m_currentState != Level1State::NOT_STARTED) {
        LOG_WARN(LOG_TAG_LEVEL1) << "Attempted to start level that's already in progress";
        return;
    }

    LOG_INFO(LOG_TAG_LEVEL1) << "========================================";
    LOG_INFO(LOG_TAG_LEVEL1) << "LEVEL 1 STARTING";
    LOG_INFO(LOG_TAG_LEVEL1) << "========================================";

    m_levelStartTime = std::chrono::steady_clock::now();
    m_totalLevelTime = 0.0f;
    m_stats = Level1Stats{};  // Reset stats

    transitionTo(Level1State::INTRO);
}

void Level1TrainingSystem::pauseLevel() {
    if (!m_paused) {
        m_paused = true;
        LOG_INFO(LOG_TAG_LEVEL1) << "Level PAUSED at state: " << level1StateToString(m_currentState);
    }
}

void Level1TrainingSystem::resumeLevel() {
    if (m_paused) {
        m_paused = false;
        LOG_INFO(LOG_TAG_LEVEL1) << "Level RESUMED at state: " << level1StateToString(m_currentState);
    }
}

void Level1TrainingSystem::skipToState(Level1State state) {
    LOG_WARN(LOG_TAG_LEVEL1) << "DEBUG: Skipping to state: " << level1StateToString(state);
    transitionTo(state);
}

float Level1TrainingSystem::getStateTimeRemaining() const {
    float duration = getStateDuration();
    return std::max(0.0f, duration - m_stateTimer);
}

float Level1TrainingSystem::getStateDuration() const {
    switch (m_currentState) {
        case Level1State::INTRO:         return INTRO_DURATION;
        case Level1State::SABER_DRILL:   return SABER_DRILL_DURATION;
        case Level1State::BLASTER_DRILL: return BLASTER_DRILL_DURATION;
        case Level1State::MIXED_DRILL:   return MIXED_DRILL_DURATION;
        case Level1State::SUMMARY:       return SUMMARY_DURATION;
        default:                         return 0.0f;
    }
}

// =============================================================================
// State Machine
// =============================================================================

void Level1TrainingSystem::transitionTo(Level1State newState) {
    if (m_currentState == newState) {
        return;
    }

    Level1State oldState = m_currentState;

    // Exit current state
    if (m_currentState != Level1State::NOT_STARTED) {
        onExitState(m_currentState);
    }

    // Log transition
    logStateTransition(oldState, newState);

    // Update state
    m_previousState = m_currentState;
    m_currentState = newState;
    m_stateTimer = 0.0f;

    // Enter new state
    onEnterState(newState);
}

void Level1TrainingSystem::updateCurrentState(float deltaTime) {
    m_stateTimer += deltaTime;

    switch (m_currentState) {
        case Level1State::INTRO:
            updateIntro(deltaTime);
            break;
        case Level1State::SABER_DRILL:
            updateSaberDrill(deltaTime);
            break;
        case Level1State::BLASTER_DRILL:
            updateBlasterDrill(deltaTime);
            break;
        case Level1State::MIXED_DRILL:
            updateMixedDrill(deltaTime);
            break;
        case Level1State::SUMMARY:
            updateSummary(deltaTime);
            break;
        default:
            break;
    }
}

// =============================================================================
// State Enter/Exit Handlers
// =============================================================================

void Level1TrainingSystem::onEnterState(Level1State state) {
    LOG_INFO(LOG_TAG_LEVEL1) << ">>> Entering state: " << level1StateToString(state);

    switch (state) {
        case Level1State::INTRO:
            LOG_INFO(LOG_TAG_LEVEL1) << "Welcome to the Dojo!";
            LOG_INFO(LOG_TAG_LEVEL1) << "Duration: " << INTRO_DURATION << "s";
            LOG_INFO(LOG_TAG_LEVEL1) << "Look at your hands:";
            LOG_INFO(LOG_TAG_LEVEL1) << "  RIGHT: Lightsaber (swing to strike, block incoming)";
            LOG_INFO(LOG_TAG_LEVEL1) << "  LEFT:  Blaster (trigger to shoot)";
            break;

        case Level1State::SABER_DRILL:
            LOG_INFO(LOG_TAG_LEVEL1) << "SABER DRILL - Practice blocking and striking";
            LOG_INFO(LOG_TAG_LEVEL1) << "Duration: " << SABER_DRILL_DURATION << "s";
            LOG_INFO(LOG_TAG_LEVEL1) << "Objectives:";
            LOG_INFO(LOG_TAG_LEVEL1) << "  - Strike the target spheres";
            LOG_INFO(LOG_TAG_LEVEL1) << "  - Block incoming slow projectiles";
            spawnSaberDrillEntities();
            break;

        case Level1State::BLASTER_DRILL:
            LOG_INFO(LOG_TAG_LEVEL1) << "BLASTER DRILL - Practice aiming and shooting";
            LOG_INFO(LOG_TAG_LEVEL1) << "Duration: " << BLASTER_DRILL_DURATION << "s";
            LOG_INFO(LOG_TAG_LEVEL1) << "Objectives:";
            LOG_INFO(LOG_TAG_LEVEL1) << "  - Shoot stationary and slow-moving drones";
            LOG_INFO(LOG_TAG_LEVEL1) << "  - Pull trigger to fire";
            spawnBlasterDrillEntities();
            break;

        case Level1State::MIXED_DRILL:
            LOG_INFO(LOG_TAG_LEVEL1) << "MIXED DRILL - Combined combat scenario";
            LOG_INFO(LOG_TAG_LEVEL1) << "Duration: " << MIXED_DRILL_DURATION << "s";
            LOG_INFO(LOG_TAG_LEVEL1) << "Objectives:";
            LOG_INFO(LOG_TAG_LEVEL1) << "  - Drones will shoot slow projectiles (block with saber)";
            LOG_INFO(LOG_TAG_LEVEL1) << "  - Shoot drones with blaster";
            LOG_INFO(LOG_TAG_LEVEL1) << "  - Watch for telegraphed dive attack!";
            m_diveAttackTriggered = false;
            spawnMixedDrillEntities();
            break;

        case Level1State::SUMMARY:
            LOG_INFO(LOG_TAG_LEVEL1) << "========================================";
            LOG_INFO(LOG_TAG_LEVEL1) << "LEVEL 1 COMPLETE!";
            LOG_INFO(LOG_TAG_LEVEL1) << "========================================";
            logLevelSummary();
            break;

        case Level1State::COMPLETED:
            LOG_INFO(LOG_TAG_LEVEL1) << "Level 1 finished. Great work!";
            break;

        default:
            break;
    }
}

void Level1TrainingSystem::onExitState(Level1State state) {
    LOG_DEBUG(LOG_TAG_LEVEL1) << "<<< Exiting state: " << level1StateToString(state)
                              << " (spent " << m_stateTimer << "s)";

    // Record time spent in each state
    switch (state) {
        case Level1State::INTRO:
            m_stats.introTime = m_stateTimer;
            break;
        case Level1State::SABER_DRILL:
            m_stats.saberDrillTime = m_stateTimer;
            clearDrillEntities();
            break;
        case Level1State::BLASTER_DRILL:
            m_stats.blasterDrillTime = m_stateTimer;
            clearDrillEntities();
            break;
        case Level1State::MIXED_DRILL:
            m_stats.mixedDrillTime = m_stateTimer;
            clearDrillEntities();
            break;
        default:
            break;
    }
}

// =============================================================================
// State Update Handlers
// =============================================================================

void Level1TrainingSystem::updateIntro(float deltaTime) {
    (void)deltaTime;  // Unused for now

    // Progress messages at intervals
    if (m_stateTimer >= 3.0f && m_stateTimer < 3.1f) {
        LOG_INFO(LOG_TAG_LEVEL1) << "Get ready...";
    }
    if (m_stateTimer >= 6.0f && m_stateTimer < 6.1f) {
        LOG_INFO(LOG_TAG_LEVEL1) << "Training begins soon...";
    }

    // Transition when timer expires
    if (m_stateTimer >= INTRO_DURATION) {
        transitionTo(Level1State::SABER_DRILL);
    }
}

void Level1TrainingSystem::updateSaberDrill(float deltaTime) {
    // Get player and saber position
    Vec3 playerPos = m_engine ? m_engine->getHeadPose().position : Vec3(0, 1.5f, 0);
    Vec3 saberPos = playerPos;  // Will be updated from right controller if available

    if (m_engine) {
        const auto& rightCtrl = m_engine->getRightController();
        if (rightCtrl.isTracked) {
            saberPos = rightCtrl.pose.position;
        }
    }

    // Update entities
    updateDrones(deltaTime, playerPos);
    updateProjectiles(deltaTime, saberPos, SABER_RADIUS);
    cleanupDeadEntities();

    // Log progress periodically
    float progress = m_stateTimer / SABER_DRILL_DURATION * 100.0f;
    if (static_cast<int>(m_stateTimer) % 15 == 0 &&
        m_stateTimer - static_cast<int>(m_stateTimer) < 0.02f) {
        LOG_DEBUG(LOG_TAG_LEVEL1) << "Saber drill progress: " << static_cast<int>(progress) << "%"
                                   << " | Blocked: " << m_stats.projectilesBlocked
                                   << " | Missed: " << m_stats.projectilesMissed;
    }

    // Transition when timer expires
    if (m_stateTimer >= SABER_DRILL_DURATION) {
        transitionTo(Level1State::BLASTER_DRILL);
    }
}

void Level1TrainingSystem::updateBlasterDrill(float deltaTime) {
    // Get player position and blaster aim
    Vec3 playerPos = m_engine ? m_engine->getHeadPose().position : Vec3(0, 1.5f, 0);
    Vec3 blasterPos = playerPos;
    Vec3 blasterDir = Vec3(0, 0, -1);

    if (m_engine) {
        const auto& leftCtrl = m_engine->getLeftController();
        if (leftCtrl.isTracked) {
            blasterPos = leftCtrl.pose.position;
            blasterDir = leftCtrl.pose.orientation.rotate(Vec3(0, 0, -1));

            // Check for trigger press to fire
            if (leftCtrl.triggerPressed) {
                checkBlasterHits(blasterPos, blasterDir);
                m_stats.shotsFired++;
            }
        }
    }

    // Update drones (no projectiles in blaster drill - drones are just targets)
    updateDrones(deltaTime, playerPos);
    cleanupDeadEntities();

    // Log progress periodically
    float progress = m_stateTimer / BLASTER_DRILL_DURATION * 100.0f;
    if (static_cast<int>(m_stateTimer) % 15 == 0 &&
        m_stateTimer - static_cast<int>(m_stateTimer) < 0.02f) {
        LOG_DEBUG(LOG_TAG_LEVEL1) << "Blaster drill progress: " << static_cast<int>(progress) << "%"
                                   << " | Hits: " << m_stats.shotsHit
                                   << " / " << m_stats.shotsFired;
    }

    // Transition when timer expires
    if (m_stateTimer >= BLASTER_DRILL_DURATION) {
        transitionTo(Level1State::MIXED_DRILL);
    }
}

void Level1TrainingSystem::updateMixedDrill(float deltaTime) {
    // Get player positions
    Vec3 playerPos = m_engine ? m_engine->getHeadPose().position : Vec3(0, 1.5f, 0);
    Vec3 saberPos = playerPos;
    Vec3 blasterPos = playerPos;
    Vec3 blasterDir = Vec3(0, 0, -1);

    if (m_engine) {
        const auto& rightCtrl = m_engine->getRightController();
        if (rightCtrl.isTracked) {
            saberPos = rightCtrl.pose.position;
        }

        const auto& leftCtrl = m_engine->getLeftController();
        if (leftCtrl.isTracked) {
            blasterPos = leftCtrl.pose.position;
            blasterDir = leftCtrl.pose.orientation.rotate(Vec3(0, 0, -1));

            // Check for trigger press to fire
            if (leftCtrl.triggerPressed) {
                checkBlasterHits(blasterPos, blasterDir);
                m_stats.shotsFired++;
            }
        }
    }

    // Update all entities
    updateDrones(deltaTime, playerPos);
    updateProjectiles(deltaTime, saberPos, SABER_RADIUS);
    cleanupDeadEntities();

    // Trigger dive attack at ~30s mark
    if (!m_diveAttackTriggered && m_stateTimer >= DIVE_ATTACK_TIME) {
        m_diveAttackTriggered = true;
        LOG_INFO(LOG_TAG_LEVEL1) << ">>> DIVE ATTACK INCOMING! <<<";

        // Find or spawn the dive drone and trigger attack
        for (auto& drone : m_drones) {
            if (drone && drone->isAlive() && drone->getBehavior() == DroneBehavior::SLOW_DIVE) {
                drone->startDiveAttack(playerPos);
                break;
            }
        }
    }

    // Check for dive attack collision with player
    for (auto& drone : m_drones) {
        if (drone && drone->isDiving()) {
            Vec3 dronePos = drone->getPosition();
            Vec3 diff = dronePos - playerPos;
            float dist = diff.length();
            if (dist < 0.5f) {  // Hit player
                LOG_INFO(LOG_TAG_LEVEL1) << "Player HIT by dive attack!";
                m_stats.divesHit++;
            } else if (drone->isDiveComplete()) {
                LOG_INFO(LOG_TAG_LEVEL1) << "Player DODGED the dive attack!";
                m_stats.divesDodged++;
            }
        }
    }

    // Log progress periodically
    float progress = m_stateTimer / MIXED_DRILL_DURATION * 100.0f;
    if (static_cast<int>(m_stateTimer) % 15 == 0 &&
        m_stateTimer - static_cast<int>(m_stateTimer) < 0.02f) {
        LOG_DEBUG(LOG_TAG_LEVEL1) << "Mixed drill progress: " << static_cast<int>(progress) << "%"
                                   << " | Blocked: " << m_stats.projectilesBlocked
                                   << " | Hits: " << m_stats.shotsHit;
    }

    // Transition when timer expires
    if (m_stateTimer >= MIXED_DRILL_DURATION) {
        transitionTo(Level1State::SUMMARY);
    }
}

void Level1TrainingSystem::updateSummary(float deltaTime) {
    (void)deltaTime;

    // Transition to completed when timer expires
    if (m_stateTimer >= SUMMARY_DURATION) {
        m_stats.totalTime = m_totalLevelTime;
        transitionTo(Level1State::COMPLETED);
    }
}

// =============================================================================
// Scene Setup
// =============================================================================

void Level1TrainingSystem::setupDojoEnvironment() {
    if (!m_engine || m_environmentSetup) return;

    LOG_DEBUG(LOG_TAG_LEVEL1) << "Setting up dojo environment...";

    // Floor
    SceneObject floor;
    floor.name = "Level1_Floor";
    floor.mesh = Mesh::createCube(1.0f);
    floor.transform.position = Vec3(0, -0.01f, 0);
    floor.transform.scale = Vec3(6.0f, 0.02f, 6.0f);
    floor.material.baseColor = Color(0.15f, 0.15f, 0.2f);  // Dark blue-gray
    m_engine->addSceneObject(floor);

    // Corner pillars
    const float pillarHeight = 3.0f;
    const float pillarSize = 0.2f;
    const float arenaRadius = 2.5f;
    Color pillarColor(0.3f, 0.1f, 0.1f);  // Dark red

    for (int i = 0; i < 4; i++) {
        float angle = (i * 90.0f + 45.0f) * 3.14159f / 180.0f;
        float x = arenaRadius * std::cos(angle);
        float z = arenaRadius * std::sin(angle);

        SceneObject pillar;
        pillar.name = "Level1_Pillar" + std::to_string(i);
        pillar.mesh = Mesh::createCube(1.0f);
        pillar.transform.position = Vec3(x, pillarHeight / 2, z);
        pillar.transform.scale = Vec3(pillarSize, pillarHeight, pillarSize);
        pillar.material.baseColor = pillarColor;
        m_engine->addSceneObject(pillar);
    }

    // Center training marker (subtle ring on floor)
    SceneObject marker;
    marker.name = "Level1_Marker";
    marker.mesh = Mesh::createCube(1.0f);
    marker.transform.position = Vec3(0, 0.005f, 0);
    marker.transform.scale = Vec3(1.5f, 0.01f, 1.5f);
    marker.material.baseColor = Color(0.2f, 0.3f, 0.4f);
    m_engine->addSceneObject(marker);

    m_environmentSetup = true;
    LOG_DEBUG(LOG_TAG_LEVEL1) << "Dojo environment setup complete";
}

void Level1TrainingSystem::setupWeapons() {
    if (!m_engine || m_weaponsSetup) return;

    LOG_DEBUG(LOG_TAG_LEVEL1) << "Setting up weapons...";

    // Saber (right hand)
    SceneObject saber;
    saber.name = "Level1_Saber";
    saber.mesh = Mesh::createCube(1.0f);
    saber.transform.scale = Vec3(0.025f, 0.025f, 0.8f);
    saber.material.baseColor = Color(0.2f, 0.9f, 1.0f);  // Cyan glow
    saber.material.emissive = 0.7f;
    m_engine->addSceneObject(saber);

    // Blaster (left hand)
    SceneObject blaster;
    blaster.name = "Level1_Blaster";
    blaster.mesh = Mesh::createCube(1.0f);
    blaster.transform.scale = Vec3(0.04f, 0.08f, 0.15f);
    blaster.material.baseColor = Color(0.35f, 0.35f, 0.4f);  // Metallic
    m_engine->addSceneObject(blaster);

    m_weaponsSetup = true;
    LOG_DEBUG(LOG_TAG_LEVEL1) << "Weapons setup complete";
}

void Level1TrainingSystem::clearDrillEntities() {
    LOG_DEBUG(LOG_TAG_LEVEL1) << "Clearing drill entities: "
                               << m_drones.size() << " drones, "
                               << m_projectiles.size() << " projectiles";

    // Remove scene objects
    if (m_engine) {
        for (const auto& name : m_drillEntityNames) {
            m_engine->removeSceneObject(name);
        }
    }
    m_drillEntityNames.clear();

    // Clear entity containers
    m_drones.clear();
    m_projectiles.clear();

    LOG_TRACE(LOG_TAG_LEVEL1) << "Cleared drill entities";
}

// =============================================================================
// Rendering
// =============================================================================

void Level1TrainingSystem::renderStateHUD(const FrameContext& ctx) {
    if (!m_engine) return;

    // Update weapon positions to follow controllers
    if (ctx.rightController.isTracked) {
        if (auto* saber = m_engine->getSceneObject("Level1_Saber")) {
            Transform saberPose = ctx.rightController.pose;
            Vec3 forward = saberPose.orientation.rotate(Vec3(0, 0, -0.4f));
            saber->transform.position = saberPose.position + forward;
            saber->transform.orientation = saberPose.orientation;
        }
    }

    if (ctx.leftController.isTracked) {
        if (auto* blaster = m_engine->getSceneObject("Level1_Blaster")) {
            Transform blasterPose = ctx.leftController.pose;
            Vec3 forward = blasterPose.orientation.rotate(Vec3(0, 0, -0.08f));
            blaster->transform.position = blasterPose.position + forward;
            blaster->transform.orientation = blasterPose.orientation;
        }
    }

    // Draw state-specific overlays
    switch (m_currentState) {
        case Level1State::INTRO:
            renderIntroText(ctx);
            break;
        case Level1State::SABER_DRILL:
        case Level1State::BLASTER_DRILL:
        case Level1State::MIXED_DRILL:
            renderDrillHUD(ctx);
            break;
        case Level1State::SUMMARY:
            renderSummary(ctx);
            break;
        default:
            break;
    }
}

void Level1TrainingSystem::renderIntroText(const FrameContext& ctx) {
    // Draw intro guidance using debug lines
    // Position floating text in front of player
    Vec3 textPos = ctx.headPose.position + Vec3(0, 0.2f, -1.5f);

    // Draw a simple "look here" indicator
    m_engine->drawLine(
        textPos + Vec3(-0.3f, 0, 0),
        textPos + Vec3(0.3f, 0, 0),
        Color::cyan(), 3.0f
    );
    m_engine->drawLine(
        textPos + Vec3(0, -0.1f, 0),
        textPos + Vec3(0, 0.1f, 0),
        Color::cyan(), 3.0f
    );
}

void Level1TrainingSystem::renderDrillHUD(const FrameContext& ctx) {
    // Draw timer indicator in peripheral vision
    float progress = m_stateTimer / getStateDuration();
    Vec3 hudPos = ctx.headPose.position + Vec3(0.8f, 0.3f, -1.0f);

    // Progress bar (simple line that shrinks)
    float barLength = 0.3f * (1.0f - progress);
    m_engine->drawLine(
        hudPos,
        hudPos + Vec3(barLength, 0, 0),
        Color::green(), 4.0f
    );
}

void Level1TrainingSystem::renderSummary(const FrameContext& ctx) {
    // Draw completion indicator
    Vec3 centerPos = ctx.headPose.position + Vec3(0, 0, -2.0f);

    // Draw a celebratory "star" pattern
    for (int i = 0; i < 8; i++) {
        float angle = i * 45.0f * 3.14159f / 180.0f;
        Vec3 dir(std::cos(angle), std::sin(angle), 0);
        m_engine->drawLine(
            centerPos,
            centerPos + dir * 0.3f,
            Color(1.0f, 0.8f, 0.2f), 3.0f  // Gold
        );
    }
}

// =============================================================================
// Logging
// =============================================================================

void Level1TrainingSystem::logStateTransition(Level1State from, Level1State to) {
    LOG_INFO(LOG_TAG_LEVEL1) << "State transition: "
                             << level1StateToString(from) << " -> "
                             << level1StateToString(to);
}

void Level1TrainingSystem::logLevelSummary() {
    LOG_INFO(LOG_TAG_LEVEL1) << "----------------------------------------";
    LOG_INFO(LOG_TAG_LEVEL1) << "LEVEL 1 SUMMARY";
    LOG_INFO(LOG_TAG_LEVEL1) << "----------------------------------------";
    LOG_INFO(LOG_TAG_LEVEL1) << "Total time: " << m_totalLevelTime << "s";
    LOG_INFO(LOG_TAG_LEVEL1) << "";
    LOG_INFO(LOG_TAG_LEVEL1) << "Phase times:";
    LOG_INFO(LOG_TAG_LEVEL1) << "  Intro:        " << m_stats.introTime << "s";
    LOG_INFO(LOG_TAG_LEVEL1) << "  Saber Drill:  " << m_stats.saberDrillTime << "s";
    LOG_INFO(LOG_TAG_LEVEL1) << "  Blaster Drill:" << m_stats.blasterDrillTime << "s";
    LOG_INFO(LOG_TAG_LEVEL1) << "  Mixed Drill:  " << m_stats.mixedDrillTime << "s";
    LOG_INFO(LOG_TAG_LEVEL1) << "";
    LOG_INFO(LOG_TAG_LEVEL1) << "Combat stats:";
    LOG_INFO(LOG_TAG_LEVEL1) << "  Targets hit:      " << m_stats.targetsHit
                             << " (accuracy: " << m_stats.getSaberAccuracy() << "%)";
    LOG_INFO(LOG_TAG_LEVEL1) << "  Projectiles blocked: " << m_stats.projectilesBlocked
                             << " (accuracy: " << m_stats.getBlockAccuracy() << "%)";
    LOG_INFO(LOG_TAG_LEVEL1) << "  Shots hit:        " << m_stats.shotsHit << "/" << m_stats.shotsFired
                             << " (accuracy: " << m_stats.getBlasterAccuracy() << "%)";
    LOG_INFO(LOG_TAG_LEVEL1) << "  Dives dodged:     " << m_stats.divesDodged << "/"
                             << (m_stats.divesDodged + m_stats.divesHit);
    LOG_INFO(LOG_TAG_LEVEL1) << "----------------------------------------";
}

// =============================================================================
// Entity Management
// =============================================================================

void Level1TrainingSystem::spawnDrone(const DroneConfig& config) {
    if (static_cast<int>(m_drones.size()) >= MAX_DRONES) {
        LOG_WARN(LOG_TAG_LEVEL1) << "Max drones (" << MAX_DRONES << ") reached, cannot spawn more";
        return;
    }

    int id = m_nextDroneId++;
    auto drone = std::make_unique<Drone>(id, config);

    // Add scene object
    if (m_engine) {
        SceneObject obj = drone->createSceneObject();
        m_engine->addSceneObject(obj);
        m_drillEntityNames.push_back(obj.name);
    }

    LOG_INFO(LOG_TAG_LEVEL1) << "Spawned drone " << id
                             << " behavior=" << droneBehaviorToString(config.behavior)
                             << " at (" << config.spawnPosition.x << ", "
                             << config.spawnPosition.y << ", " << config.spawnPosition.z << ")";

    m_drones.push_back(std::move(drone));
}

void Level1TrainingSystem::spawnProjectile(const ProjectileConfig& config) {
    if (static_cast<int>(m_projectiles.size()) >= MAX_PROJECTILES) {
        LOG_WARN(LOG_TAG_LEVEL1) << "Max projectiles (" << MAX_PROJECTILES << ") reached, cannot spawn more";
        return;
    }

    int id = m_nextProjectileId++;
    auto projectile = std::make_unique<Projectile>(id, config);

    // Add scene object
    if (m_engine) {
        SceneObject obj = projectile->createSceneObject();
        m_engine->addSceneObject(obj);
        m_drillEntityNames.push_back(obj.name);
    }

    LOG_DEBUG(LOG_TAG_LEVEL1) << "Spawned projectile " << id;
    m_projectiles.push_back(std::move(projectile));
}

void Level1TrainingSystem::updateDrones(float deltaTime, const Vec3& playerPosition) {
    for (auto& drone : m_drones) {
        if (!drone || !drone->isAlive()) continue;

        drone->update(deltaTime, playerPosition);

        // Update scene object position
        if (m_engine) {
            if (auto* obj = m_engine->getSceneObject(drone->getSceneObjectName())) {
                obj->transform.position = drone->getPosition();
                obj->transform.orientation = drone->getOrientation();

                // Visual feedback for dive telegraph
                if (drone->getState() == DroneState::TELEGRAPH) {
                    obj->material.emissive = 0.8f;  // Bright warning
                    obj->material.baseColor = Color(1.0f, 0.0f, 0.0f);  // Flash red
                } else if (drone->getState() == DroneState::DIVING) {
                    obj->material.emissive = 1.0f;
                }
            }
        }

        // Fire projectiles if able (for shooting drones)
        if (drone->canFire() && drone->getBehavior() != DroneBehavior::SLOW_DIVE) {
            fireProjectileFromDrone(*drone, playerPosition);
        }
    }
}

void Level1TrainingSystem::updateProjectiles(float deltaTime, const Vec3& saberPosition, float saberRadius) {
    for (auto& projectile : m_projectiles) {
        if (!projectile || !projectile->isActive()) continue;

        bool stillActive = projectile->update(deltaTime, saberPosition, saberRadius);

        // Update scene object position
        if (m_engine && stillActive) {
            if (auto* obj = m_engine->getSceneObject(projectile->getSceneObjectName())) {
                obj->transform.position = projectile->getPosition();
            }
        }

        // Track stats for blocked/missed
        if (!stillActive) {
            if (projectile->wasBlocked()) {
                m_stats.projectilesBlocked++;
                LOG_DEBUG(LOG_TAG_LEVEL1) << "Projectile blocked! Total: " << m_stats.projectilesBlocked;
            } else if (projectile->wasMissed()) {
                m_stats.projectilesMissed++;
                LOG_DEBUG(LOG_TAG_LEVEL1) << "Projectile missed! Total: " << m_stats.projectilesMissed;
            }
        }
    }
}

void Level1TrainingSystem::cleanupDeadEntities() {
    // Remove dead drones
    for (auto it = m_drones.begin(); it != m_drones.end(); ) {
        if (!(*it) || !(*it)->isAlive()) {
            if (*it && m_engine) {
                m_engine->removeSceneObject((*it)->getSceneObjectName());
            }
            it = m_drones.erase(it);
        } else {
            ++it;
        }
    }

    // Remove inactive projectiles
    for (auto it = m_projectiles.begin(); it != m_projectiles.end(); ) {
        if (!(*it) || !(*it)->isActive()) {
            if (*it && m_engine) {
                m_engine->removeSceneObject((*it)->getSceneObjectName());
            }
            it = m_projectiles.erase(it);
        } else {
            ++it;
        }
    }
}

void Level1TrainingSystem::fireProjectileFromDrone(Drone& drone, const Vec3& targetPosition) {
    Vec3 dronePos = drone.getPosition();
    Vec3 direction = targetPosition - dronePos;

    ProjectileConfig config;
    config.spawnPosition = dronePos;
    config.direction = direction;
    config.speed = 2.0f;  // Very slow - easy to block
    config.lifetime = 5.0f;
    config.color = Color(1.0f, 0.3f, 0.3f);  // Red glow

    spawnProjectile(config);
    drone.resetFireTimer();

    LOG_DEBUG(LOG_TAG_LEVEL1) << "Drone " << drone.getId() << " fired projectile";
}

void Level1TrainingSystem::checkBlasterHits(const Vec3& blasterPosition, const Vec3& blasterDirection) {
    // Simple raycast check against drones
    for (auto& drone : m_drones) {
        if (!drone || !drone->isAlive()) continue;

        Vec3 dronePos = drone->getPosition();
        Vec3 toTarget = dronePos - blasterPosition;

        // Project onto ray
        float t = Vec3::dot(toTarget, blasterDirection);
        if (t < 0 || t > BLASTER_RANGE) continue;  // Behind or too far

        // Check perpendicular distance
        Vec3 closestPoint = blasterPosition + blasterDirection * t;
        Vec3 diff = dronePos - closestPoint;
        float dist = diff.length();

        if (dist < BLASTER_RADIUS) {
            LOG_INFO(LOG_TAG_LEVEL1) << "Blaster HIT drone " << drone->getId() << "!";
            drone->takeDamage(1);
            m_stats.shotsHit++;
            m_stats.targetsHit++;
            return;  // One hit per shot
        }
    }
}

// =============================================================================
// Phase-Specific Entity Spawning
// =============================================================================

void Level1TrainingSystem::spawnSaberDrillEntities() {
    LOG_INFO(LOG_TAG_LEVEL1) << "Spawning saber drill entities...";

    // Spawn 1-2 drones that fire slow projectiles
    DroneConfig config1;
    config1.behavior = DroneBehavior::HOVER;
    config1.spawnPosition = Vec3(-1.5f, 1.5f, -3.0f);
    config1.fireInterval = 3.0f;  // Slow fire rate
    config1.canFire = true;
    spawnDrone(config1);

    DroneConfig config2;
    config2.behavior = DroneBehavior::HOVER;
    config2.spawnPosition = Vec3(1.5f, 1.8f, -3.5f);
    config2.fireInterval = 4.0f;
    config2.canFire = true;
    spawnDrone(config2);

    LOG_INFO(LOG_TAG_LEVEL1) << "Saber drill: " << m_drones.size() << " drones spawned";
}

void Level1TrainingSystem::spawnBlasterDrillEntities() {
    LOG_INFO(LOG_TAG_LEVEL1) << "Spawning blaster drill entities...";

    // Spawn 2 stationary + 1 orbiting drone as targets
    DroneConfig hover1;
    hover1.behavior = DroneBehavior::HOVER;
    hover1.spawnPosition = Vec3(-2.0f, 1.3f, -3.0f);
    hover1.canFire = false;  // Just targets
    spawnDrone(hover1);

    DroneConfig hover2;
    hover2.behavior = DroneBehavior::HOVER;
    hover2.spawnPosition = Vec3(2.0f, 1.7f, -3.5f);
    hover2.canFire = false;
    spawnDrone(hover2);

    DroneConfig orbit1;
    orbit1.behavior = DroneBehavior::SLOW_ORBIT;
    orbit1.spawnPosition = Vec3(0, 2.0f, -4.0f);
    orbit1.orbitRadius = 1.5f;
    orbit1.orbitSpeed = 0.5f;  // Slow orbit
    orbit1.canFire = false;
    spawnDrone(orbit1);

    LOG_INFO(LOG_TAG_LEVEL1) << "Blaster drill: " << m_drones.size() << " drones spawned";
}

void Level1TrainingSystem::spawnMixedDrillEntities() {
    LOG_INFO(LOG_TAG_LEVEL1) << "Spawning mixed drill entities...";

    // 1 shooting drone
    DroneConfig shooter;
    shooter.behavior = DroneBehavior::SLOW_ORBIT;
    shooter.spawnPosition = Vec3(0, 1.5f, -3.0f);
    shooter.orbitRadius = 2.0f;
    shooter.orbitSpeed = 0.3f;
    shooter.fireInterval = 2.5f;
    shooter.canFire = true;
    spawnDrone(shooter);

    // 1 target drone
    DroneConfig target;
    target.behavior = DroneBehavior::HOVER;
    target.spawnPosition = Vec3(-1.5f, 2.0f, -4.0f);
    target.canFire = false;
    spawnDrone(target);

    // 1 dive attack drone (will attack at 30s mark)
    DroneConfig diver;
    diver.behavior = DroneBehavior::SLOW_DIVE;
    diver.spawnPosition = Vec3(0, 2.5f, -5.0f);
    diver.canFire = false;
    spawnDrone(diver);

    LOG_INFO(LOG_TAG_LEVEL1) << "Mixed drill: " << m_drones.size() << " drones spawned (1 will dive at 30s)";
}

} // namespace lst
