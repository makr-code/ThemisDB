/**
 * @file importer_interfaces.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "importers/importer_interface.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

namespace themis {
namespace importers {

// ============================================================================
// IImportConflictResolver
// ============================================================================

enum class ConflictResolution {
    KEEP_EXISTING,        ///< Discard the incoming record; retain the existing one
    REPLACE_WITH_INCOMING,///< Overwrite the existing record with the incoming one
    MERGE_FIELDS,         ///< Merge fields from both records per FieldMergeSpec
    REJECT                ///< Treat the conflict as a fatal error; abort the batch
};

struct FieldMergeSpec {
    std::vector<std::string> take_from_incoming;
};

struct ConflictResolutionResult {
    ConflictResolution resolution = ConflictResolution::KEEP_EXISTING;
    FieldMergeSpec     merge_spec;  ///< Used only when resolution == MERGE_FIELDS
};

class IImportConflictResolver {
public:
    /**
     * @brief IImport Conflict Resolver.
     * @return Return value.
     */
    virtual ~IImportConflictResolver() = default;

    [[nodiscard]] virtual ConflictResolutionResult resolve(
        const json& existing,
        const json& incoming) const = 0;
};

// ============================================================================
// IFlatFileSchemaDetector
// ============================================================================

enum class SchemaConfidence { HIGH, MEDIUM, LOW };

struct SchemaDetectionResult {
    std::vector<std::string> columns;

    std::map<std::string, std::string> column_types;

    SchemaConfidence confidence = SchemaConfidence::LOW;

    std::string encoding = "utf-8";

    size_t rows_sampled = 0;

    std::vector<std::string> warnings;
};

class IFlatFileSchemaDetector {
public:
    /**
     * @brief IFlat File Schema Detector.
     * @return Return value.
     */
    virtual ~IFlatFileSchemaDetector() = default;

    [[nodiscard]] virtual SchemaDetectionResult detect(
        const std::string& file_path,
        size_t sample_rows = 0) const = 0;
};

// ============================================================================
// IKafkaConsumerSource
// ============================================================================

struct KafkaRecord {
    std::string topic;      ///< Kafka topic name
    int32_t     partition = 0;   ///< Partition index
    int64_t     offset    = -1;  ///< Partition offset
    std::string key;        ///< Message key (may be empty)
    std::string value;      ///< Raw message payload bytes
};

struct KafkaBatch {
    std::vector<KafkaRecord> records = {};

    bool                     empty() const { return records.empty(); }
    size_t                   size()  const { return records.size();  }
};

struct KafkaOffset {
    std::string topic;
    int32_t     partition = 0;
    int64_t     offset    = -1;  ///< Offset to commit (typically last consumed + 1)
};

enum class KafkaError {
    OK,                    ///< Operation succeeded
    MISSING_GROUP_ID,      ///< Consumer group ID was not provided in config
    MISSING_BROKERS,       ///< Broker list was not provided
    CONNECTION_FAILED,     ///< Could not connect to any broker
    TOPIC_NOT_FOUND,       ///< Topic does not exist on the broker
    POLL_TIMEOUT,          ///< poll() returned with no messages (timeout elapsed)
    COMMIT_FAILED,         ///< commitOffset() could not persist the offset
    AUTH_FAILED,           ///< SASL / SSL authentication failure
    UNKNOWN                ///< Unclassified error
};

class IKafkaConsumerSource {
public:
    /**
     * @brief IKafka Consumer Source.
     * @return Return value.
     */
    virtual ~IKafkaConsumerSource() = default;

    [[nodiscard]] virtual KafkaBatch poll(
        std::chrono::milliseconds timeout,
        KafkaError& err) = 0;

    [[nodiscard]] virtual KafkaError commitOffset(const KafkaOffset& offset) = 0;

    [[nodiscard]] virtual int64_t lag() const = 0;

    /**
     * @brief Close.
     */
    virtual void close() = 0;
};

// ============================================================================
// IIncrementalImportCursor
// ============================================================================

enum class CursorStatus {
    OK,                  ///< Batch populated successfully; more data may follow
    END_OF_STREAM,       ///< No more records; import is complete
    CHECKPOINT_REQUIRED, ///< Source requires an explicit commit before proceeding
    ERROR                ///< Fatal error; stop processing
};

class CheckpointToken {
public:
    CheckpointToken() = default;

    /**
     * @brief Checkpoint Token.
     * @param[in] serialized Input parameter.
     * @return Return value.
     */
    explicit CheckpointToken(std::string serialized)
        : value_(std::move(serialized)) {}

    const std::string& serialize() const { return value_; }

    bool valid() const { return !value_.empty(); }

    bool operator==(const CheckpointToken& o) const { return value_ == o.value_; }
    bool operator!=(const CheckpointToken& o) const { return value_ != o.value_; }

private:
    std::string value_;
};

struct ImportBatch {
    std::string source_table;

    std::vector<json> records;

    bool   empty()  const { return records.empty(); }
    size_t size()   const { return records.size();  }
};

class IIncrementalImportCursor {
public:
    /**
     * @brief IIncremental Import Cursor.
     * @return Return value.
     */
    virtual ~IIncrementalImportCursor() = default;

    [[nodiscard]] virtual CursorStatus next(ImportBatch& batch) = 0;

    [[nodiscard]] virtual CheckpointToken checkpoint() const = 0;

    [[nodiscard]] virtual int64_t estimatedRemainingRows() const = 0;

    /**
     * @brief Close.
     */
    virtual void close() = 0;
};

