/**
 * @file query_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=23, H=30, M=73, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Parallel Query Engine implementation

#define _USE_MATH_DEFINES
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include "query/query_engine.h"
#include "query/query_optimizer.h"
#include "query/query_plan_visualizer.h"
#include "query/aql_parser.h"
#include "query/aql_translator.h"
#include "query/let_evaluator.h"
#include "query/cte_cache.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/spatial_index.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/storage_engine.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "metadata/statistics_collector.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/simd_distance.h"
#include "utils/geo/ewkb.h"
#include "geo/spatial_backend.h"
#include "utils/error_registry.h"
#include <sstream>
#include <cmath>
#include <numbers>
#include <limits>
#include <exception>

#if defined(__has_include)
#if __has_include(<tbb/parallel_invoke.h>)
#define THEMIS_HAS_TBB 1
#include <tbb/parallel_invoke.h>
#include <tbb/task_group.h>
#include <tbb/parallel_sort.h> // v1.1.0: TBB Parallel Sort
#else
#define THEMIS_HAS_TBB 0
#endif
#else
#define THEMIS_HAS_TBB 0
#endif

#include "query/parallel_scan.h"
#include <algorithm>
#include <array>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <chrono>
#include "utils/audit_logger.h"

namespace geo = themis::geo;

#if !THEMIS_HAS_TBB
namespace tbb {
class task_group {
public:
    template <typename F>
    void run(F&& f) {
        if (!cancelled_) {
            tasks_.emplace_back(std::forward<F>(f));
        }
    }

    void wait() {
        for (auto& task : tasks_) {
            if (task) {
                task();
            }
        }
        tasks_.clear();
    }

    void cancel() noexcept {
        cancelled_ = true;
    }

private:
    bool cancelled_ = false;
    std::vector<std::function<void()>> tasks_;
};

template <typename Iter>
void parallel_sort(Iter first, Iter last) {
    std::sort(first, last);
}

template <typename Iter, typename Compare>
void parallel_sort(Iter first, Iter last, Compare comp) {
    std::sort(first, last, comp);
}

template <typename F1, typename F2>
void parallel_invoke(F1&& f1, F2&& f2) {
    if (f1) {
        std::forward<F1>(f1)();
    }
    if (f2) {
        std::forward<F2>(f2)();
    }
}
} // namespace tbb
#endif

namespace themis {
namespace query {

using errors::ErrorCode;  // Make ErrorCode directly accessible

namespace utils {
namespace geo = ::themis::geo;
}

// ── Q1/Q4: Timeout enforcement helper ────────────────────────────────────────
//
// Wraps a tg.wait() call with deadline-aware telemetry.  If the elapsed time
// after tg.wait() returns exceeds `timeout_ms`, a structured JSON audit event
// is emitted and a WARN message is logged.  TBB task_group has no wait_until()
// so cancellation is advisory — already-running morsels complete normally.
//
// Usage:
//   tbbWaitWithTimeout(tg, audit_logger_, query_timeout_ms_,
//                      "phase_name", query_id);
//
static void tbbWaitWithTimeout(
    tbb::task_group&               tg,
    ::themis::utils::AuditLogger*  audit_logger,
    std::chrono::milliseconds      timeout_ms,
    const std::string&             phase,
    const std::string&             query_id = "")
{
    // [WAVE3B-FIX: blocking_no_timeout — query_engine.cpp tbbWaitWithTimeout]
    //
    // Replace the post-fact advisory pattern (tg.wait(); then check elapsed)
    // with a watchdog-thread approach that actively cancels the task group
    // when the deadline fires, so stalled morsels unblock promptly.
    auto done = std::make_shared<std::atomic<bool>>(false);

    std::thread watchdog([done, &tg, timeout_ms]() noexcept {
        const auto deadline = std::chrono::steady_clock::now() + timeout_ms;
        while (!done->load(std::memory_order_acquire)) {
            if (std::chrono::steady_clock::now() >= deadline) {
                if (!done->load(std::memory_order_acquire)) {
					tg.cancel();
                }
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    const auto t0 = std::chrono::high_resolution_clock::now();
    tg.wait();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - t0);

    done->store(true, std::memory_order_release);
    watchdog.join();

    if (elapsed > timeout_ms) {
        THEMIS_WARN("Query phase '{}' exceeded timeout: elapsed={}ms limit={}ms query_id='{}'",
                    phase, elapsed.count(), timeout_ms.count(), query_id);
        if (audit_logger) {
            audit_logger->logEvent({
                {"event",      "query_timeout"},
                {"query_id",   query_id},
                {"phase",      phase},
                {"elapsed_ms", elapsed.count()},
                {"timeout_ms", timeout_ms.count()}
            });
        }
    }
}

/**
 * @brief Legacy constructor with direct RocksDB and secondary-index dependencies.
 * @param db Storage backend reference.
 * @param secIdx Secondary index manager reference.
 */
QueryEngine::QueryEngine(RocksDBWrapper& db, SecondaryIndexManager& secIdx)
	: db_(&db), secIdx_(&secIdx) {}

/**
 * @brief Legacy constructor with graph-index support.
 * @param db Storage backend reference.
 * @param secIdx Secondary index manager reference.
 * @param graphIdx Graph index manager reference.
 */
QueryEngine::QueryEngine(RocksDBWrapper& db, SecondaryIndexManager& secIdx, GraphIndexManager& graphIdx)
	: db_(&db), secIdx_(&secIdx), graphIdx_(&graphIdx) {}

/**
 * @brief Legacy constructor with optional vector/spatial index managers.
 * @param db Storage backend reference.
 * @param secIdx Secondary index manager reference.
 * @param graphIdx Graph index manager reference.
 * @param vectorIdx Optional vector index manager pointer.
 * @param spatialIdx Optional spatial index manager pointer.
 */
QueryEngine::QueryEngine(RocksDBWrapper& db, SecondaryIndexManager& secIdx, GraphIndexManager& graphIdx,
                         VectorIndexManager* vectorIdx, SpatialIndexManager* spatialIdx)
	: db_(&db), secIdx_(&secIdx), graphIdx_(&graphIdx), vectorIdx_(vectorIdx), spatialIdx_(spatialIdx) {}

/**
 * @brief DI constructor using storage/index interfaces.
 * @param storage Storage interface (may be nullptr for late binding).
 * @param index_manager Index manager interface (must not be nullptr).
 * @throws std::invalid_argument if index_manager is nullptr.
 */
QueryEngine::QueryEngine(
    IStorageEnginePtr storage,
    IIndexManagerPtr index_manager
) : storage_(storage), index_manager_(index_manager) {
    // Note: storage can be nullptr for late binding via setStorage()
    if (!index_manager_) {
        throw std::invalid_argument("QueryEngine: index_manager cannot be null");
    }
}

/**
 * @brief Inject storage dependency after construction.
 * @param storage Storage interface instance.
 */
void QueryEngine::setStorage(IStorageEnginePtr storage) {
    storage_ = storage;
}

/**
 * @brief Create expression evaluator bound to this engine instance.
 * @return Shared pointer to query expression evaluator implementation.
 */
IExpressionEvaluatorPtr QueryEngine::get_expression_evaluator() {
    return std::make_shared<QueryExpressionEvaluator>(this);
}

/**
 * @brief Create QueryEngine with default storage and index implementations.
 * @return Shared pointer to a ready-to-use QueryEngine.
 */
std::shared_ptr<QueryEngine> QueryEngine::createDefault() {
    // Create default in-memory storage + no-op index manager using the
    // StorageEngine factory (same pattern as StorageEngine::createDefault()).
    auto storage      = StorageEngine::createDefault();
    auto index_manager = StorageEngine::createDefaultIndexManager();
    return std::make_shared<QueryEngine>(
        std::static_pointer_cast<IStorageEngine>(storage),
        index_manager
    );
}

/**
 * @brief Enumerate known collection names from document and relational key prefixes.
 * @return Sorted unique collection names.
 */
std::vector<std::string> QueryEngine::listCollections() const {
    if (!db_) {
        return {};
    }
    std::unordered_set<std::string> seen;
	static constexpr std::array<std::string_view, 2> kCollectionPrefixes{"doc:", "rel:"};
    // Key schema: "doc:<collection>:<pk>" or "rel:<table>:<pk>"
    // We scan all keys and extract the second segment.
	for (const auto prefix : kCollectionPrefixes) {
        db_->scanPrefix(prefix, [&](std::string_view key, std::string_view /*value*/) {
            // key looks like "doc:name:pk" — extract "name"
            std::string_view remainder = key.substr(prefix.size());
            auto sep = remainder.find(':');
            if (sep != std::string_view::npos) {
                seen.emplace(remainder.substr(0, sep));
            }
            return true;  // continue iteration
        });
    }
    std::vector<std::string> result(seen.begin(), seen.end());
    std::sort(result.begin(), result.end());
    return result;
}

// QueryExpressionEvaluator implementation
// Delegates to the AQL parser + QueryEngine::evaluateCondition().
// The opaque `context` pointer may be either a `QueryEngine::EvaluationContext*`
// (preferred) or a `nlohmann::json*` representing the current document binding
// under the variable name "doc".

static const std::string kEvalDocVar = "doc";

static std::string stableJsonOrderKey(const nlohmann::json& doc) {
	if (doc.contains("_key") && doc["_key"].is_string()) {
		return doc["_key"].get<std::string>();
	}
	return doc.dump();
}

static bool stableJsonLess(const nlohmann::json& a, const nlohmann::json& b) {
	const std::string keyA = stableJsonOrderKey(a);
	const std::string keyB = stableJsonOrderKey(b);
	if (keyA == keyB) {
		return a.dump() < b.dump();
	}
	return keyA < keyB;
}

static bool stableJsonPtrLess(const nlohmann::json* a, const nlohmann::json* b) {
	return stableJsonLess(*a, *b);
}

static void logSortedDeserializeFailures(std::vector<std::string>& failed_pks, const char* context) {
	if (failed_pks.empty()) {
		return;
	}
	std::sort(failed_pks.begin(), failed_pks.end());
	for (const auto& pk : failed_pks) {
		THEMIS_WARN("{}: Deserialisierung fehlgeschlagen für PK={}", context, pk);
	}
}

/// Parse `expression` as an AQL expression and evaluate it against `ctx`.
/// Returns false on parse or evaluation errors.
static bool evalAqlExpression(const std::string& expression,
                               const QueryEngine::EvaluationContext* ctx,
                               const QueryEngine* engine) {
    if (!engine || expression.empty()) {
      return false;
    }
    try {
        query::AQLParser parser;
        auto expr = parser.parseExpression(expression);
        if (!expr) {
          return false;
        }
        if (!ctx) {
          return false;
        }
        return engine->evaluateCondition(expr, *ctx);
    } catch (...) {
        THEMIS_WARN("query_engine::logSortedDeserializeFailures: unhandled exception caught");
        return false;
    }
}

bool QueryEngine::QueryExpressionEvaluator::evaluate(
	const std::string& expression,
	const void* context) const {
    if (!engine_ || expression.empty()) {
      return false;
    }
    // `context` may be a QueryEngine::EvaluationContext* or a nlohmann::json*.
    // We support both: if EvaluationContext*, use it directly; if json*, build
    // a minimal EvaluationContext with the document bound to "doc".
    if (!context) {
      return false;
    }

    // Heuristic: try context as EvaluationContext first (most common caller).
    // Callers that pass a json* must cast it correctly; the layout is opaque.
    const auto* eval_ctx =
        static_cast<const QueryEngine::EvaluationContext*>(context);
    return evalAqlExpression(expression, eval_ctx, engine_);
}

std::string QueryEngine::QueryExpressionEvaluator::get_expression_type() const {
    return "AQL";
}

bool QueryEngine::QueryExpressionEvaluator::evaluateBoolean(
    std::string_view expression,
    const void* context) const {
    if (!engine_ || expression.empty() || !context) {
      return false;
    }
    const auto* eval_ctx =
        static_cast<const QueryEngine::EvaluationContext*>(context);
    return evalAqlExpression(std::string(expression), eval_ctx, engine_);
}

bool QueryEngine::QueryExpressionEvaluator::canEvaluate(std::string_view expression) const {
    if (expression.empty()) {
      return false;
    }
    try {
        query::AQLParser parser;
        auto expr = parser.parseExpression(std::string(expression));
        return expr != nullptr;
    } catch (...) {
        THEMIS_DEBUG("query_engine: unhandled exception caught");
        return false;
    }
}


Result<std::vector<std::string>>
QueryEngine::executeAndKeys(const ConjunctiveQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeAndKeys");
	span.setAttribute("query.table", q.table);
	if (!db_) {
		return Err<std::vector<std::string>>(
			errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			"Storage backend not available"
		);
	}
	span.setAttribute("query.eq_count", static_cast<int64_t>(q.predicates.size()));
	span.setAttribute("query.range_count", static_cast<int64_t>(q.rangePredicates.size()));
	span.setAttribute("query.order_by", q.orderBy.has_value());
	span.setAttribute("query.fulltext", q.fulltextPredicate.has_value());
	span.setAttribute("query.phrase", q.phrasePredicate.has_value());
	span.setAttribute("query.fuzzy", q.fuzzyPredicate.has_value());
	if (q.table.empty()) {
		return Err<std::vector<std::string>>(
			errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
			"Table name cannot be empty"
		);
	}
	// QE-2: enforce collection-level access control before any I/O
	if (collection_access_checker_ && !collection_access_checker_(q.table, collection_access_caller_id_)) {
		return Err<std::vector<std::string>>(
			errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
			"Access denied for collection: " + q.table
		);
	}

	// ── Primary-key fast path ──────────────────────────────────────────────
	// When pk_eq is set the caller already knows the primary key.  Skip all
	// secondary-index lookups and do a single direct storage existence check.
	// This is the minimal-overhead 1:1 OLTP path (hotpath).
	if (q.pk_eq.has_value()) {
		const std::string& pk = *q.pk_eq;
		auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
		span.setAttribute("query.pk_fast_path", true);
		span.setStatus(true);
		if (blob.has_value()) {
			return Ok(std::vector<std::string>{pk});
		}
		return Ok(std::vector<std::string>{});
	}

	// Handle phrase search queries
	if (q.phrasePredicate.has_value()) {
		const auto& ph = *q.phrasePredicate;
		auto child = Tracer::startSpan("index.scanPhrase");
		child.setAttribute("index.table", q.table);
		child.setAttribute("index.column", ph.column);
		child.setAttribute("index.phrase", ph.phrase);
		child.setAttribute("index.limit", static_cast<int64_t>(ph.limit));
		
		auto [st, results] = secIdx_->scanFulltextPhrase(q.table, ph.column, ph.phrase, ph.limit);
		if (!st.ok) {
			child.setStatus(false, st.message);
			return Err<std::vector<std::string>>(
				errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("Phrase search failed: {}", st.message)
			);
		}
		
		// Extract PKs from results
		std::vector<std::string> phraseKeys;
		phraseKeys.reserve(results.size());
		for (const auto& res : results) {
			phraseKeys.emplace_back(res.pk);
		}
		
		child.setAttribute("index.result_count", static_cast<int64_t>(phraseKeys.size()));
		child.setStatus(true);
		
		// If there are additional predicates, intersect
		if (!q.predicates.empty() || !q.rangePredicates.empty()) {
			auto intersectSpan = Tracer::startSpan("query.phrase_and_intersection");
			
			ConjunctiveQuery structuralQuery;
			structuralQuery.table = q.table;
			structuralQuery.predicates = q.predicates;
			structuralQuery.rangePredicates = q.rangePredicates;
			structuralQuery.orderBy = q.orderBy;
			
			auto structResult = executeAndKeysRangeAware_(structuralQuery);
			if (!structResult) {
				intersectSpan.setStatus(false, structResult.error().context());
				return Err<std::vector<std::string>>(structResult.error().code(), structResult.error().context());
			}
			auto structKeys = std::move(*structResult);
			
			tbb::parallel_sort(phraseKeys.begin(), phraseKeys.end());
			tbb::parallel_sort(structKeys.begin(), structKeys.end());
			
			std::vector<std::string> intersection;
			intersection.reserve(std::min(phraseKeys.size(), structKeys.size()));
			std::set_intersection(
				phraseKeys.begin(), phraseKeys.end(),
				structKeys.begin(), structKeys.end(),
				std::back_inserter(intersection)
			);
			
			intersectSpan.setStatus(true);
			span.setStatus(true);
			return Ok(std::move(intersection));
		}
		
		span.setStatus(true);
		return Ok(std::move(phraseKeys));
	}
	
	// Handle fuzzy search queries
	if (q.fuzzyPredicate.has_value()) {
		const auto& fz = *q.fuzzyPredicate;
		auto child = Tracer::startSpan("index.scanFuzzy");
		child.setAttribute("index.table", q.table);
		child.setAttribute("index.column", fz.column);
		child.setAttribute("index.query", fz.query);
		child.setAttribute("index.maxDistance", static_cast<int64_t>(fz.maxDistance));
		child.setAttribute("index.limit", static_cast<int64_t>(fz.limit));
		
		auto [st, results] = secIdx_->scanFulltextFuzzy(q.table, fz.column, fz.query, fz.maxDistance, fz.limit);
		if (!st.ok) {
			child.setStatus(false, st.message);
			return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, fmt::format("Fuzzy search failed: {}", st.message));
		}
		
		// Extract PKs from results
		std::vector<std::string> fuzzyKeys;
		fuzzyKeys.reserve(results.size());
		for (const auto& res : results) {
			fuzzyKeys.emplace_back(res.pk);
		}
		
		child.setAttribute("index.result_count", static_cast<int64_t>(fuzzyKeys.size()));
		child.setStatus(true);
		
		// If there are additional predicates, intersect
		if (!q.predicates.empty() || !q.rangePredicates.empty()) {
			auto intersectSpan = Tracer::startSpan("query.fuzzy_and_intersection");
			
			ConjunctiveQuery structuralQuery;
			structuralQuery.table = q.table;
			structuralQuery.predicates = q.predicates;
			structuralQuery.rangePredicates = q.rangePredicates;
			structuralQuery.orderBy = q.orderBy;
			
			auto structResult = executeAndKeysRangeAware_(structuralQuery);
			if (!structResult) {
				intersectSpan.setStatus(false, structResult.error().context());
				return Err<std::vector<std::string>>(structResult.error().code(), structResult.error().context());
			}
			auto structKeys = std::move(*structResult);
			
			tbb::parallel_sort(fuzzyKeys.begin(), fuzzyKeys.end());
			tbb::parallel_sort(structKeys.begin(), structKeys.end());
			
			std::vector<std::string> intersection;
			intersection.reserve(std::min(fuzzyKeys.size(), structKeys.size()));
			std::set_intersection(
				fuzzyKeys.begin(), fuzzyKeys.end(),
				structKeys.begin(), structKeys.end(),
				std::back_inserter(intersection)
			);
			
			intersectSpan.setStatus(true);
			span.setStatus(true);
			return Ok(std::move(intersection));
		}
		
		span.setStatus(true);
		return Ok(std::move(fuzzyKeys));
	}
	
	// Handle fulltext queries
	if (q.fulltextPredicate.has_value()) {
		const auto& ft = *q.fulltextPredicate;
		auto child = Tracer::startSpan("index.scanFulltext");
		child.setAttribute("index.table", q.table);
		child.setAttribute("index.column", ft.column);
		child.setAttribute("index.query", ft.query);
		child.setAttribute("index.limit", static_cast<int64_t>(ft.limit));
		
		auto [st, results] = secIdx_->scanFulltextWithScores(q.table, ft.column, ft.query, ft.limit);
		if (!st.ok) {
			child.setStatus(false, st.message);
			return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, fmt::format("Fulltext search failed: {}", st.message));
		}
		
		// Extract PKs from results
		std::vector<std::string> fulltextKeys;
		fulltextKeys.reserve(results.size());
		for (const auto& res : results) {
			fulltextKeys.emplace_back(res.pk);
		}
		
		child.setAttribute("index.result_count", static_cast<int64_t>(fulltextKeys.size()));
		child.setStatus(true);
		
		// If there are additional predicates (AND combination), intersect with fulltext results
		if (!q.predicates.empty() || !q.rangePredicates.empty()) {
			auto intersectSpan = Tracer::startSpan("query.fulltext_and_intersection");
			intersectSpan.setAttribute("fulltext.result_count", static_cast<int64_t>(fulltextKeys.size()));
			intersectSpan.setAttribute("additional.eq_count", static_cast<int64_t>(q.predicates.size()));
			intersectSpan.setAttribute("additional.range_count", static_cast<int64_t>(q.rangePredicates.size()));
			
			// Create a temporary query with only the structural predicates
			ConjunctiveQuery structuralQuery;
			structuralQuery.table = q.table;
			structuralQuery.predicates = q.predicates;
			structuralQuery.rangePredicates = q.rangePredicates;
			structuralQuery.orderBy = q.orderBy;
			
			// Execute structural predicates
			auto structResult = executeAndKeysRangeAware_(structuralQuery);
			if (!structResult) {
				intersectSpan.setStatus(false, structResult.error().context());
				return Err<std::vector<std::string>>(structResult.error().code(), structResult.error().context());
			}
			auto structKeys = std::move(*structResult);
			
			// Intersect fulltext results with structural predicate results
			// Both lists should be sorted for efficient intersection
			tbb::parallel_sort(fulltextKeys.begin(), fulltextKeys.end());
			tbb::parallel_sort(structKeys.begin(), structKeys.end());
			
			std::vector<std::string> intersection;
			intersection.reserve(std::min(fulltextKeys.size(), structKeys.size()));
			std::set_intersection(
				fulltextKeys.begin(), fulltextKeys.end(),
				structKeys.begin(), structKeys.end(),
				std::back_inserter(intersection)
			);
			
			intersectSpan.setAttribute("intersection.result_count", static_cast<int64_t>(intersection.size()));
			intersectSpan.setStatus(true);
			span.setAttribute("query.result_count", static_cast<int64_t>(intersection.size()));
			span.setStatus(true);
			return Ok(std::move(intersection));
		}
		
		// Standalone FULLTEXT (no additional predicates)
		span.setAttribute("query.result_count", static_cast<int64_t>(fulltextKeys.size()));
		span.setStatus(true);
		return Ok(std::move(fulltextKeys));
	}
	
	// Handle spatial queries (G3 - AQL Parser Integration)
	if (q.spatialPredicate.has_value()) {
		if (!spatialIdx_) {
			return Err<std::vector<std::string>>(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "Spatial index manager not available");
		}
		
		const auto& sp = *q.spatialPredicate;
		auto child = Tracer::startSpan("index.scanSpatial");
		child.setAttribute("index.table", q.table);
		child.setAttribute("index.column", sp.column);
		child.setAttribute("spatial.operation", static_cast<int64_t>(sp.operation));
		
		// Check if table has spatial index
		if (!spatialIdx_->hasSpatialIndex(q.table)) {
			child.setStatus(false, "No spatial index on table");
			return Err<std::vector<std::string>>(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "Table " + q.table + " has no spatial index");
		}
		
		// Execute spatial query based on operation type
		std::vector<std::string> spatialKeys;
		
		if (sp.bbox_min.has_value() && sp.bbox_max.has_value()) {
			// Use pre-computed bbox for query
			geo::MBR query_bbox(
				sp.bbox_min->first, sp.bbox_min->second,
				sp.bbox_max->first, sp.bbox_max->second
			);
			
			auto results = spatialIdx_->searchIntersects(q.table, query_bbox);
			spatialKeys.reserve(results.size());
			for (const auto& res : results) {
				spatialKeys.emplace_back(res.primary_key);
			}
		} else {
			child.setStatus(false, "Spatial predicate missing bbox");
			return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT, "Spatial predicate must have computed bbox");
		}
		
		child.setAttribute("index.result_count", static_cast<int64_t>(spatialKeys.size()));
		child.setStatus(true);
		
		// If there are additional predicates (AND combination), intersect with spatial results
		if (!q.predicates.empty() || !q.rangePredicates.empty()) {
			auto intersectSpan = Tracer::startSpan("query.spatial_and_intersection");
			intersectSpan.setAttribute("spatial.result_count", static_cast<int64_t>(spatialKeys.size()));
			intersectSpan.setAttribute("additional.eq_count", static_cast<int64_t>(q.predicates.size()));
			intersectSpan.setAttribute("additional.range_count", static_cast<int64_t>(q.rangePredicates.size()));
			
			// Create a temporary query with only the structural predicates
			ConjunctiveQuery structuralQuery;
			structuralQuery.table = q.table;
			structuralQuery.predicates = q.predicates;
			structuralQuery.rangePredicates = q.rangePredicates;
			structuralQuery.orderBy = q.orderBy;
			
			// Execute structural predicates
			auto structResult = executeAndKeysRangeAware_(structuralQuery);
			if (!structResult) {
				intersectSpan.setStatus(false, structResult.error().context());
				return Err<std::vector<std::string>>(structResult.error().code(), structResult.error().context());
			}
			auto structKeys = std::move(*structResult);
			
			// Intersect spatial results with structural predicate results
			tbb::parallel_sort(spatialKeys.begin(), spatialKeys.end());
			tbb::parallel_sort(structKeys.begin(), structKeys.end());
			
			std::vector<std::string> intersection;
			intersection.reserve(std::min(spatialKeys.size(), structKeys.size()));
			std::set_intersection(
				spatialKeys.begin(), spatialKeys.end(),
				structKeys.begin(), structKeys.end(),
				std::back_inserter(intersection)
			);
			
			intersectSpan.setAttribute("intersection.result_count", static_cast<int64_t>(intersection.size()));
			intersectSpan.setStatus(true);
			span.setAttribute("query.result_count", static_cast<int64_t>(intersection.size()));
			span.setStatus(true);
			return Ok(std::move(intersection));
		}
		
		// Standalone SPATIAL (no additional predicates)
		span.setAttribute("query.result_count", static_cast<int64_t>(spatialKeys.size()));
		span.setStatus(true);
		return Ok(std::move(spatialKeys));
	}
	
	// Erlaube ORDER BY ohne weitere Prädikate (liefert die ersten N gemäß Range-Index)
	if (q.predicates.empty() && q.rangePredicates.empty() && !q.orderBy.has_value()) {
		return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT, "executeAndKeys: keine Prädikate");
	}

	// Wenn Range-Prädikate vorhanden sind, nutze die range-aware Logik (inkl. ORDER BY)
	if (!q.rangePredicates.empty() || q.orderBy.has_value()) {
		return executeAndKeysRangeAware_(q);
	}

	// Parallele Scans pro Prädikat
	std::vector<std::vector<std::string>> all_lists(q.predicates.size());
	std::mutex errors_mutex;
	std::vector<std::string> errors;
	errors.reserve(q.predicates.size());
	tbb::task_group tg;

	for (size_t i = 0; i < q.predicates.size(); ++i) {
		const auto& p = q.predicates[i];
		tg.run([this, &q, &p, &all_lists, i, &errors, &errors_mutex]() {
			auto child = Tracer::startSpan("index.scanEqual");
			child.setAttribute("index.table", q.table);
			child.setAttribute("index.column", p.column);
			auto [st, keys] = secIdx_->scanKeysEqual(q.table, p.column, p.value);
			if (!st.ok) {
				THEMIS_ERROR("Parallel scan error ({}={}): {}", p.column, p.value, st.message);
				std::lock_guard<std::mutex> lk(errors_mutex);
				errors.push_back(st.message);
				child.setStatus(false, st.message);
				return;
			}
			// Sortieren zur späteren Schnittmenge
			tbb::parallel_sort(keys.begin(), keys.end());
			all_lists[i] = std::move(keys);
			child.setAttribute("index.result_count", static_cast<int64_t>(all_lists[i].size()));
			child.setStatus(true);
		});
	}
	
	// Timeout enforcement: tbbWaitWithTimeout replaces bare tg.wait() (Q1/REL-50)
	{
		const auto audit_config = snapshotAuditConfig();
		const auto start_time = std::chrono::high_resolution_clock::now();
		tbbWaitWithTimeout(tg, audit_config.audit_logger, audit_config.query_timeout_ms, "and_keys_scan");
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::high_resolution_clock::now() - start_time);

		// Structured audit event for this phase (Q2)
		if (audit_config.audit_logger) {
			audit_config.audit_logger->logEvent({
				{"event",           "query_execution_phase"},
				{"phase",           "and_keys_scan"},
				{"predicate_count", static_cast<int64_t>(q.predicates.size())},
				{"elapsed_ms",      elapsed.count()},
				{"error_count",     static_cast<int64_t>(errors.size())}
			});
		}
	}

	if (!errors.empty()) {
		std::sort(errors.begin(), errors.end());
		return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, "executeAndKeys: " + errors.front());
	}

	// Leere Listen früh abbrechen
	for (const auto& l : all_lists) {
		if (l.empty()) return Ok(std::vector<std::string>{});
	}

	// Schnittmenge bilden
	auto keys = intersectSortedLists_(std::move(all_lists));
	span.setAttribute("query.result_count", static_cast<int64_t>(keys.size()));
	span.setStatus(true);
	return Ok(std::move(keys));
}

