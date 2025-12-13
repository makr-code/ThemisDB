// Copyright (c) 2025 ThemisDB Contributors
// Replication enterprise plugin (stub)

#define THEMIS_ENTERPRISE_EXPORTS
#include "enterprise/enterprise_plugin.h"

namespace themis::enterprise::replication {

class ReplicationPlugin : public EnterprisePluginBase {
public:
    ReplicationPlugin() : EnterprisePluginBase(FeatureModule::REPLICATION, "Replication", "1.0.0") {}
    
    PluginResult initialize(const PluginConfig& config) override {
        initialized_ = true;
        return {true, nullptr, 0};
    }
    
    void shutdown() override { initialized_ = false; }
    bool validateLicense(const char*) override { return true; }
    const char* getCapabilities() const override {
        return R"({"features":["leader-follower","multi-master","crdt"]})";
    }
};

} // namespace

extern "C" {
    THEMIS_ENTERPRISE_API themis::enterprise::IEnterprisePlugin* createPlugin() {
        return new themis::enterprise::replication::ReplicationPlugin();
    }
    THEMIS_ENTERPRISE_API void destroyPlugin(themis::enterprise::IEnterprisePlugin* p) { delete p; }
    THEMIS_ENTERPRISE_API uint32_t getPluginApiVersion() {
        return themis::enterprise::THEMIS_PLUGIN_API_VERSION;
    }
}
