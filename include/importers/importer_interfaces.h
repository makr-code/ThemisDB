/**
 * @file importer_interfaces.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Outcome of a single conflict-resolution decision.
 *
 * Returned by `IImportConflictResolver::resolve()` to tell the import
 * pipeline how to handle a record that conflicts with an existing one.
 */
enum class ConflictResolution {
    KEEP_EXISTING,        ///< Discard the incoming record; retain the existing one
    REPLACE_WITH_INCOMING,///< Overwrite the existing record with the incoming one
    MERGE_FIELDS,         ///< Merge fields from both records per FieldMergeSpec
    REJECT                ///< Treat the conflict as a fatal error; abort the batch
};

/**
 * @brief Per-field merge specification for the `MERGE_FIELDS` resolution.
 *
 * When `ConflictResolution::MERGE_FIELDS` is returned the pipeline applies
 * this spec: fields listed in `take_from_incoming` are copied from the
 * incoming record; all other fields are taken from the existing one.
 *
 * If `take_from_incoming` is empty every field from the incoming record
 * wins (equivalent to `REPLACE_WITH_INCOMING`).
 */
struct FieldMergeSpec {
    /// Field names to take from the incoming record; all others kept from existing.
    std::vector<std::string> take_from_incoming;
};

/**
 * @brief Result of a conflict-resolution decision, combining the outcome
 *        enum and an optional field-merge specification.
 */
struct ConflictResolutionResult {
    ConflictResolution resolution = ConflictResolution::KEEP_EXISTING;
    FieldMergeSpec     merge_spec;  ///< Used only when resolution == MERGE_FIELDS
};

/**
 * @brief Stateless conflict-resolution interface.
 *
 * Called by the import pipeline when an incoming record has the same
 * conflict key as a previously processed record.  Implementations **must**
 * be stateless and safe to call concurrently from multiple worker threads.
 *
 * ### Difference from ImportConflictResolver
 *
 * `ImportConflictResolver` (in `conflict_resolver.h`) is a *stateful*
 * session-scoped resolver that maintains a per-table key registry.
 * `IImportConflictResolver` is a *stateless* interface for policy objects
 * that can be shared across sessions and called concurrently.
 *
 * ### Thread-safety
 *
 * `resolve()` must be safe to call from multiple threads simultaneously
 * without external synchronisation.
 *
 * ### Example
 * @code
 *   class AlwaysMergeResolver : public IImportConflictResolver {
 *   public:
 *       ConflictResolutionResult resolve(
 *           const json& existing, const json& incoming) const override {
 *           ConflictResolutionResult r;
 *           r.resolution = ConflictResolution::MERGE_FIELDS;
 *           // All fields from incoming win; no field is protected.
 *           return r;
 *       }
 *   };
 * @endcode
 */
class IImportConflictResolver {
public:
    virtual ~IImportConflictResolver() = default;

    /**
     * @brief Decide how to resolve a conflict between two records.
     *
     * @param existing  The record already present in the destination collection.
     * @param incoming  The record from the current import source that conflicts.
     * @return          Resolution decision (outcome + optional merge spec).
     */
    [[nodiscard]] virtual ConflictResolutionResult resolve(
        const json& existing,
        const json& incoming) const = 0;
};

// ============================================================================
// IFlatFileSchemaDetector
// ============================================================================

/**
 * @brief Confidence level of an auto-detected flat-file schema.
 *
 * - HIGH   – All sampled rows are consistent with the inferred types;
 *             the caller can use the schema without manual review.
 * - MEDIUM – Majority of rows are consistent; minor inconsistencies seen;
 *             the caller should review before applying the schema.
 * - LOW    – Schema detection was inconclusive (too few rows, mixed types,
 *             or encoding issues); callers should supply a manual schema.
 */
enum class SchemaConfidence { HIGH, MEDIUM, LOW };

/**
 * @brief Result produced by `IFlatFileSchemaDetector::detect()`.
 *
 * Contains the inferred schema and metadata about detection quality.
 */
struct SchemaDetectionResult {
    /// Ordered list of detected column names.
    std::vector<std::string> columns;

    /// Per-column inferred type name string (e.g., "integer", "double",
    /// "boolean", "string").  Key is the column name.
    std::map<std::string, std::string> column_types;

    /// Overall schema confidence based on row consistency.
    SchemaConfidence confidence = SchemaConfidence::LOW;

