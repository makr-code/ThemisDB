/**
 * @file icdc_event_schema.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — CDC Schema Evolution Interface
 *
 * Defines the abstract interface for schema-aware CDC event streams and the
 * schema evolution hook that CDC consumers can register to receive
 * notifications when a collection's schema changes.
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md):
 *  - Incompatible schema evolution triggers an automatic
 *    ICDCPauseControl::pause() until the consumer resolves the conflict.
 *  - Schema validation prevents malformed events from reaching
 *    ICDCMaterializedViewHook; invalid events route to the error callback.
 *  - ICDCMaterializedViewHook methods are noexcept; exceptions terminate.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "cdc/changefeed.h"

namespace themis {
namespace cdc {

// ── SchemaFormat ──────────────────────────────────────────────────────────────

enum class SchemaFormat {
    JSON,     ///< JSON Schema (draft-07 or later)
    AVRO,     ///< Apache Avro schema definition (JSON encoding)
    PROTOBUF, ///< Protocol Buffers .proto definition (text encoding)
};

// ── SchemaCompatibility ───────────────────────────────────────────────────────

enum class SchemaCompatibility {
    NONE,      ///< No compatibility check
    BACKWARD,  ///< New schema can read data written with the old schema
    FORWARD,   ///< Old schema can read data written with the new schema
    FULL,      ///< Both BACKWARD and FORWARD
};

// ── MigrationStrategy ────────────────────────────────────────────────────────

enum class MigrationStrategy {
    Pause,         ///< Pause the stream until migration is complete
    DropOldFormat, ///< Discard events that do not conform to the new schema
    CoerceOldFormat, ///< Attempt field-level coercion; drop on failure
};

// ── SchemaConflict ────────────────────────────────────────────────────────────

struct SchemaConflict {
    std::string field;         ///< Affected field name (empty = whole-schema conflict)
    std::string old_type;      ///< Type in the old schema
    std::string new_type;      ///< Type in the new schema
    std::string description;   ///< Human-readable conflict description
};

// ── SchemaEvolutionDescriptor ─────────────────────────────────────────────────

struct SchemaEvolutionDescriptor {
    std::string           collection;     ///< Affected collection name
    int                   old_version{0}; ///< Previous schema version number
    int                   new_version{0}; ///< New schema version number
    SchemaFormat          format{SchemaFormat::JSON};
    SchemaCompatibility   compatibility{SchemaCompatibility::NONE};
    MigrationStrategy     strategy{MigrationStrategy::Pause};
    std::vector<std::string> affected_fields; ///< Fields that changed
    std::string           old_schema;     ///< Serialised old schema definition
    std::string           new_schema;     ///< Serialised new schema definition
    std::vector<SchemaConflict> conflicts; ///< Filled only for incompatible evolutions
};

// ── ISchemaEvolutionCallback ──────────────────────────────────────────────────

class ISchemaEvolutionCallback {
public:
    /**
     * @brief ISchema Evolution Callback.
     * @return Return value.
     */
    virtual ~ISchemaEvolutionCallback() = default;

    /**
     * @brief On Compatible.
     * @param[in] descriptor Input parameter.
     */
    virtual void onCompatible(const SchemaEvolutionDescriptor& descriptor) = 0;

    /**
     * @brief On Incompatible.
     * @param[in] descriptor Input parameter.
     * @param[in] conflict Input parameter.
     */
    virtual void onIncompatible(const SchemaEvolutionDescriptor& descriptor,
                                const SchemaConflict& conflict) = 0;
};

// ── ICDCEventSchema ───────────────────────────────────────────────────────────

class ICDCEventSchema {
public:
    /**
     * @brief ICDCEvent Schema.
     * @return Return value.
     */
    virtual ~ICDCEventSchema() = default;

    [[nodiscard]] virtual bool registerSchema(const std::string& collection,
                                const std::string& schema_def,
                                SchemaFormat       format,
                                int                version) = 0;

    [[nodiscard]] virtual std::string getSchema(const std::string& collection,
                                  int                version = -1) const = 0;

    [[nodiscard]] virtual int currentVersion(const std::string& collection) const = 0;

    /**
     * @brief On Schema Evolution.
     * @param[in] collection Input parameter.
     * @param[in] callback Input parameter.
     */
    virtual void onSchemaEvolution(
        const std::string&                          collection,
        std::shared_ptr<ISchemaEvolutionCallback>   callback) = 0;

    [[nodiscard]] virtual bool triggerEvolution(const SchemaEvolutionDescriptor& descriptor) = 0;
};

// ── InMemoryCDCEventSchema ────────────────────────────────────────────────────

class InMemoryCDCEventSchema : public ICDCEventSchema {
public:
    // ── ICDCEventSchema ──────────────────────────────────────────────────────

    bool registerSchema(const std::string& collection,
                        const std::string& schema_def,
                        SchemaFormat       format,
                        int                version) override
    {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        auto& versions = schemas_[collection];
        if (versions.count(version)) return false; // already registered
        versions[version] = {schema_def, format};
        if (version > latest_version_[collection]) {
            latest_version_[collection] = version;
        }
        return true;
    }

    std::string getSchema(const std::string& collection,
                          int                version = -1) const override
    {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        auto it = schemas_.find(collection);
        if (it == schemas_.end()) return {};
        const auto& versions = it->second;
        if (version == -1) {
            // Return latest
            auto lv = latest_version_.find(collection);
            if (lv == latest_version_.end()) return {};
            auto sv = versions.find(lv->second);
            return sv != versions.end() ? sv->second.definition : std::string{};
        }
        auto sv = versions.find(version);
        return sv != versions.end() ? sv->second.definition : std::string{};
    }

    int currentVersion(const std::string& collection) const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        auto it = latest_version_.find(collection);
        return it != latest_version_.end() ? it->second : -1;
    }

    void onSchemaEvolution(
        const std::string&                          collection,
        std::shared_ptr<ISchemaEvolutionCallback>   callback) override
    {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        callbacks_[collection] = std::move(callback);
    }

    bool triggerEvolution(const SchemaEvolutionDescriptor& descriptor) override {
        std::shared_ptr<ISchemaEvolutionCallback> cb;
        {
            /**
             * @brief Lk.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::unique_lock<std::mutex> lk(mutex_);
            auto it = callbacks_.find(descriptor.collection);
            if (it == callbacks_.end() || !it->second) {
              return false;
            }
            cb = it->second;
        }
        if (descriptor.conflicts.empty()) {
            cb->onCompatible(descriptor);
        } else {
            cb->onIncompatible(descriptor, descriptor.conflicts.front());
        }
        return true;
    }

private:
    struct SchemaEntry {
        std::string  definition;
        SchemaFormat format;
    };

    mutable std::mutex mutex_;
    // collection → version → SchemaEntry
    std::unordered_map<std::string,
        std::unordered_map<int, SchemaEntry>> schemas_;
    // collection → latest version number
    std::unordered_map<std::string, int> latest_version_;
    // collection → evolution callback
    std::unordered_map<std::string,
        std::shared_ptr<ISchemaEvolutionCallback>> callbacks_;
};

} // namespace cdc
} // namespace themis
