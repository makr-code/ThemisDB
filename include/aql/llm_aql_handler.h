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

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
