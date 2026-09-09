/**
 * @file wiki_rag_source.h
 * @brief WikiRagSource — RAG stage handler that retrieves from the wiki index.
 *
 * `WikiRagSource::retrieveFromWiki` implements the `RAGStageHandler` signature
 * and can be wired into `ModularRAGPipelineConfig::retrieve_fn` (or composed
 * with other retrieve handlers) without modifying `modular_rag_pipeline.h`.
 *
 * ## Wiring example
 * @code
 *   JsonWikiIndexReader reader("artifacts/llm-wiki-mvp/index.json");
 *   reader.load();
 *   WikiRagSource wrs(reader);
 *
 *   ModularRAGPipelineConfig cfg;
 *   cfg.retrieve_fn = [&wrs](ModularRAGContext& ctx) {
 *       return wrs.retrieveFromWiki(ctx);
 *   };
 *   ModularRAGPipeline pipeline(std::move(cfg));
 *   auto result = pipeline.run("How does HNSW work?");
 * @endcode
 *
 * ## Error behaviour
 *
 * By default (`WikiRagSourceConfig::fail_open = false`) exceptions from the
 * underlying reader are surfaced as `StageStatus::Error` (fail-closed).
 * Set `fail_open = true` only for explicitly tolerated degraded paths where
 * the pipeline should continue with zero wiki candidates and
 * `StageStatus::Skipped`.
 *
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include "llm/wiki_index_store.h"
#include "rag/modular_rag_pipeline.h"

#include <string>

namespace themis {
namespace llm {

// ============================================================================
// WikiRagSourceConfig
// ============================================================================

/**
 * @brief Configuration for `WikiRagSource`.
 */
struct WikiRagSourceConfig {
    std::string source_namespace = "wiki"; ///< Namespace tag appended to each RAGCandidate
    int         top_k            = 5;      ///< Maximum number of wiki chunks to retrieve
    float       min_score        = 0.0f;   ///< Minimum score threshold for inclusion
    bool        fail_open        = false;  ///< Default fail-closed; true enables degraded skip path
};

// ============================================================================
// WikiRagSource
// ============================================================================

/**
 * @brief RAG stage handler that retrieves `RAGCandidate`s from a wiki index.
 *
 * Delegates retrieval to an `IWikiIndexReader` and converts the resulting
 * `WikiChunk` list into a `StageResult` suitable for the
 * `ModularRAGPipeline`.
 *
 * Provenance tags appended per candidate:
 *  - `"retrieve:wiki-hybrid"` when the reader is a `WikiIndexStore` (BM25+vector)
 *  - `"retrieve:wiki-json"`   when the reader is a `JsonWikiIndexReader`
 *  - `"retrieve:wiki"`        for any other `IWikiIndexReader` implementation
 */
class WikiRagSource {
public:
    /**
     * @brief Construct from an index reader and optional config.
     *
     * @param reader  Fully initialised index reader (must remain alive for
     *                the lifetime of this object).
     * @param config  Operational configuration; defaults are production-safe.
     */
    explicit WikiRagSource(IWikiIndexReader&    reader,
                           WikiRagSourceConfig  config = {});

    /**
     * @brief Retrieve wiki chunks for the query in `ctx` and populate `StageResult`.
     *
     * This method satisfies the `RAGStageHandler` type alias and may be
     * assigned directly to `ModularRAGPipelineConfig::retrieve_fn`.
     *
     * Steps:
     *  1. Calls `reader.query(ctx.query, config_.top_k, config_.min_score)`.
     *  2. Converts each `WikiChunk` to a `RAGCandidate` with
     *     `source_namespace = config_.source_namespace`.
     *  3. Appends provenance tags.
     *  4. Returns `StageResult` with `StageStatus::Success` and populated
     *     `candidates`.
     *
     * On error:
     *  - If `fail_open = false` (default): returns `StageStatus::Error` + diagnostic.
     *  - If `fail_open = true`: returns `StageStatus::Skipped` + diagnostic.
     *
     * @param ctx  Mutable pipeline context; `ctx.query` is the input text.
     * @return     Stage result with wiki candidates (or skipped on error).
     */
    [[nodiscard]] rag::StageResult retrieveFromWiki(rag::ModularRAGContext& ctx);

    /// @return Reference to the underlying reader.
    [[nodiscard]] const IWikiIndexReader& reader() const noexcept { return reader_; }

    /// @return Current source configuration.
    [[nodiscard]] const WikiRagSourceConfig& config() const noexcept { return config_; }

private:
    /**
     * @brief Determine the provenance tag for this reader type.
     * @return Provenance string such as `"retrieve:wiki-hybrid"`.
     */
    [[nodiscard]] std::string provenanceTag() const;

    IWikiIndexReader&  reader_;  ///< Underlying index reader
    WikiRagSourceConfig config_; ///< Operational configuration
};

} // namespace llm
} // namespace themis
