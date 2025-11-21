#!/bin/bash
# Build script for Lightsaber Trainer
# Usage: ./scripts/build.sh [debug|release|clean]

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
BUILD_TYPE="${1:-Release}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Handle build type argument
case "${BUILD_TYPE,,}" in
    debug)
        BUILD_TYPE="Debug"
        ;;
    release)
        BUILD_TYPE="Release"
        ;;
    clean)
        log_info "Cleaning build directory..."
        rm -rf "${BUILD_DIR}"
        log_info "Clean complete"
        exit 0
        ;;
    *)
        BUILD_TYPE="Release"
        ;;
esac

log_info "=== Lightsaber Trainer Build Script ==="
log_info "Build type: ${BUILD_TYPE}"
log_info "Project directory: ${PROJECT_DIR}"
log_info "Build directory: ${BUILD_DIR}"

# Check for required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        log_error "$1 is not installed"
        return 1
    fi
    return 0
}

log_info "Checking required tools..."
check_tool cmake || exit 1
check_tool ninja || check_tool make || { log_error "Neither ninja nor make found"; exit 1; }

# Determine generator
if command -v ninja &> /dev/null; then
    GENERATOR="Ninja"
    BUILD_CMD="ninja"
else
    GENERATOR="Unix Makefiles"
    BUILD_CMD="make -j$(nproc)"
fi

log_info "Using generator: ${GENERATOR}"

# Create build directory
mkdir -p "${BUILD_DIR}"

# Check for Conan
if command -v conan &> /dev/null; then
    log_info "Conan found, installing dependencies..."
    cd "${PROJECT_DIR}"

    # Check if conanfile.txt exists
    if [ -f "conanfile.txt" ]; then
        conan install . --output-folder="${BUILD_DIR}" --build=missing \
            -s build_type="${BUILD_TYPE}" || {
            log_warn "Conan install failed, continuing without Conan dependencies"
        }
    fi
else
    log_warn "Conan not found, using system libraries"
fi

# Configure with CMake
log_info "Configuring with CMake..."
cd "${BUILD_DIR}"

cmake -G "${GENERATOR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    "${PROJECT_DIR}" || {
    log_error "CMake configuration failed"
    exit 1
}

# Build
log_info "Building..."
${BUILD_CMD} || {
    log_error "Build failed"
    exit 1
}

# Copy compile_commands.json to project root (for IDE support)
if [ -f "compile_commands.json" ]; then
    cp compile_commands.json "${PROJECT_DIR}/"
fi

log_info "=== Build Complete ==="
log_info "Executable: ${BUILD_DIR}/bin/lightsaber_trainer"

# Print next steps
echo ""
log_info "Next steps:"
echo "  1. Generate scenes:  python3 scripts/generate_scene.py"
echo "  2. Validate scenes:  python3 scripts/validate_usd.py scenes/"
echo "  3. Run (headless):   ./build/bin/lightsaber_trainer --scene scenes/stage1_cube.usda --validate-only"
echo "  4. Run (VR):         ./build/bin/lightsaber_trainer --scene scenes/dojo_basic.usda"
