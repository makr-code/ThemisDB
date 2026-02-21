/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            docs_assistant_functions.h                         ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:32:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     276                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 95771d7b4  2026-01-17  Refactor and enhance various components: - Update Process... ║
    • f14d02f3f  2026-01-11  Integrate themis_help_lora adapter with HELP() function (... ║
    • 97caf08ef  2026-01-11  Add unified HELP() function with three-tier intent detect... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file docs_assistant_functions.h
 * @brief AQL function wrappers for Documentation Assistant with LoRA support
 * 
 * Exposes DocsAssistant functionality as AQL functions:
 * - HELP(query: string) -> string - Unified intelligent helper with LoRA support (RECOMMENDED)
 *   Three-tier intent detection: Native NLP → LLM → Regex fallback
 *   Optionally uses themis_help_lora adapter for enhanced accuracy
 * - DOCS_QUERY(query: string) -> string
 * - DOCS_SEARCH(query: string, limit: int) -> array<object>
 * - DOCS_CONFIG_HELP(topic: string) -> string
 * - DOCS_TROUBLESHOOT(error: string) -> string
 * 
 * The HELP() function uses ThemisDB's native NLP capabilities (CLASSIFY function)
 * as the primary method, with LLM-based classification and regex pattern matching
 * as fallbacks for maximum reliability. When available, it uses the themis_help_lora
 * adapter for improved context-specific assistance.
 * 
 * LoRA Integration:
 * - Dynamically loads themis_help_lora adapter when available
 * - Falls back to base LLM if adapter not available
 * - Tracks performance metrics for LoRA vs base model
 * - Supports adapter caching and hot-swapping
 * 
 * Supports SSE (Server-Sent Events) for streaming and MCP (Model Context Protocol).
 * Can incorporate user feedback for continuous improvement.
 * 
 * Usage examples:
 * ```sql
 * -- Unified helper (automatically uses LoRA if available)
 * SELECT HELP('How do I enable sharding?') AS answer;
 * SELECT HELP('Server hangs at startup') AS solution;
 * SELECT HELP('Configure security settings') AS guide;
 * 
 * -- Explicit function calls (for advanced use)
 * SELECT DOCS_QUERY('How do I enable sharding?') AS answer;
 * SELECT DOCS_SEARCH('RAID configuration', 10) AS relevant_docs;
 * SELECT DOCS_CONFIG_HELP('security') AS config_guide;
 * SELECT DOCS_TROUBLESHOOT('Server hangs at startup') AS solution;
 * ```
 */

// Forward declarations
namespace themis {
namespace llm {
    struct DocumentEntry;
}
}


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace aql {

using json = nlohmann::json;

/**
 * @brief Handler for documentation assistant AQL functions
 * 
 * This class provides a thin wrapper around the DocsAssistant
 * to make it available as AQL functions.
 */
class DocsAssistantFunctions {
public:
    /**
     * @brief Constructor - initializes documentation assistant
     */
    DocsAssistantFunctions();
    
    /**
     * @brief Destructor
     */
    ~DocsAssistantFunctions();
    
    /**
     * @brief Unified intelligent helper function with LoRA support (RECOMMENDED)
     * 
     * Automatically determines the intent from the query using a three-tier
     * detection strategy, then routes to the appropriate function:
     * - Configuration questions → configuration help
     * - Error/problem descriptions → troubleshooting help
     * - Search requests → document search
     * - General questions → RAG-powered query
     * 
     * Intent Detection Strategy (in order of preference):
     * 1. **Native NLP** - Uses ThemisDB's CLASSIFY() function (when available)
     * 2. **LLM-based** - Uses embedded LLM for semantic classification
     * 3. **Regex fallback** - Keyword pattern matching for reliability
     * 
     * LoRA Integration:
     * - When themis_help_lora adapter is available, uses it for enhanced accuracy
     * - Falls back to base LLM if adapter not loaded or unavailable
     * - Tracks performance metrics (latency, accuracy) for both modes
     * - Supports dynamic adapter loading/unloading
     * 
     * Supports SSE (Server-Sent Events) streaming and MCP (Model Context Protocol).
     * Can incorporate user feedback for continuous improvement.
     * 
     * @param query User query or question
     * @param user_id Optional user ID for logging and personalization (default: "anonymous")
     *                Note: This parameter is optional and maintains backward compatibility
     * @return Generated answer, search results, or guidance as string
     * 
     * **Backward Compatibility**: The user_id parameter is optional with a default value,
     * so existing calls like `help("question")` continue to work without modification.
     * 
     * Examples:
     * - HELP('How do I enable sharding?') → RAG query with LoRA
     * - HELP('Configure security') → Configuration help
     * - HELP('Server hangs at startup') → Troubleshooting
     * - HELP('Search for RAID documentation') → Document search
     */
    std::string help(const std::string& query, const std::string& user_id = "anonymous");
    
    /**
     * @brief Query documentation for assistance
     * @param query User query (e.g., "How do I configure sharding?")
     * @return Generated answer as string
     * 
     * Example: DOCS_QUERY('How do I enable sharding?')
     */
    std::string docsQuery(const std::string& query);
    
    /**
     * @brief Search documentation without LLM generation
     * @param query Search query
     * @param limit Maximum number of results (default: 5)
     * @return JSON array of relevant documents
     * 
     * Example: DOCS_SEARCH('RAID configuration', 10)
     * Returns:
     * [
     *   {
     *     "file_name": "raid.md",
     *     "relevance_score": 0.95,
     *     "content_preview": "..."
     *   },
     *   ...
     * ]
     */
    json docsSearch(const std::string& query, int limit = 5);
    
    /**
     * @brief Get configuration assistance for a specific topic
     * @param topic Topic (e.g., "sharding", "replication", "security")
     * @return Configuration guidance as string
     * 
     * Example: DOCS_CONFIG_HELP('security')
     */
    std::string docsConfigHelp(const std::string& topic);
    
    /**
     * @brief Get troubleshooting help for an error or issue
     * @param error_description Description of the error or issue
     * @return Troubleshooting guidance as string
     * 
     * Example: DOCS_TROUBLESHOOT('Server hangs at startup')
     */
    std::string docsTroubleshoot(const std::string& error_description);
    
    /**
     * @brief Get database statistics
     * @return JSON object with database statistics
     * 
     * Example: DOCS_STATS()
     */
    json docsStats();
    
    /**
     * @brief Check if documentation database is loaded
     * @return true if ready, false otherwise
     */
    bool isReady() const;
    
    /**
     * @brief Clear any cached results and unload LoRA adapter if loaded
     */
    void clearCache();
    
    /**
     * @brief Check if LoRA adapter is available and loaded
     * @return true if LoRA adapter is active
     */
    bool isLoRAActive() const;
    
    /**
     * @brief Get performance metrics for LoRA vs base model
     * @return JSON with performance stats
     */
    json getPerformanceMetrics() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Detect user intent using native NLP (primary method)
     * @param query User query
     * @return Intent: "configuration", "troubleshooting", "search", "general", or "unknown"
     * 
     * Uses ThemisDB's native CLASSIFY() function for zero-shot classification.
     * Currently returns "unknown" as placeholder - will be integrated at AQL parser level.
     */
    std::string detectIntentWithNativeNLP(const std::string& query);
    
    /**
     * @brief Detect user intent using LLM (secondary method)
     * @param query User query
     * @return Intent: "configuration", "troubleshooting", "search", "general", or "unknown"
     */
    std::string detectIntentWithLLM(const std::string& query);
    
    /**
     * @brief Detect user intent using regex patterns (fallback method)
     * @param query User query
     * @return Intent: "configuration", "troubleshooting", "search", or "general"
     */
    std::string detectIntentWithRegex(const std::string& query);
    
    /**
     * @brief Extract configuration topic from query
     * @param query User query
     * @return Topic name (e.g., "security", "sharding", etc.)
     */
    std::string extractTopicFromQuery(const std::string& query);
    
    /**
     * @brief Extract search query from user input
     * @param query User query with search keywords
     * @return Cleaned search query
     */
    std::string extractSearchQuery(const std::string& query);
    
    /**
     * @brief Format search results as readable text
     * @param docs Vector of document entries
     * @return Formatted string with search results
     */
    std::string formatSearchResults(const std::vector<llm::DocumentEntry>& docs);
};

/**
 * @brief Get singleton instance of DocsAssistantFunctions
 * 
 * This ensures we only have one instance of the documentation
 * database loaded in memory across all AQL queries.
 */
DocsAssistantFunctions& getDocsAssistantFunctions();

} // namespace aql
} // namespace themis
