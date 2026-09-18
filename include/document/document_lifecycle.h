/**
 * @file document_lifecycle.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB — Document Module
 *
 * File:    document_lifecycle.h
 * Module:  include/document/
 * Purpose: IDocumentLifecycleHook interface and DocumentLifecycleEvent
 *          for audit and side-effect dispatch on document CRUD operations.
 *
 * Version: 1.3.0
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "document/document_store.h"
#include <cstdint>
#include <string>

namespace themis {
namespace document {

// ─────────────────────────────────────────────────────────────────────────────
// DocumentEventType
// ─────────────────────────────────────────────────────────────────────────────

enum class DocumentEventType {
    BEFORE_CREATE,  ///< Fires before the document is written to the store
    AFTER_CREATE,   ///< Fires after Result<DocumentId> is returned to the caller
    BEFORE_UPDATE,  ///< Fires before the document body is updated
    AFTER_UPDATE,   ///< Fires after a successful update
    BEFORE_DELETE,  ///< Fires before the document is removed
    AFTER_DELETE,   ///< Fires after removal; guaranteed even on storage failure
};

// ─────────────────────────────────────────────────────────────────────────────
// DocumentLifecycleEvent
// ─────────────────────────────────────────────────────────────────────────────

struct DocumentLifecycleEvent {
    DocumentId        document_id;    ///< Document being acted upon
    CollectionId      collection_id;  ///< Owning collection
    DocumentEventType type;           ///< Lifecycle phase
    int64_t           timestamp_ms;   ///< Unix epoch milliseconds
    std::string       actor;          ///< Identity of the requesting actor (may be empty)
};

// ─────────────────────────────────────────────────────────────────────────────
// IDocumentLifecycleHook
// ─────────────────────────────────────────────────────────────────────────────

class IDocumentLifecycleHook {
public:
    /**
     * @brief IDocument Lifecycle Hook.
     * @return Return value.
     */
    virtual ~IDocumentLifecycleHook() = default;

    /**
     * @brief Before Create.
     * @param[in] evt Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void beforeCreate(const DocumentLifecycleEvent& evt) noexcept = 0;
    /**
     * @brief After Create.
     * @param[in] evt Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void afterCreate (const DocumentLifecycleEvent& evt) noexcept = 0;
    /**
     * @brief Before Update.
     * @param[in] evt Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void beforeUpdate(const DocumentLifecycleEvent& evt) noexcept = 0;
    /**
     * @brief After Update.
     * @param[in] evt Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void afterUpdate (const DocumentLifecycleEvent& evt) noexcept = 0;
    /**
     * @brief Before Delete.
     * @param[in] evt Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void beforeDelete(const DocumentLifecycleEvent& evt) noexcept = 0;
    /**
     * @brief After Delete.
     * @param[in] evt Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void afterDelete (const DocumentLifecycleEvent& evt) noexcept = 0;
};

} // namespace document
} // namespace themis
