/**
 * Math unit tests
 */

#include "Types.h"
#include <iostream>
#include <cmath>
#include <cassert>

#if USE_GTEST
#include <gtest/gtest.h>
#endif

using namespace lst;

namespace {

const float EPSILON = 0.0001f;

bool approxEqual(float a, float b) {
    return std::abs(a - b) < EPSILON;
}

bool approxEqual(const Vec3& a, const Vec3& b) {
    return approxEqual(a.x, b.x) &&
           approxEqual(a.y, b.y) &&
           approxEqual(a.z, b.z);
}

// =============================================================================
// Vec3 Tests
// =============================================================================

void testVec3Construction() {
    Vec3 v1;
    assert(v1.x == 0 && v1.y == 0 && v1.z == 0);

    Vec3 v2(1, 2, 3);
    assert(v2.x == 1 && v2.y == 2 && v2.z == 3);

    std::cout << "  Vec3 construction: PASS" << std::endl;
}

void testVec3Operations() {
    Vec3 a(1, 2, 3);
    Vec3 b(4, 5, 6);

    Vec3 sum = a + b;
    assert(approxEqual(sum, Vec3(5, 7, 9)));

    Vec3 diff = b - a;
    assert(approxEqual(diff, Vec3(3, 3, 3)));

    Vec3 scaled = a * 2.0f;
    assert(approxEqual(scaled, Vec3(2, 4, 6)));

    std::cout << "  Vec3 operations: PASS" << std::endl;
}

void testVec3Length() {
    Vec3 v(3, 4, 0);
    assert(approxEqual(v.length(), 5.0f));

    Vec3 v2(1, 0, 0);
    assert(approxEqual(v2.length(), 1.0f));

    std::cout << "  Vec3 length: PASS" << std::endl;
}

void testVec3Normalize() {
    Vec3 v(3, 4, 0);
    Vec3 normalized = v.normalized();
    assert(approxEqual(normalized.length(), 1.0f));
    assert(approxEqual(normalized.x, 0.6f));
    assert(approxEqual(normalized.y, 0.8f));

    std::cout << "  Vec3 normalize: PASS" << std::endl;
}

void testVec3Dot() {
    Vec3 a(1, 0, 0);
    Vec3 b(0, 1, 0);
    assert(approxEqual(Vec3::dot(a, b), 0.0f));

    Vec3 c(1, 2, 3);
    Vec3 d(4, 5, 6);
    assert(approxEqual(Vec3::dot(c, d), 32.0f)); // 1*4 + 2*5 + 3*6

    std::cout << "  Vec3 dot product: PASS" << std::endl;
}

void testVec3Cross() {
    Vec3 x(1, 0, 0);
    Vec3 y(0, 1, 0);
    Vec3 z = Vec3::cross(x, y);
    assert(approxEqual(z, Vec3(0, 0, 1)));

    std::cout << "  Vec3 cross product: PASS" << std::endl;
}

// =============================================================================
// Quat Tests
// =============================================================================

void testQuatConstruction() {
    Quat q;
    assert(q.x == 0 && q.y == 0 && q.z == 0 && q.w == 1);

    Quat q2 = Quat::identity();
    assert(q2.w == 1);

    std::cout << "  Quat construction: PASS" << std::endl;
}

void testQuatFromAxisAngle() {
    // 90 degree rotation around Y axis
    Quat q = Quat::fromAxisAngle(Vec3(0, 1, 0), 3.14159265359f / 2.0f);
    assert(approxEqual(q.w, 0.7071f));
    assert(approxEqual(q.y, 0.7071f));

    std::cout << "  Quat from axis-angle: PASS" << std::endl;
}

void testQuatRotate() {
    // 90 degree rotation around Y axis should transform X to -Z
    Quat q = Quat::fromAxisAngle(Vec3(0, 1, 0), 3.14159265359f / 2.0f);
    Vec3 v(1, 0, 0);
    Vec3 rotated = q.rotate(v);

    assert(approxEqual(rotated.x, 0.0f));
    assert(approxEqual(rotated.y, 0.0f));
    assert(approxEqual(rotated.z, -1.0f));

    std::cout << "  Quat rotation: PASS" << std::endl;
}

void testQuatMultiplication() {
    // Two 90 degree rotations should equal 180 degrees
    Quat q1 = Quat::fromAxisAngle(Vec3(0, 1, 0), 3.14159265359f / 2.0f);
    Quat q2 = Quat::fromAxisAngle(Vec3(0, 1, 0), 3.14159265359f / 2.0f);
    Quat combined = q1 * q2;

    Vec3 v(1, 0, 0);
    Vec3 rotated = combined.rotate(v);

    assert(approxEqual(rotated.x, -1.0f));
    assert(approxEqual(rotated.y, 0.0f));
    assert(approxEqual(rotated.z, 0.0f));

    std::cout << "  Quat multiplication: PASS" << std::endl;
}

// =============================================================================
// Transform Tests
// =============================================================================

void testTransformPoint() {
    Transform t;
    t.position = Vec3(1, 2, 3);
    t.orientation = Quat::identity();
    t.scale = Vec3(1, 1, 1);

    Vec3 point(0, 0, 0);
    Vec3 transformed = t.transformPoint(point);
    assert(approxEqual(transformed, Vec3(1, 2, 3)));

    std::cout << "  Transform point: PASS" << std::endl;
}

void testTransformWithScale() {
    Transform t;
    t.position = Vec3(0, 0, 0);
    t.orientation = Quat::identity();
    t.scale = Vec3(2, 2, 2);

    Vec3 point(1, 1, 1);
    Vec3 transformed = t.transformPoint(point);
    assert(approxEqual(transformed, Vec3(2, 2, 2)));

    std::cout << "  Transform with scale: PASS" << std::endl;
}

// =============================================================================
// Mat4 Tests
// =============================================================================

void testMat4Identity() {
    Mat4 m = Mat4::identity();
    assert(m.m[0] == 1 && m.m[5] == 1 && m.m[10] == 1 && m.m[15] == 1);

    std::cout << "  Mat4 identity: PASS" << std::endl;
}

void testMat4Multiplication() {
    Mat4 a = Mat4::identity();
    Mat4 b = Mat4::identity();
    Mat4 c = a * b;

    assert(c.m[0] == 1 && c.m[5] == 1 && c.m[10] == 1 && c.m[15] == 1);

    std::cout << "  Mat4 multiplication: PASS" << std::endl;
}

void testMat4Translation() {
    Mat4 m = Mat4::translation(Vec3(1, 2, 3));
    Vec3 point(0, 0, 0);
    Vec3 transformed = m.transformPoint(point);
    assert(approxEqual(transformed, Vec3(1, 2, 3)));

    std::cout << "  Mat4 translation: PASS" << std::endl;
}

// =============================================================================
// Mesh Tests
// =============================================================================

void testMeshCreateCube() {
    Mesh cube = Mesh::createCube(1.0f);

    // A cube has 6 faces * 4 vertices = 24 vertices
    assert(cube.vertices.size() == 24);
    // 6 faces * 2 triangles * 3 indices = 36 indices
    assert(cube.indices.size() == 36);

    std::cout << "  Mesh cube creation: PASS" << std::endl;
}

void testMeshCreateSphere() {
    Mesh sphere = Mesh::createSphere(1.0f, 16);

    // Should have vertices
    assert(sphere.vertices.size() > 0);
    // Should have indices
    assert(sphere.indices.size() > 0);

    std::cout << "  Mesh sphere creation: PASS" << std::endl;
}

void testMeshCreatePlane() {
    Mesh plane = Mesh::createPlane(10.0f, 10.0f);

    assert(plane.vertices.size() == 4);
    assert(plane.indices.size() == 6);

    std::cout << "  Mesh plane creation: PASS" << std::endl;
}

} // anonymous namespace

