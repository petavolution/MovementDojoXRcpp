#include "TrainingModule.h"
#include <iostream>
#include <cmath>

namespace lst {

// ============================================================================
// TrainingModule (Base)
// ============================================================================

TrainingModule::TrainingModule(const std::string& name)
    : m_name(name)
{
    m_result.exerciseName = name;
}

TrainingModule::~TrainingModule() = default;

void TrainingModule::start() {
    m_state = ExerciseState::Starting;
    m_progress = 0.0f;
    m_elapsedTime = 0.0f;
    m_timeRemaining = m_duration;

    m_result = ExerciseResult();
    m_result.exerciseName = m_name;

    std::cout << "Starting exercise: " << m_name << std::endl;
    m_state = ExerciseState::Running;
}

void TrainingModule::stop() {
    m_state = ExerciseState::Inactive;
    std::cout << "Stopped exercise: " << m_name << std::endl;
}

void TrainingModule::pause() {
    if (m_state == ExerciseState::Running) {
        m_state = ExerciseState::Paused;
        std::cout << "Paused exercise: " << m_name << std::endl;
    }
}

void TrainingModule::resume() {
    if (m_state == ExerciseState::Paused) {
        m_state = ExerciseState::Running;
        std::cout << "Resumed exercise: " << m_name << std::endl;
    }
}

void TrainingModule::reset() {
    m_progress = 0.0f;
    m_elapsedTime = 0.0f;
    m_timeRemaining = m_duration;
    m_state = ExerciseState::Inactive;
}

void TrainingModule::update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController) {
    (void)leftController;
    (void)rightController;

    if (m_state != ExerciseState::Running) return;

    m_elapsedTime += static_cast<float>(deltaTime);
    m_timeRemaining = m_duration - m_elapsedTime;

    if (m_timeRemaining <= 0) {
        complete(m_progress >= 1.0f);
    }
}

void TrainingModule::onSaberCollision(const Vec3& point, const std::string& targetName) {
    (void)point;
    (void)targetName;
    // Override in subclasses
}

void TrainingModule::requestFeedback(const std::string& type, const Vec3& position, const Color& color) {
    if (m_feedbackCallback) {
        m_feedbackCallback(type, position, color);
    }
}

void TrainingModule::requestHaptic(Hand hand, float intensity, float duration) {
    if (m_hapticCallback) {
        m_hapticCallback(hand, intensity, duration);
    }
}

void TrainingModule::setProgress(float progress) {
    m_progress = std::max(0.0f, std::min(1.0f, progress));
}

void TrainingModule::complete(bool passed) {
    m_state = passed ? ExerciseState::Completed : ExerciseState::Failed;
    m_result.passed = passed;
    m_result.score = m_progress * 100.0f;
    m_result.completionTime = m_elapsedTime;

    std::cout << "Exercise " << (passed ? "completed" : "failed") << ": " << m_name
              << " (score: " << m_result.score << ")" << std::endl;

    // Haptic feedback
    if (passed) {
        requestHaptic(Hand::Left, 0.5f, 0.2f);
        requestHaptic(Hand::Right, 0.5f, 0.2f);
    } else {
        requestHaptic(Hand::Left, 0.8f, 0.5f);
        requestHaptic(Hand::Right, 0.8f, 0.5f);
    }
}

// ============================================================================
// PostureHoldExercise
// ============================================================================

PostureHoldExercise::PostureHoldExercise()
    : TrainingModule("Posture Hold")
{
    m_duration = 30.0f;
}

void PostureHoldExercise::setTargetPose(const Transform& pose, float toleranceDegrees) {
    m_targetPose = pose;
    m_toleranceDegrees = toleranceDegrees;
}

