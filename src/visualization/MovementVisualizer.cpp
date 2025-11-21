/**
 * MovementVisualizer Implementation
 *
 * Connects movement analytics to visual rendering.
 */

#include "MovementVisualizer.h"
#include <iostream>

namespace lst {

// =============================================================================
// MovementVisualizer
// =============================================================================

MovementVisualizer::MovementVisualizer()
    : m_overlay(std::make_unique<OverlaySystem>())
    , m_overlayRenderer(std::make_unique<SimpleOverlayRenderer>()) {
}

MovementVisualizer::~MovementVisualizer() {
    shutdown();
}

bool MovementVisualizer::initialize(MovementAnalytics* analytics, Renderer* renderer) {
    if (!analytics || !renderer) {
        std::cerr << "MovementVisualizer: Invalid analytics or renderer" << std::endl;
        return false;
    }

    m_analytics = analytics;
    m_renderer = renderer;

    // Initialize overlay system
    if (!m_overlay->initialize(analytics)) {
        std::cerr << "MovementVisualizer: Failed to initialize overlay system" << std::endl;
        return false;
    }

    // Connect renderer to overlay renderer
    m_overlayRenderer->setRenderer(renderer);
    if (!m_overlayRenderer->initialize()) {
        std::cerr << "MovementVisualizer: Failed to initialize overlay renderer" << std::endl;
        return false;
    }

    // Apply default preset
    VisualizationPresets::applyStandard(*this);

    m_initialized = true;
    std::cout << "MovementVisualizer initialized" << std::endl;
    return true;
}

void MovementVisualizer::shutdown() {
    if (m_overlayRenderer) {
        m_overlayRenderer->shutdown();
    }
    if (m_overlay) {
        m_overlay->shutdown();
    }
    m_initialized = false;
    m_analytics = nullptr;
    m_renderer = nullptr;
}

void MovementVisualizer::setStyle(const OverlayStyle& style) {
    if (m_overlay) {
        m_overlay->setStyle(style);
    }
}

void MovementVisualizer::setPreset(const std::string& presetName) {
    if (m_overlay) {
        m_overlay->applyPreset(presetName);
    }
}

void MovementVisualizer::setVisible(bool visible) {
    if (m_overlay) {
        m_overlay->setVisible(visible);
    }
}

void MovementVisualizer::toggleVisibility() {
    if (m_overlay) {
        m_overlay->toggleVisibility();
    }
}

void MovementVisualizer::setTrailsEnabled(bool enabled) {
    if (m_overlay) {
        m_overlay->setElementEnabled(OverlayElement::MovementTrails, enabled);
    }
}

void MovementVisualizer::setCoverageEnabled(bool enabled) {
    if (m_overlay) {
        m_overlay->setElementEnabled(OverlayElement::CoverageIndicator, enabled);
    }
}

void MovementVisualizer::setGuidesEnabled(bool enabled) {
    if (m_overlay) {
        m_overlay->setElementEnabled(OverlayElement::UncommonAreaGuide, enabled);
    }
}

void MovementVisualizer::setHUDEnabled(bool enabled) {
    if (m_overlay) {
        m_overlay->setElementEnabled(OverlayElement::StatsHUD, enabled);
        m_overlay->setElementEnabled(OverlayElement::SessionTimer, enabled);
    }
}

void MovementVisualizer::update(double deltaTime, const Transform& headPose) {
    if (!m_initialized || !m_overlay) return;

    m_sessionTime += static_cast<float>(deltaTime);
    m_overlay->update(deltaTime, headPose);
}

void MovementVisualizer::render() {
    if (!m_initialized || !m_overlayRenderer || !m_overlay) return;
    if (!m_overlay->isVisible()) return;

    // Render all overlay elements
    const OverlayRenderData& data = m_overlay->getRenderData();
    m_overlayRenderer->renderAll(data);
}

bool MovementVisualizer::isVisible() const {
    return m_overlay ? m_overlay->isVisible() : false;
}

const OverlayRenderData& MovementVisualizer::getRenderData() const {
    static OverlayRenderData empty;
    return m_overlay ? m_overlay->getRenderData() : empty;
}

float MovementVisualizer::getSessionTime() const {
    return m_sessionTime;
}

float MovementVisualizer::getCoverage() const {
    if (m_analytics) {
        return m_analytics->getCoveragePercentage();
    }
    return 0.0f;
}

int MovementVisualizer::getUncommonAreasFound() const {
    if (m_analytics) {
        return m_analytics->getUncommonAreasExplored();
    }
    return 0;
}

// =============================================================================
// VisualizationPresets
// =============================================================================

void VisualizationPresets::applyMinimal(MovementVisualizer& viz) {
    viz.setTrailsEnabled(true);
    viz.setCoverageEnabled(false);
    viz.setGuidesEnabled(false);
    viz.setHUDEnabled(false);

    OverlayStyle style = OverlayStyle::getMinimal();
    style.trailLength = 1.0f;
    style.opacity = 0.4f;
    viz.setStyle(style);
}

void VisualizationPresets::applyStandard(MovementVisualizer& viz) {
    viz.setTrailsEnabled(true);
    viz.setCoverageEnabled(true);
    viz.setGuidesEnabled(false);
    viz.setHUDEnabled(true);

    viz.setStyle(OverlayStyle::getDefault());
}

void VisualizationPresets::applyExploration(MovementVisualizer& viz) {
    viz.setTrailsEnabled(true);
    viz.setCoverageEnabled(true);
    viz.setGuidesEnabled(true);
    viz.setHUDEnabled(true);

    OverlayStyle style = OverlayStyle::getDefault();
    style.opacity = 0.8f;
    style.trailLength = 3.0f;
    style.guideColor = Color(0.3f, 1.0f, 0.5f, 0.6f);
    viz.setStyle(style);

    // Enable additional elements
    if (viz.getOverlaySystem()) {
        viz.getOverlaySystem()->setElementEnabled(OverlayElement::FlowIndicator, true);
    }
}

void VisualizationPresets::applyMeditation(MovementVisualizer& viz) {
    viz.setTrailsEnabled(true);
    viz.setCoverageEnabled(false);
    viz.setGuidesEnabled(false);
    viz.setHUDEnabled(false);

    OverlayStyle style;
    style.opacity = 0.3f;
    style.trailLength = 4.0f;
    style.trailWidth = 0.003f;
    style.trailColorSlow = Color(0.5f, 0.3f, 0.8f, 0.4f);  // Soft purple
    style.trailColorFast = Color(0.5f, 0.3f, 0.8f, 0.2f);  // Fade when fast
    viz.setStyle(style);

    // Enable breathing guide for meditation
    if (viz.getOverlaySystem()) {
        viz.getOverlaySystem()->setElementEnabled(OverlayElement::BreathingGuide, true);
    }
}

void VisualizationPresets::applyFull(MovementVisualizer& viz) {
    viz.setTrailsEnabled(true);
    viz.setCoverageEnabled(true);
    viz.setGuidesEnabled(true);
    viz.setHUDEnabled(true);

    viz.setStyle(OverlayStyle::getFullVisualization());

    // Enable all elements
    if (viz.getOverlaySystem()) {
        auto* overlay = viz.getOverlaySystem();
        overlay->setElementEnabled(OverlayElement::MovementTrails, true);
        overlay->setElementEnabled(OverlayElement::CoverageIndicator, true);
        overlay->setElementEnabled(OverlayElement::UncommonAreaGuide, true);
        overlay->setElementEnabled(OverlayElement::StatsHUD, true);
        overlay->setElementEnabled(OverlayElement::FlowIndicator, true);
        overlay->setElementEnabled(OverlayElement::SessionTimer, true);
        overlay->setElementEnabled(OverlayElement::BreathingGuide, true);
    }
}

void VisualizationPresets::applyDebug(MovementVisualizer& viz) {
    // Same as full but with maximum visibility
    applyFull(viz);

    OverlayStyle style = OverlayStyle::getFullVisualization();
    style.opacity = 1.0f;
    style.trailLength = 10.0f;
    style.trailWidth = 0.01f;
    viz.setStyle(style);
}

} // namespace lst