Result<QueryEngine::KeysWithScores>
QueryEngine::executeAndKeysWithScores(const ConjunctiveQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeAndKeysWithScores");
	span.setAttribute("query.table", q.table);
	span.setAttribute("query.fulltext", q.fulltextPredicate.has_value());
	
	// If no FULLTEXT predicate, delegate to standard method (no scores)
	if (!q.fulltextPredicate.has_value()) {
		auto keysResult = executeAndKeys(q);
		if (!keysResult) {
			return Err<KeysWithScores>(keysResult.error().code(), keysResult.error().context());
		}
		KeysWithScores result;
		result.keys = std::move(keysResult.value());
		result.bm25_scores = std::make_shared<std::unordered_map<std::string, double>>();
		return Ok(std::move(result));
	}
	
	// FULLTEXT query: Extract scores
	const auto& ft = *q.fulltextPredicate;
	auto child = Tracer::startSpan("index.scanFulltextWithScores");
	child.setAttribute("index.table", q.table);
	child.setAttribute("index.column", ft.column);
	child.setAttribute("index.query", ft.query);
	child.setAttribute("index.limit", static_cast<int64_t>(ft.limit));
	
	auto [st, results] = secIdx_->scanFulltextWithScores(q.table, ft.column, ft.query, ft.limit);
	if (!st.ok) {
		child.setStatus(false, st.message);
		return Err<KeysWithScores>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, st.message);
	}
	
	// Build score map and key list
	auto scoreMap = std::make_shared<std::unordered_map<std::string, double>>();
	std::vector<std::string> fulltextKeys;
	fulltextKeys.reserve(results.size());
	scoreMap->reserve(results.size());
	
	for (const auto& res : results) {
		fulltextKeys.emplace_back(res.pk);
		scoreMap->emplace(res.pk, res.score);
	}
	
	child.setAttribute("index.result_count", static_cast<int64_t>(fulltextKeys.size()));
	child.setStatus(true);
	
	// If there are additional predicates (AND combination), intersect with fulltext results
	if (!q.predicates.empty() || !q.rangePredicates.empty()) {
		auto intersectSpan = Tracer::startSpan("query.fulltext_and_intersection");
		intersectSpan.setAttribute("fulltext.result_count", static_cast<int64_t>(fulltextKeys.size()));
		intersectSpan.setAttribute("additional.eq_count", static_cast<int64_t>(q.predicates.size()));
		intersectSpan.setAttribute("additional.range_count", static_cast<int64_t>(q.rangePredicates.size()));
		
		// Create a temporary query with only the structural predicates
		ConjunctiveQuery structuralQuery;
		structuralQuery.table = q.table;
		structuralQuery.predicates = q.predicates;
		structuralQuery.rangePredicates = q.rangePredicates;
		structuralQuery.orderBy = q.orderBy;
		
		// Execute structural predicates
		auto structResult = executeAndKeysRangeAware_(structuralQuery);
		if (!structResult) {
			intersectSpan.setStatus(false, structResult.error().context());
			return Err<KeysWithScores>(structResult.error().code(), structResult.error().context());
		}
		auto structKeys = std::move(*structResult);
		
		// Intersect fulltext results with structural predicate results
		tbb::parallel_sort(fulltextKeys.begin(), fulltextKeys.end());
		tbb::parallel_sort(structKeys.begin(), structKeys.end());
		
		std::vector<std::string> intersection;
		intersection.reserve(std::min(fulltextKeys.size(), structKeys.size()));
		std::set_intersection(
			fulltextKeys.begin(), fulltextKeys.end(),
			structKeys.begin(), structKeys.end(),
			std::back_inserter(intersection)
		);
		
		// Filter score map to only include intersection keys
		auto filteredScores = std::make_shared<std::unordered_map<std::string, double>>();
		filteredScores->reserve(intersection.size());
		for (const auto& pk : intersection) {
			auto it = scoreMap->find(pk);
			if (it != scoreMap->end()) {
				filteredScores->emplace(pk, it->second);
			}
		}
		
		intersectSpan.setAttribute("intersection.result_count", static_cast<int64_t>(intersection.size()));
		intersectSpan.setStatus(true);
		span.setAttribute("query.result_count", static_cast<int64_t>(intersection.size()));
		span.setStatus(true);
		
		KeysWithScores result;
		result.keys = std::move(intersection);
		result.bm25_scores = std::move(filteredScores);
		return Ok(std::move(result));
	}
	
	// Standalone FULLTEXT (no additional predicates)
	span.setAttribute("query.result_count", static_cast<int64_t>(fulltextKeys.size()));
	span.setStatus(true);
	
	KeysWithScores result;
	result.keys = std::move(fulltextKeys);
	result.bm25_scores = std::move(scoreMap);
	return Ok(std::move(result));
}

Result<std::vector<BaseEntity>>
QueryEngine::executeAndEntities(const ConjunctiveQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeAndEntities");
	span.setAttribute("query.table", q.table);
	if (!db_) {
		return Err<std::vector<BaseEntity>>(
			errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			"Storage backend not available"
		);
	}

	// QE-2: enforce collection-level access control before any I/O
	if (collection_access_checker_ && !collection_access_checker_(q.table, collection_access_caller_id_)) {
		return Err<std::vector<BaseEntity>>(
			errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
			"Access denied for collection: " + q.table
		);
	}

	// ── Primary-key fast path ──────────────────────────────────────────────
	// Avoid the double storage round-trip (executeAndKeys checks existence,
	// then this method fetches the blob again).  With pk_eq set we do a single
	// get and deserialise in one step.
	if (q.pk_eq.has_value()) {
		const std::string& pk = *q.pk_eq;
		auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
		span.setAttribute("query.pk_fast_path", true);
		span.setStatus(true);
		if (!blob.has_value()) return Ok(std::vector<BaseEntity>{});
		try {
			std::vector<BaseEntity> out;
			out.emplace_back(BaseEntity::deserialize(pk, *blob));
			return Ok(std::move(out));
		} catch (...) {
			THEMIS_WARN("executeAndEntities pk_fast_path: Deserialisierung fehlgeschlagen für PK={}", pk);
			return Ok(std::vector<BaseEntity>{});
		}
	}

	auto keysResult = executeAndKeys(q);
	if (!keysResult) {
	  return Err<std::vector<BaseEntity>>(keysResult.error().code(), keysResult.error().context());
	}
	auto keys = std::move(keysResult.value());

	// Paralleles Entity-Loading für große Ergebnismengen (Batch-Verarbeitung)
	constexpr size_t PARALLEL_THRESHOLD = 100;
	constexpr size_t BATCH_SIZE = 50;
	constexpr size_t kMaxResultSetSize = 1'000'000;

	if (keys.size() > kMaxResultSetSize) {
		THEMIS_WARN("executeAndEntities: result set truncated from {} to {} entries", keys.size(), kMaxResultSetSize);
		keys.resize(kMaxResultSetSize);
	}

	std::vector<BaseEntity> out;
	out.reserve(keys.size());

	if (keys.size() < PARALLEL_THRESHOLD) {
		// Sequential für kleine Mengen (weniger Overhead)
		for (const auto& pk : keys) {
			auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
			if (!blob) {
			  continue;
			}
			try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
			catch (...) { THEMIS_WARN("executeAndEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }
		}
	} else {
		// Parallel für große Mengen: Batch-Processing mit TBB
		std::vector<std::vector<BaseEntity>> batches((keys.size() + BATCH_SIZE - 1) / BATCH_SIZE);
		std::vector<std::string> failed_deserialize_pks;
		failed_deserialize_pks.reserve(keys.size() / 10 + 1);
		std::mutex failed_deserialize_mutex;
		tbb::task_group tg;

		for (size_t batch_idx = 0; batch_idx < batches.size(); ++batch_idx) {
			tg.run([this, &q, &keys, &batches, batch_idx, BATCH_SIZE, &failed_deserialize_pks, &failed_deserialize_mutex]() {
				size_t start = batch_idx * BATCH_SIZE;
				size_t end = std::min(start + BATCH_SIZE, keys.size());
				std::vector<BaseEntity> local_entities;
				local_entities.reserve(end - start);

				for (size_t i = start; i < end; ++i) {
					const auto& pk = keys[i];
					auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
					if (!blob) {
					  continue;
					}
					try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
					catch (...) {
         THEMIS_DEBUG("query_engine: unhandled exception caught");
						std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
						failed_deserialize_pks.push_back(pk);
					}
				}

				batches[batch_idx] = std::move(local_entities);
			});
		}
	
		// Timeout enforcement via helper (Q1/REL-50)
		{
			const auto audit_config = snapshotAuditConfig();
			const auto entity_load_start = std::chrono::high_resolution_clock::now();
			tbbWaitWithTimeout(tg, audit_config.audit_logger, audit_config.query_timeout_ms, "entity_loading");
			const auto entity_load_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::high_resolution_clock::now() - entity_load_start);

			if (audit_config.audit_logger) {
				audit_config.audit_logger->logEvent({
					{"event",                    "query_execution_phase"},
					{"phase",                    "entity_loading"},
					{"batch_count",              static_cast<int64_t>(batches.size())},
					{"total_entities",           static_cast<int64_t>(keys.size())},
					{"failed_deserialize_count", static_cast<int64_t>(failed_deserialize_pks.size())},
					{"elapsed_ms",               entity_load_elapsed.count()}
				});
			}
		}

		logSortedDeserializeFailures(failed_deserialize_pks, "executeAndEntities");

		// Merge batches
		for (auto& batch : batches) {
			out.insert(out.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
		}
	}
	span.setAttribute("query.entities_count", static_cast<int64_t>(out.size()));
	span.setStatus(true);
	return Ok(std::move(out));
}

std::vector<std::string>
QueryEngine::intersectSortedLists_(std::vector<std::vector<std::string>> lists) {
	// Sortiere nach Größe, beginne mit kleinsten Listen für effiziente Schnittmenge
	tbb::parallel_sort(lists.begin(), lists.end(), [](const auto& a, const auto& b) {
		if (a.size() == b.size()) {
			return a < b;
		}
		return a.size() < b.size();
	});
	if (lists.empty()) return {};
	
	std::vector<std::string> result = lists.front();
	
	for (size_t i = 1; i < lists.size(); ++i) {
		const auto& next = lists[i];
		std::vector<std::string> tmp;
		tmp.reserve(std::min(result.size(), next.size()));
		std::set_intersection(result.begin(), result.end(), next.begin(), next.end(), std::back_inserter(tmp));
		result.swap(tmp);
		if (result.empty()) {
		  break;
		}
	}
	return result;
}

std::vector<std::string>
QueryEngine::unionSortedLists_(std::vector<std::vector<std::string>> lists) {
	if (lists.empty()) return {};
	if (lists.size() == 1) {
	  return lists.front();
	}
	
	// Merge all lists using set_union (removes duplicates)
	std::vector<std::string> result = lists.front();
	
	for (size_t i = 1; i < lists.size(); ++i) {
		const auto& next = lists[i];
		std::vector<std::string> tmp;
		tmp.reserve(result.size() + next.size()); // Reserve max possible size
		std::set_union(result.begin(), result.end(), next.begin(), next.end(), std::back_inserter(tmp));
		result.swap(tmp);
	}
	return result;
}

Result<std::vector<std::string>>
QueryEngine::executeOrKeys(const DisjunctiveQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeOrKeys");
	span.setAttribute("query.table", q.table);
	span.setAttribute("query.disjuncts", static_cast<int64_t>(q.disjuncts.size()));
	if (q.table.empty()) {
		return Err<std::vector<std::string>>(
			ErrorCode::ERR_QUERY_INVALID_INPUT,
			"executeOrKeys: table darf nicht leer sein"
		);
	}
	if (q.disjuncts.empty()) {
		return Err<std::vector<std::string>>(
			ErrorCode::ERR_QUERY_INVALID_INPUT,
			"executeOrKeys: keine Disjunkte"
		);
	}
	// QE-2: enforce collection-level access control before any I/O
	if (collection_access_checker_ && !collection_access_checker_(q.table, collection_access_caller_id_)) {
		return Err<std::vector<std::string>>(
			ErrorCode::ERR_QUERY_ACCESS_DENIED,
			"Access denied for collection: " + q.table
		);
	}

	// Execute each disjunct (AND-block) and collect results
	std::vector<std::vector<std::string>> all_lists(q.disjuncts.size());
	std::mutex errors_mutex;
	std::vector<std::string> errors;
	errors.reserve(q.disjuncts.size());
	tbb::task_group tg;

	for (size_t i = 0; i < q.disjuncts.size(); ++i) {
		const auto& disjunct = q.disjuncts[i];
		tg.run([this, &disjunct, &all_lists, i, &errors, &errors_mutex]() {
			auto child = Tracer::startSpan("or.disjunct.execute");
			child.setAttribute("disjunct.eq_count", static_cast<int64_t>(disjunct.predicates.size()));
			child.setAttribute("disjunct.range_count", static_cast<int64_t>(disjunct.rangePredicates.size()));
			auto result = executeAndKeys(disjunct);
			if (!result) {
				THEMIS_ERROR("Parallel OR disjunct error: {}", result.error().context());
				std::lock_guard<std::mutex> lk(errors_mutex);
				errors.push_back(result.error().context());
				child.setStatus(false, result.error().context());
				return;
			}
			// Sort for later union
			auto keys = std::move(*result);
			tbb::parallel_sort(keys.begin(), keys.end());
			all_lists[i] = std::move(keys);
			child.setAttribute("disjunct.result_count", static_cast<int64_t>(all_lists[i].size()));
			child.setStatus(true);
		});
	}
	
	// Timeout enforcement via helper (Q1/REL-50)
	{
		const auto audit_config = snapshotAuditConfig();
		const auto or_query_start = std::chrono::high_resolution_clock::now();
		tbbWaitWithTimeout(tg, audit_config.audit_logger, audit_config.query_timeout_ms, "or_disjuncts");
		const auto or_query_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::high_resolution_clock::now() - or_query_start);

		if (audit_config.audit_logger) {
			audit_config.audit_logger->logEvent({
				{"event",          "query_execution_phase"},
				{"phase",          "or_disjuncts"},
				{"disjunct_count", static_cast<int64_t>(q.disjuncts.size())},
				{"elapsed_ms",     or_query_elapsed.count()},
				{"error_count",    static_cast<int64_t>(errors.size())}
			});
		}
	}

	if (!errors.empty()) {
		std::sort(errors.begin(), errors.end());
		return Err<std::vector<std::string>>(
			ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			"executeOrKeys: " + errors.front()
		);
	}

	// Union all result sets
	auto keys = unionSortedLists_(std::move(all_lists));
	span.setAttribute("query.result_count", static_cast<int64_t>(keys.size()));
	span.setStatus(true);
	return Ok(std::move(keys));
}

Result<std::vector<std::string>>
QueryEngine::executeOrKeysWithFallback(const DisjunctiveQuery& q, bool optimize) const {
	auto span = Tracer::startSpan("QueryEngine.executeOrKeysWithFallback");
	span.setAttribute("query.table", q.table);
	span.setAttribute("query.disjuncts", static_cast<int64_t>(q.disjuncts.size()));
	if (q.table.empty()) {
		return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT, "executeOrKeysWithFallback: table darf nicht leer sein");
	}
	if (q.disjuncts.empty()) {
		return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT, "executeOrKeysWithFallback: keine Disjunkte");
	}
	// QE-2: enforce collection-level access control before any I/O
	if (collection_access_checker_ && !collection_access_checker_(q.table, collection_access_caller_id_)) {
		return Err<std::vector<std::string>>(
			errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
			"Access denied for collection: " + q.table
		);
	}

	std::vector<std::vector<std::string>> all_lists(q.disjuncts.size());
	std::vector<std::string> errors;
	errors.reserve(q.disjuncts.size());
	std::mutex error_mutex;
	tbb::task_group tg;
	for (size_t i = 0; i < q.disjuncts.size(); ++i) {
		const auto& disjunct = q.disjuncts[i];
		tg.run([this, &disjunct, &all_lists, i, optimize, &errors, &error_mutex]() {
			auto child = Tracer::startSpan("or.disjunct.execute_fallback");
			child.setAttribute("disjunct.eq_count", static_cast<int64_t>(disjunct.predicates.size()));
			child.setAttribute("disjunct.range_count", static_cast<int64_t>(disjunct.rangePredicates.size()));
			auto result = executeAndKeysWithFallback(disjunct, optimize);
			if (!result) {
				std::lock_guard<std::mutex> eg(error_mutex);
				errors.push_back(result.error().message());
				THEMIS_ERROR("Parallel OR (fallback) disjunct error: {}", result.error().message());
				child.setStatus(false, result.error().message());
				return;
			}
			auto keys = std::move(*result);
			tbb::parallel_sort(keys.begin(), keys.end());
			all_lists[i] = std::move(keys);
			child.setAttribute("disjunct.result_count", static_cast<int64_t>(all_lists[i].size()));
			child.setStatus(true);
		});
	}
	// Timeout enforcement via helper (Q1/REL-50)
	{
		const auto audit_config = snapshotAuditConfig();
		tbbWaitWithTimeout(tg, audit_config.audit_logger, audit_config.query_timeout_ms, "or_fallback_disjuncts");
	}

	if (!errors.empty()) {
		std::sort(errors.begin(), errors.end());
		return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			"executeOrKeysWithFallback: one or more disjuncts failed: " + errors.front());
	}

	auto keys = unionSortedLists_(std::move(all_lists));
	span.setAttribute("query.result_count", static_cast<int64_t>(keys.size()));
	span.setStatus(true);
	return Ok(std::move(keys));
}

Result<std::vector<BaseEntity>>
QueryEngine::executeOrEntitiesWithFallback(const DisjunctiveQuery& q, bool optimize) const {
	auto span = Tracer::startSpan("QueryEngine.executeOrEntitiesWithFallback");
	span.setAttribute("query.table", q.table);
	auto result = executeOrKeysWithFallback(q, optimize);
	if (!result) {
		return Err<std::vector<BaseEntity>>(result.error().code(), result.error().message());
	}
	auto keys = std::move(*result);

	// Parallel entity loading (analog zu executeOrEntities)
	constexpr size_t PARALLEL_THRESHOLD = 100;
	constexpr size_t BATCH_SIZE = 50;
	constexpr size_t kMaxResultSetSize = 1'000'000;

	if (keys.size() > kMaxResultSetSize) {
		THEMIS_WARN("executeOrEntitiesWithFallback: result set truncated from {} to {} entries", keys.size(), kMaxResultSetSize);
		keys.resize(kMaxResultSetSize);
	}

	std::vector<BaseEntity> out;
	out.reserve(keys.size());

	if (keys.size() < PARALLEL_THRESHOLD) {
		for (const auto& pk : keys) {
			auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
			if (!blob) {
			  continue;
			}
			try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
			catch (...) { THEMIS_WARN("executeOrEntitiesWithFallback: Deserialisierung fehlgeschlagen für PK={}", pk); }
		}
	} else {
		std::vector<std::vector<BaseEntity>> batches((keys.size() + BATCH_SIZE - 1) / BATCH_SIZE);
		std::vector<std::string> failed_deserialize_pks;
		failed_deserialize_pks.reserve(keys.size() / 10 + 1);
		std::mutex failed_deserialize_mutex;
		tbb::task_group tg;
		for (size_t batch_idx = 0; batch_idx < batches.size(); ++batch_idx) {
			tg.run([this, &q, &keys, &batches, batch_idx, BATCH_SIZE, &failed_deserialize_pks, &failed_deserialize_mutex]() {
				size_t start = batch_idx * BATCH_SIZE;
				size_t end = std::min(start + BATCH_SIZE, keys.size());
				std::vector<BaseEntity> local_entities;
				local_entities.reserve(end - start);
				for (size_t i = start; i < end; ++i) {
					const auto& pk = keys[i];
					auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
					if (!blob) {
					  continue;
					}
					try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
					catch (...) {
         THEMIS_DEBUG("query_engine: unhandled exception caught");
						std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
						failed_deserialize_pks.push_back(pk);
					}
				}
				batches[batch_idx] = std::move(local_entities);
			});
		}
	
		// Timeout enforcement via helper (Q1/REL-50)
		{
			const auto audit_config = snapshotAuditConfig();
			const auto or_entity_load_start = std::chrono::high_resolution_clock::now();
			tbbWaitWithTimeout(tg, audit_config.audit_logger, audit_config.query_timeout_ms, "or_entity_loading");
			const auto or_entity_load_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::high_resolution_clock::now() - or_entity_load_start);

			if (audit_config.audit_logger) {
				audit_config.audit_logger->logEvent({
					{"event",                    "query_execution_phase"},
					{"phase",                    "or_entity_loading"},
					{"batch_count",              static_cast<int64_t>(batches.size())},
					{"total_entities",           static_cast<int64_t>(keys.size())},
					{"failed_deserialize_count", static_cast<int64_t>(failed_deserialize_pks.size())},
					{"elapsed_ms",               or_entity_load_elapsed.count()}
				});
			}
		}
	
		logSortedDeserializeFailures(failed_deserialize_pks, "executeOrEntitiesWithFallback");
		for (auto& batch : batches) {
			out.insert(out.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
		}
	}

	span.setAttribute("query.entities_count", static_cast<int64_t>(out.size()));
	span.setStatus(true);
	return Ok(std::move(out));
}

Result<std::vector<BaseEntity>>
QueryEngine::executeOrEntities(const DisjunctiveQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeOrEntities");
	span.setAttribute("query.table", q.table);
	auto result = executeOrKeys(q);
	if (!result) {
		return Err<std::vector<BaseEntity>>(result.error().code(), result.error().context());
	}
	auto keys = std::move(*result);

	// Parallel entity loading (same logic as executeAndEntities)
	constexpr size_t PARALLEL_THRESHOLD = 100;
	constexpr size_t BATCH_SIZE = 50;

	std::vector<BaseEntity> out;
	out.reserve(keys.size());

	if (keys.size() < PARALLEL_THRESHOLD) {
		for (const auto& pk : keys) {
			auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
			if (!blob) {
			  continue;
			}
			try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
			catch (...) { THEMIS_WARN("executeOrEntities: Deserialisierung fehlgeschlagen für PK={}", pk); }
		}
	} else {
		std::vector<std::vector<BaseEntity>> batches((keys.size() + BATCH_SIZE - 1) / BATCH_SIZE);
		std::vector<std::string> failed_deserialize_pks;
		failed_deserialize_pks.reserve(keys.size() / 10 + 1);
		std::mutex failed_deserialize_mutex;
		tbb::task_group tg;

		for (size_t batch_idx = 0; batch_idx < batches.size(); ++batch_idx) {
			tg.run([this, &q, &keys, &batches, batch_idx, BATCH_SIZE, &failed_deserialize_pks, &failed_deserialize_mutex]() {
				size_t start = batch_idx * BATCH_SIZE;
				size_t end = std::min(start + BATCH_SIZE, keys.size());
				std::vector<BaseEntity> local_entities;
				local_entities.reserve(end - start);

				for (size_t i = start; i < end; ++i) {
					const auto& pk = keys[i];
					auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
					if (!blob) {
					  continue;
					}
					try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
					catch (...) {
         THEMIS_DEBUG("query_engine: unhandled exception caught");
						std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
						failed_deserialize_pks.push_back(pk);
					}
				}

				batches[batch_idx] = std::move(local_entities);
			});
		}
	
		// Timeout enforcement via helper (Q1/REL-50)
		{
			const auto audit_config = snapshotAuditConfig();
			const auto final_or_entity_start = std::chrono::high_resolution_clock::now();
			tbbWaitWithTimeout(tg, audit_config.audit_logger, audit_config.query_timeout_ms, "final_or_entity_loading");
			const auto final_or_entity_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::high_resolution_clock::now() - final_or_entity_start);

			if (audit_config.audit_logger) {
				audit_config.audit_logger->logEvent({
					{"event",                    "query_execution_phase"},
					{"phase",                    "final_or_entity_loading"},
					{"batch_count",              static_cast<int64_t>(batches.size())},
					{"total_entities",           static_cast<int64_t>(keys.size())},
					{"failed_deserialize_count", static_cast<int64_t>(failed_deserialize_pks.size())},
					{"elapsed_ms",               final_or_entity_elapsed.count()}
				});
			}
		}

		logSortedDeserializeFailures(failed_deserialize_pks, "executeOrEntities");

		for (auto& batch : batches) {
			out.insert(out.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
		}
	}

	span.setAttribute("query.entities_count", static_cast<int64_t>(out.size()));
	span.setStatus(true);
	return Ok(std::move(out));
}

Result<std::vector<std::string>>
QueryEngine::executeAndKeysSequential(const std::string& table,
									  const std::vector<PredicateEq>& orderedPredicates) const {
	using namespace themis::errors;
	auto span = Tracer::startSpan("QueryEngine.executeAndKeysSequential");
	span.setAttribute("query.table", table);
	span.setAttribute("query.eq_count", static_cast<int64_t>(orderedPredicates.size()));
	if (table.empty()) {
		return Err<std::vector<std::string>>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
			"executeAndKeysSequential: table is empty");
	}
	if (orderedPredicates.empty()) {
		return Err<std::vector<std::string>>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
			"executeAndKeysSequential: no predicates provided");
	}
	// QE-2: enforce collection-level access control before any I/O
	if (collection_access_checker_ && !collection_access_checker_(table, collection_access_caller_id_)) {
		return Err<std::vector<std::string>>(
			ErrorCode::ERR_QUERY_ACCESS_DENIED,
			"Access denied for collection: " + table
		);
	}

	// Starte mit erster Liste
	{
		auto child = Tracer::startSpan("index.scanEqual");
		child.setAttribute("index.table", table);
		child.setAttribute("index.column", orderedPredicates[0].column);
		auto [st0, baseTmp] = secIdx_->scanKeysEqual(table, orderedPredicates[0].column, orderedPredicates[0].value);
		if (!st0.ok) { 
			child.setStatus(false, st0.message); 
			return Err<std::vector<std::string>>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
				fmt::format("executeAndKeysSequential: {}", st0.message));
		}
		std::vector<std::string> base = std::move(baseTmp);
		tbb::parallel_sort(base.begin(), base.end());
		child.setAttribute("index.result_count", static_cast<int64_t>(base.size()));
		child.setStatus(true);
		if (base.empty()) { span.setStatus(true); return Ok(std::vector<std::string>{}); }
        
		std::vector<std::string> current = std::move(base);
		for (size_t i = 1; i < orderedPredicates.size(); ++i) {
			const auto& p = orderedPredicates[i];
			auto child2 = Tracer::startSpan("index.scanEqual");
			child2.setAttribute("index.table", table);
			child2.setAttribute("index.column", p.column);
			auto [st, keys] = secIdx_->scanKeysEqual(table, p.column, p.value);
			if (!st.ok) { 
				child2.setStatus(false, st.message); 
				return Err<std::vector<std::string>>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
					fmt::format("executeAndKeysSequential: {}", st.message));
			}
			if (keys.empty()) { child2.setStatus(true); span.setStatus(true); return Ok(std::vector<std::string>{}); }
			tbb::parallel_sort(keys.begin(), keys.end());
			std::vector<std::string> tmp;
			tmp.reserve(std::min(current.size(), keys.size()));
			std::set_intersection(current.begin(), current.end(), keys.begin(), keys.end(), std::back_inserter(tmp));
			current.swap(tmp);
			child2.setAttribute("index.result_count", static_cast<int64_t>(current.size()));
			child2.setStatus(true);
			if (current.empty()) {
			  break;
			}
		}
		span.setAttribute("query.result_count", static_cast<int64_t>(current.size()));
		span.setStatus(true);
		return Ok(std::move(current));
	}
}

