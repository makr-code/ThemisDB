/**
 * @file debezium_format.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * Debezium-compatible change event envelope format for ThemisDB CDC.
 *
 * Produces JSON envelopes that match the Debezium unified change-event
 * schema, allowing downstream consumers (Kafka Connect transforms,
 * Debezium Server sinks, etc.) to process ThemisDB change events
 * without modification.
 *
 * Reference: https://debezium.io/documentation/reference/stable/
 *
 * Operation codes ("op" field):
 *   'c'  – create  (INSERT; EVENT_PUT without before_snapshot)
 *   'u'  – update  (UPDATE; EVENT_PUT with    before_snapshot)
 *   'd'  – delete  (DELETE; EVENT_DELETE)
 *   'r'  – read    (snapshot; EVENT_TRANSACTION_COMMIT / ROLLBACK)
 *
 * Envelope JSON structure (payload-only mode, include_schema = false):
 * @code
 * {
 *   "payload": {
 *     "before": null | { ... },
 *     "after":  null | { ... },
 *     "source": {
 *       "version":   "1.5.0-dev",
 *       "connector": "themisdb",
 *       "name":      "themis",
 *       "ts_ms":     1740000000000,
 *       "snapshot":  "false",
 *       "db":        "themisdb",
 *       "table":     "orders",
 *       "sequence":  42
 *     },
 *     "op":    "c",
 *     "ts_ms": 1740000000000
 *   }
 * }
 * @endcode
 *
 * When include_schema = true a "schema" block describing the envelope
 * structure is prepended.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "cdc/changefeed.h"

namespace themis {
namespace cdc {

enum class DebeziumOp {
    CREATE = 'c',  ///< Document was inserted (no prior state)
    UPDATE = 'u',  ///< Document was updated  (before and after available)
    DELETE = 'd',  ///< Document was deleted  (only before available)
    READ   = 'r',  ///< Snapshot / initial read
};

/**
 * @brief Debezium Op String.
 * @param[in] op Input parameter.
 * @return Return value.
 * @details Calls: std::string().
 */
inline std::string debeziumOpString(DebeziumOp op) {
    return std::string(1, static_cast<char>(op));
}

struct DebeziumSource {
    std::string version   = "1.5.0-dev"; ///< Connector / server version
    std::string connector = "themisdb";  ///< Connector type identifier
    std::string name      = "themis";    ///< Logical server / cluster name
    int64_t     ts_ms     = 0;           ///< Source event timestamp (epoch ms)
    std::string snapshot  = "false";     ///< "true" | "last" | "false"
    std::string db        = "themisdb";  ///< Database / keyspace name
    std::string table;                   ///< Collection / table name
    uint64_t    sequence  = 0;           ///< ThemisDB internal sequence number

    nlohmann::json toJson() const {
        return {
            {"version",   version},
            {"connector", connector},
            {"name",      name},
            {"ts_ms",     ts_ms},
            {"snapshot",  snapshot},
            {"db",        db},
            {"table",     table},
            {"sequence",  sequence}
        };
    }
};

struct DebeziumEnvelope {
    DebeziumOp     op     = DebeziumOp::READ;
    nlohmann::json before = nullptr;  ///< null for CREATE / READ
    nlohmann::json after  = nullptr;  ///< null for DELETE
    DebeziumSource source;
    int64_t        ts_ms  = 0;        ///< Processing timestamp (epoch ms)

    nlohmann::json toJson(bool include_schema = false) const {
        nlohmann::json payload = {
            {"before", before},
            {"after",  after},
            {"source", source.toJson()},
            {"op",     debeziumOpString(op)},
            {"ts_ms",  ts_ms},
            {"transaction", nullptr}
        };

        nlohmann::json result;
        result["payload"] = std::move(payload);

        if (include_schema) {
            result["schema"] = buildSchema(source.name, source.table);
        }

        return result;
    }

private:
    /**
     * @brief Build Schema.
     * @param[in] server_name Name of the server.
     * @param[in] table Input parameter.
     * @return Return value.
     * @details Calls: nlohmann::json::array(), makeValueField().
     */
    static nlohmann::json buildSchema(const std::string& server_name,
                                      const std::string& table) {
        const std::string value_schema_name =
            server_name + "." + table + ".Value";
        const std::string envelope_name =
            server_name + "." + table + ".Envelope";

        auto makeValueField = [&](const std::string& field_name) {
            return nlohmann::json{
                {"type",     "struct"},
                {"fields",   nlohmann::json::array({
                    nlohmann::json{{"field", "_document"},
                                   {"type",  "string"},
                                   {"optional", true}}
                })},
                {"optional", true},
                {"name",     value_schema_name},
                {"field",    field_name}
            };
        };

        return {
            {"type",     "struct"},
            {"optional", false},
            {"name",     envelope_name},
            {"fields",   nlohmann::json::array({
                makeValueField("before"),
                makeValueField("after"),
                nlohmann::json{
                 {"type",     "struct"},
                 {"optional", false},
                 {"name",     "io.debezium.connector.themisdb.Source"},
                 {"field",    "source"},
                 {"fields",   nlohmann::json::array({
                     nlohmann::json{{"field","version"},   {"type","string"}, {"optional",false}},
                     nlohmann::json{{"field","connector"}, {"type","string"}, {"optional",false}},
                     nlohmann::json{{"field","name"},      {"type","string"}, {"optional",false}},
                     nlohmann::json{{"field","ts_ms"},     {"type","int64"},  {"optional",false}},
                     nlohmann::json{{"field","snapshot"},  {"type","string"}, {"optional",true}},
                     nlohmann::json{{"field","db"},        {"type","string"}, {"optional",false}},
                     nlohmann::json{{"field","table"},     {"type","string"}, {"optional",false}},
                     nlohmann::json{{"field","sequence"},  {"type","int64"},  {"optional",false}}
                 })}},
                nlohmann::json{{"type","string"}, {"optional",false}, {"field","op"}},
                nlohmann::json{{"type","int64"},  {"optional",true},  {"field","ts_ms"}},
                nlohmann::json{{"type","struct"}, {"optional",true},  {"field","transaction"}}
            })}
        };
    }
};

