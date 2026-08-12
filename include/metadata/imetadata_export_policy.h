/**
 * @file imetadata_export_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — Metadata Export Policy Interface
 *
 * Pluggable policy interface that controls which tables are pushed to
 * external metadata catalogs (Apache Atlas, DataHub), when they are
 * exported, and with what delay (to allow batching).
 *
 * Implementations ship in this header-only file:
 *   - IMetadataExportPolicy — abstract policy interface
 *   - AlwaysExportPolicy    — export everything immediately (default)
 *   - NeverExportPolicy     — suppress all exports (useful for testing)
 *   - FilteredExportPolicy  — exclude explicitly listed tables; uniform delay
 *
 * Design constraints:
 *  - shouldExport() must be non-blocking; ≤ 1 µs on the hot path.
 *  - exportDelay() returns the minimum delay before the export is issued;
 *    the caller may batch multiple changes within the window.
 *  - Implementations must be thread-safe.
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <mutex>
#include <set>
#include <string>
#include <string_view>

namespace themis {
namespace metadata {

// ── MetadataExportTrigger ─────────────────────────────────────────────────────

/**
 * @brief Events that can trigger an export to an external metadata catalog.
 */
enum class MetadataExportTrigger {
    SCHEMA_CREATED,     ///< A new table was discovered or created
    SCHEMA_MODIFIED,    ///< An existing table schema was updated
    SCHEMA_DROPPED,     ///< A table was removed from the catalog
    STATISTICS_UPDATED, ///< Statistics for a table were refreshed
};

// ── IMetadataExportPolicy ─────────────────────────────────────────────────────

/**
 * @brief Abstract policy interface for external metadata catalog exports.
 *
 * The CatalogExporter consults this policy before pushing metadata to
 * Apache Atlas or DataHub, enabling deployments to exclude internal
 * tables, rate-limit exports, or batch updates.
 *
 * Implementations MUST be thread-safe.
 */
class IMetadataExportPolicy {
public:
    virtual ~IMetadataExportPolicy() = default;

    /**
     * @brief Return true if @p table_name should be exported on @p trigger.
     *
     * @param table_name  The name of the table/collection.
     * @param trigger     The event that would trigger the export.
     * @return true       if the export should proceed; false to suppress it.
     */
    virtual bool shouldExport(std::string_view      table_name,
                              MetadataExportTrigger trigger) const = 0;

    /**
     * @brief Return the delay to apply before exporting @p table_name.
     *
     * A non-zero delay lets the exporter batch several changes that arrive
     * within the window.  Zero means export immediately.
     *
     * @param table_name  The name of the table/collection.
     * @param trigger     The event that triggered the export request.
     * @return            Delay in milliseconds (0 = immediate).
     */
    virtual std::chrono::milliseconds exportDelay(
        std::string_view      table_name,
        MetadataExportTrigger trigger) const = 0;
};

// ── AlwaysExportPolicy ────────────────────────────────────────────────────────

/**
 * @brief Default policy: export every table on every trigger, immediately.
 */
class AlwaysExportPolicy : public IMetadataExportPolicy {
public:
    bool shouldExport(std::string_view      /*table_name*/,
                      MetadataExportTrigger /*trigger*/) const override {
        return true;
    }

    std::chrono::milliseconds exportDelay(
        std::string_view      /*table_name*/,
        MetadataExportTrigger /*trigger*/) const override {
        return std::chrono::milliseconds{0};
    }
};

// ── NeverExportPolicy ─────────────────────────────────────────────────────────

/**
 * @brief Policy that suppresses all exports.
 *
 * Useful for offline mode, testing environments, or when the external
 * catalog connection is unavailable.
 */
class NeverExportPolicy : public IMetadataExportPolicy {
public:
    bool shouldExport(std::string_view      /*table_name*/,
                      MetadataExportTrigger /*trigger*/) const override {
        return false;
    }

    std::chrono::milliseconds exportDelay(
        std::string_view      /*table_name*/,
        MetadataExportTrigger /*trigger*/) const override {
        return std::chrono::milliseconds{0};
    }
};

// ── FilteredExportPolicy ──────────────────────────────────────────────────────

/**
 * @brief Thread-safe policy that excludes explicitly listed tables from export.
 *
 * Tables not on the exclusion list are exported on all triggers with the
 * configured uniform delay.  Exclusion matching is exact (no prefix/glob).
 *
 * Example:
 * @code
 *   FilteredExportPolicy policy{std::chrono::milliseconds{500}};
 *   policy.addExclusion("_internal_stats");
 *   policy.addExclusion("_tmp_migration");
 *   // "_internal_stats" is excluded; all other tables are exported after 500 ms.
 * @endcode
 */
class FilteredExportPolicy : public IMetadataExportPolicy {
public:
    static constexpr std::chrono::milliseconds kDefaultDelay{0};

    explicit FilteredExportPolicy(
        std::chrono::milliseconds delay = kDefaultDelay)
        : delay_(delay) {}

    /**
     * @brief Add @p table_name to the exclusion list.
     *
     * Subsequent shouldExport() calls for that name will return false.
     */
    void addExclusion(std::string_view table_name) {
        std::unique_lock<std::mutex> lk(mutex_);
        exclusions_.insert(std::string(table_name));
    }

    /**
     * @brief Remove @p table_name from the exclusion list.
     *
     * No-op if the name was not excluded.
     */
    void removeExclusion(std::string_view table_name) {
        std::unique_lock<std::mutex> lk(mutex_);
        exclusions_.erase(std::string(table_name));
    }

    // ── IMetadataExportPolicy ─────────────────────────────────────────────────

    bool shouldExport(std::string_view      table_name,
                      MetadataExportTrigger /*trigger*/) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return exclusions_.find(std::string(table_name)) == exclusions_.end();
    }

    std::chrono::milliseconds exportDelay(
        std::string_view      /*table_name*/,
        MetadataExportTrigger /*trigger*/) const override {
        return delay_;
    }

private:
    mutable std::mutex       mutex_;
    std::set<std::string>    exclusions_;
    std::chrono::milliseconds delay_;
};

} // namespace metadata
} // namespace themis
