/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            agentic_rag.cpp                                    ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 07:04:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     377                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 141136c016  2026-02-24  audit(rag): fix dead code, unused include, wrong metadata... ║
    • 7845f6477b  2026-02-24  feat(rag): Agentic RAG with iterative retrieval loops (Ph... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file agentic_rag.cpp
 * @brief Implementation of agentic RAG with iterative retrieval loops (Phase 4).
 */

#include "rag/agentic_rag.h"
#include "utils/logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <sstream>
#include <unordered_set>

namespace themis::rag::agentic {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

/**
 * Build a flat vector of IDs for the RetrievalFn signature.
 */
std::vector<std::string> toIdVector(
    const std::unordered_set<std::string>& ids)
{
    return std::vector<std::string>(ids.begin(), ids.end());
}

/**
 * Merge @p new_docs into @p accumulator, skipping already-seen IDs.
 * Returns the number of documents actually added.
 */
size_t mergeDocuments(
    std::vector<judge::RetrievedDocument>& accumulator,
    const std::vector<judge::RetrievedDocument>& new_docs,
    std::unordered_set<std::string>& seen_ids,
    size_t max_total)
{
    size_t added = 0;
    for (const auto& doc : new_docs) {
        if (accumulator.size() >= max_total) break;
        if (seen_ids.count(doc.id)) continue;
        seen_ids.insert(doc.id);
        accumulator.push_back(doc);
        ++added;
    }
    return added;
}

inline knowledge_gap::RetrievedDocument toGapDoc(
    const judge::RetrievedDocument& jd)
{
    knowledge_gap::RetrievedDocument kd;
    kd.id               = jd.id;
    kd.content          = jd.content;
    kd.similarity_score = jd.similarity_score;
    kd.metadata         = jd.metadata;
    return kd;
}

} // anonymous namespace

// ===========================================================================
// AgenticRAG::Impl
// ===========================================================================

struct AgenticRAG::Impl {
    AgenticRAGConfig config;

    judge::RAGJudge judge;
    knowledge_gap::KnowledgeGapDetector gap_detector;

    std::atomic<bool> cancel_requested{false};

    explicit Impl(const AgenticRAGConfig& cfg)
        : config(cfg)
        , judge(cfg.judge_config)
        , gap_detector(cfg.gap_config)
    {}
};

// ===========================================================================
// AgenticRAG
// ===========================================================================

AgenticRAG::AgenticRAG()
    : impl_(std::make_unique<Impl>(AgenticRAGConfig{}))
{}

AgenticRAG::AgenticRAG(const AgenticRAGConfig& config)
    : impl_(std::make_unique<Impl>(config))
{
    THEMIS_DEBUG("AgenticRAG created: max_iterations={}, quality_threshold={:.2f}",
                 config.max_iterations, config.quality_threshold);
}

AgenticRAG::~AgenticRAG() = default;

void AgenticRAG::cancel() {
    impl_->cancel_requested.store(true, std::memory_order_relaxed);
}

AgenticRAGConfig AgenticRAG::getConfig() const {
    return impl_->config;
}

void AgenticRAG::setConfig(const AgenticRAGConfig& config) {
    impl_->config = config;
    impl_->judge.setConfig(config.judge_config);
    impl_->gap_detector.setConfig(config.gap_config);
}

// ---------------------------------------------------------------------------
// reformulateQuery
// ---------------------------------------------------------------------------

std::string AgenticRAG::reformulateQuery(
    const std::string& original_query,
    const knowledge_gap::DetectionResult& gap) const
{
    const auto& strategy = impl_->config.reformulation_strategy;

    if (strategy == "aspect_focus") {
        if (!gap.missing_aspects.empty()) {
            // Focus the query on the first missing aspect.
            return original_query + " specifically about " + gap.missing_aspects.front();
        }
        return original_query;
    }

    if (strategy == "rephrase") {
        // Prepend a rephrase marker so an upstream LLM can rewrite it.
        return "[rephrase] " + original_query;
    }

    // Default: "expand" – append all missing aspects.
    if (gap.missing_aspects.empty()) {
        return original_query;
    }

    std::ostringstream oss;
    oss << original_query;
    bool first = true;
    for (const auto& aspect : gap.missing_aspects) {
        oss << (first ? " (also: " : ", ") << aspect;
        first = false;
    }
    oss << ")";
    return oss.str();
}

// ---------------------------------------------------------------------------
// run (with retrieval callback)
// ---------------------------------------------------------------------------

AgenticRAGResult AgenticRAG::run(
    const std::string& initial_query,
    std::vector<judge::RetrievedDocument> initial_docs,
    RetrievalFn retrieval_fn)
{
    impl_->cancel_requested.store(false, std::memory_order_relaxed);

    const auto loop_start = std::chrono::steady_clock::now();

    THEMIS_INFO("AgenticRAG::run started: query='{}', initial_docs={}, max_iter={}",
                initial_query, initial_docs.size(), impl_->config.max_iterations);

    AgenticRAGResult result;
    result.stop_reason    = StopReason::MAX_ITERATIONS;
    result.quality_satisfied = false;

    // Accumulated document pool and seen-ID tracker.
    std::vector<judge::RetrievedDocument> accumulated;
    std::unordered_set<std::string> seen_ids;

    mergeDocuments(accumulated, initial_docs, seen_ids,
                   impl_->config.max_total_documents);

    std::string current_query = initial_query;

    for (size_t iter = 0; iter < impl_->config.max_iterations; ++iter) {
        if (impl_->cancel_requested.load(std::memory_order_relaxed)) {
            THEMIS_INFO("AgenticRAG::run cancelled at iteration {}", iter);
            result.stop_reason = StopReason::CANCELLED;
            break;
        }

        const auto iter_start = std::chrono::steady_clock::now();

        THEMIS_DEBUG("AgenticRAG iteration {}: query='{}', docs={}",
                     iter, current_query, accumulated.size());

        // ----------------------------------------------------------------
        // 1. Select documents for this iteration.
        // ----------------------------------------------------------------
        const auto& eval_docs =
            impl_->config.accumulate_documents ? accumulated : initial_docs;

        // ----------------------------------------------------------------
        // 2. Evaluate with RAGJudge.
        //    We pass an empty generated_answer here to get a pre-generation
        //    quality signal; callers may inject a generated answer by
        //    customising judge_config.
        // ----------------------------------------------------------------
        auto eval_result = impl_->judge.evaluate(
            current_query, eval_docs, /*generated_answer=*/"");

        THEMIS_DEBUG("AgenticRAG iter {}: overall_score={:.3f}, faithfulness={:.3f}",
                     iter, eval_result.overall_score, eval_result.faithfulness_score);

        // ----------------------------------------------------------------
        // 3. Detect knowledge gaps.
        // ----------------------------------------------------------------
        std::vector<knowledge_gap::RetrievedDocument> gap_docs;
        gap_docs.reserve(eval_docs.size());
        for (const auto& d : eval_docs) {
            gap_docs.push_back(toGapDoc(d));
        }

        auto gap_result = impl_->gap_detector.detectPreGeneration(
            current_query, gap_docs);

        // ----------------------------------------------------------------
        // 4. Record this iteration.
        // ----------------------------------------------------------------
        const auto iter_end = std::chrono::steady_clock::now();
        IterationRecord record;
        record.iteration   = iter;
        record.query_used  = current_query;
        record.documents   = eval_docs;
        record.evaluation  = eval_result;
        record.gap         = gap_result;
        record.elapsed_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
            iter_end - iter_start);
        result.iterations.push_back(std::move(record));

        // ----------------------------------------------------------------
        // 5. Check quality satisfaction.
        // ----------------------------------------------------------------
        const bool quality_ok =
            eval_result.overall_score >= impl_->config.quality_threshold &&
            eval_result.faithfulness_score >= impl_->config.faithfulness_threshold;

        if (quality_ok) {
            THEMIS_INFO("AgenticRAG quality satisfied at iteration {}: "
                        "overall={:.3f} >= {:.3f}",
                        iter, eval_result.overall_score,
                        impl_->config.quality_threshold);
            result.stop_reason    = StopReason::QUALITY_SATISFIED;
            result.quality_satisfied = true;
            break;
        }

        // ----------------------------------------------------------------
        // 6. If no gap was detected, there is nothing actionable to do.
        // ----------------------------------------------------------------
        if (!gap_result.gap_detected) {
            THEMIS_INFO("AgenticRAG no gap detected at iteration {}; stopping.", iter);
            result.stop_reason = StopReason::NO_GAP_DETECTED;
            break;
        }

        // ----------------------------------------------------------------
        // 7. Reformulate query and retrieve more documents.
        // ----------------------------------------------------------------
        const std::string next_query = reformulateQuery(current_query, gap_result);

        std::vector<judge::RetrievedDocument> new_docs;
        if (retrieval_fn) {
            const auto seen_vec = toIdVector(seen_ids);
            new_docs = retrieval_fn(next_query, seen_vec);
        }

        THEMIS_DEBUG("AgenticRAG iter {}: retrieved {} new docs for query='{}'",
                     iter, new_docs.size(), next_query);

        if (new_docs.empty()) {
            THEMIS_INFO("AgenticRAG no new documents at iteration {}; stopping.", iter);
            result.stop_reason = StopReason::NO_NEW_DOCUMENTS;
            break;
        }

        // Merge new documents and advance query.
        mergeDocuments(accumulated, new_docs, seen_ids,
                       impl_->config.max_total_documents);
        current_query = next_query;
    }

    // ----------------------------------------------------------------
    // Finalise result.
    // ----------------------------------------------------------------
    result.final_documents    = accumulated;
    result.total_iterations   = result.iterations.size();

    if (!result.iterations.empty()) {
        result.final_evaluation = result.iterations.back().evaluation;
    }

    const auto loop_end = std::chrono::steady_clock::now();
    result.total_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        loop_end - loop_start);

    THEMIS_INFO("AgenticRAG::run complete: iterations={}, stop_reason={}, "
                "quality_satisfied={}, elapsed={}ms",
                result.total_iterations,
                static_cast<int>(result.stop_reason),
                result.quality_satisfied,
                result.total_elapsed_ms.count());

    return result;
}

