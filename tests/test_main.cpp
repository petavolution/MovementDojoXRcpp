/**
 * Test Runner Main
 *
 * Dispatches to individual test suites based on command line arguments.
 */

#include <iostream>
#include <cstring>

// Forward declarations
int runMathTests();
int runUSDLoaderTests();
int runSessionManagerTests();
int runVisualizationTests();

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --math      Run math tests only" << std::endl;
    std::cout << "  --usd       Run USD loader tests only" << std::endl;
    std::cout << "  --session   Run SessionManager tests only" << std::endl;
    std::cout << "  --viz       Run visualization tests only" << std::endl;
    std::cout << "  --all       Run all tests (default)" << std::endl;
    std::cout << "  --help      Show this help" << std::endl;
}

#ifndef USE_GTEST
int main(int argc, char* argv[]) {
    bool runAll = true;
    bool runMath = false;
    bool runUSD = false;
    bool runSession = false;
    bool runViz = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--math") == 0) {
            runMath = true;
            runAll = false;
        } else if (strcmp(argv[i], "--usd") == 0) {
            runUSD = true;
            runAll = false;
        } else if (strcmp(argv[i], "--session") == 0) {
            runSession = true;
            runAll = false;
        } else if (strcmp(argv[i], "--viz") == 0) {
            runViz = true;
            runAll = false;
        } else if (strcmp(argv[i], "--all") == 0) {
            runAll = true;
        }
    }

    int result = 0;

    std::cout << "========================================" << std::endl;
    std::cout << "Movement Dojo Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    if (runAll || runMath) {
        result |= runMathTests();
        std::cout << std::endl;
    }

    if (runAll || runUSD) {
        result |= runUSDLoaderTests();
        std::cout << std::endl;
    }

    if (runAll || runSession) {
        result |= runSessionManagerTests();
        std::cout << std::endl;
    }

    if (runAll || runViz) {
        result |= runVisualizationTests();
        std::cout << std::endl;
    }

    std::cout << "========================================" << std::endl;
    if (result == 0) {
        std::cout << "All tests PASSED!" << std::endl;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return result;
}
#endif
