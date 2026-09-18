/**
 * @file document_store.h
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

using DocumentId = std::string;

using CollectionId = std::string;

// ─────────────────────────────────────────────────────────────────────────────
// DocumentRecord
// ─────────────────────────────────────────────────────────────────────────────

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

class IDocumentStore {
public:
    /**
     * @brief IDocument Store.
     * @return Return value.
     */
    virtual ~IDocumentStore() = default;

    [[nodiscard]] virtual Result<DocumentId> put(const DocumentRecord& record) = 0;

    [[nodiscard]] virtual Result<std::optional<DocumentRecord>> get(
        const CollectionId& collection, const DocumentId& id) const = 0;

    [[nodiscard]] virtual Result<void> update(const CollectionId& collection,
                                const DocumentId&   id,
                                const nlohmann::json& body) = 0;

    [[nodiscard]] virtual Result<void> remove(const CollectionId& collection,
                                const DocumentId&   id) = 0;

    [[nodiscard]] virtual Result<std::vector<DocumentId>> list(
        const CollectionId& collection) const = 0;

    [[nodiscard]] virtual Result<std::size_t> count(
        const CollectionId& collection) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryDocumentStore
// ─────────────────────────────────────────────────────────────────────────────

class InMemoryDocumentStore final : public IDocumentStore {
public:
    Result<DocumentId> put(const DocumentRecord& record) override {
        if (record.id.empty()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_INVALID_ID,
                "document id must not be empty"));
        }
        /**
         * @brief Lk.
         * @param[in] mu_ Input parameter.
         * @return Return value.
         */
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
        /**
         * @brief Lk.
         * @param[in] mu_ Input parameter.
         * @return Return value.
         */
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
        /**
         * @brief Lk.
         * @param[in] mu_ Input parameter.
         * @return Return value.
         */
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
        /**
         * @brief Lk.
         * @param[in] mu_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(mu_);
        store_.erase(makeKey(collection, id));
        return Result<void>{};
    }

    Result<std::vector<DocumentId>> list(
        const CollectionId& collection) const override
    {
        /**
         * @brief Lk.
         * @param[in] mu_ Input parameter.
         * @return Return value.
         */
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
        /**
         * @brief Lk.
         * @param[in] mu_ Input parameter.
         * @return Return value.
         */
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
    /**
     * @brief Make Key.
     * @param[in] col Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    static std::string makeKey(const CollectionId& col,
                               const DocumentId&   id)
    {
        return col + ":" + id;
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

    mutable std::mutex mu_;
    std::unordered_map<std::string, DocumentRecord> store_;
};

} // namespace document
} // namespace themis

