#!/bin/bash
# Run script for Lightsaber Trainer
# Usage: ./scripts/run.sh [options]

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
EXECUTABLE="${BUILD_DIR}/bin/lightsaber_trainer"

# Default scene
DEFAULT_SCENE="${PROJECT_DIR}/scenes/stage1_cube.usda"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "Lightsaber Trainer - Run Script"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --scene <path>      Path to USD scene file"
    echo "  --validate          Run in validation mode (no XR)"
    echo "  --generate          Generate default scenes before running"
    echo "  --build             Build before running"
    echo "  --debug             Run with debugger (gdb)"
    echo "  --help              Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Run with default scene"
    echo "  $0 --scene scenes/dojo_basic.usda   # Run with specific scene"
    echo "  $0 --validate --scene scenes/        # Validate all scenes"
    echo "  $0 --generate --build                # Generate, build, and run"
}

# Parse arguments
SCENE="${DEFAULT_SCENE}"
VALIDATE_MODE=false
GENERATE_SCENES=false
BUILD_FIRST=false
DEBUG_MODE=false
EXTRA_ARGS=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --scene)
            SCENE="$2"
            shift 2
            ;;
        --validate)
            VALIDATE_MODE=true
            shift
            ;;
        --generate)
            GENERATE_SCENES=true
            shift
            ;;
        --build)
            BUILD_FIRST=true
            shift
            ;;
        --debug)
            DEBUG_MODE=true
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            EXTRA_ARGS="${EXTRA_ARGS} $1"
            shift
            ;;
    esac
done

# Generate scenes if requested
if [ "${GENERATE_SCENES}" = true ]; then
    log_info "Generating USD scenes..."
    python3 "${PROJECT_DIR}/scripts/generate_scene.py" -o "${PROJECT_DIR}/scenes"
fi

# Build if requested
if [ "${BUILD_FIRST}" = true ]; then
    log_info "Building..."
    "${PROJECT_DIR}/scripts/build.sh"
fi

# Check executable exists
if [ ! -f "${EXECUTABLE}" ]; then
    log_error "Executable not found: ${EXECUTABLE}"
    log_info "Run './scripts/build.sh' first"
    exit 1
fi

# Resolve scene path
if [[ ! "${SCENE}" = /* ]]; then
    # Relative path - make absolute
    SCENE="${PROJECT_DIR}/${SCENE}"
fi

# Check scene exists
if [ ! -e "${SCENE}" ]; then
    log_error "Scene not found: ${SCENE}"
    log_info "Available scenes:"
    ls -la "${PROJECT_DIR}/scenes/" 2>/dev/null || echo "  No scenes directory"
    log_info "Generate scenes with: python3 scripts/generate_scene.py"
    exit 1
fi

# Build command
CMD="${EXECUTABLE} --scene ${SCENE}"

if [ "${VALIDATE_MODE}" = true ]; then
    CMD="${CMD} --validate-only"
fi

CMD="${CMD}${EXTRA_ARGS}"

# Run
log_info "Running Lightsaber Trainer..."
log_info "Command: ${CMD}"
echo ""

if [ "${DEBUG_MODE}" = true ]; then
    if command -v gdb &> /dev/null; then
        gdb --args ${CMD}
    else
        log_error "gdb not found"
        exit 1
    fi
else
    exec ${CMD}
fi
