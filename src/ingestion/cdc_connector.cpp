/**
 * @file cdc_connector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=11; TODO=1, Stub=3, Unimpl=0, Mock=5, Sim=2, Debt=0, C=2, H=0, M=13, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// When THEMIS_ENABLE_CDC_STREAM is defined the full replication-stream-backed
// implementation is compiled.  Without that flag the connector still compiles
// and:
//   - returns CONNECTOR_NOT_SUPPORTED on any live DB call, OR
//   - uses injected mock functions (unit tests).

#include "ingestion/cdc_connector.h"

#ifdef THEMIS_ENABLE_CDC_STREAM
#include <libpq-fe.h>
#include <thread>
#endif

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdio>

#ifdef ERROR
#undef ERROR
#endif

#ifdef DELETE
#undef DELETE
#endif

namespace themis {
namespace ingestion {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Serialize a CdcEvent::Operation to its string representation.
static const char* operationToString([[maybe_unused]] CdcConnector::CdcEvent::Operation op) {
    switch (op) {
        case CdcConnector::CdcEvent::Operation::INSERT: return "INSERT";
        case CdcConnector::CdcEvent::Operation::UPDATE: return "UPDATE";
        case CdcConnector::CdcEvent::Operation::DELETE: return "DELETE";
    }
    return "UNKNOWN";
}

/// Simple JSON string escaping (no external dependencies).
static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        if (c == '"')       { out += "\\\""; }
        else if (c == '\\') { out += "\\\\"; }
        else if (c == '\n') { out += "\\n"; }
        else if (c == '\r') { out += "\\r"; }
        else if (c == '\t') { out += "\\t"; }
        else                { out += static_cast<char>(c); }
    }
    return out;
}

/// Serialize a string→string map as a JSON object.
static std::string mapToJson(
        const std::unordered_map<std::string, std::string>& m) {
    std::ostringstream js;
    js << '{';
    bool first = true;
    for (const auto& kv : m) {
        if (!first) {
          js << ',';
        }
        first = false;
        js << '"' << jsonEscape(kv.first) << "\":\"" << jsonEscape(kv.second) << '"';
    }
    js << '}';
    return js.str();
}

/// Serialize a full CdcEvent to a JSON string.
static std::string cdcEventToJson([[maybe_unused]] const CdcConnector::CdcEvent& ev) {
    std::ostringstream js;
    js << '{'
       << "\"operation\":\"" << operationToString(ev.operation) << "\","
       << "\"table\":\"" << jsonEscape(ev.table) << "\","
       << "\"key\":\"" << jsonEscape(ev.key) << "\","
       << "\"lsn\":" << ev.lsn << ","
       << "\"timestamp_ms\":" << ev.timestamp_ms << ","
       << "\"before\":" << mapToJson(ev.before) << ","
       << "\"after\":"  << mapToJson(ev.after)
       << '}';
    return js.str();
}

/// Extract document text from a CDC event.
/// Uses the "after" image for INSERT/UPDATE and "before" image for DELETE.
/// When text_columns are specified, concatenates the requested column values
/// from the relevant image; falls back to full JSON serialization otherwise.
static std::string cdcEventToText(const CdcConnector::CdcEvent& ev,
                                   const std::vector<std::string>& text_columns) {
    const auto& image =
        (ev.operation == CdcConnector::CdcEvent::Operation::DELETE)
        ? ev.before : ev.after;

    if (text_columns.empty()) {
        return cdcEventToJson([[maybe_unused]] ev);
    }

    std::string text;
    for (const auto& col : text_columns) {
        auto it = image.find(col);
        if (it != image.end() && !it->second.empty()) {
            if (!text.empty()) {
              text += ' ';
            }
            text += it->second;
        }
    }
    return text.empty() ? cdcEventToJson(ev) : text;
}

/// Split a comma-separated string into trimmed tokens.
static std::vector<std::string> splitCommaCdc(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        auto b = token.find_first_not_of(" \t");
        auto e = token.find_last_not_of(" \t");
        if (b != std::string::npos) {
            result.push_back(token.substr(b, e - b + 1));
        }
    }
    return result;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// PostgreSQL replication-stream helpers (compiled only with THEMIS_ENABLE_CDC_STREAM)
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_CDC_STREAM
namespace {

/// Decode an 8-byte big-endian integer from `buf`.
static uint64_t pgDecodeBE64(const char* buf) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i)
        v = (v << 8) | static_cast<uint8_t>(buf[i]);
    return v;
}

/// Encode a 64-bit integer as 8 big-endian bytes into `buf`.
static void pgEncodeBE64(char* buf, uint64_t v) {
    for (int i = 7; i >= 0; --i) {
        buf[i] = static_cast<char>(v & 0xFFu);
        v >>= 8;
    }
}

/// Parse a PostgreSQL LSN string ("A/BBCCDDEE") to a uint64.  Returns 0 on error.
static uint64_t parsePgLsn(const std::string& s) {
    if (s.empty()) {
      return 0;
    }
    auto slash = s.find('/');
    if (slash == std::string::npos) {
      return 0;
    }
    try {
        uint32_t hi = static_cast<uint32_t>(std::stoul(s.substr(0, slash), nullptr, 16));
        uint32_t lo = static_cast<uint32_t>(std::stoul(s.substr(slash + 1), nullptr, 16));
        return (static_cast<uint64_t>(hi) << 32) | lo;
    } catch (...) { return 0; }
}

/// Format a uint64 LSN to the PostgreSQL "X/YYYYYYYY" representation.
/// PostgreSQL itself outputs LSN values in uppercase hexadecimal (e.g. `0/16E0478`);
/// `%X` matches that canonical form and is accepted by all PostgreSQL versions.
static std::string formatPgLsn(uint64_t lsn) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%X/%X",
                  static_cast<unsigned int>(lsn >> 32),
                  static_cast<unsigned int>(lsn & 0xFFFFFFFFu));
    return std::string(buf);
}

/// Seconds between the Unix epoch (1970-01-01) and the PostgreSQL epoch (2000-01-01).
static constexpr int64_t kPgEpochSeconds = int64_t{946684800};

/// Current time in microseconds since the PostgreSQL epoch (2000-01-01 UTC).
static int64_t pgTimestampNow() {
    using namespace std::chrono;
    const int64_t pg_epoch_us = kPgEpochSeconds * 1000000;
    int64_t now_us = duration_cast<microseconds>(
        system_clock::now().time_since_epoch()).count();
    return now_us - pg_epoch_us;
}

/// Send a Standby Status Update (feedback) to the server.
/// This advances the replication slot's confirmed_flush_lsn and prevents
/// indefinite WAL retention.  Returns true on success.
static bool pgSendFeedback(PGconn* conn,
                            uint64_t write_lsn,
                            uint64_t flush_lsn,
                            uint64_t apply_lsn) {
    char reply[34];
    reply[0] = 'r';                                                    // standby status update
    pgEncodeBE64(reply + 1,  write_lsn);
    pgEncodeBE64(reply + 9,  flush_lsn);
    pgEncodeBE64(reply + 17, apply_lsn);
    pgEncodeBE64(reply + 25, static_cast<uint64_t>(pgTimestampNow())); // sendTime
    reply[sizeof(reply) - 1] = 0;                                      // no reply requested
    return PQputCopyData(conn, reply, static_cast<int>(sizeof(reply))) == 1;
}

/// Build a connection string that includes `replication=database`.
static std::string pgReplicationConnStr(const std::string& base) {
    if (base.find("replication=") != std::string::npos) {
      return base;
    }
    if (base.find("://") != std::string::npos) {
        // URI form: append as query parameter
        std::string s = base;
        s += (s.find('?') != std::string::npos) ? '&' : '?';
        s += "replication=database";
        return s;
    }
    // Key=value form: "host=localhost dbname=mydb ..."
    return base + " replication=database";
}

// ── test_decoding output parser ──────────────────────────────────────────────

/// Parse one `col[type]:value` token from a test_decoding output line.
/// Advances `pos` past the token (including any trailing space).
/// Returns {column_name, value_string}; both empty on parse failure.
static std::pair<std::string, std::string>
parseColToken(const std::string& line, size_t& pos) {
    // col name: up to '['
    auto bracket = line.find('[', pos);
    if (bracket == std::string::npos) return {"", ""};

    std::string col = line.substr(pos, bracket - pos);

    // type name: between '[' and ']' (skip over it)
    auto rb = line.find(']', bracket + 1);
    if (rb == std::string::npos || rb + 1 >= line.size() || line[rb + 1] != ':')
        return {"", ""};

    pos = rb + 2; // skip ']:'

    if (pos >= line.size()) return {col, ""};

    std::string val;
    if (line[pos] == '\'') {
        // Single-quoted string; '' is an escaped single quote
        ++pos;
        while (pos < line.size()) {
            char c = line[pos];
            if (c == '\'') {
                if (pos + 1 < line.size() && line[pos + 1] == '\'') {
                    val += '\'';
                    pos += 2;
                } else {
                    ++pos; // consume closing quote
                    break;
                }
            } else {
                val += c;
                ++pos;
            }
        }
    } else {
        // Unquoted token (integer, null, …): read until space or end
        while (pos < line.size() && line[pos] != ' ')
            val += line[pos++];
    }

    // Skip trailing spaces
    while (pos < line.size() && line[pos] == ' ') {
      ++pos;
    }

    return {col, val};
}

/// Parse all column-value pairs starting at `pos` in `line`.
static std::unordered_map<std::string, std::string>
parseColSet(const std::string& line, size_t& pos) {
    std::unordered_map<std::string, std::string> result = {};

    while (pos < line.size()) {
        auto [c, v] = parseColToken(line, pos);
        if (c.empty()) {
          break;
        }
        result[c] = v;
    }
    return result;
}

/// Parse one test_decoding output line into a CdcEvent.
/// `lsn` is the WAL position from the XLogData header.
/// `server_timestamp_ms` is the server time converted to Unix milliseconds.
/// Returns true if the line represents a row-level change (INSERT/UPDATE/DELETE).
static bool parseTestDecodingLine(const std::string& line,
                                   uint64_t lsn,
                                   int64_t  server_timestamp_ms,
                                   CdcConnector::CdcEvent& ev) {
    // Trim trailing newline / CR
    std::string l = line;
    while (!l.empty() && (l.back() == '\n' || l.back() == '\r'))
        l.pop_back();
    if (l.empty()) {
      return false;
    }

    // Transaction boundaries are not row-level changes
    if (l.rfind("BEGIN ", 0) == 0 || l.rfind("COMMIT ", 0) == 0) {
      return false;
    }

    // Expected prefix: "table "
    if (l.rfind("table ", 0) != 0) {
      return false;
    }

    size_t pos = 6; // skip "table "

    // Extract qualified table name (up to ": ")
    auto colon_pos = l.find(": ", pos);
    if (colon_pos == std::string::npos) {
      return false;
    }

    std::string qualified_table = l.substr(pos, colon_pos - pos);
    pos = colon_pos + 2;

    // Strip schema prefix (schema.table → table)
    auto dot = qualified_table.rfind('.');
    ev.table = (dot != std::string::npos) ? qualified_table.substr(dot + 1) : qualified_table;

    ev.lsn          = lsn;
    ev.timestamp_ms = server_timestamp_ms;

    // Operation keyword
    if (l.compare(pos, 7, "INSERT:") == 0) {
        ev.operation = CdcConnector::CdcEvent::Operation::INSERT;
        pos += 7;
    } else if (l.compare(pos, 7, "UPDATE:") == 0) {
        ev.operation = CdcConnector::CdcEvent::Operation::UPDATE;
        pos += 7;
    } else if (l.compare(pos, 7, "DELETE:") == 0) {
        ev.operation = CdcConnector::CdcEvent::Operation::DELETE;
        pos += 7;
    } else {
        return false;
    }

    // Skip leading space after keyword
    while (pos < l.size() && l[pos] == ' ') {
      ++pos;
    }

    using Op = CdcConnector::CdcEvent::Operation;
    if (ev.operation == Op::UPDATE) {
        // test_decoding may emit "old-key: <cols> new-tuple: <cols>"
        // or just "<cols>" (full-row or no old image configured)
        if (l.compare(pos, 9, "old-key: ") == 0) {
            pos += 9;
            auto new_tuple_pos = l.find(" new-tuple: ", pos);
            if (new_tuple_pos != std::string::npos) {
                ev.before = parseColSet(l, pos);
                pos = new_tuple_pos + 12; // skip " new-tuple: "
                ev.after  = parseColSet(l, pos);
            } else {
                ev.before = parseColSet(l, pos);
            }
        } else {
            ev.after = parseColSet(l, pos);
        }
    } else if (ev.operation == Op::DELETE) {
        ev.before = parseColSet(l, pos);
    } else { // INSERT
        ev.after = parseColSet(l, pos);
    }

    // Derive key: first value from the relevant image
    const auto& key_image = (ev.operation == Op::DELETE) ? ev.before : ev.after;
    if (!key_image.empty()) {
      ev.key = key_image.begin()->second;
    }

    return true;
}

} // anonymous namespace (THEMIS_ENABLE_CDC_STREAM)
#endif // THEMIS_ENABLE_CDC_STREAM

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

/** @brief Pimpl. */
class CdcConnector::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    bool initialize(const SourceConfig& config) {
        if (config.type != SourceType::CDC) {
          return false;
        }
        config_ = config;

