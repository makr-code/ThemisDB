/**
 * @file extraction_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/file_manifest.h"
#include "ingestion/base_entity.h"
#include "ingestion/inference_backend.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// Supporting types produced by individual steps
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single text chunk produced by the chunking step.
 *
 * Steps downstream (embedding, NER, deontic extraction) iterate over chunks
 * rather than operating on the entire raw text.
 */
struct TextChunk {
    std::uint32_t seq{0};           ///< Zero-based sequence within the document
    std::string   text;             ///< The chunk text
    std::uint64_t char_start{0};    ///< Character offset in raw_text (inclusive)
    std::uint64_t char_end{0};      ///< Character offset in raw_text (exclusive)
    std::string   section_ref;      ///< For §-aware chunking: "§ 4 Abs. 1" etc.
    std::string   page_ref;         ///< Page number hint from the source, if available
    std::unordered_map<std::string, std::string> metadata;
    ///< Additional chunk-level metadata supplied by the chunking step
};

/**
 * @brief A geo-spatial feature produced by `builtin.parse_geo`.
 *
 * Simplified enough to carry SHP point/polygon data into the entity assembler
 * without pulling in a full geometry library as a public dependency.
 */
struct GeoFeature {
    std::string id;               ///< Stable identifier (e.g. from SHP attribute or generated)
    std::string geometry_wkt;     ///< WKT representation (POINT / LINESTRING / POLYGON / …)
    std::string geometry_type;    ///< "Point" | "LineString" | "Polygon" | "MultiPolygon"
    std::unordered_map<std::string, std::string> attributes;
    ///< All SHP / GeoJSON feature properties mapped to strings
};

/**
 * @brief A single row from a tabular source (XLSX, CSV).
 */
struct TableRow {
    std::uint64_t row_index{0};  ///< 0-based row index within the sheet
    std::string   sheet_name;    ///< Sheet or table name (empty for single-sheet sources)
    std::vector<std::string> values;
    ///< Column values in column order; types coerced to string
    std::vector<std::string> column_names;
    ///< Optional header row names (empty if no headers were detected)
};

// ─────────────────────────────────────────────────────────────────────────────
// ExtractionContext — the shared pipeline state
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Mutable pipeline state shared across all workflow steps.
 *
 * `ExtractionContext` is constructed once per file by the `WorkflowEngine`
 * and passed (by non-const reference) to each `IIngestionStep::execute()`.
 * Steps read earlier results and append their own extractions.
 *
 * Thread-safety
 * ─────────────
 * The context is NOT thread-safe by design.  Parallel step execution (when
 * `parallel: true` in the YAML profile) must be managed by the
 * `WorkflowEngine` itself, which merges partial contexts after all parallel
 * steps complete.
 *
 * Lifecycle
 * ─────────
 * 1. WorkflowEngine creates an ExtractionContext with the FileManifest.
 * 2. Steps execute sequentially (or in controlled parallel batches),
 *    enriching the context.
 * 3. `builtin.base_entity_assembler` reads the final context and produces
 *    a `BaseEntitySet`.
 * 4. Sinks (GraphWriter, VectorWriter, DocWriter) consume the BaseEntitySet.
 *
 * Extending the context
 * ──────────────────────
 * New step types can add domain-specific data via the `extra` map:
 * @code
 *   ctx.extra["audio_transcription"] = transcript_text;
 * @endcode
 */
struct ExtractionContext {
    // ── File identity ─────────────────────────────────────────────────────────
    FileManifest manifest;          ///< Immutable after construction

    // ── Text extraction (parse_text / OCR step) ───────────────────────────────
    std::string raw_text;           ///< Full extracted text (UTF-8)
    std::string text_language;      ///< Detected language BCP-47 code ("de", "en", …)
    bool        text_from_ocr{false}; ///< True when raw_text came from an OCR engine

    // ── Chunking (chunk_text step) ────────────────────────────────────────────
    std::vector<TextChunk> chunks;

    // ── Entity extraction (NER / LLM / deontic / regex steps) ─────────────────
    std::vector<BaseEntity> entities;

    // ── Relation extraction (reference_extract step) ──────────────────────────
    std::vector<EntityRelation> relations;

    // ── Embeddings (chunk_embed step) ─────────────────────────────────────────
    std::vector<VectorRecord> embeddings;

    // ── TT-cores (chunk_tt_decompose step) ───────────────────────────────────
    /// Tensor-Train cores produced by `builtin.chunk_tt_decompose`.
    /// Populated only when a `TensorIngestionBridge` is injected and the
    /// embedding data passes the κ-gate (shouldDecompose() == true).
    std::vector<TensorCoreRecord> tensor_cores;

    // ── Geo features (parse_geo step) ────────────────────────────────────────
    std::vector<GeoFeature> geo_features;

    // ── Tabular data (parse_table step) ──────────────────────────────────────
    std::vector<TableRow> table_rows;

    // ── Decompressed children (decompress step) ───────────────────────────────
    std::vector<std::string> extracted_file_paths;
    ///< Paths to files unpacked from a ZIP/tar archive; WorkflowEngine
    ///< recursively ingests these after the decompress step completes.

    // ── Diagnostic messages ───────────────────────────────────────────────────
    std::vector<std::string> warnings;  ///< Non-fatal per-step warnings
    std::vector<std::string> errors;    ///< Fatal step errors (step aborted)

    // ── Extension point for custom / future steps ─────────────────────────────
    std::unordered_map<std::string, std::string> extra;
    ///< Arbitrary string→string payload; use namespaced keys, e.g.
    ///< "audio.transcription_model" = "whisper-large-v3"

    // ── Helpers ───────────────────────────────────────────────────────────────

    /// Returns true when raw_text is non-empty.
    bool hasText() const { return !raw_text.empty(); }

    /// Returns true when at least one chunk was produced.
    bool hasChunks() const { return !chunks.empty(); }

    /// Returns true when at least one entity was extracted.
    bool hasEntities() const { return !entities.empty(); }

    /// Returns true when at least one embedding was produced.
    bool hasEmbeddings() const { return !embeddings.empty(); }

    /// Returns true when at least one TT-core record was produced.
    bool hasTensorCores() const { return !tensor_cores.empty(); }

    /// Returns true when at least one geo feature was extracted.
    bool hasGeoFeatures() const { return !geo_features.empty(); }

    /// Returns true when at least one table row was extracted.
    bool hasTableRows() const { return !table_rows.empty(); }

    /// Returns the `extra` value for `key`, or `default_val` when absent.
    const std::string& extraOr(const std::string& key,
                                const std::string& default_val) const {
        auto it = extra.find(key);
        return it != extra.end() ? it->second : default_val;
    }
};

} // namespace ingestion
} // namespace themis
