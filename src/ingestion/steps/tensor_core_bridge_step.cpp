/**
 * @file tensor_core_bridge_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "ingestion/ingestion_step.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/ingestion_sinks.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// TensorCoreBridgeStep
// ─────────────────────────────────────────────────────────────────────────────

class TensorCoreBridgeStep : public IIngestionStep {
public:
    explicit TensorCoreBridgeStep(std::shared_ptr<ITensorCoreBridge> sink)
        : sink_(std::move(sink)) {}

    // IThemisPlugin
    const char* getName() const override { return "builtin.tensor_core_bridge"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool initialize(const char*) override { return true; }
    void shutdown() override {}
    void* getInstance() override { return this; }

    // IIngestionStep
    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
        if (cfg.config.contains("tenant_id") && cfg.config["tenant_id"].is_string()) {
            tenant_id_override_ = cfg.config["tenant_id"].get<std::string>();
        }
        if (cfg.config.contains("skip_empty") && cfg.config["skip_empty"].is_boolean()) {
            skip_empty_ = cfg.config["skip_empty"].get<bool>();
        }
        if (cfg.config.contains("fail_on_write_error") &&
            cfg.config["fail_on_write_error"].is_boolean()) {
            fail_on_write_error_ = cfg.config["fail_on_write_error"].get<bool>();
        }

        if (ctx.tensor_cores.empty()) {
            return {}; // nothing to sink
        }

        // Resolve tenant_id.
        std::string tenant_id = tenant_id_override_;
        if (tenant_id.empty()) {
            tenant_id = "default";
        }

        for (const auto& record : ctx.tensor_cores) {
            if (skip_empty_ && record.serialized_train.empty()) {
                continue;
            }

            // Per-record tenant override: use metadata["tenant_id"] when the
            // global tenant_id is the fallback "default".
            std::string effective_tenant = tenant_id;
            if (effective_tenant == "default") {
                auto it = record.metadata.find("tenant_id");
                if (it != record.metadata.end() && !it->second.empty()) {
                    effective_tenant = it->second;
                }
            }

            auto res = sink_->write(record, effective_tenant);
            if (!res) {
                if (fail_on_write_error_) {
                    return ErrVoid(
                        errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                        "tensor_core_bridge: write failed for chunk_id='" +
                            record.chunk_id + "': " + res.error().message());
                }
                // Non-fatal: log via THEMIS_WARN (no-op if not configured)
                // and continue with the next record.
            }
        }

        return {};
    }

private:
    std::shared_ptr<ITensorCoreBridge> sink_;
    std::string                      tenant_id_override_;
    bool                             skip_empty_           = true;
    bool                             fail_on_write_error_  = false;
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<IIngestionStep> createTensorCoreBridgeStep(
    std::shared_ptr<ITensorCoreBridge> sink)
{
    if (!sink) {
        sink = std::make_shared<InMemoryTensorCoreBridge>();
    }
    return std::make_shared<TensorCoreBridgeStep>(std::move(sink));
}

} // namespace builtin
} // namespace ingestion
} // namespace themis
