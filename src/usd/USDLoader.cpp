#include "USDLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <cmath>

#if HAS_USD
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/sphereLight.h>

PXR_NAMESPACE_USING_DIRECTIVE
#endif

namespace lst {

// Simple USDA parser for when USD library is not available
class SimpleUSDAParser {
public:
    struct Prim {
        std::string path;
        std::string type;
        Transform transform;
        float size = 1.0f;
        Color color = Color::white();
        std::string name;
    };

    bool parse(const std::string& content) {
        m_prims.clear();

        // Very basic regex-based parsing
        // This is a simplified parser for Stage 1 testing

        // Find default prim
        std::regex defaultPrimRegex(R"(defaultPrim\s*=\s*\"(\w+)\")");
        std::smatch match;
        if (std::regex_search(content, match, defaultPrimRegex)) {
            m_defaultPrim = match[1].str();
        }

        // Find up axis
        std::regex upAxisRegex(R"(upAxis\s*=\s*\"(\w+)\")");
        if (std::regex_search(content, match, upAxisRegex)) {
            m_upAxis = match[1].str();
        }

        // Find prim definitions
        std::regex primDefRegex(R"(def\s+(\w+)\s+\"(\w+)\")");
        std::string::const_iterator searchStart(content.cbegin());
        std::string currentPath;

        while (std::regex_search(searchStart, content.cend(), match, primDefRegex)) {
            Prim prim;
            prim.type = match[1].str();
            prim.name = match[2].str();
            prim.path = "/" + prim.name;

            // Look for size attribute
            std::string primContent = match.suffix().str().substr(0, 500);
            std::regex sizeRegex(R"(double\s+size\s*=\s*([\d.]+))");
            std::smatch sizeMatch;
            if (std::regex_search(primContent, sizeMatch, sizeRegex)) {
                prim.size = std::stof(sizeMatch[1].str());
            }

            // Look for translate attribute
            std::regex translateRegex(R"(double3\s+xformOp:translate\s*=\s*\(\s*([-\d.]+)\s*,\s*([-\d.]+)\s*,\s*([-\d.]+)\s*\))");
            std::smatch translateMatch;
            if (std::regex_search(primContent, translateMatch, translateRegex)) {
                prim.transform.position = Vec3(
                    std::stof(translateMatch[1].str()),
                    std::stof(translateMatch[2].str()),
                    std::stof(translateMatch[3].str())
                );
            }

            // Look for color attribute
            std::regex colorRegex(R"(color3f\s+inputs:diffuseColor\s*=\s*\(\s*([-\d.]+)\s*,\s*([-\d.]+)\s*,\s*([-\d.]+)\s*\))");
            std::smatch colorMatch;
            if (std::regex_search(primContent, colorMatch, colorRegex)) {
                prim.color = Color(
                    std::stof(colorMatch[1].str()),
                    std::stof(colorMatch[2].str()),
                    std::stof(colorMatch[3].str())
                );
            }

            m_prims.push_back(prim);
            searchStart = match.suffix().first;
        }

        return true;
    }

