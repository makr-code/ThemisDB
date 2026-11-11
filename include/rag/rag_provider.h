#pragma once

#include "query/query_engine.h"
#include "rag/rag_candidate.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <variant>

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
 * @brief Per-call cooperative execution limits; no mutable global request state.
 * @note Checked before/after QueryEngine calls and between materialization steps.
 * Does not interrupt a blocking index call. Authorization is enforced by the
 * supplied QueryEngine's collection access checker, not by caller-supplied tags.
 */
struct RetrievalExecutionContext {
    std::shared_ptr<const std::atomic<bool>> cancelled;
    std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::time_point::max();
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
     * @param context Optional cancellation/deadline checked cooperatively.
     * @return Evidence (possibly empty for no matches), or the original query error,
     * invalid-input, cancellation, timeout, or content-budget error. Missing content
     * is counted as rejected, never replaced with an identifier or error message.
     * @note Thread safety follows the borrowed engine and its configured backends.
     * Allocation and backend exceptions propagate to the consumer's error boundary.
     */
    [[nodiscard]] Result<RetrievalResult> retrieve(
        const RetrievalRequest& request,
        const RetrievalExecutionContext& context = {}) const;

private:
    query::QueryEngine& engine_;
};

} // namespace themis::rag
