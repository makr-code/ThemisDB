/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_factory.cpp                                ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:57:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     207                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • be096cd97  2026-02-28  audit: update stale banner metadata in database_adapter.h... ║
    • 176df4359  2026-02-28  feat(chimera): implement AdapterCapabilityMatrix for cros... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file adapter_factory.cpp
 * @brief Implementation of the AdapterFactory for CHIMERA Suite
 * 
 * @copyright MIT License
 */

#include "chimera/database_adapter.hpp"
#include <mutex>
#include <shared_mutex>
#include <algorithm>

namespace chimera {

// Thread-safe singleton registry
std::map<std::string, AdapterFactory::AdapterCreator>& AdapterFactory::get_registry() {
    static std::map<std::string, AdapterCreator> registry;
    return registry;
}

std::unique_ptr<IDatabaseAdapter> AdapterFactory::create(const std::string& system_name) {
    auto& registry = get_registry();
    auto it = registry.find(system_name);
    if (it != registry.end()) {
        return it->second();
    }
    return nullptr;
}

bool AdapterFactory::register_adapter(const std::string& system_name, AdapterCreator creator) {
    static std::mutex registry_mutex;
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    auto& registry = get_registry();
    auto result = registry.insert({system_name, creator});
    return result.second; // true if inserted, false if already exists
}

std::vector<std::string> AdapterFactory::get_supported_systems() {
    auto& registry = get_registry();
    std::vector<std::string> systems;
    systems.reserve(registry.size());
    for (const auto& pair : registry) {
        systems.push_back(pair.first);
    }
    // Sort alphabetically for vendor-neutrality
    std::sort(systems.begin(), systems.end());
    return systems;
}

bool AdapterFactory::is_supported(const std::string& system_name) {
    auto& registry = get_registry();
    return registry.find(system_name) != registry.end();
}

// Thread-safe singleton capability hints registry
std::map<std::string, std::vector<Capability>>& AdapterFactory::get_capability_hints() {
    static std::map<std::string, std::vector<Capability>> hints;
    return hints;
}

// Shared mutex protecting the capability hints map.
// Used with exclusive ownership during writes (register_adapter overload) and
// shared ownership during reads (create_with_capabilities).
static std::shared_mutex& hints_shared_mutex() {
    static std::shared_mutex mtx;
    return mtx;
}

bool AdapterFactory::register_adapter(const std::string& system_name,
                                       AdapterCreator creator,
                                       const std::vector<Capability>& static_capabilities) {
    // Store capability hints thread-safely under exclusive ownership before
    // delegating to the base overload.  Hints are stored unconditionally so
    // that re-registration can update capabilities even when the creator slot
    // is already taken.
    {
        std::unique_lock<std::shared_mutex> lock(hints_shared_mutex());
        get_capability_hints()[system_name] = static_capabilities;
    }
    return register_adapter(system_name, std::move(creator));
}

std::unique_ptr<IDatabaseAdapter> AdapterFactory::create_with_fallback(
    const std::vector<std::string>& candidates
) {
    for (const auto& name : candidates) {
        if (!is_supported(name)) {
            continue;
        }
        auto adapter = create(name);
        if (adapter) {
            return adapter;
        }
    }
    return nullptr;
}

std::unique_ptr<IDatabaseAdapter> AdapterFactory::create_with_capabilities(
    const std::vector<std::string>& candidates,
    const std::vector<Capability>& required_capabilities
) {
    // Acquire a shared (read) lock for the duration of the lookup so that
    // concurrent registrations cannot race with hint reads.
    std::shared_lock<std::shared_mutex> hints_lock(hints_shared_mutex());
    auto& hints = get_capability_hints();

    for (const auto& name : candidates) {
        if (!is_supported(name)) {
            continue;
        }

        // Fast path: if static capability hints were registered, use them to
        // negotiate without instantiating the adapter, avoiding potentially
        // expensive construction for non-qualifying candidates.
        auto hint_it = hints.find(name);
        if (hint_it != hints.end()) {
            bool meets_requirements = true;
            for (const auto& cap : required_capabilities) {
                if (std::find(hint_it->second.begin(), hint_it->second.end(), cap)
                        == hint_it->second.end()) {
                    meets_requirements = false;
                    break;
                }
            }
            if (!meets_requirements) {
                continue;
            }
        }

        // Create the adapter — either hints confirmed it qualifies, or no
        // hints were registered and we must probe via the live instance.
        auto adapter = create(name);
        if (!adapter) {
            continue;
        }

        // If no static hints, fall back to runtime capability probing.
        if (hint_it == hints.end()) {
            bool meets_requirements = true;
            for (const auto& cap : required_capabilities) {
                if (!adapter->has_capability(cap)) {
                    meets_requirements = false;
                    break;
                }
            }
            if (!meets_requirements) {
                continue;
            }
        }

        return adapter;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// AdapterCapabilityMatrix
// ---------------------------------------------------------------------------

void AdapterCapabilityMatrix::add_entry(
    const std::string& system_name,
    const std::vector<Capability>& capabilities
) {
    CapabilityRow& row = matrix_[system_name];
    // Initialise every known capability to false, then mark supported ones.
    for (const auto& cap : all_capabilities()) {
        row[cap] = false;
    }
    for (const auto& cap : capabilities) {
        row[cap] = true;
    }
}

void AdapterCapabilityMatrix::add_adapter(
    const std::string& system_name,
    const ISystemInfoAdapter& adapter
) {
    add_entry(system_name, adapter.get_capabilities());
}

AdapterCapabilityMatrix AdapterCapabilityMatrix::build_from_factory() {
    AdapterCapabilityMatrix matrix;
    for (const auto& name : AdapterFactory::get_supported_systems()) {
        auto adapter = AdapterFactory::create(name);
        if (adapter) {
            matrix.add_adapter(name, *adapter);
        }
    }
    return matrix;
}

bool AdapterCapabilityMatrix::supports(
    const std::string& system_name,
    Capability cap
) const {
    auto row_it = matrix_.find(system_name);
    if (row_it == matrix_.end()) {
        return false;
    }
    auto cap_it = row_it->second.find(cap);
    return cap_it != row_it->second.end() && cap_it->second;
}

std::vector<std::string> AdapterCapabilityMatrix::adapters_supporting(
    Capability cap
) const {
    std::vector<std::string> result;
    for (const auto& kv : matrix_) {
        auto cap_it = kv.second.find(cap);
        if (cap_it != kv.second.end() && cap_it->second) {
            result.push_back(kv.first);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<Capability> AdapterCapabilityMatrix::capabilities_of(
    const std::string& system_name
) const {
    std::vector<Capability> result;
    auto row_it = matrix_.find(system_name);
    if (row_it == matrix_.end()) {
        return result;
    }
    for (const auto& kv : row_it->second) {
        if (kv.second) {
            result.push_back(kv.first);
        }
    }
    return result;
}

std::vector<std::string> AdapterCapabilityMatrix::system_names() const {
    std::vector<std::string> names;
    names.reserve(matrix_.size());
    for (const auto& kv : matrix_) {
        names.push_back(kv.first);
    }
    // matrix_ is a std::map so keys are already sorted; preserve order.
    return names;
}

std::vector<Capability> AdapterCapabilityMatrix::all_capabilities() {
    return {
        Capability::RELATIONAL_QUERIES,
        Capability::VECTOR_SEARCH,
        Capability::GRAPH_TRAVERSAL,
        Capability::DOCUMENT_STORE,
        Capability::FULL_TEXT_SEARCH,
        Capability::TRANSACTIONS,
        Capability::DISTRIBUTED_QUERIES,
        Capability::GEOSPATIAL_QUERIES,
        Capability::TIME_SERIES,
        Capability::STREAM_PROCESSING,
        Capability::BATCH_OPERATIONS,
        Capability::SECONDARY_INDEXES,
        Capability::MATERIALIZED_VIEWS,
        Capability::REPLICATION,
        Capability::SHARDING,
        Capability::ASYNC_OPERATIONS
    };
}

std::string AdapterCapabilityMatrix::capability_to_string(Capability cap) {
    switch (cap) {
        case Capability::RELATIONAL_QUERIES:   return "RELATIONAL_QUERIES";
        case Capability::VECTOR_SEARCH:        return "VECTOR_SEARCH";
        case Capability::GRAPH_TRAVERSAL:      return "GRAPH_TRAVERSAL";
        case Capability::DOCUMENT_STORE:       return "DOCUMENT_STORE";
        case Capability::FULL_TEXT_SEARCH:     return "FULL_TEXT_SEARCH";
        case Capability::TRANSACTIONS:         return "TRANSACTIONS";
        case Capability::DISTRIBUTED_QUERIES:  return "DISTRIBUTED_QUERIES";
        case Capability::GEOSPATIAL_QUERIES:   return "GEOSPATIAL_QUERIES";
        case Capability::TIME_SERIES:          return "TIME_SERIES";
        case Capability::STREAM_PROCESSING:    return "STREAM_PROCESSING";
        case Capability::BATCH_OPERATIONS:     return "BATCH_OPERATIONS";
        case Capability::SECONDARY_INDEXES:    return "SECONDARY_INDEXES";
        case Capability::MATERIALIZED_VIEWS:   return "MATERIALIZED_VIEWS";
        case Capability::REPLICATION:          return "REPLICATION";
        case Capability::SHARDING:             return "SHARDING";
        case Capability::ASYNC_OPERATIONS:     return "ASYNC_OPERATIONS";
        default:                               return "UNKNOWN";
    }
}

} // namespace chimera
