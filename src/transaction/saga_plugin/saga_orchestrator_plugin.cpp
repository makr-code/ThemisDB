/**
 * @file saga_orchestrator_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "plugins/plugin_interface.h"
#include "transaction/saga_orchestrator.h"

#include <memory>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#define THEMIS_SAGA_PLUGIN_EXPORT __declspec(dllexport)
#else
#define THEMIS_SAGA_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

namespace themis::transaction {

class SagaOrchestratorPlugin final : public plugins::IThemisPlugin {
public:
    SagaOrchestratorPlugin()
        : orchestrator_(std::make_unique<SAGAOrchestrator>(SAGAOrchestrator::Config{})) {}

    const char* getName() const override {
        return "saga_orchestrator";
    }

    const char* getVersion() const override {
        return "1.0.0";
    }

    plugins::PluginType getType() const override {
        return plugins::PluginType::CUSTOM;
    }

    plugins::PluginCapabilities getCapabilities() const override {
        plugins::PluginCapabilities caps;
        caps.supports_streaming = false;
        caps.supports_batching = false;
        caps.supports_transactions = true;
        caps.thread_safe = true;
        caps.gpu_accelerated = false;
        return caps;
    }

    bool initialize(const char* config_json) override {
        SAGAOrchestrator::Config config{};

        if (config_json && config_json[0] != '\0') {
            try {
                const auto cfg = nlohmann::json::parse(config_json);
                if (cfg.contains("enable_parallel") && cfg["enable_parallel"].is_boolean()) {
                    config.enable_parallel = cfg["enable_parallel"].get<bool>();
                }
                if (cfg.contains("default_timeout_ms") && cfg["default_timeout_ms"].is_number_integer()) {
                    const auto timeout_ms = cfg["default_timeout_ms"].get<int64_t>();
                    if (timeout_ms >= 0) {
                        config.default_timeout = std::chrono::milliseconds(timeout_ms);
                    }
                }
                if (cfg.contains("default_retry_delay_ms") && cfg["default_retry_delay_ms"].is_number_integer()) {
                    const auto retry_ms = cfg["default_retry_delay_ms"].get<int64_t>();
                    if (retry_ms >= 0) {
                        config.default_retry_delay = std::chrono::milliseconds(retry_ms);
                    }
                }
                if (cfg.contains("journal_path") && cfg["journal_path"].is_string()) {
                    config.journal_path = cfg["journal_path"].get<std::string>();
                }
            } catch (const nlohmann::json::exception&) {
                return false;
            } catch (...) {
                return false;
            }
        }

        orchestrator_ = std::make_unique<SAGAOrchestrator>(config);
        return true;
    }

    void shutdown() override {
        orchestrator_ = std::make_unique<SAGAOrchestrator>(SAGAOrchestrator::Config{});
    }

    void* getInstance() override {
        return orchestrator_.get();
    }

private:
    std::unique_ptr<SAGAOrchestrator> orchestrator_;
};

} // namespace themis::transaction

extern "C" THEMIS_SAGA_PLUGIN_EXPORT themis::plugins::IThemisPlugin* createPlugin() {
    return new themis::transaction::SagaOrchestratorPlugin();
}

extern "C" THEMIS_SAGA_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {
    delete plugin;
}

