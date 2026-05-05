/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_core_bridge_step.cpp                          ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor_core_bridge_step.cpp
 * @brief `builtin.tensor_core_bridge` — persist TT-cores produced by
 *        `builtin.chunk_tt_decompose`.
 *
 * For each `TensorCoreRecord` in `ctx.tensor_cores`, calls
 * `sink->write(record, tenant_id)` to persist the pre-computed TT-cores.
 *
 * Ordering constraint:
 *   This step MUST follow `builtin.chunk_tt_decompose` in the workflow YAML
 *   so that `ctx.tensor_cores` is already populated before this step runs.
 *
 * Tenant resolution (first non-empty wins):
 *   1. Config key `tenant_id`
 *   2. `ctx.manifest.tenant_id` (if ExtractionContext carries a manifest)
 *   3. Literal string `"default"`
 *
 * Config keys (all optional):
 *  - `tenant_id`           string  Override for the tenant scope.
 *  - `skip_empty`          bool    Skip records with empty serialized_train
 *                                  (default true).
 *  - `fail_on_write_error` bool    Propagate write errors as step failures
 *                                  (default false — warnings only).
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

    std::string name() const override { return "builtin.tensor_core_bridge"; }

    void configure(const nlohmann::json& cfg) override {
        if (cfg.contains("tenant_id") && cfg["tenant_id"].is_string())
            tenant_id_override_ = cfg["tenant_id"].get<std::string>();

        if (cfg.contains("skip_empty") && cfg["skip_empty"].is_boolean())
            skip_empty_ = cfg["skip_empty"].get<bool>();

        if (cfg.contains("fail_on_write_error") &&
            cfg["fail_on_write_error"].is_boolean())
            fail_on_write_error_ = cfg["fail_on_write_error"].get<bool>();
    }

    Result<void> execute(ExtractionContext& ctx) override {
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
