/**
 * @file document_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB — Document Module
 *
 * File:    document_store.h
 * Module:  include/document/
 * Purpose: IDocumentStore — pluggable backend interface for raw document
 *          persistence, plus a thread-safe in-memory reference implementation.
 *
 * Version: 1.3.0
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/expected.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace document {

// ─────────────────────────────────────────────────────────────────────────────
// Primary type aliases
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Unique document identifier (UUID string recommended).
using DocumentId = std::string;

/// @brief Identifier for a document collection (name or UUID string).
using CollectionId = std::string;

// ─────────────────────────────────────────────────────────────────────────────
// DocumentRecord
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A raw document record as stored at the backend level.
 *
 * This is the value type exchanged between IDocumentStore and its callers.
 * The @c body field carries the mutable JSON payload; @c created_at /
 * @c updated_at are Unix epoch milliseconds maintained by the store.
 */
struct DocumentRecord {
    DocumentId     id;            ///< Document unique identifier (non-empty)
    CollectionId   collection_id; ///< Owning collection
    nlohmann::json body;          ///< Mutable document payload
    int64_t        created_at{0}; ///< Unix epoch ms (set on first insert)
    int64_t        updated_at{0}; ///< Unix epoch ms (updated on every write)
};

// ─────────────────────────────────────────────────────────────────────────────
// IDocumentStore
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Pluggable backend storage interface for document persistence.
 *
 * Implementations must be thread-safe.  All methods return @c Result<T> for
 * structured error propagation; no method throws exceptions through this
 * interface boundary.
 *
 * Error codes returned by methods:
 *   - ERR_DOC_INVALID_ID       — document id is empty
 *   - ERR_DOC_ALREADY_EXISTS   — put() on an existing id
 *   - ERR_DOC_NOT_FOUND        — update() on a missing document
 */
class IDocumentStore {
public:
    virtual ~IDocumentStore() = default;

    /**
     * @brief Insert a new document record.
     *
     * @return DocumentId on success.
     * @return ERR_DOC_INVALID_ID  if @p record.id is empty.
     * @return ERR_DOC_ALREADY_EXISTS if a record with the same id already
     *         exists in the same collection.
     */
    [[nodiscard]] virtual Result<DocumentId> put(const DocumentRecord& record) = 0;

    /**
     * @brief Retrieve a document by collection and id.
     *
     * @return std::nullopt if the document does not exist (not an error).
     */
    [[nodiscard]] virtual Result<std::optional<DocumentRecord>> get(
        const CollectionId& collection, const DocumentId& id) const = 0;

    /**
     * @brief Replace the body of an existing document.
     *
     * @return ERR_DOC_NOT_FOUND if the document does not exist.
     */
    [[nodiscard]] virtual Result<void> update(const CollectionId& collection,
                                const DocumentId&   id,
                                const nlohmann::json& body) = 0;

    /**
     * @brief Remove a document.  No-op and success if not found.
     */
    [[nodiscard]] virtual Result<void> remove(const CollectionId& collection,
                                const DocumentId&   id) = 0;

    /**
     * @brief List all document IDs in a collection.
     *
     * @return Empty vector (not an error) if the collection is empty or
     *         does not exist.
     */
    [[nodiscard]] virtual Result<std::vector<DocumentId>> list(
        const CollectionId& collection) const = 0;

    /**
     * @brief Count documents in a collection.
     *
     * @return 0 (not an error) if the collection is empty or does not exist.
     */
    [[nodiscard]] virtual Result<std::size_t> count(
        const CollectionId& collection) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryDocumentStore
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of IDocumentStore.
 *
 * Stores all records in a @c std::unordered_map keyed by
 * @c "<collection>:<id>".  Intended for unit tests and development; not
 * suitable for production use.
 */
class InMemoryDocumentStore final : public IDocumentStore {
public:
    Result<DocumentId> put(const DocumentRecord& record) override {
        if (record.id.empty()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_INVALID_ID,
                "document id must not be empty"));
        }
        std::lock_guard<std::mutex> lk(mu_);
        auto key = makeKey(record.collection_id, record.id);
        if (store_.count(key)) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_ALREADY_EXISTS, record.id));
        }
        DocumentRecord r = record;
        r.created_at = nowMs();
        r.updated_at = r.created_at;
        store_[key] = std::move(r);
        return record.id;
    }

    Result<std::optional<DocumentRecord>> get(
        const CollectionId& collection,
        const DocumentId&   id) const override
    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(makeKey(collection, id));
        if (it == store_.end()) {
            return std::optional<DocumentRecord>{std::nullopt};
        }
        return std::optional<DocumentRecord>{it->second};
    }

    Result<void> update(const CollectionId& collection,
                        const DocumentId&   id,
                        const nlohmann::json& body) override
    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(makeKey(collection, id));
        if (it == store_.end()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_NOT_FOUND, id));
        }
        it->second.body       = body;
        it->second.updated_at = nowMs();
        return Result<void>{};
    }

    Result<void> remove(const CollectionId& collection,
                        const DocumentId&   id) override
    {
        std::lock_guard<std::mutex> lk(mu_);
        store_.erase(makeKey(collection, id));
        return Result<void>{};
    }

    Result<std::vector<DocumentId>> list(
        const CollectionId& collection) const override
    {
        std::lock_guard<std::mutex> lk(mu_);
        const std::string prefix = collection + ":";
        std::vector<DocumentId> ids = {};

        for (const auto& [k, v] : store_) {
            if (k.size() >= prefix.size() &&
                k.compare(0, prefix.size(), prefix) == 0)
            {
                ids.push_back(v.id);
            }
        }
        return ids;
    }

    Result<std::size_t> count(
        const CollectionId& collection) const override
    {
        std::lock_guard<std::mutex> lk(mu_);
        const std::string prefix = collection + ":";
        std::size_t n = 0;
        for (const auto& [k, _] : store_) {
            if (k.size() >= prefix.size() &&
                k.compare(0, prefix.size(), prefix) == 0)
            {
                ++n;
            }
        }
        return n;
    }

private:
    static std::string makeKey(const CollectionId& col,
                               const DocumentId&   id)
    {
        return col + ":" + id;
    }

    static int64_t nowMs() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count();
    }

    mutable std::mutex mu_;
    std::unordered_map<std::string, DocumentRecord> store_;
};

} // namespace document
} // namespace themis

