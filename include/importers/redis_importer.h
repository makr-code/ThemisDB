#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>
#include <string>
#include <vector>
#include <map>

// Forward-declare hiredis context for the THEMIS_ENABLE_REDIS production path.
// The full definition is only visible in redis_importer.cpp which includes
// <hiredis/hiredis.h> under the same build guard.
#ifdef THEMIS_ENABLE_REDIS
struct redisContext;
#endif

namespace themis {
namespace importers {

/**
 * @brief Redis Key/Value Importer
 *
 * Imports keys from a Redis server into ThemisDB collections.  Supports
 * String, Hash, List, Set, and Sorted Set (ZSet) value types with
 * configurable key-pattern filtering.
 *
 * Supported capabilities:
 * - Pattern-based key selection (SCAN with MATCH + COUNT, non-blocking)
 * - Value type support: String, Hash, List, Set, ZSet
 * - Type-aware ThemisDB document construction (each key → one document)
 * - Batch pipeline mode (PIPELINE N) to reduce round-trip overhead
 * - TTL preservation: stores `_ttl_ms` field when key has an expiry
 * - Conflict resolution (skip / overwrite / merge) on duplicate keys
 * - Structured error reporting via ImportErrorCode
 * - Observability: progress callback, metrics, trace spans
 * - Permission-check callback for ACL enforcement
 * - Dry-run mode (validates without writing)
 * - Async import via importDataAsync()
 * - Credential redaction: AUTH passwords never appear in logs or errors
 *
 * Build guard: define @c THEMIS_ENABLE_REDIS to compile the full
 * hiredis-backed implementation.  Without it every @c importData() call
 * returns @c ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE with a message
 * describing the missing build flag.
 *
 * @note Thread-safety: RedisImporter instances are not thread-safe.  Create
 *       one instance per import job, or use importDataAsync().
 *
 * Configuration JSON keys (passed to initialize()):
 * @code{.json}
 * {
 *   "host":         "127.0.0.1",  // required
 *   "port":         6379,
 *   "password":     "...",         // never logged
 *   "db":           0,             // Redis database index
 *   "key_pattern":  "*",           // SCAN MATCH pattern
 *   "batch_size":   100,           // keys per SCAN page
 *   "pipeline_size":50,            // commands per pipeline flush
 *   "timeout_ms":   5000,
 *   "tls":          false
 * }
 * @endcode
 *
 * Document layout in ThemisDB (one document per Redis key):
 * @code{.json}
 * {
 *   "_id":      "<key>",
 *   "_type":    "string|hash|list|set|zset",
 *   "value":    <type-specific payload>,
 *   "_ttl_ms":  12345          // omitted when key has no expiry
 * }
 * @endcode
 *
 * References:
 *   Redis SCAN command documentation (https://redis.io/commands/scan/)
 *   Salvatore Sanfilippo, "Redis in Action," Manning Publications, 2013.
 */
class RedisImporter : public IImporter {
public:
    RedisImporter();
    ~RedisImporter() override;

    // -------------------------------------------------------------------------
    // IImporter interface
    // -------------------------------------------------------------------------

    /** @brief Returns "Redis Importer". */
    const char* getName() const override { return "Redis Importer"; }

    /**
     * @brief Returns supported source type identifiers.
     * @return {"redis"}
     */
    std::vector<std::string> getSupportedTypes() const override;

    /**
     * @brief Initializes the importer with a JSON configuration string.
     *
     * Parses host, port, credentials, scan parameters.  Credentials are
     * never stored in plaintext in log output.
     *
     * @param config  JSON configuration (see class-level documentation).
     * @return true on success; false if required fields are missing or invalid.
     */
    bool initialize(const std::string& config) override;

    /**
     * @brief Validates that the configured Redis server is reachable.
     *
     * Sends a PING command.  On failure, appends a human-readable diagnostic
     * to @p errors with the host/port but never the password.
     *
     * @param source_path  Optional override for "host:port" (empty = use config).
     * @param errors       Output: list of validation error messages.
     * @return true if PING succeeds.
     */
    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;

