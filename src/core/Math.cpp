#include "Types.h"
#include <cmath>
#include <algorithm>

namespace lst {

// Vec3 implementation
float Vec3::length() const {
    return std::sqrt(x * x + y * y + z * z);
}

Vec3 Vec3::normalized() const {
    float len = length();
    if (len > 0.0001f) {
        return Vec3(x / len, y / len, z / len);
    }
    return Vec3(0, 0, 0);
}

float Vec3::dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Vec3::cross(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

// Quat implementation
Quat Quat::operator*(const Quat& q) const {
    return Quat(
        w * q.x + x * q.w + y * q.z - z * q.y,
        w * q.y - x * q.z + y * q.w + z * q.x,
        w * q.z + x * q.y - y * q.x + z * q.w,
        w * q.w - x * q.x - y * q.y - z * q.z
    );
}

Vec3 Quat::rotate(const Vec3& v) const {
    // q * v * q^-1
    Quat qv(v.x, v.y, v.z, 0);
    Quat result = (*this) * qv * conjugate();
    return Vec3(result.x, result.y, result.z);
}

Quat Quat::conjugate() const {
    return Quat(-x, -y, -z, w);
}

Quat Quat::normalized() const {
    float len = std::sqrt(x * x + y * y + z * z + w * w);
    if (len > 0.0001f) {
        return Quat(x / len, y / len, z / len, w / len);
    }
    return Quat::identity();
}

Quat Quat::fromAxisAngle(const Vec3& axis, float angle) {
    Vec3 n = axis.normalized();
    float halfAngle = angle * 0.5f;
    float s = std::sin(halfAngle);
    return Quat(n.x * s, n.y * s, n.z * s, std::cos(halfAngle));
}

float Quat::dot(const Quat& other) const {
    return x * other.x + y * other.y + z * other.z + w * other.w;
}

Quat Quat::slerp(const Quat& a, const Quat& b, float t) {
    // Compute dot product (cosine of angle between quaternions)
    float cosTheta = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    // Use shorter path - if dot is negative, negate one quaternion
    Quat bAdjusted = b;
    if (cosTheta < 0.0f) {
        bAdjusted = Quat(-b.x, -b.y, -b.z, -b.w);
        cosTheta = -cosTheta;
    }

    // If quaternions are very close, use linear interpolation to avoid division by zero
    if (cosTheta > 0.9995f) {
        Quat result(
            a.x + t * (bAdjusted.x - a.x),
            a.y + t * (bAdjusted.y - a.y),
            a.z + t * (bAdjusted.z - a.z),
            a.w + t * (bAdjusted.w - a.w)
        );
        return result.normalized();
    }

    // Standard slerp formula
    float theta = std::acos(cosTheta);
    float sinTheta = std::sin(theta);
    float wa = std::sin((1.0f - t) * theta) / sinTheta;
    float wb = std::sin(t * theta) / sinTheta;

    return Quat(
        wa * a.x + wb * bAdjusted.x,
        wa * a.y + wb * bAdjusted.y,
        wa * a.z + wb * bAdjusted.z,
        wa * a.w + wb * bAdjusted.w
    );
}

// Transform implementation
std::array<float, 16> Transform::toMatrix() const {
    std::array<float, 16> m;

    float xx = orientation.x * orientation.x;
    float yy = orientation.y * orientation.y;
    float zz = orientation.z * orientation.z;
    float xy = orientation.x * orientation.y;
    float xz = orientation.x * orientation.z;
    float yz = orientation.y * orientation.z;
    float wx = orientation.w * orientation.x;
    float wy = orientation.w * orientation.y;
    float wz = orientation.w * orientation.z;

    // Column-major order
    m[0] = (1.0f - 2.0f * (yy + zz)) * scale.x;
    m[1] = (2.0f * (xy + wz)) * scale.x;
    m[2] = (2.0f * (xz - wy)) * scale.x;
    m[3] = 0.0f;

    m[4] = (2.0f * (xy - wz)) * scale.y;
    m[5] = (1.0f - 2.0f * (xx + zz)) * scale.y;
    m[6] = (2.0f * (yz + wx)) * scale.y;
    m[7] = 0.0f;

    m[8] = (2.0f * (xz + wy)) * scale.z;
    m[9] = (2.0f * (yz - wx)) * scale.z;
    m[10] = (1.0f - 2.0f * (xx + yy)) * scale.z;
    m[11] = 0.0f;

    m[12] = position.x;
    m[13] = position.y;
    m[14] = position.z;
    m[15] = 1.0f;

    return m;
}

Vec3 Transform::transformPoint(const Vec3& point) const {
    Vec3 scaled(point.x * scale.x, point.y * scale.y, point.z * scale.z);
    return orientation.rotate(scaled) + position;
}

Vec3 Transform::transformDirection(const Vec3& dir) const {
    return orientation.rotate(dir);
}

// Mat4 implementation
Mat4::Mat4() {
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

Mat4::Mat4(const float* data) {
    for (int i = 0; i < 16; i++) m[i] = data[i];
}

Mat4 Mat4::identity() {
    Mat4 result;
    return result;
}

Mat4 Mat4::perspective(float fovY, float aspect, float nearZ, float farZ) {
    Mat4 result;
    float tanHalfFov = std::tan(fovY * 0.5f);

    result.m[0] = 1.0f / (aspect * tanHalfFov);
    result.m[5] = 1.0f / tanHalfFov;
    result.m[10] = -(farZ + nearZ) / (farZ - nearZ);
    result.m[11] = -1.0f;
    result.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    result.m[15] = 0.0f;

    return result;
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = (center - eye).normalized();
    Vec3 s = Vec3::cross(f, up).normalized();
    Vec3 u = Vec3::cross(s, f);

    Mat4 result;
    result.m[0] = s.x;
    result.m[4] = s.y;
    result.m[8] = s.z;
    result.m[1] = u.x;
    result.m[5] = u.y;
    result.m[9] = u.z;
    result.m[2] = -f.x;
    result.m[6] = -f.y;
    result.m[10] = -f.z;
    result.m[12] = -Vec3::dot(s, eye);
    result.m[13] = -Vec3::dot(u, eye);
    result.m[14] = Vec3::dot(f, eye);

    return result;
}

Mat4 Mat4::translation(const Vec3& t) {
    Mat4 result;
    result.m[12] = t.x;
    result.m[13] = t.y;
    result.m[14] = t.z;
    return result;
}

Mat4 Mat4::rotation(const Quat& q) {
    Mat4 result;

    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

    result.m[0] = 1.0f - 2.0f * (yy + zz);
    result.m[1] = 2.0f * (xy + wz);
    result.m[2] = 2.0f * (xz - wy);

    result.m[4] = 2.0f * (xy - wz);
    result.m[5] = 1.0f - 2.0f * (xx + zz);
    result.m[6] = 2.0f * (yz + wx);

    result.m[8] = 2.0f * (xz + wy);
    result.m[9] = 2.0f * (yz - wx);
    result.m[10] = 1.0f - 2.0f * (xx + yy);

    return result;
}

Mat4 Mat4::scale(const Vec3& s) {
    Mat4 result;
    result.m[0] = s.x;
    result.m[5] = s.y;
    result.m[10] = s.z;
    return result;
}

Mat4 Mat4::operator*(const Mat4& other) const {
    Mat4 result;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            result.m[col * 4 + row] =
                m[0 * 4 + row] * other.m[col * 4 + 0] +
                m[1 * 4 + row] * other.m[col * 4 + 1] +
                m[2 * 4 + row] * other.m[col * 4 + 2] +
                m[3 * 4 + row] * other.m[col * 4 + 3];
        }
    }
    return result;
}

Vec3 Mat4::transformPoint(const Vec3& p) const {
    float x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
    float y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
    float z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
    float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
    if (std::abs(w) > 0.0001f) {
        return Vec3(x / w, y / w, z / w);
    }
    return Vec3(x, y, z);
}

// Mesh generators
Mesh Mesh::createCube(float size) {
    Mesh mesh;
    mesh.name = "Cube";

    float h = size * 0.5f;

    // Front face
    mesh.vertices.push_back({{-h, -h, h}, {0, 0, 1}, 0, 0, Color::white()});
    mesh.vertices.push_back({{h, -h, h}, {0, 0, 1}, 1, 0, Color::white()});
    mesh.vertices.push_back({{h, h, h}, {0, 0, 1}, 1, 1, Color::white()});
    mesh.vertices.push_back({{-h, h, h}, {0, 0, 1}, 0, 1, Color::white()});

    // Back face
    mesh.vertices.push_back({{h, -h, -h}, {0, 0, -1}, 0, 0, Color::white()});
    mesh.vertices.push_back({{-h, -h, -h}, {0, 0, -1}, 1, 0, Color::white()});
    mesh.vertices.push_back({{-h, h, -h}, {0, 0, -1}, 1, 1, Color::white()});
    mesh.vertices.push_back({{h, h, -h}, {0, 0, -1}, 0, 1, Color::white()});

    // Right face
    mesh.vertices.push_back({{h, -h, h}, {1, 0, 0}, 0, 0, Color::white()});
    mesh.vertices.push_back({{h, -h, -h}, {1, 0, 0}, 1, 0, Color::white()});
    mesh.vertices.push_back({{h, h, -h}, {1, 0, 0}, 1, 1, Color::white()});
    mesh.vertices.push_back({{h, h, h}, {1, 0, 0}, 0, 1, Color::white()});

    // Left face
    mesh.vertices.push_back({{-h, -h, -h}, {-1, 0, 0}, 0, 0, Color::white()});
    mesh.vertices.push_back({{-h, -h, h}, {-1, 0, 0}, 1, 0, Color::white()});
    mesh.vertices.push_back({{-h, h, h}, {-1, 0, 0}, 1, 1, Color::white()});
    mesh.vertices.push_back({{-h, h, -h}, {-1, 0, 0}, 0, 1, Color::white()});

    // Top face
    mesh.vertices.push_back({{-h, h, h}, {0, 1, 0}, 0, 0, Color::white()});
    mesh.vertices.push_back({{h, h, h}, {0, 1, 0}, 1, 0, Color::white()});
    mesh.vertices.push_back({{h, h, -h}, {0, 1, 0}, 1, 1, Color::white()});
    mesh.vertices.push_back({{-h, h, -h}, {0, 1, 0}, 0, 1, Color::white()});

    // Bottom face
    mesh.vertices.push_back({{-h, -h, -h}, {0, -1, 0}, 0, 0, Color::white()});
    mesh.vertices.push_back({{h, -h, -h}, {0, -1, 0}, 1, 0, Color::white()});
    mesh.vertices.push_back({{h, -h, h}, {0, -1, 0}, 1, 1, Color::white()});
    mesh.vertices.push_back({{-h, -h, h}, {0, -1, 0}, 0, 1, Color::white()});

    // Indices (two triangles per face)
    for (int face = 0; face < 6; face++) {
        uint32_t base = face * 4;
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }

    return mesh;
}

Mesh Mesh::createSphere(float radius, int segments) {
    Mesh mesh;
    mesh.name = "Sphere";

    for (int lat = 0; lat <= segments; lat++) {
        float theta = lat * 3.14159265359f / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2.0f * 3.14159265359f / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            Vec3 normal(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
            Vec3 pos = normal * radius;
            float u = static_cast<float>(lon) / segments;
            float v = static_cast<float>(lat) / segments;

            mesh.vertices.push_back({pos, normal, u, v, Color::white()});
        }
    }

    for (int lat = 0; lat < segments; lat++) {
        for (int lon = 0; lon < segments; lon++) {
            uint32_t first = lat * (segments + 1) + lon;
            uint32_t second = first + segments + 1;

            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);

            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }

    return mesh;
}

Mesh Mesh::createCapsule(float radius, float height, int segments) {
    Mesh mesh;
    mesh.name = "Capsule";

    float halfHeight = height * 0.5f;
    int halfSegments = segments / 2;

    // Top hemisphere
    for (int lat = 0; lat <= halfSegments; lat++) {
        float theta = lat * 3.14159265359f / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2.0f * 3.14159265359f / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            Vec3 normal(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
            Vec3 pos(normal.x * radius, halfHeight + normal.y * radius, normal.z * radius);
            float u = static_cast<float>(lon) / segments;
            float v = static_cast<float>(lat) / segments * 0.25f;

            mesh.vertices.push_back({pos, normal, u, v, Color::white()});
        }
    }

    // Cylinder
    for (int ring = 0; ring <= 1; ring++) {
        float y = halfHeight - ring * height;
        for (int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2.0f * 3.14159265359f / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            Vec3 normal(cosPhi, 0, sinPhi);
            Vec3 pos(normal.x * radius, y, normal.z * radius);
            float u = static_cast<float>(lon) / segments;
            float v = 0.25f + ring * 0.5f;

            mesh.vertices.push_back({pos, normal, u, v, Color::white()});
        }
    }

    // Bottom hemisphere
    for (int lat = halfSegments; lat <= segments; lat++) {
        float theta = lat * 3.14159265359f / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2.0f * 3.14159265359f / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            Vec3 normal(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
            Vec3 pos(normal.x * radius, -halfHeight + normal.y * radius, normal.z * radius);
            float u = static_cast<float>(lon) / segments;
            float v = 0.75f + static_cast<float>(lat - halfSegments) / segments * 0.25f;

            mesh.vertices.push_back({pos, normal, u, v, Color::white()});
        }
    }

    // Generate indices
    int vertsPerRing = segments + 1;
    int numRings = (halfSegments + 1) + 2 + (halfSegments + 1);

    for (int ring = 0; ring < numRings - 1; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            uint32_t curr = ring * vertsPerRing + seg;
            uint32_t next = curr + vertsPerRing;

            mesh.indices.push_back(curr);
            mesh.indices.push_back(next);
            mesh.indices.push_back(curr + 1);

            mesh.indices.push_back(next);
            mesh.indices.push_back(next + 1);
            mesh.indices.push_back(curr + 1);
        }
    }

