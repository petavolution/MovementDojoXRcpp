/**
 * ProceduralMeshUtils.cpp - Procedural Mesh and Material Helpers Implementation
 */

#include "ProceduralMeshUtils.h"
#include "../core/Logger.h"
#include <cmath>
#include <algorithm>

namespace lst {

// =============================================================================
// Validation Helpers
// =============================================================================

namespace ProceduralMesh {

int clampSegments(int segments, int minVal, int maxVal) {
    if (segments < minVal) {
        LOG_WARN(LOG_TAG_PROCEDURAL) << "Segments " << segments << " below minimum, clamping to " << minVal;
        return minVal;
    }
    if (segments > maxVal) {
        LOG_WARN(LOG_TAG_PROCEDURAL) << "Segments " << segments << " above maximum, clamping to " << maxVal;
        return maxVal;
    }
    return segments;
}

float ensurePositive(float value, float fallback) {
    if (value <= 0.0f) {
        LOG_WARN(LOG_TAG_PROCEDURAL) << "Invalid dimension " << value << ", using fallback " << fallback;
        return fallback;
    }
    return value;
}

// =============================================================================
// Grid Plane
// =============================================================================

Mesh createGridPlane(const GridPlaneParams& params) {
    Mesh mesh;
    mesh.name = "GridPlane";

    // Validate parameters
    float width = ensurePositive(params.width, 10.0f);
    float depth = ensurePositive(params.depth, 10.0f);
    int subsX = clampSegments(params.subdivisionsX, 1, 256);
    int subsZ = clampSegments(params.subdivisionsZ, 1, 256);

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "Creating GridPlane: " << width << "x" << depth
                                   << " subdivisions=" << subsX << "x" << subsZ;

    // Calculate the orientation basis
    Vec3 normal = params.normal.normalized();
    Vec3 tangent, bitangent;

    // Create orthonormal basis from normal
    if (std::abs(normal.y) < 0.999f) {
        tangent = Vec3::cross(Vec3(0, 1, 0), normal).normalized();
    } else {
        tangent = Vec3::cross(Vec3(0, 0, 1), normal).normalized();
    }
    bitangent = Vec3::cross(normal, tangent).normalized();

    // Generate vertices
    int verticesPerRow = subsX + 1;
    int verticesPerCol = subsZ + 1;
    int totalVertices = verticesPerRow * verticesPerCol;
    mesh.vertices.reserve(totalVertices);

    float halfWidth = width * 0.5f;
    float halfDepth = depth * 0.5f;

    for (int z = 0; z <= subsZ; z++) {
        float zFrac = static_cast<float>(z) / subsZ;
        float localZ = -halfDepth + zFrac * depth;

        for (int x = 0; x <= subsX; x++) {
            float xFrac = static_cast<float>(x) / subsX;
            float localX = -halfWidth + xFrac * width;

            Vertex v;
            // Transform local position to world position using basis
            v.position = params.center + tangent * localX + bitangent * localZ;
            v.normal = normal;
            v.u = xFrac;
            v.v = zFrac;
            v.color = Color::white();

            mesh.vertices.push_back(v);
        }
    }

    // Generate indices (two triangles per grid cell)
    int totalQuads = subsX * subsZ;
    mesh.indices.reserve(totalQuads * 6);

    for (int z = 0; z < subsZ; z++) {
        for (int x = 0; x < subsX; x++) {
            int topLeft = z * verticesPerRow + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * verticesPerRow + x;
            int bottomRight = bottomLeft + 1;

            // First triangle
            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topRight);

            // Second triangle
            mesh.indices.push_back(topRight);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(bottomRight);
        }
    }

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "GridPlane created: " << mesh.vertices.size()
                                   << " vertices, " << mesh.indices.size() / 3 << " triangles";
    return mesh;
}

// =============================================================================
// Ring (Annulus)
// =============================================================================

Mesh createRing(const RingParams& params) {
    Mesh mesh;
    mesh.name = "Ring";

    // Validate parameters
    float innerR = ensurePositive(params.innerRadius, 0.5f);
    float outerR = ensurePositive(params.outerRadius, 1.0f);
    int segments = clampSegments(params.segments, 3, 128);

    // Ensure inner < outer
    if (innerR >= outerR) {
        LOG_WARN(LOG_TAG_PROCEDURAL) << "Inner radius >= outer radius, swapping";
        std::swap(innerR, outerR);
    }

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "Creating Ring: inner=" << innerR << " outer=" << outerR
                                   << " segments=" << segments;

    // Calculate the orientation basis
    Vec3 normal = params.normal.normalized();
    Vec3 tangent, bitangent;

    if (std::abs(normal.y) < 0.999f) {
        tangent = Vec3::cross(Vec3(0, 1, 0), normal).normalized();
    } else {
        tangent = Vec3::cross(Vec3(0, 0, 1), normal).normalized();
    }
    bitangent = Vec3::cross(normal, tangent).normalized();

    // Generate vertices (inner and outer rings)
    mesh.vertices.reserve((segments + 1) * 2);

    for (int i = 0; i <= segments; i++) {
        float angle = (static_cast<float>(i) / segments) * 2.0f * 3.14159265f;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        // Inner vertex
        Vertex vInner;
        vInner.position = params.center + tangent * (cosA * innerR) + bitangent * (sinA * innerR);
        vInner.normal = normal;
        vInner.u = static_cast<float>(i) / segments;
        vInner.v = 0.0f;
        vInner.color = Color::white();
        mesh.vertices.push_back(vInner);

        // Outer vertex
        Vertex vOuter;
        vOuter.position = params.center + tangent * (cosA * outerR) + bitangent * (sinA * outerR);
        vOuter.normal = normal;
        vOuter.u = static_cast<float>(i) / segments;
        vOuter.v = 1.0f;
        vOuter.color = Color::white();
        mesh.vertices.push_back(vOuter);
    }

    // Generate indices
    mesh.indices.reserve(segments * 6);

    for (int i = 0; i < segments; i++) {
        int innerCurrent = i * 2;
        int outerCurrent = i * 2 + 1;
        int innerNext = (i + 1) * 2;
        int outerNext = (i + 1) * 2 + 1;

        // First triangle
        mesh.indices.push_back(innerCurrent);
        mesh.indices.push_back(outerCurrent);
        mesh.indices.push_back(innerNext);

        // Second triangle
        mesh.indices.push_back(innerNext);
        mesh.indices.push_back(outerCurrent);
        mesh.indices.push_back(outerNext);
    }

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "Ring created: " << mesh.vertices.size()
                                   << " vertices, " << mesh.indices.size() / 3 << " triangles";
    return mesh;
}

// =============================================================================
// Line Streak
// =============================================================================

Mesh createLineStreak(const LineStreakParams& params) {
    Mesh mesh;
    mesh.name = "LineStreak";

    // Validate parameters
    float length = ensurePositive(params.length, 1.0f);
    float width = ensurePositive(params.width, 0.02f);

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "Creating LineStreak: length=" << length << " width=" << width;

    Vec3 direction = params.direction.normalized();
    Vec3 end = params.start + direction * length;

    // Create perpendicular vector for width
    Vec3 perpendicular;
    if (std::abs(direction.y) < 0.999f) {
        perpendicular = Vec3::cross(direction, Vec3(0, 1, 0)).normalized();
    } else {
        perpendicular = Vec3::cross(direction, Vec3(1, 0, 0)).normalized();
    }

    Vec3 offset = perpendicular * (width * 0.5f);

    // Create 4 vertices for the quad
    mesh.vertices.reserve(4);

    Vertex v0, v1, v2, v3;

    // Start edge
    v0.position = params.start - offset;
    v0.normal = Vec3::cross(direction, perpendicular).normalized();
    v0.u = 0.0f; v0.v = 0.0f;
    v0.color = Color::white();

    v1.position = params.start + offset;
    v1.normal = v0.normal;
    v1.u = 1.0f; v1.v = 0.0f;
    v1.color = Color::white();

    // End edge
    v2.position = end - offset;
    v2.normal = v0.normal;
    v2.u = 0.0f; v2.v = 1.0f;
    v2.color = Color::white();

    v3.position = end + offset;
    v3.normal = v0.normal;
    v3.u = 1.0f; v3.v = 1.0f;
    v3.color = Color::white();

    mesh.vertices.push_back(v0);
    mesh.vertices.push_back(v1);
    mesh.vertices.push_back(v2);
    mesh.vertices.push_back(v3);

    // Two triangles
    mesh.indices = {0, 2, 1, 1, 2, 3};

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "LineStreak created: 4 vertices, 2 triangles";
    return mesh;
}

// =============================================================================
// Box (Positioned Cube)
// =============================================================================

Mesh createBox(const BoxParams& params) {
    Mesh mesh;
    mesh.name = "Box";

    float w = ensurePositive(params.width, 1.0f);
    float h = ensurePositive(params.height, 1.0f);
    float d = ensurePositive(params.depth, 1.0f);

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "Creating Box: " << w << "x" << h << "x" << d
                                   << " at (" << params.center.x << ", "
                                   << params.center.y << ", " << params.center.z << ")";

    // Create unit cube and scale/position vertices
    mesh = Mesh::createCube(1.0f);

    // Scale and translate all vertices
    for (auto& vertex : mesh.vertices) {
        vertex.position.x = vertex.position.x * w + params.center.x;
        vertex.position.y = vertex.position.y * h + params.center.y;
        vertex.position.z = vertex.position.z * d + params.center.z;
    }

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "Box created: " << mesh.vertices.size()
                                   << " vertices, " << mesh.indices.size() / 3 << " triangles";
    return mesh;
}

// =============================================================================
// Cylinder (Oriented)
// =============================================================================

Mesh createCylinder(const CylinderParams& params) {
    Mesh mesh;
    mesh.name = "Cylinder";

    float radius = ensurePositive(params.radius, 0.5f);
    float height = ensurePositive(params.height, 1.0f);
    int segments = clampSegments(params.segments, 3, 128);

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "Creating Cylinder: radius=" << radius << " height=" << height
                                   << " segments=" << segments;

    // Create base cylinder along Y axis
    mesh = Mesh::createCylinder(radius, height, segments);

    // If axis is not Y, we need to rotate the cylinder
    Vec3 axis = params.axis.normalized();
    Vec3 yAxis(0, 1, 0);

    if (std::abs(Vec3::dot(axis, yAxis)) < 0.999f) {
        // Need to rotate
        Vec3 rotAxis = Vec3::cross(yAxis, axis).normalized();
        float angle = std::acos(Vec3::dot(yAxis, axis));
        Quat rotation = Quat::fromAxisAngle(rotAxis, angle);

        for (auto& vertex : mesh.vertices) {
            vertex.position = rotation.rotate(vertex.position);
            vertex.normal = rotation.rotate(vertex.normal);
        }
    }

    // Translate to center
    for (auto& vertex : mesh.vertices) {
        vertex.position = vertex.position + params.center;
    }

    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "Cylinder created: " << mesh.vertices.size()
                                   << " vertices, " << mesh.indices.size() / 3 << " triangles";
    return mesh;
}

} // namespace ProceduralMesh

