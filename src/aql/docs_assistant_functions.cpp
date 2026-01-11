/**
 * @file docs_assistant_functions.cpp
 * @brief Implementation of AQL documentation assistant functions
 */

#include "aql/docs_assistant_functions.h"
#include "llm/docs_assistant.h"
#include "llm/embedded_llm.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace themis {
namespace aql {

/**
 * @brief Implementation class for DocsAssistantFunctions
 */
class DocsAssistantFunctions::Impl {
public:
    Impl() {
        // Initialize DocsAssistant with default config
        llm::DocsAssistantConfig config;
        config.auto_discover = true;
        config.read_only = true;
        config.enable_semantic_search = true;
        config.enable_caching = true;
        
        // Try to discover database
        if (config.discoverDatabase()) {
            docs_assistant_ = std::make_unique<llm::DocsAssistant>(config);
            if (!docs_assistant_->loadDatabase()) {
                // Failed to load, but don't throw - just mark as not ready
                docs_assistant_.reset();
            }
        } else {
            // No database found, but don't throw - allow graceful degradation
            docs_assistant_.reset();
        }
    }
    
    llm::DocsAssistant* getAssistant() {
        if (!docs_assistant_) {
            throw std::runtime_error(
                "Documentation database not loaded. Please ensure docs.db is available."
            );
        }
        return docs_assistant_.get();
    }
    
    bool isReady() const {
        return docs_assistant_ && docs_assistant_->isReady();
    }

private:
    std::unique_ptr<llm::DocsAssistant> docs_assistant_;
};

DocsAssistantFunctions::DocsAssistantFunctions()
    : impl_(std::make_unique<Impl>()) {}

DocsAssistantFunctions::~DocsAssistantFunctions() = default;

std::string DocsAssistantFunctions::help(const std::string& query) {
    try {
        auto* assistant = impl_->getAssistant();
        
        // Try LLM-based intent detection first
        std::string intent = detectIntentWithLLM(query);
        
        // If LLM detection failed or returned "unknown", fall back to regex
        if (intent == "unknown" || intent.empty()) {
            intent = detectIntentWithRegex(query);
        }
        
        // Route based on detected intent
        if (intent == "configuration") {
            std::string topic = extractTopicFromQuery(query);
            auto result = assistant->getConfigHelp(topic);
            return result.generated_answer;
        }
        else if (intent == "troubleshooting") {
            auto result = assistant->getTroubleshootingHelp(query);
            return result.generated_answer;
        }
        else if (intent == "search") {
            std::string search_query = extractSearchQuery(query);
            auto docs = assistant->searchDocs(search_query, 5);
            return formatSearchResults(docs);
        }
        else {
            // Default: general RAG query
            auto result = assistant->query(query);
            return result.generated_answer;
        }
        
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("HELP failed: ") + e.what()
        );
    }
}

std::string DocsAssistantFunctions::detectIntentWithLLM(const std::string& query) {
    try {
        // Try to use embedded LLM for intent detection
        llm::EmbeddedLLM llm;
        
        // Create a classification prompt
        std::ostringstream prompt;
        prompt << "Classify the following user query into exactly ONE category:\n"
               << "- configuration: User wants to configure or set up something\n"
               << "- troubleshooting: User has an error, problem, or issue to solve\n"
               << "- search: User wants to find or search for documentation\n"
               << "- general: General question about ThemisDB\n\n"
               << "User query: \"" << query << "\"\n\n"
               << "Respond with ONLY the category name (configuration, troubleshooting, search, or general). "
               << "No explanation, just the single word.";
        
        std::string response = llm.generate(prompt.str(), 20);
        
        // Clean up response (trim whitespace and convert to lowercase)
        response.erase(0, response.find_first_not_of(" \t\n\r"));
        response.erase(response.find_last_not_of(" \t\n\r") + 1);
        std::transform(response.begin(), response.end(), response.begin(), ::tolower);
        
        // Validate response
        if (response == "configuration" || response == "troubleshooting" || 
            response == "search" || response == "general") {
            return response;
        }
        
        // If response is not valid, return unknown
        return "unknown";
        
    } catch (const std::exception&) {
        // LLM not available or failed, return unknown to trigger fallback
        return "unknown";
    }
}

std::string DocsAssistantFunctions::detectIntentWithRegex(const std::string& query) {
    // Fallback: regex-based intent detection
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    // Check for configuration intent
    if (query_lower.find("config") != std::string::npos ||
        query_lower.find("configure") != std::string::npos ||
        query_lower.find("setting") != std::string::npos ||
        query_lower.find("setup") != std::string::npos) {
        return "configuration";
    }
    
    // Check for troubleshooting intent
    if (query_lower.find("error") != std::string::npos ||
        query_lower.find("fail") != std::string::npos ||
        query_lower.find("problem") != std::string::npos ||
        query_lower.find("issue") != std::string::npos ||
        query_lower.find("hang") != std::string::npos ||
        query_lower.find("crash") != std::string::npos ||
        query_lower.find("not work") != std::string::npos ||
        query_lower.find("doesn't work") != std::string::npos) {
        return "troubleshooting";
    }
    
    // Check for search intent
    if (query_lower.find("search") != std::string::npos ||
        query_lower.find("find") != std::string::npos ||
        query_lower.find("look for") != std::string::npos ||
        query_lower.find("documentation about") != std::string::npos) {
        return "search";
    }
    
    // Default: general
    return "general";
}

