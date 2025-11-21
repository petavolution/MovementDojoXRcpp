# Test suite for Lightsaber Trainer

# Find testing framework (prefer GoogleTest if available)
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "GoogleTest found, using GTest")
    set(TEST_FRAMEWORK "gtest")
else()
    message(STATUS "GoogleTest not found, using built-in testing")
    set(TEST_FRAMEWORK "builtin")
endif()

# Test sources
set(TEST_SOURCES
    test_main.cpp
    test_math.cpp
    test_usd_loader.cpp
    test_session_manager.cpp
)

# Source files needed by tests
set(TEST_SUPPORT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/SessionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Math.cpp
    ${CMAKE_SOURCE_DIR}/src/analytics/MovementAnalytics.cpp
    ${CMAKE_SOURCE_DIR}/src/progression/ProgressionSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/overlay/OverlaySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/training/TrainingModule.cpp
    ${CMAKE_SOURCE_DIR}/src/training/FlowModes.cpp
    ${CMAKE_SOURCE_DIR}/src/usd/USDLoader.cpp
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

# Add tests to CTest
add_test(NAME MathTests COMMAND lightsaber_tests --math)
add_test(NAME USDLoaderTests COMMAND lightsaber_tests --usd)
add_test(NAME SessionManagerTests COMMAND lightsaber_tests --session)
add_test(NAME AllTests COMMAND lightsaber_tests)

# Set test properties
set_tests_properties(MathTests USDLoaderTests SessionManagerTests AllTests
    PROPERTIES
    TIMEOUT 60
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