Result<std::vector<BaseEntity>>
QueryEngine::executeAndEntitiesSequential(const std::string& table,
										  const std::vector<PredicateEq>& orderedPredicates) const {
	using namespace themis::errors;
	auto span = Tracer::startSpan("QueryEngine.executeAndEntitiesSequential");
	span.setAttribute("query.table", table);
	auto keysResult = executeAndKeysSequential(table, orderedPredicates);
	if (!keysResult) {
	  return Err<std::vector<BaseEntity>>(keysResult.error().code(), keysResult.error().message());
	}

	const auto& keys = *keysResult;

	// Paralleles Entity-Loading auch für Sequential-Variant
	constexpr size_t PARALLEL_THRESHOLD = 100;
	constexpr size_t BATCH_SIZE = 50;

	std::vector<BaseEntity> out;
	out.reserve(keys.size());

	if (keys.size() < PARALLEL_THRESHOLD) {
		// Sequential für kleine Mengen
		for (const auto& pk : keys) {
			auto blob = db_->get(KeySchema::makeRelationalKey(table, pk));
			if (!blob) {
			  continue;
			}
			try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
			catch (...) { THEMIS_WARN("executeAndEntitiesSequential: Deserialisierung fehlgeschlagen für PK={}", pk); }
		}
	} else {
		// Parallel für große Mengen
		std::vector<std::vector<BaseEntity>> batches((keys.size() + BATCH_SIZE - 1) / BATCH_SIZE);
		std::vector<std::string> failed_deserialize_pks;
		failed_deserialize_pks.reserve(keys.size() / 10 + 1);
		std::mutex failed_deserialize_mutex;
		tbb::task_group tg;

		for (size_t batch_idx = 0; batch_idx < batches.size(); ++batch_idx) {
			tg.run([this, &table, &keys, &batches, batch_idx, BATCH_SIZE, &failed_deserialize_pks, &failed_deserialize_mutex]() {
				size_t start = batch_idx * BATCH_SIZE;
				size_t end = std::min(start + BATCH_SIZE, keys.size());
				std::vector<BaseEntity> local_entities;
				local_entities.reserve(end - start);

				for (size_t i = start; i < end; ++i) {
					const auto& pk = keys[i];
					auto blob = db_->get(KeySchema::makeRelationalKey(table, pk));
					if (!blob) {
					  continue;
					}
					try { local_entities.emplace_back(BaseEntity::deserialize(pk, *blob)); }
					catch (...) {
         THEMIS_DEBUG("query_engine: unhandled exception caught");
						std::lock_guard<std::mutex> lk(failed_deserialize_mutex);
						failed_deserialize_pks.push_back(pk);
					}
				}

				batches[batch_idx] = std::move(local_entities);
			});
		}
			// Timeout enforcement via helper (Q1/REL-50)
			{
				const auto audit_config = snapshotAuditConfig();
				tbbWaitWithTimeout(tg, audit_config.audit_logger, audit_config.query_timeout_ms, "sequential_entity_loading");
			}

		logSortedDeserializeFailures(failed_deserialize_pks, "executeAndEntitiesSequential");

		// Merge batches
		for (auto& batch : batches) {
			out.insert(out.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
		}
	}

	span.setAttribute("query.entities_count", static_cast<int64_t>(out.size()));
	span.setStatus(true);
	return Ok(std::move(out));
}

Result<size_t>
QueryEngine::executeAndCount(const ConjunctiveQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeAndCount");
	span.setAttribute("query.table", q.table);

	auto keysResult = executeAndKeys(q);
	if (!keysResult) {
		span.setStatus(false, keysResult.error().context());
		return Err<size_t>(keysResult.error().code(), keysResult.error().context());
	}

	const size_t count = (keysResult.value()).size();
	span.setAttribute("query.result_count", static_cast<int64_t>(count));
	span.setStatus(true);
	return Ok(count);
}

} // namespace query
} // namespace themis

// Out-of-line EvaluationContext CTE helpers
namespace themis {
namespace query {
void QueryEngine::EvaluationContext::storeCTE(const std::string& name, std::vector<nlohmann::json> results) {
	// Prefer cache if available; fall back to in-memory map
	if (cte_cache) {
		cte_cache->store(name, std::move(results));
		return;
	}

	cte_results[name] = std::move(results);
}

std::optional<std::vector<nlohmann::json>> QueryEngine::EvaluationContext::getCTE(const std::string& name) const {
	// Prefer cache lookup first (may transparently load spilled results)
	if (cte_cache) {
		auto cached = cte_cache->get(name);
		if (cached) {
		  return cached;
		}
	}

	auto it = cte_results.find(name);
	if (it != cte_results.end()) {
	  return it->second;
	}

	if (parent) {
	  return parent->getCTE(name);
	}

	return std::nullopt;
}

// Basic helpers for AQL expression evaluation in QueryEngine
static double qe_toNumber(const nlohmann::json& v) {
	if (v.is_number()) {
	  return v.get<double>();
	}
	if (v.is_boolean()) {
	  return v.get<bool>() ? 1.0 : 0.0;
	}
	if (v.is_string()) {
		try { return std::stod(v.get<std::string>()); } catch (...) { return 0.0; }
	}
	return 0.0;
}

static bool qe_toBool(const nlohmann::json& v) {
	if (v.is_boolean()) {
	  return v.get<bool>();
	}
	if (v.is_number()) {
	  return v.get<double>() != 0.0;
	}
	if (v.is_string()) {
	  return !v.get<std::string>().empty();
	}
	if (v.is_array() || v.is_object()) {
	  return !v.empty();
	}
	return false;
}

static nlohmann::json qe_getNested(const nlohmann::json& base, const std::vector<std::string>& path) {
	const nlohmann::json* current = &base;
	for (const auto& key : path) {
		if (current->is_object()) {
			auto it = current->find(key);
			if (it == current->end()) {
			  return nullptr;
			}
			current = &(*it);
		} else if (current->is_array()) {
			try {
				size_t idx = static_cast<size_t>(std::stoull(key));
				if (idx < current->size()) {
				  current = &((*current)[idx]); else return nullptr;
				}
			} catch (...) { return nullptr; }
		} else {
			return nullptr;
		}
	}
	return *current;
}

// Forward decl
static Result<nlohmann::json> qe_evalExpr(const std::shared_ptr<themis::query::Expression>& expr,
								  const themis::query::QueryEngine::EvaluationContext& ctx);

static Result<nlohmann::json> qe_evalFunction(const std::string& funcName,
									  const std::vector<std::shared_ptr<themis::query::Expression>>& args,
									  const themis::query::QueryEngine::EvaluationContext& ctx) {
	using namespace themis::query;
	using namespace themis::errors;
	auto evalArg = [&]([[maybe_unused]] size_t i) -> Result<nlohmann::json> { return qe_evalExpr(args[i], ctx); };

	// Basic string/number functions (subset, mirroring LetEvaluator)
	if (funcName == "LENGTH") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
				fmt::format("LENGTH expects 1 argument, got {}", args.size()));
		}
		auto v = evalArg(0);
		if (!v) {
		  return v;
		}
		if (v.value().is_string()) {
		  return Ok(nlohmann::json(v.value().get<std::string>().length()));
		}
		if (v.value().is_array() || v.value().is_object()) {
		  return Ok(nlohmann::json(v.value().size()));
		}
		return Ok(nlohmann::json(0));
	}
	if (funcName == "CONCAT") {
		std::string out;
		for (size_t i = 0; i < args.size(); ++i) {
			auto v = evalArg(i);
			if (!v) {
			  return v;
			}
			out += v.value().is_string() ? v.value().get<std::string>() : v.value().dump();
		}
		return Ok(nlohmann::json(out));
	}
	if (funcName == "SUBSTRING") {
		if (args.size() < 2 || args.size() > 3) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
				fmt::format("SUBSTRING expects 2 or 3 arguments, got {}", args.size()));
		}
		auto s = evalArg(0);
		if (!s) {
		  return s;
		}
		auto st = evalArg(1);
		if (!st) {
		  return st;
		}
		if (!s.value().is_string()) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, 
				"SUBSTRING expects string as first argument");
		}
		std::string sv = s.value().get<std::string>();
		// Clamp negative/out-of-range doubles to [0, sv.size()] before narrowing to
		// size_t; a raw static_cast of a negative double is implementation-defined UB.
		const double startD = qe_toNumber(*st);
		size_t startIdx = (startD <= 0.0) ? 0 :
			(startD >= static_cast<double>(sv.size())) ? sv.size() :
			static_cast<size_t>(startD);
		if (startIdx >= sv.size()) {
		  return Ok(nlohmann::json(""));
		}
		if (args.size() == 3) {
			auto lenRes = evalArg(2);
			if (!lenRes) {
			  return lenRes;
			}
			const double lenD = qe_toNumber(*lenRes);
			size_t len = (lenD <= 0.0) ? 0 :
				(lenD >= static_cast<double>(sv.size())) ? sv.size() :
				static_cast<size_t>(lenD);
			return Ok(nlohmann::json(sv.substr(startIdx, len)));
		}
		return Ok(nlohmann::json(sv.substr(startIdx)));
	}
	if (funcName == "UPPER" || funcName == "LOWER") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
				fmt::format("{} expects 1 argument, got {}", funcName, args.size()));
		}
		auto v = evalArg(0);
		if (!v) {
		  return v;
		}
		if (!v.value().is_string()) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, 
				fmt::format("{} expects string argument", funcName));
		}
		std::string s = v.value().get<std::string>();
		if (funcName == "UPPER") {
		  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		}
		else std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return Ok(nlohmann::json(s));
	}
	if (funcName == "ABS" || funcName == "CEIL" || funcName == "FLOOR" || funcName == "ROUND") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
				fmt::format("{} expects 1 argument, got {}", funcName, args.size()));
		}
		auto argRes = evalArg(0);
		if (!argRes) {
		  return argRes;
		}
		double x = qe_toNumber(*argRes);
		if (funcName == "ABS") {
		  return Ok(nlohmann::json(std::abs(x)));
		}
		if (funcName == "CEIL") {
		  return Ok(nlohmann::json(std::ceil(x)));
		}
		if (funcName == "FLOOR") {
		  return Ok(nlohmann::json(std::floor(x)));
		}
		return Ok(nlohmann::json(std::round(x)));
	}
	if (funcName == "MIN" || funcName == "MAX") {
		if (args.empty()) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
				fmt::format("{} expects at least 1 argument", funcName));
		}
		auto arg0 = evalArg(0);
		if (!arg0) {
		  return arg0;
		}
		double val = qe_toNumber(*arg0);
		for (size_t i = 1; i < args.size(); ++i) {
			auto argRes = evalArg(i);
			if (!argRes) {
			  return argRes;
			}
			double x = qe_toNumber(*argRes);
			if (funcName == "MIN") {
			  val = std::min(val, x); else val = std::max(val, x);
			}
		}
		return Ok(nlohmann::json(val));
	}

	// ================= SPATIAL (ST_*) =================
	if (funcName == "ST_Point") {
		if (args.size() != 2) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_Point expects 2 arguments, got {}", args.size()));
		}
		auto arg0 = evalArg(0);
		if (!arg0) {
		  return arg0;
		}
		auto arg1 = evalArg(1);
		if (!arg1) {
		  return arg1;
		}
		double x = qe_toNumber(*arg0);
		double y = qe_toNumber(*arg1);
		nlohmann::json g; g["type"] = "Point"; g["coordinates"] = {x, y};
		return Ok(nlohmann::json(g));
	}

	if (funcName == "ST_AsGeoJSON") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_AsGeoJSON expects 1 argument, got {}", args.size()));
		}
		auto geomRes = evalArg(0);
		if (!geomRes) {
		  return geomRes;
		}
		auto geom = *geomRes;
		// Accept any GeoJSON object (all types: coordinates-based or geometries-based)
		if (geom.is_object() && geom.contains("type")) {
			return Ok(nlohmann::json(geom.dump()));
		}
		if (geom.is_string()) {
			std::string ewkbStr = geom.get<std::string>();
			std::vector<uint8_t> ewkb(ewkbStr.begin(), ewkbStr.end());
			try {
				auto geomInfo = geo::EWKBParser::parse(ewkb);
				return Ok(nlohmann::json(geo::EWKBParser::toGeoJSON(geomInfo)));
			} catch (const std::exception& e) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
					fmt::format("ST_AsGeoJSON: Failed to parse EWKB: {}", e.what()));
			}
		}
		return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH,
			"ST_AsGeoJSON: Argument must be GeoJSON object or EWKB binary");
	}

	if (funcName == "ST_Distance") {
		if (args.size() != 2) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_Distance expects 2 arguments, got {}", args.size()));
		}
		auto g1Res = evalArg(0);
		if (!g1Res) {
		  return g1Res;
		}
		auto g2Res = evalArg(1);
		if (!g2Res) {
		  return g2Res;
		}
		auto g1 = *g1Res;
		auto g2 = *g2Res;
		auto extractPoint = [](const nlohmann::json& g) -> Result<std::pair<double,double>> {
			if (g.is_object() && g.contains("type") && g["type"]=="Point" && g.contains("coordinates") && g["coordinates"].size()>=2) {
				return Ok(std::pair<double,double>(g["coordinates"][0].get<double>(), g["coordinates"][1].get<double>()));
			}
			return Err<std::pair<double,double>>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_Distance: Expected Point geometry");
		};
		auto p1 = extractPoint(g1);
		if (!p1) {
		  return Err<nlohmann::json>(p1.error().code(), p1.error().message());
		}
		auto p2 = extractPoint(g2);
		if (!p2) {
		  return Err<nlohmann::json>(p2.error().code(), p2.error().message());
		}
		auto [x1,y1] = *p1;
		auto [x2,y2] = *p2;
		double dx=x2-x1, dy=y2-y1; double distance = std::sqrt(dx*dx+dy*dy);
		auto looksLikeDegrees = [](double lon, double lat) { return lon >= -180.0 && lon <= 180.0 && lat >= -90.0 && lat <= 90.0; };
		if (looksLikeDegrees(x1, y1) && looksLikeDegrees(x2, y2) && (std::abs(dx) > 5.0 || std::abs(dy) > 5.0)) {
			constexpr double kEarthRadiusKm = 6371.0;
			auto deg2rad = []([[maybe_unused]] double d){ return d * std::numbers::pi_v<double> / 180.0; };
			double lat1 = deg2rad(y1), lon1 = deg2rad(x1); double lat2 = deg2rad(y2), lon2 = deg2rad(x2);
			double dlat = lat2 - lat1; double dlon = lon2 - lon1;
			double a = std::sin(dlat/2.0)*std::sin(dlat/2.0) + std::cos(lat1)*std::cos(lat2)*std::sin(dlon/2.0)*std::sin(dlon/2.0);
			double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a)); double km = kEarthRadiusKm * c;
			constexpr double kKmPerDegreeApprox = 59.0;
			return Ok(nlohmann::json(km / kKmPerDegreeApprox));
		}
		return Ok(nlohmann::json(distance));
	}

	if (funcName == "ST_GeomFromGeoJSON") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_GeomFromGeoJSON expects 1 argument, got {}", args.size()));
		}
		auto jsonArgRes = evalArg(0);
		if (!jsonArgRes) {
		  return jsonArgRes;
		}
		auto jsonArg = *jsonArgRes;
		if (jsonArg.is_object() && jsonArg.contains("type") && jsonArg.contains("coordinates")) {
			return Ok(nlohmann::json(jsonArg));
		}
		if (jsonArg.is_string()) {
			std::string jsonStr = jsonArg.get<std::string>();
			try {
				nlohmann::json geojson = nlohmann::json::parse(jsonStr);
				if (!geojson.is_object() || !geojson.contains("type") || !geojson.contains("coordinates")) {
					return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH,
						"Invalid GeoJSON: must have 'type' and 'coordinates'");
				}
				return Ok(nlohmann::json(geojson));
			} catch (const std::exception& e) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
					fmt::format("ST_GeomFromGeoJSON: Failed to parse JSON: {}", e.what()));
			}
		}
		return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH,
			"ST_GeomFromGeoJSON: Argument must be GeoJSON object or JSON string");
	}

	if (funcName == "ST_Intersects") {
		if (args.size() != 2) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_Intersects expects 2 arguments, got {}", args.size()));
		}
		auto g1Res = evalArg(0);
		if (!g1Res) {
		  return g1Res;
		}
		auto g2Res = evalArg(1);
		if (!g2Res) {
		  return g2Res;
		}
		auto g1 = *g1Res;
		auto g2 = *g2Res;
		auto extractPoint = [](const nlohmann::json& g) -> Result<std::pair<double,double>> {
			if (g.is_object() && g.contains("type") && g["type"]=="Point" && g.contains("coordinates") && g["coordinates"].size()>=2) {
				return Ok(std::pair<double,double>(g["coordinates"][0].get<double>(), g["coordinates"][1].get<double>()));
			}
			return Err<std::pair<double,double>>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_Intersects: Expected Point geometry");
		};
		auto p1 = extractPoint(g1);
		if (!p1) {
		  return Err<nlohmann::json>(p1.error().code(), p1.error().message());
		}
		auto p2 = extractPoint(g2);
		if (!p2) {
		  return Err<nlohmann::json>(p2.error().code(), p2.error().message());
		}
		auto [x1,y1] = *p1;
		auto [x2,y2] = *p2;
		const double eps=1e-5;
		return Ok(nlohmann::json(std::abs(x1-x2)<eps && std::abs(y1-y2)<eps));
	}

	if (funcName == "ST_Within") {
		if (args.size() != 2) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_Within expects 2 arguments, got {}", args.size()));
		}
		auto g1Res = evalArg(0);
		if (!g1Res) {
		  return g1Res;
		}
		auto g2Res = evalArg(1);
		if (!g2Res) {
		  return g2Res;
		}
		auto g1 = *g1Res;
		auto g2 = *g2Res;
		std::function<Result<std::pair<double,double>>(const nlohmann::json&)> extractPoint = [&]([[maybe_unused]] const nlohmann::json& g) -> Result<std::pair<double,double>> {
			if (g.is_string()) {
				try {
					auto parseRes = nlohmann::json::parse(g.get<std::string>());
					return extractPoint(parseRes);
				} catch (const std::exception& e) {
					// Parse failed, continue to other checks
					spdlog::debug("ST_Within extractPoint: Failed to parse string as JSON - {}", e.what());
				}
			}
			if (g.is_array() && g.size() >= 2) {
				double x = g[0].get<double>();
				double y = g[1].get<double>();
				return Ok(std::pair<double,double>{x,y});
			}
			if (g.is_object() && g.contains("type") && g["type"] == "Point" && g.contains("coordinates")) {
				auto coords = g["coordinates"];
				if (coords.is_array() && coords.size() >= 2) {
					double x = coords[0].get<double>();
					double y = coords[1].get<double>();
					return Ok(std::pair<double,double>{x,y});
				}
			}
			return Err<std::pair<double,double>>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_Within: Expected Point geometry");
		};

		std::function<Result<utils::geo::MBR>(const nlohmann::json&)> extractMBR = [&]([[maybe_unused]] const nlohmann::json& g) -> Result<utils::geo::MBR> {
			if (g.is_string()) {
				try {
					auto parsed = nlohmann::json::parse(g.get<std::string>());
					return extractMBR(parsed);
				} catch (const std::exception& e) {
					// Parse failed, log and continue to other checks
					spdlog::debug("ST_Within extractMBR: Failed to parse string as JSON - {}", e.what());
				}
			}
			if (g.is_array() && g.size() == 4) {
				return Ok(utils::geo::MBR{ g[0].get<double>(), g[1].get<double>(), g[2].get<double>(), g[3].get<double>() });
			}
			if (g.is_object() && g.contains("type")) {
				std::string t = g["type"];
				if (t == "Point" && g.contains("coordinates") && g["coordinates"].size() >= 2) {
					double x = g["coordinates"][0].get<double>();
					double y = g["coordinates"][1].get<double>();
					return Ok(utils::geo::MBR{x,y,x,y});
				}
				if (t == "Polygon" && g.contains("coordinates")) {
					const auto& rings = g["coordinates"];
					if (rings.is_array() && !rings.empty()) {
						const auto& ext = rings[0];
						double minx=std::numeric_limits<double>::max(), miny=std::numeric_limits<double>::max();
						double maxx=std::numeric_limits<double>::lowest(), maxy=std::numeric_limits<double>::lowest();
						for (const auto& c : ext) if (c.is_array() && c.size()>=2) {
							double x=c[0].get<double>(), y=c[1].get<double>();
							minx=std::min(minx,x); miny=std::min(miny,y); maxx=std::max(maxx,x); maxy=std::max(maxy,y);
						}
						return Ok(utils::geo::MBR{minx,miny,maxx,maxy});
					}
				}
			}
			return Err<utils::geo::MBR>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_Within: Could not extract MBR");
		};

		auto pRes = extractPoint(g1);
		if (!pRes) {
		  return Err<nlohmann::json>(pRes.error().code(), pRes.error().message());
		}
		auto mRes = extractMBR(g2);
		if (!mRes) {
			spdlog::warn("[SECURITY] ST_Within: Failed to extract MBR from geometry arg — "
			             "failing closed (no record passes broken spatial filter). Error: {}",
			             mRes.error().message());
			return Ok(nlohmann::json(false));  // fail-closed: skip record on parse error
		}
		auto p = *pRes;
		auto m = *mRes;
		return Ok(nlohmann::json(p.first>=m.minx && p.first<=m.maxx && p.second>=m.miny && p.second<=m.maxy));
	}

	if (funcName == "ST_Contains") {
		if (args.size() != 2) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_Contains expects 2 arguments, got {}", args.size()));
		}
		auto g1Res = evalArg(0);
		if (!g1Res) {
		  return g1Res;
		}
		auto g2Res = evalArg(1);
		if (!g2Res) {
		  return g2Res;
		}
		auto g1 = *g1Res;
		auto g2 = *g2Res;
		auto extractMBR = [](const nlohmann::json& g) -> Result<utils::geo::MBR> {
			if (g.is_object() && g.contains("type")) {
				std::string t=g["type"];
				if (t=="Point" && g.contains("coordinates") && g["coordinates"].size()>=2) {
					double x=g["coordinates"][0].get<double>(), y=g["coordinates"][1].get<double>();
					return Ok(utils::geo::MBR{x,y,x,y});
				}
				if (t=="Polygon" && g.contains("coordinates")) {
					const auto& rings=g["coordinates"];
					if (rings.is_array() && !rings.empty()) {
						const auto& ext=rings[0];
						double minx=std::numeric_limits<double>::max(), miny=std::numeric_limits<double>::max();
						double maxx=std::numeric_limits<double>::lowest(), maxy=std::numeric_limits<double>::lowest();
						for (const auto& c : ext) if (c.is_array() && c.size()>=2) {
							double x=c[0].get<double>(), y=c[1].get<double>();
							minx=std::min(minx,x); miny=std::min(miny,y); maxx=std::max(maxx,x); maxy=std::max(maxy,y);
						}
						return Ok(utils::geo::MBR{minx,miny,maxx,maxy});
					}
				}
			}
			return Err<utils::geo::MBR>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_Contains: Could not extract MBR");
		};
		auto m1Res = extractMBR(g1);
		if (!m1Res) {
		  return Err<nlohmann::json>(m1Res.error().code(), m1Res.error().message());
		}
		auto m2Res = extractMBR(g2);
		if (!m2Res) {
		  return Err<nlohmann::json>(m2Res.error().code(), m2Res.error().message());
		}
		auto m1 = *m1Res;
		auto m2 = *m2Res;
		return Ok(nlohmann::json(m2.minx>=m1.minx && m2.maxx<=m1.maxx && m2.miny>=m1.miny && m2.maxy<=m1.maxy));
	}

	if (funcName == "ST_DWithin") {
		if (args.size() != 3) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_DWithin expects 3 arguments, got {}", args.size()));
		}
		auto g1Res = evalArg(0);
		if (!g1Res) {
		  return g1Res;
		}
		auto g2Res = evalArg(1);
		if (!g2Res) {
		  return g2Res;
		}
		auto maxdRes = evalArg(2);
		if (!maxdRes) {
		  return maxdRes;
		}
		auto g1 = *g1Res;
		auto g2 = *g2Res;
		double maxd = qe_toNumber(*maxdRes);
		auto extractPoint = [](const nlohmann::json& g) -> Result<std::pair<double,double>> {
			if (g.is_object() && g.contains("type") && g["type"]=="Point" && g.contains("coordinates") && g["coordinates"].size()>=2) {
				return Ok(std::pair<double,double>(g["coordinates"][0].get<double>(), g["coordinates"][1].get<double>()));
			}
			return Err<std::pair<double,double>>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_DWithin: Expected Point geometry");
		};
		auto p1 = extractPoint(g1);
		if (!p1) {
		  return Err<nlohmann::json>(p1.error().code(), p1.error().message());
		}
		auto p2 = extractPoint(g2);
		if (!p2) {
		  return Err<nlohmann::json>(p2.error().code(), p2.error().message());
		}
		auto [x1,y1] = *p1;
		auto [x2,y2] = *p2;
		double dx=x2-x1, dy=y2-y1; double d=std::sqrt(dx*dx+dy*dy);
		return Ok(nlohmann::json(d <= maxd));
	}

	if (funcName == "ST_HasZ") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_HasZ expects 1 argument, got {}", args.size()));
		}
		auto gRes = evalArg(0);
		if (!gRes) {
		  return gRes;
		}
		auto g = *gRes;
		if (g.is_object() && g.contains("type") && g.contains("coordinates")) {
			const auto& c = g["coordinates"]; std::string t = g["type"];
			if (t=="Point" && c.is_array() && c.size()>=3) {
			  return Ok(nlohmann::json(true));
			}
			if ((t=="LineString"||t=="MultiPoint") && c.is_array() && !c.empty() && c[0].is_array() && c[0].size()>=3) {
			  return Ok(nlohmann::json(true));
			}
			if (t=="Polygon" && c.is_array() && !c.empty() && c[0].is_array() && !c[0].empty() && c[0][0].is_array() && c[0][0].size()>=3) {
			  return Ok(nlohmann::json(true));
			}
		}
		return Ok(nlohmann::json(false));
	}

	if (funcName == "ST_Z") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_Z expects 1 argument, got {}", args.size()));
		}
		auto gRes = evalArg(0);
		if (!gRes) {
		  return gRes;
		}
		auto g = *gRes;
		if (g.is_object() && g.contains("type") && g["type"]=="Point" && g.contains("coordinates") && g["coordinates"].is_array() && g["coordinates"].size()>=3) {
			return Ok(nlohmann::json(g["coordinates"][2]));
		}
		return Ok(nlohmann::json(nullptr));
	}

	if (funcName == "ST_ZMin" || funcName == "ST_ZMax") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("{} expects 1 argument, got {}", funcName, args.size()));
		}
		auto gRes = evalArg(0);
		if (!gRes) {
		  return gRes;
		}
		auto g = *gRes;
		if (!g.is_object() || !g.contains("type") || !g.contains("coordinates")) {
			return Ok(nlohmann::json(nullptr));
		}
		std::string t = g["type"]; const auto& coords = g["coordinates"];
		double acc = (funcName=="ST_ZMin") ? std::numeric_limits<double>::max() : std::numeric_limits<double>::lowest();
		bool hasZ=false;
		auto upd = [&]([[maybe_unused]] double z){ if (funcName=="ST_ZMin") acc = std::min(acc, z); else acc = std::max(acc, z); hasZ=true; };
		if (t=="Point" && coords.is_array() && coords.size()>=3) {
			return Ok(nlohmann::json(coords[2]));
		}
		if ((t=="LineString"||t=="MultiPoint") && coords.is_array()) {
			for (const auto& pt : coords) {
			  if (pt.is_array() && pt.size()>=3) upd(pt[2].get<double>());
			}
		} else if (t=="Polygon" && coords.is_array()) {
			for (const auto& ring : coords) {
			  if (ring.is_array()) for (const auto& pt : ring) if (pt.is_array() && pt.size()>=3) upd(pt[2].get<double>());
			}
		}
		return Ok(hasZ ? nlohmann::json(acc) : nlohmann::json(nullptr));
	}

	if (funcName == "ST_GeomFromText") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_GeomFromText expects 1 argument, got {}", args.size()));
		}
		auto wRes = evalArg(0);
		if (!wRes) {
		  return wRes;
		}
		auto w = *wRes;
		if (!w.is_string()) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH,
				"ST_GeomFromText: Argument must be WKT string");
		}
		std::string wkt = w.get<std::string>();
		auto trim = [](std::string s){ s.erase(0, s.find_first_not_of(" \t\n\r")); s.erase(s.find_last_not_of(" \t\n\r")+1); return s; };
		std::string u = trim(wkt); std::string up=u; std::transform(up.begin(), up.end(), up.begin(), ::toupper);
		nlohmann::json geojson;
		if (up.rfind("POINT",0)==0) {
			size_t a=up.find('('), b=up.find(')');
			if (a==std::string::npos||b==std::string::npos) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Invalid POINT WKT");
			}
			std::string coords = u.substr(a+1, b-a-1);
			std::istringstream iss(coords); double x,y,z;
			if (!(iss>>x>>y)) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Invalid POINT coords");
			}
			geojson["type"]="Point"; if (iss>>z) geojson["coordinates"]={x,y,z}; else geojson["coordinates"]={x,y};
			return Ok(nlohmann::json(geojson));
		}
		if (up.rfind("LINESTRING",0)==0) {
			size_t a=up.find('('), b=up.find(')');
			if (a==std::string::npos||b==std::string::npos) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Invalid LINESTRING WKT");
			}
			std::string coordsStr = u.substr(a+1, b-a-1);
			std::replace(coordsStr.begin(), coordsStr.end(), ',', ' ');
			std::istringstream iss(coordsStr); nlohmann::json arr = nlohmann::json::array(); double x,y,z;
			while (iss>>x>>y) { if (iss>>z) arr.emplace_back(nlohmann::json::array({x,y,z})); else arr.emplace_back(nlohmann::json::array({x,y})); }
			geojson["type"]="LineString"; geojson["coordinates"]=arr;
			return Ok(nlohmann::json(geojson));
		}
		if (up.rfind("POLYGON",0)==0) {
			size_t a=up.find("(("), b=up.find("))");
			if (a==std::string::npos||b==std::string::npos || b<=a+1) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Invalid POLYGON WKT");
			}
			std::string inner = u.substr(a+2, b-(a+2));
			nlohmann::json ring = nlohmann::json::array();
			std::stringstream ringStream(inner);
			std::string pointToken;
			while (std::getline(ringStream, pointToken, ',')) {
				pointToken = trim(pointToken);
				if (pointToken.empty()) {
				  continue;
				}
				std::istringstream pointIss(pointToken);
				double x, y, z;
				if (!(pointIss >> x >> y)) {
					continue;
				}
				if (pointIss >> z) {
					ring.emplace_back(nlohmann::json::array({x, y, z}));
				} else {
					ring.emplace_back(nlohmann::json::array({x, y}));
				}
			}
			if (ring.empty()) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Invalid POLYGON coords");
			}
			nlohmann::json coords = nlohmann::json::array(); coords.emplace_back(std::move(ring));
			nlohmann::json poly; poly["type"]="Polygon"; poly["coordinates"]=coords;
			return Ok(nlohmann::json(poly));
		}
		return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			"ST_GeomFromText: Unsupported WKT (POINT, LINESTRING, POLYGON)");
	}

	if (funcName == "ST_AsText") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_AsText expects 1 argument, got {}", args.size()));
		}
		auto gRes = evalArg(0);
		if (!gRes) {
		  return gRes;
		}
		auto g = *gRes;
		if (!g.is_object() || !g.contains("type") || !g.contains("coordinates")) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH,
				"ST_AsText: Invalid geometry object");
		}
		std::string t = g["type"]; const auto& c = g["coordinates"]; std::ostringstream wkt;
		if (t=="Point") {
			if (!c.is_array() || c.size()<2) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_AsText: Invalid Point");
			}
			wkt<<"POINT("<<c[0].get<double>()<<" "<<c[1].get<double>(); if (c.size()>=3) wkt<<" "<<c[2].get<double>(); wkt<<")";
			return Ok(nlohmann::json(wkt.str()));
		} else if (t=="LineString") {
			if (!c.is_array()||c.empty()) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_AsText: Invalid LineString");
			}
			wkt<<"LINESTRING("; for (size_t i=0;i<c.size();++i){ if(i>0) wkt<<","; const auto& pt=c[i]; wkt<<pt[0].get<double>()<<" "<<pt[1].get<double>(); if (pt.size()>=3) wkt<<" "<<pt[2].get<double>(); } wkt<<")";
			return Ok(nlohmann::json(wkt.str()));
		} else if (t=="Polygon") {
			if (!c.is_array()||c.empty()) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_AsText: Invalid Polygon");
			}
			wkt<<"POLYGON("; for (size_t r=0;r<c.size();++r){ if(r>0) wkt<<","; wkt<<"("; const auto& ring=c[r]; for(size_t i=0;i<ring.size();++i){ if(i>0) wkt<<","; const auto& pt=ring[i]; wkt<<pt[0].get<double>()<<" "<<pt[1].get<double>(); if (pt.size()>=3) wkt<<" "<<pt[2].get<double>(); } wkt<<")"; } wkt<<")";
			return Ok(nlohmann::json(wkt.str()));
		}
		return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			"ST_AsText: Unsupported geometry type");
	}

	if (funcName == "ST_3DDistance") {
		if (args.size() != 2) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_3DDistance expects 2 arguments, got {}", args.size()));
		}
		auto g1Res = evalArg(0);
		if (!g1Res) {
		  return g1Res;
		}
		auto g2Res = evalArg(1);
		if (!g2Res) {
		  return g2Res;
		}
		auto g1 = *g1Res;
		auto g2 = *g2Res;
		auto extract = [](const nlohmann::json& g) -> Result<std::tuple<double,double,double>> {
			if (g.is_object() && g.contains("type") && g["type"]=="Point" && g.contains("coordinates") && g["coordinates"].is_array()) {
				const auto& a=g["coordinates"]; if (a.size()>=2) { double x=a[0].get<double>(), y=a[1].get<double>(); double z = a.size()>=3 ? a[2].get<double>() : 0.0; return Ok(std::tuple<double,double,double>(x,y,z)); }
			}
			return Err<std::tuple<double,double,double>>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_3DDistance: Expected Point");
		};
		auto p1 = extract(g1);
		if (!p1) {
		  return Err<nlohmann::json>(p1.error().code(), p1.error().message());
		}
		auto p2 = extract(g2);
		if (!p2) {
		  return Err<nlohmann::json>(p2.error().code(), p2.error().message());
		}
		auto [x1,y1,z1] = *p1;
		auto [x2,y2,z2] = *p2;
		double dx=x2-x1, dy=y2-y1, dz=z2-z1;
		return Ok(nlohmann::json(std::sqrt(dx*dx+dy*dy+dz*dz)));
	}

	if (funcName == "ST_Force2D") {
		if (args.size() != 1) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_Force2D expects 1 argument, got {}", args.size()));
		}
		auto gRes = evalArg(0);
		if (!gRes) {
		  return gRes;
		}
		auto g = *gRes;
		if (!g.is_object() || !g.contains("type") || !g.contains("coordinates")) {
			return Ok(nlohmann::json(g));
		}
		nlohmann::json result = g; std::string t=g["type"];
		auto strip2D = [](const nlohmann::json& coord){ if (coord.is_array() && coord.size()>=2) return nlohmann::json::array({coord[0], coord[1]}); return coord; };
		if (t=="Point") {
		  result["coordinates"]=strip2D(g["coordinates"]);
		}
		else if (t=="LineString"||t=="MultiPoint") { nlohmann::json nc=nlohmann::json::array(); for (const auto& pt : g["coordinates"]) nc.push_back(strip2D(pt)); result["coordinates"]=nc; }
		else if (t=="Polygon"||t=="MultiLineString") { nlohmann::json nr=nlohmann::json::array(); for (const auto& ring : g["coordinates"]) { nlohmann::json r=nlohmann::json::array(); for (const auto& pt : ring) r.push_back(strip2D(pt)); nr.push_back(r);} result["coordinates"]=nr; }
		return Ok(nlohmann::json(result));
	}

	if (funcName == "ST_ZBetween") {
		if (args.size() != 3) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_ZBetween expects 3 arguments, got {}", args.size()));
		}
		auto gRes = evalArg(0);
		if (!gRes) {
		  return gRes;
		}
		auto zminRes = evalArg(1);
		if (!zminRes) {
		  return zminRes;
		}
		auto zmaxRes = evalArg(2);
		if (!zmaxRes) {
		  return zmaxRes;
		}
		auto g = *gRes;
		double zmin = qe_toNumber(*zminRes);
		double zmax = qe_toNumber(*zmaxRes);
		if (!g.is_object() || !g.contains("type") || !g.contains("coordinates")) {
			return Ok(nlohmann::json(false));
		}
		std::string t=g["type"]; const auto& c=g["coordinates"]; auto inRange=[&]([[maybe_unused]] double z){ return z>=zmin && z<=zmax; };
		if (t=="Point") { if (c.is_array() && c.size()>=3) return Ok(nlohmann::json(inRange(c[2].get<double>()))); return Ok(nlohmann::json(false)); }
		if (t=="LineString"||t=="MultiPoint") { if (c.is_array()) { for (const auto& pt : c) if (pt.is_array() && pt.size()>=3 && inRange(pt[2].get<double>())) return Ok(nlohmann::json(true)); } return Ok(nlohmann::json(false)); }
		if (t=="Polygon"||t=="MultiLineString") { if (c.is_array()) { for (const auto& ring : c) if (ring.is_array()) for (const auto& pt : ring) if (pt.is_array() && pt.size()>=3 && inRange(pt[2].get<double>())) return Ok(nlohmann::json(true)); } return Ok(nlohmann::json(false)); }
		return Ok(nlohmann::json(false));
	}

	if (funcName == "ST_Buffer") {
		if (args.size() != 2) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_Buffer expects 2 arguments, got {}", args.size()));
		}
		auto gRes = evalArg(0);
		if (!gRes) {
		  return gRes;
		}
		auto distRes = evalArg(1);
		if (!distRes) {
		  return distRes;
		}
		auto g = *gRes;
		double dist = qe_toNumber(*distRes);
		if (!g.is_object() || !g.contains("type") || !g.contains("coordinates")) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH,
				"ST_Buffer: invalid geometry");
		}
		std::string t=g["type"];
		if (t=="Point") {
			const auto& c=g["coordinates"];
			if (!c.is_array()||c.size()<2) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH,
					"ST_Buffer: invalid Point");
			}
			double x=c[0].get<double>(), y=c[1].get<double>();
			nlohmann::json ring = nlohmann::json::array({ {x-dist,y-dist},{x+dist,y-dist},{x+dist,y+dist},{x-dist,y+dist},{x-dist,y-dist} });
			nlohmann::json poly; poly["type"]="Polygon"; poly["coordinates"]=nlohmann::json::array({ring});
			return Ok(nlohmann::json(poly));
		}
		if (t=="Polygon") {
			const auto& rings=g["coordinates"];
			if (!rings.is_array()||rings.empty()) {
				return Err<nlohmann::json>(ErrorCode::ERR_QUERY_TYPE_MISMATCH,
					"ST_Buffer: invalid Polygon");
			}
			const auto& ext=rings[0]; double minx=std::numeric_limits<double>::max(), miny=std::numeric_limits<double>::max(); double maxx=std::numeric_limits<double>::lowest(), maxy=std::numeric_limits<double>::lowest();
			for (const auto& pt : ext) if (pt.is_array()&&pt.size()>=2){ double x=pt[0].get<double>(), y=pt[1].get<double>(); minx=std::min(minx,x); miny=std::min(miny,y); maxx=std::max(maxx,x); maxy=std::max(maxy,y);} 
			minx-=dist; miny-=dist; maxx+=dist; maxy+=dist;
			nlohmann::json ring=nlohmann::json::array({ {minx,miny},{maxx,miny},{maxx,maxy},{minx,maxy},{minx,miny} });
			nlohmann::json poly; poly["type"]="Polygon"; poly["coordinates"]=nlohmann::json::array({ring});
			return Ok(nlohmann::json(poly));
		}
		return Ok(nlohmann::json(g));
	}

	if (funcName == "ST_Union") {
		if (args.size() != 2) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("ST_Union expects 2 arguments, got {}", args.size()));
		}
		auto g1Res = evalArg(0);
		if (!g1Res) {
		  return g1Res;
		}
		auto g2Res = evalArg(1);
		if (!g2Res) {
		  return g2Res;
		}
		auto g1 = *g1Res;
		auto g2 = *g2Res;
		auto mbrOf=[](const nlohmann::json& g) -> Result<utils::geo::MBR> {
			if (g.is_object() && g.contains("type")){
				std::string t=g["type"];
				if (t=="Point" && g.contains("coordinates") && g["coordinates"].size()>=2){ double x=g["coordinates"][0].get<double>(), y=g["coordinates"][1].get<double>(); return Ok(utils::geo::MBR{x,y,x,y}); }
				if (t=="Polygon" && g.contains("coordinates")){
					const auto& rings=g["coordinates"]; if (rings.is_array()&&!rings.empty()){
						double minx=std::numeric_limits<double>::max(),miny=std::numeric_limits<double>::max(); double maxx=std::numeric_limits<double>::lowest(),maxy=std::numeric_limits<double>::lowest();
						const auto& ext=rings[0]; for (const auto& pt:ext) if (pt.is_array()&&pt.size()>=2){ double x=pt[0].get<double>(), y=pt[1].get<double>(); minx=std::min(minx,x); miny=std::min(miny,y); maxx=std::max(maxx,x); maxy=std::max(maxy,y);} 
						return Ok(utils::geo::MBR{minx,miny,maxx,maxy});
					}
				}
			}
			return Err<utils::geo::MBR>(ErrorCode::ERR_QUERY_TYPE_MISMATCH, "ST_Union: Unsupported geometry type for MVP");
		};
		auto m1Res = mbrOf(g1);
		if (!m1Res) {
		  return Err<nlohmann::json>(m1Res.error().code(), m1Res.error().message());
		}
		auto m2Res = mbrOf(g2);
		if (!m2Res) {
		  return Err<nlohmann::json>(m2Res.error().code(), m2Res.error().message());
		}
		auto m1 = *m1Res;
		auto m2 = *m2Res;
		utils::geo::MBR u{ std::min(m1.minx,m2.minx), std::min(m1.miny,m2.miny), std::max(m1.maxx,m2.maxx), std::max(m1.maxy,m2.maxy) };
		nlohmann::json ring=nlohmann::json::array({ {u.minx,u.miny},{u.maxx,u.miny},{u.maxx,u.maxy},{u.minx,u.maxy},{u.minx,u.miny} });
		nlohmann::json poly; poly["type"]="Polygon"; poly["coordinates"]=nlohmann::json::array({ring});
		return Ok(nlohmann::json(poly));
	}

	// GEO_BUFFER(geom, distance_m [, arc_points]) — geodesic ST_BUFFER via the CPU-exact backend.
	// ArangoDB-compatible name; distance_m is in metres (geodesic-aware).
	if (funcName == "GEO_BUFFER" || funcName == "ST_BUFFER") {
		if (args.size() < 2 || args.size() > 3) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("{} expects 2 or 3 arguments, got {}", funcName, args.size()));
		}
		auto gRes = evalArg(0);
		if (!gRes) {
		  return gRes;
		}
		auto distRes = evalArg(1);
		if (!distRes) {
		  return distRes;
		}
		const double distance_m = qe_toNumber(*distRes);
		int arc_points = 36;
		if (args.size() == 3) {
			auto apRes = evalArg(2);
			if (!apRes) {
			  return apRes;
			}
			// Clamp the double to [3, 360] BEFORE narrowing to int so that out-of-range
			// floating-point values (e.g. 1e300) cannot trigger undefined behaviour.
			const double ap_d = std::clamp(qe_toNumber(*apRes), 3.0, 360.0);
			arc_points = static_cast<int>(ap_d);
		}
		try {
			const geo::GeometryInfo geom = geo::EWKBParser::parseGeoJSON(gRes.value().dump());
			const geo::GeometryInfo result = geo::getCpuExactBackend()->stBuffer(geom, distance_m, arc_points);
			const std::string json_str = geo::EWKBParser::toGeoJSON(result);
			if (json_str == "{}" || json_str.empty()) {
				nlohmann::json empty; empty["type"]="GeometryCollection"; empty["geometries"]=nlohmann::json::array();
				return Ok(nlohmann::json(empty));
			}
			return Ok(nlohmann::json::parse(json_str));
		} catch (const std::exception& e) {
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
				fmt::format("{} error: {}", funcName, e.what()));
		}
	}

	return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED,
		fmt::format("Unknown function: {}", funcName));
}

