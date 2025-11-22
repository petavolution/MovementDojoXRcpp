#pragma once
/**
 * Environment.h - Training Environment Base Class
 *
 * Abstract base class for procedural training environments.
 * Each environment creates its own scene objects and can be
 * updated per-frame for animations.
 */

#include "../../include/Types.h"
#include "../procedural/ProceduralMeshUtils.h"
#include <string>
#include <vector>
#include <memory>

// Forward declaration
namespace lst {
    class Engine;
}

namespace lst {

#define LOG_TAG_ENV "Environment"

/**
 * Abstract base class for training environments
 */
class Environment {
public:
    virtual ~Environment() = default;

    /**
     * Build the environment in the scene
     * Creates all procedural meshes and adds them to the engine
     * @param engine  Engine to add scene objects to
     * @return true if setup succeeded
     */
    virtual bool setup(Engine* engine) = 0;

    /**
     * Remove all environment objects from the scene
     * @param engine  Engine to remove objects from
     */
    virtual void cleanup(Engine* engine) = 0;

    /**
     * Update the environment (animations, effects)
     * @param deltaTime  Time since last frame
     */
    virtual void update(float deltaTime) = 0;

    /**
     * Get the environment name
     */
    virtual const char* getName() const = 0;

    /**
     * Get the environment description
     */
    virtual const char* getDescription() const = 0;

protected:
    /**
     * Track created scene objects for cleanup
     */
    std::vector<std::string> m_objectNames;

    /**
     * Helper to add a scene object and track its name
     */
    void addTrackedObject(Engine* engine, const SceneObject& obj);

    /**
     * Helper to remove all tracked objects
     */
    void removeAllTrackedObjects(Engine* engine);
};

// =============================================================================
// Environment Type Enumeration
// =============================================================================

enum class EnvironmentType {
    OCEAN_PLATFORM,
    DOJO,
    HYPERSPACE
};

/**
 * Convert environment type to string
 */
inline const char* environmentTypeToString(EnvironmentType type) {
    switch (type) {
        case EnvironmentType::OCEAN_PLATFORM: return "ocean";
        case EnvironmentType::DOJO:           return "dojo";
        case EnvironmentType::HYPERSPACE:     return "hyperspace";
        default:                              return "unknown";
    }
}

/**
 * Parse environment type from string
 */
inline EnvironmentType parseEnvironmentType(const std::string& str) {
    if (str == "ocean" || str == "ocean_platform") return EnvironmentType::OCEAN_PLATFORM;
    if (str == "dojo" || str == "kungfu")          return EnvironmentType::DOJO;
    if (str == "hyperspace" || str == "space")     return EnvironmentType::HYPERSPACE;
    return EnvironmentType::DOJO;  // Default
}

/**
 * Factory function to create environment by type
 */
std::unique_ptr<Environment> createEnvironment(EnvironmentType type);

} // namespace lst
