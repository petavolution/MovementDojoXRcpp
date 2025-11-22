/**
 * Environment.cpp - Environment Base Class Implementation
 *
 * Provides common functionality for all training environments
 * and the factory function to create environments by type.
 */

#include "Environment.h"
#include "OceanPlatformEnvironment.h"
#include "DojoEnvironment.h"
#include "HyperspaceEnvironment.h"
#include "../core/Engine.h"
#include "../core/Logger.h"

namespace lst {

// =============================================================================
// Environment Base Class Helpers
// =============================================================================

void Environment::addTrackedObject(Engine* engine, const SceneObject& obj) {
    if (!engine) {
        LOG_ERROR(LOG_TAG_ENV) << "Cannot add object: Engine is null";
        return;
    }

    if (obj.name.empty()) {
        LOG_WARN(LOG_TAG_ENV) << "Adding object with empty name";
    }

    engine->addSceneObject(obj);
    m_objectNames.push_back(obj.name);

    LOG_DEBUG(LOG_TAG_ENV) << "  Added object: " << obj.name;
}

void Environment::removeAllTrackedObjects(Engine* engine) {
    if (!engine) {
        LOG_ERROR(LOG_TAG_ENV) << "Cannot remove objects: Engine is null";
        return;
    }

    LOG_DEBUG(LOG_TAG_ENV) << "Removing " << m_objectNames.size() << " tracked objects";

    for (const auto& name : m_objectNames) {
        engine->removeSceneObject(name);
        LOG_DEBUG(LOG_TAG_ENV) << "  Removed object: " << name;
    }

    m_objectNames.clear();
}

// =============================================================================
// Environment Factory
// =============================================================================

std::unique_ptr<Environment> createEnvironment(EnvironmentType type) {
    switch (type) {
        case EnvironmentType::OCEAN_PLATFORM:
            LOG_INFO(LOG_TAG_ENV) << "Creating Ocean Platform environment";
            return std::make_unique<OceanPlatformEnvironment>();

        case EnvironmentType::DOJO:
            LOG_INFO(LOG_TAG_ENV) << "Creating Dojo environment";
            return std::make_unique<DojoEnvironment>();

        case EnvironmentType::HYPERSPACE:
            LOG_INFO(LOG_TAG_ENV) << "Creating Hyperspace environment";
            return std::make_unique<HyperspaceEnvironment>();

        default:
            LOG_WARN(LOG_TAG_ENV) << "Unknown environment type, defaulting to Dojo";
            return std::make_unique<DojoEnvironment>();
    }
}

} // namespace lst