// =============================================================================
// Material Helpers
// =============================================================================

namespace ProceduralMaterial {

Material createUnlitColor(const Color& color) {
    Material mat;
    mat.baseColor = color;
    mat.metallic = 0.0f;
    mat.roughness = 1.0f;  // Full rough = no specular
    mat.emissive = 0.0f;
    return mat;
}

Material createEmissive(const Color& color, float emissive) {
    Material mat;
    mat.baseColor = color;
    mat.metallic = 0.0f;
    mat.roughness = 0.5f;
    mat.emissive = std::max(0.0f, emissive);
    return mat;
}

Material createPBR(const Color& baseColor, float metallic, float roughness) {
    Material mat;
    mat.baseColor = baseColor;
    mat.metallic = std::clamp(metallic, 0.0f, 1.0f);
    mat.roughness = std::clamp(roughness, 0.0f, 1.0f);
    mat.emissive = 0.0f;
    return mat;
}

Material createHolographic(const Color& color, float intensity) {
    Material mat;
    mat.baseColor = color;
    mat.metallic = 0.0f;
    mat.roughness = 0.2f;
    mat.emissive = std::max(0.0f, intensity);
    return mat;
}

// -------------------------------------------------------------------------
// Preset Materials
// -------------------------------------------------------------------------

Material wood() {
    Material mat;
    mat.name = "Wood";
    mat.baseColor = Color(0.55f, 0.35f, 0.15f);  // Brown
    mat.metallic = 0.0f;
    mat.roughness = 0.8f;
    mat.emissive = 0.0f;
    return mat;
}

Material metal() {
    Material mat;
    mat.name = "Metal";
    mat.baseColor = Color(0.7f, 0.7f, 0.75f);  // Silver-gray
    mat.metallic = 0.9f;
    mat.roughness = 0.3f;
    mat.emissive = 0.0f;
    return mat;
}

Material water() {
    Material mat;
    mat.name = "Water";
    mat.baseColor = Color(0.1f, 0.3f, 0.5f, 0.8f);  // Blue, semi-transparent
    mat.metallic = 0.0f;
    mat.roughness = 0.1f;  // Smooth/reflective
    mat.emissive = 0.05f;  // Slight glow for visibility
    return mat;
}

Material energy() {
    Material mat;
    mat.name = "Energy";
    mat.baseColor = Color(0.2f, 0.9f, 1.0f);  // Cyan
    mat.metallic = 0.0f;
    mat.roughness = 0.5f;
    mat.emissive = 0.7f;  // Strong glow
    return mat;
}

Material darkFloor() {
    Material mat;
    mat.name = "DarkFloor";
    mat.baseColor = Color(0.15f, 0.15f, 0.2f);  // Dark blue-gray
    mat.metallic = 0.0f;
    mat.roughness = 0.7f;
    mat.emissive = 0.0f;
    return mat;
}

Material sky() {
    Material mat;
    mat.name = "Sky";
    mat.baseColor = Color(0.4f, 0.6f, 0.9f);  // Light blue
    mat.metallic = 0.0f;
    mat.roughness = 1.0f;
    mat.emissive = 0.1f;  // Slight ambient glow
    return mat;
}

Material hyperspaceStreak() {
    Material mat;
    mat.name = "HyperspaceStreak";
    mat.baseColor = Color(0.8f, 0.9f, 1.0f);  // White-blue
    mat.metallic = 0.0f;
    mat.roughness = 0.2f;
    mat.emissive = 1.0f;  // Maximum glow
    return mat;
}

} // namespace ProceduralMaterial