// ============================================================================
// IImporterPlugin  &  IImporterPluginRegistry
// ============================================================================

struct ImportConfig {
    std::string source_uri;

    std::string json_config;

    // -------------------------------------------------------------------------
    // Conflict resolution policy
    // -------------------------------------------------------------------------

    ConflictStrategy conflict_strategy = ConflictStrategy::OVERWRITE;

    std::vector<std::string> conflict_key_columns;

    std::vector<std::string> protected_fields;

    int merge_depth = 1;
};

class IImporterPlugin {
public:
    /**
     * @brief IImporter Plugin.
     * @return Return value.
     */
    virtual ~IImporterPlugin() = default;

    [[nodiscard]] virtual const char* pluginId() const = 0;

    [[nodiscard]] virtual std::vector<std::string> supportedSchemes() const = 0;

    [[nodiscard]] virtual std::unique_ptr<IImporter> createImporter(
        const ImportConfig& config) const = 0;
};

class IImporterPluginRegistry {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static IImporterPluginRegistry& instance();

    /**
     * @brief IImporter Plugin Registry.
     * @return Return value.
     */
    virtual ~IImporterPluginRegistry() = default;

    /**
     * @brief Register Plugin.
     * @param[in,out] plugin Input/output parameter.
     */
    virtual void registerPlugin(IImporterPlugin* plugin) = 0;

    [[nodiscard]] virtual IImporterPlugin* resolve(const std::string& source_uri) const = 0;

    [[nodiscard]] virtual std::vector<std::string> listPluginIds() const = 0;
};

// ============================================================================
// Concrete default registry (header-only singleton implementation)
// ============================================================================

class ImporterSchemeRegistry final : public IImporterPluginRegistry {
public:
    /**
     * @brief Instance.
     * @return Return value.
     * @details Implements instance without additional internal calls.
     */
    static ImporterSchemeRegistry& instance() {
        static ImporterSchemeRegistry reg;
        return reg;
    }

    void registerPlugin(IImporterPlugin* plugin) override {
        if (!plugin) {
          return;
        }
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::shared_mutex> lk(mutex_);
        for (const auto& scheme : plugin->supportedSchemes()) {
            schemes_[scheme] = plugin;
        }
        // Store unique plugin IDs for listPluginIds()
        const char* pid = plugin->pluginId();
        if (pid) {
            /**
             * @brief Id.
             * @param[in] pid Input parameter.
             * @return Return value.
             */
            std::string id(pid);
            if (std::find(plugin_ids_.begin(), plugin_ids_.end(), id)
                    == plugin_ids_.end()) {
                plugin_ids_.push_back(id);
            }
        }
    }

    IImporterPlugin* resolve(const std::string& source_uri) const override {
        const std::string scheme = extractScheme(source_uri);
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock<std::shared_mutex> lk(mutex_);
        auto it = schemes_.find(scheme);
        return (it != schemes_.end()) ? it->second : nullptr;
    }

    std::vector<std::string> listPluginIds() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock<std::shared_mutex> lk(mutex_);
        return plugin_ids_;
    }

private:
    ImporterSchemeRegistry() = default;

    /**
     * @brief Extract Scheme.
     * @param[in] uri Input parameter.
     * @return Return value.
     * @details Calls: find(), substr().
     */
    static std::string extractScheme(const std::string& uri) {
        const auto pos = uri.find("://");
        return (pos == std::string::npos) ? uri : uri.substr(0, pos);
    }

    mutable std::shared_mutex mutex_;
    std::map<std::string, IImporterPlugin*> schemes_;
    std::vector<std::string> plugin_ids_;
};

/**
 * @brief IImporterPluginRegistry::instance() delegates to ImporterSchemeRegistry
 * @return Return value.
 * @details Implements instance without additional internal calls.
 */
inline IImporterPluginRegistry& IImporterPluginRegistry::instance() {
    return ImporterSchemeRegistry::instance();
}

} // namespace importers
} // namespace themis

// ============================================================================
// REGISTER_IMPORTER_PLUGIN – static-init registration macro
// ============================================================================

/**
 * @brief Register an `IImporterPlugin`-derived class at program startup.
 *
 * Place this macro **once** at namespace scope in one translation unit of
 * the plugin.  @p PluginClass must:
 *   - Derive from `themis::importers::IImporterPlugin`.
 *   - Be default-constructible.
 *
 * The macro creates a function-local static instance of @p PluginClass and
 * registers it with `IImporterPluginRegistry::instance()` before `main()`
 * is entered.
 *
 * Example:
 * @code
 *   class MySQLPlugin : public themis::importers::IImporterPlugin { ... };
 *   REGISTER_IMPORTER_PLUGIN(MySQLPlugin)
 * @endcode
 *
 * @note Because the registration runs at static-init time, the registry is
 *       effectively read-only once `main()` begins.  Do not call
 *       `registerPlugin()` after the first `resolve()` call in production.
 */
#define REGISTER_IMPORTER_PLUGIN(PluginClass)                                     \
    namespace {                                                                    \
    struct PluginClass##_AutoRegistrar {                                           \
        PluginClass##_AutoRegistrar() {                                            \
            static PluginClass _plugin_instance;                                   \
            themis::importers::IImporterPluginRegistry::instance()                 \
                .registerPlugin(&_plugin_instance);                                \
        }                                                                          \
    };                                                                             \
    static PluginClass##_AutoRegistrar g_##PluginClass##_registrar;                \
    } /* anonymous namespace */
