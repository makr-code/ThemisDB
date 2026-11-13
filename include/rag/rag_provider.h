#pragma once

#include "query/query_engine.h"
#include "rag/rag_candidate.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace themis::rag {

/// @brief Bounded graph traversal using the existing QueryEngine graph contract.
struct GraphRetrievalQuery {
    std::string start_vertex;
    int min_depth = 0;
    int max_depth = 2;
    query::TraversalDirection direction = query::TraversalDirection::OUTBOUND;
    std::string graph_id = "default";
    std::string edge_type;
};

/// @brief Existing query contracts; geo/relational restrictions remain hard predicates.
using RetrievalQuery = std::variant<query::ConjunctiveQuery, query::VectorGeoQuery,
                                    query::ContentGeoQuery, GraphRetrievalQuery>;

/**
 * @brief Endpoint-independent retrieval request, without inference configuration.
 * @note Vector queries require a finite, nonzero embedding supplied by an embedding
 * provider. Limits bound returned evidence, not all work inside QueryEngine.
 */
struct RetrievalRequest {
    RetrievalQuery query;
    std::size_t max_candidates = 10;
    std::size_t max_content_bytes = 1024 * 1024;
};

/**
 * @brief Per-call cooperative limits, trusted retrieval authorization, and tracing.
 * @note Deadline and cancellation are checked around QueryEngine calls and
 * between materialization steps; they do not interrupt a blocking index call.
 * The primary and additional cancellation tokens are observed when present,
 * allowing consumer-local timeout and upstream request signals to remain
 * independent.
 * The optional authorization callback is evaluated for the requested collection
 * before query execution and for graph collections before vertex loading. It
 * must come from trusted request context, never untrusted query/YAML data.
 * A non-empty tenant_id is a trusted tenant claim and restricts materialized
 * evidence to entities with a matching string `tenant_id`; missing or non-string
 * fields are rejected. Vector queries also use it as an index prefilter when the
 * secondary tenant index exists.
 * The predicate should be deterministic and safe for concurrent invocation
 * when callers share a context.
 */
struct RetrievalExecutionContext {
    std::shared_ptr<const std::atomic<bool>> cancelled;
    std::vector<std::shared_ptr<const std::atomic<bool>>> additional_cancellation_tokens;
    std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::time_point::max();
    std::function<bool(const std::string&)> authorize_collection;
    std::string tenant_id;
    /// Validated W3C trace identifiers for downstream inference correlation.
    std::string trace_id;
    std::string span_id;
};

/// @brief Owned evidence plus explicit rejection and result-limit accounting.
struct RetrievalResult {
    std::vector<RAGCandidate> candidates;
    std::size_t rejected_documents = 0;
    bool truncated = false;
};

/**
 * @brief Concrete knowledge provider; never generates text or owns model lifecycle.
 * @note Borrows QueryEngine, which must outlive the provider and have its access
 * policy configured before concurrent use. No extra per-index interfaces or ABI.
 */
class RAGProvider {
public:
    /// @brief Borrow an existing engine; no storage or index ownership is transferred.
    /// @param engine Query engine with the required index capabilities and access policy.
    explicit RAGProvider(query::QueryEngine& engine) : engine_(engine) {}

    /**
     * @brief Execute a typed query and materialize reusable, sourced evidence.
     * @param request Query with 1..1000 candidates and 1..1 MiB content budget.
     * @param context Optional trusted authorization, cancellation, deadline, and
     * tenant scope. Never populate authorization or tenant data from query or YAML
     * input; an empty tenant_id preserves unscoped behavior.
     * @return Evidence (possibly empty for no matches), or the original query error,
     * invalid-input, access-denied, cancellation, timeout, or content-budget error.
     * Missing content is counted as rejected, never replaced with an identifier or
     * error message.
     * @note Thread safety follows the borrowed engine and its configured backends.
     * The optional authorization predicate may be called more than once.
     * Allocation and backend exceptions propagate to the consumer's error boundary.
     */
    [[nodiscard]] Result<RetrievalResult> retrieve(
        const RetrievalRequest& request,
        const RetrievalExecutionContext& context = {}) const;

private:
    query::QueryEngine& engine_;
};

} // namespace themis::rag
