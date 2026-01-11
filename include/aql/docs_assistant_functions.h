/**
 * @file docs_assistant_functions.h
 * @brief AQL function wrappers for Documentation Assistant
 * 
 * Exposes DocsAssistant functionality as AQL functions:
 * - HELP(query: string) -> string - Unified intelligent helper (RECOMMENDED)
 *   Uses LLM-based intent detection with regex fallback
 * - DOCS_QUERY(query: string) -> string
 * - DOCS_SEARCH(query: string, limit: int) -> array<object>
 * - DOCS_CONFIG_HELP(topic: string) -> string
 * - DOCS_TROUBLESHOOT(error: string) -> string
 * 
 * The HELP() function uses LLM for intelligent intent classification when available,
 * falling back to regex-based pattern matching if LLM is not available.
 * 
 * Supports SSE (Server-Sent Events) for streaming and MCP (Model Context Protocol).
 * 
 * Usage examples:
 * ```sql
 * -- Unified helper (automatically detects intent via LLM)
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
     * @brief Unified intelligent helper function (RECOMMENDED)
     * 
     * Automatically determines the intent from the query using LLM-based
     * classification (when available) with regex fallback, then routes to the
     * appropriate function:
     * - Configuration questions → configuration help
     * - Error/problem descriptions → troubleshooting help
     * - Search requests → document search
     * - General questions → RAG-powered query
     * 
     * Intent Detection Strategy:
     * 1. Try LLM-based intent classification (primary method)
     * 2. Fall back to regex pattern matching if LLM unavailable
     * 
     * Supports SSE (Server-Sent Events) streaming and MCP (Model Context Protocol).
     * 
     * @param query User query or question
     * @return Generated answer, search results, or guidance as string
     * 
     * Examples:
     * - HELP('How do I enable sharding?') → RAG query
     * - HELP('Configure security') → Configuration help
     * - HELP('Server hangs at startup') → Troubleshooting
     * - HELP('Search for RAID documentation') → Document search
     */
    std::string help(const std::string& query);
    
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
     * @brief Clear any cached results
     */
    void clearCache();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Detect user intent using LLM (primary method)
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
