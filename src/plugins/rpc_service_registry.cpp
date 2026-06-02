/*
 * ThemisDB | File: rpc_service_registry.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 69
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * PR History (last 5): #4395 feat(server): MqttClientSer... (2026-03-24) | #3634 feat(plugins): build system... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
