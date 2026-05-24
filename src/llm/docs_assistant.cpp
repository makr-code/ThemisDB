/*
 * ThemisDB | File: docs_assistant.cpp | Version: 0.0.47 | Last Modified: 2026-05-18 20:49:49
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 535
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=172 | delta=169 | status=divergent
 * External Severity (v3): C=12, H=118, M=41
 * PR: #314 Add pre-compiled RocksDB documentation database with read-only mode... (2026-03-11T16:55:11Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
#include <unordered_map>
#include <cmath>
#include <limits>

namespace themis::llm {

/**
 * @brief Private implementation details
 */
struct DocsAssistant::Impl {
    DocsAssistantConfig config;
    std::vector<DocumentEntry> documents;
    bool database_loaded = false;
    json database_metadata;
    bool semantic_embedding_compatible = false;
    int embedding_dimension = 0;
    
    // Simple cache for queries
    std::unordered_map<std::string, DocsQueryResult> cache;
};

namespace {

std::vector<std::string> tokenizeLower(const std::string& text) {
    std::vector<std::string> tokens;
    std::string cur;
    for (unsigned char ch : text) {
        if (std::isalnum(ch) || ch == '_' || ch == '-') {
            cur.push_back(static_cast<char>(std::tolower(ch)));
        } else if (!cur.empty()) {
            tokens.push_back(cur);
            cur.clear();
        }
    }
    if (!cur.empty()) {
        tokens.push_back(cur);
    }
    return tokens;
}

uint64_t fnv1a64(const std::string& s) {
    constexpr uint64_t kOffset = 1469598103934665603ULL;
    constexpr uint64_t kPrime = 1099511628211ULL;
    uint64_t hash = kOffset;
    for (unsigned char c : s) {
        hash ^= static_cast<uint64_t>(c);
        hash *= kPrime;
    }
    return hash;
}

std::vector<float> hashEmbedQuery(const std::string& text, int dim) {
    std::vector<float> vec(static_cast<size_t>(dim), 0.0f);
    auto tokens = tokenizeLower(text);
    if (tokens.empty() || dim <= 0) {
        return vec;
    }

    std::unordered_map<std::string, int> freqs;
    for (const auto& t : tokens) {
        freqs[t] += 1;
    }

    for (const auto& kv : freqs) {
        const auto& tok = kv.first;
        const int tf = kv.second;
        const uint64_t h = fnv1a64(tok);
        const int idx = static_cast<int>(h % static_cast<uint64_t>(dim));
        const float sign = ((h >> 8) & 1ULL) ? -1.0f : 1.0f;
        const float weight = 1.0f + std::log(static_cast<float>(std::max(1, tf)));
        vec[static_cast<size_t>(idx)] += sign * weight;
    }

    float n2 = 0.0f;
    for (float v : vec) {
        n2 += v * v;
    }
    if (n2 > 0.0f) {
        const float inv = 1.0f / std::sqrt(n2);
        for (float& v : vec) {
            v *= inv;
        }
    }
    return vec;
}

float cosineDense(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty() || a.size() != b.size()) {
        return 0.0f;
    }
    float dot = 0.0f;
    float an = 0.0f;
    float bn = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        an += a[i] * a[i];
        bn += b[i] * b[i];
    }
    if (an <= 0.0f || bn <= 0.0f) {
        return 0.0f;
    }
    return dot / (std::sqrt(an) * std::sqrt(bn));
}

float cosineQuantized(const std::vector<float>& q, const std::vector<int16_t>& vq, float scale) {
    if (q.empty() || vq.empty() || q.size() != vq.size() || scale <= 0.0f) {
        return 0.0f;
    }
    float dot = 0.0f;
    float qn = 0.0f;
    float vn = 0.0f;
    for (size_t i = 0; i < q.size(); ++i) {
        const float dv = static_cast<float>(vq[i]) * scale;
        dot += q[i] * dv;
        qn += q[i] * q[i];
        vn += dv * dv;
    }
    if (qn <= 0.0f || vn <= 0.0f) {
        return 0.0f;
    }
    return dot / (std::sqrt(qn) * std::sqrt(vn));
}

} // namespace

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
    } catch (...) {
        return false;
    }
}

