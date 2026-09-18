/**
 * @file query_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
#include "server/auth_middleware.h"
#include "security/query_masking_policy.h"

#include <memory>
#include <mutex>
#include <string>
#include <optional>
#include <atomic>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class GraphIndexManager;
class LLMInteractionStore;
class SemanticCache;
class FieldEncryption;
class KeyProvider;
class AuthMiddleware;
class StatisticsCollector;

namespace metadata {
class IndexRecommender;
} // namespace metadata

namespace prompt_engineering {
class PromptManager;
}

namespace security {
class PKIKeyProvider;
}

// Config flags forwarded from HttpServer; use booleans to avoid circular include

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class QueryApiHandler {
public:
    QueryApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<FieldEncryption> field_encryption,
        std::shared_ptr<KeyProvider> key_provider,
        std::shared_ptr<SemanticCache> semantic_cache,
        std::shared_ptr<LLMInteractionStore> llm_store,
        std::shared_ptr<themis::prompt_engineering::PromptManager> prompt_manager,
        std::shared_ptr<::themis::AuthMiddleware> auth,
        bool feature_llm_query_enhancement,
        bool feature_llm_store
    );

    /**
     * @brief Handle Query.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleQuery(const http::request<http::string_body>& req);

    /**
     * @brief Handle Query Aql.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleQueryAql(const http::request<http::string_body>& req);

    /**
     * @brief Handle Query Enhanced.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleQueryEnhanced(const http::request<http::string_body>& req);

    /**
     * @brief Handle Query Stream Sse.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleQueryStreamSse(const http::request<http::string_body>& req);

    void setStatisticsCollector(StatisticsCollector* sc) noexcept {
        stats_collector_.store(sc, std::memory_order_release);
    }

    void setIndexRecommender(metadata::IndexRecommender* rec) noexcept {
        index_recommender_.store(rec, std::memory_order_release);
    }

    void setQueryMaskingPolicy(
        std::shared_ptr<security::QueryMaskingPolicy> policy) noexcept {
        std::atomic_store_explicit(&masking_policy_, std::move(policy), std::memory_order_release);
    }

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    std::shared_ptr<KeyProvider> key_provider_;
    std::shared_ptr<SemanticCache> semantic_cache_;
    std::shared_ptr<LLMInteractionStore> llm_store_;
    std::shared_ptr<themis::prompt_engineering::PromptManager> prompt_manager_;
    std::shared_ptr<::themis::AuthMiddleware> auth_;
    bool feature_llm_query_enhancement_{false};
    bool feature_llm_store_{false};
    std::atomic<metadata::IndexRecommender*> index_recommender_{nullptr};    ///< Optional; non-owning
    std::atomic<StatisticsCollector*> stats_collector_{nullptr};   ///< Optional; non-owning
    std::shared_ptr<security::QueryMaskingPolicy> masking_policy_;  ///< Optional PII masking

    // Helper methods
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    // Authorization helper
    /**
     * @brief Require Access.
     * @param[in] req Input parameter.
     * @param[in] permission Input parameter.
     * @param[in] resource_type Input parameter.
     * @param[in] resource_id Identifier of the resource.
     * @return Return value.
     */
    std::optional<http::response<http::string_body>> requireAccess(
        const http::request<http::string_body>& req,
        const std::string& permission,
        const std::string& resource_type,
        const std::string& resource_id);
    
    // Auth context extraction
    struct AuthContext {
        std::string user_id;
        std::vector<std::string> groups;
    };
    /**
     * @brief Extract Auth Context.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    AuthContext extractAuthContext(const http::request<http::string_body>& req);

    /**
     * @brief Apply Masking.
     * @param[in] entities Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    nlohmann::json applyMasking(
        const nlohmann::json& entities,
        const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
