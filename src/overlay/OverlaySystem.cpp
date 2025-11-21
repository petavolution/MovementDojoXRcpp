/**
 * OpenXR Overlay System Implementation
 *
 * Allows running as a transparent overlay on top of other VR applications.
 * Uses XR_EXTX_overlay extension when available.
 */

#include "OverlaySystem.h"
#include "core/Renderer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace lst {

// =============================================================================
// OverlaySystem
// =============================================================================

OverlaySystem::OverlaySystem() {
    m_style = OverlayStyle::getDefault();

    // Initialize enabled elements with defaults
    m_enabledElements[OverlayElement::MovementTrails] = true;
    m_enabledElements[OverlayElement::CoverageIndicator] = true;
    m_enabledElements[OverlayElement::UncommonAreaGuide] = false;
    m_enabledElements[OverlayElement::StatsHUD] = true;
    m_enabledElements[OverlayElement::FlowIndicator] = false;
    m_enabledElements[OverlayElement::SessionTimer] = true;
    m_enabledElements[OverlayElement::BreathingGuide] = false;
}

OverlaySystem::~OverlaySystem() {
    shutdown();
}

bool OverlaySystem::initialize(MovementAnalytics* analytics) {
    m_analytics = analytics;

    // Reset render data
    m_renderData = OverlayRenderData{};

    // Check if overlay is supported
    if (!isOverlaySupported()) {
        // Can still run, but won't appear over other apps
        return true;
    }

    return true;
}

void OverlaySystem::shutdown() {
    m_analytics = nullptr;
    m_renderData = OverlayRenderData{};
}

bool OverlaySystem::isOverlaySupported() {
    // OverlaySystem can always render overlay-style elements.
    // The XR_EXTX_overlay extension (queried in XRSession) determines if we can
    // actually run as an overlay on top of other VR apps.
    // Without the extension, we render in our own app's layer (still useful).
    return true;
}

void OverlaySystem::setElementEnabled(OverlayElement element, bool enabled) {
    m_enabledElements[element] = enabled;
}

bool OverlaySystem::isElementEnabled(OverlayElement element) const {
    auto it = m_enabledElements.find(element);
    return it != m_enabledElements.end() && it->second;
}

void OverlaySystem::setMinimalMode(bool minimal) {
    m_minimalMode = minimal;

    if (minimal) {
        m_style = OverlayStyle::getMinimal();
        // Disable most elements
        m_enabledElements[OverlayElement::MovementTrails] = true;
        m_enabledElements[OverlayElement::CoverageIndicator] = false;
        m_enabledElements[OverlayElement::UncommonAreaGuide] = false;
        m_enabledElements[OverlayElement::StatsHUD] = false;
        m_enabledElements[OverlayElement::FlowIndicator] = false;
        m_enabledElements[OverlayElement::SessionTimer] = false;
        m_enabledElements[OverlayElement::BreathingGuide] = false;
    } else {
        m_style = OverlayStyle::getDefault();
        // Re-enable standard elements
        m_enabledElements[OverlayElement::MovementTrails] = true;
        m_enabledElements[OverlayElement::CoverageIndicator] = true;
        m_enabledElements[OverlayElement::StatsHUD] = true;
        m_enabledElements[OverlayElement::SessionTimer] = true;
    }
}

void OverlaySystem::update(double deltaTime, const Transform& headPose) {
    if (!m_visible) return;

    float dt = static_cast<float>(deltaTime);

    // Update session time
    m_sessionTime += dt;

    // Update animation state
    m_pulseTimer += dt;
    if (m_pulseTimer > 6.28318f) m_pulseTimer -= 6.28318f;  // 2*PI

    m_breathPhase += dt * 0.5f;  // Slow breathing rhythm
    if (m_breathPhase > 6.28318f) m_breathPhase -= 6.28318f;

    // Update coverage from analytics
    if (m_analytics) {
        m_currentCoverage = m_analytics->getMovementSpaceCoverage();
        if (m_analytics->isInUncommonPosition()) {
            m_uncommonFound++;
        }
    }

    // Update render data
    if (isElementEnabled(OverlayElement::MovementTrails)) {
        updateTrails();
    }
    if (isElementEnabled(OverlayElement::UncommonAreaGuide)) {
        updateGuideOrbs();
    }
    if (isElementEnabled(OverlayElement::StatsHUD) ||
        isElementEnabled(OverlayElement::CoverageIndicator) ||
        isElementEnabled(OverlayElement::SessionTimer)) {
        updateHUD(headPose);
    }
    if (isElementEnabled(OverlayElement::CoverageIndicator)) {
        updateCoverageSphere();
    }
}