    /// Detected (or assumed) character encoding of the file.
    /// Values: "utf-8", "utf-16-le", "utf-16-be", "latin-1", "unknown".
    std::string encoding = "utf-8";

    /// Number of rows sampled to produce this result.
    size_t rows_sampled = 0;

    /// Human-readable description of any issues encountered during detection.
    std::vector<std::string> warnings;
};

/**
 * @brief Advisory flat-file schema detection interface.
 *
 * Implementations sample up to `sample_rows` rows from a file and infer
 * column names and types from the content.  Detection is *advisory*:
 * callers are free to proceed with a manually supplied schema when
 * `confidence == SchemaConfidence::LOW`.
 *
 * ### Encoding handling
 *
 * Implementations must detect and handle:
 * - BOM-prefixed UTF-8 (strip BOM, report "utf-8")
 * - UTF-16 LE/BE (convert internally, report "utf-16-le" / "utf-16-be")
 * - Latin-1 / ISO-8859-1 (report "latin-1")
 *
 * ### Thread-safety
 *
 * `detect()` may be called concurrently from multiple threads.
 * Implementations must not rely on mutable instance state.
 */
class IFlatFileSchemaDetector {
public:
    virtual ~IFlatFileSchemaDetector() = default;

    /**
     * @brief Sample @p sample_rows rows from @p file_path and infer the schema.
     *
     * @param file_path    Absolute path to the file to sample.
     * @param sample_rows  Maximum number of data rows to sample.
     *                     0 means "use implementation default" (typically 1 000).
     * @return             Detection result; confidence reflects how well the
     *                     sampled rows fit the inferred types.
     */
    [[nodiscard]] virtual SchemaDetectionResult detect(
        const std::string& file_path,
        size_t sample_rows = 0) const = 0;
};

// ============================================================================
// IKafkaConsumerSource
// ============================================================================

/**
 * @brief A single Kafka message delivered to a consumer.
 */
struct KafkaRecord {
    std::string topic;      ///< Kafka topic name
    int32_t     partition = 0;   ///< Partition index
    int64_t     offset    = -1;  ///< Partition offset
    std::string key;        ///< Message key (may be empty)
    std::string value;      ///< Raw message payload bytes
};

/**
 * @brief A batch of Kafka records returned by a single `poll()` call.
 */
struct KafkaBatch {
    std::vector<KafkaRecord> records;
    bool                     empty() const { return records.empty(); }
    size_t                   size()  const { return records.size();  }
};

/**
 * @brief An explicit commit target identifying a position in a topic/partition.
 */
struct KafkaOffset {
    std::string topic;
    int32_t     partition = 0;
    int64_t     offset    = -1;  ///< Offset to commit (typically last consumed + 1)
};

/**
 * @brief Error code returned by `IKafkaConsumerSource` operations.
 */
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

/**
 * @brief Async Kafka consumer source interface.
 *
 * Provides cursor-style access to a Kafka topic's messages.  Offset
 * commits are **decoupled** from message delivery: callers must call
 * `commitOffset()` explicitly after processing each `KafkaBatch`.
 *
 * ### Offset semantics
 *
 * - Delivery guarantee: at-least-once (messages may be re-delivered
 *   after a restart if `commitOffset()` was not called).
 * - Callers should commit the offset only after writing the batch to
 *   the destination, not before.
 *
 * ### Credentials
 *
 * SASL authentication parameters (username, password, SSL CA) are passed
 * at construction time via `KafkaSourceConfig` stored in the JSON config
 * supplied to `IImporter::initialize()`.  Credential values are **never**
 * stored in this interface type and must not appear in log output.
 *
 * ### Consumer group
 *
 * A non-empty consumer group ID is mandatory.  Construction of a concrete
 * implementation must fail with `KafkaError::MISSING_GROUP_ID` if the
 * group ID is absent in the supplied configuration.
 *
 * ### Thread-safety
 *
 * Instances are **not** thread-safe.  Use one instance per consumer thread.
 */
class IKafkaConsumerSource {
public:
    virtual ~IKafkaConsumerSource() = default;

    /**
     * @brief Poll the Kafka broker for the next batch of messages.
     *
     * Blocks until at least one message is available or @p timeout elapses.
     *
     * @param timeout  Maximum time to wait for messages.
     * @param[out] err Set to `KafkaError::OK` on success; error code on failure.
     * @return         Batch of records (may be empty on timeout).
     */
    [[nodiscard]] virtual KafkaBatch poll(
        std::chrono::milliseconds timeout,
        KafkaError& err) = 0;

