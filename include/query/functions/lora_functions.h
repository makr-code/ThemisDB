/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lora_functions.h                                   ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     324                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "query/functions/function_registry.h"
#include "llm/lora_framework/lora_orchestrator.h"
#include <nlohmann/json.hpp>
#include <memory>

namespace themis {
namespace query {
namespace functions {

using json = nlohmann::json;
using namespace themis::llm::lora;

/**
 * @brief LoRA AQL Functions - Native LoRA Operations in AQL
 * 
 * Provides seven AQL functions for LoRA framework operations:
 * 
 * 1. LORA_TRAIN(adapter_id, base_model, dataset, config) -> object
 *    Train a LoRA adapter on a dataset
 * 
 * 2. LORA_QUERY(model_id, adapter_id, prompt, options) -> string
 *    Execute inference with LoRA adapter
 * 
 * 3. LORA_SIMILAR(adapter_id, k, threshold) -> array<object>
 *    Find similar adapters via vector embeddings
 * 
 * 4. LORA_PATH(start_model, end_model, max_depth) -> array<object>
 *    Find adaptation path through graph
 * 
 * 5. LORA_STATS(adapter_id, metrics) -> object
 *    Get adapter statistics and metrics
 * 
 * 6. LORA_RECOMMEND(query, model_id, task, options) -> object
 *    Recommend best adapter for query/task
 * 
 * 7. LORA_LINEAGE(adapter_id, depth) -> array<object>
 *    Get adapter version history
 */

// ============================================================================
// LORA_TRAIN Function
// ============================================================================

/**
 * @brief Train a LoRA adapter on a dataset
 * 
 * Signature: LORA_TRAIN(adapter_id: string, base_model: string, dataset: object, config: object) -> object
 * 
 * @param adapter_id Unique identifier for the adapter
 * @param base_model Base model name (e.g., "llama-2-7b")
 * @param dataset Training dataset (JSON object)
 * @param config Training configuration (rank, alpha, learning_rate, epochs)
 * @return Training job information (job_id, status, estimated_completion)
 * 
 * Example:
 * ```aql
 * FOR doc IN training_datasets
 *   FILTER doc.task == "documentation_qa"
 *   RETURN LORA_TRAIN(
 *     "themis_help_lora",
 *     "llama-2-7b",
 *     doc,
 *     {rank: 8, alpha: 16, learning_rate: 0.0003, epochs: 3}
 *   )
 * ```
 */
class LoraTrainFunction : public IFunction {
public:
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_QUERY Function
// ============================================================================

/**
 * @brief Execute inference with a LoRA adapter
 * 
 * Signature: LORA_QUERY(model_id: string, adapter_id: string, prompt: string, options: object) -> string
 * 
 * @param model_id Base model identifier
 * @param adapter_id LoRA adapter identifier
 * @param prompt Input prompt
 * @param options Generation options (max_tokens, temperature, top_p)
 * @return Generated text response
 * 
 * Example:
 * ```aql
 * FOR question IN user_questions
 *   FILTER question.category == "documentation"
 *   RETURN {
 *     question: question.text,
 *     answer: LORA_QUERY(
 *       "llama-2-7b",
 *       "themis_help_lora",
 *       question.text,
 *       {max_tokens: 500, temperature: 0.7}
 *     )
 *   }
 * ```
 */
class LoraQueryFunction : public IFunction {
public:
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_SIMILAR Function
// ============================================================================

/**
 * @brief Find similar LoRA adapters based on vector embeddings
 * 
 * Signature: LORA_SIMILAR(adapter_id: string, k: number, threshold: number) -> array<object>
 * 
 * @param adapter_id Source adapter identifier
 * @param k Number of similar adapters to return
 * @param threshold Similarity threshold (0.0 - 1.0)
 * @return Array of similar adapters with scores
 * 
 * Example:
 * ```aql
 * LET similar = LORA_SIMILAR("themis_help_lora", 5, 0.85)
 * FOR adapter IN similar
 *   RETURN {
 *     adapter_id: adapter.adapter_id,
 *     similarity: adapter.score,
 *     task: adapter.task
 *   }
 * ```
 */
class LoraSimilarFunction : public IFunction {
public:
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_PATH Function
// ============================================================================

/**
 * @brief Find adaptation path between models through graph traversal
 * 
 * Signature: LORA_PATH(start_model: string, end_model: string, max_depth: number) -> array<object>
 * 
 * @param start_model Starting model identifier
 * @param end_model Target model identifier
 * @param max_depth Maximum traversal depth
 * @return Adaptation path through graph
 * 
 * Example:
 * ```aql
 * LET path = LORA_PATH("llama-2-7b", "llama-2-13b", 3)
 * FOR step IN path
 *   RETURN {
 *     node: step.node_id,
 *     type: step.node_type,
 *     edge: step.edge_type
 *   }
 * ```
 */
class LoraPathFunction : public IFunction {
public:
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_STATS Function
// ============================================================================

/**
 * @brief Get statistics and metrics for LoRA adapters
 * 
 * Signature: LORA_STATS(adapter_id: string, metrics: array<string>) -> object
 * 
 * @param adapter_id Adapter identifier
 * @param metrics List of metrics to retrieve
 * @return Object with requested metrics
 * 
 * Example:
 * ```aql
 * FOR adapter IN @@adapters
 *   FILTER adapter.status == "ready"
 *   RETURN {
 *     adapter_id: adapter.adapter_id,
 *     stats: LORA_STATS(
 *       adapter.adapter_id,
 *       ["validation_accuracy", "inference_count", "avg_latency"]
 *     )
 *   }
 * ```
 */
class LoraStatsFunction : public IFunction {
public:
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_RECOMMEND Function
// ============================================================================

/**
 * @brief Recommend best LoRA adapter for a query/task
 * 
 * Signature: LORA_RECOMMEND(query: string, model_id: string, task: string, options: object) -> object
 * 
 * @param query Input query or prompt
 * @param model_id Base model identifier
 * @param task Task type (e.g., "documentation_qa")
 * @param options Recommendation options (min_accuracy, max_latency_ms)
 * @return Recommendation with adapter_id, confidence, reason, metrics
 * 
 * Example:
 * ```aql
 * LET recommendation = LORA_RECOMMEND(
 *   "How do I configure replication?",
 *   "llama-2-7b",
 *   "documentation_qa",
 *   {min_accuracy: 0.85, max_latency_ms: 100}
 * )
 * RETURN recommendation
 * ```
 */
class LoraRecommendFunction : public IFunction {
public:
    FunctionSignature signature() const override;
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const override;
};

// ============================================================================
// LORA_LINEAGE Function
// ============================================================================

/**
 * @brief Get complete lineage/versioning history of an adapter
 * 
 * Signature: LORA_LINEAGE(adapter_id: string, depth: number) -> array<object>
 * 
 * @param adapter_id Adapter identifier
 * @param depth Maximum lineage depth to retrieve
 * @return Version history with parent/child relationships
 * 
 * Example:
 * ```aql
 * FOR adapter IN @@adapters
 *   FILTER adapter.adapter_id == "themis_help_lora"
 *   RETURN {
 *     current: adapter,
 *     lineage: LORA_LINEAGE(adapter.adapter_id, 10)
 *   }
 * ```
 */
class LoraLineageFunction : public IFunction {
public:
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
 * @brief Register all LoRA functions with the function registry
 * @param registry Function registry instance
 */
void registerLoRAFunctions(FunctionRegistry& registry);

/**
 * @brief Get or create singleton LoRA orchestrator instance
 * @return Shared pointer to LoRA orchestrator
 */
std::shared_ptr<LoRAOrchestrator> getLoRAOrchestrator();

} // namespace functions
} // namespace query
} // namespace themis
