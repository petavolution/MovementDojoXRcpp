#include "HapticManager.h"
#include "core/Input.h"
#include <iostream>
#include <algorithm>

namespace lst {

HapticManager::HapticManager() {
    // Define success pattern: quick double pulse
    m_successPattern = {
        {0.6f, 0.1f, 0.05f},
        {0.8f, 0.1f, 0.0f}
    };

    // Define failure pattern: long buzz
    m_failurePattern = {
        {0.4f, 0.3f, 0.0f}
    };

    // Define saber activate pattern: ramp up
    m_activatePattern = {
        {0.2f, 0.05f, 0.0f},
        {0.4f, 0.05f, 0.0f},
        {0.6f, 0.05f, 0.0f},
        {0.8f, 0.1f, 0.0f},
        {0.5f, 0.1f, 0.0f}
    };

    // Define saber deactivate pattern: ramp down
    m_deactivatePattern = {
        {0.6f, 0.05f, 0.0f},
        {0.4f, 0.05f, 0.0f},
        {0.2f, 0.1f, 0.0f}
    };
}

HapticManager::~HapticManager() {
    shutdown();
}

bool HapticManager::initialize(Input* input) {
    m_input = input;

    if (!m_input) {
        std::cerr << "HapticManager: Input system is null" << std::endl;
        return false;
    }

    std::cout << "HapticManager initialized" << std::endl;
    return true;
}

void HapticManager::shutdown() {
    stopAll();
    m_input = nullptr;
}

void HapticManager::pulse(Hand hand, float intensity, float duration) {
    QueuedHaptic haptic;
    haptic.hand = hand;
    haptic.intensity = std::clamp(intensity, 0.0f, 1.0f);
    haptic.duration = duration;
    haptic.delay = 0.0f;
    haptic.timeRemaining = duration;

    if (hand == Hand::Left) {
        m_leftQueue.push(haptic);
    } else {
        m_rightQueue.push(haptic);
    }
}

void HapticManager::buzz(Hand hand, float intensity, float duration) {
    // Buzz is just a quick pulse
    pulse(hand, intensity, duration);
}

void HapticManager::ramp(Hand hand, float startIntensity, float endIntensity, float duration) {
    // Create a series of pulses to simulate a ramp
    const int steps = 5;
    float stepDuration = duration / steps;

    for (int i = 0; i < steps; i++) {
        float t = static_cast<float>(i) / (steps - 1);
        float intensity = startIntensity + (endIntensity - startIntensity) * t;

        QueuedHaptic haptic;
        haptic.hand = hand;
        haptic.intensity = std::clamp(intensity, 0.0f, 1.0f);
        haptic.duration = stepDuration;
        haptic.delay = i * stepDuration;
        haptic.timeRemaining = stepDuration;

        if (hand == Hand::Left) {
            m_leftQueue.push(haptic);
        } else {
            m_rightQueue.push(haptic);
        }
    }
}

void HapticManager::playSuccessPattern(Hand hand) {
    playPattern(hand, m_successPattern);
}

void HapticManager::playFailurePattern(Hand hand) {
    playPattern(hand, m_failurePattern);
}

void HapticManager::playCollisionPattern(Hand hand, float impactStrength) {
    float intensity = std::clamp(impactStrength, 0.2f, 1.0f);
    float duration = 0.05f + impactStrength * 0.15f;
    pulse(hand, intensity, duration);
}

void HapticManager::playSaberActivatePattern(Hand hand) {
    playPattern(hand, m_activatePattern);
}

void HapticManager::playSaberDeactivatePattern(Hand hand) {
    playPattern(hand, m_deactivatePattern);
}

void HapticManager::playGuidanceNudge(Hand hand, float strength) {
    float intensity = std::clamp(strength * 0.3f, 0.1f, 0.4f);
    pulse(hand, intensity, 0.05f);
}

void HapticManager::playPattern(Hand hand, const std::vector<HapticStep>& pattern) {
    float accumulatedDelay = 0.0f;

    for (const auto& step : pattern) {
        QueuedHaptic haptic;
        haptic.hand = hand;
        haptic.intensity = std::clamp(step.intensity, 0.0f, 1.0f);
        haptic.duration = step.duration;
        haptic.delay = accumulatedDelay;
        haptic.timeRemaining = step.duration;

        if (hand == Hand::Left) {
            m_leftQueue.push(haptic);
        } else {
            m_rightQueue.push(haptic);
        }

        accumulatedDelay += step.duration + step.pauseAfter;
    }
}

void HapticManager::update(double deltaTime) {
    if (!m_input) return;

    float dt = static_cast<float>(deltaTime);

    // Process left hand queue
    m_leftRemainingTime -= dt;
    if (m_leftRemainingTime <= 0 && !m_leftQueue.empty()) {
        auto& haptic = m_leftQueue.front();
        haptic.delay -= dt;

        if (haptic.delay <= 0) {
            m_input->triggerHaptic(Hand::Left, haptic.intensity, haptic.duration);
            m_leftRemainingTime = haptic.duration;
            m_leftQueue.pop();
        }
    }

    // Process right hand queue
    m_rightRemainingTime -= dt;
    if (m_rightRemainingTime <= 0 && !m_rightQueue.empty()) {
        auto& haptic = m_rightQueue.front();
        haptic.delay -= dt;

        if (haptic.delay <= 0) {
            m_input->triggerHaptic(Hand::Right, haptic.intensity, haptic.duration);
            m_rightRemainingTime = haptic.duration;
            m_rightQueue.pop();
        }
    }
}

void HapticManager::stopAll() {
    // Clear queues
    while (!m_leftQueue.empty()) m_leftQueue.pop();
    while (!m_rightQueue.empty()) m_rightQueue.pop();

    m_leftRemainingTime = 0.0f;
    m_rightRemainingTime = 0.0f;

    // Stop any active haptics
    if (m_input) {
        m_input->triggerHaptic(Hand::Left, 0.0f, 0.0f);
        m_input->triggerHaptic(Hand::Right, 0.0f, 0.0f);
    }
}

} // namespace lst