static Result<nlohmann::json> qe_evalExpr(const std::shared_ptr<themis::query::Expression>& expr,
								  const themis::query::QueryEngine::EvaluationContext& ctx) {
	using namespace themis::query;
	using namespace themis::errors;
	if (!expr) {
	  return Ok(nlohmann::json(nullptr));
	}

	switch (expr->getType()) {
		case ASTNodeType::Literal: {
			auto lit = std::static_pointer_cast<LiteralExpr>(expr);
			nlohmann::json j;
			std::visit([&]([[maybe_unused]] auto&& arg){ j = arg; }, lit->value);
			return Ok(j);
		}
		case ASTNodeType::Variable: {
			auto v = std::static_pointer_cast<VariableExpr>(expr);
			auto bound = ctx.get(v->name);
			return Ok(bound.has_value() ? bound.value() : nlohmann::json(nullptr));
		}
		case ASTNodeType::FieldAccess: {
			auto fa = std::static_pointer_cast<FieldAccessExpr>(expr);
			auto base = qe_evalExpr(fa->object, ctx);
			if (!base) {
			  return base;
			}
			if (base.value().is_null()) {
			  return Ok(nlohmann::json(nullptr));
			}
			return Ok(qe_getNested(*base, {fa->field}));
		}
		case ASTNodeType::ArrayLiteral: {
			auto arr = std::static_pointer_cast<ArrayLiteralExpr>(expr);
			nlohmann::json a = nlohmann::json::array();
			for (const auto& e : arr->elements) {
				auto elemRes = qe_evalExpr(e, ctx);
				if (!elemRes) {
				  return elemRes;
				}
				a.push_back(*elemRes);
			}
			return Ok(a);
		}
		case ASTNodeType::ObjectConstruct: {
			auto obj = std::static_pointer_cast<ObjectConstructExpr>(expr);
			nlohmann::json o = nlohmann::json::object();
			std::vector<std::pair<std::string, std::shared_ptr<Expression>>> sorted_fields;
			sorted_fields.reserve(obj->fields.size());
			for (const auto& [k, e] : obj->fields) {
				sorted_fields.emplace_back(k, e);
			}
			std::stable_sort(sorted_fields.begin(), sorted_fields.end(),
				[](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

			for (const auto& [k, e] : sorted_fields) {
				auto fieldRes = qe_evalExpr(e, ctx);
				if (!fieldRes) {
				  return fieldRes;
				}
				o[k] = *fieldRes;
			}
			return Ok(o);
		}
		case ASTNodeType::UnaryOp: {
			auto u = std::static_pointer_cast<UnaryOpExpr>(expr);
			auto v = qe_evalExpr(u->operand, ctx);
			if (!v) {
			  return v;
			}
			if (u->op == UnaryOperator::Not) {
			  return Ok(nlohmann::json(!qe_toBool(*v)));
			}
			if (u->op == UnaryOperator::Minus) {
			  return Ok(nlohmann::json(-qe_toNumber(*v)));
			}
			if (u->op == UnaryOperator::Plus) {
			  return Ok(nlohmann::json(qe_toNumber(*v)));
			}
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Unknown unary operator");
		}
		case ASTNodeType::BinaryOp: {
			auto b = std::static_pointer_cast<BinaryOpExpr>(expr);
			auto l = qe_evalExpr(b->left, ctx);
			if (!l) {
			  return l;
			}
			auto r = qe_evalExpr(b->right, ctx);
			if (!r) {
			  return r;
			}
			switch (b->op) {
				case BinaryOperator::Add: return Ok(nlohmann::json(qe_toNumber(*l) + qe_toNumber(*r)));
				case BinaryOperator::Sub: return Ok(nlohmann::json(qe_toNumber(*l) - qe_toNumber(*r)));
				case BinaryOperator::Mul: return Ok(nlohmann::json(qe_toNumber(*l) * qe_toNumber(*r)));
				case BinaryOperator::Div: {
					double d = qe_toNumber(*r); 
					if (d==0.0) {
					  return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Division by zero");
					}
					return Ok(nlohmann::json(qe_toNumber(*l)/d));
				}
				case BinaryOperator::Mod: {
					double d = qe_toNumber(*r); 
					if (d==0.0) {
					  return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Modulo by zero");
					}
					return Ok(nlohmann::json(std::fmod(qe_toNumber(*l), d)));
				}
				case BinaryOperator::Eq: return Ok(nlohmann::json(*l == *r));
				case BinaryOperator::Neq: return Ok(nlohmann::json(*l != *r));
				case BinaryOperator::Lt: return Ok(nlohmann::json(l.value().is_number()&&r.value().is_number() ? (l.value().get<double>() < r.value().get<double>()) : (l.value().dump() < r.value().dump())));
				case BinaryOperator::Lte: return Ok(nlohmann::json(l.value().is_number()&&r.value().is_number() ? (l.value().get<double>() <= r.value().get<double>()) : (l.value().dump() <= r.value().dump())));
				case BinaryOperator::Gt: return Ok(nlohmann::json(l.value().is_number()&&r.value().is_number() ? (l.value().get<double>() > r.value().get<double>()) : (l.value().dump() > r.value().dump())));
				case BinaryOperator::Gte: return Ok(nlohmann::json(l.value().is_number()&&r.value().is_number() ? (l.value().get<double>() >= r.value().get<double>()) : (l.value().dump() >= r.value().dump())));
				case BinaryOperator::And: return Ok(nlohmann::json(qe_toBool(*l) && qe_toBool(*r)));
				case BinaryOperator::Or: return Ok(nlohmann::json(qe_toBool(*l) || qe_toBool(*r)));
				case BinaryOperator::Xor: return Ok(nlohmann::json(qe_toBool(*l) ^ qe_toBool(*r)));
				case BinaryOperator::In: {
					// Membership: left IN right (right can be array or string)
					if (r.value().is_array()) {
						for (const auto& e : *r) {
							if (e == *l) {
							  return Ok(nlohmann::json(true));
							}
						}
						return Ok(nlohmann::json(false));
					}
					if (r.value().is_string() && l.value().is_string()) {
						return Ok(nlohmann::json(r.value().get<std::string>().find(l.value().get<std::string>()) != std::string::npos));
					}
					return Ok(nlohmann::json(false));
				}
			}
			return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, "Unknown binary operator");
		}
		case ASTNodeType::FunctionCall: {
			auto f = std::static_pointer_cast<FunctionCallExpr>(expr);
			return qe_evalFunction(f->name, f->arguments, ctx);
		}
		default: break;
	}
	return Err<nlohmann::json>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
		fmt::format("Unknown expression type in QueryEngine evaluator: {}", static_cast<int>(expr->getType())));
}

Result<nlohmann::json> QueryEngine::evaluateExpression(
	const std::shared_ptr<query::Expression>& expr,
	const EvaluationContext& ctx
) const {
	return qe_evalExpr(expr, ctx);
}

bool QueryEngine::evaluateCondition(
	const std::shared_ptr<query::Expression>& expr,
	const EvaluationContext& ctx
) const {
	auto result = qe_evalExpr(expr, ctx);
	if (!result) {
	  return false;
	}
	return qe_toBool(*result);
}

} // namespace query
} // namespace themis

namespace themis {
namespace query {

std::vector<std::string> QueryEngine::fullScanAndFilter_(const ConjunctiveQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.fullScan");
	span.setAttribute("query.table", q.table);
	span.setAttribute("query.eq_count", static_cast<int64_t>(q.predicates.size()));
	span.setAttribute("query.range_count", static_cast<int64_t>(q.rangePredicates.size()));
	std::vector<std::string> out;
	if (q.table.empty()) {
	  return out;
	}
	const std::string prefix = KeySchema::makeRelationalKey(q.table, "");

	// Helper for numeric comparison: try to parse as numbers, fall back to string comparison
	auto compareValues = [](const std::string& a, const std::string& b) -> int {
		try {
			// Try integer comparison first
			size_t pos_a = 0, pos_b = 0;
			long long num_a = std::stoll(a, &pos_a);
			long long num_b = std::stoll(b, &pos_b);

			// Only use numeric comparison if entire strings parsed
			if (pos_a == a.size() && pos_b == b.size()) {
				if (num_a < num_b) {
				  return -1;
				}
				if (num_a > num_b) {
				  return 1;
				}
				return 0;
			}
		} catch (...) {
      THEMIS_WARN("query_engine: unhandled exception caught");
			// Not integers, try doubles
			try {
				size_t pos_a = 0, pos_b = 0;
				double num_a = std::stod(a, &pos_a);
				double num_b = std::stod(b, &pos_b);

				if (pos_a == a.size() && pos_b == b.size()) {
					if (num_a < num_b) {
					  return -1;
					}
					if (num_a > num_b) {
					  return 1;
					}
					return 0;
				}
			} catch (...) {}
		}

		// Fall back to lexicographic string comparison
		return a.compare(b);
	};

	// Predicate evaluation helper – called from both sequential and parallel paths.
	// q and compareValues are captured by reference; each invocation is independent.
	auto matchesPredicates = [&]([[maybe_unused]] const BaseEntity& e) -> bool {
		for (const auto& p : q.predicates) {
			auto v = e.extractField(p.column);
			if (!v || *v != p.value) {
			  return false;
			}
		}
		for (const auto& r : q.rangePredicates) {
			auto v = e.extractField(r.column);
			if (!v) {
			  return false;
			}
			if (r.lower.has_value()) {
				int cmp = compareValues(*v, *r.lower);
				if (cmp < 0 || (cmp == 0 && !r.includeLower)) {
				  return false;
				}
			}
			if (r.upper.has_value()) {
				int cmp = compareValues(*v, *r.upper);
				if (cmp > 0 || (cmp == 0 && !r.includeUpper)) {
				  return false;
				}
			}
		}
		return true;
	};

	// ── Phase 1: Collect raw entries (sequential I/O) ──────────────────────────
	// Each entry stores the primary key and the raw serialized blob.
	struct RawEntry { std::string pk; std::vector<uint8_t> blob; };
	std::vector<RawEntry> raw_entries;

	db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
		std::string pk = KeySchema::extractPrimaryKey(key);
		raw_entries.emplace_back(RawEntry{std::move(pk), {value.begin(), value.end()}});
		return true;
	});

	const size_t n = raw_entries.size();
	span.setAttribute("fullscan.scanned", static_cast<int64_t>(n));
	out.reserve(n);

	static const ParallelScanConfig kScanConfig;

	if (n < kScanConfig.parallel_threshold) {
		// ── Sequential path (small collection) ────────────────────────────────
		span.setAttribute("fullscan.mode", "sequential");
		for (auto& entry : raw_entries) {
			try {
				BaseEntity e = BaseEntity::deserialize(entry.pk, entry.blob);
				if (matchesPredicates(e)) {
				  out.emplace_back(std::move(entry.pk));
				}
			} catch (...) {
       THEMIS_DEBUG("query_engine: unhandled exception caught");
				// skip malformed entries
			}
		}
	} else {
		// ── Parallel path (large collection – morsel-driven) ──────────────────
		// Split the collected entries into fixed-size morsels; each TBB task
		// processes one morsel independently and writes its matches into a
		// per-morsel result bucket.  Results are merged after tg.wait().
		span.setAttribute("fullscan.mode", "parallel");
		const size_t morsel_size = kScanConfig.morsel_size;
		const size_t num_morsels = (n + morsel_size - 1) / morsel_size;
		std::vector<std::vector<std::string>> morsel_results(num_morsels);

		tbb::task_group tg;
		for (size_t m = 0; m < num_morsels; ++m) {
			tg.run([&, m]() {
				const size_t start = m * morsel_size;
				const size_t end   = std::min(start + morsel_size, n);
				std::vector<std::string> local;
				local.reserve(end - start);
				for (size_t i = start; i < end; ++i) {
					auto& entry = raw_entries[i];
					try {
						BaseEntity e = BaseEntity::deserialize(entry.pk, entry.blob);
						if (matchesPredicates(e)) {
						  local.emplace_back(std::move(entry.pk));
						}
					} catch (...) {
         THEMIS_DEBUG("query_engine: unhandled exception caught");
						// skip malformed entries
					}
				}
				morsel_results[m] = std::move(local);
			});
		}
			// Timeout enforcement via helper (Q1/REL-50): morsel-driven full scan
			{
				const auto audit_config = snapshotAuditConfig();
				tbbWaitWithTimeout(tg, audit_config.audit_logger, audit_config.query_timeout_ms, "fullscan_parallel_morsels");
			}

		// Merge per-morsel results into the output vector.
		for (auto& bucket : morsel_results) {
			out.insert(out.end(),
			           std::make_move_iterator(bucket.begin()),
			           std::make_move_iterator(bucket.end()));
		}
	}