// =============================================================================
// Test Runner
// =============================================================================

int runMathTests() {
    std::cout << "Running Math Tests..." << std::endl;

    testVec3Construction();
    testVec3Operations();
    testVec3Length();
    testVec3Normalize();
    testVec3Dot();
    testVec3Cross();

    testQuatConstruction();
    testQuatFromAxisAngle();
    testQuatRotate();
    testQuatMultiplication();

    testTransformPoint();
    testTransformWithScale();

    testMat4Identity();
    testMat4Multiplication();
    testMat4Translation();

    testMeshCreateCube();
    testMeshCreateSphere();
    testMeshCreatePlane();

    std::cout << "All Math Tests PASSED!" << std::endl;
    return 0;
}

#if USE_GTEST
TEST(MathTest, Vec3Construction) { testVec3Construction(); }
TEST(MathTest, Vec3Operations) { testVec3Operations(); }
TEST(MathTest, Vec3Length) { testVec3Length(); }
TEST(MathTest, Vec3Normalize) { testVec3Normalize(); }
TEST(MathTest, Vec3Dot) { testVec3Dot(); }
TEST(MathTest, Vec3Cross) { testVec3Cross(); }
TEST(MathTest, QuatConstruction) { testQuatConstruction(); }
TEST(MathTest, QuatFromAxisAngle) { testQuatFromAxisAngle(); }
TEST(MathTest, QuatRotate) { testQuatRotate(); }
TEST(MathTest, QuatMultiplication) { testQuatMultiplication(); }
TEST(MathTest, TransformPoint) { testTransformPoint(); }
TEST(MathTest, TransformWithScale) { testTransformWithScale(); }
TEST(MathTest, Mat4Identity) { testMat4Identity(); }
TEST(MathTest, Mat4Multiplication) { testMat4Multiplication(); }
TEST(MathTest, Mat4Translation) { testMat4Translation(); }
TEST(MathTest, MeshCreateCube) { testMeshCreateCube(); }
TEST(MathTest, MeshCreateSphere) { testMeshCreateSphere(); }
TEST(MathTest, MeshCreatePlane) { testMeshCreatePlane(); }
#endif
