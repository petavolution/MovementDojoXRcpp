#pragma once

#include "Types.h"
#include "overlay/OverlaySystem.h"
#include "analytics/MovementAnalytics.h"
#include "core/Renderer.h"
#include <memory>

namespace lst {

/**
 * MovementVisualizer - High-level visualization controller
 *
 * Connects MovementAnalytics -> OverlaySystem -> Renderer
 * to provide real-time visualization of movement data.
 *
 * Features:
 * - Trail rendering for hand movement paths
 * - Coverage sphere visualization
 * - Guide orbs for exploration targets
 * - HUD with session stats
 *
 * Usage:
 *   MovementVisualizer viz;
 *   viz.initialize(&analytics, &renderer);
 *
 *   // In main loop:
 *   viz.update(deltaTime, headPose);
 *   viz.render();  // Call after main scene rendering
 */

class MovementVisualizer {
public:
    MovementVisualizer();
    ~MovementVisualizer();

    // Setup
    bool initialize(MovementAnalytics* analytics, Renderer* renderer);
    void shutdown();

    // Configuration
    void setStyle(const OverlayStyle& style);
    void setPreset(const std::string& presetName);
    void setVisible(bool visible);
    void toggleVisibility();

    // Element control
    void setTrailsEnabled(bool enabled);
    void setCoverageEnabled(bool enabled);
    void setGuidesEnabled(bool enabled);
    void setHUDEnabled(bool enabled);

    // Per-frame update
    void update(double deltaTime, const Transform& headPose);

    // Render overlay elements (call after main scene)
    void render();

    // State queries
    bool isVisible() const;
    bool isInitialized() const { return m_initialized; }
    const OverlayRenderData& getRenderData() const;

    // Quick access to session stats for UI
    float getSessionTime() const;
    float getCoverage() const;
    int getUncommonAreasFound() const;

    // Get references for external use
    OverlaySystem* getOverlaySystem() { return m_overlay.get(); }
    SimpleOverlayRenderer* getRenderer() { return m_overlayRenderer.get(); }

private:
    std::unique_ptr<OverlaySystem> m_overlay;
    std::unique_ptr<SimpleOverlayRenderer> m_overlayRenderer;

    MovementAnalytics* m_analytics = nullptr;
    Renderer* m_renderer = nullptr;

    bool m_initialized = false;
    float m_sessionTime = 0.0f;
};

/**
 * VisualizationPresets - Pre-configured visualization styles
 */
struct VisualizationPresets {
    // Minimal overlay - just trails
    static void applyMinimal(MovementVisualizer& viz);

    // Standard overlay - trails + coverage + stats
    static void applyStandard(MovementVisualizer& viz);

    // Exploration mode - all guides enabled
    static void applyExploration(MovementVisualizer& viz);

    // Meditation mode - subtle, non-distracting
    static void applyMeditation(MovementVisualizer& viz);

    // Full visualization - everything enabled
    static void applyFull(MovementVisualizer& viz);

    // Debug mode - shows all raw data
    static void applyDebug(MovementVisualizer& viz);
};

} // namespace lst
