#include "Renderer.h"
#include "XRSession.h"
#include <iostream>
#include <cmath>

namespace lst {

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize(XRSession* session) {
    m_session = session;

    // In a real implementation, we would:
    // 1. Initialize Vulkan/OpenGL
    // 2. Create shaders
    // 3. Set up render pipelines
    // 4. Create vertex/index buffers

    std::cout << "Renderer initialized (stub mode)" << std::endl;
    std::cout << "  Clear color: (" << m_clearColor.r << ", "
              << m_clearColor.g << ", " << m_clearColor.b << ")" << std::endl;

    return true;
}

void Renderer::shutdown() {
    m_sceneObjects.clear();
    m_drawCommands.clear();
    m_session = nullptr;
    std::cout << "Renderer shutdown" << std::endl;
}

void Renderer::addObject(const SceneObject& object) {
    m_sceneObjects[object.name] = object;
    std::cout << "Added scene object: " << object.name
              << " (vertices=" << object.mesh.vertices.size()
              << ", indices=" << object.mesh.indices.size() << ")" << std::endl;
}

void Renderer::removeObject(const std::string& name) {
    auto it = m_sceneObjects.find(name);
    if (it != m_sceneObjects.end()) {
        m_sceneObjects.erase(it);
        std::cout << "Removed scene object: " << name << std::endl;
    }
}

void Renderer::clearScene() {
    m_sceneObjects.clear();
    std::cout << "Scene cleared" << std::endl;
}

SceneObject* Renderer::getObject(const std::string& name) {
    auto it = m_sceneObjects.find(name);
    if (it != m_sceneObjects.end()) {
        return &it->second;
    }
    return nullptr;
}

void Renderer::beginFrame() {
    m_drawCommands.clear();
    m_drawCallCount = 0;
    m_vertexCount = 0;
    m_triangleCount = 0;
}

void Renderer::renderView(int viewIndex, const XRView& view) {
    m_currentViewIndex = viewIndex;

    // Build view matrix from view pose
    // In OpenXR, the view pose is the position/orientation of the eye
    // We need to invert it to get the view matrix
    Quat invRotation = view.pose.orientation.conjugate();
    Vec3 invPosition = invRotation.rotate(view.pose.position * -1.0f);

    m_viewMatrix = Mat4::rotation(invRotation) * Mat4::translation(invPosition);

    // Build asymmetric projection matrix from FoV
    float left = std::tan(view.fov[0]);
    float right = std::tan(view.fov[1]);
    float up = std::tan(view.fov[2]);
    float down = std::tan(view.fov[3]);

    float nearZ = 0.05f;
    float farZ = 100.0f;

    m_projectionMatrix = Mat4::identity();
    m_projectionMatrix.m[0] = 2.0f / (right - left);
    m_projectionMatrix.m[5] = 2.0f / (up - down);
    m_projectionMatrix.m[8] = (right + left) / (right - left);
    m_projectionMatrix.m[9] = (up + down) / (up - down);
    m_projectionMatrix.m[10] = -(farZ + nearZ) / (farZ - nearZ);
    m_projectionMatrix.m[11] = -1.0f;
    m_projectionMatrix.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    m_projectionMatrix.m[15] = 0.0f;

    // Queue draw commands for all visible scene objects
    for (auto& [name, object] : m_sceneObjects) {
        if (!object.visible) continue;

        RenderCommand cmd;
        cmd.mesh = object.mesh;
        cmd.transform = object.transform;
        cmd.material = object.material;
        submitDrawCommand(cmd);
    }

    // Flush all draw commands
    flushDrawCommands();
}

void Renderer::endFrame() {
    // In a real implementation, we would:
    // 1. Present the swapchain images
    // 2. Signal frame completion

    // Print frame stats periodically
    static int frameCount = 0;
    if (++frameCount % 300 == 0) {
        std::cout << "Frame " << frameCount << ": "
                  << m_drawCallCount << " draw calls, "
                  << m_vertexCount << " verts, "
                  << m_triangleCount << " tris" << std::endl;
    }
}

void Renderer::drawCube(const Transform& transform, const Color& color) {
    RenderCommand cmd;
    cmd.mesh = Mesh::createCube(1.0f);
    cmd.transform = transform;
    cmd.material.baseColor = color;
    submitDrawCommand(cmd);
}

void Renderer::drawSphere(const Transform& transform, float radius, const Color& color) {
    Transform scaledTransform = transform;
    scaledTransform.scale = Vec3(radius, radius, radius);

    RenderCommand cmd;
    cmd.mesh = Mesh::createSphere(1.0f, 16);
    cmd.transform = scaledTransform;
    cmd.material.baseColor = color;
    submitDrawCommand(cmd);
}

void Renderer::drawSphere(const Vec3& position, float radius, const Color& color) {
    Transform transform;
    transform.position = position;
    transform.orientation = Quat::identity();
    transform.scale = Vec3(radius, radius, radius);

    RenderCommand cmd;
    cmd.mesh = Mesh::createSphere(1.0f, 8);  // Lower poly for many orbs
    cmd.transform = transform;
    cmd.material.baseColor = color;
    cmd.material.emissive = 0.5f;  // Slight glow for guide orbs
    submitDrawCommand(cmd);
}

void Renderer::drawLine(const Vec3& start, const Vec3& end, const Color& color) {
    // For stub mode, we just log lines
    // In real implementation, this would create a line primitive
    (void)start;
    (void)end;
    (void)color;
}

void Renderer::drawLine(const Vec3& start, const Vec3& end, const Color& color, float width) {
    // Create a thin cylinder between start and end points
    Vec3 dir = end - start;
    float length = dir.length();
    if (length < 0.001f) return;

    Vec3 center = (start + end) * 0.5f;
    Vec3 forward = dir.normalized();

    // Calculate rotation to orient cylinder along the line
    Vec3 up(0, 1, 0);
    if (std::abs(Vec3::dot(forward, up)) > 0.999f) {
        up = Vec3(1, 0, 0);
    }
    Vec3 right = Vec3::cross(up, forward).normalized();
    up = Vec3::cross(forward, right);

    // For simplicity, just create a stretched cube (real impl would use cylinder)
    Transform transform;
    transform.position = center;
    transform.scale = Vec3(width, width, length);
    // In a real implementation, we'd compute proper orientation

    RenderCommand cmd;
    cmd.mesh = Mesh::createCube(1.0f);
    cmd.transform = transform;
    cmd.material.baseColor = color;
    cmd.material.emissive = 0.3f;  // Slight glow for trails
    submitDrawCommand(cmd);
}

void Renderer::drawText(const Vec3& position, const std::string& text, const Color& color, float scale) {
    // Stub implementation - real version would use a text rendering library
    // For now, just track that text was requested (useful for logging)
    (void)position;
    (void)text;
    (void)color;
    (void)scale;
}

void Renderer::drawGrid(float size, int divisions) {
    // Draw a grid on the XZ plane
    float halfSize = size * 0.5f;
    float step = size / divisions;

    for (int i = 0; i <= divisions; i++) {
        float offset = -halfSize + i * step;

        // X-parallel lines
        drawLine(Vec3(-halfSize, 0, offset), Vec3(halfSize, 0, offset), Color(0.3f, 0.3f, 0.3f, 1.0f));

        // Z-parallel lines
        drawLine(Vec3(offset, 0, -halfSize), Vec3(offset, 0, halfSize), Color(0.3f, 0.3f, 0.3f, 1.0f));
    }
}

void Renderer::setCameraTransform(const Transform& transform) {
    m_cameraTransform = transform;
}

void Renderer::submitDrawCommand(const RenderCommand& cmd) {
    m_drawCommands.push_back(cmd);
}

void Renderer::flushDrawCommands() {
    for (const auto& cmd : m_drawCommands) {
        // In a real implementation, we would:
        // 1. Bind the appropriate shader
        // 2. Set uniforms (MVP matrix, material properties)
        // 3. Bind vertex/index buffers
        // 4. Issue draw call

        // For now, just count statistics
        m_drawCallCount++;
        m_vertexCount += static_cast<int>(cmd.mesh.vertices.size());
        m_triangleCount += static_cast<int>(cmd.mesh.indices.size()) / 3;
    }

    m_drawCommands.clear();
}

} // namespace lst
