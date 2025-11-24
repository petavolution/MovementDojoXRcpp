#pragma once
/**
 * ProceduralMeshUtils.h - Procedural Mesh and Material Helpers
 *
 * Provides reusable building blocks for creating procedural geometry
 * and materials for training environments.
 *
 * Design principles:
 * - Simple, focused API for common primitives
 * - Integrates with existing Engine types (Mesh, Material, SceneObject)
 * - Input validation with graceful fallbacks
 * - Debug logging for mesh creation
 */

#include "../../include/Types.h"
#include <string>

namespace lst {

// Log tag for procedural mesh utilities
#define LOG_TAG_PROCEDURAL "Procedural"

// =============================================================================
// Mesh Creation Parameters
// =============================================================================

/**
 * Parameters for creating a grid plane (subdivided plane)
 * Used for surfaces that need per-vertex animation (water, terrain)
 */
struct GridPlaneParams {
    float width = 10.0f;          // X dimension
    float depth = 10.0f;          // Z dimension
    int subdivisionsX = 10;       // Number of subdivisions along X
    int subdivisionsZ = 10;       // Number of subdivisions along Z
    Vec3 center = Vec3(0, 0, 0);  // Center position
    Vec3 normal = Vec3(0, 1, 0);  // Surface normal (default: up)
};

/**
 * Parameters for creating a ring/torus shape
 * Used for decorative elements, weapon racks, etc.
 */
struct RingParams {
    float innerRadius = 0.8f;     // Inner hole radius
    float outerRadius = 1.0f;     // Outer edge radius
    int segments = 32;            // Number of angular segments
    Vec3 center = Vec3(0, 0, 0);  // Center position
    Vec3 normal = Vec3(0, 1, 0);  // Ring normal (default: up)
};

/**
 * Parameters for creating a line streak
 * Used for hyperspace star trails, motion lines
 */
struct LineStreakParams {
    float length = 2.0f;          // Length of the streak
    float width = 0.02f;          // Width of the streak
    Vec3 start = Vec3(0, 0, 0);   // Start position
    Vec3 direction = Vec3(0, 0, -1); // Direction (will be normalized)
};

/**
 * Parameters for creating a positioned box
 */
struct BoxParams {
    float width = 1.0f;           // X dimension
    float height = 1.0f;          // Y dimension
    float depth = 1.0f;           // Z dimension
    Vec3 center = Vec3(0, 0, 0);  // Center position
};

/**
 * Parameters for creating a positioned cylinder
 */
struct CylinderParams {
    float radius = 0.5f;          // Radius
    float height = 1.0f;          // Height
    int segments = 16;            // Number of angular segments (min: 3)
    Vec3 center = Vec3(0, 0, 0);  // Center position
    Vec3 axis = Vec3(0, 1, 0);    // Cylinder axis (default: up)
};

// =============================================================================
// Procedural Mesh Namespace
// =============================================================================

namespace ProceduralMesh {

    // -------------------------------------------------------------------------
    // Basic Primitives (return Mesh objects)
    // -------------------------------------------------------------------------

    /**
     * Create a subdivided grid plane
     * Useful for water surfaces, terrain, etc.
     */
    Mesh createGridPlane(const GridPlaneParams& params);

    /**
     * Create a flat ring (annulus) shape
     * Useful for decorative elements, weapon rack bases
     */
    Mesh createRing(const RingParams& params);

    /**
     * Create a line streak (elongated quad)
     * Useful for hyperspace star trails, motion effects
     */
    Mesh createLineStreak(const LineStreakParams& params);

    /**
     * Create a box at a specified position
     * Wrapper around Mesh::createCube with positioning
     */
    Mesh createBox(const BoxParams& params);

    /**
     * Create a cylinder with specified axis orientation
     * Wrapper around Mesh::createCylinder with orientation
     */
    Mesh createCylinder(const CylinderParams& params);

    // -------------------------------------------------------------------------
    // Convenience Functions (simpler signatures)
    // -------------------------------------------------------------------------

