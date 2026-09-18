/**
 * @file rpc_service_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "plugins/rpc_plugin_interface.h"
#include <mutex>
#include <unordered_map>

namespace themis {
namespace plugins {
namespace rpc {

class RPCServiceRegistryImpl {
public:
    /**
     * @brief Instance.
     * @return Return value.
     * @details Implements instance without additional internal calls.
     */
    static RPCServiceRegistryImpl& instance() {
        static RPCServiceRegistryImpl inst;
        return inst;
    }
    
    /**
     * @brief Register Service.
     * @param[in] name Input parameter.
     * @param[in,out] impl Input/output parameter.
     * @details Calls: lock().
     */
    void registerService(const std::string& name, void* impl) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_[name] = impl;
    }
    
    /**
     * @brief Get Service.
     * @param[in] name Input parameter.
     * @return Pointer to the result.
     * @details Calls: lock(), find(), end().
     */
    void* getService(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = services_.find(name);
        return (it != services_.end()) ? it->second : nullptr;
    }
    
    /**
     * @brief Unregister Service.
     * @param[in] name Input parameter.
     * @details Calls: lock(), erase().
     */
    void unregisterService(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_.erase(name);
    }
    
private:
    std::unordered_map<std::string, void*> services_;
    std::mutex mutex_;
};

// Static method implementations
/**
 * @brief Register Service.
 * @param[in] name Input parameter.
 * @param[in,out] impl Input/output parameter.
 * @details Calls: RPCServiceRegistryImpl::instance().
 */
void RPCServiceRegistry::registerService(const std::string& name, void* impl) {
    RPCServiceRegistryImpl::instance().registerService(name, impl);
}

/**
 * @brief Get Service.
 * @param[in] name Input parameter.
 * @return Pointer to the result.
 * @details Calls: RPCServiceRegistryImpl::instance().
 */
void* RPCServiceRegistry::getService(const std::string& name) {
    return RPCServiceRegistryImpl::instance().getService(name);
}

/**
 * @brief Unregister Service.
 * @param[in] name Input parameter.
 * @details Calls: RPCServiceRegistryImpl::instance().
 */
void RPCServiceRegistry::unregisterService(const std::string& name) {
    RPCServiceRegistryImpl::instance().unregisterService(name);
}

/**
 * @brief Instance.
 * @return Return value.
 * @details Implements instance without additional internal calls.
 */
RPCServiceRegistry& RPCServiceRegistry::instance() {
    static RPCServiceRegistry inst;
    return inst;
}

} // namespace rpc
} // namespace plugins
} // namespace themis
