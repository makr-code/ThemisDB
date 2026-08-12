/**
 * @file base_entity.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cstdint>

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// EntityType — the kind of thing a BaseEntity represents
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Semantic type of a `BaseEntity`.
 *
 * Legal domain types live in the LEGAL_* range.  Generic NLP types (PER, ORG,
 * LOC, DATE) are reused across all ingestion workflows.  Additional types can
 * be added without breaking the closed set; the assembler writes the raw
 * string label into `properties["entity_type_raw"]` for forward compatibility.
 */
enum class EntityType : std::uint16_t {
    // ── Generic / cross-domain ───────────────────────────────────────────────
    UNKNOWN         = 0,
    CHUNK           = 1,   ///< Raw text chunk (no further classification)
    PERSON          = 2,   ///< Named person (NER: PER)
    ORGANIZATION    = 3,   ///< Named organisation (NER: ORG)
    LOCATION        = 4,   ///< Named location / place (NER: LOC / GPE)
    DATE            = 5,   ///< Date or date-range expression
    URL             = 6,   ///< Hyperlink / URI
    TABLE_ROW       = 7,   ///< Row from a parsed spreadsheet / table
    GEO_FEATURE     = 8,   ///< Geo-spatial feature (point, polygon, linestring)
    IMAGE_REGION    = 9,   ///< Region identified inside an image

    // ── Legal domain ─────────────────────────────────────────────────────────
    LEGAL_PROVISION         = 100, ///< §, Article, Abs, Nr — a discrete legal provision
    LEGAL_NORM_REFERENCE    = 101, ///< Reference to another norm (§ X Abs Y BGB)
    LEGAL_OBLIGATION        = 102, ///< Deontic: OBLIGATION / MUSS
    LEGAL_PROHIBITION       = 103, ///< Deontic: PROHIBITION / DARF NICHT
    LEGAL_PERMISSION        = 104, ///< Deontic: PERMISSION / DARF
    LEGAL_AUTHORITY         = 105, ///< Zuständige Behörde (e.g. "Umweltbundesamt")
    LEGAL_AKTENZEICHEN      = 106, ///< File / reference number ("Az. 12 A 345/24")
    LEGAL_DECISION          = 107, ///< Bescheid / Verwaltungsakt
    LEGAL_APPLICANT         = 108, ///< Antragsteller
    LEGAL_EFFECTIVE_DATE    = 109, ///< Inkrafttreten / Außerkrafttreten datum
};

// ─────────────────────────────────────────────────────────────────────────────
// RelationType — directed semantic edge between two BaseEntity nodes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Directed relation between two `BaseEntity` nodes.
 *
 * Used to build the graph-RAG graph.  The direction is always from→to.
 */
enum class RelationType : std::uint16_t {
    UNKNOWN       = 0,
    CITES         = 1,   ///< from cites / references to
    AMENDS        = 2,   ///< from amends to (modification relation)
    SUPERSEDES    = 3,   ///< from supersedes to (replacement relation)
    REGULATES     = 4,   ///< from (authority/norm) regulates to (subject/activity)
    PART_OF       = 5,   ///< from is part of to  (hierarchical containment)
    CO_OCCURS     = 6,   ///< from and to co-occur in the same text window
    GEO_CONTAINS  = 7,   ///< from geo-polygon contains to geo-point
    ISSUED_BY     = 8,   ///< from (decision/norm) issued by to (authority)
    APPLIES_TO    = 9,   ///< from (norm) applies to to (subject)
};

// ─────────────────────────────────────────────────────────────────────────────
// Provenance — traceability per BaseEntity
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Records which workflow step produced a `BaseEntity`.
 */
struct EntityProvenance {
    std::string step_name;         ///< Name of the workflow step (e.g. "deontic_extract")
    std::string plugin_name;       ///< Fully qualified plugin name (e.g. "builtin.deontic_extractor")
    double      confidence{1.0};   ///< Extraction confidence [0.0 – 1.0]
    std::string extracted_at;      ///< ISO-8601 timestamp of extraction
};

