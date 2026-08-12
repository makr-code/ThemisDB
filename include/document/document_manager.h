/**
 * @file document_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB — Document Module
 *
 * File:    document_manager.h
 * Module:  include/document/
 * Purpose: IDocumentManager — primary document CRUD interface with
 *          Result<T>-based error propagation, lifecycle-hook support, and
 *          encrypted-entity factory.  Includes InMemoryDocumentManager
 *          as a thread-safe reference implementation.
 *
 * Version: 1.3.0
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "document/document_lifecycle.h"
#include "document/document_store.h"
#include "utils/expected.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

namespace themis {
namespace document {

// ─────────────────────────────────────────────────────────────────────────────
// KeyRotationDescriptor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Plain-data descriptor for an encryption key-rotation operation.
 *
 * Contains only key identifiers — never raw key material.  Passed to
 * IEncryptedDocumentEntity::reencrypt() to trigger key rotation without
 * decrypting to plaintext through the public API.
 */
struct KeyRotationDescriptor {
    std::string old_key_id;      ///< Identifier of the current encryption key
    std::string new_key_id;      ///< Identifier of the replacement key
    int64_t     rotation_timestamp_ms{0}; ///< Unix epoch ms (0 = use current time)
};

// ─────────────────────────────────────────────────────────────────────────────
// IEncryptedDocumentEntity
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Opaque handle to an encrypted document stored in a collection.
 *
 * ### Opacity contract
 * - Key material, cipher parameters, and internal buffer pointers are never
 *   accessible through this interface.
 * - Instances are obtained exclusively via
 *   IDocumentManager::createEncrypted(); there are no public constructors.
 *
 * ### Thread safety
 * - documentId() and collectionId() are safe to call concurrently.
 * - reencrypt() is not re-entrant on the same handle.
 */
class IEncryptedDocumentEntity {
public:
    virtual ~IEncryptedDocumentEntity() = default;

    /// @brief The document's unique identifier within its collection.
    [[nodiscard]] virtual const DocumentId&   documentId()   const noexcept = 0;

    /// @brief The owning collection identifier.
    [[nodiscard]] virtual const CollectionId& collectionId() const noexcept = 0;

    /**
     * @brief Rotate the encryption key without exposing plaintext.
     *
     * @return ERR_DOC_INVALID_ARGUMENT if @p desc.new_key_id is empty.
     * @return ERR_DOC_ENCRYPT_FAILED   on key-provider or crypto failure.
     */
    [[nodiscard]] virtual Result<void> reencrypt(const KeyRotationDescriptor& desc) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IDocumentManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Primary document CRUD interface.
 *
 * All methods return @c Result<T>; no method throws exceptions through this
 * interface boundary.  Implementations must be thread-safe.
 *
 * Documents are stored as @c nlohmann::json payloads.  Collection-level ACL
 * is validated at the interface boundary before any operation proceeds.
 *
 * ### Lifecycle hooks
 * - Hooks are dispatched synchronously in registration order.
 * - @c beforeCreate fires before the storage write; @c afterCreate fires after
 *   the @c Result<DocumentId> is returned to the caller.
 * - @c afterDelete is guaranteed to fire even on storage failure.
 *
 * ### Error codes
 *   - ERR_DOC_INVALID_ID        — empty document id
 *   - ERR_DOC_ALREADY_EXISTS    — create() on a duplicate id
 *   - ERR_DOC_NOT_FOUND         — get/update/remove on a missing document
 *   - ERR_DOC_INVALID_ARGUMENT  — missing required argument
 */
class IDocumentManager {
public:
    virtual ~IDocumentManager() = default;

    // ── CRUD ──────────────────────────────────────────────────────────────

    /**
     * @brief Create a new document in @p collection with the given @p id.
     *
     * @return ERR_DOC_INVALID_ID     if @p id is empty.
     * @return ERR_DOC_ALREADY_EXISTS if the id already exists.
     */
    [[nodiscard]] virtual Result<DocumentId> create(const CollectionId&   collection,
                                      const DocumentId&     id,
                                      const nlohmann::json& body) = 0;

    /**
     * @brief Retrieve a document by collection and id.
     *
     * @return std::nullopt (success) if the document does not exist.
     */
    [[nodiscard]] virtual Result<std::optional<nlohmann::json>> get(
        const CollectionId& collection,
        const DocumentId&   id) const = 0;

    /**
     * @brief Replace the body of an existing document.
     *
     * @return ERR_DOC_NOT_FOUND if the document does not exist.
     */
    [[nodiscard]] virtual Result<void> update(const CollectionId&   collection,
                                const DocumentId&     id,
                                const nlohmann::json& body) = 0;

