/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_aql_handler.h                                  ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     137                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/llama_wrapper.h"
#include <string>
#include <memory>
#include <vector>

namespace themis {
namespace aql {

/**
 * @brief Handler for LLM-specific AQL commands
 * 
 * Provides execution logic for all LLM commands in AQL:
 * - LLM INFER: Standard text generation
 * - LLM RAG: Retrieval-augmented generation with vector search
 * - LLM EMBED: Generate embeddings
 * - LLM MODEL: Model management (load, unload, list, ingest)
 * - LLM LORA: LoRA management (load, unload, list)
 * - LLM STATS: Performance statistics
 * - LLM CACHE: Cache management (stats, clear)
 */
class LLMAQLHandler {
public:
    LLMAQLHandler();
    ~LLMAQLHandler();

    // Inference commands
    std::string executeInfer(
        const std::string& prompt,
        const std::string& model_id = "",
        const std::string& lora_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );

    std::string executeRAG(
        const std::string& query,
        const std::string& collection,
        int top_k = 5,
        const std::string& lora_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );

    std::vector<float> executeEmbed(
        const std::string& text,
        const std::string& model_id = ""
    );

    // Model management commands
    void executeModelLoad(const std::string& model_id, const std::string& path);
    void executeModelUnload(const std::string& model_id);
    std::vector<std::string> executeModelList();
    void executeModelIngest(const std::string& model_id, const std::string& blob_urn);

    // LoRA management commands
    void executeLoRALoad(const std::string& lora_id, const std::string& path);
    void executeLoRAUnload(const std::string& lora_id);
    std::vector<std::string> executeLoRAList();

    // Statistics commands
    std::string executeStats();
    std::string executeCacheStats();
    void executeCacheClear();

    // Batch optimization
    struct BatchInferRequest {
        std::string prompt;
        std::string model_id;
        std::string lora_id;
        std::unordered_map<std::string, std::string> options;
    };

    std::vector<std::string> executeBatchInfer(
        const std::vector<BatchInferRequest>& requests
    );

    // Natural Language to AQL Translation
    /**
     * @brief Translate natural language query to AQL
     * @param nl_query Natural language query (e.g., "Find all users in Seattle")
     * @param schema_context Optional database schema context for better translation
     * @return Generated AQL query as string
     * @throws std::runtime_error if translation fails
     */
    std::string translateNLToAQL(
        const std::string& nl_query,
        const std::string& schema_context = ""
    );

    // Conversation/Chat Support
    /**
     * @brief Execute chat interaction with message history
     * @param messages Conversation history
     * @param model_id Optional model identifier
     * @param options Generation options
     * @return Assistant response
     */
    std::string executeChat(
        const std::vector<llm::ChatMessage>& messages,
        const std::string& model_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );

    // =========================================================================
    // Confidence scoring
    // =========================================================================

    /**
     * @brief Score the quality and correctness of a generated AQL query.
     *
     * Asks the LLM to rate the query on a scale from 0.0 to 1.0 and to
     * provide a brief explanation and improvement suggestions.
     *
     * When no LLM model is loaded the method returns a default result with
     * score = -1.0 indicating that scoring is unavailable.
     */
    struct QueryConfidenceScore {
        float                    score;       ///< 0.0 (worst) to 1.0 (best); -1.0 = unavailable
        std::string              explanation; ///< Why this score was assigned
        std::vector<std::string> suggestions; ///< Concrete improvement suggestions
    };

    /**
     * @brief Score a generated AQL query.
     * @param aql_query      The AQL query to evaluate
     * @param original_intent Natural-language intent used to generate the query
     *                        (empty string if unknown)
     * @param schema_context  Optional schema description used during generation
     * @return QueryConfidenceScore (score = -1.0 when LLM unavailable)
     */
    QueryConfidenceScore scoreQueryConfidence(
        const std::string& aql_query,
        const std::string& original_intent  = "",
        const std::string& schema_context   = ""
    );

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
