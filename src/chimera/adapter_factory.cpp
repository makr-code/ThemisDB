/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_factory.cpp                                ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     81                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

} // namespace chimera
