/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            docs_assistant_functions.h                         ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:50:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     349                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3a758b465a  2026-04-12  feat(aql): AQL module enhancements — Features 8, 10, 12, ... ║
    • a7b41e3e73  2026-03-22  feat(docs): refactor DocsAssistantFunctions to use unique... ║
    • f62f9c89c4  2026-03-14  feat(aql): wire detectIntentWithNativeNLP() to IClassifyF... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
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

#include "aql/classify_bridge.h"
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
 *
 * Degraded-mode behaviour: when the documentation database or LoRA adapter
 * cannot be loaded the object degrades gracefully. Use @c isFullyReady() and
 * @c degradedReason() to inspect the current degradation state. In embedded
 * deployments without a docs database this is the expected steady state;
 * affected commands fall back to LLM generation.
 */
class DocsAssistantFunctions {
public:
    /**
     * @brief Reason why the assistant is operating in degraded mode.
     *
     * Check @c degradedReason() to obtain a human-readable string for logging
     * or diagnostics. @c OK means the assistant is fully operational.
     */
    enum class DegradedReason {
        OK,                  ///< All components loaded successfully
        DATABASE_NOT_FOUND,  ///< Documentation database file could not be located
        DATABASE_LOAD_FAILED, ///< Database found but failed to load
        LORA_LOAD_FAILED,    ///< LoRA adapter initialisation failed
    };
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
     * @brief Return true only when all components (database AND LoRA) are loaded.
     *
     * Use this for health-check endpoints that need to distinguish a partially
     * degraded assistant from a fully operational one.
     *
     * @return true iff @c isReady() is true AND the LoRA adapter is available.
     */
    bool isFullyReady() const;

    /**
     * @brief Return a human-readable description of the current degradation state.
     *
     * Returns an empty string when @c degradedReason() == @c DegradedReason::OK.
     *
     * Examples:
     * - "Documentation database not found"
     * - "Documentation database failed to load: <exception message>"
     * - "LoRA adapter failed to load: <exception message>"
     */
    std::string degradedReason() const;
    
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

    /**
     * @brief Inject a classifier for native NLP intent detection.
     *
     * When a non-null @p classifier is provided, detectIntentWithNativeNLP()
     * delegates to it instead of returning "unknown".  Pass nullptr (or omit
     * the call) to revert to the no-op NullClassifyFn behaviour.
     *
     * Ownership is NOT transferred; the caller must keep the object alive for
     * the lifetime of this DocsAssistantFunctions instance.
     *
     * Typical usage:
     * @code
     * static AQLFunctionClassifyBridge bridge;
     * getDocsAssistantFunctions().setClassifier(&bridge);
     * @endcode
     */
    void setClassifier(IClassifyFn* classifier);

protected:
    /**
     * @brief Detect user intent using native NLP (primary method)
     * @param query User query
     * @return Intent: "configuration", "troubleshooting", "search", "general", or "unknown"
     *
     * Delegates to the IClassifyFn set via setClassifier() when non-null.
     * Returns "unknown" (triggering LLM fallback) when no classifier has been
     * injected.
     */
    std::string detectIntentWithNativeNLP(const std::string& query);

private:
    class Impl;
  Impl& ensureImpl();
  Impl* tryGetImpl() const;
  mutable std::unique_ptr<Impl> impl_;

    /// Injected classifier; null means native NLP is not available (NullClassifyFn semantics).
    IClassifyFn* classifier_ = nullptr;

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