// ---------------------------------------------------------------------------
// run (without retrieval callback)
// ---------------------------------------------------------------------------

AgenticRAGResult AgenticRAG::run(
    const std::string& query,
    std::vector<judge::RetrievedDocument> docs)
{
    return run(query, std::move(docs), RetrievalFn{});
}

// ===========================================================================
// AgenticRAGFactory
// ===========================================================================

std::unique_ptr<AgenticRAG> AgenticRAGFactory::createAggressive() {
    AgenticRAGConfig cfg;
    cfg.max_iterations   = 8;
    cfg.quality_threshold = 0.85;
    cfg.faithfulness_threshold = 0.85;
    cfg.reformulation_strategy = "expand";
    return std::make_unique<AgenticRAG>(cfg);
}

std::unique_ptr<AgenticRAG> AgenticRAGFactory::createBalanced() {
    // Default configuration
    return std::make_unique<AgenticRAG>();
}

std::unique_ptr<AgenticRAG> AgenticRAGFactory::createConservative() {
    AgenticRAGConfig cfg;
    cfg.max_iterations   = 3;
    cfg.quality_threshold = 0.65;
    cfg.faithfulness_threshold = 0.70;
    cfg.reformulation_strategy = "aspect_focus";
    return std::make_unique<AgenticRAG>(cfg);
}

} // namespace themis::rag::agentic
