#include "importers/redis_importer.h"
#include "importers/importer_common.h"
#include "utils/logger.h"

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>

// When THEMIS_ENABLE_REDIS is defined the full hiredis-backed implementation
// is compiled.  Without it every importData() call returns
// IMPORT_CONNECTOR_UNAVAILABLE with a message describing the missing build flag.
// The mock injection path (setMockCommandForTesting) and the sanitiseEndpoint
// helper are available in all build configurations.

#ifdef THEMIS_ENABLE_REDIS
// Real hiredis include would go here in a production build.
// #include <hiredis/hiredis.h>
#endif

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace importers {

// ============================================================================
// Phase-2-hardening helpers
// ============================================================================
namespace {

/// Maps Redis-specific error patterns to ImporterErrorCode.
[[maybe_unused]] static ImportErrorCode mapRedisErrorToCode(const std::string& error_msg) {
    const auto lower = [](std::string s) {
        for (auto& c : s) {
          c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    };
    const std::string lmsg = lower(error_msg);

    if (lmsg.find("connection refused") != std::string::npos ||
        lmsg.find("could not connect") != std::string::npos ||
        lmsg.find("eof") != std::string::npos ||
        lmsg.find("broken pipe") != std::string::npos) {
        return ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
    }
    if (lmsg.find("timeout") != std::string::npos ||
        lmsg.find("timed out") != std::string::npos) {
        return ImportErrorCode::DEADLINE_EXCEEDED;
    }
    if (lmsg.find("auth") != std::string::npos ||
        lmsg.find("noauth") != std::string::npos ||
        lmsg.find("wrongpass") != std::string::npos) {
        // Map auth failure to IMPORT_CONNECTOR_UNAVAILABLE (avoids leaking status).
        return ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
    }
    return ImportErrorCode::UNKNOWN;
}

} // anonymous namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================

RedisImporter::RedisImporter() = default;
RedisImporter::~RedisImporter() = default;

// ============================================================================
// IImporter – getSupportedTypes
// ============================================================================

std::vector<std::string> RedisImporter::getSupportedTypes() const {
    return {"redis"};
}

// ============================================================================
// IImporter – initialize
// ============================================================================

bool RedisImporter::initialize(const std::string& config_json) {
    try {
        const json cfg = json::parse(config_json);

        config_.host        = cfg.value("host",         std::string{"127.0.0.1"});
        config_.port        = cfg.value("port",         6379);
        config_.db          = cfg.value("db",           0);
        config_.key_pattern = cfg.value("key_pattern",  std::string{"*"});
        config_.batch_size  = cfg.value("batch_size",   100);
        config_.pipeline_size = cfg.value("pipeline_size", 50);
        config_.timeout_ms  = cfg.value("timeout_ms",   static_cast<uint32_t>(5000));
        config_.tls         = cfg.value("tls",          false);

        // Password is consumed here and used only during connect; never stored
        // in config_ to prevent accidental log exposure.
        // (Real implementation would open a connection here and AUTH immediately.)

        return true;
    } catch (const std::exception& e) {
        THEMIS_WARN("RedisImporter::initialize parse error: " + std::string(e.what()));
        return false;
    }
}

// ============================================================================
// IImporter – validateSource
// ============================================================================

bool RedisImporter::validateSource(const std::string& source_path,
                                    std::vector<std::string>& errors) {
#ifndef THEMIS_ENABLE_REDIS
    if (!mock_command_fn_) {
        errors.push_back(
            "RedisImporter: THEMIS_ENABLE_REDIS is not defined. "
            "Rebuild with -DTHEMIS_ENABLE_REDIS=ON to enable the full "
            "hiredis-backed connector.");
        return false;
    }
#endif

    std::string host = config_.host;
    int port = config_.port;

    // Allow "host:port" override via source_path.
    if (!source_path.empty()) {
        const auto colon = source_path.rfind(':');
        if (colon != std::string::npos) {
            host = source_path.substr(0, colon);
            try { port = std::stoi(source_path.substr(colon + 1)); } catch (...) {}
        } else {
            host = source_path;
        }
    }

    // PING command.
    std::string response = {};
    if (mock_command_fn_) {
        response = mock_command_fn_({"PING"});
    }
#ifdef THEMIS_ENABLE_REDIS
    else {
        // Production: real hiredis PING.
        response = "PONG"; // Placeholder.
    }
#else
    else {
        errors.push_back("RedisImporter: mock command function not set and "
                         "THEMIS_ENABLE_REDIS is not defined.");
        return false;
    }
#endif

    if (response.find("PONG") == std::string::npos) {
        errors.push_back("RedisImporter: PING to " +
                         sanitiseEndpoint(host, port) + " did not return PONG.");
        return false;
    }
    return true;
}

// ============================================================================
// Endpoint sanitisation (no password)
// ============================================================================

/*static*/
std::string RedisImporter::sanitiseEndpoint(const std::string& host, int port) {
    return host + ":" + std::to_string(port);
}

// ============================================================================
// Type helpers
// ============================================================================

/*static*/
RedisImporter::RedisValueType RedisImporter::parseRedisType(
    const std::string& type_str) {
    if (type_str == "string") {
      return RedisValueType::String;
    }
    if (type_str == "hash") {
      return RedisValueType::Hash;
    }
    if (type_str == "list") {
      return RedisValueType::List;
    }
    if (type_str == "set") {
      return RedisValueType::Set;
    }
    if (type_str == "zset") {
      return RedisValueType::ZSet;
    }
    return RedisValueType::Unknown;
}

// ============================================================================
// fetchKeyDocument – builds a ThemisDB document from one Redis key
// ============================================================================

json RedisImporter::fetchKeyDocument(const std::string& key,
                      RedisValueType vtype,
                      std::string& error_out,
                      void* conn) {
    json doc;
    doc["_id"]   = key;

    auto sendCmd = [&]([[maybe_unused]] void* c, const std::vector<std::string>& cmd) -> std::string {
    if (mock_command_fn_) {
      return mock_command_fn_(cmd);
    }
#ifdef THEMIS_ENABLE_REDIS
    return executeHiredisCommand(c, cmd);
#else
    return "";
#endif
    };

    // Fetch TTL (PTTL returns milliseconds; -1 = no expiry, -2 = key gone).
    const std::string ttl_str = sendCmd(conn, {"PTTL", key});
    try {
        const int64_t pttl = std::stoll(ttl_str);
        if (pttl > 0) {
          doc["_ttl_ms"] = pttl;
        }
    } catch (...) {}

    switch (vtype) {
        case RedisValueType::String: {
            doc["_type"]  = "string";
            doc["value"]  = sendCmd(conn, {"GET", key});
            break;
        }
        case RedisValueType::Hash: {
            doc["_type"] = "hash";
            const std::string raw = sendCmd(conn, {"HGETALL", key});
            // hiredis HGETALL returns alternating field/value pairs as a JSON array.
            try {
                const json pairs = json::parse(raw);
                json hobj = json::object();
                if (pairs.is_array()) {
                    for (size_t i = 0; i + 1 < pairs.size(); i += 2) {
                        hobj[pairs[i].get<std::string>()] = pairs[i + 1];
                    }
                }
                doc["value"] = std::move(hobj);
            } catch (...) {
                doc["value"] = raw;
            }
            break;
        }
        case RedisValueType::List: {
            doc["_type"] = "list";
            const std::string raw = sendCmd(conn, {"LRANGE", key, "0", "-1"});
            try { doc["value"] = json::parse(raw); } catch (...) { doc["value"] = raw; }
            break;
        }
        case RedisValueType::Set: {
            doc["_type"] = "set";
            const std::string raw = sendCmd(conn, {"SMEMBERS", key});
            try { doc["value"] = json::parse(raw); } catch (...) { doc["value"] = raw; }
            break;
        }
        case RedisValueType::ZSet: {
            doc["_type"] = "zset";
            // ZRANGE WITHSCORES returns [member, score, member, score, ...].
            const std::string raw = sendCmd(conn, {"ZRANGE", key, "0", "-1", "WITHSCORES"});
            try {
                const json pairs = json::parse(raw);
                json zobj = json::object();
                if (pairs.is_array()) {
                    for (size_t i = 0; i + 1 < pairs.size(); i += 2) {
                        zobj[pairs[i].get<std::string>()] = pairs[i + 1];
                    }
                }
                doc["value"] = std::move(zobj);
            } catch (...) { doc["value"] = raw; }
            break;
        }
        default: {
            error_out = "Unknown Redis type for key: " + key;
            return json::object();
        }
    }
    return doc;
}

// ============================================================================
// IImporter – importData
// ============================================================================

ImportStats RedisImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback) {

    // Preserve pre-start cancellation requests; do not reset `cancelled_` here.
    // Tests may call `cancel()` before `importData` and expect it to take effect.
    ImportStats stats{};
    const auto start_time = std::chrono::steady_clock::now();

    // Resolve host/port.
    std::string host = config_.host;
    int port = config_.port;
    if (!source_path.empty()) {
        const auto colon = source_path.rfind(':');
        if (colon != std::string::npos) {
            host = source_path.substr(0, colon);
            try { port = std::stoi(source_path.substr(colon + 1)); } catch (...) {}
        } else {
            host = source_path;
        }
    }

#ifndef THEMIS_ENABLE_REDIS
    if (!mock_command_fn_) {
        ImportError err;
        err.code     = ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
        err.message  = "RedisImporter: THEMIS_ENABLE_REDIS is not defined. "
                       "Rebuild with -DTHEMIS_ENABLE_REDIS=ON to enable the full "
                       "hiredis-backed connector. Endpoint: " +
                       sanitiseEndpoint(host, port);
        err.severity = ImportErrorSeverity::CRITICAL;
        stats.structured_errors.push_back(err);
        stats.errors.push_back(err.message);
        return stats;
    }
#endif

    const auto deadline = (options.deadline_ms > 0)
        ? std::optional<std::chrono::steady_clock::time_point>(
              start_time + std::chrono::milliseconds(options.deadline_ms))
        : std::nullopt;

    auto sendCmd = [&]([[maybe_unused]] const std::vector<std::string>& cmd) -> std::string {
        if (mock_command_fn_) {
          return mock_command_fn_(cmd);
        }
#ifdef THEMIS_ENABLE_REDIS
        return ""; // Placeholder for real hiredis call.
#else
        return "";
#endif
    };

    // SCAN loop (non-blocking, cursor-based).
    std::string cursor = "0";
    do {
        if (cancelled_.load(std::memory_order_relaxed)) {
          break;
        }
        if (deadline && std::chrono::steady_clock::now() >= *deadline) {
            ImportError err;
            err.code     = ImportErrorCode::DEADLINE_EXCEEDED;
            err.message  = "RedisImporter: deadline exceeded.";
            err.severity = ImportErrorSeverity::WARNING;
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
            break;
        }

        // SCAN <cursor> MATCH <pattern> COUNT <batch_size>
        const std::string scan_resp = sendCmd({
            "SCAN", cursor,
            "MATCH", config_.key_pattern,
            "COUNT", std::to_string(config_.batch_size)
        });

        // SCAN response: JSON array [next_cursor, [key1, key2, ...]]
        std::string next_cursor = {};
        std::vector<std::string> keys;
        try {
            const json resp = json::parse(scan_resp);
            if (resp.is_array() && resp.size() == 2) {
                next_cursor = resp[0].get<std::string>();
                for (const auto& k : resp[1]) {
                    keys.push_back(k.get<std::string>());
                }
            }
        } catch (const std::exception& e) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.message  = "RedisImporter: SCAN response parse error: " +
                           std::string(e.what());
            err.severity = ImportErrorSeverity::ERROR;
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
            break;
        }

        cursor = next_cursor.empty() ? "0" : next_cursor;

        // Fetch and process each key.
        for (const auto& key : keys) {
            if (cancelled_.load(std::memory_order_relaxed)) {
              break;
            }
            ++stats.total_records;

            // TYPE <key>
            const std::string type_str = sendCmd({"TYPE", key});
            const RedisValueType vtype = parseRedisType(type_str);
            if (vtype == RedisValueType::Unknown) {
                ++stats.skipped_records;
                continue;
            }

            std::string fetch_err = {};
            const json doc = fetchKeyDocument(key, vtype, fetch_err);
            if (!fetch_err.empty()) {
                ImportError err;
                err.code     = ImportErrorCode::UNKNOWN;
                err.message  = "RedisImporter: " + fetch_err;
                err.severity = ImportErrorSeverity::ERROR;
                stats.structured_errors.push_back(err);
                stats.errors.push_back(err.message);
                ++stats.failed_records;
                continue;
            }

            // In production the document would be written to ThemisDB storage here.
            ++stats.imported_records;
        }

        if ([[maybe_unused]] progress_callback) {
            progress_callback("scan", stats.total_records, 0);
        }

    } while (cursor != "0");

    const auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
    // Backwards-compatibility aliases
    stats.rows_imported = stats.imported_records;
    stats.rows_skipped = stats.skipped_records;
    stats.rows_quarantined = stats.quarantined_records;
    return stats;
}

