#!/bin/bash
# =============================================================================
# Engine Startup Validation Script
#
# Validates that the Movement Dojo engine can:
# 1. Initialize in headless mode
# 2. Generate mock tracking data
# 3. Run through its frame loop
# 4. Shutdown cleanly
#
# Usage:
#   ./scripts/validate_startup.sh           # Quick validation (100 frames)
#   ./scripts/validate_startup.sh --full    # Full validation (1000 frames)
#   ./scripts/validate_startup.sh --stress  # Stress test (10000 frames)
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Default frame count
FRAME_COUNT=100

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --full)
            FRAME_COUNT=1000
            shift
            ;;
        --stress)
            FRAME_COUNT=10000
            shift
            ;;
        --frames)
            FRAME_COUNT="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --full      Run 1000 frames"
            echo "  --stress    Run 10000 frames (stress test)"
            echo "  --frames N  Run N frames"
            echo "  --help      Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${CYAN}=============================================${NC}"
echo -e "${CYAN}  Movement Dojo - Engine Startup Validation${NC}"
echo -e "${CYAN}=============================================${NC}"
echo ""

# Check if build exists
if [ ! -f "${BUILD_DIR}/movement_dojo_simple" ]; then
    echo -e "${YELLOW}Build not found. Building project...${NC}"
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    cmake .. -DBUILD_TESTS=ON
    make -j$(nproc) movement_dojo_simple
    cd "${PROJECT_DIR}"
fi

# Remove old debug log
rm -f "${PROJECT_DIR}/debug-log.txt"

echo -e "${CYAN}Running headless validation (${FRAME_COUNT} frames)...${NC}"
echo ""

# Run the engine in headless mode
START_TIME=$(date +%s.%N)

if "${BUILD_DIR}/movement_dojo_simple" --headless --mock --frames ${FRAME_COUNT}; then
    END_TIME=$(date +%s.%N)
    ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
    FPS=$(echo "scale=2; ${FRAME_COUNT} / ${ELAPSED}" | bc)

    echo ""
    echo -e "${GREEN}=============================================${NC}"
    echo -e "${GREEN}  VALIDATION PASSED${NC}"
    echo -e "${GREEN}=============================================${NC}"
    echo ""
    echo "  Frames: ${FRAME_COUNT}"
    echo "  Time: ${ELAPSED}s"
    echo "  FPS: ~${FPS}"

    # Check debug log was created
    if [ -f "${PROJECT_DIR}/debug-log.txt" ]; then
        LOG_LINES=$(wc -l < "${PROJECT_DIR}/debug-log.txt")
        echo "  Debug log: ${LOG_LINES} lines written"

        # Show last few log entries
        echo ""
        echo -e "${CYAN}Last log entries:${NC}"
        tail -5 "${PROJECT_DIR}/debug-log.txt"
    else
        echo -e "${YELLOW}  Warning: debug-log.txt not created${NC}"
    fi

    echo ""
    exit 0
else
    echo ""
    echo -e "${RED}=============================================${NC}"
    echo -e "${RED}  VALIDATION FAILED${NC}"
    echo -e "${RED}=============================================${NC}"

    # Show debug log if available
    if [ -f "${PROJECT_DIR}/debug-log.txt" ]; then
        echo ""
        echo -e "${CYAN}Debug log contents:${NC}"
        cat "${PROJECT_DIR}/debug-log.txt"
    fi

    echo ""
    exit 1
fi