// =============================================================================
// SceneObject Builder
// =============================================================================

SceneObjectBuilder::SceneObjectBuilder(const std::string& name) {
    m_object.name = name;
    m_object.visible = true;
    m_object.isStatic = true;
}

SceneObjectBuilder& SceneObjectBuilder::mesh(const Mesh& m) {
    m_object.mesh = m;
    return *this;
}

SceneObjectBuilder& SceneObjectBuilder::material(const Material& mat) {
    m_object.material = mat;
    return *this;
}

SceneObjectBuilder& SceneObjectBuilder::color(const Color& c) {
    m_object.material.baseColor = c;
    return *this;
}

SceneObjectBuilder& SceneObjectBuilder::emissive(float e) {
    m_object.material.emissive = e;
    return *this;
}

SceneObjectBuilder& SceneObjectBuilder::position(const Vec3& pos) {
    m_object.transform.position = pos;
    return *this;
}

SceneObjectBuilder& SceneObjectBuilder::rotation(const Quat& rot) {
    m_object.transform.orientation = rot;
    return *this;
}

SceneObjectBuilder& SceneObjectBuilder::scale(const Vec3& s) {
    m_object.transform.scale = s;
    return *this;
}

SceneObjectBuilder& SceneObjectBuilder::scale(float uniformScale) {
    m_object.transform.scale = Vec3(uniformScale, uniformScale, uniformScale);
    return *this;
}

SceneObjectBuilder& SceneObjectBuilder::visible(bool v) {
    m_object.visible = v;
    return *this;
}

SceneObjectBuilder& SceneObjectBuilder::isStatic(bool s) {
    m_object.isStatic = s;
    return *this;
}

SceneObject SceneObjectBuilder::build() const {
    LOG_DEBUG(LOG_TAG_PROCEDURAL) << "Built SceneObject: " << m_object.name;
    return m_object;
}

} // namespace lst