	span.setAttribute("query.result_count", static_cast<int64_t>(out.size()));
	span.setStatus(true);
	return out;
}

Result<std::vector<std::string>>
QueryEngine::executeAndKeysWithFallback(const ConjunctiveQuery& q, bool optimize) const {
	auto span = Tracer::startSpan("QueryEngine.executeAndKeysWithFallback");
	span.setAttribute("query.table", q.table);
	span.setAttribute("query.eq_count", static_cast<int64_t>(q.predicates.size()));
	span.setAttribute("query.range_count", static_cast<int64_t>(q.rangePredicates.size()));
	// QE-2: enforce collection-level access control before any I/O
	if (collection_access_checker_ && !collection_access_checker_(q.table, collection_access_caller_id_)) {
		return Err<std::vector<std::string>>(
			errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
			"Access denied for collection: " + q.table
		);
	}
	// If no predicates at all, must do full scan
	if (q.predicates.empty() && q.rangePredicates.empty() && !q.orderBy.has_value()) {
		auto keys = fullScanAndFilter_(q);
		span.setAttribute("query.exec_mode", "full_scan");
		span.setAttribute("query.result_count", static_cast<int64_t>(keys.size()));
		span.setStatus(true);
		return Ok(std::move(keys));
	}
	
	// Try index-based path; on failure due to missing index, fallback to full scan
	bool missingIndex = false;

	// Prüfe Gleichheitsindizes
	if (!q.predicates.empty()) {
		size_t bestIdx = 0; size_t bestEst = SIZE_MAX; [[maybe_unused]] bool bestCapped=false;
		for (size_t i=0;i<q.predicates.size();++i) {
			bool capped=false; size_t est = secIdx_->estimateCountEqual(q.table, q.predicates[i].column, q.predicates[i].value, 16, &capped);
			size_t eff = capped ? 16 : est;
			if (eff < bestEst) { bestEst = eff; bestIdx = i; bestCapped = capped; }
		}
		{
			auto [st, _] = secIdx_->scanKeysEqual(q.table, q.predicates[bestIdx].column, q.predicates[bestIdx].value);
			if (!st.ok) {
			  missingIndex = true;
			}
		}
	}

	// Prüfe Range-Indizes
	for (const auto& r : q.rangePredicates) {
		if (!secIdx_->hasRangeIndex(q.table, r.column)) { missingIndex = true; break; }
	}

	if (!missingIndex) {
		if (!q.rangePredicates.empty() || q.orderBy.has_value()) {
			auto result = executeAndKeysRangeAware_(q);
			if (!result) {
				span.setStatus(false, result.error().message());
				return Err<std::vector<std::string>>(result.error().code(), result.error().context());
			}
			span.setAttribute("query.exec_mode", "range_aware");
			span.setAttribute("query.result_count", static_cast<int64_t>(result.value().size()));
			span.setStatus(true);
			return Ok(std::move(result.value()));
		}
		if (optimize) {
			QueryOptimizer opt(*secIdx_);  // Dereference pointer to reference
			auto plan = opt.chooseOrderForAndQuery(q);
			auto keysResult = executeAndKeysSequential(q.table, plan.orderedPredicates);
			if (!keysResult) { 
				span.setStatus(false, keysResult.error().message()); 
				return Err<std::vector<std::string>>(keysResult.error().code(), keysResult.error().context());
			}
			span.setAttribute("query.exec_mode", "index_optimized");
			span.setAttribute("query.result_count", static_cast<int64_t>(keysResult.value().size()));
			span.setStatus(true);
			return Ok(std::move(keysResult.value()));
		}
		auto result = executeAndKeys(q);
		if (!result) {
			span.setStatus(false, result.error().message());
			return Err<std::vector<std::string>>(result.error().code(), result.error().context());
		}
		span.setAttribute("query.exec_mode", "index_parallel");
		span.setAttribute("query.result_count", static_cast<int64_t>(result.value().size()));
		span.setStatus(true);
		return Ok(std::move(result.value()));
	}

	// Fallback: full scan (inkl. Range-Prädikate)
	auto keys = fullScanAndFilter_(q);
	span.setAttribute("query.exec_mode", "full_scan_fallback");
	span.setAttribute("query.result_count", static_cast<int64_t>(keys.size()));
	span.setStatus(true);
	return Ok(std::move(keys));
}

Result<std::vector<BaseEntity>>
QueryEngine::executeAndEntitiesWithFallback(const ConjunctiveQuery& q, bool optimize) const {
	auto span = Tracer::startSpan("QueryEngine.executeAndEntitiesWithFallback");
	span.setAttribute("query.table", q.table);
	auto result = executeAndKeysWithFallback(q, optimize);
	if (!result) {
		return Err<std::vector<BaseEntity>>(result.error().code(), result.error().message());
	}
	auto keys = std::move(*result);
	std::vector<BaseEntity> out; out.reserve(keys.size());
	for (const auto& pk : keys) {
		auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
		if (!blob) {
		  continue;
		}
		try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
		catch (...) { THEMIS_WARN("executeAndEntitiesWithFallback: Deserialisierung fehlgeschlagen für PK={}", pk); }
	}
	span.setAttribute("query.entities_count", static_cast<int64_t>(out.size()));
	span.setStatus(true);
	return Ok(std::move(out));
}

// ===== Range-aware Ausführung =====
namespace {
static inline size_t bigLimit() { return static_cast<size_t>(1000000000ULL); }
}

Result<std::vector<std::string>>
QueryEngine::executeAndKeysRangeAware_(const ConjunctiveQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeAndKeysRangeAware");
	span.setAttribute("query.table", q.table);
	span.setAttribute("query.range_count", static_cast<int64_t>(q.rangePredicates.size()));

	// Cardinality-based predicate reordering: if statistics are available,
	// sort equality predicates by ascending selectivity (most selective first)
	// so that early intersection cuts down the candidate set as fast as possible.
	std::vector<PredicateEq> ordered_predicates = q.predicates;
	if (stats_collector_ && !ordered_predicates.empty()) {
		auto stats_result = stats_collector_->getStats(q.table);
		if (stats_result.ok) {
			const auto& tbl_stats = stats_result.value;
			std::stable_sort(ordered_predicates.begin(), ordered_predicates.end(),
				[&tbl_stats](const PredicateEq& a, const PredicateEq& b) {
					double sel_a = 1.0, sel_b = 1.0;
					auto it_a = tbl_stats.column_stats.find(a.column);
					if (it_a != tbl_stats.column_stats.end()) {
					  sel_a = it_a->second.selectivity;
					}
					auto it_b = tbl_stats.column_stats.find(b.column);
					if (it_b != tbl_stats.column_stats.end()) {
					  sel_b = it_b->second.selectivity;
					}
					constexpr double kSelEps = 1e-9;
					if (std::abs(sel_a - sel_b) < kSelEps) {
						if (a.column != b.column) {
							return a.column < b.column;
						}
						return a.value < b.value;
					}
					return sel_a < sel_b;  // lower selectivity = more discriminating
				});
		}
	}

	// 1) Hole Listen für alle Gleichheitsprädikate
	std::vector<std::vector<std::string>> lists;
	lists.reserve(ordered_predicates.size() + q.rangePredicates.size());

	for (const auto& p : ordered_predicates) {
		auto child = Tracer::startSpan("index.scanEqual");
		child.setAttribute("index.table", q.table);
		child.setAttribute("index.column", p.column);
		auto [st, keys] = secIdx_->scanKeysEqual(q.table, p.column, p.value);
		if (!st.ok) {
		  return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, st.message);
		}
		tbb::parallel_sort(keys.begin(), keys.end());
		lists.emplace_back(std::move(keys));
		child.setAttribute("index.result_count", static_cast<int64_t>(lists.back().size()));
		child.setStatus(true);
	}

	// 2) Range-Prädikate
	for (const auto& r : q.rangePredicates) {
		if (!secIdx_->hasRangeIndex(q.table, r.column)) {
			return Err<std::vector<std::string>>(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "Missing range index for column: " + r.column);
		}
		auto child = Tracer::startSpan("index.scanRange");
		child.setAttribute("index.table", q.table);
		child.setAttribute("index.column", r.column);
		child.setAttribute("range.has_lower", r.lower.has_value());
		child.setAttribute("range.has_upper", r.upper.has_value());
		child.setAttribute("range.includeLower", r.includeLower);
		child.setAttribute("range.includeUpper", r.includeUpper);
		auto [st, keys] = secIdx_->scanKeysRange(q.table, r.column, r.lower, r.upper, r.includeLower, r.includeUpper, bigLimit(), false);
		if (!st.ok) {
		  return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, st.message);
		}
		tbb::parallel_sort(keys.begin(), keys.end());
		lists.emplace_back(std::move(keys));
		child.setAttribute("index.result_count", static_cast<int64_t>(lists.back().size()));
		child.setStatus(true);
	}

	// Wenn keine Prädikate aber nur ORDER BY: initial candidates leer => special case
	std::vector<std::string> candidates;
	if (lists.empty()) {
		candidates.clear();
	} else {
		// 3) Schnittmenge
		candidates = intersectSortedLists_(std::move(lists));
	}

	// 4) ORDER BY
	if (q.orderBy.has_value()) {
		const auto& ob = q.orderBy.value();
		if (!secIdx_->hasRangeIndex(q.table, ob.column)) {
			return Err<std::vector<std::string>>(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "ORDER BY requires range index on column: " + ob.column);
		}
		// Bestimme Bounds aus passendem Range-Prädikat, falls vorhanden
		std::optional<std::string> lb; std::optional<std::string> ub; bool il=true, iu=true;
		for (const auto& r : q.rangePredicates) {
			if (r.column == ob.column) { lb = r.lower; ub = r.upper; il = r.includeLower; iu = r.includeUpper; break; }
		}
		// Erzeuge Kandidaten-Set für schnelles Membership-Checking (falls es Prädikate gab)
		std::unordered_set<std::string> candSet;
		if (!candidates.empty()) {
			candSet.reserve(candidates.size());
			candSet.insert(std::make_move_iterator(candidates.begin()), std::make_move_iterator(candidates.end()));
		}

		std::vector<std::string> ordered;
		ordered.reserve(ob.limit);
		// Cursor-unterstützte Range-Scans: starte nach (value, pk), falls vorhanden
		std::optional<std::pair<std::string,std::string>> anchor;
		if (ob.cursor_value.has_value() && ob.cursor_pk.has_value()) {
			anchor = std::make_pair(ob.cursor_value.value(), ob.cursor_pk.value());
		}
		auto [st, scan] = secIdx_->scanKeysRangeAnchored(q.table, ob.column, lb, ub, il, iu, bigLimit(), ob.desc, anchor);
		if (!st.ok) {
		  return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, st.message);
		}
		for (auto& k : scan) {
			if (!candSet.empty() && candSet.find(k) == candSet.end()) continue; // filter
			ordered.emplace_back(std::move(k));
			if (ordered.size() >= ob.limit) {
			  break;
			}
		}
		span.setAttribute("query.ordered_count", static_cast<int64_t>(ordered.size()));
		span.setStatus(true);
		return Ok(std::move(ordered));
	}
	span.setAttribute("query.result_count", static_cast<int64_t>(candidates.size()));
	span.setStatus(true);
	return Ok(std::move(candidates));
}

Result<std::vector<BaseEntity>>
QueryEngine::executeAndEntitiesRangeAware_(const ConjunctiveQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeAndEntitiesRangeAware");
	span.setAttribute("query.table", q.table);
	auto keysResult = executeAndKeysRangeAware_(q);
	if (!keysResult) {
	  return Err<std::vector<BaseEntity>>(keysResult.error().code(), keysResult.error().context());
	}
	auto& keys = *keysResult;
	std::vector<BaseEntity> out; out.reserve(keys.size());
	for (const auto& pk : keys) {
		auto blob = db_->get(KeySchema::makeRelationalKey(q.table, pk));
		if (!blob) {
		  continue;
		}
		try { out.emplace_back(BaseEntity::deserialize(pk, *blob)); }
		catch (...) { THEMIS_WARN("executeAndEntitiesRangeAware_: Deserialisierung fehlgeschlagen für PK={}", pk); }
	}
	span.setAttribute("query.entities_count", static_cast<int64_t>(out.size()));
	span.setStatus(true);
	return Ok(std::move(out));
}

// ============================================================================
// Join/LET/COLLECT Support (MVP)
// ============================================================================
// Helper: Extract all variable names referenced in an expression
static void collectVariables(
	const std::shared_ptr<query::Expression>& expr,
	std::set<std::string>& vars
) {
	if (!expr) {
	  return;
	}
	
	switch (expr->getType()) {
		case query::ASTNodeType::Variable: {
			auto varExpr = std::static_pointer_cast<query::VariableExpr>(expr);
			vars.insert(varExpr->name);
			break;
		}
		case query::ASTNodeType::FieldAccess: {
			auto fa = std::static_pointer_cast<query::FieldAccessExpr>(expr);
			collectVariables(fa->object, vars);
			break;
		}
		case query::ASTNodeType::BinaryOp: {
			auto bin = std::static_pointer_cast<query::BinaryOpExpr>(expr);
			collectVariables(bin->left, vars);
			collectVariables(bin->right, vars);
			break;
		}
		case query::ASTNodeType::UnaryOp: {
			auto un = std::static_pointer_cast<query::UnaryOpExpr>(expr);
			collectVariables(un->operand, vars);
			break;
		}
		case query::ASTNodeType::FunctionCall: {
			auto fn = std::static_pointer_cast<query::FunctionCallExpr>(expr);
			for (const auto& arg : fn->arguments) {
				collectVariables(arg, vars);
			}
			break;
		}
		case query::ASTNodeType::ArrayLiteral: {
			auto arr = std::static_pointer_cast<query::ArrayLiteralExpr>(expr);
			for (const auto& elem : arr->elements) {
				collectVariables(elem, vars);
			}
			break;
		}
		case query::ASTNodeType::ObjectConstruct: {
			auto obj = std::static_pointer_cast<query::ObjectConstructExpr>(expr);
			for (const auto& [key, val] : obj->fields) {
				collectVariables(val, vars);
			}
			break;
		}
		default:
			// Literal: no variables
			break;
	}
}

// Helper: Analyze if filters contain equi-join condition for hash-join optimization
struct EquiJoinCondition {
	bool found = false;
	std::string left_var;
	std::string left_field;
	std::string right_var;
	std::string right_field;
};

static EquiJoinCondition analyzeEquiJoin(
	const std::vector<std::shared_ptr<query::FilterNode>>& filters,
	const std::string& var1,
	const std::string& var2
) {
	EquiJoinCondition result;
	
	for (const auto& filter : filters) {
		auto expr = filter->condition;
		if (expr->getType() != query::ASTNodeType::BinaryOp) {
		  continue;
		}
		
		auto bin = std::static_pointer_cast<query::BinaryOpExpr>(expr);
		if (bin->op != query::BinaryOperator::Eq) {
		  continue;
		}
		
		// Check if left is var.field and right is var.field
		auto checkFieldAccess = [](const std::shared_ptr<query::Expression>& e) -> std::pair<std::string, std::string> {
			if (e->getType() != query::ASTNodeType::FieldAccess) return {"", ""};
			auto fa = std::static_pointer_cast<query::FieldAccessExpr>(e);
			if (fa->object->getType() != query::ASTNodeType::Variable) return {"", ""};
			auto varExpr = std::static_pointer_cast<query::VariableExpr>(fa->object);
			return {varExpr->name, fa->field};
		};
		
		auto [lvar, lfield] = checkFieldAccess(bin->left);
		auto [rvar, rfield] = checkFieldAccess(bin->right);
		
		if (lvar.empty() || rvar.empty()) {
		  continue;
		}
		
		// Check if this matches our two variables
		if ((lvar == var1 && rvar == var2) || (lvar == var2 && rvar == var1)) {
			result.found = true;
			result.left_var = lvar;
			result.left_field = lfield;
			result.right_var = rvar;
			result.right_field = rfield;
			return result;
		}
	}
	
	return result;
}

Result<std::vector<nlohmann::json>> QueryEngine::executeJoin(
	const std::vector<query::ForNode>& for_nodes,
	const std::vector<std::shared_ptr<query::FilterNode>>& filters,
	const std::vector<query::LetNode>& let_nodes,
	const std::shared_ptr<query::ReturnNode>& return_node,
	const std::shared_ptr<query::SortNode>& sort,
	const std::shared_ptr<query::LimitNode>& limit,
	const EvaluationContext* parent_context  // Phase 4.1
) const {
	auto span = Tracer::startSpan("QueryEngine.executeJoin");
	span.setAttribute("join.for_count", static_cast<int64_t>(for_nodes.size()));
	
	if (for_nodes.empty()) {
		return Err<std::vector<nlohmann::json>>(
			errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			"executeJoin: No FOR clauses provided"
		);
	}
	
	std::vector<nlohmann::json> results;
	
	// Phase 4.1: Initialize context with parent CTEs if provided
	EvaluationContext initial_context;
	if (parent_context) {
		initial_context.cte_results = parent_context->cte_results;
		initial_context.bm25_scores = parent_context->bm25_scores;
		initial_context.cte_cache = parent_context->cte_cache;
	}
	
	// Phase 4.3: Initialize CTE cache if not inherited from parent
	if (!initial_context.cte_cache) {
		query::CTECache::Config cache_config;
		cache_config.max_memory_bytes = 100 * 1024 * 1024; // 100MB default
		cache_config.spill_directory = "./themis_cte_spill";
		cache_config.enable_compression = false;
		cache_config.auto_cleanup = true;
		initial_context.cte_cache = std::make_shared<query::CTECache>(cache_config);
	}
	
	// OPTIMIZATION: Predicate Push-down
	// Classify filters by which variables they reference
	std::map<std::string, std::vector<std::shared_ptr<query::FilterNode>>> single_var_filters;
	std::vector<std::shared_ptr<query::FilterNode>> multi_var_filters;
	
	for (const auto& filter : filters) {
		std::set<std::string> vars;
		collectVariables(filter->condition, vars);
		
		if (vars.size() == 1) {
			// Single-variable filter: can be pushed down
			single_var_filters[*vars.begin()].push_back(filter);
			span.setAttribute("join.pushed_filters", static_cast<int64_t>(single_var_filters.size()));
		} else {
			// Multi-variable filter: must be applied after join
			multi_var_filters.push_back(filter);
		}
	}
	
	// OPTIMIZATION: Hash-Join for 2-way equi-joins
	if (for_nodes.size() == 2) {
		auto equiJoin = analyzeEquiJoin(multi_var_filters, for_nodes[0].variable, for_nodes[1].variable);
		if (equiJoin.found) {
			span.setAttribute("join.algorithm", "hash_join");
			THEMIS_DEBUG("Using hash-join for {} = {}", 
				equiJoin.left_var + "." + equiJoin.left_field,
				equiJoin.right_var + "." + equiJoin.right_field);
			
			// Build phase: Create hash table from first collection
			std::unordered_map<std::string, std::vector<nlohmann::json>> hash_table;
			const auto& build_for = for_nodes[0];
			
			// Phase 4.4: Check if build side is a CTE
			auto build_cte = initial_context.getCTE(build_for.collection);
			if (build_cte.has_value()) {
				// Build from CTE results
				for (const auto& doc : build_cte.value()) {
					// Apply pushed-down filters
					auto build_filters = single_var_filters.find(build_for.variable);
					if (build_filters != single_var_filters.end()) {
						EvaluationContext filter_ctx = initial_context;
						filter_ctx.bind(build_for.variable, doc);
						bool pass = true;
						for (const auto& filter : build_filters->second) {
							if (!evaluateCondition(filter->condition, filter_ctx)) {
								pass = false;
								break;
							}
						}
						if (!pass) {
						  continue;
						}
					}
					
					// Extract join key
					std::string join_key_field = (equiJoin.left_var == build_for.variable) 
						? equiJoin.left_field 
						: equiJoin.right_field;
					
					if (doc.contains(join_key_field)) {
						std::string join_key = doc[join_key_field].dump();
						auto [bucket_it, inserted] = hash_table.try_emplace(std::move(join_key));
						bucket_it->second.push_back(doc);
					}
				}
			} else {
				// Build from table scan
				const std::string build_prefix = KeySchema::makeRelationalKey(build_for.collection, "");
				
				// Apply pushed-down filters for build side
				auto build_filters = single_var_filters.find(build_for.variable);
				
				db_->scanPrefix(build_prefix, [&](std::string_view key, std::string_view value) -> bool {
					std::string pk = KeySchema::extractPrimaryKey(key);
					std::vector<uint8_t> blob(value.begin(), value.end());
					try {
						BaseEntity entity = BaseEntity::deserialize(pk, blob);
						nlohmann::json doc = nlohmann::json::parse(entity.toJson());
						doc["_key"] = pk;
						
						// Apply pushed-down filters
						if (build_filters != single_var_filters.end()) {
							EvaluationContext filter_ctx = initial_context;
							filter_ctx.bind(build_for.variable, doc);
							bool pass = true;
							for (const auto& filter : build_filters->second) {
								if (!evaluateCondition(filter->condition, filter_ctx)) {
									pass = false;
									break;
								}
							}
							if (!pass) return true; // Skip this document
						}
						
						// Extract join key
						std::string join_key_field = (equiJoin.left_var == build_for.variable) 
							? equiJoin.left_field 
							: equiJoin.right_field;
						
						if (doc.contains(join_key_field)) {
							std::string join_key = doc[join_key_field].dump();
							auto [bucket_it, inserted] = hash_table.try_emplace(std::move(join_key));
							bucket_it->second.push_back(doc);
						}
					} catch (...) {}
					return true;
				});
			}
			
			// Probe phase: Scan second collection and probe hash table
			const auto& probe_for = for_nodes[1];
			
			// Apply pushed-down filters for probe side
			auto probe_filters = single_var_filters.find(probe_for.variable);
			
			// Phase 4.4: Check if probe side is a CTE
			auto probe_cte = initial_context.getCTE(probe_for.collection);

			for (auto& [join_key, bucket] : hash_table) {
				std::sort(bucket.begin(), bucket.end(), stableJsonLess);
			}
		
			auto processProbeDoc = [&]([[maybe_unused]] const nlohmann::json& doc) {
				// Apply pushed-down filters
				if (probe_filters != single_var_filters.end()) {
					EvaluationContext filter_ctx = initial_context;
					filter_ctx.bind(probe_for.variable, doc);
					bool pass = true;
					for (const auto& filter : probe_filters->second) {
						if (!evaluateCondition(filter->condition, filter_ctx)) {
							pass = false;
							break;
						}
					}
					if (!pass) {
						return;
					}
				}
			
				// Extract join key
				const std::string join_key_field = (equiJoin.right_var == probe_for.variable)
					? equiJoin.right_field
					: equiJoin.left_field;
				if (!doc.contains(join_key_field)) {
					return;
				}
				const std::string join_key = doc[join_key_field].dump();
			
				// Probe hash table
				auto it = hash_table.find(join_key);
				if (it == hash_table.end()) {
					return;
				}
			
				for (const auto& build_doc : it->second) {
					EvaluationContext ctx = initial_context;
					ctx.bind(build_for.variable, build_doc);
					ctx.bind(probe_for.variable, doc);
				
					// Process LET bindings using LetEvaluator to ensure nested references resolve correctly
					query::LetEvaluator letEval;
					if (secIdx_) {
					  letEval.setSecondaryIndexManager(secIdx_);
					}
					nlohmann::json currentDoc;
					currentDoc.emplace(build_for.variable, build_doc);
					currentDoc.emplace(probe_for.variable, doc);
					for (const auto& let : let_nodes) {
						if (!letEval.evaluateLet(let, currentDoc)) {
							THEMIS_WARN("LET evaluation failed for variable '{}' in hash-join", let.variable);
							continue;
						}
						auto letVal = letEval.resolveVariable(let.variable);
						if (letVal.has_value()) {
							ctx.bind(let.variable, letVal.value());
							currentDoc[let.variable] = letVal.value();
						}
					}
				
					bool passFilters = true;
					for (const auto& filter : multi_var_filters) {
						// Skip the equi-join condition that we already handled via hash table lookup
						if (filter->condition->getType() == query::ASTNodeType::BinaryOp) {
							auto bin = std::static_pointer_cast<query::BinaryOpExpr>(filter->condition);
							if (bin->op == query::BinaryOperator::Eq) {
								const bool isJoinPredicate = [&]() {
									if (bin->left->getType() != query::ASTNodeType::FieldAccess) {
									  return false;
									}
									if (bin->right->getType() != query::ASTNodeType::FieldAccess) {
									  return false;
									}
									auto lfa = std::static_pointer_cast<query::FieldAccessExpr>(bin->left);
									auto rfa = std::static_pointer_cast<query::FieldAccessExpr>(bin->right);
									if (lfa->object->getType() != query::ASTNodeType::Variable) {
									  return false;
									}
									if (rfa->object->getType() != query::ASTNodeType::Variable) {
									  return false;
									}
									auto lvar = std::static_pointer_cast<query::VariableExpr>(lfa->object)->name;
									auto rvar = std::static_pointer_cast<query::VariableExpr>(rfa->object)->name;
									return (lvar == equiJoin.left_var && lfa->field == equiJoin.left_field &&
									        rvar == equiJoin.right_var && rfa->field == equiJoin.right_field);
								}();
								if (isJoinPredicate) {
									continue;
								}
							}
						}
					
						if (!evaluateCondition(filter->condition, ctx)) {
							passFilters = false;
							break;
						}
					}
				
					if (!passFilters) {
						continue;
					}
				
					if (return_node) {
						auto result_or_err = evaluateExpression(return_node->expression, ctx);
						if (!result_or_err) {
							// Expression evaluation failed - log and skip this result
							THEMIS_WARN("Expression evaluation failed in join: {}", result_or_err.error().message());
							continue;
						}
						results.emplace_back(std::move(*result_or_err));
					}
				}
			};
			
			// Execute probe based on CTE or table
			if (probe_cte.has_value()) {
				// Probe from CTE
				auto ordered_probe_docs = probe_cte.value();
				std::sort(ordered_probe_docs.begin(), ordered_probe_docs.end(), stableJsonLess);
				for (const auto& doc : ordered_probe_docs) {
					processProbeDoc(doc);
				}
			} else {
				// Probe from table scan
				const std::string probe_prefix = KeySchema::makeRelationalKey(probe_for.collection, "");
				std::vector<nlohmann::json> ordered_probe_docs;
				db_->scanPrefix(probe_prefix, [&](std::string_view key, std::string_view value) -> bool {
					std::string pk = KeySchema::extractPrimaryKey(key);
					std::vector<uint8_t> blob(value.begin(), value.end());
					try {
						BaseEntity entity = BaseEntity::deserialize(pk, blob);
						nlohmann::json doc = nlohmann::json::parse(entity.toJson());
						doc["_key"] = pk;
						ordered_probe_docs.emplace_back(std::move(doc));
					} catch (...) {}
					return true;
				});
				std::sort(ordered_probe_docs.begin(), ordered_probe_docs.end(), stableJsonLess);
				for (const auto& doc : ordered_probe_docs) {
					processProbeDoc(doc);
				}
			}
			
			// Apply SORT/LIMIT and return
			goto apply_sort_limit;
		}
	}
	
	// FALLBACK: Nested-loop join implementation
	{
		span.setAttribute("join.algorithm", "nested_loop");

		// FOR var1 IN coll1 FOR var2 IN coll2 FILTER var1.x == var2.y RETURN {v1: var1, v2: var2}
		
		std::function<void(size_t, EvaluationContext)> nestedLoop;
		nestedLoop = [&](size_t depth, EvaluationContext ctx) {
			if (depth >= for_nodes.size()) {
				// Process LET bindings using LetEvaluator
				query::LetEvaluator letEval;
				if (secIdx_) {
				  letEval.setSecondaryIndexManager(secIdx_);
				}
				nlohmann::json currentDoc; // Aggregate all bindings for LET evaluation
				std::vector<std::pair<std::string, nlohmann::json>> sorted_bindings;
				sorted_bindings.reserve(ctx.bindings.size());
				for (const auto& [var, val] : ctx.bindings) {
					sorted_bindings.emplace_back(var, val);
				}
				std::sort(sorted_bindings.begin(), sorted_bindings.end(),
					[](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
				for (const auto& [var, val] : sorted_bindings) {
					currentDoc.emplace(var, val);
				}
				
				for (const auto& let : let_nodes) {
					// Evaluate LET with LetEvaluator for proper variable resolution
					if (!letEval.evaluateLet(let, currentDoc)) {
						THEMIS_WARN("LET evaluation failed for variable '{}', skipping result", let.variable);
						return;
					}
					// Bind LET variable to context for downstream use
					auto letVal = letEval.resolveVariable(let.variable);
					if (letVal.has_value()) {
						ctx.bind(let.variable, letVal.value());
						currentDoc[let.variable] = letVal.value(); // Update for subsequent LETs
					}
				}
				
				// Apply multi-variable FILTER conditions
				bool passFilters = true;
				for (const auto& filter : multi_var_filters) {
					if (!evaluateCondition(filter->condition, ctx)) {
						passFilters = false;
						break;
					}
				}
				
				if (!passFilters) {
				  return;
				}
				
				// Evaluate RETURN expression
				if (return_node) {
					auto result_or_err = evaluateExpression(return_node->expression, ctx);
					if (!result_or_err) {
						// Expression evaluation failed - log and skip this result
						THEMIS_WARN("Expression evaluation failed in nested loop join: {}", result_or_err.error().message());
						return;
					}
					results.emplace_back(std::move(*result_or_err));
				}
				return;
			}
			
			// Load all documents from current collection
			const auto& for_node = for_nodes[depth];
			
			// Phase 4.4: Check if collection is a CTE
			auto cte_data = ctx.getCTE(for_node.collection);
			if (cte_data.has_value()) {
				// Iterate over CTE results instead of table scan
				auto ordered_cte_docs = cte_data.value();
				std::sort(ordered_cte_docs.begin(), ordered_cte_docs.end(), stableJsonLess);
				for (const auto& doc : ordered_cte_docs) {
					// Apply pushed-down filters
					auto push_filters = single_var_filters.find(for_node.variable);
					if (push_filters != single_var_filters.end()) {
						EvaluationContext filter_ctx = ctx;
						filter_ctx.bind(for_node.variable, doc);
						bool pass = true;
						for (const auto& filter : push_filters->second) {
							if (!evaluateCondition(filter->condition, filter_ctx)) {
								pass = false;
								break;
							}
						}
						if (!pass) continue; // Skip this result
					}
					
					// Bind current variable
					EvaluationContext newCtx = ctx;
					newCtx.bind(for_node.variable, doc);
					
					// Recurse to next FOR level
					nestedLoop(depth + 1, newCtx);
				}
				return; // CTE iteration complete
			}
			
			// Regular table scan
			const std::string prefix = KeySchema::makeRelationalKey(for_node.collection, "");
			
			// Get pushed-down filters for this variable
			auto push_filters = single_var_filters.find(for_node.variable);
			std::vector<nlohmann::json> ordered_scan_docs;
			
			db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) -> bool {
				// Extract PK
				std::string pk = KeySchema::extractPrimaryKey(key);
				
				// Deserialize entity
				std::vector<uint8_t> blob(value.begin(), value.end());
				try {
					BaseEntity entity = BaseEntity::deserialize(pk, blob);
					nlohmann::json doc = nlohmann::json::parse(entity.toJson());
					doc["_key"] = pk;
					
					// Apply pushed-down filters
					if (push_filters != single_var_filters.end()) {
						EvaluationContext filter_ctx = ctx;
						filter_ctx.bind(for_node.variable, doc);
						bool pass = true;
						for (const auto& filter : push_filters->second) {
							if (!evaluateCondition(filter->condition, filter_ctx)) {
								pass = false;
								break;
							}
						}
						if (!pass) return true; // Skip this document
					}

					ordered_scan_docs.emplace_back(std::move(doc));
				} catch (...) {
        THEMIS_WARN("query_engine: unhandled exception caught");
					// Skip malformed entities
				}
				return true; // Continue iteration
			});

			std::sort(ordered_scan_docs.begin(), ordered_scan_docs.end(), stableJsonLess);

			for (const auto& doc : ordered_scan_docs) {
				// Bind current variable
				EvaluationContext newCtx = ctx;
				newCtx.bind(for_node.variable, doc);

				// Recurse to next FOR level
				nestedLoop(depth + 1, newCtx);
			}
		};
		
		nestedLoop(0, initial_context);
	}
	
apply_sort_limit:
	// Apply SORT if specified
	if (sort && !sort->specifications.empty()) {
		const auto& spec = sort->specifications[0];
		tbb::parallel_sort(results.begin(), results.end(), [&](const nlohmann::json& a, const nlohmann::json& b) {
			EvaluationContext ctxA, ctxB;
			ctxA.bindings["doc"] = a;
			ctxB.bindings["doc"] = b;
			auto valA_or_err = evaluateExpression(spec.expression, ctxA);
			auto valB_or_err = evaluateExpression(spec.expression, ctxB);
			// If either evaluation fails, fall back to comparing original jsons
			if (!valA_or_err || !valB_or_err) {
				return stableJsonLess(a, b);
			}
			if (*valA_or_err == *valB_or_err) {
				return stableJsonLess(a, b);
			}
			return spec.ascending ? (*valA_or_err < *valB_or_err) : (*valA_or_err > *valB_or_err);
		});
	} else {
		std::sort(results.begin(), results.end(), stableJsonLess);
	}
	
	// Apply LIMIT if specified
	if (limit) {
		// Guard against negative int64_t values: cast to size_t only after clamping.
		size_t offset = (limit->offset <= 0) ? 0 : static_cast<size_t>(limit->offset);
		size_t count  = (limit->count  <= 0) ? 0 : static_cast<size_t>(limit->count);
		if (offset >= results.size()) {
			results.clear();
		} else {
			size_t end = std::min(offset + count, results.size());
			results = std::vector<nlohmann::json>(
				results.begin() + offset,
				results.begin() + end
			);
		}
	}
	
	span.setAttribute("join.result_count", static_cast<int64_t>(results.size()));
	span.setStatus(true);
	return Ok(std::move(results));
}

