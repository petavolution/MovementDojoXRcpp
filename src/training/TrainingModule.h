#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace lst {

/**
 * TrainingModule - Base class for proprioception training exercises
 *
 * Exercises include:
 * - Posture holding (maintain saber at specific angles)
 * - Path following (trace 3D paths with saber tip)
 * - Target blocking (deflect incoming projectiles)
 * - Vader Dojo style wave combat
 */

// Exercise state
enum class ExerciseState {
    Inactive,
    Starting,
    Running,
    Paused,
    Completed,
    Failed
};

// Exercise result
struct ExerciseResult {
    std::string exerciseName;
    float score;
    float accuracy;
    float completionTime;
    int targetsHit;
    int targetsMissed;
    bool passed;
};

// Base training module
class TrainingModule {
public:
    TrainingModule(const std::string& name);
    virtual ~TrainingModule();

    // Lifecycle
    virtual void start();
    virtual void stop();
    virtual void pause();
    virtual void resume();
    virtual void reset();

    // Per-frame update
    virtual void update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController);

    // Feedback
    virtual void onSaberCollision(const Vec3& point, const std::string& targetName);

    // State
    ExerciseState getState() const { return m_state; }
    const std::string& getName() const { return m_name; }
    float getProgress() const { return m_progress; }
    float getTimeRemaining() const { return m_timeRemaining; }
    const ExerciseResult& getResult() const { return m_result; }

    // Visual feedback requests
    using FeedbackCallback = std::function<void(const std::string& type, const Vec3& position, const Color& color)>;
    void setFeedbackCallback(FeedbackCallback callback) { m_feedbackCallback = callback; }

    // Haptic feedback requests
    using HapticCallback = std::function<void(Hand hand, float intensity, float duration)>;
    void setHapticCallback(HapticCallback callback) { m_hapticCallback = callback; }

protected:
    void requestFeedback(const std::string& type, const Vec3& position, const Color& color);
    void requestHaptic(Hand hand, float intensity, float duration);
    void setProgress(float progress);
    void complete(bool passed);

    std::string m_name;
    ExerciseState m_state = ExerciseState::Inactive;
    float m_progress = 0.0f;
    float m_duration = 30.0f;
    float m_timeRemaining = 0.0f;
    float m_elapsedTime = 0.0f;
    ExerciseResult m_result;

    FeedbackCallback m_feedbackCallback;
    HapticCallback m_hapticCallback;
};

/**
 * PostureHoldExercise - Hold saber at a specific position/orientation
 */
class PostureHoldExercise : public TrainingModule {
public:
    PostureHoldExercise();

    void setTargetPose(const Transform& pose, float toleranceDegrees = 5.0f);
    void update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController) override;

private:
    Transform m_targetPose;
    float m_toleranceDegrees = 5.0f;
    float m_holdTime = 0.0f;
    float m_requiredHoldTime = 3.0f;
    bool m_isInPosition = false;
};

/**
 * PathFollowExercise - Trace a 3D path with the saber tip
 */
class PathFollowExercise : public TrainingModule {
public:
    PathFollowExercise();

    void setPath(const std::vector<Vec3>& path, float toleranceMeters = 0.1f);
    void update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController) override;

private:
    std::vector<Vec3> m_path;
    float m_toleranceMeters = 0.1f;
    int m_currentPathIndex = 0;
    float m_totalPathLength = 0.0f;
};

/**
 * TargetBlockExercise - Block incoming targets
 */
class TargetBlockExercise : public TrainingModule {
public:
    TargetBlockExercise();

    void setTargetCount(int count);
    void setSpawnRate(float targetsPerSecond);
    void update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController) override;
    void onSaberCollision(const Vec3& point, const std::string& targetName) override;

private:
    struct Target {
        std::string name;
        Vec3 position;
        Vec3 velocity;
        float radius;
        bool active;
    };

    std::vector<Target> m_targets;
    int m_targetCount = 10;
    float m_spawnRate = 1.0f;
    float m_spawnTimer = 0.0f;
    int m_targetsSpawned = 0;
    int m_targetsHit = 0;
    int m_targetsMissed = 0;
};

/**
 * TrainingManager - Manages available training modules
 */
class TrainingManager {
public:
    TrainingManager();
    ~TrainingManager();

    // Module management
    void registerModule(std::unique_ptr<TrainingModule> module);
    TrainingModule* getModule(const std::string& name);
    const std::vector<std::unique_ptr<TrainingModule>>& getModules() const { return m_modules; }

    // Start/stop exercises
    bool startExercise(const std::string& name);
    void stopCurrentExercise();

    // Update active exercise
    void update(double deltaTime, const ControllerState& leftController, const ControllerState& rightController);

    // Get current exercise
    TrainingModule* getCurrentExercise() const { return m_currentExercise; }

    // Results history
    const std::vector<ExerciseResult>& getHistory() const { return m_history; }

private:
    std::vector<std::unique_ptr<TrainingModule>> m_modules;
    TrainingModule* m_currentExercise = nullptr;
    std::vector<ExerciseResult> m_history;
};

} // namespace lst
