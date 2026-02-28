/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_factory.cpp                                ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
        Capability::SHARDING
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
        default:                               return "UNKNOWN";
    }
}

} // namespace chimera
