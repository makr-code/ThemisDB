/**
 * @file process_linker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: process_linker.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_linker.h
 * Module:  include/process/
 * Purpose: Attaching documents/metadata to process instances and linking
 *          process instances to each other (parent/child, sub-process,
 *          cross-references).  Part of the Graph-RAG layer for German
 *          administrative proceedings (Verwaltungsvorgänge).
 */

#pragma once

#include "storage/rocksdb_wrapper.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// ProcessLinkType
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Typed relationship between a process instance and a data object or
 *        between two process instances / models.
 */
enum class ProcessLinkType {
    HAS_DOCUMENT,       ///< Instance has an attached document
    HAS_METADATA,       ///< Instance has structured metadata
    REQUIRES_DOCUMENT,  ///< Process node requires a document (model-level)
    IS_INSTANCE_OF,     ///< Instance created from a model definition
    SUB_PROCESS,        ///< Instance is a sub-process of another instance
    CROSS_REFERENCE,    ///< References another administrative case
    TRIGGERS,           ///< Completion of one process triggers another
    EVIDENCE_FOR,       ///< Document is evidence for a process decision
};

/** @brief Human-readable name for @p t. */
std::string_view toString(ProcessLinkType t);

/** @brief Parse a ProcessLinkType from its string representation. */
ProcessLinkType processLinkTypeFromString(std::string_view s);

// ─────────────────────────────────────────────────────────────────────────────
// ProcessAttachment
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Descriptor of a data object attached to a process instance.
 *
 * Stored in RocksDB under the key prefix @c proc:attach:<instance_id>:<object_id>.
 *
 * The @c metadata field carries link-specific properties, e.g.:
 * @code{.json}
 * { "required": true, "doc_type": "Bauzeichnung", "pages": 4 }
 * @endcode
 */
struct ProcessAttachment {
    std::string id;                  ///< "attach:<instance_id>:<object_id>"
    std::string instance_id;         ///< Process instance the object is attached to
    std::string object_id;           ///< ID of the attached object
    std::string object_collection;   ///< Collection name: "documents", "metadata", "cases", …
    ProcessLinkType link_type{ProcessLinkType::HAS_DOCUMENT};
    std::optional<std::string> node_id;  ///< Process node this attachment belongs to
    std::string attached_by;
    int64_t attached_at_ms{0};
    nlohmann::json metadata;         ///< Additional link metadata

    [[nodiscard]] nlohmann::json toDocument() const;
    static ProcessAttachment fromDocument(const nlohmann::json& doc);
};

// ─────────────────────────────────────────────────────────────────────────────
// ProcessLink
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Typed relationship between two process entities (instance↔instance or
 *        model↔model).
 *
 * Stored under @c proc:link:<source_id>:<target_id>:<link_type_str>.
 */
struct ProcessLink {
    std::string link_id;
    std::string source_id;
    std::string target_id;
    ProcessLinkType link_type{ProcessLinkType::CROSS_REFERENCE};
    nlohmann::json properties;
    int64_t created_at_ms{0};

    [[nodiscard]] nlohmann::json toDocument() const;
    static ProcessLink fromDocument(const nlohmann::json& doc);
};

// ─────────────────────────────────────────────────────────────────────────────
// ProcessLinker
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Manages attachment of documents/metadata to process instances and
 *        linking of process instances to each other.
 *
 * All data is persisted in the supplied @c RocksDBWrapper instance.
 *
 * Key-prefix scheme:
 * - Attachments  : @c proc:attach:<instance_id>:<object_id>
 * - Links        : @c proc:link:<source_id>:<target_id>:<link_type_str>
 * - Required docs: @c proc:req_doc:<model_id>:<node_id>:<doc_type>
 *
 * @section runtime_bounds Runtime Boundaries and Guarantees
 *
 * ### Deterministic Behavior
 * - All linking operations are thread-safe and deterministic.
 * - No transactional guarantees across multiple operations; callers must
 *   implement their own consistency protocols if needed.
 * - State transitions follow a linear append model: operations are idempotent
 *   (reapplying the same link does not change the result).
 *
 * ### Bounded Retrieval
 * - All retrieval operations (getAttachments, getLinks, etc.) complete within
 *   the configured timeout window (see kMaxOperationTimeoutMs in process_common.h).
 * - Maximum retrieval depth for linked hierarchies is bounded by
 *   kMaxRetrievalDepth (typically 50).
 * - Accumulated context size during traversal is bounded by
 *   kMaxRetrievalContextBytes (typically 1 MiB).
 *
 * ### Error Handling
 * - All operations return explicit success/failure indicators (std::pair<bool, string>).
 * - No silent failures; all error conditions are logged and signaled.
 * - Link validation occurs before state commitment (fail-safe).
 *
 * @section threading Threading Guarantees
 * ProcessLinker is thread-safe. Multiple threads may call any method concurrently
 * provided the underlying RocksDBWrapper is thread-safe (which it is).
 */