    /**
     * @brief Commit the given offset to the Kafka broker.
     *
     * Should be called after the caller has successfully processed all
     * records up to (and including) @p offset.offset - 1.
     *
     * @param offset  Offset to commit.
     * @return        `KafkaError::OK` on success; error code otherwise.
     */
    [[nodiscard]] virtual KafkaError commitOffset(const KafkaOffset& offset) = 0;

    /**
     * @brief Return the current lag (messages not yet consumed) for the
     *        subscribed topic/partition.
     *
     * @return Estimated number of unconsumed messages; -1 if unknown.
     */
    [[nodiscard]] virtual int64_t lag() const = 0;

    /**
     * @brief Close the consumer and release all broker connections.
     *
     * Safe to call multiple times (idempotent).
     */
    virtual void close() = 0;
};

// ============================================================================
// IIncrementalImportCursor
// ============================================================================

/**
 * @brief Status code returned by `IIncrementalImportCursor::next()`.
 */
enum class CursorStatus {
    OK,                  ///< Batch populated successfully; more data may follow
    END_OF_STREAM,       ///< No more records; import is complete
    CHECKPOINT_REQUIRED, ///< Source requires an explicit commit before proceeding
    ERROR                ///< Fatal error; stop processing
};

/**
 * @brief A serializable opaque token representing a cursor position.
 *
 * Tokens can be persisted to disk (as a UTF-8 string) and used to
 * resume a cursor from the last committed position after a restart.
 *
 * The internal representation is an opaque, signed string; the
 * signature prevents injection of forged resume positions.
 */
class CheckpointToken {
public:
    CheckpointToken() = default;

    /// Construct from a serialized token string (as returned by `serialize()`).
    explicit CheckpointToken(std::string serialized)
        : value_(std::move(serialized)) {}

    /// Returns the serialized string representation (safe to persist to disk).
    const std::string& serialize() const { return value_; }

    /// Returns `true` if the token holds a non-empty value.
    bool valid() const { return !value_.empty(); }

    bool operator==(const CheckpointToken& o) const { return value_ == o.value_; }
    bool operator!=(const CheckpointToken& o) const { return value_ != o.value_; }

private:
    std::string value_;
};

/**
 * @brief A batch of records yielded by one `IIncrementalImportCursor::next()` call.
 */
struct ImportBatch {
    /// Source table / collection name that these records belong to.
    std::string source_table;

    /// Converted records as JSON objects (field-name → value).
    std::vector<json> records;

    bool   empty()  const { return records.empty(); }
    size_t size()   const { return records.size();  }
};

/**
 * @brief Pull-based resumable import cursor interface.
 *
 * Provides a standardised pull API for incremental and checkpoint-based
 * import from any source type (database, file, stream).
 *
 * ### Resumption
 *
 * At any point the caller may call `checkpoint()` to obtain a
 * `CheckpointToken`.  The token can be serialized and stored durably.
 * On restart, pass the token to `IImporter::openCursor(token)` (when
 * supported by the concrete importer) to resume without duplicating
 * already-processed records.
 *
 * ### End of stream
 *
 * `next()` returns `CursorStatus::END_OF_STREAM` when no more records
 * are available.  The batch is empty in this case.
 *
 * ### Checkpoint required
 *
 * Some sources (e.g., Kafka) require an explicit commit before
 * proceeding.  When `CursorStatus::CHECKPOINT_REQUIRED` is returned
 * the caller must call `checkpoint()` (and optionally persist the token)
 * before calling `next()` again.
 *
 * @note A cursor is **not** thread-safe.  Use one cursor per thread.
 */
class IIncrementalImportCursor {
public:
    virtual ~IIncrementalImportCursor() = default;

    /**
     * @brief Fetch the next batch of records.
     *
     * @param[out] batch  Populated with the next batch on `CursorStatus::OK`.
     *                    Empty on `END_OF_STREAM`, `CHECKPOINT_REQUIRED`, or `ERROR`.
     * @return            Status of the operation.
     */
    [[nodiscard]] virtual CursorStatus next(ImportBatch& batch) = 0;

    /**
     * @brief Capture the current cursor position as a serializable token.
     *
     * May be called at any time, even before the first `next()` call
     * (token represents the start position in that case).
     *
     * @return  Serializable checkpoint token.
     */
    [[nodiscard]] virtual CheckpointToken checkpoint() const = 0;