Result<std::vector<nlohmann::json>> QueryEngine::executeGroupBy(
	const query::ForNode& for_node,
	const std::shared_ptr<query::CollectNode>& collect,
	const std::vector<std::shared_ptr<query::FilterNode>>& filters,
	const std::shared_ptr<query::ReturnNode>& return_node
) const {
	auto span = Tracer::startSpan("QueryEngine.executeGroupBy");
	
	if (!collect || collect->groups.empty()) {
		return Err<std::vector<nlohmann::json>>(
			errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			"executeGroupBy: No GROUP BY clause provided"
		);
	}
	
	// Hash-based grouping
	std::unordered_map<std::string, std::vector<nlohmann::json>> groups;
	
	// Scan collection
	const std::string prefix = KeySchema::makeRelationalKey(for_node.collection, "");
	
	db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) -> bool {
		std::string pk = KeySchema::extractPrimaryKey(key);
		std::vector<uint8_t> blob(value.begin(), value.end());
		
		try {
			BaseEntity entity = BaseEntity::deserialize(pk, blob);
			nlohmann::json doc = nlohmann::json::parse(entity.toJson());
			doc["_key"] = pk;
			
			EvaluationContext ctx;
			ctx.bind(for_node.variable, doc);
			
			// Apply FILTER conditions
			bool passFilters = true;
			for (const auto& filter : filters) {
				if (!evaluateCondition(filter->condition, ctx)) {
					passFilters = false;
					break;
				}
			}
			if (!passFilters) return true; // Continue to next document
			
			// Evaluate group key
			auto groupKey_or_err = evaluateExpression(collect->groups[0].second, ctx);
			if (!groupKey_or_err) {
				// Failed to evaluate group key - skip this document
				THEMIS_WARN("Failed to evaluate group key: {}", groupKey_or_err.error().message());
				return true;
			}
			std::string key_str = groupKey_or_err.value().dump();
			
			auto [group_it, inserted] = groups.try_emplace(std::move(key_str));
			group_it->second.push_back(doc);
		} catch (...) {
      THEMIS_WARN("query_engine: unhandled exception caught");
			// Skip malformed entities
		}
		return true; // Continue iteration
	});
	
	// Compute aggregations
	std::vector<nlohmann::json> results;
	results.reserve(groups.size());
	std::vector<std::string> sorted_group_keys;
	sorted_group_keys.reserve(groups.size());
	for (const auto& [group_key, _] : groups) {
		sorted_group_keys.push_back(group_key);
	}
	std::sort(sorted_group_keys.begin(), sorted_group_keys.end());
	
	for (const auto& key_str : sorted_group_keys) {
		const auto& docs = groups.at(key_str);
		std::vector<const nlohmann::json*> ordered_docs;
		ordered_docs.reserve(docs.size());
		for (const auto& doc : docs) {
			ordered_docs.push_back(&doc);
		}
		std::sort(ordered_docs.begin(), ordered_docs.end(), stableJsonPtrLess);
		EvaluationContext ctx;
		
		// Bind group key variable
		if (!collect->groups.empty()) {
			auto groupKey = nlohmann::json::parse(key_str);
			ctx.bind(collect->groups[0].first, groupKey);
		}
		
		// Compute aggregations
		for (const auto& agg : collect->aggregations) {
			nlohmann::json aggValue = nullptr;
			
			std::string funcUpper = agg.funcName;
			std::transform(funcUpper.begin(), funcUpper.end(), funcUpper.begin(), ::toupper);
			
			if (funcUpper == "COUNT") {
				aggValue = static_cast<int64_t>(docs.size());
			} else if (funcUpper == "SUM") {
				double sum = 0.0;
				for (const auto* doc : ordered_docs) {
					EvaluationContext docCtx;
					docCtx.bind(for_node.variable, *doc);
					auto val_or_err = evaluateExpression(agg.argument, docCtx);
					if (val_or_err && val_or_err.value().is_number()) {
						sum += val_or_err.value().get<double>();
					}
				}
				aggValue = sum;
			} else if (funcUpper == "AVG") {
				double sum = 0.0;
				int count = 0;
				for (const auto* doc : ordered_docs) {
					EvaluationContext docCtx;
					docCtx.bind(for_node.variable, *doc);
					auto val_or_err = evaluateExpression(agg.argument, docCtx);
					if (val_or_err && val_or_err.value().is_number()) {
						sum += val_or_err.value().get<double>();
						count++;
					}
				}
				aggValue = (count > 0) ? (sum / count) : 0.0;
			} else if (funcUpper == "MIN") {
				double minVal = std::numeric_limits<double>::max();
				for (const auto* doc : ordered_docs) {
					EvaluationContext docCtx;
					docCtx.bind(for_node.variable, *doc);
					auto val_or_err = evaluateExpression(agg.argument, docCtx);
					if (val_or_err && val_or_err.value().is_number()) {
						minVal = std::min(minVal, val_or_err.value().get<double>());
					}
				}
				aggValue = minVal;
			} else if (funcUpper == "MAX") {
				double maxVal = std::numeric_limits<double>::lowest();
				for (const auto* doc : ordered_docs) {
					EvaluationContext docCtx;
					docCtx.bind(for_node.variable, *doc);
					auto val_or_err = evaluateExpression(agg.argument, docCtx);
					if (val_or_err && val_or_err.value().is_number()) {
						maxVal = std::max(maxVal, val_or_err.value().get<double>());
					}
				}
				aggValue = maxVal;
			}
			
			ctx.bind(agg.varName, std::move(aggValue));
		}
		
		// Evaluate RETURN expression
		if (return_node) {
			auto result_or_err = evaluateExpression(return_node->expression, ctx);
			if (!result_or_err) {
				// Expression evaluation failed - log and skip this group
				THEMIS_WARN("Expression evaluation failed in group-by return: {}", result_or_err.error().message());
				continue;
			}
			results.emplace_back(std::move(*result_or_err));
		}
	}
	
	span.setAttribute("groupby.group_count", static_cast<int64_t>(results.size()));
	span.setStatus(true);
	return Ok(std::move(results));
}

// Forward declaration for helper function
static std::optional<utils::geo::MBR> extractBBoxFromFilter(
    const std::shared_ptr<themis::query::Expression>& expr
);

// Recursive Path Query Implementation (Multi-Hop Traversal with Temporal Support)
Result<std::vector<std::vector<std::string>>>
QueryEngine::executeRecursivePathQuery(const RecursivePathQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeRecursivePathQuery");
	span.setAttribute("query.start_node", q.start_node);
	span.setAttribute("query.end_node", q.end_node);
	span.setAttribute("query.max_depth", static_cast<int64_t>(q.max_depth));
	constexpr size_t kMaxTraversalDepth = 100;
	const size_t boundedDepth = std::min(q.max_depth, kMaxTraversalDepth);
	const int bfsDepth = static_cast<int>(
		std::min(boundedDepth, static_cast<size_t>(std::numeric_limits<int>::max())));
	span.setAttribute("query.max_depth_effective", static_cast<int64_t>(boundedDepth));
	
	if (!graphIdx_) {
		return Err<std::vector<std::vector<std::string>>>(ErrorCode::ERR_INDEX_NOT_FOUND, "GraphIndexManager nicht verfügbar");
	}
	
	if (q.start_node.empty()) {
		return Err<std::vector<std::vector<std::string>>>(ErrorCode::ERR_QUERY_INVALID_INPUT, "start_node darf nicht leer sein");
	}
	
	// Dynamische Branching-Faktor Schätzung (Sampling über erste 2 Tiefen)
	size_t sampledEdges = 0; size_t sampledVertices = 0; double branchingEstimate = 0.0;
	{
		std::unordered_set<std::string> frontier{q.start_node};
		for (size_t depth=0; depth<2 && !frontier.empty(); ++depth) {
			std::unordered_set<std::string> next;
			for (const auto &v : frontier) {
				auto [st, adj] = graphIdx_->outAdjacency(v);
				if (!st.ok) {
				  continue;
				}
				// Optional edge type filter
				for (auto &ai : adj) {
					if (!q.edge_type.empty() && ai.graphId != q.edge_type) continue; // simplistic match, adjust if edge_type stored differently
					next.insert(ai.targetPk);
				}
				sampledVertices++;
				sampledEdges += adj.size();
			}
			frontier.swap(next);
		}
		if (sampledVertices>0) {
		  branchingEstimate = static_cast<double>(sampledEdges) / static_cast<double>(sampledVertices);
		}
	}
	if (branchingEstimate <= 0.0) {
	  branchingEstimate = 1.0;
	}
	// Räumliche Selektivität schätzen wenn Constraint vorhanden (vereinfachte Annahme: bboxRatio falls extrahierbar)
	double spatialSelectivity = 1.0;
	if (q.spatial_constraint && q.spatial_constraint->spatial_filter) {
		auto bbox = extractBBoxFromFilter(q.spatial_constraint->spatial_filter);
		if (bbox && spatialIdx_ && spatialIdx_->hasSpatialIndex("vertices")) { // Tabelle "vertices" hypothetisch
			auto stats = spatialIdx_->getStats("vertices");
			double totalArea = (std::max)((stats.total_bounds.maxx - stats.total_bounds.minx) * (stats.total_bounds.maxy - stats.total_bounds.miny), 1e-9);
			double bboxArea = (std::max)((bbox->maxx - bbox->minx) * (bbox->maxy - bbox->miny), 0.0);
			spatialSelectivity = (std::min)((std::max)(bboxArea / totalArea, 0.0), 1.0);
		}
	}
	QueryOptimizer::GraphPathCostInput gci; gci.maxDepth = boundedDepth; gci.branchingFactor = static_cast<size_t>(std::ceil(branchingEstimate)); gci.hasSpatialConstraint = q.spatial_constraint.has_value(); gci.spatialSelectivity = spatialSelectivity;
	auto gcr = QueryOptimizer::estimateGraphPath(gci);
	span.setAttribute("optimizer.graph.branching_estimate", static_cast<int64_t>(branchingEstimate));
	span.setAttribute("optimizer.graph.expanded_estimate", static_cast<int64_t>(gcr.estimatedExpandedVertices));
	span.setAttribute("optimizer.graph.time_ms_estimate", gcr.estimatedTimeMs);
	// Frühabbruch bei sehr großer Expansion
	const double ABORT_THRESHOLD = 1e6; // heuristisch
	if (gcr.estimatedExpandedVertices > ABORT_THRESHOLD) {
		span.setAttribute("optimizer.graph.aborted", true);
		span.setStatus(true);
		return Ok(std::vector<std::vector<std::string>>{}); // leere Pfadliste als Schutz vor Explosion
	}
	// Temporal filter setup
	// REL-20: wrap stoll() calls in try/catch — valid_from/valid_to are user-supplied
	// strings; malformed or out-of-range values must not propagate an unhandled exception.
	auto parseTimestampMs = [](const std::string& s) -> std::optional<int64_t> {
		try {
			return std::stoll(s);
		} catch (...) {
      THEMIS_DEBUG("query_engine: unhandled exception caught");
			return std::nullopt;
		}
	};
	std::optional<int64_t> timestamp_ms;
	if (q.valid_from.has_value() && q.valid_to.has_value()) {
		auto from = parseTimestampMs(*q.valid_from);
		auto to   = parseTimestampMs(*q.valid_to);
		if (from.has_value() && to.has_value()) {
			// Use midpoint of time window as query timestamp
			timestamp_ms = (*from + *to) / 2;
		}
	} else if (q.valid_from.has_value()) {
		timestamp_ms = parseTimestampMs(*q.valid_from);
	} else if (q.valid_to.has_value()) {
		timestamp_ms = parseTimestampMs(*q.valid_to);
	}
	
	std::vector<std::vector<std::string>> allPaths;
	
	// If end_node is specified, use Dijkstra for single shortest path (early exit optimizations)
	if (!q.end_node.empty()) {
		GraphIndexManager::PathResult pathResult;
		GraphIndexManager::Status st;
		
	// Use edge_type filtering if specified
	bool hasTypeFilter = !q.edge_type.empty();
	std::string graphId = q.graph_id.empty() ? std::string("default") : q.graph_id;
		
		if (timestamp_ms.has_value()) {
			auto [status, result] = graphIdx_->dijkstraAtTime(q.start_node, q.end_node, *timestamp_ms);
			st = status;
			pathResult = result;
		} else if (hasTypeFilter) {
			auto [status, result] = graphIdx_->dijkstra(q.start_node, q.end_node, q.edge_type, graphId);
			st = status;
			pathResult = result;
		} else {
			auto [status, result] = graphIdx_->dijkstra(q.start_node, q.end_node);
			st = status;
			pathResult = result;
		}
		
		// "No path found" is not an error, just an empty result
		if (!st.ok && st.message.find("Kein Pfad gefunden") == std::string::npos) {
			span.setStatus(false, st.message);
			return Err<std::vector<std::vector<std::string>>>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, st.message);
		}
		// Early exit: if shortestPath flag set (from AQL sugar) and path found, skip spatial filtering unless required
		// bool needSpatial = q.spatial_constraint.has_value(); // unused currently
		if (q.end_node == q.start_node) {
			// Trivial path
			allPaths.emplace_back(std::vector<std::string>{q.start_node});
			span.setAttribute("query.path_count", static_cast<int64_t>(1));
			span.setStatus(true);
			return Ok(std::move(allPaths));
		}
		
		// Graph + Geo: Apply spatial filter to path vertices
		if (!pathResult.path.empty() && q.spatial_constraint.has_value()) {
			auto spatialSpan = Tracer::startSpan("spatial_filter_path");
			const auto& sc = *q.spatial_constraint;
			
			std::vector<std::string> filteredPath;
			bool pathValid = true;
			
			// Batch load all vertices in path
			std::vector<std::string> vertexKeys;
			vertexKeys.reserve(pathResult.path.size());
			for (const auto& vertexPk : pathResult.path) {
				// Extract table from PK format "collection/id" -> DB key is "collection:collection/id"
				std::string table;
				auto slashPos = vertexPk.find('/');
				if (slashPos != std::string::npos) {
					table = vertexPk.substr(0, slashPos);
				} else {
					table = "default"; // fallback
				}
				vertexKeys.emplace_back(table + ":" + vertexPk);
			}
			
			auto vertexDataList = db_->multiGet(vertexKeys);
			
			// Evaluate spatial filter for each vertex
			for (size_t i = 0; i < pathResult.path.size(); ++i) {
				const auto& vertexPk = pathResult.path[i];
				const auto& vertexDataOpt = vertexDataList[i];
				
				if (!vertexDataOpt.has_value()) {
					pathValid = false;
					break;
				}
				
				nlohmann::json vertex;
				try {
					auto entity = BaseEntity::deserialize(vertexPk, *vertexDataOpt);
					vertex = nlohmann::json::parse(entity.toJson());
				} catch (...) {
        THEMIS_DEBUG("query_engine: unhandled exception caught");
					pathValid = false;
					break;
				}
				
				// Evaluate spatial filter
				EvaluationContext ctx;
				ctx.bind("v", vertex);
				
				if (!evaluateCondition(sc.spatial_filter, ctx)) {
					pathValid = false; // Vertex outside spatial constraint
					break;
				}
				
				filteredPath.push_back(vertexPk);
			}
			
			if (pathValid && !filteredPath.empty()) {
				allPaths.emplace_back(std::move(filteredPath));
			}
			
			spatialSpan.setAttribute("path_valid", pathValid);
			spatialSpan.setAttribute("batch_loaded", static_cast<int64_t>(pathResult.path.size()));
			spatialSpan.setStatus(true);
		} else if (!pathResult.path.empty()) {
			allPaths.emplace_back(std::move(pathResult.path));
		}
	} else {
		// No end_node: BFS to find all reachable nodes up to max_depth; optimize with optional spatial constraint pruning
		std::vector<std::string> reachableNodes;
		GraphIndexManager::Status st;
		
	// Use edge_type filtering if specified
	bool hasTypeFilter = !q.edge_type.empty();
	std::string graphId = q.graph_id.empty() ? std::string("default") : q.graph_id;
		
		if (timestamp_ms.has_value()) {
			auto [status, nodes] = graphIdx_->bfsAtTime(q.start_node, *timestamp_ms, bfsDepth);
			st = status;
			reachableNodes = std::move(nodes);
		} else if (hasTypeFilter) {
			auto [status, nodes] = graphIdx_->bfs(q.start_node, bfsDepth, q.edge_type, graphId);
			st = status;
			reachableNodes = std::move(nodes);
		} else {
			auto [status, nodes] = graphIdx_->bfs(q.start_node, bfsDepth);
			st = status;
			reachableNodes = std::move(nodes);
		}
		
		if (!st.ok) {
			span.setStatus(false, st.message);
			return Err<std::vector<std::vector<std::string>>>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, st.message);
		}
		
		// Graph + Geo: Apply spatial filter to reachable nodes (early pruning)
		if (q.spatial_constraint.has_value()) {
			auto spatialSpan = Tracer::startSpan("spatial_filter_nodes");
			const auto& sc = *q.spatial_constraint;
			std::vector<std::string> filteredNodes;
			
			// Batch load all reachable vertices - need to add table prefix
			std::vector<std::string> vertexKeys;
			vertexKeys.reserve(reachableNodes.size());
			for (const auto& vertexPk : reachableNodes) {
				// Extract table from PK format "collection/id" -> DB key is "collection:collection/id"
				std::string table;
				auto slashPos = vertexPk.find('/');
				if (slashPos != std::string::npos) {
					table = vertexPk.substr(0, slashPos);
				} else {
					table = "default"; // fallback
				}
				vertexKeys.emplace_back(table + ":" + vertexPk);
			}
			auto vertexDataList = db_->multiGet(vertexKeys);
			
			// Evaluate spatial filter for each vertex (parallel)
			const size_t n = reachableNodes.size();
			const size_t T = std::max<unsigned>(1u, std::thread::hardware_concurrency());
			const size_t CHUNK = std::max<std::size_t>(128, (n + T - 1) / T);
			std::vector<std::vector<std::string>> buckets((n + CHUNK - 1) / CHUNK);
			tbb::task_group tg3;
			for (size_t bi = 0; bi < buckets.size(); ++bi) {
				tg3.run([&, bi]() {
					size_t start = bi * CHUNK;
					size_t end = std::min(start + CHUNK, n);
					std::vector<std::string> buf;
					buf.reserve(end - start);
					for (size_t i = start; i < end; ++i) {
						const auto& vertexPk = reachableNodes[i];
						const auto& vertexDataOpt = vertexDataList[i];
						if (!vertexDataOpt.has_value()) {
						  continue;
						}
						nlohmann::json vertex;
						try { auto entity = BaseEntity::deserialize(vertexPk, *vertexDataOpt); vertex = nlohmann::json::parse(entity.toJson()); }
						catch (...) { continue; }
						EvaluationContext ctx; ctx.bind("v", vertex);
						if (evaluateCondition(sc.spatial_filter, ctx)) {
						  buf.push_back(vertexPk);
						}
					}
					buckets[bi] = std::move(buf);
				});
			}
				// Timeout enforcement via helper (Q1/REL-50): graph spatial filter
				{
					const auto audit_config = snapshotAuditConfig();
					tbbWaitWithTimeout(tg3, audit_config.audit_logger, audit_config.query_timeout_ms, "graph_spatial_filter");
				}
			for (auto& b : buckets) {
				filteredNodes.insert(filteredNodes.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
			}
			
			reachableNodes = std::move(filteredNodes);
			spatialSpan.setAttribute("filtered_nodes", static_cast<int64_t>(reachableNodes.size()));
			spatialSpan.setAttribute("batch_loaded", static_cast<int64_t>(vertexDataList.size()));
			spatialSpan.setStatus(true);
		}

		std::sort(reachableNodes.begin(), reachableNodes.end());
		
		// Reconstruct actual multi-hop paths from start_node to each reachable
		// node using Dijkstra.  For large result sets we cap the number of
		// individual path lookups to avoid O(N²) worst-case runtime; nodes
		// beyond the cap are represented as trivial 2-hop paths.
		static constexpr size_t kMaxPathLookups = 500;
		for (size_t i = 0; i < reachableNodes.size(); ++i) {
			const auto& node = reachableNodes[i];
			if (node == q.start_node) {
				continue;
			}
			if (i < kMaxPathLookups) {
				// Attempt to reconstruct the full path via Dijkstra.
				GraphIndexManager::PathResult pathResult;
				if (!q.edge_type.empty()) {
					std::string graphIdForPath = q.graph_id.empty() ? std::string("default") : q.graph_id;
					auto dijkstraResult = graphIdx_->dijkstra(q.start_node, node, q.edge_type, graphIdForPath);
					st = dijkstraResult.first;
					pathResult = std::move(dijkstraResult.second);
				} else {
					auto dijkstraResult = graphIdx_->dijkstra(q.start_node, node);
					st = dijkstraResult.first;
					pathResult = std::move(dijkstraResult.second);
				}
				if (st.ok && !pathResult.path.empty()) {
					allPaths.emplace_back(std::move(pathResult.path));
				} else {
					// Dijkstra unavailable for this pair — fall back to 2-hop
					allPaths.emplace_back(std::vector<std::string>{q.start_node, node});
				}
			} else {
				// Beyond lookup cap: emit a minimal reachability path
				allPaths.emplace_back(std::vector<std::string>{q.start_node, node});
			}
		}
	}
	
	span.setAttribute("query.path_count", static_cast<int64_t>(allPaths.size()));
	span.setStatus(true);
	return Ok(std::move(allPaths));
}

// ============================================================================
// General Graph Traversal (Non-Shortest Path)
// ============================================================================

