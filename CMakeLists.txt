cmake_minimum_required(VERSION 3.16)
project(LightsaberTrainer VERSION 0.1.0 LANGUAGES CXX C)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_VALIDATION "Enable OpenXR validation layer" ON)
option(USE_SYSTEM_USD "Use system-installed USD" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Vulkan REQUIRED)

# OpenXR - try find_package first, fall back to pkg-config
find_package(OpenXR QUIET)
if(NOT OpenXR_FOUND)
    pkg_check_modules(OPENXR REQUIRED openxr)
    set(OPENXR_INCLUDE_DIRS ${OPENXR_INCLUDE_DIRS})
    set(OPENXR_LIBRARIES ${OPENXR_LIBRARIES})
else()
    set(OPENXR_INCLUDE_DIRS "")
    set(OPENXR_LIBRARIES OpenXR::openxr_loader)
endif()

# GLFW for windowing
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    pkg_check_modules(GLFW3 REQUIRED glfw3)
endif()

# GLM for math
find_package(glm QUIET)
if(NOT glm_FOUND)
    pkg_check_modules(GLM REQUIRED glm)
endif()

# USD - optional, can use stub if not available
if(USE_SYSTEM_USD)
    find_package(pxr QUIET)
    if(pxr_FOUND)
        set(USD_FOUND TRUE)
        set(USD_INCLUDE_DIRS ${PXR_INCLUDE_DIRS})
        set(USD_LIBRARIES ${PXR_LIBRARIES})
    else()
        pkg_check_modules(USD usd)
    endif()
endif()

# Bullet Physics - optional
find_package(Bullet QUIET)
if(NOT Bullet_FOUND)
    pkg_check_modules(BULLET bullet)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
    ${OPENXR_INCLUDE_DIRS}
)

if(glfw3_FOUND)
    # Modern CMake target available
else()
    include_directories(${GLFW3_INCLUDE_DIRS})
endif()

if(glm_FOUND)
    # Modern CMake target available
else()
    include_directories(${GLM_INCLUDE_DIRS})
endif()

if(USD_FOUND)
    include_directories(${USD_INCLUDE_DIRS})
    add_compile_definitions(HAS_USD=1)
else()
    add_compile_definitions(HAS_USD=0)
endif()

if(BULLET_FOUND OR Bullet_FOUND)
    include_directories(${BULLET_INCLUDE_DIRS})
    add_compile_definitions(HAS_BULLET=1)
else()
    add_compile_definitions(HAS_BULLET=0)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

if(ENABLE_VALIDATION)
    add_compile_definitions(XR_ENABLE_VALIDATION=1)
endif()

# Source files
set(CORE_SOURCES
    src/core/XRSession.cpp
    src/core/Renderer.cpp
    src/core/Input.cpp
    src/core/Math.cpp
    src/core/SessionManager.cpp
)

set(USD_SOURCES
    src/usd/USDLoader.cpp
)

set(TRAINING_SOURCES
    src/training/TrainingModule.cpp
    src/training/FlowModes.cpp
)

set(ANALYTICS_SOURCES
    src/analytics/MovementAnalytics.cpp
)

set(PROGRESSION_SOURCES
    src/progression/ProgressionSystem.cpp
)

set(OVERLAY_SOURCES
    src/overlay/OverlaySystem.cpp
)

set(HAPTIC_SOURCES
    src/haptics/HapticManager.cpp
)

set(PHYSICS_SOURCES
    src/physics/PhysicsEngine.cpp
)

# Main executable
add_executable(lightsaber_trainer
    src/main.cpp
    ${CORE_SOURCES}
    ${USD_SOURCES}
    ${TRAINING_SOURCES}
    ${ANALYTICS_SOURCES}
    ${PROGRESSION_SOURCES}
    ${OVERLAY_SOURCES}
    ${HAPTIC_SOURCES}
    ${PHYSICS_SOURCES}
)

# Link libraries
target_link_libraries(lightsaber_trainer
    ${OPENXR_LIBRARIES}
    ${Vulkan_LIBRARIES}
    Threads::Threads
)

if(glfw3_FOUND)
    target_link_libraries(lightsaber_trainer glfw)
else()
    target_link_libraries(lightsaber_trainer ${GLFW3_LIBRARIES})
endif()

if(glm_FOUND)
    target_link_libraries(lightsaber_trainer glm::glm)
endif()

if(USD_FOUND)
    target_link_libraries(lightsaber_trainer ${USD_LIBRARIES})
endif()

if(BULLET_FOUND)
    target_link_libraries(lightsaber_trainer ${BULLET_LIBRARIES})
elseif(Bullet_FOUND)
    target_link_libraries(lightsaber_trainer
        Bullet::Bullet
    )
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS lightsaber_trainer
    RUNTIME DESTINATION bin
)

install(DIRECTORY scenes/
    DESTINATION share/lightsaber_trainer/scenes
    FILES_MATCHING PATTERN "*.usda"
)

install(DIRECTORY assets/
    DESTINATION share/lightsaber_trainer/assets
)

# CPack configuration
set(CPACK_PACKAGE_NAME "lightsaber-trainer")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "USD-Based OpenXR Lightsaber Proprioception Trainer")
set(CPACK_GENERATOR "DEB;TGZ")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Developer")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libopenxr-loader1, libvulkan1, libglfw3")
include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "=== Lightsaber Trainer Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenXR: Found")
message(STATUS "Vulkan: ${Vulkan_FOUND}")
message(STATUS "GLFW: ${glfw3_FOUND}")
message(STATUS "GLM: ${glm_FOUND}")
message(STATUS "USD: ${USD_FOUND}")
message(STATUS "Bullet: ${BULLET_FOUND}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Validation: ${ENABLE_VALIDATION}")
message(STATUS "=========================================")
message(STATUS "")