bool DocsAssistant::parseDatabase(const json& db_json) {
    try {
        // Extract metadata
        if (db_json.contains("metadata")) {
            impl_->database_metadata = db_json["metadata"];
        }

        // New artifact format: prefer precomputed chunks for runtime load-only search
        if (db_json.contains("chunks") && db_json["chunks"].is_array()) {
            impl_->semantic_embedding_compatible = false;
            impl_->embedding_dimension = 0;
            if (db_json.contains("pipeline") && db_json["pipeline"].contains("embedding")) {
                const auto& emb = db_json["pipeline"]["embedding"];
                if (emb.contains("dimension")) {
                    impl_->embedding_dimension = emb["dimension"].get<int>();
                }
                if (emb.contains("backend")) {
                    const std::string backend = emb["backend"].get<std::string>();
                    // Runtime can only compute query embeddings for hash-compatible backend.
                    impl_->semantic_embedding_compatible = (backend.find("hash-fallback") != std::string::npos);
                }
            }

            std::unordered_map<std::string, std::string> doc_id_to_path;
            if (db_json.contains("artifact_documents") && db_json["artifact_documents"].is_array()) {
                for (const auto& d : db_json["artifact_documents"]) {
                    if (d.contains("doc_id") && d.contains("file_path")) {
                        doc_id_to_path[d["doc_id"].get<std::string>()] = d["file_path"].get<std::string>();
                    }
                }
            }

            impl_->documents.clear();
            for (const auto& chunk_json : db_json["chunks"]) {
                DocumentEntry doc;

                std::string doc_id = chunk_json.value("doc_id", "");
                doc.file_path = chunk_json.value("file_path", "");
                if (doc.file_path.empty() && !doc_id.empty()) {
                    auto it = doc_id_to_path.find(doc_id);
                    if (it != doc_id_to_path.end()) {
                        doc.file_path = it->second;
                    }
                }

                doc.file_name = std::filesystem::path(doc.file_path).filename().string();
                if (doc.file_name.empty()) {
                    doc.file_name = chunk_json.value("chunk_id", "chunk");
                }

                doc.content_type = "text/markdown";
                doc.text_content = chunk_json.value("text", "");
                doc.content_length = chunk_json.value("token_count", static_cast<int>(doc.text_content.size()));

                doc.metadata = {
                    {"chunk_id", chunk_json.value("chunk_id", "")},
                    {"chunk_index", chunk_json.value("chunk_index", 0)},
                    {"doc_id", doc_id},
                    {"artifact_source", "chunks"}
                };

                if (chunk_json.contains("embedding")) {
                    doc.themis_metadata["vector"]["embedding"] = chunk_json["embedding"];
                    if (chunk_json["embedding"].is_array()) {
                        for (const auto& x : chunk_json["embedding"]) {
                            doc.embedding.push_back(x.get<float>());
                        }
                        doc.has_embedding = !doc.embedding.empty();
                        doc.is_quantized_embedding = false;
                    }
                }
                if (chunk_json.contains("embedding_q") && chunk_json["embedding_q"].is_array()) {
                    for (const auto& x : chunk_json["embedding_q"]) {
                        doc.embedding_q.push_back(static_cast<int16_t>(x.get<int>()));
                    }
                    doc.embedding_scale = chunk_json.value("embedding_scale", 0.0f);
                    doc.has_embedding = !doc.embedding_q.empty();
                    doc.is_quantized_embedding = true;
                }
                doc.themis_metadata["vector"]["text_content"] = doc.text_content;
                doc.themis_metadata["vector"]["content_length"] = doc.content_length;

                impl_->documents.push_back(std::move(doc));
            }

            impl_->database_loaded = !impl_->documents.empty();
            return impl_->database_loaded;
        }
        
        // Legacy format fallback: extract documents
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
        
    } catch (...) {
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
    const bool use_semantic = impl_->config.enable_semantic_search &&
                              impl_->semantic_embedding_compatible &&
                              impl_->embedding_dimension > 0;
    std::vector<float> query_vec;
    if (use_semantic) {
        query_vec = hashEmbedQuery(query, impl_->embedding_dimension);
    }

    for (auto& doc : impl_->documents) {
        const float keyword_score = computeRelevance(doc, query);
        float semantic_score = 0.0f;

        if (use_semantic && doc.has_embedding) {
            if (doc.is_quantized_embedding) {
                semantic_score = cosineQuantized(query_vec, doc.embedding_q, doc.embedding_scale);
            } else {
                semantic_score = cosineDense(query_vec, doc.embedding);
            }
            // Normalize cosine [-1,1] to [0,1]
            semantic_score = (semantic_score + 1.0f) * 0.5f;
        }

        doc.relevance_score = use_semantic ? (0.75f * semantic_score + 0.25f * keyword_score) : keyword_score;
        if (doc.relevance_score > 0.1f) {  // Threshold for inclusion
            scored_docs.push_back(doc);
        }
    }
    
    // Sort by relevance (descending)
    std::sort(scored_docs.begin(), scored_docs.end(), 
              [](const DocumentEntry& a, const DocumentEntry& b) {
                  return a.relevance_score > b.relevance_score;
              });

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
    const auto saturating_to_int = [](size_t value) {
        const size_t max_int = static_cast<size_t>(std::numeric_limits<int>::max());
        return static_cast<int>(value > max_int ? max_int : value);
    };
    
    // Search for relevant documents
    result.relevant_docs = searchDocs(query, impl_->config.max_context_docs);
    result.total_docs_searched = saturating_to_int(impl_->documents.size());
    result.docs_included_in_context = saturating_to_int(result.relevant_docs.size());
    
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

