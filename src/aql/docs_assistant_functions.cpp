/**
 * @file docs_assistant_functions.cpp
 * @brief Implementation of AQL documentation assistant functions
 */

#include "aql/docs_assistant_functions.h"
#include "llm/docs_assistant.h"
#include <stdexcept>
#include <sstream>

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