class DebeziumFormatter {
public:
    struct Config {
        std::string server_name = "themis";    ///< Logical server / cluster name
        std::string db_name     = "themisdb";  ///< Database name in source block
        std::string version     = "1.5.0-dev"; ///< Connector version string
    };

    /**
     * @brief Debezium Formatter.
     * @return Return value.
     */
    explicit DebeziumFormatter() = default;
    explicit DebeziumFormatter(Config cfg) : cfg_(std::move(cfg)) {}

    DebeziumEnvelope toEnvelope(const Changefeed::ChangeEvent& event,
                                const std::string& collection = "") const {
        DebeziumEnvelope env;
        env.op    = opFromEvent(event);
        env.ts_ms = event.timestamp_ms;

        // Populate before / after JSON from snapshot fields
        if (event.before_snapshot.has_value()) {
            env.before = parseDocument(*event.before_snapshot);
        }
        if (event.after_snapshot.has_value()) {
            env.after = parseDocument(*event.after_snapshot);
        }

        // For PUT events, also use the value field as the "after" document
        // when after_snapshot is not set.
        if (event.type == Changefeed::ChangeEventType::EVENT_PUT &&
            !event.after_snapshot.has_value() &&
            event.value.has_value())
        {
            env.after = parseDocument(*event.value);
        }

        // Build source block
        env.source.version   = cfg_.version;
        env.source.connector = "themisdb";
        env.source.name      = cfg_.server_name;
        env.source.ts_ms     = event.timestamp_ms;
        env.source.db        = cfg_.db_name;
        env.source.table     = collection.empty()
                                   ? collectionFromKey(event.key)
                                   : collection;
        env.source.sequence  = event.sequence;

        // Propagate redaction marker into source metadata
        if (event.redacted) {
            // snapshot field encodes the redacted state for audit consumers
            env.source.snapshot = "redacted";
        }

        return env;
    }

    nlohmann::json toJson(const Changefeed::ChangeEvent& event,
                          const std::string& collection = "") const {
        return toEnvelope(event, collection).toJson(false);
    }

    nlohmann::json toJsonWithSchema(const Changefeed::ChangeEvent& event,
                                    const std::string& collection = "") const {
        return toEnvelope(event, collection).toJson(true);
    }

private:
    Config cfg_;

    /**
     * @brief Op From Event.
     * @param[in] event Input parameter.
     * @return Return value.
     * @details Calls: has_value().
     */
    static DebeziumOp opFromEvent(const Changefeed::ChangeEvent& event) {
        switch (event.type) {
            case Changefeed::ChangeEventType::EVENT_PUT:
                return event.before_snapshot.has_value()
                           ? DebeziumOp::UPDATE
                           : DebeziumOp::CREATE;
            case Changefeed::ChangeEventType::EVENT_DELETE:
                return DebeziumOp::DELETE;
            default:
                return DebeziumOp::READ;
        }
    }

    /**
     * @brief Collection From Key.
     * @param[in] key Input parameter.
     * @return Return value.
     * @details Calls: find(), substr().
     */
    static std::string collectionFromKey(const std::string& key) {
        const auto pos = key.find(':');
        return pos != std::string::npos ? key.substr(0, pos) : key;
    }

    /**
     * @brief Parse Document.
     * @param[in] s Input parameter.
     * @return Return value.
     * @details Calls: nlohmann::json::parse().
     */
    static nlohmann::json parseDocument(const std::string& s) {
        try {
            return nlohmann::json::parse(s);
        } catch (const nlohmann::json::parse_error&) {
            return {{"_raw", s}};
        }
    }
};

}  // namespace cdc
}  // namespace themis