std::string DocsAssistantFunctions::extractTopicFromQuery(const std::string& query) {
    // Extract topic from query (simple heuristic: look for key topics)
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    if (query_lower.find("security") != std::string::npos) return "security";
    if (query_lower.find("shard") != std::string::npos) return "sharding";
    if (query_lower.find("replica") != std::string::npos) return "replication";
    if (query_lower.find("cache") != std::string::npos) return "caching";
    if (query_lower.find("network") != std::string::npos) return "networking";
    if (query_lower.find("storage") != std::string::npos) return "storage";
    
    return "general";
}

std::string DocsAssistantFunctions::extractSearchQuery(const std::string& query) {
    // Extract actual search query (remove "search for", "find", etc.)
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    std::string search_query = query;
    size_t pos;
    
    if ((pos = query_lower.find("search for")) != std::string::npos) {
        search_query = query.substr(pos + 11);
    } else if ((pos = query_lower.find("find")) != std::string::npos) {
        search_query = query.substr(pos + 5);
    } else if ((pos = query_lower.find("look for")) != std::string::npos) {
        search_query = query.substr(pos + 9);
    }
    
    // Trim whitespace
    search_query.erase(0, search_query.find_first_not_of(" \t\n\r"));
    search_query.erase(search_query.find_last_not_of(" \t\n\r") + 1);
    
    return search_query;
}

std::string DocsAssistantFunctions::formatSearchResults(const std::vector<llm::DocumentEntry>& docs) {
    // Format search results as text
    std::ostringstream result;
    result << "Found " << docs.size() << " relevant documents:\n\n";
    
    for (size_t i = 0; i < docs.size(); ++i) {
        result << (i + 1) << ". " << docs[i].file_name 
               << " (relevance: " << std::fixed << std::setprecision(2) 
               << (docs[i].relevance_score * 100) << "%)\n";
        result << "   " << docs[i].file_path << "\n";
        
        std::string preview = docs[i].text_content;
        if (preview.length() > 150) {
            preview = preview.substr(0, 150) + "...";
        }
        result << "   " << preview << "\n\n";
    }
    
    return result.str();
}

std::string DocsAssistantFunctions::docsQuery(const std::string& query) {
    try {
        auto* assistant = impl_->getAssistant();
        auto result = assistant->query(query);
        return result.generated_answer;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("DOCS_QUERY failed: ") + e.what()
        );
    }
}

json DocsAssistantFunctions::docsSearch(const std::string& query, int limit) {
    try {
        auto* assistant = impl_->getAssistant();
        auto docs = assistant->searchDocs(query, limit);
        
        // Convert to JSON array
        json results = json::array();
        for (const auto& doc : docs) {
            json doc_json;
            doc_json["file_name"] = doc.file_name;
            doc_json["file_path"] = doc.file_path;
            doc_json["relevance_score"] = doc.relevance_score;
            doc_json["content_type"] = doc.content_type;
            
            // Add preview of content (first 200 chars)
            std::string preview = doc.text_content;
            if (preview.length() > 200) {
                preview = preview.substr(0, 200) + "...";
            }
            doc_json["content_preview"] = preview;
            
            // Add metadata if available
            if (!doc.metadata.empty()) {
                doc_json["metadata"] = doc.metadata;
            }
            
            results.push_back(doc_json);
        }
        
        return results;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("DOCS_SEARCH failed: ") + e.what()
        );
    }
}

std::string DocsAssistantFunctions::docsConfigHelp(const std::string& topic) {
    try {
        auto* assistant = impl_->getAssistant();
        auto result = assistant->getConfigHelp(topic);
        return result.generated_answer;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("DOCS_CONFIG_HELP failed: ") + e.what()
        );
    }
}

std::string DocsAssistantFunctions::docsTroubleshoot(const std::string& error_description) {
    try {
        auto* assistant = impl_->getAssistant();
        auto result = assistant->getTroubleshootingHelp(error_description);
        return result.generated_answer;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("DOCS_TROUBLESHOOT failed: ") + e.what()
        );
    }
}

json DocsAssistantFunctions::docsStats() {
    try {
        auto* assistant = impl_->getAssistant();
        return assistant->getStats();
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("DOCS_STATS failed: ") + e.what()
        );
    }
}

bool DocsAssistantFunctions::isReady() const {
    return impl_->isReady();
}

void DocsAssistantFunctions::clearCache() {
    try {
        auto* assistant = impl_->getAssistant();
        assistant->clearCache();
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Clear cache failed: ") + e.what()
        );
    }
}

/**
 * @brief Singleton instance
 */
static DocsAssistantFunctions* g_docs_assistant_functions = nullptr;

DocsAssistantFunctions& getDocsAssistantFunctions() {
    if (!g_docs_assistant_functions) {
        g_docs_assistant_functions = new DocsAssistantFunctions();
    }
    return *g_docs_assistant_functions;
}

} // namespace aql
} // namespace themis
