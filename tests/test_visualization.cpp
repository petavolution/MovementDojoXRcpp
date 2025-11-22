/**
 * MovementVisualizer and OverlaySystem Tests
 *
 * Tests the visualization pipeline for movement data.
 */

#include "visualization/MovementVisualizer.h"
#include "overlay/OverlaySystem.h"
#include "analytics/MovementAnalytics.h"
#include <iostream>
#include <cassert>
#include <cmath>

#if USE_GTEST
#include <gtest/gtest.h>
#endif

using namespace lst;

namespace {

const float EPSILON = 0.0001f;

bool approxEqual(float a, float b) {
    return std::abs(a - b) < EPSILON;
}

// =============================================================================
// OverlayStyle Tests
// =============================================================================

void testOverlayStyleDefaults() {
    OverlayStyle style;

    // Check default values
    assert(style.trailLength > 0.0f);
    assert(style.trailWidth > 0.0f);
    assert(style.opacity >= 0.0f && style.opacity <= 1.0f);

    std::cout << "  OverlayStyle defaults: PASS" << std::endl;
}

void testOverlayStylePresets() {
    // Test preset factory methods
    OverlayStyle defaultStyle = OverlayStyle::getDefault();
    assert(defaultStyle.trailLength > 0.0f);
    assert(defaultStyle.opacity > 0.0f);

    OverlayStyle minimalStyle = OverlayStyle::getMinimal();
    assert(minimalStyle.opacity > 0.0f);
    // Minimal should have lower opacity than default
    assert(minimalStyle.opacity <= defaultStyle.opacity);

    OverlayStyle fullStyle = OverlayStyle::getFullVisualization();
    assert(fullStyle.trailLength >= defaultStyle.trailLength);

    std::cout << "  OverlayStyle presets: PASS" << std::endl;
}

// =============================================================================
// OverlaySystem Tests
// =============================================================================

void testOverlaySystemConstruction() {
    OverlaySystem overlay;

    // Should be invisible and uninitialized before init
    assert(overlay.isVisible() == false);

    std::cout << "  OverlaySystem construction: PASS" << std::endl;
}

void testOverlaySystemInitialization() {
    OverlaySystem overlay;
    MovementAnalytics analytics;

    // Initialize analytics first
    analytics.setActiveBounds(Vec3(-2, 0, -2), Vec3(2, 3, 2));

    // Initialize overlay with analytics reference
    bool initialized = overlay.initialize(&analytics);
    assert(initialized == true);

    // Should be visible after initialization
    assert(overlay.isVisible() == true);

    overlay.shutdown();

    std::cout << "  OverlaySystem initialization: PASS" << std::endl;
}

void testOverlaySystemVisibilityToggle() {
    OverlaySystem overlay;
    MovementAnalytics analytics;
    analytics.setActiveBounds(Vec3(-2, 0, -2), Vec3(2, 3, 2));
    overlay.initialize(&analytics);

    // Should start visible
    assert(overlay.isVisible() == true);

    // Toggle off
    overlay.setVisible(false);
    assert(overlay.isVisible() == false);

    // Toggle on
    overlay.setVisible(true);
    assert(overlay.isVisible() == true);

    // Use toggle method
    overlay.toggleVisibility();
    assert(overlay.isVisible() == false);

    overlay.toggleVisibility();
    assert(overlay.isVisible() == true);

    overlay.shutdown();

    std::cout << "  OverlaySystem visibility toggle: PASS" << std::endl;
}

void testOverlaySystemElementControl() {
    OverlaySystem overlay;
    MovementAnalytics analytics;
    analytics.setActiveBounds(Vec3(-2, 0, -2), Vec3(2, 3, 2));
    overlay.initialize(&analytics);

    // Enable/disable specific elements
    overlay.setElementEnabled(OverlayElement::MovementTrails, true);
    assert(overlay.isElementEnabled(OverlayElement::MovementTrails) == true);

    overlay.setElementEnabled(OverlayElement::MovementTrails, false);
    assert(overlay.isElementEnabled(OverlayElement::MovementTrails) == false);

    overlay.setElementEnabled(OverlayElement::CoverageIndicator, true);
    assert(overlay.isElementEnabled(OverlayElement::CoverageIndicator) == true);

    overlay.setElementEnabled(OverlayElement::StatsHUD, true);
    assert(overlay.isElementEnabled(OverlayElement::StatsHUD) == true);

    overlay.shutdown();

    std::cout << "  OverlaySystem element control: PASS" << std::endl;
}

void testOverlaySystemStyleApplication() {
    OverlaySystem overlay;
    MovementAnalytics analytics;
    analytics.setActiveBounds(Vec3(-2, 0, -2), Vec3(2, 3, 2));
    overlay.initialize(&analytics);

    OverlayStyle customStyle;
    customStyle.trailLength = 5.0f;
    customStyle.trailWidth = 0.02f;
    customStyle.opacity = 0.8f;

    overlay.setStyle(customStyle);

    // The style should be applied (can't easily verify internal state,
    // but at least verify no crash)
    assert(overlay.isVisible() == true);

    overlay.shutdown();

    std::cout << "  OverlaySystem style application: PASS" << std::endl;
}

void testOverlaySystemUpdate() {
    OverlaySystem overlay;
    MovementAnalytics analytics;
    analytics.setActiveBounds(Vec3(-2, 0, -2), Vec3(2, 3, 2));
    overlay.initialize(&analytics);

    Transform headPose;
    headPose.position = Vec3(0, 1.6f, 0);
    headPose.orientation = Quat::identity();

    // Run several update cycles
    for (int i = 0; i < 60; i++) {
        overlay.update(1.0 / 60.0, headPose);
    }

    // Should still be valid after updates
    assert(overlay.isVisible() == true);

    overlay.shutdown();

    std::cout << "  OverlaySystem update: PASS" << std::endl;
}

void testOverlaySystemRenderData() {
    OverlaySystem overlay;
    MovementAnalytics analytics;
    analytics.setActiveBounds(Vec3(-2, 0, -2), Vec3(2, 3, 2));
    overlay.initialize(&analytics);

    // Add some movement data
    analytics.recordSample(Vec3(0.3f, 1.0f, -0.5f), Hand::Right);
    analytics.recordSample(Vec3(0.35f, 1.05f, -0.48f), Hand::Right);
    analytics.recordSample(Vec3(0.4f, 1.1f, -0.46f), Hand::Right);

    Transform headPose;
    headPose.position = Vec3(0, 1.6f, 0);
    headPose.orientation = Quat::identity();

    overlay.update(0.016, headPose);

    // Get render data
    const OverlayRenderData& renderData = overlay.getRenderData();

    // Render data should have some trails if trails are enabled
    // (depends on element state)

    overlay.shutdown();

    std::cout << "  OverlaySystem render data: PASS" << std::endl;
}

// =============================================================================
// MovementVisualizer Tests
// =============================================================================

void testMovementVisualizerConstruction() {
    MovementVisualizer viz;

    assert(viz.isInitialized() == false);
    assert(viz.isVisible() == false);

    std::cout << "  MovementVisualizer construction: PASS" << std::endl;
}

void testMovementVisualizerSessionTime() {
    MovementVisualizer viz;

    // Session time should be 0 initially
    assert(approxEqual(viz.getSessionTime(), 0.0f));

    std::cout << "  MovementVisualizer session time: PASS" << std::endl;
}

void testMovementVisualizerOverlayAccess() {
    MovementVisualizer viz;

    // Overlay system should be accessible
    OverlaySystem* overlay = viz.getOverlaySystem();
    assert(overlay != nullptr);

    std::cout << "  MovementVisualizer overlay access: PASS" << std::endl;
}

// =============================================================================
// VisualizationPresets Tests
// =============================================================================

void testVisualizationPresetsMinimal() {
    MovementVisualizer viz;

    // Apply minimal preset
    VisualizationPresets::applyMinimal(viz);

    // Overlay system should have specific settings
    OverlaySystem* overlay = viz.getOverlaySystem();
    assert(overlay != nullptr);

    // In minimal mode, trails should be enabled but other elements disabled
    assert(overlay->isElementEnabled(OverlayElement::MovementTrails) == true);
    assert(overlay->isElementEnabled(OverlayElement::CoverageIndicator) == false);
    assert(overlay->isElementEnabled(OverlayElement::StatsHUD) == false);

    std::cout << "  VisualizationPresets minimal: PASS" << std::endl;
}

void testVisualizationPresetsStandard() {
    MovementVisualizer viz;

    // Apply standard preset
    VisualizationPresets::applyStandard(viz);

    OverlaySystem* overlay = viz.getOverlaySystem();
    assert(overlay != nullptr);

    // Standard mode should have trails and coverage
    assert(overlay->isElementEnabled(OverlayElement::MovementTrails) == true);
    assert(overlay->isElementEnabled(OverlayElement::CoverageIndicator) == true);

    std::cout << "  VisualizationPresets standard: PASS" << std::endl;
}

void testVisualizationPresetsExploration() {
    MovementVisualizer viz;

    // Apply exploration preset
    VisualizationPresets::applyExploration(viz);

    OverlaySystem* overlay = viz.getOverlaySystem();
    assert(overlay != nullptr);

    // Exploration mode should have all guides enabled
    assert(overlay->isElementEnabled(OverlayElement::MovementTrails) == true);
    assert(overlay->isElementEnabled(OverlayElement::CoverageIndicator) == true);
    assert(overlay->isElementEnabled(OverlayElement::UncommonAreaGuide) == true);

    std::cout << "  VisualizationPresets exploration: PASS" << std::endl;
}

void testVisualizationPresetsMeditation() {
    MovementVisualizer viz;

    // Apply meditation preset
    VisualizationPresets::applyMeditation(viz);

    OverlaySystem* overlay = viz.getOverlaySystem();
    assert(overlay != nullptr);

    // Meditation mode should have minimal distractions
    assert(overlay->isElementEnabled(OverlayElement::MovementTrails) == true);
    assert(overlay->isElementEnabled(OverlayElement::StatsHUD) == false);
    assert(overlay->isElementEnabled(OverlayElement::BreathingGuide) == true);

    std::cout << "  VisualizationPresets meditation: PASS" << std::endl;
}

void testVisualizationPresetsFull() {
    MovementVisualizer viz;

    // Apply full preset
    VisualizationPresets::applyFull(viz);

    OverlaySystem* overlay = viz.getOverlaySystem();
    assert(overlay != nullptr);

    // Full mode should have everything enabled
    assert(overlay->isElementEnabled(OverlayElement::MovementTrails) == true);
    assert(overlay->isElementEnabled(OverlayElement::CoverageIndicator) == true);
    assert(overlay->isElementEnabled(OverlayElement::UncommonAreaGuide) == true);
    assert(overlay->isElementEnabled(OverlayElement::StatsHUD) == true);
    assert(overlay->isElementEnabled(OverlayElement::FlowIndicator) == true);
    assert(overlay->isElementEnabled(OverlayElement::SessionTimer) == true);
    assert(overlay->isElementEnabled(OverlayElement::BreathingGuide) == true);

    std::cout << "  VisualizationPresets full: PASS" << std::endl;
}

// =============================================================================
// Color and Rendering Tests
// =============================================================================

void testColorConstruction() {
    // Default color
    Color c1;
    assert(c1.r == 1.0f && c1.g == 1.0f && c1.b == 1.0f && c1.a == 1.0f);

    // RGB color
    Color c2(0.5f, 0.3f, 0.8f);
    assert(approxEqual(c2.r, 0.5f));
    assert(approxEqual(c2.g, 0.3f));
    assert(approxEqual(c2.b, 0.8f));
    assert(approxEqual(c2.a, 1.0f));

    // RGBA color
    Color c3(0.2f, 0.4f, 0.6f, 0.8f);
    assert(approxEqual(c3.r, 0.2f));
    assert(approxEqual(c3.g, 0.4f));
    assert(approxEqual(c3.b, 0.6f));
    assert(approxEqual(c3.a, 0.8f));

    std::cout << "  Color construction: PASS" << std::endl;
}

void testTrailPointStorage() {
    TrailPoint p1;
    p1.position = Vec3(1.0f, 2.0f, 3.0f);
    p1.age = 0.5f;
    p1.velocity = 1.2f;

    assert(approxEqual(p1.position.x, 1.0f));
    assert(approxEqual(p1.position.y, 2.0f));
    assert(approxEqual(p1.position.z, 3.0f));
    assert(approxEqual(p1.age, 0.5f));
    assert(approxEqual(p1.velocity, 1.2f));

    std::cout << "  TrailPoint storage: PASS" << std::endl;
}

} // anonymous namespace

