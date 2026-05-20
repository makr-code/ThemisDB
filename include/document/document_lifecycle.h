/*
 * ThemisDB | File: document_lifecycle.h | Version: 0.0.3 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 94
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #4662 feat(document): IDocumentStore, IDocumentManager, lifecycle, schema... (2026-04-15T06:14:28Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

/**
 * @brief Discriminator for the phase of a document lifecycle event.
 */
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

/**
 * @brief Plain-data descriptor of a single document lifecycle event.
 *
 * Passed by const-ref to every IDocumentLifecycleHook callback.
 */
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

/**
 * @brief Observer interface for document lifecycle side-effects.
 *
 * Register implementations with IDocumentManager::registerLifecycleHook().
 *
 * ### Contract
 * - All callbacks are @c noexcept.  If an implementation throws, the process
 *   terminates via @c std::terminate.
 * - Hook registration and unregistration are thread-safe.
 * - Hooks that are in-flight at the time unregisterLifecycleHook() is called
 *   complete before the unregister call returns.
 * - The @c afterDelete callback is guaranteed to fire even when the underlying
 *   storage operation fails.
 */
class IDocumentLifecycleHook {
public:
    virtual ~IDocumentLifecycleHook() = default;

    virtual void beforeCreate(const DocumentLifecycleEvent& evt) noexcept = 0;
    virtual void afterCreate (const DocumentLifecycleEvent& evt) noexcept = 0;
    virtual void beforeUpdate(const DocumentLifecycleEvent& evt) noexcept = 0;
    virtual void afterUpdate (const DocumentLifecycleEvent& evt) noexcept = 0;
    virtual void beforeDelete(const DocumentLifecycleEvent& evt) noexcept = 0;
    virtual void afterDelete (const DocumentLifecycleEvent& evt) noexcept = 0;
};

} // namespace document
} // namespace themis
