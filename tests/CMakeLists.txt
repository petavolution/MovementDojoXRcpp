# Test suite for Movement Dojo XR
# Comprehensive CLI-first testing framework

# Find testing framework (prefer GoogleTest if available)
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "GoogleTest found, using GTest")
    set(TEST_FRAMEWORK "gtest")
else()
    message(STATUS "GoogleTest not found, using built-in testing")
    set(TEST_FRAMEWORK "builtin")
endif()

# Test sources - organized by category
set(TEST_SOURCES
    # Main test runner
    test_main.cpp

    # Core engine tests
    test_math.cpp
    test_physics.cpp

    # Session and state tests
    test_session_manager.cpp

    # Asset and scene tests
    test_usd_loader.cpp

    # Feedback tests
    test_haptics.cpp

    # Analytics and tracking tests
    test_analytics.cpp
    test_visualization.cpp

    # Game systems tests
    test_progression.cpp

    # Integration tests
    test_integration.cpp
)

# Source files needed by tests
set(TEST_SUPPORT_SOURCES
    # Core systems
    ${CMAKE_SOURCE_DIR}/src/core/SessionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Math.cpp

    # Physics (simplified implementation for tests)
    ${CMAKE_SOURCE_DIR}/src/physics/PhysicsEngine.cpp

    # Haptics
    ${CMAKE_SOURCE_DIR}/src/haptics/HapticManager.cpp

    # Analytics
    ${CMAKE_SOURCE_DIR}/src/analytics/MovementAnalytics.cpp

    # Visualization
    ${CMAKE_SOURCE_DIR}/src/visualization/MovementVisualizer.cpp

    # Progression
    ${CMAKE_SOURCE_DIR}/src/progression/ProgressionSystem.cpp

    # Training
    ${CMAKE_SOURCE_DIR}/src/training/TrainingModule.cpp
    ${CMAKE_SOURCE_DIR}/src/training/FlowModes.cpp

    # USD
    ${CMAKE_SOURCE_DIR}/src/usd/USDLoader.cpp

    # Overlay
    ${CMAKE_SOURCE_DIR}/src/overlay/OverlaySystem.cpp
)

# Create test executable
add_executable(lightsaber_tests ${TEST_SOURCES} ${TEST_SUPPORT_SOURCES})

# Link with main project libraries
target_link_libraries(lightsaber_tests
    PRIVATE
    ${OPENXR_LIBRARIES}
    Threads::Threads
)

# Link with test framework
if(TEST_FRAMEWORK STREQUAL "gtest")
    target_link_libraries(lightsaber_tests PRIVATE GTest::gtest GTest::gtest_main)
    target_compile_definitions(lightsaber_tests PRIVATE USE_GTEST=1)
else()
    target_compile_definitions(lightsaber_tests PRIVATE USE_GTEST=0)
endif()

# Include directories
target_include_directories(lightsaber_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Enable all warnings for test code
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lightsaber_tests PRIVATE -Wall -Wextra)
endif()

# ==============================================================================
# CTest Configuration
# ==============================================================================

# Individual test categories
add_test(NAME MathTests COMMAND lightsaber_tests --math)
add_test(NAME USDLoaderTests COMMAND lightsaber_tests --usd)
add_test(NAME SessionManagerTests COMMAND lightsaber_tests --session)
add_test(NAME VisualizationTests COMMAND lightsaber_tests --viz)
add_test(NAME PhysicsTests COMMAND lightsaber_tests --physics)
add_test(NAME HapticsTests COMMAND lightsaber_tests --haptics)
add_test(NAME ProgressionTests COMMAND lightsaber_tests --progression)
add_test(NAME AnalyticsTests COMMAND lightsaber_tests --analytics)
add_test(NAME IntegrationTests COMMAND lightsaber_tests --integration)

# Test groups
add_test(NAME CoreTests COMMAND lightsaber_tests --core)
add_test(NAME AllTests COMMAND lightsaber_tests --all)

# Set test properties
set_tests_properties(
    MathTests
    USDLoaderTests
    SessionManagerTests
    VisualizationTests
    PhysicsTests
    HapticsTests
    ProgressionTests
    AnalyticsTests
    IntegrationTests
    CoreTests
    AllTests
    PROPERTIES
    TIMEOUT 120
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Label tests for filtering
set_tests_properties(MathTests PhysicsTests SessionManagerTests PROPERTIES LABELS "core")
set_tests_properties(ProgressionTests AnalyticsTests PROPERTIES LABELS "gameplay")
set_tests_properties(IntegrationTests PROPERTIES LABELS "integration")
set_tests_properties(HapticsTests PROPERTIES LABELS "hardware")

# ==============================================================================
# Output configuration
# ==============================================================================

# Put test binary in a known location
set_target_properties(lightsaber_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Custom target for running quick tests
add_custom_target(test-quick
    COMMAND ${CMAKE_CTEST_COMMAND} -L core --output-on-failure
    DEPENDS lightsaber_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running core tests..."
)

# Custom target for running all tests with verbose output
add_custom_target(test-verbose
    COMMAND ${CMAKE_BINARY_DIR}/bin/lightsaber_tests --all
    DEPENDS lightsaber_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running all tests with verbose output..."
)