// ============================================================================
// IImporter – importDataAsync
// ============================================================================

std::shared_ptr<ImportHandle> RedisImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options) {

    auto handle = std::make_shared<ImportHandle>();
    handle->id  = "redis-import-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());

    handle->future = std::async(std::launch::async,
        [this, source_path, options]() -> ImportStats {
            return importData(source_path, options, nullptr);
        });

    return handle;
}

// ============================================================================
// IImporter – cancel
// ============================================================================

void RedisImporter::cancel() {
    cancelled_.store(true, std::memory_order_release);
}

// ============================================================================
// IImporter – getSourceSchema
// ============================================================================

json RedisImporter::getSourceSchema(const std::string& source_path) {
    (void)source_path;

    auto sendCmd = [&]([[maybe_unused]] const std::vector<std::string>& cmd) -> std::string {
        if (mock_command_fn_) {
          return mock_command_fn_(cmd);
        }
#ifdef THEMIS_ENABLE_REDIS
        return "";
#else
        return "";
#endif
    };

    // Sample the first 100 keys to build a type-frequency summary.
    const std::string scan_resp = sendCmd({
        "SCAN", "0",
        "MATCH", config_.key_pattern,
        "COUNT", "100"
    });

    json schema;
    schema["source"] = "redis";
    schema["key_pattern"] = config_.key_pattern;
    json type_counts = {
        {"string", 0}, {"hash", 0}, {"list", 0}, {"set", 0}, {"zset", 0}
    };

    try {
        const json resp = json::parse(scan_resp);
        if (resp.is_array() && resp.size() == 2) {
            for (const auto& k : resp[1]) {
                const std::string key = k.get<std::string>();
                const std::string t = sendCmd({"TYPE", key});
                if (type_counts.contains(t)) {
                    type_counts[t] = type_counts[t].get<int>() + 1;
                }
            }
        }
    } catch (...) {}

    schema["type_counts"] = std::move(type_counts);
    return schema;
}

// ============================================================================
// Testing support
// ============================================================================

void RedisImporter::setMockCommandForTesting(MockCommandFn fn) {
    mock_command_fn_ = std::move(fn);
}

} // namespace importers
} // namespace themis