    /**
     * @brief Estimated number of records remaining, or -1 if unknown.
     *
     * Streaming sources (e.g., Kafka) typically return -1.  File-backed
     * sources may return an estimate based on remaining file size.
     */
    [[nodiscard]] virtual int64_t estimatedRemainingRows() const = 0;

    /**
     * @brief Close the cursor and release any held resources.
     *
     * Safe to call multiple times (idempotent).
     */
    virtual void close() = 0;
};

// ============================================================================
// IImporterPlugin  &  IImporterPluginRegistry
// ============================================================================

/**
 * @brief Configuration bundle passed to `IImporterPlugin::createImporter()`.
 *
 * In addition to the source location and JSON initialisation string, callers
 * may pre-configure the per-import-job conflict resolution policy here.  When
 * a plugin's `createImporter()` implementation propagates these fields into
 * the `ImportOptions` that it passes to each `IImporter::importData()` call,
 * operators can control conflict behaviour from a single configuration point.
 */
struct ImportConfig {
    /// Source URI, e.g. "mysql://host:3306/dbname" or "s3://bucket/key".
    std::string source_uri;

    /// JSON configuration string forwarded verbatim to `IImporter::initialize()`.
    std::string json_config;

    // -------------------------------------------------------------------------
    // Conflict resolution policy
    // -------------------------------------------------------------------------

    /// Strategy to apply when the same conflict key is encountered more than
    /// once during an import session.
    /// Default is OVERWRITE for backward compatibility.
    ConflictStrategy conflict_strategy = ConflictStrategy::OVERWRITE;

    /// Columns whose values form the conflict detection key.
    /// If empty, no in-session conflict detection is performed.
    /// Example: {"id"} or {"tenant_id", "user_id"}
    std::vector<std::string> conflict_key_columns;

    /// Fields that the MERGE strategy must not overwrite with incoming values.
    /// Ignored by SKIP, OVERWRITE, and ERROR strategies.
    std::vector<std::string> protected_fields;

    /// Recursion depth for the MERGE strategy.
    ///  1  = top-level fields only (default; nested objects replaced entirely).
    /// -1  = deep recursive merge for all nested JSON objects.
    ///  N  = merge up to N levels deep.
    int merge_depth = 1;
};

/**
 * @brief URI-scheme-based importer plugin entry point.
 *
 * An `IImporterPlugin` is a lightweight factory descriptor that:
 *   1. Declares which URI schemes it handles (`supportedSchemes()`).
 *   2. Creates a fully-initialised `IImporter` for a given `ImportConfig`
 *      (`createImporter()`).
 *
 * Plugins are registered at static-init time via the
 * `REGISTER_IMPORTER_PLUGIN` macro; the registry becomes logically
 * read-only once the first `resolve()` call has been made.
 *
 * ### Example
 * @code
 *   class MySQLPlugin : public IImporterPlugin {
 *   public:
 *       const char* pluginId() const override { return "mysql_plugin"; }
 *
 *       std::vector<std::string> supportedSchemes() const override {
 *           return {"mysql", "mariadb"};
 *       }
 *
 *       std::unique_ptr<IImporter> createImporter(
 *               const ImportConfig& cfg) const override {
 *           auto imp = std::make_unique<MySQLImporter>();
 *           imp->initialize(cfg.json_config);
 *           return imp;
 *       }
 *   };
 *
 *   REGISTER_IMPORTER_PLUGIN(MySQLPlugin)
 * @endcode
 *
 * @note `createImporter()` must **not** throw; exceptions from the
 *       implementation are caught by the registry and converted to a
 *       `nullptr` return value.
 */
class IImporterPlugin {
public:
    virtual ~IImporterPlugin() = default;

    /// Unique identifier for this plugin (snake_case recommended).
    [[nodiscard]] virtual const char* pluginId() const = 0;

    /**
     * @brief URI schemes handled by this plugin.
     *
     * The scheme is the part before "://" in a source URI.
     * Example: { "mysql", "mariadb" } for a MySQL/MariaDB plugin.
     */
    [[nodiscard]] virtual std::vector<std::string> supportedSchemes() const = 0;

    /**
     * @brief Create a new, initialised importer for the given configuration.
     *
     * @param config  Source URI and JSON configuration.
     * @return        Initialised `IImporter` instance, or `nullptr` on failure.
     *                Must **not** throw.
     */
    [[nodiscard]] virtual std::unique_ptr<IImporter> createImporter(
        const ImportConfig& config) const = 0;
};

