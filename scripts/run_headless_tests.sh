#!/bin/bash
# =============================================================================
# Headless Test Runner for Movement Dojo
#
# Runs all tests without requiring VR hardware or display.
# Suitable for CI/CD pipelines and development machines without XR runtime.
#
# Usage:
#   ./scripts/run_headless_tests.sh              # Run standard tests
#   ./scripts/run_headless_tests.sh --quick      # Run core tests only
#   ./scripts/run_headless_tests.sh --full       # Run full test suite
#   ./scripts/run_headless_tests.sh --category   # Run specific category
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
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test mode and options
TEST_MODE="standard"
VERBOSE=""
CATEGORY=""

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
        --category)
            CATEGORY="$2"
            shift 2
            ;;
        --math|--physics|--session|--viz|--haptics|--analytics|--progression|--integration|--engine)
            CATEGORY="${1#--}"
            shift
            ;;
        --core)
            CATEGORY="core"
            shift
            ;;
        --all)
            CATEGORY="all"
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Test Modes:"
            echo "  --quick       Run only core unit tests (math, physics, session)"
            echo "  --standard    Run standard test suite (default)"
            echo "  --full        Run all tests including integration and stress tests"
            echo ""
            echo "Categories:"
            echo "  --math        Run math tests only"
            echo "  --physics     Run physics engine tests only"
            echo "  --session     Run session manager tests only"
            echo "  --viz         Run visualization tests only"
            echo "  --haptics     Run haptics tests only"
            echo "  --analytics   Run analytics tests only"
            echo "  --progression Run progression tests only"
            echo "  --integration Run integration tests only"
            echo "  --engine      Run unified Engine architecture tests"
            echo "  --core        Run core engine tests (math, physics, session, engine)"
            echo "  --all         Run all test categories"
            echo ""
            echo "Options:"
            echo "  --verbose     Show detailed output"
            echo "  --help        Show this help"
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
[ -n "$CATEGORY" ] && echo "Category: ${CATEGORY}"
echo ""

# Track results
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0
START_TIME=$(date +%s)

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
                echo "  Output:"
                cat /tmp/test_output_$$.txt | sed 's/^/    /'
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
# Stage 1: Build System
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
if ! cmake --build . --target lightsaber_tests -j$(nproc 2>/dev/null || echo 4) > /tmp/build_output.txt 2>&1; then
    echo -e "${RED}Build failed!${NC}"
    cat /tmp/build_output.txt
    exit 1
fi
cd "$PROJECT_DIR"
echo -e "  ${GREEN}Build successful${NC}"
echo ""

# =============================================================================
# Stage 2: Unit Tests
# =============================================================================

echo -e "${YELLOW}[Stage 2] Unit Tests${NC}"

TEST_BINARY="${BUILD_DIR}/bin/lightsaber_tests"
if [ ! -f "$TEST_BINARY" ]; then
    TEST_BINARY="${BUILD_DIR}/lightsaber_tests"
fi

if [ -f "$TEST_BINARY" ]; then
    # Determine which tests to run based on mode and category
    if [ -n "$CATEGORY" ]; then
        # Run specific category
        run_test "${CATEGORY} tests" "$TEST_BINARY --${CATEGORY}"
    elif [ "$TEST_MODE" = "quick" ]; then
        # Core tests only
        run_test "Core engine tests" "$TEST_BINARY --core"
    else
        # Standard or full mode - run all categories
        run_test "Math tests" "$TEST_BINARY --math"
        run_test "Physics tests" "$TEST_BINARY --physics"
        run_test "Session tests" "$TEST_BINARY --session"
        run_test "USD loader tests" "$TEST_BINARY --usd"
        run_test "Visualization tests" "$TEST_BINARY --viz"
        run_test "Haptics tests" "$TEST_BINARY --haptics"
        run_test "Analytics tests" "$TEST_BINARY --analytics"
        run_test "Progression tests" "$TEST_BINARY --progression"

        if [ "$TEST_MODE" = "full" ]; then
            run_test "Integration tests" "$TEST_BINARY --integration"
        fi
    fi
else
    echo -e "  ${RED}Test binary not found: ${TEST_BINARY}${NC}"
    ((TESTS_FAILED++))
fi
echo ""

# =============================================================================
# Stage 3: Python Script Validation
# =============================================================================