        auto opt = [&](const std::string& k, const std::string& def) {
            auto it = config.options.find(k);
            return (it != config.options.end()) ? it->second : def;
        };

        connection_url_    = config.location;
        slot_name_         = opt("slot_name",       "themis_cdc");
        from_lsn_str_      = opt("from_lsn",        "");

        std::string table_filter_str = opt("table_filter", "");
        table_filter_ = table_filter_str.empty()
                        ? std::vector<std::string>{}
                        : splitCommaCdc(table_filter_str);

        std::string ops_str = opt("operations", "INSERT,UPDATE,DELETE");
        ops_filter_ = splitCommaCdc(ops_str);
        // Normalize to uppercase
        for (auto& op : ops_filter_) {
            std::transform(op.begin(), op.end(), op.begin(),
                           [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
        }

        std::string text_cols_str = opt("text_columns", "");
        text_columns_ = text_cols_str.empty()
                        ? std::vector<std::string>{}
                        : splitCommaCdc(text_cols_str);

        try { batch_size_ = static_cast<size_t>(std::stoull(opt("batch_size", "500"))); }
        catch (...) { batch_size_ = 500; }
        if (batch_size_ == 0) {
          batch_size_ = 500;
        }

        try { max_events_ = static_cast<size_t>(std::stoull(opt("max_events", "0"))); }
        catch ([[maybe_unused]] ...) { max_events_ = 0; }

        try { poll_timeout_ms_ = std::stoi(opt("poll_timeout_ms", "1000")); }
        catch (...) { poll_timeout_ms_ = 1000; }

        try { max_empty_polls_ = std::stoi(opt("max_empty_polls", "3")); }
        catch (...) { max_empty_polls_ = 3; }
        if (max_empty_polls_ <= 0) {
          max_empty_polls_ = 3;
        }

        if (connection_url_.empty()) {
          return false;
        }
        return true;
    }

    bool isAvailable() const {
        if (event_fetch_fn_) return true; // test mock always available
#ifdef THEMIS_ENABLE_CDC_STREAM
        if (connection_url_.empty()) {
          return false;
        }
        // Attempt a lightweight IDENTIFY_SYSTEM command to verify connectivity.
        PGconn* conn = PQconnectdb(
            pgReplicationConnStr(connection_url_).c_str());
        if (PQstatus(conn) != CONNECTION_OK) {
            PQfinish(conn);
            return false;
        }
        PGresult* res = PQexec(conn, "IDENTIFY_SYSTEM");
        bool ok = (PQresultStatus(res) == PGRES_TUPLES_OK);
        PQclear(res);
        PQfinish(conn);
        return ok;
#else
        return false;
#endif
    }

    size_t getDocumentCount() const {
        // CDC streams are unbounded; total count is not known in advance.
        return 0;
    }

    IngestionStats ingest(const std::string& /*target_collection*/,
                          ProgressCallback progress_callback) {
        IngestionStats stats;
        auto start_time = std::chrono::steady_clock::now();

        if (connection_url_.empty()) {
            stats.addError(IngestionErrorCode::SOURCE_NOT_CONFIGURED,
                           IngestionErrorSeverity::FATAL,
                           "CdcConnector not configured: no connection URL",
                           config_.source_id);
            finaliseStats(stats, start_time);
            return stats;
        }

        // ------------------------------------------------------------------
        // Test mock path: no replication driver required
        // ------------------------------------------------------------------
        if ([[maybe_unused]] event_fetch_fn_) {
            ingestFromMock(stats, progress_callback);
            finaliseStats(stats, start_time);
            return stats;
        }

        // ------------------------------------------------------------------
        // Production path: CDC stream
        // ------------------------------------------------------------------
#ifdef THEMIS_ENABLE_CDC_STREAM
        ingestFromStream(stats, progress_callback);
#else
        stats.addError(IngestionErrorCode::CONNECTOR_NOT_SUPPORTED,
                       IngestionErrorSeverity::FATAL,
                       "CdcConnector requires THEMIS_ENABLE_CDC_STREAM at build time",
                       config_.source_id);
#endif
        finaliseStats(stats, start_time);
        return stats;
    }

    void setRetryConfig(const RetryConfig& c)           { retry_config_ = c; }
    void setCdcEventFetchForTesting([[maybe_unused]] CdcEventFetchFn fn) { event_fetch_fn_ = std::move(fn); }

private:
    // -----------------------------------------------------------------------
    // Operation filter helper
    // -----------------------------------------------------------------------
    bool isOperationAllowed([[maybe_unused]] CdcEvent::Operation op) const {
        if (ops_filter_.empty()) {
          return true;
        }
        const char* op_str = operationToString(op);
        for (const auto& f : ops_filter_) {
            if (f == op_str) {
              return true;
            }
        }
        return false;
    }

    // Table filter helper
    bool isTableAllowed(const std::string& table) const {
        if (table_filter_.empty()) {
          return true;
        }
        for (const auto& t : table_filter_) {
            if (t == table) {
              return true;
            }
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // Process a single event into stats
    // -----------------------------------------------------------------------
    void processEvent(const CdcEvent& ev, IngestionStats& stats) {
        if (!isOperationAllowed(ev.operation)) {
          return;
        }
        if (!isTableAllowed(ev.table)) {
          return;
        }

        std::string json = cdcEventToJson([[maybe_unused]] ev);
        std::string text = cdcEventToText(ev, text_columns_);

        stats.bytes_processed += json.size();
        if (!text.empty()) {
            ++stats.documents_processed;
        } else {
            ++stats.documents_failed;
            stats.addError(IngestionErrorCode::EXTRACTION_FAILED,
                           IngestionErrorSeverity::WARNING,
                           "Empty text extracted from CDC event on table: " + ev.table,
                           config_.source_id);
        }
    }

    // -----------------------------------------------------------------------
    // STUB/SIMULATION NOTE:
    // Purpose: Enable unit-testing of CdcConnector without a live database by
    //   using an injected event_fetch_fn_ instead of a real CDC stream.
    // Activation: Active when event_fetch_fn_ is non-null (set via
    //   CdcConnector::setEventFetchForTesting()).
    // Production Delta: Events come from the injected lambda instead of a real
    //   database change-data-capture connection.  No network I/O or
    //   authentication occurs.
    // Roadmap ref: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"
    // Removal Plan: Not removed — remains the test-injection path.  Production
    //   path (ingestFromLive) must be used when event_fetch_fn_ is null.
    // Roadmap ref: src/ingestion/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
    // -----------------------------------------------------------------------
    void ingestFromMock(IngestionStats& stats,
                        ProgressCallback& progress_callback) {
        size_t fetched = 0;
        try {
            while (true) {
                if (max_events_ > 0 && fetched >= max_events_) {
                  break;
                }

                auto batch = event_fetch_fn_();
                if (batch.empty()) {
                  break;
                }

                for (const auto& ev : batch) {
                    if (max_events_ > 0 && fetched >= max_events_) {
                      break;
                    }
                    processEvent(ev, stats);
                    ++fetched;
                }

                if ([[maybe_unused]] progress_callback) {
                    progress_callback(config_.source_id,
                                      stats.documents_processed,
                                      0, // total unknown (streaming)
                                      "consumed " + std::to_string([[maybe_unused]] fetched) + " events");
                }
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in CdcConnector mock ingest: " +
                           std::string(e.what()),
                           config_.source_id);
        }
    }

#ifdef THEMIS_ENABLE_CDC_STREAM
    // -----------------------------------------------------------------------
    // Production replication-stream ingestion (PostgreSQL logical replication)
    // -----------------------------------------------------------------------
    void ingestFromStream(IngestionStats& stats,
                          ProgressCallback& progress_callback) {
        // 1. Open replication connection ─────────────────────────────────────
        std::string conn_str = pgReplicationConnStr(connection_url_);
        PGconn* conn = PQconnectdb(conn_str.c_str());
        if (PQstatus(conn) != CONNECTION_OK) {
            stats.addError(IngestionErrorCode::SOURCE_UNAVAILABLE,
                           IngestionErrorSeverity::FATAL,
                           std::string("PQconnectdb failed: ") + PQerrorMessage(conn),
                           config_.source_id);
            PQfinish(conn);
            return;
        }

        // 2. Create replication slot if it does not yet exist ────────────────
        // "IF NOT EXISTS" was added in PostgreSQL 9.6; we fall back to checking
        // for a duplicate-slot error when connecting to older versions.
        std::string create_cmd =
            "CREATE_REPLICATION_SLOT \"" + slot_name_ +
            "\" IF NOT EXISTS LOGICAL test_decoding";
        PGresult* res = PQexec(conn, create_cmd.c_str());
        ExecStatusType create_status = PQresultStatus(res);
        if (create_status != PGRES_TUPLES_OK && create_status != PGRES_COMMAND_OK) {
            // Tolerate "already exists" (SQLSTATE 42710 / duplicate_object)
            const char* sqlstate = PQresultErrorField(res, PG_DIAG_SQLSTATE);
            std::string err = sqlstate ? sqlstate : "";
            if (err != "42710") {
                stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                               IngestionErrorSeverity::FATAL,
                               std::string("CREATE_REPLICATION_SLOT failed: ") +
                               PQresultErrorMessage(res),
                               config_.source_id);
                PQclear(res);
                PQfinish(conn);
                return;
            }
        }
        PQclear(res);

        // 3. START_REPLICATION ───────────────────────────────────────────────
        std::string start_lsn = from_lsn_str_.empty() ? "0/0" : from_lsn_str_;
        std::string start_cmd =
            "START_REPLICATION SLOT \"" + slot_name_ +
            "\" LOGICAL " + start_lsn;
        res = PQexec(conn, start_cmd.c_str());
        if (PQresultStatus(res) != PGRES_COPY_BOTH) {
            stats.addError(IngestionErrorCode::SOURCE_UNAVAILABLE,
                           IngestionErrorSeverity::FATAL,
                           std::string("START_REPLICATION failed: ") +
                           PQresultErrorMessage(res),
                           config_.source_id);
            PQclear(res);
            PQfinish(conn);
            return;
        }
        PQclear(res);

        // 4. Polling loop ────────────────────────────────────────────────────
        // Consecutive timeouts (zero-data polls) before stopping the batch.
        // Configurable via `options["max_empty_polls"]` (default: 3).
        int consecutive_timeouts = 0;
        size_t consumed = 0;

        // Track the highest LSN seen for feedback.
        uint64_t last_lsn = parsePgLsn(start_lsn);

        try {
            while (true) {
                if (max_events_ > 0 && consumed >= max_events_) {
                  break;
                }

                char* buf = nullptr;
                // PQgetCopyData with async=1: returns 0 when no data is yet
                // available, -1 at end of COPY stream, -2 on error.
                int len = PQgetCopyData(conn, &buf, 1 /*async*/);

                if (len == 0) {
                    // No data available; sleep and retry up to max_empty_polls_.
                    ++consecutive_timeouts;
                    if (consecutive_timeouts >= max_empty_polls_) {
                      break;
                    }
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(poll_timeout_ms_));
                    // Send a keepalive feedback to keep the connection alive.
                    pgSendFeedback(conn, last_lsn, last_lsn, last_lsn);
                    continue;
                }

                if (len < 0) {
                    // -1: end of COPY stream (clean finish); -2: error
                    if (len == -2) {
                        stats.addError(IngestionErrorCode::PROCESSING_FAILED,
                                       IngestionErrorSeverity::WARNING,
                                       std::string("PQgetCopyData error: ") +
                                       PQerrorMessage(conn),
                                       config_.source_id);
                    }
                    break;
                }

                // len > 0: we have a WAL message
                consecutive_timeouts = 0;

                if (len < 1) { PQfreemem(buf); continue; }

                const char msg_type = buf[0];

                if (msg_type == 'k') {
                    // Primary keepalive message (17 bytes minimum):
                    //   1-byte type + 8-byte walEnd + 8-byte serverTime + 1-byte replyRequired
                    if (len >= 18) {
                        uint64_t wal_end = pgDecodeBE64(buf + 1);
                        if (wal_end > last_lsn) {
                          last_lsn = wal_end;
                        }
                        bool reply_requested = (buf[17] != 0);
                        if (reply_requested) {
                            pgSendFeedback(conn, last_lsn, last_lsn, last_lsn);
                        }
                    }
                    PQfreemem(buf);
                    continue;
                }

                if (msg_type == 'w') {
                    // XLogData message:
                    //   1-byte type + 8-byte walDataStart + 8-byte walEnd +
                    //   8-byte serverTime + <payload>
                    //   Minimum header: 1 + 8 + 8 + 8 = 25 bytes
                    if (len > 25) {
                        // walDataStart: LSN of the first byte in the payload (used as event LSN)
                        uint64_t wal_start       = pgDecodeBE64(buf + 1);
                        // walEnd: current end of WAL on server (used to track slot progress)
                        uint64_t wal_end         = pgDecodeBE64(buf + 9);
                        int64_t  server_time_us  = static_cast<int64_t>(pgDecodeBE64(buf + 17));

                        if (wal_end > last_lsn) {
                          last_lsn = wal_end;
                        }

                        // Convert PG timestamp (us since 2000-01-01) to Unix ms
                        const int64_t pg_epoch_ms = kPgEpochSeconds * 1000;
                        int64_t server_time_ms = server_time_us / 1000 + pg_epoch_ms;

                        // Payload is one test_decoding output line
                        std::string payload(buf + 25, static_cast<size_t>(len - 25));

                        CdcEvent ev;
                        if (parseTestDecodingLine(payload, wal_start,
                                                   server_time_ms, ev)) {
                            processEvent(ev, stats);
                            ++consumed;

                            if ([[maybe_unused]] progress_callback) {
                                progress_callback(
                                    config_.source_id,
                                    stats.documents_processed,
                                    0, // total unknown for streaming
                                    "consumed " + std::to_string([[maybe_unused]] consumed) + " events");
                            }
                        }
                    }
                    PQfreemem(buf);
                    continue;
                }

                // Unknown message type – discard safely
                PQfreemem(buf);
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in CdcConnector stream ingest: " +
                           std::string(e.what()),
                           config_.source_id);
        }

        // 5. Send final standby status update to advance the replication slot ──
        pgSendFeedback(conn, last_lsn, last_lsn, last_lsn);

        // End the COPY stream
        PQputCopyEnd(conn, nullptr);
        res = PQgetResult(conn);
        if (res) {
          PQclear(res);
        }

        PQfinish(conn);
    }
#endif

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    static void finaliseStats(IngestionStats& stats,
                               const std::chrono::steady_clock::time_point& start) {
        auto end = std::chrono::steady_clock::now();
        stats.elapsed_seconds =
            std::chrono::duration<double>(end - start).count();
    }

    // Fields
    SourceConfig               config_;
    std::string                connection_url_;
    std::string                slot_name_;
    std::string                from_lsn_str_;
    std::vector<std::string>   table_filter_;
    std::vector<std::string>   ops_filter_;
    std::vector<std::string>   text_columns_;
    size_t                     batch_size_               = 500;
    size_t                     max_events_               = 0;
    int                        poll_timeout_ms_          = 1000;
    int                        max_empty_polls_          = 3;   ///< consecutive empty polls before stopping
    RetryConfig                retry_config_;
    CdcEventFetchFn            event_fetch_fn_;
};

// ---------------------------------------------------------------------------
// CdcConnector public API (thin wrappers around Pimpl)
// ---------------------------------------------------------------------------

CdcConnector::CdcConnector()
    : impl_(std::make_unique<Impl>()) {}

CdcConnector::~CdcConnector() = default;

bool CdcConnector::initialize(const SourceConfig& config) {
    return impl_->initialize(config);
}

bool CdcConnector::isAvailable() const {
    return impl_->isAvailable();
}

size_t CdcConnector::getDocumentCount() const {
    return impl_->getDocumentCount();
}

IngestionStats CdcConnector::ingest(const std::string& target_collection,
                                     ProgressCallback progress_callback) {
    return impl_->ingest(target_collection, std::move(progress_callback));
}

void CdcConnector::setRetryConfig(const RetryConfig& config) {
    impl_->setRetryConfig(config);
}

void CdcConnector::setCdcEventFetchForTesting([[maybe_unused]] CdcEventFetchFn fn) {
    setEventBatchProvider([[maybe_unused]] std::move(fn));
}

void CdcConnector::setEventBatchProvider([[maybe_unused]] CdcEventFetchFn fn) {
    impl_->setCdcEventFetchForTesting([[maybe_unused]] std::move(fn));
}

} // namespace ingestion
} // namespace themis


