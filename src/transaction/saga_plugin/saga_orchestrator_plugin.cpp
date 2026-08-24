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
#include <new>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#define THEMIS_SAGA_PLUGIN_EXPORT __declspec(dllexport)
#else
#define THEMIS_SAGA_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

namespace themis::transaction {

/**
 * @brief RAII wrapper for SAGAOrchestrator lifecycle management.
 * 
 * Ensures proper creation and cleanup of SAGAOrchestrator instances,
 * preventing use-after-free and resource leaks on exception.
 */
class SAGAOrchestratorGuard {
public:
    /**
     * @brief Create orchestrator with given configuration.
     * @throws std::bad_alloc or any exception from orchestrator initialization
     */
    explicit SAGAOrchestratorGuard(const SAGAOrchestrator::Config& config = {})
        : orchestrator_(nullptr)
        , lifetime_count_(0)
    {
        try {
            orchestrator_ = std::make_unique<SAGAOrchestrator>(config);
            lifetime_count_.store(1, std::memory_order_release);
        } catch (...) {
            // Ensure cleanup on exception
            orchestrator_.reset();
            lifetime_count_.store(0, std::memory_order_release);
            throw;
        }
    }

    /**
     * @brief Destructor waits for pending operations and cleans up.
     */
    ~SAGAOrchestratorGuard() noexcept {
        try {
            if (orchestrator_) {
                // Wait for any pending operations to complete
                // This gives in-flight transactions time to finish gracefully
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                orchestrator_.reset();
            }
        } catch (...) {
            // Suppress exceptions in destructor
        }
        lifetime_count_.store(0, std::memory_order_release);
    }

    // Prevent copying
    SAGAOrchestratorGuard(const SAGAOrchestratorGuard&) = delete;
    SAGAOrchestratorGuard& operator=(const SAGAOrchestratorGuard&) = delete;

    // Prevent moving (ensures lifetime is tied to the plugin instance)
    SAGAOrchestratorGuard(SAGAOrchestratorGuard&&) = delete;
    SAGAOrchestratorGuard& operator=(SAGAOrchestratorGuard&&) = delete;

    /**
     * @brief Get the orchestrator instance.
     * @return Pointer to orchestrator, or nullptr if not initialized
     */
    SAGAOrchestrator* get() noexcept {
        return orchestrator_.get();
    }

    /**
     * @brief Get the orchestrator instance (const).
     */
    const SAGAOrchestrator* get() const noexcept {
        return orchestrator_.get();
    }

    /**
     * @brief Check if orchestrator is valid.
     */
    bool valid() const noexcept {
        return orchestrator_ != nullptr && lifetime_count_.load(std::memory_order_acquire) > 0;
    }

    /**
     * @brief Replace the orchestrator with a new one.
     * @throws std::bad_alloc or any exception from orchestrator initialization
     */
    void reset(const SAGAOrchestrator::Config& config = {}) {
        try {
            auto new_orchestrator = std::make_unique<SAGAOrchestrator>(config);
            orchestrator_ = std::move(new_orchestrator);
            lifetime_count_.store(1, std::memory_order_release);
        } catch (...) {
            orchestrator_.reset();
            lifetime_count_.store(0, std::memory_order_release);
            throw;
        }
    }

private:
    std::unique_ptr<SAGAOrchestrator> orchestrator_;
    std::atomic<int> lifetime_count_;
};

/** @brief Saga orchestrator plugin. */
class SagaOrchestratorPlugin final : public plugins::IThemisPlugin {
public:
    SagaOrchestratorPlugin()
        : guard_(SAGAOrchestrator::Config{}) {}

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

        try {
            guard_.reset(config);
            return true;
        } catch (...) {
            return false;
        }
    }

    void shutdown() override {
        try {
            guard_.reset(SAGAOrchestrator::Config{});
        } catch (...) {
            // Suppress exceptions on shutdown
        }
    }

    void* getInstance() override {
        return guard_.get();
    }

private:
    SAGAOrchestratorGuard guard_;
};

} // namespace themis::transaction

/**
 * @brief Plugin factory function - C interface for dynamic loading.
 * 
 * @note MEMORY OWNERSHIP:
 * - Caller is responsible for calling destroyPlugin() to free returned pointer
 * - Returns nullptr on failure (C-ABI safe error reporting)
 * - Exception safety: no exceptions escape this function
 * 
 * @return Pointer to newly allocated SagaOrchestratorPlugin instance; nullptr on failure
 */
extern "C" THEMIS_SAGA_PLUGIN_EXPORT themis::plugins::IThemisPlugin* createPlugin() noexcept {
    // Keep C ABI boundary strictly non-throwing.
    auto* plugin = new (std::nothrow) themis::transaction::SagaOrchestratorPlugin();
    return plugin;
}

/**
 * @brief Plugin destroyer function - C interface for dynamic unloading.
 * 
 * @note PRECONDITION: plugin must be a non-null pointer returned from createPlugin()
 * @note POSTCONDITION: plugin pointer is deleted; caller should not use it afterward
 * @note EXCEPTION SAFETY: noexcept; any exceptions are logged and suppressed
 * 
 * @param plugin Pointer to SagaOrchestratorPlugin instance to destroy
 */
extern "C" THEMIS_SAGA_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin* plugin) noexcept {
    if (!plugin) {
        // Silently ignore null pointers - double-delete safety
        // This is safe even if called multiple times with same null
        return;
    }
    
    try {
        // Delete the plugin instance
        delete plugin;
    } catch (const std::exception& e) {
        // Exception in destructor - log and suppress per noexcept contract
        try {
            spdlog::error("Exception during plugin destruction: {}", e.what());
        } catch (...) {
            // Even logging might throw - suppress silently
        }
    } catch (...) {
        // Unknown exception - suppress per noexcept contract
        try {
            spdlog::error("Unknown exception during plugin destruction");
        } catch (...) {
            // Even logging might throw - suppress silently
        }
    }
}