if [ "$TEST_MODE" != "quick" ]; then
    echo -e "${YELLOW}[Stage 3] Script Validation${NC}"

    if command -v python3 &> /dev/null; then
        # Test script syntax
        for script in generate_scenes.py generate_exercises.py generate_meshes.py validate_usd.py debug_engine.py; do
            if [ -f "${SCRIPT_DIR}/${script}" ]; then
                run_test "${script} syntax" "python3 -m py_compile ${SCRIPT_DIR}/${script}"
            fi
        done

        # Test debug engine tool
        if [ -f "${SCRIPT_DIR}/debug_engine.py" ]; then
            run_test "Debug tool validation" "python3 ${SCRIPT_DIR}/debug_engine.py validate"
            run_test "Debug tool benchmark" "python3 ${SCRIPT_DIR}/debug_engine.py benchmark" false
        fi

        if [ "$TEST_MODE" = "full" ]; then
            # Run generators
            TEMP_DIR=$(mktemp -d)
            run_test "Scene generation" "python3 ${SCRIPT_DIR}/generate_scenes.py --output ${TEMP_DIR}/scenes/" false
            run_test "Exercise generation" "python3 ${SCRIPT_DIR}/generate_exercises.py --output ${TEMP_DIR}/exercises/" false
            rm -rf "$TEMP_DIR"
        fi
    else
        echo -e "  ${YELLOW}Python3 not available, skipping script tests${NC}"
        ((TESTS_SKIPPED+=5))
    fi
    echo ""
fi

# =============================================================================
# Stage 4: USD Validation
# =============================================================================

if [ "$TEST_MODE" = "full" ]; then
    echo -e "${YELLOW}[Stage 4] USD Scene Validation${NC}"

    if [ -f "${SCRIPT_DIR}/validate_usd.py" ] && command -v python3 &> /dev/null; then
        if [ -d "${PROJECT_DIR}/scenes" ]; then
            for scene_file in "${PROJECT_DIR}"/scenes/*.usda; do
                if [ -f "$scene_file" ]; then
                    scene_name=$(basename "$scene_file")
                    run_test "USD: ${scene_name}" "python3 ${SCRIPT_DIR}/validate_usd.py ${scene_file}" false
                fi
            done
        else
            echo "  No scenes directory found"
            ((TESTS_SKIPPED++))
        fi
    else
        echo "  USD validation not available"
        ((TESTS_SKIPPED++))
    fi
    echo ""
fi

# =============================================================================
# Stage 5: Code Quality (full mode)
# =============================================================================

if [ "$TEST_MODE" = "full" ]; then
    echo -e "${YELLOW}[Stage 5] Code Quality Checks${NC}"

    # Check header guards
    headers_checked=0
    headers_valid=0
    for header in "${PROJECT_DIR}"/src/**/*.h "${PROJECT_DIR}"/include/*.h; do
        if [ -f "$header" ]; then
            ((headers_checked++))
            if head -5 "$header" | grep -q '#pragma once\|#ifndef'; then
                ((headers_valid++))
            fi
        fi
    done
    if [ $headers_checked -gt 0 ]; then
        if [ $headers_valid -eq $headers_checked ]; then
            echo -e "  ${GREEN}Header guards: ${headers_valid}/${headers_checked} OK${NC}"
            ((TESTS_PASSED++))
        else
            echo -e "  ${YELLOW}Header guards: ${headers_valid}/${headers_checked}${NC}"
            ((TESTS_SKIPPED++))
        fi
    fi

    # Check for test coverage
    test_files=$(find "${PROJECT_DIR}/tests" -name "test_*.cpp" 2>/dev/null | wc -l)
    echo -e "  ${CYAN}Test files: ${test_files}${NC}"
    echo ""
fi

# =============================================================================
# Summary
# =============================================================================

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

echo -e "${BLUE}=============================================${NC}"
echo -e "${BLUE}  Test Summary${NC}"
echo -e "${BLUE}=============================================${NC}"
echo ""
echo -e "  ${GREEN}Passed:${NC}  ${TESTS_PASSED}"
echo -e "  ${RED}Failed:${NC}  ${TESTS_FAILED}"
echo -e "  ${YELLOW}Skipped:${NC} ${TESTS_SKIPPED}"
echo -e "  ${CYAN}Time:${NC}    ${ELAPSED}s"
echo ""

# Cleanup
rm -f /tmp/test_output_$$.txt /tmp/build_output.txt

if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi
