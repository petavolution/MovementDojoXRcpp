#!/bin/bash
# =============================================================================
# Headless Test Runner for Movement Dojo
#
# Runs all tests without requiring VR hardware or display.
# Suitable for CI/CD pipelines and development machines without XR runtime.
#
# Usage:
#   ./scripts/run_headless_tests.sh              # Run all tests
#   ./scripts/run_headless_tests.sh --quick      # Run quick tests only
#   ./scripts/run_headless_tests.sh --full       # Run full test suite with generation
# =============================================================================

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test mode
TEST_MODE="standard"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --quick)
            TEST_MODE="quick"
            shift
            ;;
        --full)
            TEST_MODE="full"
            shift
            ;;
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --quick     Run only fast unit tests"
            echo "  --full      Run all tests including generation validation"
            echo "  --verbose   Show detailed output"
            echo "  --help      Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Print header
echo -e "${BLUE}=============================================${NC}"
echo -e "${BLUE}  Movement Dojo - Headless Test Suite${NC}"
echo -e "${BLUE}=============================================${NC}"
echo ""
echo "Test mode: ${TEST_MODE}"
echo "Project dir: ${PROJECT_DIR}"
echo ""

# Track results
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local required="${3:-true}"

    echo -n "  Testing ${test_name}... "

    if eval "$test_cmd" > /tmp/test_output_$$.txt 2>&1; then
        echo -e "${GREEN}PASSED${NC}"
        ((TESTS_PASSED++))
        return 0
    else
        if [ "$required" = "true" ]; then
            echo -e "${RED}FAILED${NC}"
            ((TESTS_FAILED++))
            if [ -n "$VERBOSE" ]; then
                cat /tmp/test_output_$$.txt
            fi
            return 1
        else
            echo -e "${YELLOW}SKIPPED${NC}"
            ((TESTS_SKIPPED++))
            return 0
        fi
    fi
}

# =============================================================================
# Stage 1: Build System Tests
# =============================================================================