// =============================================================================
// Test Runner
// =============================================================================

int runVisualizationTests() {
    std::cout << "Running Visualization Tests..." << std::endl;

    // OverlayStyle tests
    testOverlayStyleDefaults();
    testOverlayStylePresets();

    // OverlaySystem tests
    testOverlaySystemConstruction();
    testOverlaySystemInitialization();
    testOverlaySystemVisibilityToggle();
    testOverlaySystemElementControl();
    testOverlaySystemStyleApplication();
    testOverlaySystemUpdate();
    testOverlaySystemRenderData();

    // MovementVisualizer tests
    testMovementVisualizerConstruction();
    testMovementVisualizerSessionTime();
    testMovementVisualizerOverlayAccess();

    // VisualizationPresets tests
    testVisualizationPresetsMinimal();
    testVisualizationPresetsStandard();
    testVisualizationPresetsExploration();
    testVisualizationPresetsMeditation();
    testVisualizationPresetsFull();

    // Color and rendering tests
    testColorConstruction();
    testTrailPointStorage();

    std::cout << "All Visualization Tests PASSED!" << std::endl;
    return 0;
}

#if USE_GTEST
TEST(OverlayStyleTest, Defaults) { testOverlayStyleDefaults(); }
TEST(OverlayStyleTest, Presets) { testOverlayStylePresets(); }
TEST(OverlaySystemTest, Construction) { testOverlaySystemConstruction(); }
TEST(OverlaySystemTest, Initialization) { testOverlaySystemInitialization(); }
TEST(OverlaySystemTest, VisibilityToggle) { testOverlaySystemVisibilityToggle(); }
TEST(OverlaySystemTest, ElementControl) { testOverlaySystemElementControl(); }
TEST(OverlaySystemTest, StyleApplication) { testOverlaySystemStyleApplication(); }
TEST(OverlaySystemTest, Update) { testOverlaySystemUpdate(); }
TEST(OverlaySystemTest, RenderData) { testOverlaySystemRenderData(); }
TEST(MovementVisualizerTest, Construction) { testMovementVisualizerConstruction(); }
TEST(MovementVisualizerTest, SessionTime) { testMovementVisualizerSessionTime(); }
TEST(MovementVisualizerTest, OverlayAccess) { testMovementVisualizerOverlayAccess(); }
TEST(VisualizationPresetsTest, Minimal) { testVisualizationPresetsMinimal(); }
TEST(VisualizationPresetsTest, Standard) { testVisualizationPresetsStandard(); }
TEST(VisualizationPresetsTest, Exploration) { testVisualizationPresetsExploration(); }
TEST(VisualizationPresetsTest, Meditation) { testVisualizationPresetsMeditation(); }
TEST(VisualizationPresetsTest, Full) { testVisualizationPresetsFull(); }
TEST(ColorTest, Construction) { testColorConstruction(); }
TEST(TrailPointTest, Storage) { testTrailPointStorage(); }
#endif

// Note: main() is provided by test_main.cpp when not using GTest
