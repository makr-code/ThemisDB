/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rpc_service_registry.cpp                           ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:27:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "plugins/rpc_plugin_interface.h"
#include <mutex>
#include <unordered_map>

namespace themis {
namespace plugins {
namespace rpc {

/**
 * @brief Implementation of RPC Service Registry
 */
class RPCServiceRegistryImpl {
public:
    static RPCServiceRegistryImpl& instance() {
        static RPCServiceRegistryImpl inst;
        return inst;
    }
    
    void registerService(const std::string& name, void* impl) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_[name] = impl;
    }
    
    void* getService(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = services_.find(name);
        return (it != services_.end()) ? it->second : nullptr;
    }
    
    void unregisterService(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_.erase(name);
    }
    
private:
    std::unordered_map<std::string, void*> services_;
    std::mutex mutex_;
};

// Static method implementations
void RPCServiceRegistry::registerService(const std::string& name, void* impl) {
    RPCServiceRegistryImpl::instance().registerService(name, impl);
}

void* RPCServiceRegistry::getService(const std::string& name) {
    return RPCServiceRegistryImpl::instance().getService(name);
}

void RPCServiceRegistry::unregisterService(const std::string& name) {
    RPCServiceRegistryImpl::instance().unregisterService(name);
}

RPCServiceRegistry& RPCServiceRegistry::instance() {
    static RPCServiceRegistry inst;
    return inst;
}

} // namespace rpc
} // namespace plugins
} // namespace themis