void PostureHoldExercise::update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController) {
    if (m_state != ExerciseState::Running) return;

    TrainingModule::update(deltaTime, leftController, rightController);

    // Use right controller for saber
    const ControllerState& saberController = rightController;

    if (!saberController.isTracked) return;

    // Calculate angle difference between current and target orientation
    Quat currentOri = saberController.pose.orientation;
    Quat targetOri = m_targetPose.orientation;

    // Calculate the rotation from current to target
    Quat diff = targetOri * currentOri.conjugate();
    float angle = 2.0f * std::acos(std::abs(diff.w)) * 180.0f / 3.14159265359f;

    // Also check position if specified
    Vec3 posDiff = saberController.pose.position - m_targetPose.position;
    float posDistance = posDiff.length();

    // Check if within tolerance
    bool angleOk = angle < m_toleranceDegrees;
    bool positionOk = posDistance < 0.15f; // 15cm tolerance
    m_isInPosition = angleOk && positionOk;

    if (m_isInPosition) {
        m_holdTime += static_cast<float>(deltaTime);

        // Gentle haptic pulse when in position
        if (static_cast<int>(m_holdTime * 2) % 2 == 0) {
            requestHaptic(Hand::Right, 0.1f, 0.05f);
        }

        // Visual feedback - green glow
        requestFeedback("glow", saberController.pose.position, Color::green());
    } else {
        m_holdTime = 0.0f;

        // Gentle correction haptic
        requestHaptic(Hand::Right, 0.2f, 0.1f);

        // Visual feedback - red indicator
        requestFeedback("indicator", m_targetPose.position, Color::red());
    }

    // Update progress based on hold time
    setProgress(m_holdTime / m_requiredHoldTime);

    if (m_holdTime >= m_requiredHoldTime) {
        complete(true);
    }
}

// ============================================================================
// PathFollowExercise
// ============================================================================

PathFollowExercise::PathFollowExercise()
    : TrainingModule("Path Follow")
{
    m_duration = 60.0f;
}

void PathFollowExercise::setPath(const std::vector<Vec3>& path, float toleranceMeters) {
    m_path = path;
    m_toleranceMeters = toleranceMeters;
    m_currentPathIndex = 0;

    // Calculate total path length
    m_totalPathLength = 0.0f;
    for (size_t i = 1; i < path.size(); i++) {
        m_totalPathLength += (path[i] - path[i - 1]).length();
    }
}

void PathFollowExercise::update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController) {
    if (m_state != ExerciseState::Running) return;

    TrainingModule::update(deltaTime, leftController, rightController);

    const ControllerState& saberController = rightController;

    if (!saberController.isTracked || m_path.empty()) return;

    // Get saber tip position (offset from controller)
    Vec3 saberTip = saberController.pose.transformPoint(Vec3(0, 0, -0.5f)); // 50cm blade

    // Find distance to current path point
    if (m_currentPathIndex < static_cast<int>(m_path.size())) {
        Vec3 targetPoint = m_path[m_currentPathIndex];
        float distance = (saberTip - targetPoint).length();

        if (distance < m_toleranceMeters) {
            // Reached this point, move to next
            m_currentPathIndex++;

            // Success haptic
            requestHaptic(Hand::Right, 0.3f, 0.1f);
            requestFeedback("sparkle", targetPoint, Color::cyan());

            // Update progress
            setProgress(static_cast<float>(m_currentPathIndex) / static_cast<float>(m_path.size()));
        } else {
            // Show guide to next point
            requestFeedback("pathGuide", targetPoint, Color::blue());
        }
    }

    if (m_currentPathIndex >= static_cast<int>(m_path.size())) {
        complete(true);
    }
}

// ============================================================================
// TargetBlockExercise
// ============================================================================

TargetBlockExercise::TargetBlockExercise()
    : TrainingModule("Target Block")
{
    m_duration = 60.0f;
}

void TargetBlockExercise::setTargetCount(int count) {
    m_targetCount = count;
}

void TargetBlockExercise::setSpawnRate(float targetsPerSecond) {
    m_spawnRate = targetsPerSecond;
}

