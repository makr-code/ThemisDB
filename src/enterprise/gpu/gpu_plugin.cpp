// Copyright (c) 2025 ThemisDB Contributors
// GPU acceleration enterprise plugin (stub)
// This file is a placeholder for the GPU acceleration module

#define THEMIS_ENTERPRISE_EXPORTS
#include "enterprise/enterprise_plugin.h"

namespace themis::enterprise::gpu {

class GPUPlugin : public EnterprisePluginBase {
public:
    GPUPlugin() : EnterprisePluginBase(FeatureModule::GPU, "GPU", "1.0.0") {}
    
    PluginResult initialize(const PluginConfig& config) override {
        initialized_ = true;
        return {true, nullptr, 0};
    }
    
    void shutdown() override { initialized_ = false; }
    bool validateLicense(const char*) override { return true; }
    const char* getCapabilities() const override {
        return R"({"features":["cuda","vulkan","hip"]})";
    }
};

} // namespace

extern "C" {
    THEMIS_ENTERPRISE_API themis::enterprise::IEnterprisePlugin* createPlugin() {
        return new themis::enterprise::gpu::GPUPlugin();
    }
    THEMIS_ENTERPRISE_API void destroyPlugin(themis::enterprise::IEnterprisePlugin* p) { delete p; }
    THEMIS_ENTERPRISE_API uint32_t getPluginApiVersion() {
        return themis::enterprise::THEMIS_PLUGIN_API_VERSION;
    }
}
