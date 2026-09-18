/**
 * @file document_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

struct KeyRotationDescriptor {
    std::string old_key_id;      ///< Identifier of the current encryption key
    std::string new_key_id;      ///< Identifier of the replacement key
    int64_t     rotation_timestamp_ms{0}; ///< Unix epoch ms (0 = use current time)
};

// ─────────────────────────────────────────────────────────────────────────────
// IEncryptedDocumentEntity
// ─────────────────────────────────────────────────────────────────────────────

class IEncryptedDocumentEntity {
public:
    /**
     * @brief IEncrypted Document Entity.
     * @return Return value.
     */
    virtual ~IEncryptedDocumentEntity() = default;

    [[nodiscard]] virtual const DocumentId&   documentId()   const noexcept = 0;

    [[nodiscard]] virtual const CollectionId& collectionId() const noexcept = 0;

    [[nodiscard]] virtual Result<void> reencrypt(const KeyRotationDescriptor& desc) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IDocumentManager
// ─────────────────────────────────────────────────────────────────────────────

class IDocumentManager {
public:
    /**
     * @brief IDocument Manager.
     * @return Return value.
     */
    virtual ~IDocumentManager() = default;

    // ── CRUD ──────────────────────────────────────────────────────────────

    [[nodiscard]] virtual Result<DocumentId> create(const CollectionId&   collection,
                                      const DocumentId&     id,
                                      const nlohmann::json& body) = 0;

    [[nodiscard]] virtual Result<std::optional<nlohmann::json>> get(
        const CollectionId& collection,
        const DocumentId&   id) const = 0;

    [[nodiscard]] virtual Result<void> update(const CollectionId&   collection,
                                const DocumentId&     id,
                                const nlohmann::json& body) = 0;

    [[nodiscard]] virtual Result<void> remove(const CollectionId& collection,
                                const DocumentId&   id) = 0;

    [[nodiscard]] virtual Result<std::vector<DocumentId>> list(
        const CollectionId& collection) const = 0;

    // ── Encrypted entity factory ─────────────────────────────────────────

    [[nodiscard]] virtual Result<std::unique_ptr<IEncryptedDocumentEntity>> createEncrypted(
        const CollectionId&   collection,
        const DocumentId&     id,
        const nlohmann::json& body) = 0;

    /**
     * @brief ── Lifecycle hooks ───────────────────────────────────────────────────
     * @param[in,out] hook Input/output parameter.
     */

    virtual void registerLifecycleHook(IDocumentLifecycleHook& hook) = 0;

    /**
     * @brief Unregister Lifecycle Hook.
     * @param[in,out] hook Input/output parameter.
     */
    virtual void unregisterLifecycleHook(IDocumentLifecycleHook& hook) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryEncryptedEntity  (package-private implementation detail)
// ─────────────────────────────────────────────────────────────────────────────

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
        /**
         * @brief Lk.
         * @param[in] hooks_mu_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::shared_mutex> lk(hooks_mu_);
        auto ptr = &hook;
        if (std::find(hooks_.begin(), hooks_.end(), ptr) == hooks_.end()) {
            hooks_.push_back(ptr);
        }
    }

    void unregisterLifecycleHook(IDocumentLifecycleHook& hook) override {
        /**
         * @brief Lk.
         * @param[in] hooks_mu_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::shared_mutex> lk(hooks_mu_);
        auto ptr = &hook;
        hooks_.erase(std::remove(hooks_.begin(), hooks_.end(), ptr),
                     hooks_.end());
    }

private:
    void dispatchHooks(const DocumentLifecycleEvent& evt) const {
        /**
         * @brief Lk.
         * @param[in] hooks_mu_ Input parameter.
         * @return Return value.
         */
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

    /**
     * @brief Now Ms.
     * @return Return value.
     * @details Calls: system_clock::now(), time_since_epoch(), count().
     */
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