    /**
     * Create a simple plane at origin
     * @param width   Width (X dimension)
     * @param depth   Depth (Z dimension)
     */
    inline Mesh createSimplePlane(float width = 10.0f, float depth = 10.0f) {
        return Mesh::createPlane(width, depth);
    }

    /**
     * Create a box with uniform size
     * @param size   Size of each dimension
     */
    inline Mesh createUniformBox(float size = 1.0f) {
        return Mesh::createCube(size);
    }

    /**
     * Create a sphere
     * @param radius    Sphere radius
     * @param segments  Number of segments (higher = smoother)
     */
    inline Mesh createSphere(float radius = 1.0f, int segments = 16) {
        return Mesh::createSphere(radius, segments);
    }

    // -------------------------------------------------------------------------
    // Validation Helpers
    // -------------------------------------------------------------------------

    /**
     * Clamp segments to valid range [3, 128]
     */
    int clampSegments(int segments, int minVal = 3, int maxVal = 128);

    /**
     * Ensure positive dimension, return fallback if invalid
     */
    float ensurePositive(float value, float fallback = 1.0f);

} // namespace ProceduralMesh

// =============================================================================
// Material Helpers Namespace
// =============================================================================

namespace ProceduralMaterial {

    // -------------------------------------------------------------------------
    // Basic Material Creators
    // -------------------------------------------------------------------------

    /**
     * Create a simple unlit color material
     * @param color  Base color
     */
    Material createUnlitColor(const Color& color);

    /**
     * Create a material with emissive glow
     * @param color      Base color
     * @param emissive   Emissive intensity (0.0 - 1.0+)
     */
    Material createEmissive(const Color& color, float emissive = 0.5f);

    /**
     * Create a simple PBR-like material
     * @param baseColor  Base color
     * @param metallic   Metallic factor (0.0 - 1.0)
     * @param roughness  Roughness factor (0.0 - 1.0)
     */
    Material createPBR(const Color& baseColor, float metallic = 0.0f, float roughness = 0.5f);

    /**
     * Create a holographic/screen material (emissive with transparency hint)
     * @param color      Base color
     * @param intensity  Glow intensity
     */
    Material createHolographic(const Color& color, float intensity = 0.8f);

    // -------------------------------------------------------------------------
    // Preset Materials
    // -------------------------------------------------------------------------

    /** Wooden surface (brown, matte) */
    Material wood();

    /** Metal surface (gray, shiny) */
    Material metal();

    /** Water surface (blue, slightly reflective) */
    Material water();

    /** Glowing energy (cyan, high emissive) */
    Material energy();

    /** Dark floor (dark gray, matte) */
    Material darkFloor();

    /** Sky/background (light blue, matte) */
    Material sky();

    /** Hyperspace streak (white-blue, very emissive) */
    Material hyperspaceStreak();

} // namespace ProceduralMaterial

// =============================================================================
// SceneObject Builder
// =============================================================================

/**
 * Helper class for building SceneObjects with fluent API
 *
 * Usage:
 *   SceneObject obj = SceneObjectBuilder("MyFloor")
 *       .mesh(ProceduralMesh::createGridPlane(params))
 *       .material(ProceduralMaterial::water())
 *       .position(Vec3(0, -0.5f, 0))
 *       .scale(Vec3(10, 1, 10))
 *       .build();
 */
class SceneObjectBuilder {
public:
    explicit SceneObjectBuilder(const std::string& name);

    // Mesh
    SceneObjectBuilder& mesh(const Mesh& m);

    // Material
    SceneObjectBuilder& material(const Material& mat);
    SceneObjectBuilder& color(const Color& c);
    SceneObjectBuilder& emissive(float e);

    // Transform
    SceneObjectBuilder& position(const Vec3& pos);
    SceneObjectBuilder& rotation(const Quat& rot);
    SceneObjectBuilder& scale(const Vec3& s);
    SceneObjectBuilder& scale(float uniformScale);

    // Flags
    SceneObjectBuilder& visible(bool v);
    SceneObjectBuilder& isStatic(bool s);

    // Build final object
    SceneObject build() const;

private:
    SceneObject m_object;
};

} // namespace lst
