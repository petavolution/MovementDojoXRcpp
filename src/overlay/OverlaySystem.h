#pragma once

#include "Types.h"
#include "analytics/MovementAnalytics.h"
#include <vector>
#include <string>
#include <memory>

namespace lst {

/**
 * OpenXR Overlay System
 *
 * Allows running as an overlay on top of other VR applications.
 * This enables movement tracking and visualization while playing
 * any other VR game (like Lone Echo, Beat Saber, etc.)
 *
 * Uses XR_EXTX_overlay extension when available.
 *
 * Features:
 * - Transparent overlay layer
 * - Movement trail visualization
 * - Coverage heatmap
 * - Stats HUD
 * - Non-intrusive by default (can be toggled)
 */

// Overlay element types
enum class OverlayElement {
    None,
    MovementTrails,      // Show hand movement trails
    CoverageIndicator,   // Show exploration coverage %
    UncommonAreaGuide,   // Highlight unexplored areas
    StatsHUD,            // Mini stats display
    FlowIndicator,       // Show current flow state
    SessionTimer,        // Time in session
    BreathingGuide       // Visual breathing rhythm
};

// Overlay visual style
struct OverlayStyle {
    float opacity;           // Overall overlay opacity (0-1)
    float trailLength;       // Trail duration in seconds
    float trailWidth;        // Trail line width
    Color trailColorSlow;    // Color for slow movement
    Color trailColorFast;    // Color for fast movement
    Color guideColor;        // Color for exploration guides
    Color hudColor;          // HUD text/element color
    float hudScale;          // HUD size scale
    Vec3 hudOffset;          // HUD position offset from head

    static OverlayStyle getDefault() {
        OverlayStyle style;
        style.opacity = 0.7f;
        style.trailLength = 2.0f;
        style.trailWidth = 0.005f;
        style.trailColorSlow = Color(0.2f, 0.6f, 1.0f, 0.6f);
        style.trailColorFast = Color(1.0f, 0.3f, 0.2f, 0.8f);
        style.guideColor = Color(0.3f, 1.0f, 0.5f, 0.4f);
        style.hudColor = Color(1.0f, 1.0f, 1.0f, 0.8f);
        style.hudScale = 1.0f;
        style.hudOffset = Vec3(0.3f, -0.2f, -0.5f);  // Lower right of view
        return style;
    }

    static OverlayStyle getMinimal() {
        OverlayStyle style = getDefault();
        style.opacity = 0.4f;
        style.trailLength = 1.0f;
        return style;
    }

    static OverlayStyle getFullVisualization() {
        OverlayStyle style = getDefault();
        style.opacity = 0.9f;
        style.trailLength = 5.0f;
        return style;
    }
};

// Rendered overlay element data
struct OverlayRenderData {
    // Trail segments
    struct TrailSegment {
        Vec3 start;
        Vec3 end;
        Color color;
        float width;
    };
    std::vector<TrailSegment> leftTrail;
    std::vector<TrailSegment> rightTrail;

    // Guide orbs (exploration targets)
    struct GuideOrb {
        Vec3 position;
        float radius;
        Color color;
        float pulsePhase;
    };
    std::vector<GuideOrb> guideOrbs;

    // HUD elements
    struct HUDElement {
        std::string text;
        Vec3 worldPosition;
        Color color;
        float scale;
    };
    std::vector<HUDElement> hudElements;

    // Coverage sphere (for visualization)
    struct CoveragePoint {
        Vec3 direction;
        float intensity;
        Color color;
    };
    std::vector<CoveragePoint> coverageSphere;
};

/**
 * OverlaySystem - Main overlay management class
 */
class OverlaySystem {
public:
    OverlaySystem();
    ~OverlaySystem();

    // Initialize overlay mode
    bool initialize(MovementAnalytics* analytics);
    void shutdown();

    // Check if overlay mode is supported
    static bool isOverlaySupported();

    // Enable/disable overlay elements
    void setElementEnabled(OverlayElement element, bool enabled);
    bool isElementEnabled(OverlayElement element) const;

    // Style configuration
    void setStyle(const OverlayStyle& style) { m_style = style; }
    const OverlayStyle& getStyle() const { return m_style; }

    // Toggle overlay visibility
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

    // Toggle between full overlay and minimal mode
    void setMinimalMode(bool minimal);
    bool isMinimalMode() const { return m_minimalMode; }

    // Update overlay (call each frame)
    void update(double deltaTime, const Transform& headPose);

    // Get render data for current frame
    const OverlayRenderData& getRenderData() const { return m_renderData; }

    // Quick toggle hotkey support
    void toggleVisibility() { m_visible = !m_visible; }
    void toggleMinimalMode() { setMinimalMode(!m_minimalMode); }
    void cycleElements();  // Cycle through different element combinations

    // Presets
    void applyPreset(const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;

private:
    void updateTrails();
    void updateGuideOrbs();
    void updateHUD(const Transform& headPose);
    void updateCoverageSphere();

    MovementAnalytics* m_analytics = nullptr;

    bool m_visible = true;
    bool m_minimalMode = false;
    OverlayStyle m_style;

    // Enabled elements
    std::map<OverlayElement, bool> m_enabledElements;

    // Current render data
    OverlayRenderData m_renderData;

    // Animation state
    float m_pulseTimer = 0;
    float m_breathPhase = 0;

    // Session stats for HUD
    float m_sessionTime = 0;
    float m_currentCoverage = 0;
    int m_uncommonFound = 0;
};

/**
 * OverlayRenderer - Handles actual rendering of overlay elements
 *
 * Abstract base class - implement for specific graphics APIs
 */
class OverlayRenderer {
public:
    virtual ~OverlayRenderer() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void beginOverlayFrame() = 0;
    virtual void endOverlayFrame() = 0;

    virtual void renderTrails(const std::vector<OverlayRenderData::TrailSegment>& trails) = 0;
    virtual void renderGuideOrbs(const std::vector<OverlayRenderData::GuideOrb>& orbs) = 0;
    virtual void renderHUD(const std::vector<OverlayRenderData::HUDElement>& elements) = 0;
    virtual void renderCoverageSphere(const std::vector<OverlayRenderData::CoveragePoint>& points) = 0;
};

// Forward declaration
class Renderer;

/**
 * SimpleOverlayRenderer - Basic implementation using debug line rendering
 *
 * Connects to the main Renderer to actually draw overlay elements.
 */
class SimpleOverlayRenderer : public OverlayRenderer {
public:
    SimpleOverlayRenderer() = default;
    explicit SimpleOverlayRenderer(Renderer* renderer) : m_renderer(renderer) {}

    void setRenderer(Renderer* renderer) { m_renderer = renderer; }

    bool initialize() override;
    void shutdown() override;

    void beginOverlayFrame() override;
    void endOverlayFrame() override;

    void renderTrails(const std::vector<OverlayRenderData::TrailSegment>& trails) override;
    void renderGuideOrbs(const std::vector<OverlayRenderData::GuideOrb>& orbs) override;
    void renderHUD(const std::vector<OverlayRenderData::HUDElement>& elements) override;
    void renderCoverageSphere(const std::vector<OverlayRenderData::CoveragePoint>& points) override;

    // Render all overlay data in one call
    void renderAll(const OverlayRenderData& data);

private:
    Renderer* m_renderer = nullptr;
};

} // namespace lst
