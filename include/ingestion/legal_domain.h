/*
 * ThemisDB | File: legal_domain.h | Version: 0.0.2 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 391
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

/**
 * @file legal_domain.h
 * @brief Ingestion v2.0 Phase 6 — Legal Domain Specialisation.
 *
 * Provides six specialised subsystems for German legal text processing:
 *
 * | Class                | Responsibility                                    |
 * |----------------------|---------------------------------------------------|
 * | GesetzParser         | Teil → Abschnitt → § recursive hierarchy          |
 * | TemporalExtractor    | effective_from / effective_to extraction          |
 * | BehoerdenMapper      | Norm reference → responsible authority lookup     |
 * | BescheidExtractor    | Aktenzeichen, Antragsteller, Bescheiddatum, Auflagen |
 * | CrossDocumentLinker  | Cross-file § X Gesetz Y → § Z Gesetz W edge      |
 * | LegalEntityExport    | JSON-LD + Turtle/N-Triples RDF export             |
 */

#include "ingestion/base_entity.h"
#include "ingestion/extraction_context.h"
#include "utils/expected.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// GesetzParser — Phase 6.1: Teil → Abschnitt → § recursive hierarchy
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Node type within a Gesetz hierarchy.
enum class GesetzNodeType {
    GESETZ,       ///< Top-level statute
    TEIL,         ///< Part / Teil
    ABSCHNITT,    ///< Section / Abschnitt / Kapitel / Unterabschnitt
    PARAGRAPH,    ///< § paragraph
    ABSATZ,       ///< (N) Absatz within a paragraph
};

/**
 * @brief A single node in the GesetzStruktur parse tree.
 *
 * Nodes form a tree: GESETZ → TEIL* → ABSCHNITT* → PARAGRAPH* → ABSATZ*.
 */
struct GesetzNode {
    GesetzNodeType          type{GesetzNodeType::GESETZ};
    std::string             number;    ///< "§ 3", "Teil 2", "Abs. 1", …
    std::string             heading;   ///< Heading text (may be empty)
    std::string             text;      ///< Plain text content of this node
    std::vector<GesetzNode> children;  ///< Direct child nodes

    /// Depth-first traversal helper.
    void traverse(std::function<void(const GesetzNode&, int depth)> fn,
                  int depth = 0) const;
};

/**
 * @brief Result of a full statute parse.
 */
struct GesetzHierarchy {
    std::string              norm_abbreviation; ///< e.g. "BImSchG"
    std::string              full_title;        ///< Full official title if found
    GesetzNode               root;              ///< GESETZ root node
    std::vector<std::string> warnings;          ///< Non-fatal parse issues
};

/**
 * @brief Parser for German legal text structure.
 *
 * Recognises:
 *  - §-paragraphs (§ N, §§ N-M)
 *  - Teil / Abschnitt / Kapitel / Unterabschnitt headings
 *  - Absatz numbering within paragraphs: (1), (2), …
 *
 * Implements a single-pass regex-based parser; no external dependencies.
 */
class GesetzParser {
public:
    GesetzParser() = default;

    /**
     * @brief Parse a full statute text into a hierarchy.
     *
     * @param text               Raw text of the statute.
     * @param norm_abbreviation  Optional short name (e.g. "BImSchG").
     * @return                   Hierarchy on success.
     */
    Result<GesetzHierarchy> parse(const std::string& text,
                                   const std::string& norm_abbreviation = "") const;

    /**
     * @brief Extract all §-paragraphs from a text block as a flat list.
     *
     * Faster than a full hierarchy parse when only paragraph texts are needed.
     */
    std::vector<GesetzNode> extractParagraphs(const std::string& text) const;