Result<std::vector<TraversalResult>>
QueryEngine::executeGeneralTraversal(
    const std::string& startVertex,
    int minDepth,
    int maxDepth,
    TraversalDirection direction,
    const std::string& graphId,
    const std::string& edgeTypeFilter
) const {
	auto span = Tracer::startSpan("QueryEngine.executeGeneralTraversal");
	span.setAttribute("query.start_vertex", startVertex);
	span.setAttribute("query.min_depth", static_cast<int64_t>(minDepth));
	span.setAttribute("query.max_depth", static_cast<int64_t>(maxDepth));
	span.setAttribute("query.graph_id", graphId);
	if (!edgeTypeFilter.empty()) {
		span.setAttribute("query.edge_type_filter", edgeTypeFilter);
	}
	
	if (!graphIdx_) {
		return Err<std::vector<TraversalResult>>(ErrorCode::ERR_INDEX_NOT_FOUND, "GraphIndexManager not available");
	}
	
	if (startVertex.empty()) {
		return Err<std::vector<TraversalResult>>(ErrorCode::ERR_QUERY_INVALID_INPUT, "startVertex cannot be empty");
	}
	
	if (minDepth < 0 || maxDepth < minDepth) {
		return Err<std::vector<TraversalResult>>(ErrorCode::ERR_QUERY_INVALID_INPUT, "Invalid depth range: minDepth=" + std::to_string(minDepth) + 
		                      ", maxDepth=" + std::to_string(maxDepth));
	}
	
	// Safety limit to prevent excessive memory consumption
	const size_t MAX_RESULTS = 100000;  // Configurable limit
	
	std::vector<TraversalResult> results;
	
	// BFS with depth tracking
	struct VisitInfo {
		std::string vertex;
		int depth;
		std::vector<std::string> path;   // vertex PKs from start
		std::vector<std::string> edges;  // edge IDs traversed
	};
	
	std::queue<VisitInfo> queue;
	std::unordered_set<std::string> visited;
	
	// Start with the initial vertex
	queue.push({startVertex, 0, {startVertex}, {}});
	visited.insert(startVertex);
	
	while (!queue.empty()) {
		VisitInfo current = std::move(queue.front());
		queue.pop();
		
		// Add to results if within depth range
		if (current.depth >= minDepth && current.depth <= maxDepth) {
			TraversalResult result;
			result.vertex_pk = current.vertex;
			result.depth = current.depth;
			result.path = current.path;
			result.edges = current.edges;
			
			// Try to load vertex data from storage
			// Extract table from PK format "collection/id"
			// Note: Uses standard key format "table:pk" consistent with rest of codebase
			std::string table;
			auto slashPos = current.vertex.find('/');
			if (slashPos != std::string::npos) {
				table = current.vertex.substr(0, slashPos);
			} else {
				table = "vertices"; // default fallback
			}
			
			std::string dbKey = table + ":" + current.vertex;
			auto vertexDataOpt = db_->get(dbKey);
			if (vertexDataOpt.has_value()) {
				try {
					result.vertex_data = nlohmann::json::parse(*vertexDataOpt);
				} catch (...) {
        THEMIS_DEBUG("query_engine: unhandled exception caught");
					// If parsing fails, create minimal JSON
					result.vertex_data = nlohmann::json{{"_key", current.vertex}};
				}
			} else {
				// Vertex not found in storage, create minimal JSON
				result.vertex_data = nlohmann::json{{"_key", current.vertex}};
			}
			
			results.emplace_back(std::move(result));
			
			// Check result size limit to prevent memory exhaustion
			if (results.size() >= MAX_RESULTS) {
				span.setAttribute("query.result_limit_reached", true);
				span.setStatus(true);
				return Ok(std::move(results));
			}
		}
		
		// Don't expand beyond maxDepth
		if (current.depth >= maxDepth) {
			continue;
		}
		
		// Get neighbors based on direction
		std::vector<GraphIndexManager::AdjacencyInfo> neighbors;
		
		switch (direction) {
			case TraversalDirection::OUTBOUND: {
				auto [st, adj] = graphIdx_->outAdjacency(current.vertex);
				if (st.ok) {
				  neighbors = std::move(adj);
				}
				break;
			}
			case TraversalDirection::INBOUND: {
				auto [st, adj] = graphIdx_->inAdjacency(current.vertex);
				if (st.ok) {
				  neighbors = std::move(adj);
				}
				break;
			}
			case TraversalDirection::ANY: {
				// Get both outbound and inbound neighbors
				auto [st1, adj1] = graphIdx_->outAdjacency(current.vertex);
				auto [st2, adj2] = graphIdx_->inAdjacency(current.vertex);
				if (st1.ok) {
				  neighbors = std::move(adj1);
				}
				if (st2.ok) {
					neighbors.insert(neighbors.end(), 
					                std::make_move_iterator(adj2.begin()),
					                std::make_move_iterator(adj2.end()));
				}
				break;
			}
		}
		
		// Filter by graph ID and optional edge type
		std::sort(neighbors.begin(), neighbors.end(), [](const auto& a, const auto& b) {
			if (a.targetPk != b.targetPk) {
				return a.targetPk < b.targetPk;
			}
			if (a.edgeId != b.edgeId) {
				return a.edgeId < b.edgeId;
			}
			return a.graphId < b.graphId;
		});
		for (const auto& adj : neighbors) {
			// Filter by graph ID - adj.graphId contains the graph namespace
			if (!graphId.empty() && graphId != "default" && !adj.graphId.empty() && adj.graphId != graphId) {
				continue;
			}

			// Edge type filter: adj.graphId doubles as edge type identifier
			// (same convention as RecursivePathQuery::edge_type).
			if (!edgeTypeFilter.empty() && !adj.graphId.empty() && adj.graphId != edgeTypeFilter) {
				continue;
			}
			
			// Skip already visited vertices (cycle prevention)
			if (visited.find(adj.targetPk) != visited.end()) {
				continue;
			}
			
			// Add to queue
			VisitInfo next;
			next.vertex = adj.targetPk;
			next.depth = current.depth + 1;
			next.path = current.path;
			next.path.push_back(adj.targetPk);
			next.edges = current.edges;
			next.edges.push_back(adj.edgeId);
			
			queue.push(std::move(next));
			visited.insert(adj.targetPk);
		}
	}
	
	span.setAttribute("query.result_count", static_cast<int64_t>(results.size()));
	span.setStatus(true);
	return Ok(std::move(results));
}

// ============================================================================
// Hybrid Multi-Model Query Implementations
// ============================================================================

// Helper: Extract MBR from spatial filter expression for index optimization
static std::optional<utils::geo::MBR> extractBBoxFromFilter(
    const std::shared_ptr<themis::query::Expression>& expr
) {
    using namespace themis::query;
    if (!expr) {
      return std::nullopt;
    }
    
    // Handle ST_Within(geom, POLYGON(...))
    if (expr->getType() == ASTNodeType::FunctionCall) {
        auto funcCall = std::static_pointer_cast<FunctionCallExpr>(expr);
        
        if (funcCall->name == "ST_Within" && funcCall->arguments.size() == 2) {
            // Second argument should be ST_GeomFromText("POLYGON(...)")
            auto arg2 = funcCall->arguments[1];
            if (arg2->getType() == ASTNodeType::FunctionCall) {
                auto geomFunc = std::static_pointer_cast<FunctionCallExpr>(arg2);
                if (geomFunc->name == "ST_GeomFromText" && geomFunc->arguments.size() == 1) {
                    auto litExpr = geomFunc->arguments[0];
                    if (litExpr->getType() == ASTNodeType::Literal) {
                        auto lit = std::static_pointer_cast<LiteralExpr>(litExpr);
                        if (std::holds_alternative<std::string>(lit->value)) {
                            std::string wkt = std::get<std::string>(lit->value);
                            // Parse POLYGON((minx miny, maxx miny, maxx maxy, minx maxy, minx miny))
                            if (wkt.rfind("POLYGON", 0) == 0) {
                                size_t start = wkt.find("((");
                                size_t end = wkt.find("))");
                                if (start != std::string::npos && end != std::string::npos) {
                                    std::string coords = wkt.substr(start + 2, end - start - 2);
                                    std::replace(coords.begin(), coords.end(), ',', ' ');
                                    std::istringstream iss(coords);
                                    double minx = std::numeric_limits<double>::max();
                                    double miny = std::numeric_limits<double>::max();
                                    double maxx = std::numeric_limits<double>::lowest();
                                    double maxy = std::numeric_limits<double>::lowest();
                                    double x, y;
                                    while (iss >> x >> y) {
                                        minx = std::min(minx, x); miny = std::min(miny, y);
                                        maxx = std::max(maxx, x); maxy = std::max(maxy, y);
                                    }
                                    if (minx <= maxx && miny <= maxy) {
                                        return utils::geo::MBR{minx, miny, maxx, maxy};
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Handle ST_DWithin(geom, ST_Point(x,y), distance)
        if (funcCall->name == "ST_DWithin" && funcCall->arguments.size() == 3) {
            auto arg2 = funcCall->arguments[1];
            auto arg3 = funcCall->arguments[2];
            
            if (arg2->getType() == ASTNodeType::FunctionCall && arg3->getType() == ASTNodeType::Literal) {
                auto pointFunc = std::static_pointer_cast<FunctionCallExpr>(arg2);
                auto distLit = std::static_pointer_cast<LiteralExpr>(arg3);
                
                if (pointFunc->name == "ST_Point" && pointFunc->arguments.size() == 2) {
                    if (pointFunc->arguments[0]->getType() == ASTNodeType::Literal &&
                        pointFunc->arguments[1]->getType() == ASTNodeType::Literal) {
                        
                        auto xLit = std::static_pointer_cast<LiteralExpr>(pointFunc->arguments[0]);
                        auto yLit = std::static_pointer_cast<LiteralExpr>(pointFunc->arguments[1]);
                        
                        double x = std::holds_alternative<double>(xLit->value) ? std::get<double>(xLit->value) : 0.0;
                        double y = std::holds_alternative<double>(yLit->value) ? std::get<double>(yLit->value) : 0.0;
                        double d = std::holds_alternative<double>(distLit->value) ? std::get<double>(distLit->value) : 0.0;
                        
                        return utils::geo::MBR{x - d, y - d, x + d, y + d};
                    }
                }
            }
        }
    }
    
    return std::nullopt;
}

// Hybrid plan tuning read from DB config; defaults are safe
struct HybridVGConfig {
	size_t overfetch = 5;              // multiplier for vector-first plan
	double bbox_ratio_threshold = 0.25; // when bbox covers >= threshold of total area → prefer vector-first
	size_t min_chunk_spatial_eval = 64; // parallel chunk size for spatial eval
	size_t min_chunk_vector_bf = 128;   // parallel chunk size for brute-force vector
};

static HybridVGConfig loadHybridConfig_(RocksDBWrapper& db) {
	HybridVGConfig cfg;
	try {
		auto result = db.get("config:hybrid_query");
		if (result.has_value()) {
			auto j = nlohmann::json::parse(result.value());
			if (j.contains("vector_first_overfetch")) {
			  cfg.overfetch = (std::max)(static_cast<size_t>(1), static_cast<size_t>(j.value("vector_first_overfetch", static_cast<int64_t>(cfg.overfetch))));
			}
			if (j.contains("bbox_ratio_threshold")) {
			  cfg.bbox_ratio_threshold = (std::min)(1.0, (std::max)(0.0, j.value("bbox_ratio_threshold", cfg.bbox_ratio_threshold)));
			}
			if (j.contains("min_chunk_spatial_eval")) {
			  cfg.min_chunk_spatial_eval = (std::max)(static_cast<size_t>(16), static_cast<size_t>(j.value("min_chunk_spatial_eval", static_cast<int64_t>(cfg.min_chunk_spatial_eval))));
			}
			if (j.contains("min_chunk_vector_bf")) {
			  cfg.min_chunk_vector_bf = (std::max)(static_cast<size_t>(64), static_cast<size_t>(j.value("min_chunk_vector_bf", static_cast<int64_t>(cfg.min_chunk_vector_bf))));
			}
		}
	} catch (...) {
     THEMIS_WARN("query_engine::loadHybridConfig_: unhandled exception caught");
		// keep defaults
	}
	return cfg;
}

// Simple cost model for Vector+Geo: decide whether to run spatial pre-filter first
// or vector-first then spatial filter. Uses bbox/total_bounds ratio if available.
enum class VGPlan { SpatialThenVector, VectorThenSpatial };

[[maybe_unused]] static VGPlan chooseVGPlan(
	const VectorGeoQuery& q,
	const index::SpatialIndexManager* spatialIdx,
	const VectorIndexManager* vectorIdx,
	double bbox_ratio_threshold,
	const std::optional<std::vector<std::string>>& eqPrefilter
) {
	if (!vectorIdx) {
	  return VGPlan::SpatialThenVector;
	}
	// If we cannot parse a bbox, prefer vector-first (no good spatial mask)
	auto bbox = extractBBoxFromFilter(q.spatial_filter);
	if (!bbox.has_value()) {
	  return VGPlan::VectorThenSpatial;
	}
	if (!spatialIdx || !spatialIdx->hasSpatialIndex(q.table)) {
	  return VGPlan::SpatialThenVector;
	}
	// Estimate selectivity via bbox area ratio
	auto stats = spatialIdx->getStats(q.table);
	const auto& tb = stats.total_bounds;
	double totalArea = std::max((tb.maxx - tb.minx) * (tb.maxy - tb.miny), 1e-9);
	double bboxArea = std::max((bbox->maxx - bbox->minx) * (bbox->maxy - bbox->miny), 0.0);
	double ratio = std::min(std::max(bboxArea / totalArea, 0.0), 1.0);
	// Integrate equality prefilter cardinality: if strongly selective (< 5% of table estimated) prefer vector-first even if bbox small.
		if (eqPrefilter && !eqPrefilter->empty()) {
			// crude heuristic: treat prefilter size as candidate universe
			if (eqPrefilter->size() < stats.entry_count * 0.05) {
			  return VGPlan::VectorThenSpatial;
			}
		}
	// Heuristic: configurable threshold on bbox ratio
	if (ratio >= bbox_ratio_threshold) {
	  return VGPlan::VectorThenSpatial;
	}
	return VGPlan::SpatialThenVector;
}

// Vector + Geo: Spatial-filtered ANN search
Result<std::vector<QueryEngine::VectorGeoResult>>
QueryEngine::executeVectorGeoQuery(const VectorGeoQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeVectorGeoQuery");
	span.setAttribute("query.table", q.table);
	span.setAttribute("query.k", static_cast<int64_t>(q.k));

	// QE-2: enforce collection-level access control before any I/O
	if (collection_access_checker_ && !collection_access_checker_(q.table, collection_access_caller_id_)) {
		return Err<std::vector<VectorGeoResult>>(
			errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
			"Access denied for collection: " + q.table
		);
	}

	// Erweiterte Index-basierte Vorselektion (Equality, Range, Composite)
	std::optional<std::vector<std::string>> indexPrefilter;
	if (!q.extra_filters.empty()) {
		std::vector<std::string> current; bool first=true;
		// Hilfsstrukturen für Range Zusammenführung: column -> (lower, includeLower, upper, includeUpper)
		struct RangeAcc { std::optional<std::string> lower; bool includeLower=true; std::optional<std::string> upper; bool includeUpper=true; };
		std::unordered_map<std::string, RangeAcc> rangeMap;
		// Gleichheits-Map für Composite Index Auswertung
		std::unordered_map<std::string, std::string> equalityMap;
		// Sammle einfache Gleichheiten + Ranges
		for (auto &ef : q.extra_filters) {
			if (!ef) {
			  continue;
			}
			auto *bin = dynamic_cast<query::BinaryOpExpr*>(ef.get());
			if (!bin) {
			  continue;
			}
			auto *fa = dynamic_cast<query::FieldAccessExpr*>(bin->left.get());
			auto *lit = dynamic_cast<query::LiteralExpr*>(bin->right.get());
			if (!fa || !lit) {
			  continue;
			}
			auto *var = dynamic_cast<query::VariableExpr*>(fa->object.get()); if (!var) continue;
			// Literal -> String
			std::string value;
			if (std::holds_alternative<std::string>(lit->value)) {
			  value = std::get<std::string>(lit->value);
			}
			else if (std::holds_alternative<int64_t>(lit->value)) value = std::to_string(std::get<int64_t>(lit->value));
			else if (std::holds_alternative<double>(lit->value)) { std::ostringstream oss; oss<<std::get<double>(lit->value); value=oss.str(); }
			else if (std::holds_alternative<bool>(lit->value)) value = std::get<bool>(lit->value)?"true":"false"; else continue;
			// Equality
			if (bin->op == query::BinaryOperator::Eq && secIdx_->hasIndex(q.table, fa->field)) {
				auto [st, keys] = secIdx_->scanKeysEqual(q.table, fa->field, value); if (!st.ok) continue; tbb::parallel_sort(keys.begin(), keys.end());
				if (first) { current = std::move(keys); first=false; }
				else {
					std::vector<std::string> intersected; intersected.reserve(std::min(current.size(), keys.size()));
					auto it1=current.begin(); auto it2=keys.begin();
					while(it1!=current.end() && it2!=keys.end()) { if(*it1<*it2) ++it1; else if(*it2<*it1) ++it2; else { intersected.push_back(*it1); ++it1; ++it2; } }
					current.swap(intersected);
				}
				// In Equality-Map eintragen (für mögliche Composite Nutzung)
				equalityMap[fa->field] = value;
				continue;
			}
			// Range via RangeIndex
			if ((bin->op == query::BinaryOperator::Gt || bin->op == query::BinaryOperator::Gte || bin->op == query::BinaryOperator::Lt || bin->op == query::BinaryOperator::Lte) && secIdx_->hasRangeIndex(q.table, fa->field)) {
				auto &acc = rangeMap[fa->field];
				if (bin->op == query::BinaryOperator::Gt) { acc.lower = value; acc.includeLower=false; }
				else if (bin->op == query::BinaryOperator::Gte) { acc.lower = value; acc.includeLower=true; }
				else if (bin->op == query::BinaryOperator::Lt) { acc.upper = value; acc.includeUpper=false; }
				else if (bin->op == query::BinaryOperator::Lte) { acc.upper = value; acc.includeUpper=true; }
				continue;
			}
		}
		// Composite Index Nutzung: prüfe vorhandene Equality Map gegen definierte Composite Indizes
		if (!equalityMap.empty()) {
			try {
				auto allStats = secIdx_->getAllIndexStats(q.table);
				for (const auto &st : allStats) {
					// Heuristik: Spaltenliste enthält '+' => Composite
					if (st.column.find('+') == std::string::npos) {
					  continue;
					}
					// Zerlege Spalten
					std::vector<std::string> cols; cols.reserve(4);
					{
						std::string tmp = st.column; size_t pos=0; while(true){ size_t n = tmp.find('+', pos); if(n==std::string::npos){ cols.push_back(tmp.substr(pos)); break; } cols.push_back(tmp.substr(pos, n-pos)); pos = n+1; }
					}
					// Prüfe ob alle Spalten Gleichheit haben
					std::vector<std::string> vals; vals.reserve(cols.size()); bool all=true;
					for (auto &c : cols) { auto it = equalityMap.find(c); if (it==equalityMap.end()) { all=false; break; } vals.push_back(it->second); }
					if (!all) {
					  continue;
					}
					// Prüfe Existenz des Composite Index explizit
					if (!secIdx_->hasCompositeIndex(q.table, cols)) {
					  continue;
					}
					auto [cst, keys] = secIdx_->scanKeysEqualComposite(q.table, cols, vals);
					if (!cst.ok) {
					  continue;
					}
					tbb::parallel_sort(keys.begin(), keys.end());
					if (first) { current = std::move(keys); first=false; }
					else {
						std::vector<std::string> intersected; intersected.reserve(std::min(current.size(), keys.size()));
						auto it1=current.begin(); auto it2=keys.begin();
						while(it1!=current.end() && it2!=keys.end()) { if(*it1<*it2) ++it1; else if(*it2<*it1) ++it2; else { intersected.push_back(*it1); ++it1; ++it2; } }
						current.swap(intersected);
					}
					span.setAttribute("composite_prefilter_applied", true);
				}
			} catch (...) {
				// defensiv: bei Fehler keine Composite-Nutzung
				THEMIS_WARN("VectorGeoQuery: composite prefilter failed, skipping");
			}
		}
		// Wende Range-Prädikate an (intersect) in stabiler Reihenfolge an,
		// damit Tracing/Debugging nicht von unordered_map-Iteration abhängt.
		std::vector<std::string> sortedRangeColumns;
		sortedRangeColumns.reserve(rangeMap.size());
		for (const auto& kv : rangeMap) {
			sortedRangeColumns.push_back(kv.first);
		}
		std::sort(sortedRangeColumns.begin(), sortedRangeColumns.end());
		for (const auto& column : sortedRangeColumns) {
			const auto itRange = rangeMap.find(column);
			if (itRange == rangeMap.end()) {
				continue;
			}
			const auto& range = itRange->second;
			auto [st, keys] = secIdx_->scanKeysRange(q.table, column, range.lower, range.upper, range.includeLower, range.includeUpper, 100000, false);
			if (!st.ok) {
			  continue;
			}
			tbb::parallel_sort(keys.begin(), keys.end());
			if (first) { current = std::move(keys); first=false; }
			else {
				std::vector<std::string> intersected; intersected.reserve(std::min(current.size(), keys.size()));
				auto it1=current.begin(); auto it2=keys.begin();
				while(it1!=current.end() && it2!=keys.end()) { if(*it1<*it2) ++it1; else if(*it2<*it1) ++it2; else { intersected.push_back(*it1); ++it1; ++it2; } }
				current.swap(intersected);
			}
		}
		if (!first) {
			indexPrefilter = std::move(current);
			span.setAttribute("index_prefilter_size", static_cast<int64_t>(indexPrefilter->size()));
			if (indexPrefilter->empty()) { span.setAttribute("result_count", static_cast<int64_t>(0)); span.setStatus(true); return Ok(std::vector<VectorGeoResult>{}); }
		}
	}
	
	// Strategy: Two-phase filtering
	// 1. Execute spatial filter to get candidate PKs (whitelist)
	// 2. Execute vector search with whitelist
	
	std::vector<VectorGeoResult> results;
	
	if (!q.spatial_filter) {
		// Erlaube reine Vektorabfrage mittels Syntax-Sugar (SIMILARITY ohne Spatial)
		// Fallback: direkter ANN/Brute-Force Pfad (ohne Hybrid-Plan-Auswahl)
		size_t k = q.k;
		if (vectorIdx_) {
			// Falls Index-Prefilter vorhanden, als Whitelist verwenden
			auto [st, vr] = vectorIdx_->searchKnn(q.query_vector, k, indexPrefilter ? &*indexPrefilter : nullptr);
			if (!st.ok) {
			  return Err<std::vector<VectorGeoResult>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, st.message);
			}
			// Lade Entities
			std::vector<std::string> keys; keys.reserve(vr.size());
			for (auto& r : vr) {
			  keys.emplace_back(q.table + ":" + r.pk);
			}
			auto blobs = db_->multiGet(keys);
			for (size_t i=0;i<vr.size();++i) {
				if (!blobs[i].has_value()) {
				  continue;
				}
				nlohmann::json doc; try { std::string s(blobs[i]->begin(), blobs[i]->end()); doc = nlohmann::json::parse(s);} catch (...) { continue; }
				// Evaluate extra filters conjunctively
				bool ok = true;
				if (!q.extra_filters.empty()) {
					EvaluationContext ctx; ctx.bind("doc", doc);
					for (auto& ef : q.extra_filters) { if (!evaluateCondition(ef, ctx)) { ok = false; break; } }
				}
				if (!ok) {
				  continue;
				}
				VectorGeoResult r; r.pk = vr[i].pk; r.vector_distance = vr[i].distance; r.entity = std::move(doc); results.emplace_back(std::move(r));
			}
			return Ok(std::move(results));
		}
		// Brute-force Scan
		std::vector<std::pair<std::string,float>> tmp;
		std::string prefix = q.table + ":";
		db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
			std::string pk = std::string(key).substr(prefix.length());
			nlohmann::json doc;
			try {
				std::vector<uint8_t> blob(value.begin(), value.end());
				BaseEntity entity = BaseEntity::deserialize(pk, blob);
				doc = nlohmann::json::parse(entity.toJson());
			} catch (...) { return true; }
			if (!doc.contains(q.vector_field) || !doc[q.vector_field].is_array()) {
			  return true;
			}
			std::vector<float> vec = doc[q.vector_field].get<std::vector<float>>();
			if (vec.size() != q.query_vector.size()) {
			  return true;
			}
			EvaluationContext ctx; ctx.bind("doc", doc);
			bool ok = true;
			for (auto& ef : q.extra_filters) {
				if (!evaluateCondition(ef, ctx)) { ok = false; break; }
			}
			if (!ok) {
			  return true;
			}
			float d = simd::l2_distance(vec.data(), q.query_vector.data(), vec.size());
			tmp.emplace_back(pk, d);
			return true;
		});
		tbb::parallel_sort(tmp.begin(), tmp.end(), [](auto& a, auto& b) {
			if (a.second == b.second) {
				return a.first < b.first;
			}
			return a.second < b.second;
		});
		for (size_t i=0;i<std::min(tmp.size(),k);++i) {
			VectorGeoResult r;
			r.pk = tmp[i].first;
			r.vector_distance = tmp[i].second;
			auto val_opt = db_->get(q.table + ":" + tmp[i].first);
			if (val_opt) {
				try {
					r.entity = nlohmann::json::parse(std::string(val_opt->begin(), val_opt->end()));
				} catch (...) {}
			}
			results.emplace_back(std::move(r));
		}
		return Ok(std::move(results));
	}
	
	// Optional: choose plan when vector index is available
	auto cfg = loadHybridConfig_(*db_);  // Dereference pointer to reference
	VGPlan plan = VGPlan::SpatialThenVector;
	if (vectorIdx_) {
		// Use cost model via QueryOptimizer
		QueryOptimizer::VectorGeoCostInput ci; 
		ci.hasVectorIndex = true; 
		ci.hasSpatialIndex = (spatialIdx_ && spatialIdx_->hasSpatialIndex(q.table));
		if (spatialIdx_ && q.spatial_filter) {
			auto bbox = extractBBoxFromFilter(q.spatial_filter); 
			if (bbox) {
					try {
						auto stats = spatialIdx_->getStats(q.table); 
						double totalArea = std::max((stats.total_bounds.maxx - stats.total_bounds.minx) * (stats.total_bounds.maxy - stats.total_bounds.miny), 1e-9);
						double bboxArea = std::max((bbox->maxx - bbox->minx) * (bbox->maxy - bbox->miny), 0.0); 
						ci.bboxRatio = std::min(std::max(bboxArea / totalArea, 0.0), 1.0); 
						ci.spatialIndexEntries = stats.entry_count;
					} catch (...) {
         THEMIS_WARN("query_engine: unhandled exception caught");
					}
			}
		}
		ci.prefilterSize = indexPrefilter ? indexPrefilter->size() : 0; ci.k = q.k; ci.vectorDim = q.query_vector.size(); ci.overfetch = cfg.overfetch;
		auto cr = QueryOptimizer::chooseVectorGeoPlan(ci);
		plan = (cr.plan == QueryOptimizer::VectorGeoPlan::SpatialThenVector) ? VGPlan::SpatialThenVector : VGPlan::VectorThenSpatial;
		span.setAttribute("optimizer.plan", plan == VGPlan::SpatialThenVector ? "spatial_then_vector" : "vector_then_spatial");
		span.setAttribute("optimizer.cost_spatial_first", cr.costSpatialFirst);
		span.setAttribute("optimizer.cost_vector_first", cr.costVectorFirst);
	}

	// Vector-first plan: run vector search over full index with overfetch, then spatial filter
	if (plan == VGPlan::VectorThenSpatial && vectorIdx_) {
		auto child0 = Tracer::startSpan("phase0.vector_first");
		size_t overfetch = std::max<std::size_t>(q.k * cfg.overfetch, q.k);
		child0.setAttribute("overfetch", static_cast<int64_t>(overfetch));
		// Wenn Index-Prefilter vorhanden: Suchraum einschränken
		auto [st, vr] = vectorIdx_->searchKnn(q.query_vector, overfetch, indexPrefilter ? &*indexPrefilter : nullptr);
		if (!st.ok) {
			child0.setStatus(false, st.message);
		} else {
			// Load entities in batch
			std::vector<std::string> keys;
			keys.reserve(vr.size());
			for (const auto& r : vr) {
			  keys.emplace_back(q.table + ":" + r.pk);
			}
			auto blobs = db_->multiGet(keys);

			// Evaluate spatial filter in parallel
			std::vector<VectorGeoResult> local;
			local.reserve(vr.size());
			const size_t n = vr.size();
			const size_t T = std::max<unsigned>(1u, std::thread::hardware_concurrency());
			const size_t CHUNK = std::max<std::size_t>(cfg.min_chunk_spatial_eval, (n + T - 1) / T);
			std::vector<std::vector<VectorGeoResult>> buckets((n + CHUNK - 1) / CHUNK);
			tbb::task_group tg;
			for (size_t bi = 0; bi < buckets.size(); ++bi) {
				tg.run([&, bi]() {
					size_t start = bi * CHUNK;
					size_t end = std::min(start + CHUNK, n);
					std::vector<VectorGeoResult> buf;
					buf.reserve(end - start);
					for (size_t i = start; i < end; ++i) {
						if (!blobs[i].has_value()) {
						  continue;
						}
						nlohmann::json doc;
						try {
							// Deserialize MessagePack blob to BaseEntity, then convert to JSON
							BaseEntity entity = BaseEntity::deserialize(vr[i].pk, *blobs[i]);
							doc = nlohmann::json::parse(entity.toJson());
						}
						catch (...) { continue; }
						EvaluationContext ctx; ctx.bind("doc", doc);
						if (evaluateCondition(q.spatial_filter, ctx)) {
							VectorGeoResult r; r.pk = vr[i].pk; r.vector_distance = vr[i].distance; r.entity = std::move(doc);
							buf.emplace_back(std::move(r));
						}
					}
					buckets[bi] = std::move(buf);
				});
			}
				// Timeout enforcement via helper (Q1/REL-50): vector-geo spatial filter
				{
					const auto audit_config = snapshotAuditConfig();
					tbbWaitWithTimeout(tg, audit_config.audit_logger, audit_config.query_timeout_ms, "vector_geo_spatial_filter");
				}
			for (auto& b : buckets) {
				results.insert(results.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
			}
			// Sort by vector distance and keep top-k
			tbb::parallel_sort(results.begin(), results.end(), [](const auto& a, const auto& b){
				if (a.vector_distance == b.vector_distance) {
					return a.pk < b.pk;
				}
				return a.vector_distance < b.vector_distance;
			});
			if (results.size() > q.k) {
			  results.resize(q.k);
			}
			child0.setAttribute("vector_first_after_spatial", static_cast<int64_t>(results.size()));
			child0.setStatus(true);
			span.setAttribute("result_count", static_cast<int64_t>(results.size()));
			span.setStatus(true);
			return Ok(std::move(results));
		}
		// If vector-first failed, fall through to spatial-first plan
	}

	// Phase 1: Spatial pre-filtering (default plan)
	// Get all entities from table and filter by spatial constraint
	auto child1 = Tracer::startSpan("phase1.spatial_filter");
	
	std::vector<std::string> spatialCandidates;
	std::unordered_map<std::string, nlohmann::json> entityCache;
	bool usedSpatialIndex = false;
	
	// Optimized: Use SpatialIndexManager if available
	if (spatialIdx_) {
		auto bbox = extractBBoxFromFilter(q.spatial_filter);
		if (bbox.has_value()) {
			usedSpatialIndex = true;
			child1.setAttribute("method", "spatial_index");
			child1.setAttribute("bbox_minx", bbox->minx);
			child1.setAttribute("bbox_miny", bbox->miny);
			child1.setAttribute("bbox_maxx", bbox->maxx);
			child1.setAttribute("bbox_maxy", bbox->maxy);
			THEMIS_INFO("VectorGeo: spatial_index searchWithin table='{}' bbox=[{},{}]-[{},{}]", q.table, bbox->minx, bbox->miny, bbox->maxx, bbox->maxy);
			
			// Use R-Tree range query
			auto indexResults = spatialIdx_->searchWithin(q.table, *bbox);
			THEMIS_INFO("VectorGeo: spatial_index returned {} candidates", indexResults.size());
			// Batch-load entities for candidates
			std::vector<std::string> keys; keys.reserve(indexResults.size());
			for (const auto& r : indexResults) {
			  keys.emplace_back(q.table + ":" + r.primary_key);
			}
			auto blobs = db_->multiGet(keys);
			for (size_t i = 0; i < indexResults.size(); ++i) {
				const auto& r = indexResults[i];
				if (!blobs[i].has_value()) {
				  continue;
				}
				try {
					// Deserialize MessagePack blob to BaseEntity, then convert to JSON
					BaseEntity entity = BaseEntity::deserialize(r.primary_key, *blobs[i]);
					nlohmann::json doc = nlohmann::json::parse(entity.toJson());
					spatialCandidates.push_back(r.primary_key);
					entityCache.try_emplace(r.primary_key, std::move(doc));
				} catch (...) { /* skip */ }
			}
		} else {
			THEMIS_WARN("SpatialIndexManager available but could not extract BBox from filter, falling back to full scan");
		}
	}
	
	// Fallback: Full scan with spatial filter (+ extra predicates)
	if (!usedSpatialIndex) {
		child1.setAttribute("method", "full_scan");
		std::string prefix = q.table + ":";
		
		db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
			std::string pk = std::string(key).substr(prefix.length());
			
			try {
				std::vector<uint8_t> blob(value.begin(), value.end());
				BaseEntity ent = BaseEntity::deserialize(pk, blob);
				nlohmann::json entity = nlohmann::json::parse(ent.toJson());
				
				// Evaluate spatial filter
				EvaluationContext ctx;
				ctx.bind("doc", entity);
				
				bool spatialOK = evaluateCondition(q.spatial_filter, ctx);
				bool extraOK = true;
				if (spatialOK && !q.extra_filters.empty()) {
					for (auto& ef : q.extra_filters) {
						if (!evaluateCondition(ef, ctx)) { extraOK=false; break; }
					}
				}
				if (spatialOK && extraOK) {
					spatialCandidates.push_back(pk);
					entityCache.try_emplace(pk, std::move(entity));
				}
			} catch (...) {
       THEMIS_WARN("query_engine: unhandled exception caught");
				// Skip invalid JSON
			}
			return true;
		});
	}
	
	child1.setAttribute("spatial_candidates", static_cast<int64_t>(spatialCandidates.size()));
	child1.setStatus(true);

	// Fallback: if spatial index produced zero candidates, try full-scan evaluation
	if (usedSpatialIndex && spatialCandidates.empty()) {
		THEMIS_WARN("VectorGeo: spatial index returned 0 candidates, falling back to full-scan spatial evaluation");
		std::string prefix = q.table + ":";
		std::unordered_map<std::string, nlohmann::json> tmpCache;
		db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
			std::string pk = std::string(key).substr(prefix.length());
			try {
				std::vector<uint8_t> blob(value.begin(), value.end());
				BaseEntity ent = BaseEntity::deserialize(pk, blob);
				nlohmann::json entity = nlohmann::json::parse(ent.toJson());
				EvaluationContext ctx; ctx.bind("doc", entity);
				bool spatialOK = evaluateCondition(q.spatial_filter, ctx);
				THEMIS_DEBUG("VectorGeo fallback: pk='{}' spatialOK={} ", pk, spatialOK);
				bool extraOK = true;
				if (spatialOK && !q.extra_filters.empty()) {
					for (auto& ef : q.extra_filters) { if (!evaluateCondition(ef, ctx)) { extraOK=false; break; } }
				}
				if (spatialOK && extraOK) { spatialCandidates.push_back(pk); tmpCache.try_emplace(pk, std::move(entity)); }
			} catch (...) { /* skip */ }
			return true;
		});
		// merge cache
		for (auto &kv : tmpCache) { entityCache.try_emplace(kv.first, std::move(kv.second)); }
		child1.setAttribute("spatial_candidates_fallback", static_cast<int64_t>(spatialCandidates.size()));
	}

	if (spatialCandidates.empty()) {
		span.setAttribute("result_count", static_cast<int64_t>(0));
		span.setStatus(true);
		return Ok(std::vector<VectorGeoResult>{});
	}
	
	// Phase 2: Vector search with whitelist
	// Phase 2: Vector search with whitelist
	auto child2 = Tracer::startSpan("phase2.vector_search");
	
	// Optimized: Use VectorIndexManager if available
	if (vectorIdx_) {
		child2.setAttribute("method", "hnsw_with_whitelist");
		
		auto [st, indexResults] = vectorIdx_->searchKnn(q.query_vector, q.k, &spatialCandidates);
		if (st.ok) {
			// Convert VectorIndexManager::Result to VectorGeoResult
			for (const auto& r : indexResults) {
				VectorGeoResult vgr;
				vgr.pk = r.pk;
				vgr.vector_distance = r.distance;
				const auto cached = entityCache.find(r.pk);
				if (cached == entityCache.end()) {
					THEMIS_WARN("VectorGeo: missing cached entity for pk {}", r.pk);
					continue;
				}
				vgr.entity = cached->second;
				results.emplace_back(std::move(vgr));
			}
			
			child2.setAttribute("vector_results", static_cast<int64_t>(results.size()));
			child2.setStatus(true);
			span.setAttribute("result_count", static_cast<int64_t>(results.size()));
			span.setStatus(true);
			return Ok(std::move(results));
		} else {
			THEMIS_WARN("VectorIndexManager::searchKnn failed: {}, falling back to brute-force", st.message);
		}
	}
	
	// Fallback: brute-force over spatial candidates if no VectorIndexManager
	child2.setAttribute("method", "brute_force");
	std::vector<std::pair<std::string, float>> vectorResults;
	vectorResults.reserve(spatialCandidates.size());
	const size_t n = spatialCandidates.size();
	const size_t T = std::max<unsigned>(1u, std::thread::hardware_concurrency());
	const size_t CHUNK = std::max<std::size_t>(cfg.min_chunk_vector_bf, (n + T - 1) / T);
	std::vector<std::vector<std::pair<std::string, float>>> buckets((n + CHUNK - 1) / CHUNK);
	tbb::task_group tg2;
	for (size_t bi = 0; bi < buckets.size(); ++bi) {
		tg2.run([&, bi]() {
			size_t start = bi * CHUNK;
			size_t end = std::min(start + CHUNK, n);
			std::vector<std::pair<std::string, float>> buf;
			buf.reserve(end - start);
			for (size_t i = start; i < end; ++i) {
				const auto& pk = spatialCandidates[i];
				const auto it = entityCache.find(pk);
				if (it == entityCache.end()) {
				  continue;
				}
				const auto& entity = it->second;
				if (!entity.contains(q.vector_field) || !entity[q.vector_field].is_array()) {
				  continue;
				}
				// Evaluate extra filters again (not cached in brute force vector phase for spatial-first plan)
				if (!q.extra_filters.empty()) {
					EvaluationContext ctx; ctx.bind("doc", entity);
					bool ok = true; for (auto& ef : q.extra_filters) { if (!evaluateCondition(ef, ctx)) { ok=false; break; } }
					if (!ok) {
					  continue;
					}
				}
				std::vector<float> vec = entity[q.vector_field].get<std::vector<float>>();
				if (vec.size() != q.query_vector.size()) {
				  continue;
				}
				float d = simd::l2_distance(vec.data(), q.query_vector.data(), vec.size());
				buf.emplace_back(pk, d);
			}
			buckets[bi] = std::move(buf);
		});
	}
		// Timeout enforcement via helper (Q1/REL-50): brute-force vector search
		{
			const auto audit_config = snapshotAuditConfig();
			tbbWaitWithTimeout(tg2, audit_config.audit_logger, audit_config.query_timeout_ms, "bruteforce_vector_search");
		}
	for (auto& b : buckets) {
		vectorResults.insert(vectorResults.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
	}
	
	// Sort by distance and take top-k
	tbb::parallel_sort(vectorResults.begin(), vectorResults.end(),
	          [](const auto& a, const auto& b) {
			  if (a.second == b.second) {
				  return a.first < b.first;
			  }
			  return a.second < b.second;
		  });
	
	size_t resultCount = std::min(vectorResults.size(), q.k);
	for (size_t i = 0; i < resultCount; ++i) {
		VectorGeoResult r;
		r.pk = vectorResults[i].first;
		r.vector_distance = vectorResults[i].second;
		const auto cached = entityCache.find(r.pk);
		if (cached == entityCache.end()) {
			THEMIS_WARN("VectorGeo: missing cached entity for pk {} in brute-force phase", r.pk);
			continue;
		}
		r.entity = cached->second;
		results.emplace_back(std::move(r));
	}
	
	child2.setAttribute("vector_results", static_cast<int64_t>(results.size()));
	child2.setStatus(true);
	
	span.setAttribute("result_count", static_cast<int64_t>(results.size()));
	span.setStatus(true);
	return Ok(std::move(results));
}

// Content + Geo: Fulltext + Spatial hybrid search
Result<std::vector<QueryEngine::ContentGeoResult>>
QueryEngine::executeContentGeoQuery(const ContentGeoQuery& q) const {
	auto span = Tracer::startSpan("QueryEngine.executeContentGeoQuery");
	span.setAttribute("query.table", q.table);
	span.setAttribute("query.fulltext", q.fulltext_query);

	// QE-2: enforce collection-level access control before any I/O
	if (collection_access_checker_ && !collection_access_checker_(q.table, collection_access_caller_id_)) {
		return Err<std::vector<ContentGeoResult>>(
			errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
			"Access denied for collection: " + q.table
		);
	}

	std::vector<ContentGeoResult> results;
	if (!q.spatial_filter) {
		return Err<std::vector<ContentGeoResult>>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT, "Content+Geo query requires spatial_filter");
	}

	// Kostenmodell: wähle Reihenfolge (Fulltext->Spatial oder Spatial->Fulltext)
	bool hasFT = secIdx_->hasFulltextIndex(q.table, q.text_field);
	bool hasSpatial = (spatialIdx_ && spatialIdx_->hasSpatialIndex(q.table));
	double bboxRatio = 1.0;
	if (hasSpatial) {
		auto bbox = extractBBoxFromFilter(q.spatial_filter); if (bbox) {
			auto stats = spatialIdx_->getStats(q.table); double totalArea = std::max((stats.total_bounds.maxx - stats.total_bounds.minx) * (stats.total_bounds.maxy - stats.total_bounds.miny), 1e-9);
			double bboxArea = std::max((bbox->maxx - bbox->minx) * (bbox->maxy - bbox->miny), 0.0);
			bboxRatio = std::min(std::max(bboxArea / totalArea, 0.0), 1.0);
		}
	}
	QueryOptimizer::ContentGeoCostInput ci; ci.hasFulltextIndex = hasFT; ci.hasSpatialIndex = hasSpatial; ci.bboxRatio = bboxRatio; ci.limit = q.limit; ci.fulltextHits = q.limit; // grobe Schätzung
	auto cr = QueryOptimizer::estimateContentGeo(ci);
	bool fulltextFirst = cr.chooseFulltextFirst || !hasSpatial || !hasFT;
	span.setAttribute("optimizer.cg.plan", fulltextFirst ? "fulltext_then_spatial" : "spatial_then_fulltext");
	span.setAttribute("optimizer.cg.cost_fulltext_first", cr.costFulltextThenSpatial);
	span.setAttribute("optimizer.cg.cost_spatial_first", cr.costSpatialThenFulltext);

	if (fulltextFirst) {
		// Phase 1: Fulltext search
		auto child1 = Tracer::startSpan("phase1.fulltext_search");
		auto [st, ftResults] = secIdx_->scanFulltextWithScores(q.table, q.text_field, q.fulltext_query, q.limit);
		if (!st.ok) { child1.setStatus(false, st.message); span.setStatus(false, st.message); return Err<std::vector<ContentGeoResult>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, st.message); }
		child1.setAttribute("fulltext_results", static_cast<int64_t>(ftResults.size())); child1.setStatus(true);
		if (ftResults.empty()) { span.setAttribute("result_count", static_cast<int64_t>(0)); span.setStatus(true); return Ok(std::vector<ContentGeoResult>{}); }
		results.reserve(ftResults.size());
		// Phase 2: Spatial filtering
		auto child2 = Tracer::startSpan("phase2.spatial_filter");
		std::vector<std::string> keys; keys.reserve(ftResults.size());
		std::vector<std::string> pks; pks.reserve(ftResults.size());
		std::unordered_map<std::string,double> bm25; bm25.reserve(ftResults.size());
		for (const auto &kv : ftResults) { keys.emplace_back(q.table+":"+kv.pk); pks.emplace_back(kv.pk); bm25.try_emplace(kv.pk, kv.score); }
		auto blobs = db_->multiGet(keys);
		const size_t n = pks.size(); const size_t T = std::max<unsigned>(1u, std::thread::hardware_concurrency()); const size_t CHUNK = std::max<std::size_t>(64,(n+T-1)/T);
		std::vector<std::vector<ContentGeoResult>> buckets((n+CHUNK-1)/CHUNK); tbb::task_group tg;
		for(size_t bi=0; bi<buckets.size(); ++bi){ tg.run([&,bi](){ size_t start=bi*CHUNK; size_t end=std::min(start+CHUNK,n); std::vector<ContentGeoResult> buf; buf.reserve(end-start); for(size_t i=start;i<end;++i){ if(!blobs[i].has_value()) continue; nlohmann::json doc; try { auto entity = BaseEntity::deserialize(pks[i], *blobs[i]); doc = nlohmann::json::parse(entity.toJson()); } catch (...) { continue; } EvaluationContext ctx; ctx.bind("doc", doc); if(!evaluateCondition(q.spatial_filter, ctx)) continue; ContentGeoResult r; r.pk=pks[i]; const auto bm25_it = bm25.find(pks[i]); r.bm25_score = (bm25_it != bm25.end()) ? bm25_it->second : 0.0; r.entity=std::move(doc); if(q.boost_by_distance && q.center_point){ const auto& docRef=r.entity; if(docRef.contains(q.geom_field)){ nlohmann::json geom; if(docRef[q.geom_field].is_string()){ try { geom=nlohmann::json::parse(docRef[q.geom_field].get<std::string>()); } catch (...) {} } else if(docRef[q.geom_field].is_object()){ geom=docRef[q.geom_field]; } if(!geom.is_null() && geom.contains("type") && geom["type"]=="Point" && geom.contains("coordinates") && geom["coordinates"].is_array() && geom["coordinates"].size()>=2){ double x=geom["coordinates"][0].get<double>(); double y=geom["coordinates"][1].get<double>(); double cx=(*q.center_point)[0]; double cy=(*q.center_point)[1]; double dx=x-cx; double dy=y-cy; r.geo_distance=std::sqrt(dx*dx+dy*dy); } } } buf.emplace_back(std::move(r)); } buckets[bi]=std::move(buf); }); }
		// [WAVE3B-FIX: blocking_no_timeout — query_engine.cpp inline tg.wait()]
		// Replace inline tg.wait() (advisory post-fact) with tbbWaitWithTimeout()
		// so stalled content-geo morsels are cancelled within the query timeout.
		tbbWaitWithTimeout(tg, audit_logger_,
		                   query_timeout_ms_,
		                   "content_geo_spatial_filter");
		for(auto &b : buckets){ results.insert(results.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end())); }
		child2.setAttribute("spatial_results", static_cast<int64_t>(results.size())); child2.setStatus(true);
	} else {
		// Spatial-first Plan: verwende SpatialIndex zur Kandidatenmenge, dann naive Fulltext-Evaluation
		auto childS = Tracer::startSpan("phase1.spatial_first_candidates");
		std::vector<std::string> spatialCandidates;
		std::unordered_map<std::string,nlohmann::json> cache;
		if (spatialIdx_) {
			auto bbox = extractBBoxFromFilter(q.spatial_filter);
			if (bbox) {
				childS.setAttribute("method","spatial_index");
				auto indexResults = spatialIdx_->searchWithin(q.table, *bbox);
				spatialCandidates.reserve(indexResults.size());
				cache.reserve(indexResults.size());
				std::vector<std::string> keys; keys.reserve(indexResults.size());
				for (auto &r : indexResults) {
				  keys.emplace_back(q.table+":"+r.primary_key);
				}
				auto blobs = db_->multiGet(keys);
				for(size_t i=0;i<indexResults.size();++i){ if(!blobs[i].has_value()) continue; try { auto entity = BaseEntity::deserialize(indexResults[i].primary_key, *blobs[i]); nlohmann::json doc = nlohmann::json::parse(entity.toJson()); spatialCandidates.emplace_back(indexResults[i].primary_key); cache.emplace(indexResults[i].primary_key, std::move(doc));} catch (...) {} }
			} else {
				childS.setAttribute("method","bbox_extract_failed_fallback_scan");
			}
		}
		childS.setAttribute("spatial_candidates", static_cast<int64_t>(spatialCandidates.size())); childS.setStatus(true);
		if (spatialCandidates.empty()) { span.setAttribute("result_count", static_cast<int64_t>(0)); span.setStatus(true); return Ok(std::vector<ContentGeoResult>{}); }
		std::sort(spatialCandidates.begin(), spatialCandidates.end());
		// Fulltext-Evaluation (naiv) über Kandidaten: AND aller Tokens
		auto childFT = Tracer::startSpan("phase2.fulltext_eval");
		auto tokens = SecondaryIndexManager::tokenize(q.fulltext_query);
		std::unordered_set<std::string> tokenSet(tokens.begin(), tokens.end());
		for (auto &pk : spatialCandidates) {
			const auto it = cache.find(pk); if (it==cache.end()) continue;
			const auto &doc = it->second;
			if (!doc.contains(q.text_field)) {
			  continue;
			}
			std::string text;
			try { if (doc[q.text_field].is_string()) text = doc[q.text_field].get<std::string>(); else continue; } catch (...) { continue; }
			auto docTokens = SecondaryIndexManager::tokenize(text); std::unordered_set<std::string> docSet(docTokens.begin(), docTokens.end());
			bool all=true; for(auto &t : tokenSet){ if(docSet.find(t)==docSet.end()){ all=false; break; } }
			if (!all) {
			  continue;
			}
			ContentGeoResult r; r.pk = pk; r.entity = doc; r.bm25_score = static_cast<double>(tokenSet.size());
			if (q.boost_by_distance && q.center_point){ if (doc.contains(q.geom_field)){ nlohmann::json geom; if(doc[q.geom_field].is_string()){ try { geom=nlohmann::json::parse(doc[q.geom_field].get<std::string>()); } catch (...) {} } else if(doc[q.geom_field].is_object()){ geom=doc[q.geom_field]; } if(!geom.is_null() && geom.contains("type") && geom["type"]=="Point" && geom.contains("coordinates") && geom["coordinates"].is_array() && geom["coordinates"].size()>=2){ double x=geom["coordinates"][0].get<double>(); double y=geom["coordinates"][1].get<double>(); double cx=(*q.center_point)[0]; double cy=(*q.center_point)[1]; double dx=x-cx; double dy=y-cy; r.geo_distance=std::sqrt(dx*dx+dy*dy); } } }
			results.emplace_back(std::move(r));
		}
		childFT.setAttribute("fulltext_matches", static_cast<int64_t>(results.size())); childFT.setStatus(true);
	}
	// Ranking
	if (q.boost_by_distance) {
		tbb::parallel_sort(results.begin(), results.end(), [](const auto& a, const auto& b){
			double sa = a.bm25_score - (a.geo_distance.value_or(0.0)*0.1);
			double sb = b.bm25_score - (b.geo_distance.value_or(0.0)*0.1);
			constexpr double kScoreEps = 1e-9;
			if (std::abs(sa - sb) < kScoreEps) {
				return a.pk < b.pk;
			}
			return sa > sb;
		});
	} else {
		tbb::parallel_sort(results.begin(), results.end(), [](const auto& a, const auto& b){
			constexpr double kBm25Eps = 1e-9;
			if (std::abs(a.bm25_score - b.bm25_score) < kBm25Eps) {
				return a.pk < b.pk;
			}
			return a.bm25_score > b.bm25_score;
		});
	}
	if (results.size() > q.limit) {
	  results.resize(q.limit);
	}
	span.setAttribute("result_count", static_cast<int64_t>(results.size())); span.setStatus(true); return Ok(std::move(results));
}