    /**
     * @brief Remove a document.  No-op (success) if not found.
     *
     * @note  @c afterDelete is fired even if the underlying storage fails.
     */
    [[nodiscard]] virtual Result<void> remove(const CollectionId& collection,
                                const DocumentId&   id) = 0;

    /**
     * @brief List all document IDs in a collection.
     */
    [[nodiscard]] virtual Result<std::vector<DocumentId>> list(
        const CollectionId& collection) const = 0;

    // ── Encrypted entity factory ─────────────────────────────────────────

    /**
     * @brief Create an opaque encrypted document handle.
     *
     * The document is stored with its payload encrypted.  The returned
     * IEncryptedDocumentEntity exposes no key material or cipher parameters.
     *
     * @return ERR_DOC_INVALID_ID    if @p id is empty.
     * @return ERR_DOC_ENCRYPT_FAILED on key-provider failure.
     */
    [[nodiscard]] virtual Result<std::unique_ptr<IEncryptedDocumentEntity>> createEncrypted(
        const CollectionId&   collection,
        const DocumentId&     id,
        const nlohmann::json& body) = 0;

    // ── Lifecycle hooks ───────────────────────────────────────────────────

    /**
     * @brief Register a lifecycle observer.
     *
     * Thread-safe.  The hook is called for every subsequent CRUD operation.
     * Duplicates (same pointer) are silently ignored.
     */
    virtual void registerLifecycleHook(IDocumentLifecycleHook& hook) = 0;

    /**
     * @brief Unregister a previously registered observer.
     *
     * Thread-safe.  Any in-flight callback on @p hook completes before this
     * method returns.  No-op if @p hook was not registered.
     */
    virtual void unregisterLifecycleHook(IDocumentLifecycleHook& hook) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryEncryptedEntity  (package-private implementation detail)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief In-memory concrete implementation of IEncryptedDocumentEntity.
 *
 * No real encryption is performed; this is intended for unit tests and
 * development use only.
 *
 * STUB/SIMULATION NOTE:
 * Purpose:          Provide a testable IEncryptedDocumentEntity without a
 *                   real key provider.
 * Activation:       Always active in InMemoryDocumentManager.
 * Production Delta: reencrypt() records the new key_id but does not
 *                   re-cipher any data bytes; equivalent to a no-op cipher
 *                   on an empty plaintext (valid for in-memory/test use).
 * Removal Plan:     Replace with a KeyProviderEncryptedEntity that wraps a
 *                   real IKeyProvider when key management is wired in.
 */
class InMemoryEncryptedEntity final : public IEncryptedDocumentEntity {
public:
    InMemoryEncryptedEntity(DocumentId doc_id, CollectionId col_id)
        : doc_id_(std::move(doc_id)), col_id_(std::move(col_id)) {}

    const DocumentId&   documentId()   const noexcept override { return doc_id_; }
    const CollectionId& collectionId() const noexcept override { return col_id_; }

    Result<void> reencrypt(const KeyRotationDescriptor& desc) override {
        if (desc.new_key_id.empty()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                "new_key_id must not be empty"));
        }
        // Record the key rotation so callers can verify key-id progression.
        // No actual data re-ciphering is performed; this entity holds no
        // encrypted bytes.  Valid for unit tests and in-memory development.
        current_key_id_ = desc.new_key_id;
        return Result<void>{};
    }

    /**
     * @brief Return the current key ID after the last successful reencrypt().
     *
     * Returns an empty string if reencrypt() has never been called.
     */
    [[nodiscard]] const std::string& currentKeyId() const noexcept {
        return current_key_id_;
    }

private:
    DocumentId   doc_id_;
    CollectionId col_id_;
    std::string  current_key_id_;  ///< Updated on each successful reencrypt().
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryDocumentManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of IDocumentManager.
 *
 * Backed by an @c InMemoryDocumentStore.  Suitable for unit tests and
 * development; not production-grade.
 */
class InMemoryDocumentManager final : public IDocumentManager {
public:
    // ── CRUD ──────────────────────────────────────────────────────────────

    Result<DocumentId> create(const CollectionId&   collection,
                              const DocumentId&     id,
                              const nlohmann::json& body) override
    {
        if (id.empty()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_INVALID_ID,
                "document id must not be empty"));
        }
        DocumentLifecycleEvent evt{id, collection,
                                   DocumentEventType::BEFORE_CREATE,
                                   nowMs(), ""};
        dispatchHooks(evt);

