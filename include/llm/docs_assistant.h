/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            docs_assistant.h                                   ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:39:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     238                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file docs_assistant.h
 * @brief Documentation Assistant using LLM and pre-compiled docs database
 * 
 * This component provides LLM-based assistance for ThemisDB configuration
 * and troubleshooting by leveraging a pre-compiled documentation database.
 * 
 * Features:
 * - Load pre-compiled documentation database
 * - Vector-based similarity search for relevant documentation
 * - RAG (Retrieval Augmented Generation) for context-aware answers
 * - Configuration assistance
 * - Troubleshooting support
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace themis::llm {

using json = nlohmann::json;

/**
 * @brief Represents a single documentation document
 */
struct DocumentEntry {
    std::string file_path;
    std::string file_hash;
    std::string file_name;
    std::string content_type;
    std::string text_content;
    int content_length = 0;
    json metadata;
    json themis_metadata;
    
    // Computed at runtime
    float relevance_score = 0.0f;
};

/**
 * @brief Configuration for documentation assistant
 */
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
     * @brief Auto-discover documentation database
     * 
     * Search order:
     * 1. Explicit config: docs_database_path
     * 2. data/docs.db (RocksDB)
     * 3. data/docs_database.json (JSON)
     * 4. ./docs.db (RocksDB in current dir)
     * 5. ./docs_database.json (JSON in current dir)
     */
    bool discoverDatabase() {
        if (!auto_discover) {
            return false;  // Use explicit configuration
        }
        
        // Search order
        std::vector<std::pair<std::string, std::string>> search_paths = {
            {"data/docs.db", "rocksdb"},
            {"data/docs_database.json", "json"},
            {"./docs.db", "rocksdb"},
            {"./docs_database.json", "json"},
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

/**
 * @brief Query result from documentation search
 */
struct DocsQueryResult {
    std::vector<DocumentEntry> relevant_docs;
    std::string generated_answer;
    float confidence_score = 0.0f;
    int total_docs_searched = 0;
    int docs_included_in_context = 0;
    std::chrono::milliseconds search_time_ms{0};
    std::chrono::milliseconds generation_time_ms{0};
};

/**
 * @brief Documentation Assistant for ThemisDB
 * 
 * Provides LLM-powered assistance by searching pre-compiled documentation
 * and generating context-aware answers using RAG.
 */
class DocsAssistant {
public:
    /**
     * @brief Constructor
     * @param config Configuration for the assistant
     */
    explicit DocsAssistant(const DocsAssistantConfig& config = DocsAssistantConfig());
    
    /**
     * @brief Destructor
     */
    ~DocsAssistant();
    
    // Disable copy
    DocsAssistant(const DocsAssistant&) = delete;
    DocsAssistant& operator=(const DocsAssistant&) = delete;
    
    /**
     * @brief Load documentation database
     * @param path Path to the documentation database JSON file
     * @return true if successful, false otherwise
     */
    bool loadDatabase(const std::string& path = "");
    
    /**
     * @brief Check if database is loaded
     * @return true if database is loaded and ready
     */
    bool isReady() const;
    
    /**
     * @brief Query documentation for assistance
     * @param query User query (e.g., "How do I configure sharding?")
     * @return Query result with relevant docs and generated answer
     */
    DocsQueryResult query(const std::string& query);
    
    /**
     * @brief Search documentation without LLM generation
     * @param query Search query
     * @param max_results Maximum number of results to return
     * @return Vector of relevant documents
     */
    std::vector<DocumentEntry> searchDocs(const std::string& query, int max_results = 5);
    
    /**
     * @brief Get configuration assistance for a specific topic
     * @param topic Topic (e.g., "sharding", "replication", "security")
     * @return Query result with configuration guidance
     */
    DocsQueryResult getConfigHelp(const std::string& topic);
    
    /**
     * @brief Get troubleshooting help for an error or issue
     * @param error_description Description of the error or issue
     * @return Query result with troubleshooting guidance
     */
    DocsQueryResult getTroubleshootingHelp(const std::string& error_description);
    
    /**
     * @brief Get database statistics
     * @return JSON object with database statistics
     */
    json getStats() const;
    
    /**
     * @brief Clear any cached results
     */
    void clearCache();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Parse and load documentation database
     */
    bool parseDatabase(const json& db_json);
    
    /**
     * @brief Compute relevance score for a document
     * @param doc Document to score
     * @param query Query string
     * @return Relevance score (0.0 to 1.0)
     */
    float computeRelevance(const DocumentEntry& doc, const std::string& query) const;
    
    /**
     * @brief Generate answer using LLM with RAG
     * @param query User query
     * @param context_docs Relevant documentation for context
     * @return Generated answer
     */
    std::string generateAnswer(const std::string& query, 
                               const std::vector<DocumentEntry>& context_docs);
};

} // namespace themis::llm