// Phase 4.1: Execute CTEs and store results in context
// GAP-002: Migrated from Status to Result<void> for unified error handling
Result<void> QueryEngine::executeCTEs(
	const std::vector<QueryEngine::CTESpec>& ctes,
    EvaluationContext& context
) const {
    auto span = Tracer::startSpan("QueryEngine.executeCTEs");
    span.setAttribute("cte_count", static_cast<int64_t>(ctes.size()));
    
    // Execute CTEs in order (to support CTEs referencing previous CTEs)
    for (const auto& cte : ctes) {
        auto cteSpan = Tracer::startSpan("QueryEngine.executeCTE");
        cteSpan.setAttribute("cte_name", cte.name);
        cteSpan.setAttribute("should_materialize", cte.should_materialize);
        
        if (!cte.subquery) {
            cteSpan.setStatus(false);
            span.setStatus(false);
            return ErrVoid(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                fmt::format("CTE '{}' has null subquery", cte.name)
            );
        }
        
        // Translate CTE subquery to executable form
        auto translation = AQLTranslator::translate(cte.subquery);
        if (!translation.success) {
            cteSpan.setStatus(false);
            span.setStatus(false);
            return ErrVoid(
                errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                fmt::format("CTE '{}' translation failed: {}", cte.name, translation.error_message)
            );
        }
        
        // Execute CTE based on query type
        std::vector<nlohmann::json> cte_results;
        
        if (translation.join.has_value()) {
            // JOIN query
            auto& join = translation.join.value();
            auto result = executeJoin(
                join.for_nodes,
                join.filters,
                join.let_nodes,
                join.return_node,
                join.sort,
                join.limit
            );
            if (!result) {
                cteSpan.setStatus(false);
                span.setStatus(false);
                return ErrVoid(
                    errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                    fmt::format("CTE '{}' JOIN execution failed: {}", cte.name, result.error().message())
                );
            }
            cte_results = std::move(*result);
            
        } else if (translation.success && !translation.join.has_value() && !translation.disjunctive.has_value() && !translation.traversal.has_value()) {
			// Conjunctive query (default query field)
			auto result = executeAndEntitiesWithFallback(translation.conjunctive_query);
            if (!result) {
                cteSpan.setStatus(false);
                span.setStatus(false);
                return ErrVoid(
                    errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                    fmt::format("CTE '{}' conjunctive execution failed: {}", cte.name, result.error().message())
                );
            }
            auto entities = std::move(*result);
            cte_results.reserve(entities.size());
            for (auto& entity : entities) {
                cte_results.push_back(entity.toJson());
            }
            
        } else if (translation.disjunctive.has_value()) {
            // Disjunctive query
            auto result = executeOrEntitiesWithFallback(translation.disjunctive.value());
            if (!result) {
                cteSpan.setStatus(false);
                span.setStatus(false);
                return ErrVoid(
                    errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                    fmt::format("CTE '{}' disjunctive execution failed: {}", cte.name, result.error().message())
                );
            }
            auto entities = std::move(*result);
            cte_results.reserve(entities.size());
            for (auto& entity : entities) {
                cte_results.push_back(entity.toJson());
            }
            
        } else if (translation.traversal.has_value()) {
            // Graph traversal - not typically used in CTEs but supported
            cteSpan.setStatus(false);
            span.setStatus(false);
            return ErrVoid(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                fmt::format("CTE '{}': Graph traversal queries not yet supported in CTEs", cte.name)
            );
            
		} else if (translation.vector_geo.has_value()) {
            // Vector+Geo hybrid
			auto vgResult = executeVectorGeoQuery(translation.vector_geo.value());
            if (!vgResult) {
                cteSpan.setStatus(false);
                span.setStatus(false);
                return ErrVoid(
                    errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                    fmt::format("CTE '{}' vector+geo execution failed: {}", cte.name, vgResult.error().message())
                );
            }
            auto& results = *vgResult;
            cte_results.reserve(results.size());
            for (auto& result : results) {
                cte_results.push_back(result.entity);
            }
            
		} else if (translation.content_geo.has_value()) {
            // Content+Geo hybrid
			auto cgResult = executeContentGeoQuery(translation.content_geo.value());
            if (!cgResult) {
                cteSpan.setStatus(false);
                span.setStatus(false);
                return ErrVoid(
                    errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                    fmt::format("CTE '{}' content+geo execution failed: {}", cte.name, cgResult.error().message())
                );
            }
            auto& results = *cgResult;
            cte_results.reserve(results.size());
            for (auto& result : results) {
                cte_results.push_back(result.entity);
            }
            
        } else {
            cteSpan.setStatus(false);
            span.setStatus(false);
            return ErrVoid(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                fmt::format("CTE '{}': Unknown query type", cte.name)
            );
        }
        
        // Store CTE results in context
        cteSpan.setAttribute("result_count", static_cast<int64_t>(cte_results.size()));
        context.storeCTE(cte.name, std::move(cte_results));
        cteSpan.setStatus(true);
    }
    
    span.setStatus(true);
    return OkVoid();
}

// ============================================================================
// Query Plan Visualisation
// ============================================================================

query::QueryPlanNode QueryEngine::buildExplainPlan(const ConjunctiveQuery& q) const {
    QueryOptimizer::Plan plan;
    if (secIdx_ != nullptr) {
        QueryOptimizer opt(*secIdx_);
        plan = opt.chooseOrderForAndQuery(q);
    } else {
        // No index manager: emit an un-optimised plan with the original predicate order.
        for (const auto& pred : q.predicates) {
            plan.orderedPredicates.push_back(pred);
            QueryOptimizer::Estimation est;
            est.pred = pred;
            est.estimatedCount = 0;
            plan.details.push_back(est);
        }
    }
    return query::QueryPlanVisualizer::buildPlan(q, plan);
}

} // namespace query
} // namespace themis

