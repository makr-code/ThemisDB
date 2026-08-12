/**
 * @file icdc_event_schema.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Wire-format / encoding of a collection schema.
 */
enum class SchemaFormat {
    JSON,     ///< JSON Schema (draft-07 or later)
    AVRO,     ///< Apache Avro schema definition (JSON encoding)
    PROTOBUF, ///< Protocol Buffers .proto definition (text encoding)
};

// ── SchemaCompatibility ───────────────────────────────────────────────────────

/**
 * @brief Avro/Confluent schema compatibility mode.
 */
enum class SchemaCompatibility {
    NONE,      ///< No compatibility check
    BACKWARD,  ///< New schema can read data written with the old schema
    FORWARD,   ///< Old schema can read data written with the new schema
    FULL,      ///< Both BACKWARD and FORWARD
};

// ── MigrationStrategy ────────────────────────────────────────────────────────

/**
 * @brief Strategy for handling in-flight events during schema evolution.
 */
enum class MigrationStrategy {
    Pause,         ///< Pause the stream until migration is complete
    DropOldFormat, ///< Discard events that do not conform to the new schema
    CoerceOldFormat, ///< Attempt field-level coercion; drop on failure
};

// ── SchemaConflict ────────────────────────────────────────────────────────────

/**
 * @brief Describes why two schema versions are incompatible.
 */
struct SchemaConflict {
    std::string field;         ///< Affected field name (empty = whole-schema conflict)
    std::string old_type;      ///< Type in the old schema
    std::string new_type;      ///< Type in the new schema
    std::string description;   ///< Human-readable conflict description
};

// ── SchemaEvolutionDescriptor ─────────────────────────────────────────────────

/**
 * @brief Carries all information about a schema change event.
 *
 * Passed to ISchemaEvolutionCallback::onCompatible() /
 * ISchemaEvolutionCallback::onIncompatible() by the CDC schema evolution
 * machinery.
 */
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

/**
 * @brief Callback interface for schema evolution events.
 *
 * Registered with ICDCEventSchema::onSchemaEvolution().  The CDC layer
 * invokes the appropriate method when it detects a schema change.
 *
 * Thread-safety: implementations must be thread-safe.
 */
class ISchemaEvolutionCallback {
public:
    virtual ~ISchemaEvolutionCallback() = default;

    /**
     * @brief Called when the new schema is compatible with the old one.
     *
     * The stream continues without interruption.
     *
     * @param descriptor  Describes the schema change.
     */
    virtual void onCompatible(const SchemaEvolutionDescriptor& descriptor) = 0;

    /**
     * @brief Called when the new schema is incompatible with the old one.
     *
     * The CDC layer will have already paused the stream (if configured).
     * The implementation must return before the stream can be resumed.
     *
     * @param descriptor  Describes the schema change and the conflicts.
     * @param conflict    The first (or most critical) conflict detected.
     */
    virtual void onIncompatible(const SchemaEvolutionDescriptor& descriptor,
                                const SchemaConflict& conflict) = 0;
};

// ── ICDCEventSchema ───────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for schema-aware CDC event streams.
 *
 * Consumers register a schema and a schema-evolution callback for each
 * collection they subscribe to.  The CDC layer validates every ChangeEvent
 * against the registered schema and routes invalid events to the error
 * callback instead of the main onEvents path.
 *
 * Thread-safety: all methods must be thread-safe in every implementation.
 *
 * Design constraints:
 *  - Schema is immutable after registration for a given collection/version.
 *  - onSchemaEvolution() may be called from any thread that detects a schema
 *    change; the implementation must not block the caller indefinitely.
 */
class ICDCEventSchema {
public:
    virtual ~ICDCEventSchema() = default;

    /**
     * @brief Register a schema for a collection.
     *
     * @param collection  The collection to register the schema for.
     * @param schema_def  Serialised schema definition (JSON Schema / Avro / Proto).
     * @param format      Encoding format of schema_def.
     * @param version     Schema version (monotonically increasing).
     * @return true if the schema was registered; false if a schema for this
     *         collection at this version already exists.
     */
    [[nodiscard]] virtual bool registerSchema(const std::string& collection,
                                const std::string& schema_def,
                                SchemaFormat       format,
                                int                version) = 0;

    /**
     * @brief Look up the registered schema for a collection.
     *
     * @param collection  Collection name.
     * @param version     Schema version (-1 = latest).
     * @return The schema definition string, or empty if not found.
     */
    [[nodiscard]] virtual std::string getSchema(const std::string& collection,
                                  int                version = -1) const = 0;

    /**
     * @brief Current schema version for a collection (-1 if none registered).
     */
    [[nodiscard]] virtual int currentVersion(const std::string& collection) const = 0;

    /**
     * @brief Register (or replace) the schema evolution callback for a collection.
     *
     * @param collection  Collection name.
     * @param callback    Non-null callback; ownership is shared.
     */
    virtual void onSchemaEvolution(
        const std::string&                          collection,
        std::shared_ptr<ISchemaEvolutionCallback>   callback) = 0;

    /**
     * @brief Simulate a schema evolution event (primarily for testing).
     *
     * Triggers the registered callback for @p collection with the provided
     * descriptor.
     *
     * @return true if a callback was registered and invoked; false otherwise.
     */
    [[nodiscard]] virtual bool triggerEvolution(const SchemaEvolutionDescriptor& descriptor) = 0;
};

// ── InMemoryCDCEventSchema ────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of ICDCEventSchema.
 *
 * Suitable for unit tests and standalone use.  Stores schemas in a nested
 * map (collection → version → definition) and callbacks in a flat map
 * (collection → callback).
 */
class InMemoryCDCEventSchema : public ICDCEventSchema {
public:
    // ── ICDCEventSchema ──────────────────────────────────────────────────────

    bool registerSchema(const std::string& collection,
                        const std::string& schema_def,
                        SchemaFormat       format,
                        int                version) override
    {
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
        std::unique_lock<std::mutex> lk(mutex_);
        auto it = latest_version_.find(collection);
        return it != latest_version_.end() ? it->second : -1;
    }

    void onSchemaEvolution(
        const std::string&                          collection,
        std::shared_ptr<ISchemaEvolutionCallback>   callback) override
    {
        std::unique_lock<std::mutex> lk(mutex_);
        callbacks_[collection] = std::move(callback);
    }

    bool triggerEvolution(const SchemaEvolutionDescriptor& descriptor) override {
        std::shared_ptr<ISchemaEvolutionCallback> cb;
        {
            std::unique_lock<std::mutex> lk(mutex_);
            auto it = callbacks_.find(descriptor.collection);
            if (it == callbacks_.end() || !it->second) return false;
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
