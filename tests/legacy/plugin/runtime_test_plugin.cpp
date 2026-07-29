#include "plugins/plugin_interface.h"

#include <nlohmann/json.hpp>

#include <string>

namespace themis { namespace plugins { 

class RuntimeTestPlugin final : public IThemisPlugin {
public:
    [[nodiscard]] const char* getName() const override {
        return "runtime_test_plugin";
    }

    [[nodiscard]] const char* getVersion() const override {
        return "1.0.0";
    }

    [[nodiscard]] PluginType getType() const override {
        return PluginType::CUSTOM;
    }

    [[nodiscard]] PluginCapabilities getCapabilities() const override {
        PluginCapabilities capabilities;
        capabilities.supports_batching = true;
        capabilities.thread_safe = true;
        return capabilities;
    }

    [[nodiscard]] bool initialize(const char* config_json) override {
        try {
            if (config_json == nullptr) {
                return false;
            }

            const auto config_text = std::string(config_json);
            if (!config_text.empty()) {
                config_ = nlohmann::json::parse(config_text);
            } else {
                config_ = nlohmann::json::object();
            }

            initialized_ = true;
            return true;
        } catch (const std::exception&) {
            initialized_ = false;
            return false;
        }
    }

    void shutdown() override {
        initialized_ = false;
        config_.clear();
    }

    [[nodiscard]] void* getInstance() override {
        return initialized_ ? this : nullptr;
    }

private:
    bool initialized_ = false;
    nlohmann::json config_ = nlohmann::json::object();
};
} } // namespace themis::plugins
extern "C" THEMIS_PLUGIN_EXPORT themis::plugins::IThemisPlugin* createPlugin() {
    return new themis::plugins::RuntimeTestPlugin();
}

extern "C" THEMIS_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {
    delete plugin;
}