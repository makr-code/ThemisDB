/**
 * @file adapter_registry.cpp
 * @brief Non-template AdapterRegistry method implementations.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 91/100
 * @note Gap Summary: total=1; TODO=0, Stub=1, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready
 */

#include "core/concerns/adapter_registry.h"

#include <iostream>
#include <typeindex>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// count()
// ---------------------------------------------------------------------------

size_t AdapterRegistry::count() const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    return registry_.size();
}

// ---------------------------------------------------------------------------
// hasAdapter()
// ---------------------------------------------------------------------------

bool AdapterRegistry::hasAdapter(std::type_index type) const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    return registry_.find(type) != registry_.end();
}

// ---------------------------------------------------------------------------
// loadFromPlugin() — STUB
//
// STUB/SIMULATION NOTE:
// Purpose:    Placeholder for future plugin-based adapter loading (Issue #1706)
// Activation: NOT active — dlopen/LoadLibrary support requires a platform-
//             specific implementation that has not yet been written.
// Production Delta: Adapters are registered programmatically only; no dynamic
//             library loading occurs at runtime.
// Removal Plan: Replace this stub with plugin_loader.h when Issue #1706 is
//             implemented (Target: Q4 2026).
// ---------------------------------------------------------------------------

bool AdapterRegistry::loadFromPlugin(const std::string& /*path*/,
                                      const std::string& /*adapter_id*/) {
    std::cerr << "[AdapterRegistry] plugin loading not yet implemented (Issue #1706)\n";
    return false;
}

} // namespace concerns
} // namespace core
} // namespace themis