/**
 * @brief Thread-safe registry that maps URI schemes to importer plugins.
 *
 * `resolve(uri)` extracts the scheme from a source URI and returns the
 * matching `IImporterPlugin*`, or `nullptr` if no plugin handles that
 * scheme.
 *
 * Plugins are expected to register themselves at static-init time via
 * `REGISTER_IMPORTER_PLUGIN`.  After the first `resolve()` call the
 * registry is treated as logically sealed: `registerPlugin()` will still
 * succeed but callers should not rely on late registration in production.
 *
 * @see REGISTER_IMPORTER_PLUGIN
 */
class IImporterPluginRegistry {
public:
    /**
     * @brief Process-wide singleton.
     */
    static IImporterPluginRegistry& instance();

    virtual ~IImporterPluginRegistry() = default;

    /**
     * @brief Register a plugin.
     *
     * If a plugin with overlapping schemes already exists it is replaced
     * for those schemes.
     *
     * @param plugin  Non-owning pointer; the plugin object's lifetime must
     *                exceed the registry's lifetime.
     */
    virtual void registerPlugin(IImporterPlugin* plugin) = 0;

    /**
     * @brief Resolve a source URI to the matching plugin.
     *
     * Extracts the scheme from @p source_uri (text before "://", or the
     * entire string if no "://" is present) and returns the registered
     * plugin that declared that scheme via `supportedSchemes()`.
     *
     * @param source_uri  Source URI, e.g. "mysql://host:3306/dbname".
     * @return            Matching plugin, or `nullptr` if none registered.
     *                    Never throws.
     */
    [[nodiscard]] virtual IImporterPlugin* resolve(const std::string& source_uri) const = 0;

    /**
     * @brief List all registered plugin IDs.
     */
    [[nodiscard]] virtual std::vector<std::string> listPluginIds() const = 0;
};

// ============================================================================
// Concrete default registry (header-only singleton implementation)
// ============================================================================

/**
 * @brief Default, scheme-keyed implementation of IImporterPluginRegistry.
 *
 * Used by `IImporterPluginRegistry::instance()` and the
 * `REGISTER_IMPORTER_PLUGIN` macro.
 *
 * ### Thread-safety
 *
 * Uses a `std::shared_mutex` internally so that concurrent `resolve()` calls
 * (read path) proceed without exclusive locking.  `registerPlugin()` acquires
 * a unique (write) lock.  On a read-heavy workload (many concurrent imports)
 * this avoids serialising all `resolve()` calls behind a single mutex.
 */
class ImporterSchemeRegistry final : public IImporterPluginRegistry {
public:
    static ImporterSchemeRegistry& instance() {
        static ImporterSchemeRegistry reg;
        return reg;
    }

    void registerPlugin(IImporterPlugin* plugin) override {
        if (!plugin) {
          return;
        }
        std::unique_lock<std::shared_mutex> lk(mutex_);
        for (const auto& scheme : plugin->supportedSchemes()) {
            schemes_[scheme] = plugin;
        }
        // Store unique plugin IDs for listPluginIds()
        const char* pid = plugin->pluginId();
        if (pid) {
            std::string id(pid);
            if (std::find(plugin_ids_.begin(), plugin_ids_.end(), id)
                    == plugin_ids_.end()) {
                plugin_ids_.push_back(id);
            }
        }
    }

    /// @note `resolve()` acquires a shared (read) lock and is safe to call
    ///       concurrently from multiple threads.
    IImporterPlugin* resolve(const std::string& source_uri) const override {
        const std::string scheme = extractScheme(source_uri);
        std::shared_lock<std::shared_mutex> lk(mutex_);
        auto it = schemes_.find(scheme);
        return (it != schemes_.end()) ? it->second : nullptr;
    }

    std::vector<std::string> listPluginIds() const override {
        std::shared_lock<std::shared_mutex> lk(mutex_);
        return plugin_ids_;
    }

private:
    ImporterSchemeRegistry() = default;

    static std::string extractScheme(const std::string& uri) {
        const auto pos = uri.find("://");
        return (pos == std::string::npos) ? uri : uri.substr(0, pos);
    }

    mutable std::shared_mutex mutex_;
    std::map<std::string, IImporterPlugin*> schemes_;
    std::vector<std::string> plugin_ids_;
};

// IImporterPluginRegistry::instance() delegates to ImporterSchemeRegistry
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