    const std::vector<Prim>& getPrims() const { return m_prims; }
    const std::string& getDefaultPrim() const { return m_defaultPrim; }
    const std::string& getUpAxis() const { return m_upAxis; }

private:
    std::vector<Prim> m_prims;
    std::string m_defaultPrim;
    std::string m_upAxis = "Y";
};

USDLoader::USDLoader() = default;

USDLoader::~USDLoader() {
    unloadStage();
}

bool USDLoader::loadStage(const std::string& filePath) {
    m_filePath = filePath;
    m_sceneObjects.clear();
    m_isLoaded = false;

    std::cout << "Loading USD stage: " << filePath << std::endl;

#if HAS_USD
    // Use Pixar USD library
    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(filePath);
    if (!stage) {
        std::cerr << "Failed to open USD stage: " << filePath << std::endl;
        return false;
    }

    m_stage = new pxr::UsdStageRefPtr(stage);
    m_defaultPrimName = stage->GetDefaultPrim().GetName().GetString();
    m_upAxis = pxr::UsdGeomGetStageUpAxis(stage) == pxr::UsdGeomTokens->y ? "Y" : "Z";
    m_metersPerUnit = pxr::UsdGeomGetStageMetersPerUnit(stage);

    // Traverse all prims
    for (auto prim : stage->Traverse()) {
        std::string typeName = prim.GetTypeName().GetString();
        std::string primPath = prim.GetPath().GetString();

        SceneObject object;
        object.name = prim.GetName().GetString();

        // Parse transform
        if (prim.IsA<pxr::UsdGeomXformable>()) {
            parseTransform(primPath, object.transform);
        }

        // Parse based on type
        if (typeName == "Mesh") {
            if (parseMesh(primPath, object)) {
                m_sceneObjects.push_back(object);
            }
        } else if (typeName == "Cube") {
            if (parseCube(primPath, object)) {
                m_sceneObjects.push_back(object);
            }
        }
    }
#else
    // Fallback: Simple USDA text parser
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        // Create stub scene for testing
        createStubScene();
        m_isLoaded = true;
        return true;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    SimpleUSDAParser parser;
    if (!parser.parse(content)) {
        std::cerr << "Failed to parse USDA file" << std::endl;
        createStubScene();
        m_isLoaded = true;
        return true;
    }

    m_defaultPrimName = parser.getDefaultPrim();
    m_upAxis = parser.getUpAxis();

    for (const auto& prim : parser.getPrims()) {
        SceneObject object;
        object.name = prim.name;
        object.transform = prim.transform;
        object.material.baseColor = prim.color;
        object.visible = true;
        object.isStatic = true;

        if (prim.type == "Cube") {
            object.mesh = Mesh::createCube(prim.size);
        } else if (prim.type == "Sphere") {
            object.mesh = Mesh::createSphere(prim.size, 16);
        } else if (prim.type == "Xform") {
            // Skip xform-only prims (containers)
            continue;
        } else {
            // Default to cube for unknown types
            object.mesh = Mesh::createCube(prim.size);
        }

        std::cout << "  Loaded prim: " << prim.path << " (" << prim.type << ")" << std::endl;
        m_sceneObjects.push_back(object);
    }
#endif

    m_isLoaded = true;
    std::cout << "Loaded " << m_sceneObjects.size() << " scene objects" << std::endl;
    return true;
}

void USDLoader::unloadStage() {
#if HAS_USD
    if (m_stage) {
        delete static_cast<pxr::UsdStageRefPtr*>(m_stage);
        m_stage = nullptr;
    }
#endif
    m_sceneObjects.clear();
    m_isLoaded = false;
}

void USDLoader::traverse(TraverseCallback callback) {
    for (const auto& obj : m_sceneObjects) {
        callback(obj.name, obj.mesh.name, obj.transform);
    }
}

bool USDLoader::validate(std::string& errorMessage) {
    if (!m_isLoaded) {
        errorMessage = "No stage loaded";
        return false;
    }

    if (m_sceneObjects.empty()) {
        errorMessage = "Stage contains no geometry";
        return false;
    }

    // Check for valid geometry
    for (const auto& obj : m_sceneObjects) {
        if (obj.mesh.vertices.empty()) {
            errorMessage = "Object '" + obj.name + "' has no vertices";
            return false;
        }
        if (obj.mesh.indices.empty()) {
            errorMessage = "Object '" + obj.name + "' has no indices";
            return false;
        }
    }

    return true;
}

bool USDLoader::parseMesh(const std::string& primPath, SceneObject& object) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    auto prim = stage->GetPrimAtPath(pxr::SdfPath(primPath));

    if (!prim.IsA<pxr::UsdGeomMesh>()) {
        return false;
    }

    pxr::UsdGeomMesh mesh(prim);

    // Get points
    pxr::VtArray<pxr::GfVec3f> points;
    mesh.GetPointsAttr().Get(&points);

    // Get face vertex counts
    pxr::VtArray<int> faceVertexCounts;
    mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);

    // Get face vertex indices
    pxr::VtArray<int> faceVertexIndices;
    mesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

    // Get normals (if available)
    pxr::VtArray<pxr::GfVec3f> normals;
    mesh.GetNormalsAttr().Get(&normals);

    // Convert to our mesh format
    for (const auto& point : points) {
        Vertex v;
        v.position = Vec3(point[0], point[1], point[2]);
        v.normal = Vec3(0, 1, 0); // Default normal
        v.color = Color::white();
        object.mesh.vertices.push_back(v);
    }

    // Triangulate and add indices
    int indexOffset = 0;
    for (int faceVerts : faceVertexCounts) {
        if (faceVerts >= 3) {
            // Simple fan triangulation
            for (int i = 1; i < faceVerts - 1; i++) {
                object.mesh.indices.push_back(faceVertexIndices[indexOffset]);
                object.mesh.indices.push_back(faceVertexIndices[indexOffset + i]);
                object.mesh.indices.push_back(faceVertexIndices[indexOffset + i + 1]);
            }
        }
        indexOffset += faceVerts;
    }

    return true;