void OverlaySystem::cycleElements() {
    // Cycle through preset element combinations
    static int cycle = 0;
    cycle = (cycle + 1) % 4;

    switch (cycle) {
        case 0:  // Minimal - just trails
            setMinimalMode(true);
            break;
        case 1:  // Standard
            setMinimalMode(false);
            break;
        case 2:  // Exploration focus
            setMinimalMode(false);
            m_enabledElements[OverlayElement::UncommonAreaGuide] = true;
            m_enabledElements[OverlayElement::FlowIndicator] = false;
            break;
        case 3:  // Full visualization
            setMinimalMode(false);
            m_style = OverlayStyle::getFullVisualization();
            for (auto& pair : m_enabledElements) {
                pair.second = true;
            }
            break;
    }
}

void OverlaySystem::applyPreset(const std::string& presetName) {
    if (presetName == "minimal") {
        setMinimalMode(true);
    } else if (presetName == "standard") {
        setMinimalMode(false);
    } else if (presetName == "exploration") {
        setMinimalMode(false);
        m_enabledElements[OverlayElement::UncommonAreaGuide] = true;
        m_enabledElements[OverlayElement::CoverageIndicator] = true;
    } else if (presetName == "meditation") {
        setMinimalMode(false);
        m_enabledElements[OverlayElement::MovementTrails] = true;
        m_enabledElements[OverlayElement::BreathingGuide] = true;
        m_enabledElements[OverlayElement::FlowIndicator] = true;
        m_enabledElements[OverlayElement::StatsHUD] = false;
    } else if (presetName == "full") {
        m_style = OverlayStyle::getFullVisualization();
        for (auto& pair : m_enabledElements) {
            pair.second = true;
        }
    }
}

std::vector<std::string> OverlaySystem::getAvailablePresets() const {
    return {"minimal", "standard", "exploration", "meditation", "full"};
}

void OverlaySystem::updateTrails() {
    m_renderData.leftTrail.clear();
    m_renderData.rightTrail.clear();

    if (!m_analytics) return;

    const auto& samples = m_analytics->getSamples();
    if (samples.size() < 2) return;

    // Calculate how many samples to use based on trail length
    double trailDuration = m_style.trailLength;
    double currentTime = samples.back().timestamp;

    // Find samples within trail duration
    std::vector<const MovementSample*> recentSamples;
    for (auto it = samples.rbegin(); it != samples.rend(); ++it) {
        if (currentTime - it->timestamp > trailDuration) break;
        recentSamples.push_back(&(*it));
    }

    // Generate trail segments
    for (size_t i = 1; i < recentSamples.size(); i++) {
        const auto* prev = recentSamples[i];
        const auto* curr = recentSamples[i - 1];

        // Calculate age for fading (newer = more opaque)
        float age = static_cast<float>((currentTime - prev->timestamp) / trailDuration);
        float alpha = (1.0f - age) * m_style.opacity;

        // Calculate speed for color
        Vec3 leftVel = curr->leftHandPosition - prev->leftHandPosition;
        Vec3 rightVel = curr->rightHandPosition - prev->rightHandPosition;
        float leftSpeed = leftVel.length() / 0.016f;  // Assuming ~60fps
        float rightSpeed = rightVel.length() / 0.016f;

        // Interpolate color based on speed (0-2 m/s range)
        float leftSpeedNorm = std::min(leftSpeed / 2.0f, 1.0f);
        float rightSpeedNorm = std::min(rightSpeed / 2.0f, 1.0f);

        auto lerpColor = [](const Color& a, const Color& b, float t) {
            return Color(
                a.r + (b.r - a.r) * t,
                a.g + (b.g - a.g) * t,
                a.b + (b.b - a.b) * t,
                a.a + (b.a - a.a) * t
            );
        };

        Color leftColor = lerpColor(m_style.trailColorSlow, m_style.trailColorFast, leftSpeedNorm);
        leftColor.a *= alpha;
        Color rightColor = lerpColor(m_style.trailColorSlow, m_style.trailColorFast, rightSpeedNorm);
        rightColor.a *= alpha;

        // Add segments
        OverlayRenderData::TrailSegment leftSeg;
        leftSeg.start = prev->leftHandPosition;
        leftSeg.end = curr->leftHandPosition;
        leftSeg.color = leftColor;
        leftSeg.width = m_style.trailWidth;
        m_renderData.leftTrail.push_back(leftSeg);

        OverlayRenderData::TrailSegment rightSeg;
        rightSeg.start = prev->rightHandPosition;
        rightSeg.end = curr->rightHandPosition;
        rightSeg.color = rightColor;
        rightSeg.width = m_style.trailWidth;
        m_renderData.rightTrail.push_back(rightSeg);
    }
}