    /**
     * @brief Imports keys matching the configured pattern from Redis.
     *
     * Uses non-blocking SCAN to iterate keys.  For each key the value type is
     * inspected and the full value is fetched via the appropriate command
     * (GET / HGETALL / LRANGE 0 -1 / SMEMBERS / ZRANGE WITHSCORES).
     * Documents are dispatched in batches bounded to ≤ 64 MB.
     *
     * @param source_path        Optional "host:port" override.
     * @param options            Import options (batch size, conflict strategy,
     *                           include/exclude key patterns, deadline_ms).
     * @param progress_callback  Optional callback invoked after each SCAN page.
     * @return Import statistics.
     */
    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr) override;

    /**
     * @brief Starts an asynchronous import job.
     * @param source_path  Optional "host:port" override.
     * @param options      Import options.
     * @return Shared handle to the running import job.
     */
    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options) override;

    /** @brief Requests cancellation of an in-progress import. */
    void cancel() override;

    /**
     * @brief Returns a lightweight schema description for the Redis keyspace.
     *
     * Scans a configurable sample of keys (default: first 100) to build a
     * summary of observed key patterns and value types.  The result is a JSON
     * object compatible with ThemisDB's SchemaInferenceEngine output format.
     *
     * @param source_path  Optional "host:port" override.
     * @return JSON schema summary.
     */
    json getSourceSchema(const std::string& source_path) override;

    // -------------------------------------------------------------------------
    // Testing support
    // -------------------------------------------------------------------------

    /**
     * @brief Injects a mock command function for unit testing.
     *
     * When set, the importer calls this function instead of making real Redis
     * commands.  The function receives a command vector (e.g. {"GET", "mykey"})
     * and returns a JSON-encoded reply.
     *
     * @note Available in all build configurations.
     */
    using MockCommandFn = std::function<std::string(
        const std::vector<std::string>& cmd)>;
    void setMockCommandForTesting(MockCommandFn fn);

private:
    enum class RedisValueType { String, Hash, List, Set, ZSet, Unknown };

    struct Config {
        std::string host{"127.0.0.1"};
        int port{6379};
        // password is never stored after auth; zeroed on connection.
        int db{0};
        std::string key_pattern{"*"};
        int batch_size{100};
        int pipeline_size{50};
        uint32_t timeout_ms{5000};
        bool tls{false};
    };

    /// Maps a Redis TYPE reply string to RedisValueType.
    static RedisValueType parseRedisType(const std::string& type_str);

    /// Sanitises "host:port" for log output (never includes password).
    static std::string sanitiseEndpoint(const std::string& host, int port);

    /// Fetches the full value for a single key and returns a ThemisDB document.
    /// Returns an empty json object on error; sets @p error_out.
    /// @param conn  Active hiredis redisContext* cast to void*; nullptr uses mock.
    json fetchKeyDocument(const std::string& key,
                          RedisValueType vtype,
                          std::string& error_out,
                          void* conn = nullptr);

    Config config_;
    std::atomic<bool> cancelled_{false};
    MockCommandFn mock_command_fn_;

#ifdef THEMIS_ENABLE_REDIS
    /**
     * @brief Sends a hiredis command and returns a JSON-serialised reply string.
     *
     * Handles all hiredis reply types (string, integer, status, array, nil,
     * error).  Array replies are serialised as a JSON array of strings.
     *
     * @param conn  Active redisContext* (cast to void* for header isolation).
     * @param cmd   Command vector, e.g. {"GET", "mykey"}.
     * @return JSON-serialised reply, or empty string on nil/error.
     */
    static std::string executeHiredisCommand(void* conn,
                                              const std::vector<std::string>& cmd);

    /**
     * @brief Opens a hiredis connection using the current config_.
     * @return New redisContext* on success; nullptr on failure.
     *         Caller owns the pointer and must call redisFree() when done.
     */
    redisContext* openRedisConnection() const;
#endif
};

} // namespace importers
} // namespace themis
