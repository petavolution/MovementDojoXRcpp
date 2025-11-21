#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <array>

// Forward declarations for OpenXR types
struct XrInstance_T;
struct XrSession_T;
struct XrSpace_T;
struct XrAction_T;
struct XrActionSet_T;
struct XrSwapchain_T;

typedef struct XrInstance_T* XrInstance;
typedef struct XrSession_T* XrSession;
typedef struct XrSpace_T* XrSpace;
typedef struct XrAction_T* XrAction;
typedef struct XrActionSet_T* XrActionSet;
typedef struct XrSwapchain_T* XrSwapchain;
typedef uint64_t XrSystemId;
typedef uint64_t XrPath;

namespace lst {

// Basic math types
struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }

    float length() const;
    Vec3 normalized() const;
    static float dot(const Vec3& a, const Vec3& b);
    static Vec3 cross(const Vec3& a, const Vec3& b);
};

struct Quat {
    float x, y, z, w;

    Quat() : x(0), y(0), z(0), w(1) {}
    Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    Quat operator*(const Quat& other) const;
    Vec3 rotate(const Vec3& v) const;
    Quat conjugate() const;
    Quat normalized() const;

    static Quat fromAxisAngle(const Vec3& axis, float angle);
    static Quat identity() { return Quat(0, 0, 0, 1); }
};

struct Transform {
    Vec3 position;
    Quat orientation;
    Vec3 scale;

    Transform() : position(), orientation(Quat::identity()), scale(1, 1, 1) {}
    Transform(const Vec3& pos, const Quat& rot) : position(pos), orientation(rot), scale(1, 1, 1) {}

    std::array<float, 16> toMatrix() const;
    Vec3 transformPoint(const Vec3& point) const;
    Vec3 transformDirection(const Vec3& dir) const;
};

struct Mat4 {
    float m[16];

    Mat4();
    Mat4(const float* data);

    static Mat4 identity();
    static Mat4 perspective(float fovY, float aspect, float nearZ, float farZ);
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
    static Mat4 translation(const Vec3& t);
    static Mat4 rotation(const Quat& q);
    static Mat4 scale(const Vec3& s);

    Mat4 operator*(const Mat4& other) const;
    Vec3 transformPoint(const Vec3& p) const;
};

// Color
struct Color {
    float r, g, b, a;

    Color() : r(1), g(1), b(1), a(1) {}
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    static Color white() { return Color(1, 1, 1, 1); }
    static Color black() { return Color(0, 0, 0, 1); }
    static Color red() { return Color(1, 0, 0, 1); }
    static Color green() { return Color(0, 1, 0, 1); }
    static Color blue() { return Color(0, 0, 1, 1); }
    static Color cyan() { return Color(0, 1, 1, 1); }
};

// Vertex data
struct Vertex {
    Vec3 position;
    Vec3 normal;
    float u, v;
    Color color;
};

// Mesh data
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string name;

    static Mesh createCube(float size = 1.0f);
    static Mesh createSphere(float radius = 1.0f, int segments = 16);
    static Mesh createCapsule(float radius = 0.05f, float height = 1.0f, int segments = 16);
    static Mesh createPlane(float width = 10.0f, float depth = 10.0f);
    static Mesh createCylinder(float radius = 0.02f, float height = 1.0f, int segments = 16);
};

// Material
struct Material {
    Color baseColor;
    float metallic;
    float roughness;
    float emissive;
    std::string name;

    Material() : baseColor(Color::white()), metallic(0), roughness(0.5f), emissive(0) {}
};

// Scene object
struct SceneObject {
    std::string name;
    Transform transform;
    Mesh mesh;
    Material material;
    bool visible;
    bool isStatic;

    SceneObject() : visible(true), isStatic(true) {}
};

// XR View (per eye)
struct XRView {
    Transform pose;
    float fov[4]; // left, right, up, down angles
    int width;
    int height;
};

// Controller hand
enum class Hand {
    Left = 0,
    Right = 1
};

// Controller state
struct ControllerState {
    Hand hand;
    Transform pose;
    bool isTracked;
    bool triggerPressed;
    float triggerValue;
    bool gripPressed;
    float gripValue;
    bool primaryButtonPressed;
    bool secondaryButtonPressed;
    Vec3 velocity;
    Vec3 angularVelocity;
};

// Haptic event
struct HapticEvent {
    Hand hand;
    float intensity;
    float duration;
    float frequency;
};

// Application configuration
struct AppConfig {
    std::string appName;
    uint32_t appVersion;
    std::string scenePath;
    bool validateOnly;
    bool enableValidation;
    int targetFPS;

    AppConfig()
        : appName("Lightsaber Trainer")
        , appVersion(1)
        , validateOnly(false)
        , enableValidation(true)
        , targetFPS(90) {}
};

// Application state
enum class AppState {
    Initializing,
    Running,
    Paused,
    Exiting,
    Error
};

// Callback types
using FrameCallback = std::function<void(double deltaTime)>;
using InputCallback = std::function<void(const ControllerState& left, const ControllerState& right)>;
using CollisionCallback = std::function<void(const std::string& objectA, const std::string& objectB, const Vec3& point)>;

} // namespace lst