void OverlaySystem::updateGuideOrbs() {
    m_renderData.guideOrbs.clear();

    if (!m_analytics) return;

    // Get unexplored areas from analytics
    std::vector<Vec3> unexplored = m_analytics->getUnexploredAreas();

    // Limit to top 3-5 nearest unexplored areas
    const size_t maxOrbs = m_minimalMode ? 1 : 5;

    for (size_t i = 0; i < std::min(unexplored.size(), maxOrbs); i++) {
        OverlayRenderData::GuideOrb orb;
        orb.position = unexplored[i];
        orb.radius = 0.05f + 0.02f * std::sin(m_pulseTimer + i * 1.0f);  // Pulsing
        orb.color = m_style.guideColor;
        orb.color.a = 0.4f + 0.2f * std::sin(m_pulseTimer + i * 0.5f);
        orb.pulsePhase = m_pulseTimer + i * 0.5f;
        m_renderData.guideOrbs.push_back(orb);
    }
}

void OverlaySystem::updateHUD(const Transform& headPose) {
    m_renderData.hudElements.clear();

    // Calculate HUD position relative to head
    Vec3 hudPos = headPose.position + headPose.orientation.rotate(m_style.hudOffset);

    float yOffset = 0;
    float lineHeight = 0.025f * m_style.hudScale;

    // Session timer
    if (isElementEnabled(OverlayElement::SessionTimer)) {
        int minutes = static_cast<int>(m_sessionTime) / 60;
        int seconds = static_cast<int>(m_sessionTime) % 60;

        OverlayRenderData::HUDElement timer;
        timer.text = std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
        timer.worldPosition = hudPos + Vec3(0, yOffset, 0);
        timer.color = m_style.hudColor;
        timer.scale = m_style.hudScale;
        m_renderData.hudElements.push_back(timer);
        yOffset -= lineHeight;
    }

    // Coverage indicator
    if (isElementEnabled(OverlayElement::CoverageIndicator)) {
        OverlayRenderData::HUDElement coverage;
        coverage.text = "Coverage: " + std::to_string(static_cast<int>(m_currentCoverage)) + "%";
        coverage.worldPosition = hudPos + Vec3(0, yOffset, 0);
        coverage.color = m_style.hudColor;
        if (m_currentCoverage > 50) coverage.color = Color(0.3f, 1.0f, 0.3f, 0.8f);
        coverage.scale = m_style.hudScale;
        m_renderData.hudElements.push_back(coverage);
        yOffset -= lineHeight;
    }

    // Stats HUD (uncommon areas found)
    if (isElementEnabled(OverlayElement::StatsHUD)) {
        OverlayRenderData::HUDElement uncommon;
        uncommon.text = "Uncommon: " + std::to_string(m_uncommonFound);
        uncommon.worldPosition = hudPos + Vec3(0, yOffset, 0);
        uncommon.color = m_style.hudColor;
        uncommon.scale = m_style.hudScale * 0.8f;
        m_renderData.hudElements.push_back(uncommon);
        yOffset -= lineHeight;
    }

    // Flow indicator
    if (isElementEnabled(OverlayElement::FlowIndicator)) {
        OverlayRenderData::HUDElement flow;
        flow.text = "Flow Active";  // Simplified - would normally show flow score
        flow.worldPosition = hudPos + Vec3(0, yOffset, 0);
        flow.color = Color(0.8f, 0.4f, 1.0f, 0.8f);
        flow.scale = m_style.hudScale;
        m_renderData.hudElements.push_back(flow);
        yOffset -= lineHeight;
    }

    // Breathing guide
    if (isElementEnabled(OverlayElement::BreathingGuide)) {
        // Visual breathing indicator
        float breathProgress = (std::sin(m_breathPhase) + 1.0f) / 2.0f;  // 0-1
        std::string breathText = breathProgress > 0.5f ? "Inhale..." : "Exhale...";

        OverlayRenderData::HUDElement breath;
        breath.text = breathText;
        breath.worldPosition = hudPos + Vec3(0, yOffset, 0);
        breath.color = Color(0.5f, 0.8f, 1.0f, 0.6f + breathProgress * 0.3f);
        breath.scale = m_style.hudScale;
        m_renderData.hudElements.push_back(breath);
    }
}