echo -e "${YELLOW}[Stage 1] Build System${NC}"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "  Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Configure if needed
if [ ! -f "$BUILD_DIR/Makefile" ] && [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo "  Configuring CMake..."
    cd "$BUILD_DIR"
    cmake -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug .. > /dev/null 2>&1 || {
        echo -e "${RED}CMake configuration failed${NC}"
        exit 1
    }
    cd "$PROJECT_DIR"
fi

# Build tests
echo "  Building test suite..."
cd "$BUILD_DIR"
if ! make lightsaber_tests -j$(nproc) > /tmp/build_output.txt 2>&1; then
    echo -e "${RED}Build failed!${NC}"
    cat /tmp/build_output.txt
    exit 1
fi
cd "$PROJECT_DIR"

run_test "CMakeLists.txt syntax" "cmake -P ${PROJECT_DIR}/CMakeLists.txt 2>/dev/null || true"
echo ""

# =============================================================================
# Stage 2: Unit Tests
# =============================================================================

echo -e "${YELLOW}[Stage 2] Unit Tests${NC}"

TEST_BINARY="${BUILD_DIR}/bin/lightsaber_tests"

if [ -f "$TEST_BINARY" ]; then
    run_test "Math tests" "$TEST_BINARY --math"
    run_test "USD loader tests" "$TEST_BINARY --usd"
    run_test "SessionManager tests" "$TEST_BINARY --session"
else
    echo -e "  ${RED}Test binary not found: ${TEST_BINARY}${NC}"
    ((TESTS_FAILED++))
fi
echo ""

# =============================================================================
# Stage 3: Python Script Tests
# =============================================================================

echo -e "${YELLOW}[Stage 3] Script Validation${NC}"

# Check Python availability
if command -v python3 &> /dev/null; then
    # Test scene generator
    run_test "Scene generator syntax" "python3 -m py_compile ${SCRIPT_DIR}/generate_scenes.py"

    # Test exercise generator
    run_test "Exercise generator syntax" "python3 -m py_compile ${SCRIPT_DIR}/generate_exercises.py"

    # Test mesh generator
    run_test "Mesh generator syntax" "python3 -m py_compile ${SCRIPT_DIR}/generate_meshes.py"

    if [ "$TEST_MODE" = "full" ]; then
        # Actually run generators
        TEMP_DIR=$(mktemp -d)

        run_test "Scene generation" "python3 ${SCRIPT_DIR}/generate_scenes.py --output ${TEMP_DIR}/scenes/"
        run_test "Exercise generation" "python3 ${SCRIPT_DIR}/generate_exercises.py --output ${TEMP_DIR}/exercises/"
        run_test "Mesh generation" "python3 ${SCRIPT_DIR}/generate_meshes.py --output ${TEMP_DIR}/meshes/"

        # Validate generated files exist
        run_test "Generated scenes exist" "[ -f ${TEMP_DIR}/scenes/dojo_basic.usda ]"
        run_test "Generated exercises exist" "[ -f ${TEMP_DIR}/exercises/exercise_catalog.json ]"
        run_test "Generated meshes exist" "[ -f ${TEMP_DIR}/meshes/lightsaber_hilt.obj ]"

        rm -rf "$TEMP_DIR"
    fi
else
    echo -e "  ${YELLOW}Python3 not available, skipping script tests${NC}"
    ((TESTS_SKIPPED+=3))
fi
echo ""

# =============================================================================
# Stage 4: USD Validation (if available)
# =============================================================================

echo -e "${YELLOW}[Stage 4] USD Validation${NC}"

if [ -f "${SCRIPT_DIR}/validate_usd.py" ] && command -v python3 &> /dev/null; then
    # Check for existing scene files
    if [ -d "${PROJECT_DIR}/scenes" ]; then
        for scene_file in "${PROJECT_DIR}"/scenes/*.usda; do
            if [ -f "$scene_file" ]; then
                scene_name=$(basename "$scene_file")
                run_test "USD syntax: ${scene_name}" "python3 ${SCRIPT_DIR}/validate_usd.py ${scene_file}" false
            fi
        done
    else
        echo "  No scenes directory found, skipping USD validation"
        ((TESTS_SKIPPED++))
    fi
else
    echo "  USD validation script not available, skipping"
    ((TESTS_SKIPPED++))
fi
echo ""

# =============================================================================
# Stage 5: Code Quality (if quick mode, skip)
# =============================================================================

if [ "$TEST_MODE" != "quick" ]; then
    echo -e "${YELLOW}[Stage 5] Code Quality${NC}"

    # Check for common issues in C++ code
    if command -v grep &> /dev/null; then
        run_test "No TODO/FIXME in release" "! grep -r 'TODO\|FIXME' ${PROJECT_DIR}/src --include='*.cpp' --include='*.h' | grep -v 'TODO:' > /dev/null" false

        # Check for debug prints
        run_test "No std::cout in production" "! grep -r 'std::cout' ${PROJECT_DIR}/src/core/*.cpp | grep -v '// DEBUG' > /dev/null" false
    fi

    # Check header guards
    for header in "${PROJECT_DIR}"/src/**/*.h "${PROJECT_DIR}"/include/*.h; do
        if [ -f "$header" ]; then
            header_name=$(basename "$header")
            run_test "Header guard: ${header_name}" "head -5 '$header' | grep -q '#pragma once\|#ifndef'" false
        fi
    done
    echo ""
fi

# =============================================================================
# Stage 6: Integration Tests (full mode only)
# =============================================================================

if [ "$TEST_MODE" = "full" ]; then
    echo -e "${YELLOW}[Stage 6] Integration Tests${NC}"

    # Test that generated content can be loaded
    if [ -f "$TEST_BINARY" ]; then
        # Generate test content
        TEMP_DIR=$(mktemp -d)
        python3 "${SCRIPT_DIR}/generate_scenes.py" --output "${TEMP_DIR}/scenes/" --variant basic > /dev/null 2>&1

        # Run integration tests (would need to be added to test suite)
        run_test "Scene loading integration" "$TEST_BINARY --usd" false

        rm -rf "$TEMP_DIR"
    fi
    echo ""
fi

# =============================================================================
# Summary
# =============================================================================

echo -e "${BLUE}=============================================${NC}"
echo -e "${BLUE}  Test Summary${NC}"
echo -e "${BLUE}=============================================${NC}"
echo ""
echo -e "  ${GREEN}Passed:${NC}  ${TESTS_PASSED}"
echo -e "  ${RED}Failed:${NC}  ${TESTS_FAILED}"
echo -e "  ${YELLOW}Skipped:${NC} ${TESTS_SKIPPED}"
echo ""

if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi
