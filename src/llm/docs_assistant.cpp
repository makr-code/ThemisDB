/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            docs_assistant.cpp                                 ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:34:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     338                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 334ca1434e  2026-03-11  fix: selectAdapterForRequest traffic routing; DocsAssista... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file docs_assistant.cpp
 * @brief Implementation of Documentation Assistant
 */

#include "llm/docs_assistant.h"
#include "llm/embedded_llm.h"
#include "llm/llm_plugin_manager.h"
#include <fstream>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <cctype>

namespace themis::llm {

/**
 * @brief Private implementation details
 */
struct DocsAssistant::Impl {
    DocsAssistantConfig config;
    std::vector<DocumentEntry> documents;
    bool database_loaded = false;
    json database_metadata;
    
    // Simple cache for queries
    std::unordered_map<std::string, DocsQueryResult> cache;
};

DocsAssistant::DocsAssistant(const DocsAssistantConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

DocsAssistant::~DocsAssistant() = default;

bool DocsAssistant::loadDatabase(const std::string& path) {
    std::string db_path = path.empty() ? impl_->config.docs_database_path : path;
    
    std::ifstream file(db_path);
    if (!file.is_open()) {
        // Database file not found - this is not an error, just means no docs assistant
        return false;
    }
    
    try {
        json db_json;
        file >> db_json;
        file.close();
        
        return parseDatabase(db_json);
    } catch (const std::exception& e) {
        return false;
    }
}

bool DocsAssistant::parseDatabase(const json& db_json) {
    try {
        // Extract metadata
        if (db_json.contains("metadata")) {
            impl_->database_metadata = db_json["metadata"];
        }
        
        // Extract documents
        if (!db_json.contains("documents") || !db_json["documents"].is_array()) {
            return false;
        }
        
        impl_->documents.clear();
        for (const auto& doc_json : db_json["documents"]) {
            DocumentEntry doc;
            
            if (doc_json.contains("file_path")) {
                doc.file_path = doc_json["file_path"].get<std::string>();
            }
            
            if (doc_json.contains("file_hash")) {
                doc.file_hash = doc_json["file_hash"].get<std::string>();
            }
            
            if (doc_json.contains("metadata") && doc_json["metadata"].contains("file_name")) {
                doc.file_name = doc_json["metadata"]["file_name"].get<std::string>();
            }
            
            if (doc_json.contains("mime_type")) {
                doc.content_type = doc_json["mime_type"].get<std::string>();
            }
            
            // Extract text content from themis_metadata.vector.text_content
            if (doc_json.contains("themis_metadata")) {
                doc.themis_metadata = doc_json["themis_metadata"];
                
                if (doc.themis_metadata.contains("vector") && 
                    doc.themis_metadata["vector"].contains("text_content")) {
                    doc.text_content = doc.themis_metadata["vector"]["text_content"].get<std::string>();
                    
                    if (doc.themis_metadata["vector"].contains("content_length")) {
                        doc.content_length = doc.themis_metadata["vector"]["content_length"].get<int>();
                    }
                }
            }
            
            if (doc_json.contains("metadata")) {
                doc.metadata = doc_json["metadata"];
            }
            
            impl_->documents.push_back(std::move(doc));
        }
        
        impl_->database_loaded = !impl_->documents.empty();
        return impl_->database_loaded;
        
    } catch (const std::exception& e) {
        impl_->database_loaded = false;
        return false;
    }
}

bool DocsAssistant::isReady() const {
    return impl_->database_loaded;
}

float DocsAssistant::computeRelevance(const DocumentEntry& doc, const std::string& query) const {
    // Simple keyword-based relevance scoring
    // Convert both to lowercase for case-insensitive matching
    std::string query_lower = query;
    std::string content_lower = doc.text_content;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    std::transform(content_lower.begin(), content_lower.end(), content_lower.begin(), ::tolower);
    
    // Split query into words
    std::istringstream iss(query_lower);
    std::vector<std::string> query_words;
    std::string word;
    while (iss >> word) {
        // Remove punctuation
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        if (!word.empty() && word.length() > 2) {  // Ignore very short words
            query_words.push_back(word);
        }
    }
    
    if (query_words.empty()) {
        return 0.0f;
    }
    
    // Count keyword matches
    int matches = 0;
    for (const auto& qword : query_words) {
        if (content_lower.find(qword) != std::string::npos) {
            matches++;
        }
    }
    
    // Compute score (0.0 to 1.0)
    float score = static_cast<float>(matches) / static_cast<float>(query_words.size());
    
    // Boost score if file name contains query words
    std::string filename_lower = doc.file_name;
    std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), ::tolower);
    for (const auto& qword : query_words) {
        if (filename_lower.find(qword) != std::string::npos) {
            score += 0.2f;  // Bonus for filename match
        }
    }
    
    return std::min(score, 1.0f);
}