void OverlaySystem::updateCoverageSphere() {
    m_renderData.coverageSphere.clear();

    // Create a sphere of points representing movement space coverage
    // This is a simplified visualization

    const int numPoints = 64;
    const float goldenRatio = (1.0f + std::sqrt(5.0f)) / 2.0f;

    for (int i = 0; i < numPoints; i++) {
        // Fibonacci sphere distribution
        float y = 1.0f - (i / static_cast<float>(numPoints - 1)) * 2.0f;
        float radiusAtY = std::sqrt(1.0f - y * y);
        float theta = 2.0f * 3.14159f * i / goldenRatio;

        OverlayRenderData::CoveragePoint point;
        point.direction = Vec3(
            std::cos(theta) * radiusAtY,
            y,
            std::sin(theta) * radiusAtY
        );

        // Intensity based on coverage (simplified - real implementation
        // would query actual voxel data)
        point.intensity = m_currentCoverage / 100.0f;

        // Color: green for explored, dim for unexplored
        if (point.intensity > 0.5f) {
            point.color = Color(0.2f, 1.0f, 0.4f, 0.3f);
        } else {
            point.color = Color(0.3f, 0.3f, 0.3f, 0.1f);
        }

        m_renderData.coverageSphere.push_back(point);
    }
}

// =============================================================================
// SimpleOverlayRenderer - Actual rendering implementation
// =============================================================================

bool SimpleOverlayRenderer::initialize() {
    if (!m_renderer) {
        std::cout << "SimpleOverlayRenderer: Warning - no renderer set" << std::endl;
        return false;
    }
    std::cout << "SimpleOverlayRenderer initialized" << std::endl;
    return true;
}

void SimpleOverlayRenderer::shutdown() {
    m_renderer = nullptr;
}

void SimpleOverlayRenderer::beginOverlayFrame() {
    // Overlay rendering happens within the main render pass
    // No separate begin/end needed for this simple implementation
}

void SimpleOverlayRenderer::endOverlayFrame() {
    // Overlay rendering happens within the main render pass
}

void SimpleOverlayRenderer::renderTrails(const std::vector<OverlayRenderData::TrailSegment>& trails) {
    if (!m_renderer) return;

    for (const auto& segment : trails) {
        m_renderer->drawLine(segment.start, segment.end, segment.color, segment.width);
    }
}

void SimpleOverlayRenderer::renderGuideOrbs(const std::vector<OverlayRenderData::GuideOrb>& orbs) {
    if (!m_renderer) return;

    for (const auto& orb : orbs) {
        // Apply pulse effect to radius
        float pulseScale = 1.0f + 0.2f * std::sin(orb.pulsePhase * 6.28318f);
        float radius = orb.radius * pulseScale;

        // Draw with slight transparency variation based on pulse
        Color pulsedColor = orb.color;
        pulsedColor.a *= (0.7f + 0.3f * std::sin(orb.pulsePhase * 6.28318f));

        m_renderer->drawSphere(orb.position, radius, pulsedColor);
    }
}

void SimpleOverlayRenderer::renderHUD(const std::vector<OverlayRenderData::HUDElement>& elements) {
    if (!m_renderer) return;

    for (const auto& element : elements) {
        // Draw text at world position
        // Note: Text rendering is currently a stub in Renderer
        m_renderer->drawText(element.worldPosition, element.text, element.color, element.scale);

        // As a fallback, draw a small indicator sphere at HUD element positions
        // This provides visual feedback even without text rendering
        m_renderer->drawSphere(element.worldPosition, 0.01f * element.scale, element.color);
    }
}

void SimpleOverlayRenderer::renderCoverageSphere(const std::vector<OverlayRenderData::CoveragePoint>& points) {
    if (!m_renderer) return;

    // Render coverage as small spheres in the directions that have been explored
    // Center at player position (approximately)
    const float coverageRadius = 0.8f;  // Distance from head center
    const float pointSize = 0.02f;

    for (const auto& point : points) {
        // Only render points with significant intensity
        if (point.intensity > 0.1f) {
            Vec3 position = point.direction * coverageRadius;
            float size = pointSize * point.intensity;
            m_renderer->drawSphere(position, size, point.color);
        }
    }
}

void SimpleOverlayRenderer::renderAll(const OverlayRenderData& data) {
    if (!m_renderer) return;

    beginOverlayFrame();

    // Render in back-to-front order for proper transparency
    renderCoverageSphere(data.coverageSphere);
    renderGuideOrbs(data.guideOrbs);
    renderTrails(data.leftTrail);
    renderTrails(data.rightTrail);
    renderHUD(data.hudElements);

    endOverlayFrame();
}

} // namespace lst