void TargetBlockExercise::update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController) {
    if (m_state != ExerciseState::Running) return;

    TrainingModule::update(deltaTime, leftController, rightController);
    (void)leftController;
    (void)rightController;

    float dt = static_cast<float>(deltaTime);

    // Spawn new targets
    m_spawnTimer += dt;
    float spawnInterval = 1.0f / m_spawnRate;

    if (m_spawnTimer >= spawnInterval && m_targetsSpawned < m_targetCount) {
        m_spawnTimer = 0.0f;

        Target target;
        target.name = "target_" + std::to_string(m_targetsSpawned);

        // Spawn in front of user at random position
        float angle = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
        float height = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.8f;

        target.position = Vec3(std::sin(angle) * 3.0f, height, -3.0f);
        target.velocity = Vec3(0, 0, 1.5f); // Move towards user
        target.radius = 0.15f;
        target.active = true;

        m_targets.push_back(target);
        m_targetsSpawned++;

        requestFeedback("spawn", target.position, Color::red());
    }

    // Update targets
    for (auto& target : m_targets) {
        if (!target.active) continue;

        target.position = target.position + target.velocity * dt;

        // Check if target passed the user (missed)
        if (target.position.z > 1.0f) {
            target.active = false;
            m_targetsMissed++;

            requestHaptic(Hand::Left, 0.5f, 0.3f);
            requestHaptic(Hand::Right, 0.5f, 0.3f);
            requestFeedback("miss", target.position, Color::red());
        }
    }

    // Update progress
    int totalProcessed = m_targetsHit + m_targetsMissed;
    if (totalProcessed > 0) {
        setProgress(static_cast<float>(totalProcessed) / static_cast<float>(m_targetCount));
        m_result.accuracy = static_cast<float>(m_targetsHit) / static_cast<float>(totalProcessed);
    }

    m_result.targetsHit = m_targetsHit;
    m_result.targetsMissed = m_targetsMissed;

    // Check completion
    if (totalProcessed >= m_targetCount) {
        complete(m_result.accuracy >= 0.7f); // 70% accuracy to pass
    }
}

void TargetBlockExercise::onSaberCollision(const Vec3& point, const std::string& targetName) {
    for (auto& target : m_targets) {
        if (target.name == targetName && target.active) {
            target.active = false;
            m_targetsHit++;

            requestHaptic(Hand::Right, 0.4f, 0.15f);
            requestFeedback("hit", point, Color::green());
            break;
        }
    }
}

// ============================================================================
// TrainingManager
// ============================================================================

TrainingManager::TrainingManager() {
    // Register default modules
    registerModule(std::make_unique<PostureHoldExercise>());
    registerModule(std::make_unique<PathFollowExercise>());
    registerModule(std::make_unique<TargetBlockExercise>());
}

TrainingManager::~TrainingManager() = default;

void TrainingManager::registerModule(std::unique_ptr<TrainingModule> module) {
    std::cout << "Registered training module: " << module->getName() << std::endl;
    m_modules.push_back(std::move(module));
}

TrainingModule* TrainingManager::getModule(const std::string& name) {
    for (auto& module : m_modules) {
        if (module->getName() == name) {
            return module.get();
        }
    }
    return nullptr;
}

bool TrainingManager::startExercise(const std::string& name) {
    stopCurrentExercise();

    TrainingModule* module = getModule(name);
    if (!module) {
        std::cerr << "Training module not found: " << name << std::endl;
        return false;
    }

    m_currentExercise = module;
    m_currentExercise->start();
    return true;
}

void TrainingManager::stopCurrentExercise() {
    if (m_currentExercise) {
        // Save result if exercise was running
        if (m_currentExercise->getState() == ExerciseState::Running ||
            m_currentExercise->getState() == ExerciseState::Completed ||
            m_currentExercise->getState() == ExerciseState::Failed) {
            m_history.push_back(m_currentExercise->getResult());
        }

        m_currentExercise->stop();
        m_currentExercise = nullptr;
    }
}

void TrainingManager::update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController) {
    if (m_currentExercise && m_currentExercise->getState() == ExerciseState::Running) {
        m_currentExercise->update(deltaTime, leftController, rightController);
    }
}

} // namespace lst
