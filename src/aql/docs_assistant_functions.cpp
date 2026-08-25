/**
 * @file docs_assistant_functions.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "aql/docs_assistant_functions.h"

#include <algorithm>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

#include "aql/classify_bridge.h"
#include "llm/applications/themis_help_lora.h"
#include "llm/docs_assistant.h"
#include "llm/embedded_llm.h"

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
        config.auto_discover          = true;
        config.read_only              = true;
        config.enable_semantic_search = true;
        config.enable_caching         = true;

        // Try to discover database
        if (config.discoverDatabase()) {
            docs_assistant_ = std::make_unique<llm::DocsAssistant>(config);
            if (!docs_assistant_->loadDatabase()) {
                // Failed to load, but don't throw - just mark as not ready
                spdlog::warn("DocsAssistantFunctions: documentation database failed to load; "
                             "falling back to degraded mode (LLM generation only)");
                docs_assistant_.reset();
                degraded_reason_  = DocsAssistantFunctions::DegradedReason::DATABASE_LOAD_FAILED;
                degraded_message_ = "Documentation database failed to load";
            }
        } else {
            // No database found, but don't throw - allow graceful degradation
            spdlog::warn("DocsAssistantFunctions: no documentation database found; "
                         "falling back to degraded mode (LLM generation only)");
            docs_assistant_.reset();
            degraded_reason_  = DocsAssistantFunctions::DegradedReason::DATABASE_NOT_FOUND;
            degraded_message_ = "Documentation database not found";
        }

        // Try to initialize ThemisHelpLoRA
        try {
            llm::applications::ThemisHelpLoRA::Config lora_config;
            lora_config.adapter_id    = "themis_help_lora";
            lora_config.base_model_id = "llama-2-7b";
            // Note: db and blob_manager would need to be injected in production
            // For now, we'll create without them and let it handle gracefully

            help_lora_      = std::make_unique<llm::applications::ThemisHelpLoRA>(lora_config);
            lora_available_ = true;
            spdlog::info("ThemisHelpLoRA initialized successfully");
        } catch (const std::exception &e) {
            spdlog::warn("ThemisHelpLoRA initialization failed, using base LLM: {}", e.what());
            lora_available_ = false;
            // Only override degraded_reason_ if no prior degradation was recorded
            if (degraded_reason_ == DocsAssistantFunctions::DegradedReason::OK) {
                degraded_reason_  = DocsAssistantFunctions::DegradedReason::LORA_LOAD_FAILED;
                degraded_message_ = std::string("LoRA adapter failed to load: ") + e.what();
            }
        }
    }

    llm::DocsAssistant *getAssistant() {
        if (!docs_assistant_) {
            throw std::runtime_error("Documentation database not loaded. Please ensure docs.db is available.");
        }
        return docs_assistant_.get();
    }

    bool isReady() const {
        return docs_assistant_ && docs_assistant_->isReady();
    }

    bool isFullyReady() const {
        return isReady() && lora_available_ && help_lora_ != nullptr;
    }

    DocsAssistantFunctions::DegradedReason getDegradedReason() const {
        return degraded_reason_;
    }

    const std::string &getDegradedMessage() const {
        return degraded_message_;
    }

    bool isLoRAAvailable() const {
        return lora_available_ && help_lora_ != nullptr;
    }

    llm::applications::ThemisHelpLoRA *getLoRA() {
        if (!help_lora_) {
            return nullptr;
        }
        return help_lora_.get();
    }

  private:
    std::unique_ptr<llm::DocsAssistant> docs_assistant_;
    std::unique_ptr<llm::applications::ThemisHelpLoRA> help_lora_;
    bool lora_available_                                    = false;
    DocsAssistantFunctions::DegradedReason degraded_reason_ = DocsAssistantFunctions::DegradedReason::OK;
    std::string degraded_message_;
};

DocsAssistantFunctions::DocsAssistantFunctions() = default;

DocsAssistantFunctions::~DocsAssistantFunctions() = default;

DocsAssistantFunctions::Impl &DocsAssistantFunctions::ensureImpl() {
    if (!impl_) {
        impl_ = std::make_unique<Impl>();
    }
    return *impl_;
}

DocsAssistantFunctions::Impl *DocsAssistantFunctions::tryGetImpl() const {
    return impl_.get();
}

std::string DocsAssistantFunctions::help(const std::string &query, const std::string &user_id) {
    try {
        auto &impl      = ensureImpl();
        auto *assistant = impl.getAssistant();

        // Try to use LoRA adapter if available
        bool using_lora = false;
        std::string answer;
        auto start_time = std::chrono::high_resolution_clock::now();

        if (impl.isLoRAAvailable()) {
            try {
                auto *lora = impl.getLoRA();
                if (lora) {
                    spdlog::debug("Using ThemisHelpLoRA for query: {}", query);
                    answer     = lora->query(query, user_id);
                    using_lora = true;
                }
            } catch (const std::exception &e) {
                spdlog::warn("LoRA query failed, falling back to base: {}", e.what());
                // Fall through to base implementation
            }
        }

        // If LoRA not available or failed, use standard approach
        if (!using_lora) {
            spdlog::debug("Using base DocsAssistant for query: {}", query);

            // Try three-tier intent detection:
            // 1. Native NLP (if available)
            // 2. LLM-based (if LLM available)
            // 3. Regex fallback
            std::string intent = detectIntentWithNativeNLP(query);

            if (intent == "unknown" || intent.empty()) {
                intent = detectIntentWithLLM(query);
            }

            if (intent == "unknown" || intent.empty()) {
                intent = detectIntentWithRegex(query);
            }

            // Route based on detected intent
            if (intent == "configuration") {
                std::string topic = extractTopicFromQuery(query);
                auto result       = assistant->getConfigHelp(topic);
                answer            = result.generated_answer;
            } else if (intent == "troubleshooting") {
                auto result = assistant->getTroubleshootingHelp(query);
                answer      = result.generated_answer;
            } else if (intent == "search") {
                std::string search_query = extractSearchQuery(query);
                auto docs                = assistant->searchDocs(search_query, 5);
                answer                   = formatSearchResults(docs);
            } else {
                // Default: general RAG query
                auto result = assistant->query(query);
                answer      = result.generated_answer;
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        spdlog::info("HELP() query completed in {}ms using {}", duration.count(), using_lora ? "LoRA" : "base");

        return answer;

    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("HELP failed: ") + e.what());
    }
}

std::string DocsAssistantFunctions::detectIntentWithNativeNLP(const std::string &query) {
    try {
        if (!classifier_) {
            // No classifier injected – fall through to LLM path.
            return "unknown";
        }

        static const std::vector<std::string> categories = {"configuration", "troubleshooting", "search", "general"};

        ClassifyResult result = classifier_->classify(query, categories);

        if (result.category.empty() || result.confidence <= 0.0) {
            return "unknown";
        }

        // Guard against classifiers that return labels outside the expected set.
        const bool valid = std::find(categories.begin(), categories.end(), result.category) != categories.end();
        if (!valid) {
            return "unknown";
        }

        return result.category;

    } catch (...) {
        spdlog::debug("[DocsAssistant] classify via native NLP failed; returning 'unknown'");
        return "unknown";
    }
}

void DocsAssistantFunctions::setClassifier(IClassifyFn *classifier) {
    classifier_ = classifier;
}

std::string DocsAssistantFunctions::detectIntentWithLLM(const std::string &query) {
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
        if (response == "configuration" || response == "troubleshooting" || response == "search"
            || response == "general") {
            return response;
        }

        // If response is not valid, return unknown
        return "unknown";

    } catch (...) {
        spdlog::debug("[DocsAssistant] classify via LLM failed; returning 'unknown' to trigger regex fallback");
        return "unknown";
    }
}

std::string DocsAssistantFunctions::detectIntentWithRegex(const std::string &query) {
    // Fallback: regex-based intent detection
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);

    // Check for configuration intent
    if (query_lower.find("config") != std::string::npos || query_lower.find("configure") != std::string::npos
        || query_lower.find("setting") != std::string::npos || query_lower.find("setup") != std::string::npos) {
        return "configuration";
    }

    // Check for troubleshooting intent
    if (query_lower.find("error") != std::string::npos || query_lower.find("fail") != std::string::npos
        || query_lower.find("problem") != std::string::npos || query_lower.find("issue") != std::string::npos
        || query_lower.find("hang") != std::string::npos || query_lower.find("crash") != std::string::npos
        || query_lower.find("not work") != std::string::npos || query_lower.find("doesn't work") != std::string::npos) {
        return "troubleshooting";
    }

    // Check for search intent
    if (query_lower.find("search") != std::string::npos || query_lower.find("find") != std::string::npos
        || query_lower.find("look for") != std::string::npos
        || query_lower.find("documentation about") != std::string::npos) {
        return "search";
    }

    // Default: general
    return "general";
}

std::string DocsAssistantFunctions::extractTopicFromQuery(const std::string &query) {
    // Extract topic from query (simple heuristic: look for key topics)
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);

    if (query_lower.find("security") != std::string::npos) {
        return "security";
    }
    if (query_lower.find("shard") != std::string::npos) {
        return "sharding";
    }
    if (query_lower.find("replica") != std::string::npos) {
        return "replication";
    }
    if (query_lower.find("cache") != std::string::npos) {
        return "caching";
    }
    if (query_lower.find("network") != std::string::npos) {
        return "networking";
    }
    if (query_lower.find("storage") != std::string::npos) {
        return "storage";
    }

    return "general";
}

std::string DocsAssistantFunctions::extractSearchQuery(const std::string &query) {
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

std::string DocsAssistantFunctions::formatSearchResults(const std::vector<llm::DocumentEntry> &docs) {
    // Format search results as text
    std::ostringstream result;
    result << "Found " << docs.size() << " relevant documents:\n\n";

    for (size_t i = 0; i < docs.size(); ++i) {
        result << (i + 1) << ". " << docs[i].file_name << " (relevance: " << std::fixed << std::setprecision(2)
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

std::string DocsAssistantFunctions::docsQuery(const std::string &query) {
    try {
        auto *assistant = ensureImpl().getAssistant();
        auto result     = assistant->query(query);
        return result.generated_answer;
    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("DOCS_QUERY failed: ") + e.what());
    }
}

json DocsAssistantFunctions::docsSearch(const std::string &query, int limit) {
    try {
        auto *assistant = ensureImpl().getAssistant();
        auto docs       = assistant->searchDocs(query, limit);

        // Convert to JSON array
        json results = json::array();
        for (const auto &doc : docs) {
            json doc_json;
            doc_json["file_name"]       = doc.file_name;
            doc_json["file_path"]       = doc.file_path;
            doc_json["relevance_score"] = doc.relevance_score;
            doc_json["content_type"]    = doc.content_type;

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
    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("DOCS_SEARCH failed: ") + e.what());
    }
}

std::string DocsAssistantFunctions::docsConfigHelp(const std::string &topic) {
    try {
        auto *assistant = ensureImpl().getAssistant();
        auto result     = assistant->getConfigHelp(topic);
        return result.generated_answer;
    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("DOCS_CONFIG_HELP failed: ") + e.what());
    }
}

std::string DocsAssistantFunctions::docsTroubleshoot(const std::string &error_description) {
    try {
        auto *assistant = ensureImpl().getAssistant();
        auto result     = assistant->getTroubleshootingHelp(error_description);
        return result.generated_answer;
    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("DOCS_TROUBLESHOOT failed: ") + e.what());
    }
}

json DocsAssistantFunctions::docsStats() {
    try {
        auto *assistant = ensureImpl().getAssistant();
        return assistant->getStats();
    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("DOCS_STATS failed: ") + e.what());
    }
}

bool DocsAssistantFunctions::isReady() const {
    const auto *impl = tryGetImpl();
    return impl ? impl->isReady() : false;
}

bool DocsAssistantFunctions::isFullyReady() const {
    const auto *impl = tryGetImpl();
    return impl ? impl->isFullyReady() : false;
}

std::string DocsAssistantFunctions::degradedReason() const {
    const auto *impl = tryGetImpl();
    if (!impl) {
        return "Not initialised";
    }
    const auto &msg = impl->getDegradedMessage();
    return msg;
}

void DocsAssistantFunctions::clearCache() {
    try {
        auto &impl      = ensureImpl();
        auto *assistant = impl.getAssistant();
        assistant->clearCache();

        // Also log unload event if LoRA is active
        if (impl.isLoRAAvailable()) {
            spdlog::info("Cache cleared, LoRA adapter remains loaded");
        }
    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("Clear cache failed: ") + e.what());
    }
}

bool DocsAssistantFunctions::isLoRAActive() const {
    auto *impl = tryGetImpl();
    return impl ? impl->isLoRAAvailable() : false;
}

json DocsAssistantFunctions::getPerformanceMetrics() const {
    json metrics;
    auto *impl = tryGetImpl();
    if (!impl) {
        metrics["lora_active"] = false;
        return metrics;
    }

    // Add base assistant metrics
    if (impl->isReady()) {
        try {
            auto *assistant           = impl->getAssistant();
            metrics["base_assistant"] = assistant->getStats();
        } catch (...) {
            spdlog::debug("[DocsAssistant] getStats() on base assistant failed; metrics entry set to null");
            metrics["base_assistant"] = nullptr;
        }
    }

    // Add LoRA metrics if available
    if (impl->isLoRAAvailable()) {
        try {
            auto *lora = impl->getLoRA();
            if (lora) {
                auto lora_metrics = lora->getMetrics();
                metrics["lora"]   = {{"total_queries", lora_metrics.total_queries},
                                     {"successful_queries", lora_metrics.successful_queries},
                                     {"failed_queries", lora_metrics.failed_queries},
                                     {"success_rate", lora_metrics.success_rate},
                                     {"average_latency_ms", lora_metrics.average_latency_ms},
                                     {"cache_hit_rate", lora_metrics.cache_hit_rate}};

                auto feedback_stats      = lora->getFeedbackStats();
                metrics["lora_feedback"] = {{"total_feedback", feedback_stats.total_feedback},
                                            {"positive_feedback", feedback_stats.positive_feedback},
                                            {"negative_feedback", feedback_stats.negative_feedback},
                                            {"positive_ratio", feedback_stats.positive_ratio}};

                metrics["lora_version"] = lora->getVersion();
                metrics["lora_trained"] = lora->isTrained();
            }
        } catch (const std::exception &e) {
            metrics["lora_error"] = e.what();
        }
    }

    metrics["lora_active"] = impl->isLoRAAvailable();

    return metrics;
}

/**
 * @brief Thread-safe singleton instance using Meyer's singleton pattern
 * 
 * This implementation is exception-safe and leak-free:
 * - Static local variable initialization is thread-safe in C++11+
 * - Automatic destruction on program exit
 * - No manual new/delete required
 * 
 * @return Reference to the singleton DocsAssistantFunctions instance
 * @exception None (noexcept) - any exceptions during initialization propagate once
 * @note Strong exception guarantee: instance is fully constructed or not at all
 */
DocsAssistantFunctions &getDocsAssistantFunctions() noexcept {
    // Thread-safe initialization per C++11 §6.7 (local static initialization)
    // The first call constructs the object; all subsequent calls return the same object.
    // Destruction is automatic on program exit via std::atexit().
    static DocsAssistantFunctions instance;
    return instance;
}

} // namespace aql
} // namespace themis

