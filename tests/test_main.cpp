/**
 * Test Runner Main
 *
 * Dispatches to individual test suites based on command line arguments.
 * Supports running all tests or selecting specific test categories.
 */

#include <iostream>
#include <cstring>
#include <chrono>

// Forward declarations for all test suites
int runMathTests();
int runUSDLoaderTests();
int runSessionManagerTests();
int runVisualizationTests();
int runPhysicsTests();
int runHapticsTests();
int runProgressionTests();
int runAnalyticsTests();
int runIntegrationTests();

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Test Categories:" << std::endl;
    std::cout << "  --math        Run math/geometry tests" << std::endl;
    std::cout << "  --usd         Run USD loader tests" << std::endl;
    std::cout << "  --session     Run SessionManager tests" << std::endl;
    std::cout << "  --viz         Run visualization tests" << std::endl;
    std::cout << "  --physics     Run physics engine tests" << std::endl;
    std::cout << "  --haptics     Run haptic feedback tests" << std::endl;
    std::cout << "  --progression Run progression system tests" << std::endl;
    std::cout << "  --analytics   Run movement analytics tests" << std::endl;
    std::cout << "  --integration Run integration tests" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --all         Run all tests (default)" << std::endl;
    std::cout << "  --core        Run core engine tests (math, physics, session)" << std::endl;
    std::cout << "  --quick       Run quick tests only (skip stress tests)" << std::endl;
    std::cout << "  --verbose     Show detailed test output" << std::endl;
    std::cout << "  --help        Show this help" << std::endl;
}

void printSeparator(const std::string& title = "") {
    std::cout << "========================================" << std::endl;
    if (!title.empty()) {
        std::cout << title << std::endl;
        std::cout << "========================================" << std::endl;
    }
}

#ifndef USE_GTEST
int main(int argc, char* argv[]) {
    // Test selection flags
    bool runAll = true;
    bool runCore = false;
    bool runMath = false;
    bool runUSD = false;
    bool runSession = false;
    bool runViz = false;
    bool runPhysics = false;
    bool runHaptics = false;
    bool runProgression = false;
    bool runAnalytics = false;
    bool runIntegration = false;

    // Parse command line arguments
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
        } else if (strcmp(argv[i], "--physics") == 0) {
            runPhysics = true;
            runAll = false;
        } else if (strcmp(argv[i], "--haptics") == 0) {
            runHaptics = true;
            runAll = false;
        } else if (strcmp(argv[i], "--progression") == 0) {
            runProgression = true;
            runAll = false;
        } else if (strcmp(argv[i], "--analytics") == 0) {
            runAnalytics = true;
            runAll = false;
        } else if (strcmp(argv[i], "--integration") == 0) {
            runIntegration = true;
            runAll = false;
        } else if (strcmp(argv[i], "--core") == 0) {
            runCore = true;
            runAll = false;
        } else if (strcmp(argv[i], "--all") == 0) {
            runAll = true;
        }
    }

    // If core selected, enable core tests
    if (runCore) {
        runMath = true;
        runPhysics = true;
        runSession = true;
    }

    int result = 0;
    int totalPassed = 0;
    int totalFailed = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    printSeparator("Movement Dojo Test Suite");
    std::cout << std::endl;

    // Run selected test suites
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

    if (runAll || runPhysics) {
        result |= runPhysicsTests();
        std::cout << std::endl;
    }

    if (runAll || runHaptics) {
        result |= runHapticsTests();
        std::cout << std::endl;
    }

    if (runAll || runProgression) {
        result |= runProgressionTests();
        std::cout << std::endl;
    }

    if (runAll || runAnalytics) {
        result |= runAnalyticsTests();
        std::cout << std::endl;
    }

    if (runAll || runIntegration) {
        result |= runIntegrationTests();
        std::cout << std::endl;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    printSeparator();
    if (result == 0) {
        std::cout << "All tests PASSED!" << std::endl;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
    }
    std::cout << "Total time: " << duration.count() << "ms" << std::endl;
    printSeparator();

    return result;
}
#endif
