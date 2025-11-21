#pragma once

#include "Types.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace lst {

class XRSession;

/**
 * Renderer - Basic rendering system for VR
 *
 * For Stage 1, this is a minimal stub that demonstrates the rendering
 * pipeline structure. In later stages, this would use Vulkan or OpenGL
 * for actual GPU rendering.
 */
class Renderer {
public:
    Renderer();
    ~Renderer();

    // Initialization
    bool initialize(XRSession* session);
    void shutdown();

    // Scene management
    void addObject(const SceneObject& object);
    void removeObject(const std::string& name);
    void clearScene();
    SceneObject* getObject(const std::string& name);

    // Per-frame rendering
    void beginFrame();
    void renderView(int viewIndex, const XRView& view);
    void endFrame();

    // Debug visualization
    void drawCube(const Transform& transform, const Color& color);
    void drawSphere(const Transform& transform, float radius, const Color& color);
    void drawSphere(const Vec3& position, float radius, const Color& color);
    void drawLine(const Vec3& start, const Vec3& end, const Color& color);
    void drawLine(const Vec3& start, const Vec3& end, const Color& color, float width);
    void drawGrid(float size, int divisions);
    void drawText(const Vec3& position, const std::string& text, const Color& color, float scale = 1.0f);

    // Camera
    void setCameraTransform(const Transform& transform);
    const Transform& getCameraTransform() const { return m_cameraTransform; }

    // Statistics
    int getDrawCallCount() const { return m_drawCallCount; }
    int getVertexCount() const { return m_vertexCount; }
    int getTriangleCount() const { return m_triangleCount; }

private:
    struct RenderCommand {
        Mesh mesh;
        Transform transform;
        Material material;
    };

    void submitDrawCommand(const RenderCommand& cmd);
    void flushDrawCommands();

    XRSession* m_session = nullptr;

    // Scene objects
    std::unordered_map<std::string, SceneObject> m_sceneObjects;

    // Render state
    Transform m_cameraTransform;
    Mat4 m_viewMatrix;
    Mat4 m_projectionMatrix;
    int m_currentViewIndex = 0;

    // Draw commands for current frame
    std::vector<RenderCommand> m_drawCommands;

    // Statistics
    int m_drawCallCount = 0;
    int m_vertexCount = 0;
    int m_triangleCount = 0;

    // Clear color
    Color m_clearColor = Color(0.1f, 0.1f, 0.15f, 1.0f);

    // Ground plane for visualization
    bool m_showGround = true;
    float m_groundSize = 10.0f;
};

} // namespace lst