    /**
     * @brief Convert a `GesetzHierarchy` into a flat list of `BaseEntity`s
     *        using canonical IDs (`law:<norm>:§<n>:Abs<m>`).
     */
    std::vector<BaseEntity> toEntities(const GesetzHierarchy& hierarchy) const;

private:
    /**
     * @brief Extract §-paragraphs together with their byte-offset in the source text.
     *
     * Returns pairs of (match_start_offset, GesetzNode) so that `parse()` can
     * assign each paragraph to the correct Teil section based on relative position.
     */
    std::vector<std::pair<std::size_t, GesetzNode>>
        extractParagraphsWithOffsets(const std::string& text) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// TemporalExtractor — Phase 6.2: effective_from / effective_to
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Temporal validity window for a legal document or provision.
struct TemporalValidity {
    std::optional<std::string> effective_from;  ///< ISO-8601 or raw string
    std::optional<std::string> effective_to;    ///< ISO-8601 or raw string; nullopt = open-ended
    std::string                source_hint;     ///< Snippet that triggered extraction
};

/**
 * @brief Extracts temporal validity from German legal text and metadata.
 *
 * Recognises patterns such as:
 *  - "in Kraft getreten am DD.MM.YYYY"
 *  - "tritt am DD.MM.YYYY in Kraft"
 *  - "gilt ab DD.MM.YYYY"
 *  - "außer Kraft getreten am DD.MM.YYYY"
 *  - "aufgehoben mit Wirkung vom DD.MM.YYYY"
 *  - ISO 8601 date literals in metadata JSON
 *
 * German month names and numeric "DD.MM.YYYY" are normalised to ISO 8601.
 */
class TemporalExtractor {
public:
    TemporalExtractor() = default;

    /**
     * @brief Extract temporal validity from free text.
     *
     * @param text          Document or section text.
     * @return              Validity; both fields nullopt if nothing found.
     */
    TemporalValidity extract(const std::string& text) const;

    /**
     * @brief Merge text-derived validity with metadata hints.
     *
     * @param text_validity  Result from `extract(text)`.
     * @param metadata       JSON metadata object (may carry "effective_from",
     *                       "effective_to", "date", "Inkrafttreten" keys).
     * @return               Merged result; metadata takes precedence.
     */
    TemporalValidity merge(const TemporalValidity& text_validity,
                            const nlohmann::json&    metadata) const;

    /**
     * @brief Normalise a German date string to ISO 8601 ("YYYY-MM-DD").
     *
     * Handles DD.MM.YYYY and German month names; returns input unchanged on
     * parse failure.
     */
    static std::string normaliseDate(const std::string& raw);
};

// ─────────────────────────────────────────────────────────────────────────────
// BehoerdenMapper — Phase 6.3: norm reference → responsible authority
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Maps a German norm abbreviation to the responsible authority.
 *
 * Bundles a static lookup table covering the 30 most common Federal laws and
 * supports injection of custom mappings and a fallback lookup function.
 *
 * Thread-safety: all const methods are thread-safe after construction.
 */
class BehoerdenMapper {
public:
    BehoerdenMapper();  ///< Loads built-in mappings.

    /**
     * @brief Return the responsible authority for a norm abbreviation.
     *
     * Lookup order: (1) custom mappings, (2) built-in table,
     * (3) injected fallback function.
     *
     * @param norm_abbreviation  e.g. "BImSchG", "DSGVO", "GG"
     * @return                   Authority name or nullopt when unknown.
     */
    std::optional<std::string> lookupAuthority(
        const std::string& norm_abbreviation) const;

    /**
     * @brief Register or override a norm → authority mapping.
     */
    void addMapping(const std::string& norm, const std::string& authority);

    /**
     * @brief Set a fallback function called when built-in lookup fails.
     *
     * @param fn  Returns authority string or empty string on miss.
     */
    void setFallback(std::function<std::string(const std::string&)> fn);

    /// Number of entries in the combined table.
    std::size_t mappingCount() const;

private:
    std::unordered_map<std::string, std::string>        builtin_;
    std::unordered_map<std::string, std::string>        custom_;
    std::function<std::string(const std::string&)>      fallback_;
};

// ─────────────────────────────────────────────────────────────────────────────
// BescheidExtractor — Phase 6.4: Aktenzeichen, Antragsteller, etc.
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Structured content extracted from an administrative decision (Bescheid).
struct BescheidEntity {
    std::string              aktenzeichen;   ///< e.g. "2024-UE-0042"
    std::string              antragsteller;  ///< Applicant name / company
    std::string              bescheid_datum; ///< ISO 8601 date or raw string
    std::string              behoerde;       ///< Issuing authority
    std::vector<std::string> auflagen;       ///< List of conditions/obligations
    std::vector<std::string> nebenbestimmungen; ///< Ancillary provisions

    bool empty() const {
        return aktenzeichen.empty() && antragsteller.empty()
            && bescheid_datum.empty() && auflagen.empty();
    }
};

/**
 * @brief Extracts structured fields from an administrative decision text.
 *
 * Recognises German patterns such as:
 *  - "Aktenzeichen:" / "Az.:" / "Geschäftszeichen:"
 *  - "Antragsteller:" / "Antragstellerin:"
 *  - "Datum:" / "Bescheid vom"
 *  - "Auflage [N]:" / numbered list under "Auflagen" section header
 *  - "Nebenbestimmung:"
 */
class BescheidExtractor {
public:
    BescheidExtractor() = default;

