/*
 * ThemisDB | File: rpc_service_registry.cpp | Version: 0.0.47 | Last Modified: 2026-04-15 18:58:58
 * Author: ThemisDB Version Bot | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 68
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=17 | delta=14 | status=divergent
 * External Severity (v3): C=4, H=12, M=1
 * PR: #4395 feat(server): MqttClientService â€” bidirectional MQTT client integ... (2026-03-24T08:49:16Z)
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

void RPCServiceRegistry::un