class ProcessLinker {
public:
    explicit ProcessLinker(RocksDBWrapper& db);

    // ── Attach data objects to process instances ──────────────────────────

    /**
     * @brief Attach any object (document, metadata, case file) to a process
     *        instance.
     *
     * @param instance_id       Target process instance ID.
     * @param object_id         ID of the object to attach.
     * @param object_collection Collection the object lives in ("documents", …).
     * @param link_type         Semantic relationship type.
     * @param node_id           Optional: which process node this attachment
     *                          belongs to.
     * @param metadata          Optional: additional link properties.
     * @param attached_by       Actor performing the attachment.
     * @return {true, attachment_id} on success; {false, error_message} on failure.
     */
    std::pair<bool, std::string> attachObject(
        std::string_view instance_id,
        std::string_view object_id,
        std::string_view object_collection,
        ProcessLinkType  link_type,
        std::optional<std::string_view> node_id = std::nullopt,
        nlohmann::json   metadata   = {},
        std::string_view attached_by = ""
    );

    /**
     * @brief Detach an object from a process instance by attachment ID.
     * @return true if the attachment existed and was removed.
     */
    bool detachObject(std::string_view attachment_id);

    /**
     * @brief Get all attachments for a process instance.
     *
     * @param instance_id  The process instance.
     * @param filter_type  Optional: only return attachments of this link type.
     */
    [[nodiscard]] std::vector<ProcessAttachment> getAttachments(
        std::string_view instance_id,
        std::optional<ProcessLinkType> filter_type = std::nullopt
    ) const;

    /**
     * @brief Get all attachments for a specific process node within an instance.
     */
    [[nodiscard]] std::vector<ProcessAttachment> getNodeAttachments(
        std::string_view instance_id,
        std::string_view node_id
    ) const;

    /**
     * @brief Find all instance IDs that have a specific object attached.
     *
     * Performs a full scan over the attachment prefix; use with care on large
     * datasets.
     */
    [[nodiscard]] std::vector<std::string> findInstancesWithObject(
        std::string_view object_id,
        std::string_view object_collection
    ) const;

    // ── Process-to-process linking ─────────────────────────────────────────

    /**
     * @brief Create a typed link between two process instances (or models).
     *
     * @return {true, link_id} on success; {false, error_message} on failure.
     */
    std::pair<bool, std::string> linkProcesses(
        std::string_view source_id,
        std::string_view target_id,
        ProcessLinkType  link_type,
        nlohmann::json   properties = {}
    );

    /**
     * @brief Get all outgoing links from a process instance (or model).
     *
     * @param process_id   Source process instance/model ID.
     * @param filter_type  Optional: only return links of this type.
     */
    [[nodiscard]] std::vector<ProcessLink> getLinks(
        std::string_view process_id,
        std::optional<ProcessLinkType> filter_type = std::nullopt
    ) const;

    // ── Required documents (model-level) ──────────────────────────────────

    /**
     * @brief Register a required document type for a process node in a model.
     *
     * @param model_id   Process model definition ID.
     * @param node_id    Node within the model that requires the document.
     * @param doc_type   Human-readable document type, e.g. "Bauzeichnung".
     * @param mandatory  Whether the document is strictly required.
     * @param schema     Optional JSON Schema for document validation.
     * @return true on success.
     */
    bool registerRequiredDocument(
        std::string_view model_id,
        std::string_view node_id,
        std::string_view doc_type,
        bool             mandatory,
        nlohmann::json   schema = {}
    );

    /**
     * @brief Get all required document descriptors for a node in a model.
     *
     * Each JSON object contains at minimum: @c doc_type, @c mandatory,
     * and optionally @c schema.
     */
    [[nodiscard]] std::vector<nlohmann::json> getRequiredDocuments(
        std::string_view model_id,
        std::string_view node_id
    ) const;

    /**
     * @brief Determine which required documents are missing for an instance at
     *        a given node.
     *
     * Cross-references the required-document registry (model-level) against
     * the actual attachments (instance-level, filtered by @p node_id).
     *
     * @return List of @c doc_type strings for missing mandatory documents.
     */
    [[nodiscard]] std::vector<std::string> getMissingDocuments(
        std::string_view instance_id,
        std::string_view node_id,
        std::string_view model_id
    ) const;

private:
    RocksDBWrapper& db_;

    std::string makeAttachKey_(std::string_view instance_id,
                               std::string_view object_id) const;
    /// Reverse-lookup key for findInstancesWithObject():
    ///   proc:obj_idx:<object_id>:<collection>:<instance_id>
    std::string makeObjIdxKey_(std::string_view object_id,
                               std::string_view collection,
                               std::string_view instance_id) const;
    std::string makeLinkKey_(std::string_view source_id,
                             std::string_view target_id,
                             ProcessLinkType  link_type) const;
    std::string makeReqDocKey_(std::string_view model_id,
                               std::string_view node_id,
                               std::string_view doc_type) const;
};

} // namespace process
} // namespace themis

