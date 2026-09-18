/**
 * @file lora_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/functions/function_registry.h"
#include "llm/lora_framework/lora_orchestrator.h"
#include "themis/llm/lora_orchestrator_interface.h"
#include <nlohmann/json.hpp>
#include <memory>

namespace themis {
namespace query {
namespace functions {

using json = nlohmann::json;
using namespace themis::llm::lora;


// ============================================================================
// LORA_TRAIN Function
// ============================================================================

class LoraTrainFunction : public IFunction {
public:
    ~LoraTrainFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_QUERY Function
// ============================================================================

class LoraQueryFunction : public IFunction {
public:
    ~LoraQueryFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_SIMILAR Function
// ============================================================================

class LoraSimilarFunction : public IFunction {
public:
    ~LoraSimilarFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_PATH Function
// ============================================================================

class LoraPathFunction : public IFunction {
public:
    ~LoraPathFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_STATS Function
// ============================================================================

class LoraStatsFunction : public IFunction {
public:
    ~LoraStatsFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_RECOMMEND Function
// ============================================================================

class LoraRecommendFunction : public IFunction {
public:
    ~LoraRecommendFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_LINEAGE Function
// ============================================================================

class LoraLineageFunction : public IFunction {
public:
    ~LoraLineageFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_PROVENANCE Function
// ============================================================================

class LoraProvenanceFunction : public IFunction {
public:
    ~LoraProvenanceFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_AUDIT_LOG Function
// ============================================================================

class LoraAuditLogFunction : public IFunction {
public:
    ~LoraAuditLogFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_SNAPSHOTS Function
// ============================================================================

class LoraSnapshotsFunction : public IFunction {
public:
    ~LoraSnapshotsFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_VERIFY_CHAIN Function
// ============================================================================

class LoraVerifyChainFunction : public IFunction {
public:
    ~LoraVerifyChainFunction() override = default;
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// Registration Functions
// ============================================================================

/**
 * @brief Register Lo RAFunctions.
 * @param[in,out] registry Input/output parameter.
 */
void registerLoRAFunctions(FunctionRegistry& registry);

/**
 * @brief Get Lo RAOrchestrator.
 * @return Return value.
 */
std::shared_ptr<themis::llm::lora::ILoRAOrchestrator> getLoRAOrchestrator();

} // namespace functions
} // namespace query
} // namespace themis