        DocumentRecord rec;
        rec.id            = id;
        rec.collection_id = collection;
        rec.body          = body;

        auto result = store_.put(rec);
        if (!result) {
            return tl::unexpected(result.error());
        }

        evt.type = DocumentEventType::AFTER_CREATE;
        dispatchHooks(evt);
        return id;
    }

    Result<std::optional<nlohmann::json>> get(
        const CollectionId& collection,
        const DocumentId&   id) const override
    {
        auto result = store_.get(collection, id);
        if (!result) {
            return tl::unexpected(result.error());
        }
        const auto& opt = *result;
        if (!opt.has_value()) {
            return std::optional<nlohmann::json>{std::nullopt};
        }
        return std::optional<nlohmann::json>{opt.value().body};
    }

    Result<void> update(const CollectionId&   collection,
                        const DocumentId&     id,
                        const nlohmann::json& body) override
    {
        DocumentLifecycleEvent evt{id, collection,
                                   DocumentEventType::BEFORE_UPDATE,
                                   nowMs(), ""};
        dispatchHooks(evt);

        auto result = store_.update(collection, id, body);
        if (!result) {
            return tl::unexpected(result.error());
        }

        evt.type = DocumentEventType::AFTER_UPDATE;
        dispatchHooks(evt);
        return Result<void>{};
    }

    Result<void> remove(const CollectionId& collection,
                        const DocumentId&   id) override
    {
        DocumentLifecycleEvent evt{id, collection,
                                   DocumentEventType::BEFORE_DELETE,
                                   nowMs(), ""};
        dispatchHooks(evt);

        auto result = store_.remove(collection, id);

        // afterDelete is guaranteed to fire even on storage failure.
        evt.type = DocumentEventType::AFTER_DELETE;
        dispatchHooks(evt);

        return result;
    }

    Result<std::vector<DocumentId>> list(
        const CollectionId& collection) const override
    {
        return store_.list(collection);
    }

    // ── Encrypted entity factory ─────────────────────────────────────────

    Result<std::unique_ptr<IEncryptedDocumentEntity>> createEncrypted(
        const CollectionId&   collection,
        const DocumentId&     id,
        const nlohmann::json& body) override
    {
        if (id.empty()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_INVALID_ID,
                "document id must not be empty"));
        }
        DocumentRecord rec;
        rec.id            = id;
        rec.collection_id = collection;
        rec.body          = body;

        auto result = store_.put(rec);
        if (!result) {
            return tl::unexpected(result.error());
        }
        std::unique_ptr<IEncryptedDocumentEntity> entity =
            std::make_unique<InMemoryEncryptedEntity>(id, collection);
        return entity;
    }

    // ── Lifecycle hooks ───────────────────────────────────────────────────

    void registerLifecycleHook(IDocumentLifecycleHook& hook) override {
        std::unique_lock<std::shared_mutex> lk(hooks_mu_);
        auto ptr = &hook;
        if (std::find(hooks_.begin(), hooks_.end(), ptr) == hooks_.end()) {
            hooks_.push_back(ptr);
        }
    }

    void unregisterLifecycleHook(IDocumentLifecycleHook& hook) override {
        std::unique_lock<std::shared_mutex> lk(hooks_mu_);
        auto ptr = &hook;
        hooks_.erase(std::remove(hooks_.begin(), hooks_.end(), ptr),
                     hooks_.end());
    }

private:
    void dispatchHooks(const DocumentLifecycleEvent& evt) const {
        std::shared_lock<std::shared_mutex> lk(hooks_mu_);
        for (auto* h : hooks_) {
            switch (evt.type) {
                case DocumentEventType::BEFORE_CREATE: h->beforeCreate(evt); break;
                case DocumentEventType::AFTER_CREATE:  h->afterCreate(evt);  break;
                case DocumentEventType::BEFORE_UPDATE: h->beforeUpdate(evt); break;
                case DocumentEventType::AFTER_UPDATE:  h->afterUpdate(evt);  break;
                case DocumentEventType::BEFORE_DELETE: h->beforeDelete(evt); break;
                case DocumentEventType::AFTER_DELETE:  h->afterDelete(evt);  break;
            }
        }
    }

    static int64_t nowMs() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count();
    }

    InMemoryDocumentStore                   store_;
    mutable std::shared_mutex               hooks_mu_;
    std::vector<IDocumentLifecycleHook*>    hooks_;
};

} // namespace document
} // namespace themis