// ─────────────────────────────────────────────────────────────────────────────
// BaseEntity
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Canonical, graph-ready representation of an extracted information unit.
 *
 * `BaseEntity` is the fundamental output type of the ingestion pipeline.  Every
 * step that identifies a piece of information (a legal provision, a named
 * entity, a geo-feature, a chunk) produces one or more `BaseEntity` values
 * that are stored in `ExtractionContext::entities`.
 *
 * The `BaseEntityAssembler` step post-processes the list:
 *  1. Deduplication via `id` (canonical_id strategy).
 *  2. Normalisation of `entity_type`.
 *  3. Routing to Graph Store (nodes) and Vector Index (embeddings).
 *
 * Graph-RAG usage
 * ───────────────
 * • `id` is the node identifier in the graph.
 * • `entity_type` maps to a node label.
 * • `properties` provides additional node attributes.
 * • Embeddings in `embeddings` are stored in the companion vector index.
 *
 * Canonical ID scheme (legal domain)
 * ───────────────────────────────────
 * • Law provisions:  "law:<Kurzname>:§<n>[:Abs<m>]"
 *   e.g. "law:BImSchG:§4:Abs1"
 * • Norm references: "normref:<target_law>:§<n>"
 * • Decisions:       "bescheid:<aktenzeichen>"
 * • Generic chunks:  "chunk:<file_id>:<seq>"
 */
struct BaseEntity {
    // ── Identity ──────────────────────────────────────────────────────────────
    std::string id;                ///< Canonical entity ID (see scheme above)
    EntityType  entity_type{EntityType::UNKNOWN};

    // ── Source traceability ───────────────────────────────────────────────────
    std::string source_file_id;    ///< sha256 of the originating file
    std::string source_text_ref;   ///< Optional: character offset range "start:end"

    // ── Content ───────────────────────────────────────────────────────────────
    std::string text;              ///< Representative text for this entity

    // ── Flexible attributes ───────────────────────────────────────────────────
    std::unordered_map<std::string, std::string> properties;
    ///< Additional domain-specific fields, e.g.:
    ///< "section_ref" → "§ 4 Abs. 1", "deontic_category" → "OBLIGATION",
    ///< "norm_id" → "BImSchG", "aktenzeichen" → "12 A 345/24"

    // ── Vector representation ─────────────────────────────────────────────────
    std::vector<float> embeddings; ///< Dense embedding for semantic / vector-RAG search

    // ── Provenance ────────────────────────────────────────────────────────────
    EntityProvenance provenance;

    // ── Helpers ───────────────────────────────────────────────────────────────

    /// Returns `properties[key]` or `default_val` when absent.
    const std::string& propertyOr(const std::string& key,
                                   const std::string& default_val) const {
        auto it = properties.find(key);
        return it != properties.end() ? it->second : default_val;
    }

    /// Returns true when the entity carries a dense embedding.
    bool hasEmbedding() const { return !embeddings.empty(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// EntityRelation — directed edge in the graph
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A directed, typed relation between two `BaseEntity` nodes.
 *
 * Written to the graph store as an edge `from_id ─[relation_type]→ to_id`.
 */
struct EntityRelation {
    std::string  from_id;                  ///< Source node ID
    std::string  to_id;                    ///< Target node ID
    RelationType relation_type{RelationType::UNKNOWN};
    std::unordered_map<std::string, std::string> properties;
    ///< Optional edge attributes, e.g. "weight", "evidence_text", "effective_from"
};

// ─────────────────────────────────────────────────────────────────────────────
// VectorRecord — individual vector index entry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief One entry in the vector index produced by the embedding step.
 *
 * A `VectorRecord` is a lighter-weight companion to `BaseEntity`: it carries
 * just the embedding + enough metadata to reconstruct the context for a
 * retrieval hit.  In practice, `chunk_id` references the `BaseEntity::id` of
 * a `CHUNK` entity.
 */
struct VectorRecord {
    std::string        chunk_id;        ///< References BaseEntity::id of the chunk
    std::string        source_file_id;  ///< SHA-256 of the originating file
    std::string        text_snippet;    ///< The chunk text (for hit inspection)
    std::vector<float> embedding;       ///< Dense embedding vector
    std::unordered_map<std::string, std::string> metadata;
    ///< Additional retrieval-time metadata, e.g. "section_ref", "page", "language"
};

// ─────────────────────────────────────────────────────────────────────────────
// BaseEntitySet — complete output of the assembler step
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Assembled, de-duplicated output ready for sink (graph + vector + doc).
 *
 * Produced by `builtin.base_entity_assembler` from a finalised
 * `ExtractionContext`.  The three sinks (GraphWriter, VectorWriter,
 * DocWriter) operate on this structure.
 */
struct BaseEntitySet {
    std::vector<BaseEntity>   nodes;    ///< Graph nodes
    std::vector<EntityRelation> edges;  ///< Graph edges
    std::vector<VectorRecord> chunks;   ///< Vector index entries
    std::string source_file_id;         ///< SHA-256 of the originating file
    double quality_score{0.0};          ///< Aggregated quality metric [0.0–1.0]
};

} // namespace ingestion
} // namespace themis