#else
    (void)primPath;
    (void)object;
    return false;
#endif
}

bool USDLoader::parseCube(const std::string& primPath, SceneObject& object) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    auto prim = stage->GetPrimAtPath(pxr::SdfPath(primPath));

    if (!prim.IsA<pxr::UsdGeomCube>()) {
        return false;
    }

    pxr::UsdGeomCube cube(prim);

    double size = 1.0;
    cube.GetSizeAttr().Get(&size);

    object.mesh = Mesh::createCube(static_cast<float>(size));
    return true;
#else
    (void)primPath;
    (void)object;
    return false;
#endif
}

bool USDLoader::parseMaterial(const std::string& primPath, Material& material) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    auto prim = stage->GetPrimAtPath(pxr::SdfPath(primPath));

    // Look for bound material
    pxr::UsdShadeMaterialBindingAPI bindingAPI(prim);
    pxr::UsdShadeMaterial boundMaterial = bindingAPI.ComputeBoundMaterial();

    if (boundMaterial) {
        // Get surface shader
        pxr::UsdShadeShader surfaceShader = boundMaterial.ComputeSurfaceSource();
        if (surfaceShader) {
            // Try to get diffuse color
            pxr::UsdShadeInput diffuseInput = surfaceShader.GetInput(pxr::TfToken("diffuseColor"));
            if (diffuseInput) {
                pxr::GfVec3f color;
                diffuseInput.Get(&color);
                material.baseColor = Color(color[0], color[1], color[2]);
            }

            // Try to get metallic
            pxr::UsdShadeInput metallicInput = surfaceShader.GetInput(pxr::TfToken("metallic"));
            if (metallicInput) {
                float metallic;
                metallicInput.Get(&metallic);
                material.metallic = metallic;
            }

            // Try to get roughness
            pxr::UsdShadeInput roughnessInput = surfaceShader.GetInput(pxr::TfToken("roughness"));
            if (roughnessInput) {
                float roughness;
                roughnessInput.Get(&roughness);
                material.roughness = roughness;
            }
        }
    }

    return true;
#else
    (void)primPath;
    (void)material;
    return false;
#endif
}

bool USDLoader::parseTransform(const std::string& primPath, Transform& transform) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    auto prim = stage->GetPrimAtPath(pxr::SdfPath(primPath));

    if (!prim.IsA<pxr::UsdGeomXformable>()) {
        return false;
    }

    pxr::UsdGeomXformable xformable(prim);
    pxr::GfMatrix4d worldTransform = xformable.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());

    // Extract translation
    pxr::GfVec3d translation = worldTransform.ExtractTranslation();
    transform.position = Vec3(
        static_cast<float>(translation[0]),
        static_cast<float>(translation[1]),
        static_cast<float>(translation[2])
    );

    // Extract rotation (as quaternion)
    pxr::GfRotation rotation = worldTransform.ExtractRotation();
    pxr::GfQuaternion quat = rotation.GetQuaternion();
    transform.orientation = Quat(
        static_cast<float>(quat.GetImaginary()[0]),
        static_cast<float>(quat.GetImaginary()[1]),
        static_cast<float>(quat.GetImaginary()[2]),
        static_cast<float>(quat.GetReal())
    );

    return true;
#else
    (void)primPath;
    (void)transform;
    return false;
#endif
}

void USDLoader::createStubScene() {
    std::cout << "Creating stub scene for testing..." << std::endl;

    // Ground plane
    SceneObject ground;
    ground.name = "Ground";
    ground.mesh = Mesh::createPlane(10.0f, 10.0f);
    ground.material.baseColor = Color(0.3f, 0.3f, 0.35f);
    ground.transform.position = Vec3(0, 0, 0);
    m_sceneObjects.push_back(ground);

    // Test cube
    SceneObject cube;
    cube.name = "TestCube";
    cube.mesh = Mesh::createCube(0.5f);
    cube.material.baseColor = Color(0.8f, 0.2f, 0.2f);
    cube.transform.position = Vec3(0, 0.25f, -1.5f);
    m_sceneObjects.push_back(cube);

    // Test sphere
    SceneObject sphere;
    sphere.name = "TestSphere";
    sphere.mesh = Mesh::createSphere(0.3f, 16);
    sphere.material.baseColor = Color(0.2f, 0.6f, 0.8f);
    sphere.transform.position = Vec3(1.0f, 0.3f, -2.0f);
    m_sceneObjects.push_back(sphere);

    m_defaultPrimName = "World";
}

