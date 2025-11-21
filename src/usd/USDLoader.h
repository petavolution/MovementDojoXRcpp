#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace lst {

/**
 * USDLoader - Loads and parses USD scene files
 *
 * Handles:
 * - Loading .usda/.usd/.usdc files
 * - Traversing prim hierarchy
 * - Extracting geometry (UsdGeomMesh)
 * - Extracting materials (UsdPreviewSurface)
 * - Extracting transforms (UsdGeomXformable)
 * - Extracting lights (UsdLux)
 *
 * When HAS_USD is not defined, provides stub implementations
 * that create procedural geometry for testing.
 */
class USDLoader {
public:
    USDLoader();
    ~USDLoader();

    // Load a USD stage from file
    bool loadStage(const std::string& filePath);

    // Unload the current stage
    void unloadStage();

    // Check if a stage is loaded
    bool isLoaded() const { return m_isLoaded; }

    // Get loaded scene objects
    const std::vector<SceneObject>& getSceneObjects() const { return m_sceneObjects; }

    // Get default prim name
    std::string getDefaultPrimName() const { return m_defaultPrimName; }

    // Traverse callback
    using TraverseCallback = std::function<void(const std::string& primPath,
                                                 const std::string& typeName,
                                                 const Transform& transform)>;
    void traverse(TraverseCallback callback);

    // Validation
    bool validate(std::string& errorMessage);

    // Stage info
    std::string getUpAxis() const { return m_upAxis; }
    double getMetersPerUnit() const { return m_metersPerUnit; }

private:
    // Parse a mesh prim
    bool parseMesh(const std::string& primPath, SceneObject& object);

    // Parse a cube prim
    bool parseCube(const std::string& primPath, SceneObject& object);

    // Parse material
    bool parseMaterial(const std::string& primPath, Material& material);

    // Parse transform
    bool parseTransform(const std::string& primPath, Transform& transform);

    // Create stub geometry when USD is not available
    void createStubScene();

    bool m_isLoaded = false;
    std::string m_filePath;
    std::string m_defaultPrimName;
    std::string m_upAxis = "Y";
    double m_metersPerUnit = 1.0;

    std::vector<SceneObject> m_sceneObjects;

    // Opaque pointer to USD stage (only used when HAS_USD=1)
    void* m_stage = nullptr;
};

/**
 * USDSceneGenerator - Creates USD scenes programmatically
 *
 * Used for generating test scenes and procedural content.
 */
class USDSceneGenerator {
public:
    USDSceneGenerator();
    ~USDSceneGenerator();

    // Create a new stage
    bool createStage(const std::string& filePath);

    // Set stage metadata
    void setUpAxis(const std::string& axis);
    void setMetersPerUnit(double meters);
    void setDefaultPrim(const std::string& primPath);

    // Add primitives
    void addCube(const std::string& primPath, float size, const Transform& transform, const Color& color);
    void addSphere(const std::string& primPath, float radius, const Transform& transform, const Color& color);
    void addPlane(const std::string& primPath, float width, float depth, const Transform& transform, const Color& color);
    void addMesh(const std::string& primPath, const Mesh& mesh, const Transform& transform, const Material& material);

    // Add lights
    void addDistantLight(const std::string& primPath, const Vec3& direction, float intensity, const Color& color);
    void addPointLight(const std::string& primPath, const Vec3& position, float intensity, const Color& color);

    // Add custom attributes (for training data)
    void addCustomAttribute(const std::string& primPath, const std::string& attrName, const std::string& value);

    // Save the stage
    bool save();

private:
    std::string m_filePath;
    void* m_stage = nullptr;

    // For non-USD builds, store scene as text
    std::string m_usdaContent;
};

} // namespace lst