    /**
     * @brief Extract structured fields from a Bescheid document text.
     *
     * @param text  Full plain text of the administrative decision.
     * @return      Extracted fields; use `BescheidEntity::empty()` to detect
     *              documents that don't look like a Bescheid.
     */
    BescheidEntity extract(const std::string& text) const;

    /**
     * @brief Convert a `BescheidEntity` to a `BaseEntity` for the graph store.
     *
     * The canonical ID is `bescheid:<aktenzeichen>` when aktenzeichen is
     * non-empty, else `bescheid:<hash>`.
     */
    BaseEntity toEntity(const BescheidEntity& bescheid,
                         const std::string&    source_doc) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// CrossDocumentLinker — Phase 6.5: cross-file § references
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Creates `EntityRelation` edges across two `ExtractionContext`s.
 *
 * Scans the entities of both contexts for matching canonical IDs.  A match
 * is created when a reference entity in ctx1 (`type == "norm_reference"`) has
 * an ID that equals an entity ID in ctx2, producing a `CITES` edge.
 *
 * Also matches cross-§ references of the form:
 *  - entity label "§ 3 BImSchG" in ctx1 → entity id "law:bimschg:§3" in ctx2
 *
 * Thread-safety: `linkDocuments()` is thread-safe (stateless).
 */
class CrossDocumentLinker {
public:
    CrossDocumentLinker() = default;

    /**
     * @brief Generate cross-document relation edges.
     *
     * @param ctx1  Source context (contains referencing entities).
     * @param ctx2  Target context (contains referenced entities).
     * @return      Vector of `CITES` / `AMENDS` edges.
     */
    std::vector<EntityRelation> linkDocuments(const ExtractionContext& ctx1,
                                               const ExtractionContext& ctx2) const;

    /**
     * @brief Batch overload: link one source context against many targets.
     *
     * @param source   Context that references others.
     * @param targets  Contexts to search for matching entity IDs.
     * @return         All cross-document edges found.
     */
    std::vector<EntityRelation> linkDocumentBatch(
        const ExtractionContext&              source,
        const std::vector<ExtractionContext>& targets) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// LegalEntityExport — Phase 6.6: JSON-LD + RDF Turtle/N-Triples
// ─────────────────────────────────────────────────────────────────────────────

/// @brief RDF serialisation format selector.
enum class RdfFormat {
    TURTLE,   ///< Turtle (.ttl)
    N_TRIPLES ///< N-Triples (.nt)
};

/**
 * @brief Exports a `BaseEntitySet` in JSON-LD or RDF format.
 *
 * ### JSON-LD
 * Uses a `@context` that maps common entity types to schema.org and
 * juris (https://juris.bundesrecht.de/vocabulary/) terms.
 *
 * ### RDF
 * Produces a minimal but valid Turtle or N-Triples document using the
 * `https://themisdb.io/legal/` base IRI.  Entity properties are mapped to
 * `rdfs:label`, `dc:identifier`, and `themis:property` predicates.
 *
 * All output is deterministic given the same input (stable sorting).
 */
class LegalEntityExport {
public:
    LegalEntityExport() = default;

    /**
     * @brief Export entity set as JSON-LD.
     *
     * @param entity_set  Assembled entities and relations.
     * @param base_iri    Optional base IRI override.
     * @return            JSON-LD document as `nlohmann::json`.
     */
    nlohmann::json exportJsonLd(const BaseEntitySet& entity_set,
                                 const std::string&   base_iri
                                     = "https://themisdb.io/legal/") const;

    /**
     * @brief Export entity set as RDF (Turtle or N-Triples).
     *
     * @param entity_set  Assembled entities and relations.
     * @param format      Output serialisation format.
     * @param base_iri    Optional base IRI override.
     * @return            Serialised RDF string.
     */
    std::string exportRdf(const BaseEntitySet& entity_set,
                           RdfFormat            format  = RdfFormat::TURTLE,
                           const std::string&   base_iri
                               = "https://themisdb.io/legal/") const;

private:
    static std::string escapeIriComponent(const std::string& s);
    static std::string escapeTurtleLiteral(const std::string& s);
    std::string buildTurtle(const BaseEntitySet& es,
                             const std::string&   base) const;
    std::string buildNTriples(const BaseEntitySet& es,
                               const std::string&   base) const;
};

} // namespace ingestion
} // namespace themis