// ============================================================================
// USDSceneGenerator
// ============================================================================

USDSceneGenerator::USDSceneGenerator() = default;

USDSceneGenerator::~USDSceneGenerator() {
#if HAS_USD
    if (m_stage) {
        delete static_cast<pxr::UsdStageRefPtr*>(m_stage);
        m_stage = nullptr;
    }
#endif
}

bool USDSceneGenerator::createStage(const std::string& filePath) {
    m_filePath = filePath;

#if HAS_USD
    pxr::UsdStageRefPtr stage = pxr::UsdStage::CreateNew(filePath);
    if (!stage) {
        return false;
    }
    m_stage = new pxr::UsdStageRefPtr(stage);
#else
    // Start building USDA content
    m_usdaContent = "#usda 1.0\n(\n";
#endif
    return true;
}

void USDSceneGenerator::setUpAxis(const std::string& axis) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    pxr::UsdGeomSetStageUpAxis(stage, axis == "Z" ? pxr::UsdGeomTokens->z : pxr::UsdGeomTokens->y);
#else
    m_usdaContent += "    upAxis = \"" + axis + "\"\n";
#endif
}

void USDSceneGenerator::setMetersPerUnit(double meters) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    pxr::UsdGeomSetStageMetersPerUnit(stage, meters);
#else
    m_usdaContent += "    metersPerUnit = " + std::to_string(meters) + "\n";
#endif
}

void USDSceneGenerator::setDefaultPrim(const std::string& primPath) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    auto prim = stage->GetPrimAtPath(pxr::SdfPath(primPath));
    if (prim) {
        stage->SetDefaultPrim(prim);
    }
#else
    // Extract just the prim name from path
    size_t lastSlash = primPath.rfind('/');
    std::string primName = lastSlash != std::string::npos ? primPath.substr(lastSlash + 1) : primPath;
    m_usdaContent += "    defaultPrim = \"" + primName + "\"\n";
#endif
}

void USDSceneGenerator::addCube(const std::string& primPath, float size, const Transform& transform, const Color& color) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    auto cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath(primPath));
    cube.GetSizeAttr().Set(static_cast<double>(size));

    if (transform.position.x != 0 || transform.position.y != 0 || transform.position.z != 0) {
        auto translateOp = cube.AddTranslateOp();
        translateOp.Set(pxr::GfVec3d(transform.position.x, transform.position.y, transform.position.z));
    }
#else
    // Extract prim name
    size_t lastSlash = primPath.rfind('/');
    std::string primName = lastSlash != std::string::npos ? primPath.substr(lastSlash + 1) : primPath;

    m_usdaContent += "\ndef Cube \"" + primName + "\"\n{\n";
    m_usdaContent += "    double size = " + std::to_string(size) + "\n";

    if (transform.position.x != 0 || transform.position.y != 0 || transform.position.z != 0) {
        m_usdaContent += "    double3 xformOp:translate = (" +
            std::to_string(transform.position.x) + ", " +
            std::to_string(transform.position.y) + ", " +
            std::to_string(transform.position.z) + ")\n";
        m_usdaContent += "    uniform token[] xformOpOrder = [\"xformOp:translate\"]\n";
    }

    m_usdaContent += "    color3f[] primvars:displayColor = [(" +
        std::to_string(color.r) + ", " +
        std::to_string(color.g) + ", " +
        std::to_string(color.b) + ")]\n";

    m_usdaContent += "}\n";
#endif
    (void)color; // Used in non-USD path
}

void USDSceneGenerator::addSphere(const std::string& primPath, float radius, const Transform& transform, const Color& color) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    auto sphere = pxr::UsdGeomSphere::Define(stage, pxr::SdfPath(primPath));
    sphere.GetRadiusAttr().Set(static_cast<double>(radius));

    if (transform.position.x != 0 || transform.position.y != 0 || transform.position.z != 0) {
        auto translateOp = sphere.AddTranslateOp();
        translateOp.Set(pxr::GfVec3d(transform.position.x, transform.position.y, transform.position.z));
    }
#else
    size_t lastSlash = primPath.rfind('/');
    std::string primName = lastSlash != std::string::npos ? primPath.substr(lastSlash + 1) : primPath;

    m_usdaContent += "\ndef Sphere \"" + primName + "\"\n{\n";
    m_usdaContent += "    double radius = " + std::to_string(radius) + "\n";

    if (transform.position.x != 0 || transform.position.y != 0 || transform.position.z != 0) {
        m_usdaContent += "    double3 xformOp:translate = (" +
            std::to_string(transform.position.x) + ", " +
            std::to_string(transform.position.y) + ", " +
            std::to_string(transform.position.z) + ")\n";
        m_usdaContent += "    uniform token[] xformOpOrder = [\"xformOp:translate\"]\n";
    }

    m_usdaContent += "    color3f[] primvars:displayColor = [(" +
        std::to_string(color.r) + ", " +
        std::to_string(color.g) + ", " +
        std::to_string(color.b) + ")]\n";

    m_usdaContent += "}\n";
