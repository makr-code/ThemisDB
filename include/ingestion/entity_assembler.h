/**
 * @file entity_assembler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/base_entity.h"
#include "ingestion/extraction_context.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// EntityNormalizerConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for `EntityNormalizer`.
 */
struct EntityNormalizerConfig {
    /// Deduplication strategy: "canonical_id" (default) | "none"
    std::string dedup_strategy{"canonical_id"};
    /// Minimum confidence to retain an entity (0.0 = keep all)
    double min_confidence{0.0};
    /// Known law abbreviations for canonical ID building, e.g. {"BImSchG", "BImSchV"}
    std::unordered_set<std::string> known_law_abbreviations;
};

// ─────────────────────────────────────────────────────────────────────────────
// EntityNormalizer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Post-processes and deduplicates `BaseEntity` collections.
 *
 * ## Canonical ID scheme (legal domain)
 *
 * | Entity type      | ID pattern                         | Example                     |
 * |------------------|------------------------------------|------------------------------|
 * | LEGAL_PROVISION  | `law:<Abbr>:§<n>[:Abs<m>]`        | `law:BImSchG:§4:Abs1`        |
 * | LEGAL_NORM_REF   | `normref:<Abbr>:§<n>`             | `normref:BGB:§823`           |
 * | LEGAL_DECISION   | `bescheid:<aktenzeichen>`          | `bescheid:12_A_345_24`       |
 * | PERSON           | `person:<hash(text)>`              | `person:abc123`              |
 * | ORGANIZATION     | `org:<hash(text)>`                 | `org:def456`                 |
 * | CHUNK            | `chunk:<file_id>:<seq>`            | `chunk:sha256:abc:0`         |
 * | Default          | `<type>:<file_id>:<hash(text)>`    | `date:sha256:abc:789`        |
 *
 * ## Deduplication
 * When `dedup_strategy == "canonical_id"`, entities with the same canonical
 * ID are merged: the one with higher `confidence` is retained; properties
 * from the lower-confidence duplicate are merged in if the key is absent.
 *
 * ## Usage
 * @code
 * EntityNormalizerConfig cfg;
 * cfg.known_law_abbreviations = {"BImSchG", "BGB", "StGB"};
 * EntityNormalizer normalizer(cfg);
 *
 * normalizer.normalize(ctx);  // updates ctx.entities in-place
 * @endcode
 */
class EntityNormalizer {
public:
    explicit EntityNormalizer(EntityNormalizerConfig cfg = {});

    /**
     * @brief Normalise and deduplicate `ctx.entities` in-place.
     *
     * Steps:
     * 1. Assign canonical IDs to entities that lack them.
     * 2. Filter by `min_confidence`.
     * 3. Deduplicate by canonical ID (if configured).
     *
     * @param ctx  Extraction context whose `entities` vector is modified.
     */
    void normalize(ExtractionContext& ctx) const;

    /**
     * @brief Compute a canonical ID for a single entity.
     *
     * Does NOT modify the entity; the caller must assign the result.
     *
     * @param ent       Entity to compute the ID for.
     * @param file_id   SHA-256 of the originating file.
     * @param seq       Sequence number within the file (for CHUNK types).
     */
    std::string canonicalId(const BaseEntity& ent,
                             const std::string& file_id,
                             std::size_t seq = 0) const;

private:
    EntityNormalizerConfig cfg_;

    /// Extract law abbreviation and section from a legal norm text.
    /// Returns {"abbr": "BImSchG", "section": "4", "abs": "1"} or empty map.
    std::unordered_map<std::string, std::string>
    parseLegalRef(const std::string& text) const;

    /// Normalise a string for use in IDs (replace spaces/special chars with _).
    static std::string toIdToken(const std::string& s);

    /// FNV-1a 32-bit hash of a string (for non-legal entity IDs).
    static std::string shortHash(const std::string& s);
};

// ─────────────────────────────────────────────────────────────────────────────
// RelationBuilderConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for `RelationBuilder`.
 */
struct RelationBuilderConfig {
    /// Relation types to extract; empty = all supported types.
    std::vector<std::string> relation_types;
    /// Minimum entity confidence for participating in relations.
    double min_entity_confidence{0.0};
    /// When true, build CO_OCCURS edges for entity pairs in the same chunk.
    bool build_co_occurrence{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// RelationBuilder
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Constructs `EntityRelation` edges from a finalised entity list.
 *
 * ## Relation extraction strategy
 *
 * ### CITES / AMENDS / SUPERSEDES
 * For each `LEGAL_PROVISION` or `LEGAL_NORM_REFERENCE` entity that has a
 * `properties["norm_ref_target"]` key, a `CITES` edge is emitted from the
 * provision to the referenced norm.
 *
 * `AMENDS` and `SUPERSEDES` are detected from deontic/metadata properties:
 * `properties["relation_hint"] == "amends"` → `AMENDS` edge.
 *
 * ### PART_OF
 * A `LEGAL_PROVISION` entity with a `properties["parent_section"]` key gets
 * a `PART_OF` edge to its parent entity (must be present in the entity list).
 *
 * ### CO_OCCURS
 * When `build_co_occurrence` is true, all entity pairs that appear in the
 * same `TextChunk` (same `section_ref`) are connected with `CO_OCCURS` edges.
 *
 * ### ISSUED_BY
 * A `LEGAL_DECISION` entity that has a `properties["authority"]` key gets an
 * `ISSUED_BY` edge to the corresponding `LEGAL_AUTHORITY` entity.
 *
 * ## Usage
 * @code
 * RelationBuilderConfig cfg;
 * cfg.relation_types = {"CITES", "AMENDS", "PART_OF"};
 * RelationBuilder builder(cfg);
 *
 * builder.build(ctx);  // appends to ctx.relations
 * @endcode
 */
class RelationBuilder {
public:
    explicit RelationBuilder(RelationBuilderConfig cfg = {});

    /**
     * @brief Derive and append relations to `ctx.relations`.
     *
     * Idempotent if called multiple times (duplicate edges are not added).
     *
     * @param ctx  Extraction context to read entities from and append relations to.
     */
    void build(ExtractionContext& ctx) const;

private:
    RelationBuilderConfig cfg_;

    bool wantsType(const std::string& t) const;

    void buildCitesRelations(ExtractionContext& ctx) const;
    void buildPartOfRelations(ExtractionContext& ctx) const;
    void buildCoOccurrence(ExtractionContext& ctx) const;
    void buildIssuedByRelations(ExtractionContext& ctx) const;

    /// Returns true when an identical edge already exists in ctx.relations.
    static bool edgeExists(const std::vector<EntityRelation>& rels,
                            const std::string& from,
                            const std::string& to,
                            RelationType rt);
};

} // namespace ingestion
} // namespace themis

