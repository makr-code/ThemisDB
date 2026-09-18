/**
 * @file docs_assistant.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace themis::llm {

using json = nlohmann::json;

struct DocumentEntry {
    /**
     * @brief Document Entry.
     * @return Return value.
     */
    virtual ~DocumentEntry() = default;
    std::string file_path;
    std::string file_hash;
    std::string file_name;
    std::string content_type;
    std::string text_content;
    int content_length = 0;
    json metadata;
    json themis_metadata;

    // Optional precomputed embedding payload (load-only runtime path)
    std::vector<float> embedding;
    std::vector<int16_t> embedding_q;
    float embedding_scale = 0.0f;
    bool has_embedding = false;
    bool is_quantized_embedding = false;
    
    // Computed at runtime
    float relevance_score = 0.0f;
};

struct DocsAssistantConfig {
    std::string docs_database_path = "data/docs_database.json";
    std::string database_type = "json";  // "json" or "rocksdb"
    bool auto_discover = true;  // Auto-discover docs.db if not explicitly configured
    bool read_only = true;  // Open database in read-only mode (recommended for security)
    int max_context_docs = 5;  // Maximum number of docs to include in RAG context
    int context_preview_length = 1000;  // Characters to include per document
    bool enable_semantic_search = true;
    bool enable_caching = true;
    std::string llm_model_id = "";  // Empty = use default
    
    /**
     * @brief Discover Database.
     * @return True when the operation succeeds.
     * @details Calls: test(), good(), std::filesystem::exists().
     */
    bool discoverDatabase() {
        if (!auto_discover) {
            return false;  // Use explicit configuration
        }
        
        // Search order
        std::vector<std::pair<std::string, std::string>> search_paths = {
            {"data/docs_artifact.json", "json"},
            {"data/docs.db", "rocksdb"},
            {"data/docs_database.json", "json"},
            {"./docs_artifact.json", "json"},
            {"./docs.db", "rocksdb"},
            {"./docs_database.json", "json"},
            {"../data/docs_artifact.json", "json"},
            {"../data/docs.db", "rocksdb"},
            {"../data/docs_database.json", "json"}
        };
        
        for (const auto& [path, type] : search_paths) {
            std::ifstream test(path);
            if (test.good() || std::filesystem::exists(path)) {
                docs_database_path = path;
                database_type = type;
                return true;
            }
        }
        
        return false;  // No database found
    }
};

struct DocsQueryResult {
    /**
     * @brief Docs Query Result.
     * @return Return value.
     */
    virtual ~DocsQueryResult() = default;
    std::vector<DocumentEntry> relevant_docs;
    std::string generated_answer;
    float confidence_score = 0.0f;
    int total_docs_searched = 0;
    int docs_included_in_context = 0;
    std::chrono::milliseconds search_time_ms{0};
    std::chrono::milliseconds generation_time_ms{0};
};

class DocsAssistant {
public:
    explicit DocsAssistant(const DocsAssistantConfig& config = DocsAssistantConfig());
    
    ~DocsAssistant();
    
    // Disable copy
    DocsAssistant(const DocsAssistant&) = delete;
    DocsAssistant& operator=(const DocsAssistant&) = delete;
    
    bool loadDatabase(const std::string& path = "");
    
    /**
     * @brief Is Ready.
     * @return True when the operation succeeds.
     */
    bool isReady() const;
    
    /**
     * @brief Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    DocsQueryResult query(const std::string& query);
    
    std::vector<DocumentEntry> searchDocs(const std::string& query, int max_results = 5);
    
    /**
     * @brief Get Config Help.
     * @param[in] topic Input parameter.
     * @return Return value.
     */
    DocsQueryResult getConfigHelp(const std::string& topic);
    
    /**
     * @brief Get Troubleshooting Help.
     * @param[in] error_description Input parameter.
     * @return Return value.
     */
    DocsQueryResult getTroubleshootingHelp(const std::string& error_description);
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    json getStats() const;
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Parse Database.
     * @param[in] db_json Input parameter.
     * @return True when the operation succeeds.
     */
    bool parseDatabase(const json& db_json);
    
    /**
     * @brief Compute Relevance.
     * @param[in] doc Input parameter.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    float computeRelevance(const DocumentEntry& doc, const std::string& query) const;
    
    /**
     * @brief Generate Answer.
     * @param[in] query Input parameter.
     * @param[in] context_docs Input parameter.
     * @return Return value.
     */
    std::string generateAnswer(const std::string& query, 
                               const std::vector<DocumentEntry>& context_docs);
};

} // namespace themis::llm