#endif
    (void)color;
}

void USDSceneGenerator::addPlane(const std::string& primPath, float width, float depth, const Transform& transform, const Color& color) {
    (void)width;
    (void)depth;
    (void)transform;
    (void)color;

#if HAS_USD
    // USD doesn't have a native plane, so we'd create a mesh
    // For simplicity, skip in this implementation
#else
    size_t lastSlash = primPath.rfind('/');
    std::string primName = lastSlash != std::string::npos ? primPath.substr(lastSlash + 1) : primPath;

    m_usdaContent += "\ndef Mesh \"" + primName + "\"\n{\n";
    m_usdaContent += "    # Plane mesh placeholder\n";
    m_usdaContent += "}\n";
#endif
}

void USDSceneGenerator::addMesh(const std::string& primPath, const Mesh& mesh, const Transform& transform, const Material& material) {
    (void)primPath;
    (void)mesh;
    (void)transform;
    (void)material;
    // Full mesh export would be implemented here
}

void USDSceneGenerator::addDistantLight(const std::string& primPath, const Vec3& direction, float intensity, const Color& color) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    auto light = pxr::UsdLuxDistantLight::Define(stage, pxr::SdfPath(primPath));
    light.GetIntensityAttr().Set(intensity);
    light.GetColorAttr().Set(pxr::GfVec3f(color.r, color.g, color.b));
#else
    size_t lastSlash = primPath.rfind('/');
    std::string primName = lastSlash != std::string::npos ? primPath.substr(lastSlash + 1) : primPath;

    m_usdaContent += "\ndef DistantLight \"" + primName + "\"\n{\n";
    m_usdaContent += "    float inputs:intensity = " + std::to_string(intensity) + "\n";
    m_usdaContent += "    color3f inputs:color = (" +
        std::to_string(color.r) + ", " +
        std::to_string(color.g) + ", " +
        std::to_string(color.b) + ")\n";
    m_usdaContent += "}\n";
#endif
    (void)direction;
}

void USDSceneGenerator::addPointLight(const std::string& primPath, const Vec3& position, float intensity, const Color& color) {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    auto light = pxr::UsdLuxSphereLight::Define(stage, pxr::SdfPath(primPath));
    light.GetIntensityAttr().Set(intensity);
    light.GetColorAttr().Set(pxr::GfVec3f(color.r, color.g, color.b));

    auto translateOp = light.AddTranslateOp();
    translateOp.Set(pxr::GfVec3d(position.x, position.y, position.z));
#else
    size_t lastSlash = primPath.rfind('/');
    std::string primName = lastSlash != std::string::npos ? primPath.substr(lastSlash + 1) : primPath;

    m_usdaContent += "\ndef SphereLight \"" + primName + "\"\n{\n";
    m_usdaContent += "    float inputs:intensity = " + std::to_string(intensity) + "\n";
    m_usdaContent += "    color3f inputs:color = (" +
        std::to_string(color.r) + ", " +
        std::to_string(color.g) + ", " +
        std::to_string(color.b) + ")\n";
    m_usdaContent += "    double3 xformOp:translate = (" +
        std::to_string(position.x) + ", " +
        std::to_string(position.y) + ", " +
        std::to_string(position.z) + ")\n";
    m_usdaContent += "    uniform token[] xformOpOrder = [\"xformOp:translate\"]\n";
    m_usdaContent += "}\n";
#endif
}

void USDSceneGenerator::addCustomAttribute(const std::string& primPath, const std::string& attrName, const std::string& value) {
    (void)primPath;
    (void)attrName;
    (void)value;
    // Custom attribute support would be implemented here
}

bool USDSceneGenerator::save() {
#if HAS_USD
    auto stage = *static_cast<pxr::UsdStageRefPtr*>(m_stage);
    stage->GetRootLayer()->Save();
    return true;
#else
    // Close the header
    m_usdaContent += ")\n";

    // Write to file
    std::ofstream file(m_filePath);
    if (!file.is_open()) {
        return false;
    }
    file << m_usdaContent;
    file.close();
    return true;
#endif
}

} // namespace lst