    return mesh;
}

Mesh Mesh::createPlane(float width, float depth) {
    Mesh mesh;
    mesh.name = "Plane";

    float hw = width * 0.5f;
    float hd = depth * 0.5f;

    mesh.vertices.push_back({{-hw, 0, -hd}, {0, 1, 0}, 0, 0, Color::white()});
    mesh.vertices.push_back({{hw, 0, -hd}, {0, 1, 0}, 1, 0, Color::white()});
    mesh.vertices.push_back({{hw, 0, hd}, {0, 1, 0}, 1, 1, Color::white()});
    mesh.vertices.push_back({{-hw, 0, hd}, {0, 1, 0}, 0, 1, Color::white()});

    mesh.indices = {0, 2, 1, 0, 3, 2};

    return mesh;
}

Mesh Mesh::createCylinder(float radius, float height, int segments) {
    Mesh mesh;
    mesh.name = "Cylinder";

    float halfHeight = height * 0.5f;

    // Top cap center
    mesh.vertices.push_back({{0, halfHeight, 0}, {0, 1, 0}, 0.5f, 0.5f, Color::white()});

    // Top cap ring
    for (int i = 0; i <= segments; i++) {
        float phi = i * 2.0f * 3.14159265359f / segments;
        float x = std::cos(phi) * radius;
        float z = std::sin(phi) * radius;
        mesh.vertices.push_back({{x, halfHeight, z}, {0, 1, 0}, 0.5f + x / (2 * radius), 0.5f + z / (2 * radius), Color::white()});
    }

    // Side vertices
    for (int ring = 0; ring <= 1; ring++) {
        float y = halfHeight - ring * height;
        for (int i = 0; i <= segments; i++) {
            float phi = i * 2.0f * 3.14159265359f / segments;
            float x = std::cos(phi);
            float z = std::sin(phi);
            mesh.vertices.push_back({{x * radius, y, z * radius}, {x, 0, z}, static_cast<float>(i) / segments, static_cast<float>(ring), Color::white()});
        }
    }

    // Bottom cap center
    int bottomCenterIdx = static_cast<int>(mesh.vertices.size());
    mesh.vertices.push_back({{0, -halfHeight, 0}, {0, -1, 0}, 0.5f, 0.5f, Color::white()});

    // Bottom cap ring
    for (int i = 0; i <= segments; i++) {
        float phi = i * 2.0f * 3.14159265359f / segments;
        float x = std::cos(phi) * radius;
        float z = std::sin(phi) * radius;
        mesh.vertices.push_back({{x, -halfHeight, z}, {0, -1, 0}, 0.5f + x / (2 * radius), 0.5f + z / (2 * radius), Color::white()});
    }

    // Top cap indices
    for (int i = 0; i < segments; i++) {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i + 1);
        mesh.indices.push_back(i + 2);
    }

    // Side indices
    int sideStartIdx = segments + 2;
    for (int i = 0; i < segments; i++) {
        uint32_t curr = sideStartIdx + i;
        uint32_t next = curr + segments + 1;

        mesh.indices.push_back(curr);
        mesh.indices.push_back(next);
        mesh.indices.push_back(curr + 1);

        mesh.indices.push_back(next);
        mesh.indices.push_back(next + 1);
        mesh.indices.push_back(curr + 1);
    }

    // Bottom cap indices
    for (int i = 0; i < segments; i++) {
        mesh.indices.push_back(bottomCenterIdx);
        mesh.indices.push_back(bottomCenterIdx + i + 2);
        mesh.indices.push_back(bottomCenterIdx + i + 1);
    }

    return mesh;
}

} // namespace lst
