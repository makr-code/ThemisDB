/**
 * @file rpc_service_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