std::vector<DocumentEntry> DocsAssistant::searchDocs(const std::string& query, int max_results) {
    if (!isReady()) {
        return {};
    }
    
    // Compute relevance scores
    std::vector<DocumentEntry> scored_docs;
    for (auto& doc : impl_->documents) {
        doc.relevance_score = computeRelevance(doc, query);
        if (doc.relevance_score > 0.1f) {  // Threshold for inclusion
            scored_docs.push_back(doc);
        }
    }
    
    // Sort by relevance (descending)
    std::sort(scored_docs.begin(), scored_docs.end(), 
              [](const DocumentEntry& a, const DocumentEntry& b) {
                  return a.relevance_score > b.relevance_score;
              });
    
    // Return top results
    if (scored_docs.size() > static_cast<size_t>(max_results)) {
        scored_docs.resize(max_results);
    }
    
    return scored_docs;
}

std::string DocsAssistant::generateAnswer(const std::string& query, 
                                         const std::vector<DocumentEntry>& context_docs) {
    // Build context from documentation
    std::stringstream context;
    context << "# ThemisDB Documentation Context\n\n";
    
    for (const auto& doc : context_docs) {
        context << "## Document: " << doc.file_name << "\n";
        context << "Relevance: " << (doc.relevance_score * 100.0f) << "%\n\n";
        
        // Include preview of content
        std::string preview = doc.text_content;
        if (preview.length() > static_cast<size_t>(impl_->config.context_preview_length)) {
            preview = preview.substr(0, impl_->config.context_preview_length) + "...";
        }
        context << preview << "\n\n";
        context << "---\n\n";
    }
    
    // Build prompt for LLM
    std::stringstream prompt;
    prompt << "You are a helpful ThemisDB documentation assistant. ";
    prompt << "Answer the user's question based on the provided documentation context. ";
    prompt << "Be concise, accurate, and provide specific references to configuration options or commands when applicable.\n\n";
    prompt << context.str();
    prompt << "\nUser Question: " << query << "\n\n";
    prompt << "Answer:";
    
    // Generate answer using LLM
    try {
#ifdef THEMIS_ENABLE_LLM
        if (themis::llm::EmbeddedLLMManager::instance().isInitialized()) {
            return THEMIS_LLM_GENERATE(prompt.str());
        }
#endif
        return "[LLM not available — initialize EmbeddedLLM to enable answer generation]";
    } catch (const std::exception& e) {
        return "Error generating answer: " + std::string(e.what());
    }
}

DocsQueryResult DocsAssistant::query(const std::string& query) {
    DocsQueryResult result;
    
    if (!isReady()) {
        result.generated_answer = "Documentation database not loaded. Please ensure docs_database.json is available.";
        return result;
    }
    
    // Check cache
    if (impl_->config.enable_caching) {
        auto cache_it = impl_->cache.find(query);
        if (cache_it != impl_->cache.end()) {
            return cache_it->second;
        }
    }
    
    auto search_start = std::chrono::high_resolution_clock::now();
    
    // Search for relevant documents
    result.relevant_docs = searchDocs(query, impl_->config.max_context_docs);
    result.total_docs_searched = impl_->documents.size();
    result.docs_included_in_context = result.relevant_docs.size();
    
    auto search_end = std::chrono::high_resolution_clock::now();
    result.search_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(search_end - search_start);
    
    if (result.relevant_docs.empty()) {
        result.generated_answer = "No relevant documentation found for your query. Please try rephrasing or check the ThemisDB documentation manually.";
        result.confidence_score = 0.0f;
        return result;
    }
    
    // Generate answer using LLM with RAG
    auto gen_start = std::chrono::high_resolution_clock::now();
    result.generated_answer = generateAnswer(query, result.relevant_docs);
    auto gen_end = std::chrono::high_resolution_clock::now();
    result.generation_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(gen_end - gen_start);
    
    // Compute confidence based on relevance scores
    float total_relevance = 0.0f;
    for (const auto& doc : result.relevant_docs) {
        total_relevance += doc.relevance_score;
    }
    result.confidence_score = total_relevance / static_cast<float>(result.relevant_docs.size());
    
    // Cache result
    if (impl_->config.enable_caching) {
        impl_->cache[query] = result;
    }
    
    return result;
}

DocsQueryResult DocsAssistant::getConfigHelp(const std::string& topic) {
    std::string query = "How do I configure " + topic + " in ThemisDB? What are the configuration options and environment variables?";
    return this->query(query);
}

DocsQueryResult DocsAssistant::getTroubleshootingHelp(const std::string& error_description) {
    std::string query = "I'm experiencing this issue with ThemisDB: " + error_description + ". How can I troubleshoot and fix this?";
    return this->query(query);
}

json DocsAssistant::getStats() const {
    json stats;
    stats["database_loaded"] = impl_->database_loaded;
    stats["total_documents"] = impl_->documents.size();
    stats["cache_size"] = impl_->cache.size();
    stats["database_metadata"] = impl_->database_metadata;
    return stats;
}

void DocsAssistant::clearCache() {
    impl_->cache.clear();
}

} // namespace themis::llm
