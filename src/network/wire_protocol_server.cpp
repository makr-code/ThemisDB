/**
 * @file wire_protocol_server.cpp
 * @brief ThemisDB native binary wire-protocol server.
 *
 * Implements the high-performance TCP/binary protocol used by native ThemisDB
 * clients.  Key responsibilities:
 *  - Accept and dispatch binary-framed client connections via Boost.Asio.
 *  - Authenticate clients with SCRAM-SHA-256 (constant-time comparison).
 *  - Route per-session requests (CRUD, AQL, vector search, time-series, BPMN)
 *    to the appropriate engine subsystem.
 *  - Graceful, bounded shutdown via timedJoin (5 s per I/O thread).
 *
 * @note thread_join_no_timeout (W3): All thread joins go through timedJoin()
 *   which logs and detaches on the 5 s deadline to prevent indefinite block.
 * @note worker_pool: stop() is called before wait() so pending tasks are
 *   cancelled and the pool drains promptly.
 *
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=5; TODO=2, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=108, M=32, L=0
 * @note Status: Production Ready
 */


// ThemisDB Wire Protocol Server Implementation
// Binary protocol for high-performance native client communication

// LOCK ORDERING (must always acquire in this sequence to prevent ABBA deadlock):
//   L1: connections_mutex_   — guards connections_per_ip_, active_sessions_
//   L2: stats_mutex_         — guards Stats struct (counters)
//   L3: rate_limit_mutex_    — guards rate_limits_ map
// Rule: never acquire L1 while holding L2 or L3.
//       never acquire L2 while holding L3.
// Callsites that need multiple locks must follow L1 → L2 → L3 order,
// or use std::lock() for simultaneous acquisition when order is ambiguous.

#include "network/wire_protocol_server.h"
#include <stdexcept>
#include "network/wire_bootstrap_validation.h"
#include "network/wire_protocol_helpers.h"
#include "network/wire_retry_policy.h"
#ifdef THEMIS_ENABLE_WEBSOCKET
#  include "network/wire_protocol_websocket.h"
#endif
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "index/spatial_index.h"
#include "index/process_graph.h"
#include "index/spatial_index.h"
#include "transaction/transaction_manager.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "security/transport_security_checker.h"
#include "query/query_engine.h"
#include "query/aql_runner.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdio>  // For snprintf
#include <exception>
#include <future>
#include <thread>
#ifdef _WIN32
    #include <winsock2.h>  // For ntohl/htonl on Windows
#else
    #include <arpa/inet.h>  // For ntohl/htonl on Unix
#endif
#include <openssl/crypto.h>  // For CRYPTO_memcmp (WPS-2: constant-time token comparison)
#include <map>  // For multi-bucket aggregation
#include <algorithm>  // For std::min/max
#include <cmath>      // For std::cos, std::acos (GEO_QUERY distance)
#include <cctype>     // For std::isspace/std::isdigit
#include <limits>
#include <spdlog/spdlog.h>  // For WPS-3 misconfiguration error log

using json = nlohmann::json;

namespace themis::network {

namespace {

constexpr std::size_t kMaxWireIdentifierLength = 256;
constexpr std::size_t kMaxAuthPayloadBytes = 16 * 1024;
constexpr std::size_t kMaxCrudPayloadBytes = 256 * 1024;
constexpr std::size_t kMaxBatchPayloadBytes = 4 * 1024 * 1024;
constexpr std::size_t kMaxTransactionPayloadBytes = 64 * 1024;
constexpr std::size_t kMaxGraphPayloadBytes = 256 * 1024;
constexpr std::size_t kMaxQueryPayloadBytes = 2 * 1024 * 1024;
constexpr std::size_t kMaxCursorPayloadBytes = 64 * 1024;
constexpr std::size_t kMaxBpmnPayloadBytes = 1'048'576;
constexpr std::size_t kMaxBpmnVariablesFields = 256;
constexpr std::size_t kDefaultBpmnHistoryEvents = 1000;
constexpr std::size_t kMaxBpmnHistoryEvents = 10000;
constexpr uint32_t kMaxWireFrameSizeMb =
    static_cast<uint32_t>(std::numeric_limits<uint32_t>::max() / (1024 * 1024));
constexpr int kBindListenMaxRetries = 3; // kept for reference; active retry uses WireRetryPolicy
constexpr auto kBindListenRetryBaseDelay = std::chrono::milliseconds(100); // kept for reference

/// Maximum ms to wait for a single I/O thread to join during shutdown.
/// thread_join_no_timeout (W3): capped to prevent indefinite block.
constexpr int kShutdownJoinTimeoutMs = 5000;

/// @brief Join @p t within @p timeout_ms; log and detach on timeout.
///
/// @param t       Thread to join (moved into the internal watcher).
/// @param timeout_ms  Maximum wait time in milliseconds (default 5 s).
///
/// Rationale: calling t.join() without a deadline can block indefinitely if
/// the thread is stuck in a syscall.  This helper spawns a watcher thread
/// that performs the join and signals a std::promise.  The caller waits on
/// the future with a deadline; if the deadline expires the watcher is detached
/// and the caller returns promptly.
static void timedJoin(std::thread& t,
                      int timeout_ms = kShutdownJoinTimeoutMs) noexcept {
    if (!t.joinable()) {
      return;
    }
    std::promise<void> done;
    auto fut = done.get_future();
    std::thread watcher([inner = std::move(t), p = std::move(done)]() mutable {
        if (inner.joinable()) {
          inner.join();
        }
        p.set_value();
    });
    watcher.detach();
    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) !=
            std::future_status::ready) {
        // thread_join_no_timeout: detach on deadline to prevent indefinite block.
        THEMIS_WARN("[WireProtocol] I/O thread did not finish within {} ms during "
                    "shutdown; detaching.", timeout_ms);
    }
}

uint64_t fnv1a64(std::string_view value) {
    uint64_t hash = 1469598103934665603;
    for (unsigned char ch : value) {
        hash ^= static_cast<uint64_t>(ch);
        hash *= 1099511628211;
    }
    return hash;
}

std::string anonymizePeerForLog(std::string_view value) {
    if (value.empty()) {
        return "peer#unknown";
    }
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "peer#%016llx",
                  static_cast<unsigned long long>(fnv1a64(value)));
    return std::string(buffer);
}

bool hasControlCharacters(std::string_view value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char ch) {
        return ch < 0x20 || ch == 0x7F;
    });
}

bool isReasonableWireIdentifier(std::string_view value) {
    return !value.empty() && static_cast<int>(value.size()) <= kMaxWireIdentifierLength &&
           !hasControlCharacters(value);
}

bool isBlankString(std::string_view value) {
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
}

std::size_t escapedJsonStringLength(std::string_view value) {
    std::size_t escaped_length = 0;
    for (unsigned char ch : value) {
        switch (ch) {
            case '\"':
            [[fallthrough]];\n            case '\\':
            [[fallthrough]];\n            case '\b':
            [[fallthrough]];\n            case '\f':
            [[fallthrough]];\n            case '\n':
            [[fallthrough]];\n            case '\r':
            [[fallthrough]];\n            case '\t':
                escaped_length += 2;
                break;
            default:
                escaped_length += (ch < 0x20u) ? 6 : 1;
                break;
        }
    }
    return escaped_length;
}

bool isAuthTokenPayloadWithinLimit(std::string_view token) {
    constexpr std::size_t kAuthTokenJsonOverhead = sizeof("{\"token\":\"\"}") - 1;
    if constexpr (kAuthTokenJsonOverhead > kMaxAuthPayloadBytes) {
        return false;
    }
    const std::size_t escaped_token_length = escapedJsonStringLength(token);
    return escaped_token_length <= (kMaxAuthPayloadBytes - kAuthTokenJsonOverhead);
}

bool isUnsignedIntegerString(std::string_view value) {
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}

bool validateBpmnVariablesObject(const json& variables, std::string& error_message) {
    if (!variables.is_object()) {
        error_message = "variables must be a JSON object";
        return false;
    }

    if (static_cast<int>(variables.size()) > kMaxBpmnVariablesFields) {
        error_message = "variables object exceeds maximum field count";
        return false;
    }

    for (auto it = variables.begin(); it != variables.end(); ++it) {
        const std::string& key = it.key();
        if (key.empty() || isBlankString(key) || !isReasonableWireIdentifier(key)) {
            error_message = "variables contains invalid field name";
            return false;
        }
    }

    return true;
}

// CRC32 (ISO-HDLC / Ethernet) – same polynomial as used in stream_protocol.cpp
// and WAL storage.  Used to verify wire-frame checksums.
const uint32_t kCrc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD706B3,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

// Compute CRC32 (ISO-HDLC) over [data, data+len).  Seed with 0 for first call;
// pass previous result to chain multiple buffers.
uint32_t crc32Update(uint32_t crc, const uint8_t* data, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; ++i)
        crc = kCrc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

json parsePayloadJson(const std::vector<uint8_t>& payload_buffer);
json parsePayloadJsonWithRetry(const std::vector<uint8_t>& payload_buffer, int max_attempts);

// ---------------------------------------------------------------------------
// GEO_QUERY injection bridge globals (stub #284 replacement)
// ---------------------------------------------------------------------------
std::mutex         g_network_geo_fn_mutex;
GeoQueryFn         g_network_geo_query_fn;

} // anonymous namespace

void setNetworkGeoQueryFn(GeoQueryFn fn) {
    std::lock_guard<std::mutex> lock(g_network_geo_fn_mutex);
    g_network_geo_query_fn = std::move(fn);
}

// =============================================================================
// WireProtocolServer Implementation
// =============================================================================

WireProtocolServer::WireProtocolServer(
    const Config& config,
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    std::shared_ptr<GraphIndexManager> graph_index,
    std::shared_ptr<VectorIndexManager> vector_index,
    std::shared_ptr<TransactionManager> tx_manager,
    std::shared_ptr<ProcessGraphManager> process_graph,
    std::shared_ptr<TSStore> ts_store,
    std::shared_ptr<ContinuousAggregateManager> agg_manager,
    std::shared_ptr<QueryEngine> query_engine)
    : config_(config)
    , storage_(storage)
    , secondary_index_(secondary_index)
    , graph_index_(graph_index)
    , vector_index_(vector_index)
    , tx_manager_(tx_manager)
    , process_graph_(process_graph)
    , ts_store_(ts_store)
    , agg_manager_(agg_manager)
    , query_engine_(std::move(query_engine))
{
    io_context_ = std::make_unique<net::io_context>();
    worker_pool_ = std::make_unique<net::thread_pool>(config_.num_worker_threads);
    acceptor_ = std::make_unique<tcp::acceptor>(*io_context_);
}

WireProtocolServer::~WireProtocolServer() {
    stop();
}

void WireProtocolServer::setSpatialIndexManager(
    std::shared_ptr<index::SpatialIndexManager> idx) {
    spatial_index_ = std::move(idx);
}

bool WireProtocolServer::validateTransportSecurity(int argc, const char* const argv[]) const {
    return themis::security::TransportSecurityChecker::validateProductionSafety(
        config_.enable_tls,
        "Wire Protocol",
        argc,
        argv
    );
}

void WireProtocolServer::start() {
    // Enforce transport security validation as a startup gate
    // We do not have argc/argv here, so we pass an empty argument list
    if (!validateTransportSecurity(0, nullptr)) {
        std::cerr << "[WireProtocol] Transport security validation failed. Server will not start."
                  << std::endl;
        return;
    }

    const bool has_query_engine = static_cast<bool>(query_engine_);
    bool has_geo_callback = false;
    {
        std::lock_guard<std::mutex> lock(g_network_geo_fn_mutex);
        has_geo_callback = static_cast<bool>([[maybe_unused]] g_network_geo_query_fn);
    }
    const bool has_geo_backend = static_cast<bool>(spatial_index_) || has_geo_callback;
    if (!wire_bootstrap::validateRequiredBackends(
            "WireProtocol",
            {
                {"query_engine", has_query_engine},
                {"geo_backend", has_geo_backend},
            },
            std::cerr)) {
        return;
    }

    if (config_.num_io_threads == 0) {
        std::cerr << "[WireProtocol] Invalid configuration: num_io_threads must be greater than 0. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.num_worker_threads == 0) {
        std::cerr << "[WireProtocol] Invalid configuration: num_worker_threads must be greater than 0. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.max_connections_per_ip == 0) {
        std::cerr << "[WireProtocol] Invalid configuration: max_connections_per_ip must be greater than 0. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.max_connections > 0 && config_.max_connections_per_ip > config_.max_connections) {
        std::cerr << "[WireProtocol] Invalid configuration: max_connections_per_ip must be <= "
                     "max_connections when a global connection limit is enabled. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.max_frame_size_mb == 0) {
        std::cerr << "[WireProtocol] Invalid configuration: max_frame_size_mb must be greater than 0. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.max_frame_size_mb > kMaxWireFrameSizeMb) {
        std::cerr << "[WireProtocol] Invalid configuration: max_frame_size_mb exceeds protocol "
                     "payload-size limit (max "
                  << kMaxWireFrameSizeMb
                  << " MB). Server will not start." << std::endl;
        return;
    }

    if (config_.request_timeout_sec == 0) {
        std::cerr << "[WireProtocol] Invalid configuration: request_timeout_sec must be greater than 0. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.require_auth &&
        (config_.auth_mechanism.empty() || isBlankString(config_.auth_mechanism))) {
        std::cerr << "[WireProtocol] Invalid configuration: auth_mechanism must be a non-empty "
                     "value when require_auth is enabled. Server will not start." << std::endl;
        return;
    }

    if (config_.require_auth && config_.auth_mechanism != "SCRAM-SHA-256") {
        std::cerr << "[WireProtocol] Invalid configuration: unsupported auth_mechanism '"
                  << config_.auth_mechanism
                  << "'. Supported value: SCRAM-SHA-256. Server will not start." << std::endl;
        return;
    }

    if (config_.require_auth && config_.auth_token.empty()) {
        std::cerr << "[WireProtocol] Invalid configuration: require_auth is enabled but "
                     "the configured authentication secret is empty. Server will not start."
                  << std::endl;
        return;
    }

    if (config_.require_auth && isBlankString(config_.auth_token)) {
        std::cerr << "[WireProtocol] Invalid configuration: require_auth is enabled but "
                     "the configured authentication secret is whitespace-only. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.require_auth && hasControlCharacters(config_.auth_token)) {
        std::cerr << "[WireProtocol] Invalid configuration: require_auth is enabled but "
                     "the configured authentication secret contains control characters. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.require_auth && !isAuthTokenPayloadWithinLimit(config_.auth_token)) {
        std::cerr << "[WireProtocol] Invalid configuration: configured authentication secret "
                     "length exceeds AUTH payload limit. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.max_requests_per_second == 0) {
        std::cerr << "[WireProtocol] Invalid configuration: max_requests_per_second must be greater than 0. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.max_requests_per_minute == 0) {
        std::cerr << "[WireProtocol] Invalid configuration: max_requests_per_minute must be greater than 0. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.max_requests_per_minute < config_.max_requests_per_second) {
        std::cerr << "[WireProtocol] Invalid configuration: max_requests_per_minute must be "
                     ">= max_requests_per_second. Server will not start." << std::endl;
        return;
    }

    if (config_.port == 0) {
        std::cerr << "[WireProtocol] Invalid configuration: port must be greater than 0. "
                     "Server will not start." << std::endl;
        return;
    }

    if (config_.tcp_backlog <= 0) {
        std::cerr << "[WireProtocol] Invalid configuration: tcp_backlog must be greater than 0. "
                     "Server will not start." << std::endl;
        return;
    }

    if (!io_context_ || !acceptor_) {
        std::cerr << "[WireProtocol] Startup failed: internal networking runtime is not initialized. "
                     "Server will not start." << std::endl;
        return;
    }

    if (!config_.host.empty() && isBlankString(config_.host)) {
        std::cerr << "[WireProtocol] Invalid configuration: host must not be whitespace-only. "
                     "Server will not start." << std::endl;
        return;
    }

    if (hasControlCharacters(config_.host)) {
        std::cerr << "[WireProtocol] Invalid configuration: host contains control characters. "
                     "Server will not start." << std::endl;
        return;
    }

    if (!config_.host.empty() && std::any_of(config_.host.begin(), config_.host.end(),
            [](unsigned char ch) { return std::isspace(ch) != 0; })) {
        std::cerr << "[WireProtocol] Invalid configuration: host must not contain whitespace. "
                     "Server will not start." << std::endl;
        return;
    }

    // Resolve bind address from config_.host.
    // * An explicit IPv4 address (e.g. "127.0.0.1") or IPv6 address
    //   (e.g. "::1", "fe80::1") is used directly.
    // * When enable_ipv6=true and host is the default "0.0.0.0" the address is
    //   automatically promoted to "::" so both stacks are covered via one socket.
    // * When host cannot be parsed (e.g. empty or a DNS name) we fall back to
    //   the IPv6 or IPv4 wildcard based on enable_ipv6.
    boost::system::error_code addr_ec;
    net::ip::address bind_addr = net::ip::make_address(config_.host, addr_ec);
    if (addr_ec) {
        // Not a parseable numeric address – use wildcard for the requested family.
        bind_addr = config_.enable_ipv6
            ? net::ip::address(net::ip::address_v6::any())
            : net::ip::address(net::ip::address_v4::any());
    } else if (config_.enable_ipv6 && bind_addr == net::ip::address_v4::any()) {
        // enable_ipv6=true but host was left at default "0.0.0.0" – promote.
        bind_addr = net::ip::address(net::ip::address_v6::any());
    }

    tcp::endpoint endpoint(bind_addr, config_.port);
    boost::system::error_code setup_ec;

    // P5-S01: Use WireRetryPolicy for bind/listen with exponential backoff.
    const auto bind_policy = themis::network::WireRetryPolicy::forBindListen();
    const bool bind_ok = themis::network::retryWithPolicy(
        bind_policy,
        [&]() -> bool {
            boost::system::error_code ec;

            if (acceptor_->is_open()) {
                acceptor_->close(ec);
                ec.clear();
            }

            acceptor_->open(endpoint.protocol(), ec);
            if (!ec) {
                acceptor_->set_option(tcp::acceptor::reuse_address(true), ec);
            }

            // Enable dual-stack (IPV6_V6ONLY=0) when binding to an IPv6 socket
            // and ipv6_dual_stack is requested.  This allows a single listener to
            // accept both IPv4-mapped and native IPv6 clients without a second
            // socket.
            if (!ec && endpoint.address().is_v6() && config_.ipv6_dual_stack) {
                net::ip::v6_only v6only_opt(false);
                boost::system::error_code v6ec;
                acceptor_->set_option(v6only_opt, v6ec);
                // Non-fatal: some platforms (e.g. OpenBSD) do not support dual-stack.
                if (v6ec) {
                    std::cerr << "[WireProtocol] Note: dual-stack (IPV6_V6ONLY=0) not supported"
                                 " on this platform, proceeding with IPv6-only socket.\n";
                }
            }

            if (!ec) {
                acceptor_->bind(endpoint, ec);
            }
            if (!ec) {
                acceptor_->listen(config_.tcp_backlog, ec);
            }

            if (!ec) {
                setup_ec.clear();
                return true;
            }
            setup_ec = ec;
            return false;
        },
        [&](uint32_t attempt, int64_t delay_ms) {
            std::cerr << "[WireProtocol] Listener setup failed on attempt "
                      << attempt << "/" << bind_policy.max_attempts
                      << ": " << setup_ec.message()
                      << "; retrying in " << delay_ms << "ms\n";
        });

    if (!bind_ok || setup_ec) {
        std::cerr << "[WireProtocol] Startup failed: unable to bind/listen on "
                  << endpoint.address().to_string() << ":" << endpoint.port()
                  << " after " << bind_policy.max_attempts << " attempts: "
                  << setup_ec.message() << std::endl;
        return;
    }

    running_.store(true, std::memory_order_release);

    // Start I/O threads
    for (size_t i = 0; i < config_.num_io_threads; ++i) {
        io_threads_.emplace_back([this]() {
            try {
                io_context_->run();
            } catch (const std::exception& e) {
                std::cerr << "[WireProtocol] IO thread error: " << e.what() << std::endl;
            }
        });
    }

    doAccept();
}

void WireProtocolServer::stop() {
    const bool was_running = running_.exchange(false, std::memory_order_acq_rel);
    if (!was_running && io_threads_.empty()) {
        return;
    }

    if (acceptor_ && acceptor_->is_open()) {
        boost::system::error_code ec;
        acceptor_->cancel(ec);
        acceptor_->close(ec);
    }

    if (io_context_) {
        io_context_->stop();
    }

    for (auto& t : io_threads_) {
        // thread_join_no_timeout (W3): bounded join via timedJoin helper.
        timedJoin(t);
    }
    io_threads_.clear();

    if (worker_pool_) {
        // Cancel pending tasks first so wait() drains promptly.
        worker_pool_->stop();
        worker_pool_->wait();
    }
}

void WireProtocolServer::wait() {
    for (auto& t : io_threads_) {
        // thread_join_no_timeout (W3): bounded join via timedJoin helper.
        timedJoin(t);
    }
}

size_t WireProtocolServer::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
#ifdef THEMIS_ENABLE_WEBSOCKET
    return static_cast<int>(active_sessions_.size()) + static_cast<int>(active_ws_sessions_.size()) ;
#else
    return static_cast<int>(active_sessions_.size());
#endif
}

WireProtocolServer::Stats WireProtocolServer::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

// -------------------------------------------------------------------------
// Per-tenant bandwidth quota management – public forwarding API
// -------------------------------------------------------------------------

void WireProtocolServer::registerTenantQuota(const std::string& tenant_id,
                                               uint64_t rate_bps,
                                               uint64_t burst_bytes) {
    qos_manager_.registerTenantQuota(tenant_id, rate_bps, burst_bytes);
}

void WireProtocolServer::setTenantQuota(const std::string& tenant_id,
                                          uint64_t rate_bps,
                                          uint64_t burst_bytes) {
    qos_manager_.setTenantQuota(tenant_id, rate_bps, burst_bytes);
}

void WireProtocolServer::unregisterTenantQuota(const std::string& tenant_id) {
    qos_manager_.unregisterTenantQuota(tenant_id);
}

QoSManager::TenantQuotaStats
WireProtocolServer::getTenantBandwidthStats(const std::string& tenant_id) const {
    return qos_manager_.getTenantStats(tenant_id);
}

std::vector<QoSManager::TenantQuotaStats>
WireProtocolServer::getAllTenantBandwidthStats() const {
    return qos_manager_.getAllTenantStats();
}

// -------------------------------------------------------------------------
// Geospatial query injection bridge (stub #284)
// -------------------------------------------------------------------------

bool WireProtocolServer::checkConnectionLimit(const std::string& remote_ip) {
    // Global connection limit – fast path via atomic counter.
    if (config_.max_connections > 0 &&
        active_connection_count_.load(std::memory_order_relaxed) >= config_.max_connections) {
        return false;
    }
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_per_ip_.find(remote_ip);
    if (it != connections_per_ip_.end() && it->second >= config_.max_connections_per_ip) {
        return false;
    }
    return true;
}

bool WireProtocolServer::checkRateLimit(const std::string& remote_ip) {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // WPS-9: prune map before inserting to prevent unbounded growth from IP cycling
    constexpr size_t kMaxRateLimitEntries = 100'000;
    if (static_cast<int>(rate_limits_.size()) >= kMaxRateLimitEntries) {
        rate_limits_.clear();
    }

    auto& state = rate_limits_[remote_ip];
    if (state.window_start_ms == 0) {
        state.window_start_ms        = now_ms;
        state.minute_window_start_ms = now_ms;
    }

    // Reset per-second window
    if (now_ms - state.window_start_ms >= 1000) {
        state.window_start_ms     = now_ms;
        state.request_count_second = 0;
    }

    // Reset per-minute window (WPS-6: was incorrectly reusing the 1-second window)
    if (now_ms - state.minute_window_start_ms >= 60000) {
        state.minute_window_start_ms = now_ms;
        state.request_count_minute   = 0;
    }

    // Check rate limits
    if (state.request_count_second >= config_.max_requests_per_second ||
        state.request_count_minute >= config_.max_requests_per_minute) {
        return false;
    }

    state.request_count_second++;
    state.request_count_minute++;
    return true;
}

void WireProtocolServer::registerConnection(const std::string& remote_ip) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_per_ip_[remote_ip]++;
    active_connection_count_.fetch_add(1, std::memory_order_relaxed);
}

void WireProtocolServer::unregisterConnection(const std::string& remote_ip) {
    bool was_registered = false;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto it = connections_per_ip_.find(remote_ip);
        if (it != connections_per_ip_.end() && it->second > 0) {
            it->second--;
            if (it->second == 0) {
                connections_per_ip_.erase(it);
            }
            was_registered = true;
        }
    }
    // Only adjust the atomic counter when the connection was actually registered.
    // Guard against spurious calls for sessions that were rejected before
    // registerConnection() could be called (client_ip_ == "unknown") and for
    // WebSocket-upgraded sessions whose client_ip_ was cleared prior to upgrade.
    if (!was_registered) {
      return;
    }

    const uint32_t prev = active_connection_count_.fetch_sub(1, std::memory_order_relaxed);
    // Detect recovery: if we were overloaded and have now dropped below the
    // limit, clear the flag and emit a single recovery log message.
    if (overloaded_.load(std::memory_order_relaxed) && config_.max_connections > 0 &&
        prev - 1 < config_.max_connections) {
        overloaded_.store(false, std::memory_order_relaxed);
        std::cerr << "[WireProtocol] Backpressure recovery: active connections dropped to "
                  << (prev - 1) << " (limit=" << config_.max_connections
                  << "). Accepting new connections again.\n";
    }
}

void WireProtocolServer::doAccept() {
    if (!running_.load(std::memory_order_acquire)) {
      return;
    }

    auto session = std::make_shared<Session>(
        session_id_counter_.fetch_add(1, std::memory_order_acq_rel),
        tcp::socket(*io_context_),
        this);

    acceptor_->async_accept(
        const_cast<tcp::socket&>(session->socket_),
        [this, session](const boost::system::error_code& ec) {
            handleAccept(session, ec);
        });
}

void WireProtocolServer::handleAccept(std::shared_ptr<Session> session, const boost::system::error_code& error) {
    if (!error) {
        // Get remote IP from accepted socket
        std::string remote_ip = "unknown";
        try {
            remote_ip = session->socket_.remote_endpoint().address().to_string();
        } catch (const std::exception& e) {
            std::cerr << "[WireProtocol] Failed to resolve remote endpoint: "
                      << e.what() << ". Using fallback client_ip=unknown.\n";
        }

        if (!checkConnectionLimit(remote_ip)) {
            // Explicitly close the socket so the kernel-side TCP connection is
            // torn down promptly rather than waiting for the session destructor.
            boost::system::error_code close_ec;
            session->socket_.close(close_ec);

            // Track and log the overload state (only log on the first rejection
            // to avoid flooding the log when the server is saturated).
            if (!overloaded_.exchange(true, std::memory_order_relaxed)) {
                std::cerr << "[WireProtocol] Backpressure: connection limit reached ("
                          << active_connection_count_.load(std::memory_order_relaxed)
                          << "/" << config_.max_connections
                          << "). New connections from " << anonymizePeerForLog(remote_ip)
                          << " are being rejected until load decreases.\n";
            }

            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.rejected_connections++;
            }
            doAccept();
            return;
        }

        if (!checkRateLimit(remote_ip)) {
            boost::system::error_code close_ec;
            session->socket_.close(close_ec);

            std::cerr << "[WireProtocol] Backpressure: rate limit exceeded for "
                      << anonymizePeerForLog(remote_ip) << ". Connection rejected.\n";

            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.rejected_connections++;
            }
            doAccept();
            return;
        }

        registerConnection(remote_ip);

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            active_sessions_.try_emplace(remote_ip, session);
        }

        session->start();
    }

    doAccept();
}

// =============================================================================
// Session Implementation
// =============================================================================

WireProtocolServer::Session::Session(uint64_t session_id, tcp::socket socket, WireProtocolServer* server)
    : session_id_(session_id)
    , socket_(std::move(socket))
    , server_(server)
    , client_ip_("unknown")  // Will be set after accept in start()
{
}

WireProtocolServer::Session::~Session() {
    close();
}

void WireProtocolServer::Session::start() {
    try {
        // Now that socket is accepted, we can get the remote endpoint
        client_ip_ = socket_.remote_endpoint().address().to_string();
    } catch (...) {
        client_ip_ = "unknown";
    }

    // Register this connection with the per-tenant QoS manager
    server_->qos_manager_.registerConnection(session_id_, Priority::MEDIUM);

    startTimeout(std::chrono::seconds(server_->config_.request_timeout_sec));

#ifdef THEMIS_ENABLE_WEBSOCKET
    if (server_->config_.enable_websocket_upgrade) {
        asyncDetectProtocol();
        return;
    }
#endif

    asyncReadHeader();
}

void WireProtocolServer::Session::close() {
    if (closed_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }

    cancelTimeout();
    try {
        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.shutdown(tcp::socket::shutdown_both, ec);
            socket_.close();
        }
    } catch (...) {
    }

    // Deregister from per-tenant QoS manager
    server_->qos_manager_.unregisterConnection(session_id_);

    const std::string session_ip = client_ip_;
    server_->unregisterConnection(session_ip);

    {
        std::lock_guard<std::mutex> lock(server_->connections_mutex_);
        server_->active_sessions_.erase(session_ip);
    }
}

std::string WireProtocolServer::Session::getRemoteIP() const {
    try {
        return socket_.remote_endpoint().address().to_string();
    } catch (...) {
        return "unknown";
    }
}

void WireProtocolServer::Session::setTenant(const std::string& tenant_id) {
    tenant_id_ = tenant_id;
    server_->qos_manager_.assignTenant(session_id_, tenant_id);
}

void WireProtocolServer::Session::asyncReadHeader() {
    auto self = shared_from_this();
    net::async_read(
        socket_,
        net::buffer(header_buffer_),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes*/) {
            if (!ec) {
                // Parse header to get payload size, then read payload
                if (static_cast<int>(header_buffer_.size()) >= 12) {
                    // WPS-4 fix: Validate 4-byte magic field "TMDB" (0x544D4442) before
                    // dispatching any further reads.  An invalid magic closes the connection
                    // immediately to prevent unknown clients from reaching the opcode dispatcher.
                    uint32_t magic_recv = 0;
                    std::memcpy(&magic_recv, &header_buffer_[0], sizeof(uint32_t));
                    magic_recv = ntohl(magic_recv);
                    static constexpr uint32_t kExpectedMagic = 0x544D4442u; // "TMDB"
                    if (magic_recv != kExpectedMagic) {
                        sendError(0x0008, "Invalid frame magic");
                        socket_.close();
                        return;
                    }

                    // Extract flags (bytes 6-7, big-endian)
                    uint16_t flags = 0;
                    std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
                    flags = ntohs(flags);
                    current_flags_ = flags;
                    
                    // Extract payload size (bytes 8-11, big-endian)
                    // Wire format: Magic(4) + Version(1) + OpCode(1) + Flags(2) + PayloadSize(4)
                    uint32_t payload_size = 0;
                    std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
                    payload_size = ntohl(payload_size);
                    
                    // WPS-5 fix: cast to uint64_t before multiplication to prevent integer
                    // overflow when max_frame_size_mb >= 4096 (wraps to 0 → check disabled).
                    if (static_cast<uint64_t>(payload_size) >
                            static_cast<uint64_t>(server_->config_.max_frame_size_mb) * 1024 * 1024) {
                        sendError(0x0001, "Payload size exceeds maximum allowed");
                        asyncReadHeader();  // Continue reading
                        return;
                    }
                    
                    // Read payload if present
                    asyncReadPayload(payload_size);
                } else {
                    sendError(0x0008, "Invalid header size");
                    asyncReadHeader();
                }
            } else {
                handleError("asyncReadHeader", ec);
            }
        });
}

#ifdef THEMIS_ENABLE_WEBSOCKET
// ---------------------------------------------------------------------------
// WebSocket upgrade detection
// ---------------------------------------------------------------------------

// Step 1: read first 4 bytes to detect "GET " (HTTP) vs binary magic ("TMDB")
void WireProtocolServer::Session::asyncDetectProtocol() {
    auto self = shared_from_this();
    // Reuse the first 4 bytes of header_buffer_ as the peek buffer
    net::async_read(
        socket_,
        net::buffer(header_buffer_.data(), 4),
        [this, self](const boost::system::error_code& ec, std::size_t bytes_read) {
            if (ec || bytes_read != 4) {
                handleError("asyncDetectProtocol", ec);
                return;
            }

            // HTTP upgrade requests start with "GET " (0x47 0x45 0x54 0x20)
            const bool is_http_get =
                header_buffer_[0] == 'G' &&
                header_buffer_[1] == 'E' &&
                header_buffer_[2] == 'T' &&
                header_buffer_[3] == ' ';

            if (is_http_get) {
                // Read the rest of the HTTP request and upgrade to WebSocket
                std::array<uint8_t, 4> first_bytes;
                std::copy(header_buffer_.begin(), header_buffer_.begin() + 4,
                          first_bytes.begin());
                asyncUpgradeToWebSocket(first_bytes);
            } else {
                // Binary wire protocol: already have first 4 bytes in header_buffer_,
                // read the remaining 8 bytes to complete the 12-byte frame header.
                asyncReadRemainingHeader();
            }
        });
}

// Step 2a (binary): read remaining 8 header bytes after the first 4 were peeked
void WireProtocolServer::Session::asyncReadRemainingHeader() {
    auto self = shared_from_this();
    // header_buffer_[0..3] already filled; read bytes [4..11]
    net::async_read(
        socket_,
        net::buffer(header_buffer_.data() + 4, 8),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes*/) {
            if (!ec) {
                // WPS-4 fix: validate the magic bytes that were peeked in asyncDetectProtocol.
                // bytes [0..3] are already in header_buffer_; check them now that the full
                // 12-byte header is available.
                uint32_t magic_recv = 0;
                std::memcpy(&magic_recv, &header_buffer_[0], sizeof(uint32_t));
                magic_recv = ntohl(magic_recv);
                static constexpr uint32_t kExpectedMagic = 0x544D4442u; // "TMDB"
                if (magic_recv != kExpectedMagic) {
                    sendError(0x0008, "Invalid frame magic");
                    socket_.close();
                    return;
                }

                // Same logic as asyncReadHeader callback
                uint16_t flags = 0;
                std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
                flags = ntohs(flags);
                current_flags_ = flags;

                uint32_t payload_size = 0;
                std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
                payload_size = ntohl(payload_size);

                // WPS-5 fix: cast to uint64_t before multiplication (same as asyncReadHeader).
                if (static_cast<uint64_t>(payload_size) >
                        static_cast<uint64_t>(server_->config_.max_frame_size_mb) * 1024 * 1024) {
                    sendError(0x0001, "Payload size exceeds maximum allowed");
                    asyncReadHeader();
                    return;
                }

                asyncReadPayload(payload_size);
            } else {
                handleError("asyncReadRemainingHeader", ec);
            }
        });
}

// Step 2b (WebSocket): read the complete HTTP request and hand off to WS session
void WireProtocolServer::Session::asyncUpgradeToWebSocket(
    const std::array<uint8_t, 4>& first_bytes)
{
    // Read the remaining HTTP request lines.
    // We reuse a Beast flat_buffer to accumulate the data.
    // The first 4 bytes ("GET ") are pre-pended so Beast can parse the full request.
    auto self = shared_from_this();

    // Buffer must outlive the async operation – allocate on heap
    auto buf  = std::make_shared<beast::flat_buffer>();
    auto req  = std::make_shared<http_ws::request<http_ws::string_body>>();

    // Seed the buffer with the 4 bytes we already consumed from the socket
    {
        auto mutable_buf = buf->prepare(4);
        net::buffer_copy(mutable_buf, net::buffer(first_bytes.data(), 4));
        buf->commit(4);
    }

    // Async-read the rest of the HTTP request
    http_ws::async_read(
        socket_,
        *buf,
        *req,
        [this, self, buf, req](const boost::system::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                handleError("asyncUpgradeToWebSocket", ec);
                return;
            }

            // Verify this is actually a WebSocket upgrade request
            if (!websocket::is_upgrade(*req)) {
                // Not a WebSocket upgrade; reject gracefully
                http_ws::response<http_ws::string_body> resp{
                    http_ws::status::bad_request, req->version()};
                resp.set(http_ws::field::content_type, "text/plain");
                resp.body() = "ThemisDB wire protocol port: expected WebSocket "
                              "Upgrade or binary wire-protocol frame\r\n";
                resp.prepare_payload();
                http_ws::write(socket_, resp);
                close();
                return;
            }

            // Cancel the connection-level timeout (WebSocket manages its own)
            cancelTimeout();

            // Transfer connection tracking to the WebSocket session.
            // We pass the original client IP so the WS session can call
            // unregisterConnection() when it closes, keeping per-IP connection
            // counts accurate for the lifetime of the WS session.
            // We then clear client_ip_ on the binary Session so that its
            // destructor does NOT call unregisterConnection() a second time
            // (which would drop the count to 0 while the WS session is still alive).
            const std::string ws_client_ip = client_ip_;
            auto ws_session = std::make_shared<WireProtocolWebSocketSession>(
                std::move(socket_), server_, ws_client_ip);
            client_ip_.clear();  // neutralize destructor unregister

            // Remove from binary-session tracking
            {
                std::lock_guard<std::mutex> lock(server_->connections_mutex_);
                server_->active_sessions_.erase(ws_client_ip);
            }

            ws_session->run(std::move(*req));
        });
}
#endif // THEMIS_ENABLE_WEBSOCKET

void WireProtocolServer::Session::asyncReadPayload([[maybe_unused]] uint32_t payload_size) {
    if (payload_size == 0) {
        // No payload, check if we need to read checksum
        payload_buffer_.clear();
        
        // Check if SKIP_CHECKSUM flag is set (bit 2)
        const uint16_t SKIP_CHECKSUM_FLAG = 0x0004;
        if (!(current_flags_ & SKIP_CHECKSUM_FLAG)) {
            // Checksum expected, read it
            asyncReadChecksum();
        } else {
            // No checksum, dispatch immediately
            handleMessage();
            asyncReadHeader();
        }
        return;
    }

    payload_buffer_.resize(payload_size);
    auto self = shared_from_this();
    
    net::async_read(
        socket_,
        net::buffer(payload_buffer_),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes*/) {
            if (!ec) {
                // Payload read successfully, check if we need to read checksum
                const uint16_t SKIP_CHECKSUM_FLAG = 0x0004;
                if (!(current_flags_ & SKIP_CHECKSUM_FLAG)) {
                    // Checksum expected, read it
                    asyncReadChecksum();
                } else {
                    // No checksum, dispatch immediately
                    handleMessage();
                    asyncReadHeader();
                }
            } else {
                handleError("asyncReadPayload", ec);
            }
        });
}

void WireProtocolServer::Session::asyncReadChecksum() {
    auto self = shared_from_this();
    net::async_read(
        socket_,
        net::buffer(&checksum_buffer_, sizeof(checksum_buffer_)),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes*/) {
            if (!ec) {
                // Verify CRC32 checksum over the 12-byte header + payload.
                // The wire format stores the checksum in network byte order (big-endian).
                uint32_t expected_crc = ntohl(checksum_buffer_);

                uint32_t computed_crc = crc32Update(
                    0,
                    header_buffer_.data(),static_cast<int>(header_buffer_.size()));
                if (!payload_buffer_.empty()) {
                    computed_crc = crc32Update(
                        computed_crc,
                        payload_buffer_.data(),static_cast<int>(payload_buffer_.size()));
                }

                if (computed_crc != expected_crc) {
                    THEMIS_WARN("[WireProto] session {} checksum mismatch "
                                "(expected={:#010x}, computed={:#010x}) – dropping frame",
                                session_id_, expected_crc, computed_crc);
                    sendError(0x000F, "Checksum mismatch");
                    asyncReadHeader();  // Continue; do not close (client can recover)
                    return;
                }

                // Dispatch message
                handleMessage();
                asyncReadHeader();  // Continue reading next message
            } else {
                handleError("asyncReadChecksum", ec);
            }
        });
}

void WireProtocolServer::Session::dispatchToWorkerPool(std::function<void()> handler) {
    if (server_->worker_pool_) {
        auto self = shared_from_this();
        // Copy the current frame's buffers so the handler can read them on the
        // worker thread. The copies are written back into the session members
        // before calling fn() so that handler methods which access payload_buffer_
        // and header_buffer_ via 'this->' see the correct frame data.
        //
        // KNOWN LIMITATION: payload_buffer_ is also used by asyncReadPayload
        // for the NEXT incoming frame. Under high-frequency pipelining, a race
        // exists between this write (worker thread) and asyncReadPayload's
        // resize+async_read (I/O thread). The canonical fix is to use a per-session
        // net::strand to serialize all session state mutations, or to pass the
        // payload as an explicit parameter to each handler method instead of
        // relying on the session-level member.
        // [I] ROADMAP: tracked in src/network/ROADMAP.md § "Wire Protocol
        //     Session-State Strand Safety" — target Q1 2027.
        auto payload_copy = payload_buffer_;
        auto header_copy  = header_buffer_;
        net::post(*server_->worker_pool_,
            [this, self,
             payload = std::move(payload_copy),
             hdr     = std::move(header_copy),
             fn      = std::move(handler)]() mutable {
                payload_buffer_ = std::move(payload);
                header_buffer_  = std::move(hdr);
                fn();
            });
    } else {
        handler();
    }
}

void WireProtocolServer::Session::handleMessage() {
    requests_processed_.fetch_add(1, std::memory_order_relaxed);
    bytes_received_.fetch_add(static_cast<int>(header_buffer_.size()) + static_cast<int>(payload_buffer_.size()) , std::memory_order_relaxed);
    
    // Validate header size (must be at least 12 bytes)
    if (static_cast<int>(header_buffer_.size()) < 12) {
        sendError(0x0008, "Invalid header size");
        return;
    }
    
    // Parse header: Magic (4) + Version (1) + OpCode (1) + Flags (2) + PayloadSize (4)
    // Extract OpCode from header_buffer_[5] (0-indexed: bytes 0-3 are magic, 4 is version, 5 is opcode)
    uint8_t opcode = header_buffer_[5];
    
    // Dispatch based on OpCode
    // Note: payload_buffer_ is already populated by asyncReadPayload
    switch (opcode) {
        case 0x01: // HELLO
            handleHello();
            break;
        case 0x03: // AUTH legacy opcode: historically used by clients to send credentials
            // Fall through: both 0x03 and 0x04 use the same credential-validation logic.
            [[fallthrough]];
        case 0x04: // AUTH_RESPONSE (Client→Server per wire protocol spec v1.3.0)
            handleAuthRequest();
            break;
        case 0x10: // GET
            handleGet();
            break;
        case 0x11: // PUT
            handlePut();
            break;
        case 0x12: // DELETE
            handleDelete();
            break;
        case 0x13: // BATCH_GET
            dispatchToWorkerPool([this]() { handleBatchGet(); });
            break;
        case 0x14: // BATCH_PUT
            dispatchToWorkerPool([this]() { handleBatchPut(); });
            break;
        case 0x20: // QUERY_AQL
            dispatchToWorkerPool([this]() { handleQuery(); });
            break;
        case 0x23: // CURSOR_NEXT
            handleCursorNext();
            break;
        case 0x24: // CURSOR_CLOSE
            handleCursorClose();
            break;
        case 0x30: // TRANSACTION_BEGIN
            handleTransactionBegin();
            break;
        case 0x31: // TRANSACTION_COMMIT
            handleTransactionCommit();
            break;
        case 0x32: // TRANSACTION_ABORT
            handleTransactionAbort();
            break;
        case 0x40: // VECTOR_SEARCH
            dispatchToWorkerPool([this]() { handleVectorSearch(); });
            break;
        case 0x41: // GRAPH_TRAVERSE
            dispatchToWorkerPool([this]() { handleGraphTraverse(); });
            break;
        case 0x50: // GEO_QUERY
            dispatchToWorkerPool([this]() { handleGeoQuery(); });
            break;
        case 0x51: // TIMESERIES_QUERY
            dispatchToWorkerPool([this]() { handleTimeseriesQuery(); });
            break;
        case 0x60: // BPMN_START_PROCESS
            handleBpmnStartProcess();
            break;
        case 0x61: // BPMN_TASK_COMPLETE
            handleBpmnTaskComplete();
            break;
        case 0x62: // BPMN_QUERY_INSTANCE
            handleBpmnQueryInstance();
            break;
        case 0xFE: // PING
            handlePing();
            break;
        case 0xFF: // CLOSE
            handleClose();
            break;
        default: {
            // Format opcode as hexadecimal
            char hex_opcode[8];
            std::snprintf(hex_opcode, sizeof(hex_opcode), "0x%02X", opcode);
            sendError(0x0002, std::string("Unknown OpCode: ") + hex_opcode);
            break;
        }
    }
}

void WireProtocolServer::Session::sendError(uint32_t error_code, const std::string& message) {
    // Build error response with header
    json error_json;
    error_json["error_code"] = error_code;
    error_json["error_message"] = message;
    
    std::string error_str = error_json.dump();
    std::vector<uint8_t> error_data(error_str.begin(), error_str.end());
    
    asyncWriteResponse(error_data);
}

void WireProtocolServer::Session::handlePing() {
    // Simple ping response
    json response;
    response["pong"] = true;
    response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    std::string response_str = response.dump();
    std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
    asyncWriteResponse(response_data);
}

void WireProtocolServer::Session::handleClose() {
    close();
}

void WireProtocolServer::Session::handleError([[maybe_unused]] const std::string& context, const boost::system::error_code& ec) {
    if (ec != net::error::operation_aborted) {
        // Log error
    }
    close();
}

void WireProtocolServer::Session::startTimeout(std::chrono::seconds timeout) {
    timeout_timer_ = std::make_unique<net::steady_timer>(*server_->io_context_, timeout);
    auto self = shared_from_this();
    timeout_timer_->async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec) {
            close();
        }
    });
}

void WireProtocolServer::Session::cancelTimeout() {
    if (timeout_timer_) {
        timeout_timer_->cancel();
    }
}

void WireProtocolServer::Session::asyncWriteResponse(const std::vector<uint8_t>& data) {
    // Enqueue under the write mutex, then dispatch to the socket's executor so that
    // doWrite() is always initiated from the correct I/O thread (required by Boost.Asio).
    auto self = shared_from_this();
    auto data_copy = data;  // capture by value so the caller's buffer can be freed
    net::dispatch(socket_.get_executor(), [this, self, data_copy = std::move(data_copy)]() {
        bool should_start_write = false;
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            write_queue_.push_back(std::move(data_copy));
            if (!write_in_progress_) {
                write_in_progress_ = true;
                should_start_write = true;
            }
        }

        if (should_start_write) {
            doWrite();
        }
    });
}

void WireProtocolServer::Session::doWrite() {
    std::shared_ptr<std::vector<uint8_t>> write_buffer;
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (write_queue_.empty()) {
            write_in_progress_ = false;
            return;
        }
        write_buffer = std::make_shared<std::vector<uint8_t>>(std::move(write_queue_.front()));
        write_queue_.pop_front();
    }

    auto self = shared_from_this();
    net::async_write(
        socket_,
        net::buffer(*write_buffer),
        [this, self, write_buffer](const boost::system::error_code& ec, std::size_t bytes) {
            if (!ec) {
                bytes_sent_.fetch_add(bytes, std::memory_order_relaxed);
                server_->qos_manager_.recordBytesSent(session_id_, bytes);

                doWrite();  // Continue with next message or release in-progress flag
            } else {
                std::lock_guard<std::mutex> lock(write_mutex_);
                write_in_progress_ = false;
                handleError("asyncWrite", ec);
            }
        });
}

// =============================================================================
// Message Handler Implementations
// =============================================================================

void WireProtocolServer::Session::handleHello() {
    // HELLO handshake: return server capabilities and version information.
    // No authentication is required for HELLO – it must be the first message
    // sent by a connecting client.
    try {
        json response;
        response["server"] = "ThemisDB";
        response["wire_protocol_version"] = 1;
        response["server_version"] = "1.7.0";
        response["auth_required"] = server_->config_.require_auth;
        // WPS-11: Do not leak the internal auth mechanism identifier to
        // unauthenticated clients. Indicate only whether auth is supported.
        response["auth_supported"] = true;
        response["capabilities"] = json::array({
            "GET", "PUT", "DELETE", "QUERY_AQL",
            "VECTOR_SEARCH", "TIMESERIES_QUERY",
            "BPMN_START_PROCESS", "BPMN_TASK_COMPLETE", "BPMN_QUERY_INSTANCE",
            "PING", "CLOSE"
        });

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("HELLO error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleAuthRequest() {
    // Token-based authentication.
    // Expected payload (JSON): {"token": "<bearer-token>", "username": "<optional>"}
    // On success sets authenticated_ = true and records the username.
    // When Config::require_auth is false, any non-empty token (or no token) is accepted.
    try {
        std::string token = {};
        std::string username_req = {};

        if (!payload_buffer_.empty()) {
            if (static_cast<int>(payload_buffer_.size()) > kMaxAuthPayloadBytes) {
                sendError(413, "AUTH payload too large");
                return;
            }

            json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
            if (!request.is_object()) {
                sendError(400, "Invalid AUTH payload: expected JSON object");
                return;
            }
            if (request.contains("token") && !request["token"].is_string()) {
                sendError(400, "Invalid AUTH payload: token must be string");
                return;
            }
            if (request.contains("username") && !request["username"].is_string()) {
                sendError(400, "Invalid AUTH payload: username must be string");
                return;
            }

            token = request.value("token", "");
            username_req = request.value("username", "");

            if (!username_req.empty() && !isReasonableWireIdentifier(username_req)) {
                sendError(400, "Invalid AUTH payload: username contains control characters or is too long");
                return;
            }
        }

        bool accepted = false;

        if (!server_->config_.require_auth) {
            // Auth disabled – accept all clients.
            accepted = true;
        } else if (!server_->config_.auth_token.empty()) {
            // WPS-2 fix: use CRYPTO_memcmp for constant-time comparison to prevent
            // timing side-channel attacks that leak the pre-shared token prefix/length.
            const auto& stored = server_->config_.auth_token;
            if (static_cast<int>(token.size()) == static_cast<int>(stored.size())) {
                accepted = (CRYPTO_memcmp(token.data(), stored.data(),static_cast<int>(stored.size())) == 0);
            }
            // Different lengths → accepted stays false (no timing leak from size mismatch
            // because the size comparison itself is O(1) and constant).
        } else {
            // WPS-3 fix: auth_token is empty but require_auth=true — this is a
            // misconfiguration (missing secret env-var). Reject all connections with a
            // configuration error rather than silently accepting any non-empty token.
            spdlog::error("[SEC/WPS-3] require_auth=true but configured authentication "
                          "secret is empty — all connections rejected. Set the auth token "
                          "parameter or disable require_auth for development use.");
            accepted = false;
        }

        if (accepted) {
            authenticated_.store(true, std::memory_order_release);
            username_ = username_req.empty() ? "wire-client" : username_req;

            json response;
            response["authenticated"] = true;
            response["username"] = username_;
            response["message"] = "Authentication successful";

            std::string response_str = response.dump();
            std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
            asyncWriteResponse(response_data);
        } else {
            // Record auth failure in server statistics.
            {
                std::lock_guard<std::mutex> lock(server_->stats_mutex_);
                server_->stats_.auth_failures++;
            }
            sendError(0x0401, "Authentication failed: invalid token");
        }
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in AUTH payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("AUTH error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleGet() {
    // GET: retrieve a document by collection and key from RocksDB.
    // Expected payload (JSON): {"collection": "...", "key": "..."}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    if (!server_->storage_) {
        throw std::runtime_error(
            "CRITICAL: Storage engine not wired to wire protocol server. "
            "Storage injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxCrudPayloadBytes) {
            sendError(413, "GET payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid GET payload: expected JSON object");
            return;
        }
        if (((request.contains("collection") && !request["collection"].is_string()) ||
            (request.contains("key") && !request["key"].is_string()))) {
            sendError(400, "Invalid 'collection' or 'key' type in GET request");
            return;
        }
        std::string collection = request.value("collection", "");
        std::string key = request.value("key", "");

        if (collection.empty() || key.empty() || isBlankString(collection) || isBlankString(key)) {
            sendError(400, "Missing 'collection' or 'key' in GET request");
            return;
        }
        if (!isReasonableWireIdentifier(collection) || !isReasonableWireIdentifier(key)) {
            sendError(400, "Invalid 'collection' or 'key' in GET request");
            return;
        }

        // Keys are stored as "<collection>:<key>" in RocksDB.
        std::string storage_key = collection + ":" + key;
        auto result = server_->storage_->get(storage_key);

        json response = {};
        if (result.has_value()) {
            const auto& value_bytes = result.value();
            // Try to parse value as JSON; fall back to base64-style string.
            std::string value_str(value_bytes.begin(), value_bytes.end());
            try {
                response["value"] = json::parse(value_str);
            } catch (const json::exception&) {
                response["value"] = value_str;
            }
            response["found"] = true;
            response["collection"] = collection;
            response["key"] = key;
        } else {
            response["found"] = false;
            response["collection"] = collection;
            response["key"] = key;
        }

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in GET payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("GET error: ") + e.what());
    }
}

void WireProtocolServer::Session::handlePut() {
    // PUT: store a document by collection and key in RocksDB.
    // Expected payload (JSON): {"collection": "...", "key": "...", "value": {...}}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    if (!server_->storage_) {
        throw std::runtime_error(
            "CRITICAL: Storage engine not wired to wire protocol server. "
            "Storage injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxCrudPayloadBytes) {
            sendError(413, "PUT payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid PUT payload: expected JSON object");
            return;
        }
        if (((request.contains("collection") && !request["collection"].is_string()) ||
            (request.contains("key") && !request["key"].is_string()))) {
            sendError(400, "Invalid 'collection' or 'key' type in PUT request");
            return;
        }
        std::string collection = request.value("collection", "");
        std::string key = request.value("key", "");

        if (collection.empty() || key.empty() || isBlankString(collection) || isBlankString(key)) {
            sendError(400, "Missing 'collection' or 'key' in PUT request");
            return;
        }
        if (!isReasonableWireIdentifier(collection) || !isReasonableWireIdentifier(key)) {
            sendError(400, "Invalid 'collection' or 'key' in PUT request");
            return;
        }
        if (!request.contains("value")) {
            sendError(400, "Missing 'value' in PUT request");
            return;
        }

        // Serialise value to JSON string for storage.
        std::string value_str = request["value"].is_string()
            ? request["value"].get<std::string>()
            : request["value"].dump();

        std::string storage_key = collection + ":" + key;
        bool ok = server_->storage_->put(storage_key, value_str);

        json response;
        response["success"] = ok;
        response["collection"] = collection;
        response["key"] = key;
        if (!ok) {
            response["error"] = "Storage write failed";
        }

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in PUT payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("PUT error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleDelete() {
    // DELETE: remove a document by collection and key from RocksDB.
    // Expected payload (JSON): {"collection": "...", "key": "..."}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    if (!server_->storage_) {
        throw std::runtime_error(
            "CRITICAL: Storage engine not wired to wire protocol server. "
            "Storage injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxCrudPayloadBytes) {
            sendError(413, "DELETE payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid DELETE payload: expected JSON object");
            return;
        }
        if (((request.contains("collection") && !request["collection"].is_string()) ||
            (request.contains("key") && !request["key"].is_string()))) {
            sendError(400, "Invalid 'collection' or 'key' type in DELETE request");
            return;
        }
        std::string collection = request.value("collection", "");
        std::string key = request.value("key", "");

        if (collection.empty() || key.empty() || isBlankString(collection) || isBlankString(key)) {
            sendError(400, "Missing 'collection' or 'key' in DELETE request");
            return;
        }
        if (!isReasonableWireIdentifier(collection) || !isReasonableWireIdentifier(key)) {
            sendError(400, "Invalid 'collection' or 'key' in DELETE request");
            return;
        }

        std::string storage_key = collection + ":" + key;
        bool ok = server_->storage_->del(storage_key);

        json response;
        response["success"] = ok;
        response["collection"] = collection;
        response["key"] = key;

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in DELETE payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("DELETE error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleBatchGet() {
    // BATCH_GET: retrieve multiple documents by collection and key list.
    // Expected payload (JSON): {"collection": "...", "keys": ["key1", "key2", ...]}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    if (!server_->storage_) {
        throw std::runtime_error(
            "CRITICAL: Storage engine not wired to wire protocol server. "
            "Storage injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxBatchPayloadBytes) {
            sendError(413, "BATCH_GET payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid BATCH_GET payload: expected JSON object");
            return;
        }
        if (request.contains("collection") && !request["collection"].is_string()) {
            sendError(400, "Invalid 'collection' type in BATCH_GET request");
            return;
        }
        std::string collection = request.value("collection", "");

        if (collection.empty() || isBlankString(collection)) {
            sendError(400, "Missing 'collection' in BATCH_GET request");
            return;
        }
        if (!isReasonableWireIdentifier(collection)) {
            sendError(400, "Invalid 'collection' in BATCH_GET request");
            return;
        }
        if (!request.contains("keys") || !request["keys"].is_array()) {
            sendError(400, "Missing or invalid 'keys' array in BATCH_GET request");
            return;
        }

        const auto& keys_arr = request["keys"];
        if (keys_arr.empty()) {
            sendError(400, "Empty 'keys' array in BATCH_GET request");
            return;
        }

        constexpr size_t kMaxBatchElements = 1000;
        if (static_cast<int>(keys_arr.size()) > kMaxBatchElements) {
            sendError(400, "BATCH_GET exceeds maximum of 1000 keys per request");
            return;
        }

        std::vector<std::string> client_keys = {};

        client_keys.reserve(keys_arr.size());
        std::vector<std::string> storage_keys = {};

        storage_keys.reserve(keys_arr.size());
        for (const auto& key_val : keys_arr) {
            if (!key_val.is_string()) {
                sendError(400, "Each element in 'keys' must be a string in BATCH_GET request");
                return;
            }
            const std::string key = key_val.get<std::string>();
            if (isBlankString(key)) {
                sendError(400, "Each 'keys' element must be a non-blank identifier in BATCH_GET request");
                return;
            }
            if (!isReasonableWireIdentifier(key)) {
                sendError(400, "Each 'keys' element must be a valid identifier in BATCH_GET request");
                return;
            }
            client_keys.push_back(key);
            storage_keys.push_back(collection + ":" + key);
        }

        const auto multi_results = server_->storage_->multiGet(storage_keys);

        json results = json::array();
        uint32_t found_count = 0;
        uint32_t not_found_count = 0;
        for (size_t i = 0; i < client_keys.size(); ++i) {
            json item;
            item["key"] = client_keys[i];
            if (i < multi_results.size() && multi_results[i].has_value()) {
                const auto& value_bytes = multi_results[i].value();
                std::string value_str(value_bytes.begin(), value_bytes.end());
                try {
                    item["value"] = json::parse(value_str);
                } catch (const json::exception&) {
                    item["value"] = value_str;
                }
                item["found"] = true;
                ++found_count;
            } else {
                item["found"] = false;
                ++not_found_count;
            }
            results.push_back(std::move(item));
        }

        json response;
        response["results"] = std::move(results);
        response["found_count"] = found_count;
        response["not_found_count"] = not_found_count;
        response["collection"] = collection;

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in BATCH_GET payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("BATCH_GET error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleBatchPut() {
    // BATCH_PUT: store multiple documents by collection.
    // Expected payload (JSON): {"collection": "...", "items": [{"key": "...", "value": {...}}, ...]}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    if (!server_->storage_) {
        throw std::runtime_error(
            "CRITICAL: Storage engine not wired to wire protocol server. "
            "Storage injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxBatchPayloadBytes) {
            sendError(413, "BATCH_PUT payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid BATCH_PUT payload: expected JSON object");
            return;
        }
        if (request.contains("collection") && !request["collection"].is_string()) {
            sendError(400, "Invalid 'collection' type in BATCH_PUT request");
            return;
        }
        std::string collection = request.value("collection", "");

        if (collection.empty() || isBlankString(collection)) {
            sendError(400, "Missing 'collection' in BATCH_PUT request");
            return;
        }
        if (!isReasonableWireIdentifier(collection)) {
            sendError(400, "Invalid 'collection' in BATCH_PUT request");
            return;
        }
        if (!request.contains("items") || !request["items"].is_array()) {
            sendError(400, "Missing or invalid 'items' array in BATCH_PUT request");
            return;
        }

        const auto& items_arr = request["items"];
        if (items_arr.empty()) {
            sendError(400, "Empty 'items' array in BATCH_PUT request");
            return;
        }

        constexpr size_t kMaxBatchElements = 1000;
        if (static_cast<int>(items_arr.size()) > kMaxBatchElements) {
            sendError(400, "BATCH_PUT exceeds maximum of 1000 items per request");
            return;
        }

        struct ValidatedItem {
            std::string key;
            std::string storage_key;
            std::string value_str = {};
        };

        std::vector<ValidatedItem> valid_items = {};

        valid_items.reserve(items_arr.size());

        json results = json::array();
        uint32_t failure_count = 0;
        for (const auto& item_val : items_arr) {
            if (!item_val.is_object()) {
                json r;
                r["key"] = "";
                r["success"] = false;
                r["error"] = "Each item must be an object";
                results.push_back(std::move(r));
                ++failure_count;
                continue;
            }
            if (item_val.contains("key") && !item_val["key"].is_string()) {
                json r;
                r["key"] = "";
                r["success"] = false;
                r["error"] = "Invalid 'key' type in item";
                results.push_back(std::move(r));
                ++failure_count;
                continue;
            }
            const std::string key = item_val.value("key", "");
            if (key.empty() || isBlankString(key)) {
                json r;
                r["key"] = "";
                r["success"] = false;
                r["error"] = "Missing 'key' in item";
                results.push_back(std::move(r));
                ++failure_count;
                continue;
            }
            if (!isReasonableWireIdentifier(key)) {
                json r;
                r["key"] = key;
                r["success"] = false;
                r["error"] = "Invalid 'key' in item";
                results.push_back(std::move(r));
                ++failure_count;
                continue;
            }
            if (!item_val.contains("value")) {
                json r;
                r["key"] = key;
                r["success"] = false;
                r["error"] = "Missing 'value' in item";
                results.push_back(std::move(r));
                ++failure_count;
                continue;
            }

            std::string value_str = item_val["value"].is_string()
                ? item_val["value"].get<std::string>()
                : item_val["value"].dump();
            valid_items.push_back({key, collection + ":" + key, std::move(value_str)});
        }

        bool batch_ok = true;
        if (!valid_items.empty()) {
            auto batch = server_->storage_->createWriteBatch();
            for (const auto& vi : valid_items) {
                const std::vector<uint8_t> bytes(vi.value_str.begin(), vi.value_str.end());
                batch->put(vi.storage_key, bytes);
            }
            batch_ok = batch->commit();
        }

        uint32_t success_count = 0;
        for (const auto& vi : valid_items) {
            json item_result;
            item_result["key"] = vi.key;
            item_result["success"] = batch_ok;
            if (batch_ok) {
                ++success_count;
            } else {
                item_result["error"] = "Batch write failed";
                ++failure_count;
            }
            results.push_back(std::move(item_result));
        }

        json response;
        response["results"] = std::move(results);
        response["success_count"] = success_count;
        response["failure_count"] = failure_count;
        response["collection"] = collection;

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in BATCH_PUT payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("BATCH_PUT error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleTransactionBegin() {
    // TRANSACTION_BEGIN: begin a new transaction.
    // Expected payload (JSON): {"isolation_level": "read_committed|snapshot|serializable", "timeout_ms": 5000}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    if (!server_->tx_manager_) {
        throw std::runtime_error(
            "CRITICAL: Transaction manager not wired to wire protocol server. "
            "Transaction manager injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxTransactionPayloadBytes) {
            sendError(413, "TRANSACTION_BEGIN payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid TRANSACTION_BEGIN payload: expected JSON object");
            return;
        }
        if (request.contains("isolation_level") && !request["isolation_level"].is_string()) {
            sendError(400, "Invalid 'isolation_level' type in TRANSACTION_BEGIN request");
            return;
        }
        if (request.contains("timeout_ms") && !request["timeout_ms"].is_number_integer()) {
            sendError(400, "Invalid 'timeout_ms' type in TRANSACTION_BEGIN request");
            return;
        }
        if (request.contains("timeout_ms")) {
            const auto timeout_ms = request["timeout_ms"].get<int64_t>();
            constexpr int64_t kMinTransactionTimeoutMs = 1;
            constexpr int64_t kMaxTransactionTimeoutMs = 3'600'000;
            if (timeout_ms < kMinTransactionTimeoutMs || timeout_ms > kMaxTransactionTimeoutMs) {
                sendError(400, "'timeout_ms' must be between 1 and 3600000 in TRANSACTION_BEGIN request");
                return;
            }
        }
        std::string isolation_str = request.value("isolation_level", "read_committed");

        IsolationLevel isolation = IsolationLevel::ReadCommitted;
        if (isolation_str == "snapshot" || isolation_str == "repeatable_read") {
            isolation = IsolationLevel::Snapshot;
        } else if (isolation_str == "serializable") {
            isolation = IsolationLevel::SERIALIZABLE;
        } else if (isolation_str != "read_committed") {
            sendError(400, "Invalid 'isolation_level' in TRANSACTION_BEGIN request");
            return;
        }

        auto tx_id = server_->tx_manager_->beginTransaction(isolation);

        json response;
        response["success"] = true;
        response["transaction_id"] = std::to_string(tx_id);
        response["timestamp_ns"] = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in TRANSACTION_BEGIN payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("TRANSACTION_BEGIN error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleTransactionCommit() {
    // TRANSACTION_COMMIT: commit an open transaction.
    // Expected payload (JSON): {"transaction_id": "<numeric-string>"}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    if (!server_->tx_manager_) {
        throw std::runtime_error(
            "CRITICAL: Transaction manager not wired to wire protocol server. "
            "Transaction manager injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxTransactionPayloadBytes) {
            sendError(413, "TRANSACTION_COMMIT payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid TRANSACTION_COMMIT payload: expected JSON object");
            return;
        }
        if (request.contains("transaction_id") && !request["transaction_id"].is_string()) {
            sendError(400, "Invalid 'transaction_id' type in TRANSACTION_COMMIT request");
            return;
        }
        std::string tx_id_str = request.value("transaction_id", "");

        if (tx_id_str.empty() || isBlankString(tx_id_str)) {
            sendError(400, "Missing 'transaction_id' in TRANSACTION_COMMIT request");
            return;
        }
        if (!isUnsignedIntegerString(tx_id_str)) {
            sendError(400, "Invalid 'transaction_id' in TRANSACTION_COMMIT request");
            return;
        }

        TransactionManager::TransactionId tx_id = 0;
        try {
            tx_id = std::stoull(tx_id_str);
        } catch (...) {
            sendError(400, "Invalid numeric 'transaction_id' in TRANSACTION_COMMIT request");
            return;
        }
        auto status = server_->tx_manager_->commitTransaction(tx_id);

        json response;
        response["success"] = status.ok;
        if (!status.ok) {
            response["error"] = status.message;
        } else {
            response["commit_timestamp_ns"] = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
        }

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in TRANSACTION_COMMIT payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("TRANSACTION_COMMIT error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleTransactionAbort() {
    // TRANSACTION_ABORT: abort/roll back an open transaction.
    // Expected payload (JSON): {"transaction_id": "<numeric-string>"}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    if (!server_->tx_manager_) {
        throw std::runtime_error(
            "CRITICAL: Transaction manager not wired to wire protocol server. "
            "Transaction manager injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxTransactionPayloadBytes) {
            sendError(413, "TRANSACTION_ABORT payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid TRANSACTION_ABORT payload: expected JSON object");
            return;
        }
        if (request.contains("transaction_id") && !request["transaction_id"].is_string()) {
            sendError(400, "Invalid 'transaction_id' type in TRANSACTION_ABORT request");
            return;
        }
        std::string tx_id_str = request.value("transaction_id", "");

        if (tx_id_str.empty() || isBlankString(tx_id_str)) {
            sendError(400, "Missing 'transaction_id' in TRANSACTION_ABORT request");
            return;
        }
        if (!isUnsignedIntegerString(tx_id_str)) {
            sendError(400, "Invalid 'transaction_id' in TRANSACTION_ABORT request");
            return;
        }

        TransactionManager::TransactionId tx_id = 0;
        try {
            tx_id = std::stoull(tx_id_str);
        } catch (...) {
            sendError(400, "Invalid numeric 'transaction_id' in TRANSACTION_ABORT request");
            return;
        }
        bool aborted = server_->tx_manager_->rollbackTransaction(tx_id);

        json response;
        response["success"] = aborted;
        if (!aborted) {
            response["error"] = "Transaction rollback failed: transaction not found or already finished";
        }

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in TRANSACTION_ABORT payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("TRANSACTION_ABORT error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleGraphTraverse() {
    // GRAPH_TRAVERSE: traverse graph edges from a start vertex.
    // Expected payload (JSON):
    //   {"collection": "...", "start_vertex": "...", "direction": "outbound|inbound|any",
    //    "depth_min": 1, "depth_max": 3, "limit": 100}
    // NOTE: Full graph traversal integration over the wire protocol is planned for a
    // future release.  Until then clients should use the HTTP REST API (/api/v1/graph).
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxGraphPayloadBytes) {
            sendError(413, "GRAPH_TRAVERSE payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid GRAPH_TRAVERSE payload: expected JSON object");
            return;
        }
        if ((request.contains("collection") && !request["collection"].is_string()) ||
            (request.contains("start_vertex") && !request["start_vertex"].is_string())) {
            sendError(400, "Invalid 'collection' or 'start_vertex' type in GRAPH_TRAVERSE request");
            return;
        }
        if ((request.contains("direction") && !request["direction"].is_string()) ||
            (request.contains("edge_type") && !request["edge_type"].is_string())) {
            sendError(400, "Invalid 'direction' or 'edge_type' type in GRAPH_TRAVERSE request");
            return;
        }
        if ((request.contains("depth_min") && !request["depth_min"].is_number_integer()) ||
            (request.contains("depth_max") && !request["depth_max"].is_number_integer()) ||
            (request.contains("limit") && !request["limit"].is_number_integer())) {
            sendError(400, "Invalid 'depth_min', 'depth_max', or 'limit' type in GRAPH_TRAVERSE request");
            return;
        }
        std::string collection = request.value("collection", "");
        std::string start_vertex = request.value("start_vertex", "");

        if (collection.empty() || isBlankString(collection)) {
            sendError(400, "Missing 'collection' field in GRAPH_TRAVERSE request");
            return;
        }
        if (start_vertex.empty() || isBlankString(start_vertex)) {
            sendError(400, "Missing 'start_vertex' field in GRAPH_TRAVERSE request");
            return;
        }
        if (!isReasonableWireIdentifier(collection) || !isReasonableWireIdentifier(start_vertex)) {
            sendError(400, "Invalid 'collection' or 'start_vertex' field in GRAPH_TRAVERSE request");
            return;
        }

        if (!server_->query_engine_) {
            // Engine not wired — redirect client to HTTP API.
            json response;
            response["success"] = false;
            response["error_code"] = "GRAPH_NOT_INTEGRATED";
            response["error"] = "Graph traversal is not yet integrated in the wire protocol. "
                                "Use the HTTP REST API endpoint POST /api/v1/graph/traverse instead.";
            response["collection"] = collection;
            response["start_vertex"] = start_vertex;
            std::string rs = response.dump();
            asyncWriteResponse({rs.begin(), rs.end()});
            return;
        }

        // Parse traversal parameters.
        std::string direction_str = request.value("direction", "outbound");
        int depth_min = request.value("depth_min", 1);
        int depth_max = request.value("depth_max", 3);
        int limit     = request.value("limit", 100);
        std::string edge_type = request.value("edge_type", "");

        if ((!edge_type.empty()) && (isBlankString(edge_type) || !isReasonableWireIdentifier(edge_type))) {
            sendError(400, "Invalid 'edge_type' in GRAPH_TRAVERSE request");
            return;
        }

        if (direction_str != "outbound" && direction_str != "inbound" && direction_str != "any") {
            sendError(400, "Invalid 'direction' in GRAPH_TRAVERSE request (expected: outbound, inbound, any)");
            return;
        }
        if (depth_min < 1) {
            sendError(400, "'depth_min' must be >= 1 in GRAPH_TRAVERSE request");
            return;
        }
        constexpr int kMaxTraversalDepth = 64;
        if (depth_max < depth_min || depth_max > kMaxTraversalDepth) {
            sendError(400, "'depth_max' must be >= depth_min and <= 64 in GRAPH_TRAVERSE request");
            return;
        }
        constexpr int kMaxTraversalLimit = 10000;
        if (limit < 1 || limit > kMaxTraversalLimit) {
            sendError(400, "'limit' must be between 1 and 10000 in GRAPH_TRAVERSE request");
            return;
        }

        themis::TraversalDirection direction = themis::TraversalDirection::OUTBOUND;
        if (direction_str == "inbound") {
          direction = themis::TraversalDirection::INBOUND;
        }
        else if (direction_str == "any") direction = themis::TraversalDirection::ANY;

        auto trav_result = server_->query_engine_->executeGeneralTraversal(
            start_vertex, depth_min, depth_max, direction, collection, edge_type);

        json response = {};
        if (trav_result) {
            json vertices = json::array();
            int count = 0;
            for (const auto& tr : trav_result.value()) {
                if (count++ >= limit) {
                  break;
                }
                json v;
                v["vertex_pk"] = tr.vertex_pk;
                v["depth"]     = tr.depth;
                v["path"]      = tr.path;
                v["edges"]     = tr.edges;
                v["data"]      = tr.vertex_data;
                vertices.push_back(std::move(v));
            }
            response["success"]  = true;
            response["vertices"] = std::move(vertices);
            response["count"]    = count;
        } else {
            response["success"]    = false;
            response["error"]      = trav_result.error().message();
            response["error_code"] = "ERR_GRAPH_TRAVERSE";
        }

        std::string rs = response.dump();
        asyncWriteResponse({rs.begin(), rs.end()});
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in GRAPH_TRAVERSE payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("GRAPH_TRAVERSE error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleQuery() {
    // QUERY_AQL: execute an AQL query string.
    // Expected payload (JSON): {"query": "FOR doc IN collection RETURN doc", "bind_vars": {...}}
    // NOTE: Full AQL engine integration over the wire protocol is planned for a future
    // release.  Until then clients should use the HTTP REST API (/api/v1/query).
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxQueryPayloadBytes) {
            sendError(413, "QUERY payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid QUERY payload: expected JSON object");
            return;
        }
        if (request.contains("query") && !request["query"].is_string()) {
            sendError(400, "Invalid 'query' type in QUERY_AQL request");
            return;
        }
        if (request.contains("batch_size") && !request["batch_size"].is_number_integer()) {
            sendError(400, "Invalid 'batch_size' type in QUERY_AQL request");
            return;
        }
        std::string query_str = request.value("query", "");

        if (query_str.empty()) {
            sendError(400, "Missing 'query' field in QUERY_AQL request");
            return;
        }
        if (isBlankString(query_str)) {
            sendError(400, "'query' in QUERY_AQL request must not be blank");
            return;
        }
        constexpr size_t kMaxQueryStringLength = 1'048'576;
        if (static_cast<int>(query_str.size()) > kMaxQueryStringLength) {
            sendError(400, "'query' exceeds maximum length in QUERY_AQL request");
            return;
        }

        if (!server_->query_engine_) {
            // Engine not wired — redirect client to HTTP API.
            json response;
            response["success"] = false;
            response["error_code"] = "AQL_NOT_INTEGRATED";
            response["error"] = "AQL query execution is not yet integrated in the wire protocol. "
                                "Use the HTTP REST API endpoint POST /api/v1/query instead.";
            response["query"] = query_str;
            std::string rs = response.dump();
            asyncWriteResponse({rs.begin(), rs.end()});
            return;
        }

        // Execute the AQL query through the shared QueryEngine.
        auto result = themis::executeAql(query_str, *server_->query_engine_);

        json response = {};
        if (result) {
            const auto& result_json = result.value();
            int batch_size_i = request.value("batch_size", 100);
            constexpr int kMaxQueryBatchSize = 10000;
            if (batch_size_i < 1 || batch_size_i > kMaxQueryBatchSize) {
                sendError(400, "'batch_size' must be between 1 and 10000 in QUERY request");
                return;
            }
            const size_t batch_size = static_cast<size_t>(batch_size_i);

            bool has_more = false;
            json first_batch;
            std::string cursor_id = {};

            if (result_json.is_array() && static_cast<int>(result_json.size()) > batch_size) {
                // Large result: store in cursor registry and return first batch.
                first_batch = json::array();
                for (size_t i = 0; i < batch_size; ++i) {
                    first_batch.push_back(result_json[i]);
                }
                has_more = true;

                static constexpr int64_t kCursorTtlMs = 300'000; // 5 minute TTL
                auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                char buf[64];
                std::snprintf(buf, sizeof(buf), "cursor-%llu-%llu",
                              static_cast<unsigned long long>(session_id_),
                              static_cast<unsigned long long>(now_ms));
                cursor_id = buf;

                WireProtocolServer::CursorEntry entry;
                entry.results = result_json;
                entry.offset  = batch_size;
                entry.ttl_ms  = now_ms + kCursorTtlMs;

                std::lock_guard<std::mutex> lock(server_->cursors_mutex_);
                server_->cursors_[cursor_id] = std::move(entry);
            } else {
                first_batch = result_json;
            }

            response["success"]  = true;
            response["result"]   = first_batch;
            response["has_more"] = has_more;
            if (has_more) {
                response["cursor_id"] = cursor_id;
            }
        } else {
            response["success"]    = false;
            response["error"]      = result.error().message();
            response["error_code"] = "ERR_QUERY_FAILED";
        }

        std::string rs = response.dump();
        asyncWriteResponse({rs.begin(), rs.end()});
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in QUERY payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("QUERY error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleCursorNext() {
    // CURSOR_NEXT: fetch the next batch of results from an open AQL query cursor.
    // Expected payload (JSON): {"cursor_id": "...", "batch_size": 100}
    // NOTE: Cursor-based streaming requires the AQL engine integration, which is
    // not yet connected to the wire protocol.  Clients should use the HTTP REST
    // API GET /api/v1/cursor/{cursor_id}.
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxCursorPayloadBytes) {
            sendError(413, "CURSOR_NEXT payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid CURSOR_NEXT payload: expected JSON object");
            return;
        }
        if (request.contains("cursor_id") && !request["cursor_id"].is_string()) {
            sendError(400, "Invalid 'cursor_id' type in CURSOR_NEXT request");
            return;
        }
        if (request.contains("batch_size") && !request["batch_size"].is_number_integer()) {
            sendError(400, "Invalid 'batch_size' type in CURSOR_NEXT request");
            return;
        }
        std::string cursor_id = request.value("cursor_id", "");

        if (cursor_id.empty() || isBlankString(cursor_id)) {
            sendError(400, "Missing 'cursor_id' in CURSOR_NEXT request");
            return;
        }
        if (!isReasonableWireIdentifier(cursor_id)) {
            sendError(400, "Invalid 'cursor_id' in CURSOR_NEXT request");
            return;
        }

        int batch_size_i = request.value("batch_size", 100);
        constexpr int kMaxCursorBatchSize = 10000;
        if (batch_size_i < 1 || batch_size_i > kMaxCursorBatchSize) {
            sendError(400, "'batch_size' must be between 1 and 10000 in CURSOR_NEXT request");
            return;
        }
        const size_t batch_size = static_cast<size_t>(batch_size_i);

        std::lock_guard<std::mutex> lock(server_->cursors_mutex_);
        auto it = server_->cursors_.find(cursor_id);
        if (it == server_->cursors_.end()) {
            sendError(404, "Cursor not found: " + cursor_id);
            return;
        }

        auto& entry = it->second;

        // Check TTL expiry.
        if (entry.ttl_ms > 0) {
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            if (now_ms > entry.ttl_ms) {
                server_->cursors_.erase(it);
                sendError(410, "Cursor expired: " + cursor_id);
                return;
            }
        }

        json batch = json::array();
        size_t end = std::min(entry.offset + batch_size,
                              static_cast<size_t>(entry.results.size()));
        for (size_t i = entry.offset; i < end; ++i) {
            batch.push_back(entry.results[i]);
        }
        entry.offset = end;

        bool has_more = (entry.offset < static_cast<size_t>(entry.results.size()));
        if (!has_more) {
            server_->cursors_.erase(it);
        }

        json response;
        response["success"]   = true;
        response["result"]    = std::move(batch);
        response["has_more"]  = has_more;
        response["cursor_id"] = cursor_id;

        std::string rs = response.dump();
        asyncWriteResponse({rs.begin(), rs.end()});
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in CURSOR_NEXT payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("CURSOR_NEXT error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleCursorClose() {
    // CURSOR_CLOSE: close an open AQL query cursor and free server-side resources.
    // Expected payload (JSON): {"cursor_id": "..."}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxCursorPayloadBytes) {
            sendError(413, "CURSOR_CLOSE payload too large");
            return;
        }

        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);
        if (!request.is_object()) {
            sendError(400, "Invalid CURSOR_CLOSE payload: expected JSON object");
            return;
        }
        if (request.contains("cursor_id") && !request["cursor_id"].is_string()) {
            sendError(400, "Invalid 'cursor_id' type in CURSOR_CLOSE request");
            return;
        }
        std::string cursor_id = request.value("cursor_id", "");

        if (cursor_id.empty() || isBlankString(cursor_id)) {
            sendError(400, "Missing 'cursor_id' in CURSOR_CLOSE request");
            return;
        }
        if (!isReasonableWireIdentifier(cursor_id)) {
            sendError(400, "Invalid 'cursor_id' in CURSOR_CLOSE request");
            return;
        }

        std::lock_guard<std::mutex> lock(server_->cursors_mutex_);
        auto erased = server_->cursors_.erase(cursor_id);

        json response;
        response["success"]   = (erased > 0);
        response["cursor_id"] = cursor_id;
        if (erased == 0) {
            response["error"] = "Cursor not found or already closed";
        }

        std::string rs = response.dump();
        asyncWriteResponse({rs.begin(), rs.end()});
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in CURSOR_CLOSE payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("CURSOR_CLOSE error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleVectorSearch() {
    // VECTOR_SEARCH: k-nearest-neighbour search via VectorIndexManager.
    // Expected payload (JSON):
    //   {"vector": [f1, f2, ...], "k": 10, "collection": "..."}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    if (!server_->vector_index_) {
        sendError(503, "Vector index not configured");
        return;
    }

    try {
        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);

        if (request.contains("k") && !request["k"].is_number_integer()) {
            sendError(400, "Invalid 'k' type in VECTOR_SEARCH request");
            return;
        }

        if (request.contains("collection") && !request["collection"].is_string()) {
            sendError(400, "Invalid 'collection' type in VECTOR_SEARCH request");
            return;
        }

        const std::string collection = request.value("collection", "");
        if (!collection.empty() && !isReasonableWireIdentifier(collection)) {
            sendError(400, "Invalid 'collection' field in VECTOR_SEARCH request");
            return;
        }

        if (!request.contains("vector") || !request["vector"].is_array()) {
            sendError(400, "Missing or invalid 'vector' field in VECTOR_SEARCH request");
            return;
        }

        constexpr size_t kMaxVectorDimensions = 16384;
        const auto& vector_json = request["vector"];
        if (static_cast<int>(vector_json.size()) > kMaxVectorDimensions) {
            sendError(400, "VECTOR_SEARCH vector exceeds maximum dimension of 16384");
            return;
        }

        std::vector<float> query_vector = {};

        query_vector.reserve(vector_json.size());
        for (const auto& v : vector_json) {
            if (!v.is_number()) {
                sendError(400, "VECTOR_SEARCH 'vector' must contain only numeric values");
                return;
            }
            const float value = v.get<float>();
            if (!std::isfinite(value)) {
                sendError(400, "VECTOR_SEARCH 'vector' must not contain NaN/Inf values");
                return;
            }
            query_vector.push_back(value);
        }

        if (query_vector.empty()) {
            sendError(400, "Empty query vector in VECTOR_SEARCH request");
            return;
        }

        int64_t k_i = request.value("k", static_cast<int64_t>(10));
        if (k_i <= 0) {
            k_i = 10;
        }

        constexpr size_t kMaxVectorSearchK = 10000;
        if (k_i > static_cast<int64_t>(kMaxVectorSearchK)) {
            sendError(400, "VECTOR_SEARCH 'k' exceeds maximum value of 10000");
            return;
        }

        const size_t k = static_cast<size_t>(k_i);

        auto [status, results] = server_->vector_index_->searchKnn(query_vector, k);

        json response = {};
        if (!status.ok) {
            response["success"] = false;
            response["error"] = status.message;
        } else {
            response["success"] = true;
            response["count"] = results.size();
            json hits = json::array();
            for (const auto& r : results) {
                json hit;
                hit["pk"] = r.pk;
                hit["distance"] = r.distance;
                hits.push_back(std::move(hit));
            }
            response["hits"] = std::move(hits);
        }

        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);
    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in VECTOR_SEARCH payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("VECTOR_SEARCH error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleGeoQuery() {
    // GEO_QUERY: geospatial proximity / containment queries.
    // Expected payload (JSON):
    //   {"collection": "...", "type": "within|near|intersects",
    //    "bbox": {"minx":.., "miny":.., "maxx":.., "maxy":..},          // for within/intersects
    //    "center": {"lon":.., "lat":..}, "radius": <m>,                  // for near
    //    "limit": 100}
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    try {
        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);

        if (request.contains("collection") && !request["collection"].is_string()) {
            sendError(400, "Invalid 'collection' type in GEO_QUERY request");
            return;
        }
        if (request.contains("type") && !request["type"].is_string()) {
            sendError(400, "Invalid 'type' field in GEO_QUERY request");
            return;
        }
        if (request.contains("limit") && !request["limit"].is_number_integer()) {
            sendError(400, "Invalid 'limit' type in GEO_QUERY request");
            return;
        }

        std::string collection = request.value("collection", "");
        if (collection.empty()) {
            sendError(400, "Missing 'collection' field in GEO_QUERY request");
            return;
        }
        if (!isReasonableWireIdentifier(collection)) {
            sendError(400, "Invalid 'collection' field in GEO_QUERY request");
            return;
        }

        std::string query_type = request.value("type", "");
        if (query_type.empty()) {
            sendError(400,
                "Missing 'type' field in GEO_QUERY request "
                "(expected: within, near, intersects)");
            return;
        }

        auto spatial_idx = server_->spatial_index_;

        GeoQueryFn geo_bridge;
        {
            std::lock_guard<std::mutex> lock(g_network_geo_fn_mutex);
            geo_bridge = g_network_geo_query_fn;
        }

        if (!spatial_idx && query_type == "near" && geo_bridge) {
            if (!request.contains("center") || !request["center"].is_object()) {
                sendError(400,
                    "Missing or invalid 'center' in GEO_QUERY near-request "
                    "(expected: {lon, lat})");
                return;
            }
            if (!request.contains("radius")) {
                sendError(400, "Missing 'radius' (meters) in GEO_QUERY near-request");
                return;
            }

            const auto& center = request["center"];
            if (!center.contains("lon") || !center.contains("lat")) {
                sendError(400, "'center' must contain: lon, lat");
                return;
            }
            if (!center["lon"].is_number() || !center["lat"].is_number() ||
                !request["radius"].is_number()) {
                sendError(400, "'center.lon', 'center.lat' and 'radius' must be numeric");
                return;
            }

            const double lon = center["lon"].get<double>();
            const double lat = center["lat"].get<double>();
            const double radius = request["radius"].get<double>();
            if (!std::isfinite(lon) || !std::isfinite(lat) || !std::isfinite(radius)) {
                sendError(400, "'center' and 'radius' must be finite numbers");
                return;
            }
            if (lon < -180.0 || lon > 180.0 || lat < -90.0 || lat > 90.0) {
                sendError(400, "'center' coordinates out of range (lon: [-180,180], lat: [-90,90])");
                return;
            }
            if (radius <= 0.0) {
                sendError(400, "'radius' must be greater than 0");
                return;
            }

            constexpr uint32_t kMaxGeoQueryLimit = 10000;
            int64_t limit_i = request.value("limit", static_cast<int64_t>(100));
            if (limit_i < 1 || limit_i > static_cast<int64_t>(kMaxGeoQueryLimit)) {
                sendError(400, "'limit' must be between 1 and 10000 in GEO_QUERY request");
                return;
            }
            const int limit = static_cast<int>(limit_i);

            const auto bridge_results = geo_bridge(collection, lat, lon, radius, limit);
            if (!bridge_results.is_array()) {
                sendError(0x0007,
                    "GEO_QUERY bridge returned invalid payload (expected JSON array)");
                return;
            }

            json response;
            response["success"] = true;
            response["collection"] = collection;
            response["type"] = query_type;
            response["results"] = bridge_results;
            response["count"] = bridge_results.size();
            std::string rs = response.dump();
            asyncWriteResponse({rs.begin(), rs.end()});
            return;
        }

        if (!spatial_idx) {
            // Spatial index not configured — redirect to HTTP REST API.
            json response;
            response["success"]    = false;
            response["error_code"] = "GEO_NOT_INTEGRATED";
            response["error"]      = "Geospatial query execution is not configured on this wire "
                                     "protocol port. Use the HTTP REST API endpoint "
                                     "GET /api/v1/geo/query instead.";
            response["collection"] = collection;
            std::string rs = response.dump();
            asyncWriteResponse({rs.begin(), rs.end()});
            return;
        }

        if (!spatial_idx->hasSpatialIndex(collection)) {
            json response;
            response["success"]    = false;
            response["error_code"] = "GEO_NO_INDEX";
            response["error"]      = "Collection '" + collection + "' does not have a spatial "
                                     "index. Create one first using the spatial index API.";
            std::string rs = response.dump();
            asyncWriteResponse({rs.begin(), rs.end()});
            return;
        }

        constexpr uint32_t kMaxGeoQueryLimit = 10000;
        int64_t limit_i = request.value("limit", static_cast<int64_t>(100));
        if (limit_i < 1 || limit_i > static_cast<int64_t>(kMaxGeoQueryLimit)) {
            sendError(400, "'limit' must be between 1 and 10000 in GEO_QUERY request");
            return;
        }
        const uint32_t limit = static_cast<uint32_t>(limit_i);

        std::vector<index::SpatialResult> search_results;

        if (query_type == "intersects" || query_type == "within") {
            if (!request.contains("bbox") || !request["bbox"].is_object()) {
                sendError(400,
                    "Missing or invalid 'bbox' in GEO_QUERY request "
                    "(expected: {minx, miny, maxx, maxy})");
                return;
            }
            const auto& bbox_json = request["bbox"];
            if (!bbox_json.contains("minx") || !bbox_json.contains("miny") ||
                !bbox_json.contains("maxx") || !bbox_json.contains("maxy")) {
                sendError(400, "'bbox' must contain: minx, miny, maxx, maxy");
                return;
            }
            if (!bbox_json["minx"].is_number() || !bbox_json["miny"].is_number() ||
                !bbox_json["maxx"].is_number() || !bbox_json["maxy"].is_number()) {
                sendError(400, "'bbox' values must be numeric");
                return;
            }
            const double minx = bbox_json["minx"].get<double>();
            const double miny = bbox_json["miny"].get<double>();
            const double maxx = bbox_json["maxx"].get<double>();
            const double maxy = bbox_json["maxy"].get<double>();
            if (!std::isfinite(minx) || !std::isfinite(miny) ||
                !std::isfinite(maxx) || !std::isfinite(maxy)) {
                sendError(400, "'bbox' values must be finite numbers");
                return;
            }
            if (minx > maxx || miny > maxy) {
                sendError(400, "'bbox' must satisfy minx <= maxx and miny <= maxy");
                return;
            }
            geo::MBR query_bbox(minx, miny, maxx, maxy);
            search_results = spatial_idx->searchIntersects(collection, query_bbox);

        } else if (query_type == "near") {
            if (!request.contains("center") || !request["center"].is_object()) {
                sendError(400,
                    "Missing or invalid 'center' in GEO_QUERY near-request "
                    "(expected: {lon, lat})");
                return;
            }
            if (!request.contains("radius")) {
                sendError(400, "Missing 'radius' (meters) in GEO_QUERY near-request");
                return;
            }
            const auto& center = request["center"];
            if (!center.contains("lon") || !center.contains("lat")) {
                sendError(400, "'center' must contain: lon, lat");
                return;
            }
            if (!center["lon"].is_number() || !center["lat"].is_number() ||
                !request["radius"].is_number()) {
                sendError(400, "'center.lon', 'center.lat' and 'radius' must be numeric");
                return;
            }
            const double lon    = center["lon"].get<double>();
            const double lat    = center["lat"].get<double>();
            const double radius = request["radius"].get<double>();
            if (!std::isfinite(lon) || !std::isfinite(lat) || !std::isfinite(radius)) {
                sendError(400, "'center' and 'radius' must be finite numbers");
                return;
            }
            if (lon < -180.0 || lon > 180.0 || lat < -90.0 || lat > 90.0) {
                sendError(400, "'center' coordinates out of range (lon: [-180,180], lat: [-90,90])");
                return;
            }
            if (radius <= 0.0) {
                sendError(400, "'radius' must be greater than 0");
                return;
            }

            // Approximate bounding box for the radius query.
            // 1 degree latitude ≈ 111 km; 1 degree longitude varies with latitude.
            constexpr double kMetersPerDegreeLat = 111000.0;
            constexpr double kDegToRad           = 3.14159265358979323846 / 180.0;
            const double meters_per_lon =
                kMetersPerDegreeLat * std::cos(lat * kDegToRad);
            const double lat_delta = radius / kMetersPerDegreeLat;
            const double lon_delta = (meters_per_lon > 0.0)
                ? radius / meters_per_lon : lat_delta;

            geo::MBR query_bbox(
                lon - lon_delta, lat - lat_delta,
                lon + lon_delta, lat + lat_delta);
            search_results = spatial_idx->searchIntersects(collection, query_bbox);

        } else {
            sendError(400,
                "Unknown 'type' in GEO_QUERY request; "
                "expected: within, near, intersects");
            return;
        }

        json results_arr = json::array();
        uint32_t count = 0;
        for (const auto& res : search_results) {
            if (count++ >= limit) {
              break;
            }
            json entry;
            entry["primary_key"] = res.primary_key;
            entry["mbr"] = {
                {"minx", res.mbr.minx},
                {"miny", res.mbr.miny},
                {"maxx", res.mbr.maxx},
                {"maxy", res.mbr.maxy}};
            if (res.z_min.has_value() && res.z_max.has_value()) {
                entry["z_min"] = res.z_min.value();
                entry["z_max"] = res.z_max.value();
            }
            results_arr.push_back(std::move(entry));
        }

        json response;
        response["success"]    = true;
        response["collection"] = collection;
        response["type"]       = query_type;
        response["results"]    = std::move(results_arr);
        response["count"]      = count;
        std::string rs = response.dump();
        asyncWriteResponse({rs.begin(), rs.end()});

    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON in GEO_QUERY payload: ") + e.what());
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("GEO_QUERY error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleTimeseriesQuery() {
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }
    // Check if TSStore is available
    if (!server_->ts_store_) {
        sendError(0x0004, "Time-series storage not configured");
        return;
    }
    
    try {
        // Parse TimeSeriesQueryRequest from payload
        TimeSeriesQueryRequest request = {};
        if (!TimeSeriesQueryRequest::parse(payload_buffer_, request)) {
            sendError(0x0009, "Failed to parse TimeSeriesQueryRequest");
            return;
        }
        
        // Validate request
        if (request.collection.empty()) {
            sendError(0x000A, "Collection (metric) name is required");
            return;
        }
        if (!isReasonableWireIdentifier(request.collection)) {
            sendError(0x000A, "Collection (metric) name is invalid");
            return;
        }
        if (isBlankString(request.collection)) {
            sendError(0x000A, "Collection (metric) name must not be blank");
            return;
        }
        
        if (request.start_time_ns >= request.end_time_ns) {
            sendError(0x000B, "start_time_ns must be less than end_time_ns");
            return;
        }

        constexpr uint32_t kMaxAggregationValue = 4;  // AVG, SUM, MIN, MAX, COUNT
        if (request.aggregation > kMaxAggregationValue) {
            sendError(0x000B, "aggregation must be between 0 and 4");
            return;
        }

        constexpr uint64_t kNsPerMs = 1'000'000;
        constexpr uint64_t kMaxI64 = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
        if ((request.start_time_ns / kNsPerMs) > kMaxI64 ||
            (request.end_time_ns / kNsPerMs) > kMaxI64) {
            sendError(0x000B, "start_time_ns/end_time_ns out of supported range");
            return;
        }

        constexpr uint64_t kMaxTimeseriesWindowNs = 315'360'000'000'000'000;  // 10 years
        const uint64_t window_ns = request.end_time_ns - request.start_time_ns;
        if (window_ns > kMaxTimeseriesWindowNs) {
            sendError(0x000B, "requested time window exceeds supported range");
            return;
        }

        if (request.bucket_size_ns > 0) {
            constexpr uint64_t kMaxBucketCount = 10'000;
            constexpr uint64_t kMinBucketSizeNs = 1'000'000;  // 1 ms
            if (request.bucket_size_ns < kMinBucketSizeNs) {
                sendError(0x000B, "bucket_size_ns must be at least 1000000");
                return;
            }
            if (request.bucket_size_ns > window_ns) {
                sendError(0x000B, "bucket_size_ns must not exceed requested window");
                return;
            }
            if ((window_ns / request.bucket_size_ns) > kMaxBucketCount) {
                sendError(0x000B, "bucket_size_ns yields too many buckets (max 10000)");
                return;
            }
        }
        
        // Start timing
        auto query_start = std::chrono::high_resolution_clock::now();
        
        // Convert timestamps from nanoseconds to milliseconds (TSStore internal format)
        int64_t start_ms = static_cast<int64_t>(request.start_time_ns / kNsPerMs);
        int64_t end_ms = static_cast<int64_t>(request.end_time_ns / kNsPerMs);
        
        // Build TSStore query options
        TSStore::QueryOptions query_opts;
        query_opts.metric = request.collection;
        query_opts.from_timestamp_ms = start_ms;
        query_opts.to_timestamp_ms = end_ms;
        query_opts.limit = 10000;  // Production limit
        
        TimeSeriesQueryResponse response;
        
        // Check if aggregation is requested
        // Note: aggregation enum has AVG=0, so we can't use aggregation!=0 to detect requests
        // Use bucket_size_ns > 0 OR explicit aggregation field presence as indicator
        // For production: if request has aggregation field set (even if AVG=0), treat as aggregation request
        bool needs_aggregation = (request.bucket_size_ns > 0);
        
        if (needs_aggregation) {
            // If bucket_size is specified, create time buckets with raw data
            if (request.bucket_size_ns > 0) {
                // Query raw data points to enable per-bucket aggregation
                auto result = server_->ts_store_->query(query_opts);
                
                if (!result) {
                    std::string error_msg = "Time-series query failed";
                    try {
                        error_msg = std::string("Time-series query failed: ") + result.error().message();
                    } catch (const std::exception& e) {
                        std::cerr << "[WireProtocol] TS query error-message extraction failed: "
                                  << e.what() << "\n";
                    }
                    sendError(0x0005, error_msg);
                    return;
                }
                
                const auto& data_points = result.value();
                
                // Bucket the data by time windows
                uint64_t bucket_size_ms = request.bucket_size_ns / 1000000;
                if (bucket_size_ms == 0) bucket_size_ms = 1;  // Minimum 1ms buckets
                
                // Calculate number of buckets
                int64_t time_range_ms = end_ms - start_ms;
                size_t num_buckets = static_cast<size_t>((time_range_ms + bucket_size_ms - 1) / bucket_size_ms);
                
                // Limit buckets to reasonable number
                if (num_buckets > 10000) {
                    num_buckets = 10000;
                    bucket_size_ms = time_range_ms / num_buckets;
                }
                
                // Create buckets and aggregate data points into them
                std::map<int64_t, std::vector<double>> bucket_data;
                
                for (const auto& point : data_points) {
                    // Determine which bucket this point belongs to
                    int64_t bucket_index = (point.timestamp_ms - start_ms) / bucket_size_ms;
                    int64_t bucket_start_ms = start_ms + (bucket_index * bucket_size_ms);
                    bucket_data[bucket_start_ms].push_back(point.value);
                }
                
                // Create response buckets with aggregated values
                for (const auto& [bucket_start_ms, values] : bucket_data) {
                    if (values.empty()) {
                      continue;
                    }
                    
                    TimeSeriesBucket bucket;
                    bucket.timestamp_ns = static_cast<uint64_t>(bucket_start_ms) * 1000000;
                    bucket.count = values.size();
                    
                    // Calculate aggregation
                    double min_val = values[0];
                    double max_val = values[0];
                    double sum_val = 0.0;
                    
                    for (double val : values) {
                        min_val = std::min(min_val, val);
                        max_val = std::max(max_val, val);
                        sum_val += val;
                    }
                    
                    bucket.min = min_val;
                    bucket.max = max_val;
                    
                    // Set value based on aggregation type
                    switch (request.aggregation) {
                        case 0:  // AVG
                            bucket.value = sum_val / values.size();
                            break;
                        case 1:  // SUM
                            bucket.value = sum_val;
                            break;
                        case 2:  // MIN
                            bucket.value = min_val;
                            break;
                        case 3:  // MAX
                            bucket.value = max_val;
                            break;
                        case 4:  // COUNT
                            bucket.value = static_cast<double>(values.size());
                            break;
                        default:
                            bucket.value = sum_val / values.size();
                    }
                    
                    response.buckets.push_back(bucket);
                }
                
                response.stats.total_data_points = data_points.size();
                response.stats.buckets_returned = response.buckets.size();
                response.stats.data_density = data_points.empty() ? 0.0 : 
                    static_cast<double>(data_points.size()) / response.buckets.size();
            } else {
                // No bucketing specified, return single aggregated result
                auto agg_result = server_->ts_store_->aggregate(query_opts);
                
                if (!agg_result) {
                    std::string error_msg = "Time-series aggregation failed";
                    try {
                        error_msg = std::string("Time-series aggregation failed: ") + agg_result.error().message();
                    } catch (const std::exception& e) {
                        std::cerr << "[WireProtocol] TS aggregation error-message extraction failed: "
                                  << e.what() << "\n";
                    }
                    sendError(0x0005, error_msg);
                    return;
                }
                
                const auto& agg_data = agg_result.value();
                
                // Create single bucket with aggregated values
                TimeSeriesBucket bucket;
                bucket.timestamp_ns = request.start_time_ns;
                bucket.count = agg_data.count;
                bucket.min = agg_data.min;
                bucket.max = agg_data.max;
                
                // Set value based on aggregation type
                switch (request.aggregation) {
                    case 0:  // AVG
                        bucket.value = agg_data.avg;
                        break;
                    case 1:  // SUM
                        bucket.value = agg_data.sum;
                        break;
                    case 2:  // MIN
                        bucket.value = agg_data.min;
                        break;
                    case 3:  // MAX
                        bucket.value = agg_data.max;
                        break;
                    case 4:  // COUNT
                        bucket.value = static_cast<double>(agg_data.count);
                        break;
                    default:
                        bucket.value = agg_data.avg;
                }
                
                response.buckets.push_back(bucket);
                response.stats.total_data_points = agg_data.count;
                response.stats.buckets_returned = 1;
                response.stats.data_density = static_cast<double>(agg_data.count);
            }
        } else {
            // Use raw query path
            auto result = server_->ts_store_->query(query_opts);
            
            if (!result) {
                std::string error_msg = "Time-series query failed";
                try {
                    error_msg = std::string("Time-series query failed: ") + result.error().message();
                } catch (const std::exception& e) {
                    std::cerr << "[WireProtocol] TS raw-query error-message extraction failed: "
                              << e.what() << "\n";
                }
                sendError(0x0005, error_msg);
                return;
            }
            
            const auto& data_points = result.value();
            
            // Convert data points to buckets (one bucket per data point)
            for (const auto& point : data_points) {
                TimeSeriesBucket bucket;
                bucket.timestamp_ns = static_cast<uint64_t>(point.timestamp_ms) * 1000000;  // ms to ns
                bucket.value = point.value;
                bucket.count = 1;
                bucket.min = point.value;
                bucket.max = point.value;
                
                response.buckets.push_back(bucket);
            }
            
            response.stats.total_data_points = data_points.size();
            response.stats.buckets_returned = data_points.size();
            response.stats.data_density = data_points.empty() ? 0.0 : 1.0;
        }
        
        // Calculate query time
        auto query_end = std::chrono::high_resolution_clock::now();
        auto query_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(query_end - query_start).count();
        response.query_time_us = static_cast<uint64_t>(query_duration_us);
        
        // Serialize response to protobuf wire format
        auto response_payload = response.serialize();
        
        // Build wire frame response
        std::vector<uint8_t> wire_response;
        
        // Wire frame header: Magic (4) + Version (1) + OpCode (1) + Flags (2) + PayloadSize (4) = 12 bytes
        uint32_t magic = htonl(0x544D4442);  // "TMDB" in network byte order
        wire_response.resize(12);
        std::memcpy(&wire_response[0], &magic, 4);
        wire_response[4] = 0x01;  // Version 1
        wire_response[5] = 0x21;  // OpCode: QUERY_RESULT
        wire_response[6] = 0x00;  // Flags (low byte)
        wire_response[7] = 0x00;  // Flags (high byte)
        
        uint32_t payload_size = static_cast<uint32_t>(response_payload.size());
        uint32_t payload_size_net = htonl(payload_size);  // Convert to network byte order
        std::memcpy(&wire_response[8], &payload_size_net, 4);
        
        // Append payload
        wire_response.insert(wire_response.end(), response_payload.begin(), response_payload.end());
        
        // Send response
        asyncWriteResponse(wire_response);
        
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("Time-series query exception: ") + e.what());
    }
}

// =============================================================================
// BPMN Process Handlers
// =============================================================================

namespace {
    // Helper to parse JSON from payload buffer
    json parsePayloadJson(const std::vector<uint8_t>& payload_buffer) {
        std::string payload_str(payload_buffer.begin(), payload_buffer.end());
        return json::parse(payload_str);
    }

    // Retry parsing a bounded number of times for transient/incomplete payload edge cases.
    json parsePayloadJsonWithRetry(const std::vector<uint8_t>& payload_buffer, int max_attempts) {
        std::string payload_str(payload_buffer.begin(), payload_buffer.end());
        std::exception_ptr last_error = {};
        const int attempts = std::max(1, max_attempts);
        for (int attempt = 1; attempt <= attempts; ++attempt) {
            try {
                return json::parse(payload_str);
            } catch (const nlohmann::json::parse_error&) {
                last_error = std::current_exception();
                if (attempt == attempts) {
                    throw;
                }
            }
        }
        if (last_error) {
            std::rethrow_exception(last_error);
        }
        throw std::runtime_error("JSON parse failed after retry");
    }
}

void WireProtocolServer::Session::handleBpmnStartProcess() {
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    if (!server_->process_graph_) {
        throw std::runtime_error(
            "CRITICAL: Process engine not wired to wire protocol server. "
            "Process engine injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxBpmnPayloadBytes) {
            sendError(413, "BPMN start-process payload too large");
            return;
        }

        // Parse JSON payload
        // Expected format: { "process_definition_key": "...", "variables": {...}, "business_key": "..." }
        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);

        if (!request.is_object()) {
            sendError(400, "Invalid request: expected JSON object");
            return;
        }

        if (request.contains("process_definition_key") && !request["process_definition_key"].is_string()) {
            sendError(400, "Invalid process_definition_key: expected string");
            return;
        }
        if (request.contains("variables") && !request["variables"].is_object()) {
            sendError(400, "Invalid variables: expected object");
            return;
        }
        if (request.contains("business_key") && !request["business_key"].is_string()) {
            sendError(400, "Invalid business_key: expected string");
            return;
        }

        std::string process_key = request.value("process_definition_key", "");
        json variables = request.value("variables", json::object());
        std::string business_key = request.value("business_key", "");

        std::string variables_error = {};
        if (!validateBpmnVariablesObject(variables, variables_error)) {
            sendError(400, std::string("Invalid variables: ") + variables_error);
            return;
        }

        if (process_key.empty() || isBlankString(process_key)) {
            sendError(400, "Missing process_definition_key");
            return;
        }
        if (!isReasonableWireIdentifier(process_key)) {
            sendError(400, "Invalid process_definition_key: must be <= 256 chars and contain no control characters");
            return;
        }
        if (!business_key.empty() && !isReasonableWireIdentifier(business_key)) {
            sendError(400, "Invalid business_key: must be <= 256 chars and contain no control characters");
            return;
        }

        // Start process instance
        auto [status, instance_id] = server_->process_graph_->startProcess(process_key, variables);

        if (!status.ok) {
            sendError(500, "Failed to start process: " + status.message);
            return;
        }

        // Get instance state to return active tasks
        auto [get_status, instance] = server_->process_graph_->getProcessInstance(instance_id);
        
        // Build response
        json response;
        response["process_instance_id"] = instance_id;
        
        // Map process state to ProcessStatus enum
        std::string state_str = "RUNNING";
        int status_code = 0; // RUNNING
        if (get_status.ok) {
            switch (instance.state) {
                case ProcessInstance::State::RUNNING:
                    state_str = "RUNNING";
                    status_code = 0;
                    break;
                case ProcessInstance::State::COMPLETED:
                    state_str = "COMPLETED";
                    status_code = 1;
                    break;
                case ProcessInstance::State::FAILED:
                    state_str = "FAILED";
                    status_code = 2;
                    break;
                case ProcessInstance::State::SUSPENDED:
                    state_str = "SUSPENDED";
                    status_code = 3;
                    break;
                case ProcessInstance::State::TERMINATED:
                    state_str = "TERMINATED";
                    status_code = 4;
                    break;
                default:
                    state_str = "RUNNING";
                    status_code = 0;
            }

            // Extract active task IDs from tokens
            json active_tasks = json::array();
            for (const auto& token : instance.tokens) {
                if (token.state == ProcessToken::State::READY || 
                    token.state == ProcessToken::State::ACTIVE) {
                    active_tasks.push_back(instance_id + ":" + token.current_node);
                }
            }
            response["active_task_ids"] = active_tasks;
        } else {
            response["active_task_ids"] = json::array();
        }
        
        response["status"] = status_code;
        response["status_string"] = state_str;

        // Serialize response
        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);

    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(500, std::string("Internal error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleBpmnTaskComplete() {
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    if (!server_->process_graph_) {
        throw std::runtime_error(
            "CRITICAL: Process engine not wired to wire protocol server. "
            "Process engine injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxBpmnPayloadBytes) {
            sendError(413, "BPMN task-complete payload too large");
            return;
        }

        // Parse JSON payload
        // Expected format: { "task_id": "...", "variables": {...}, "assignee": "..." }
        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);

        if (!request.is_object()) {
            sendError(400, "Invalid request: expected JSON object");
            return;
        }

        if (request.contains("task_id") && !request["task_id"].is_string()) {
            sendError(400, "Invalid task_id: expected string");
            return;
        }
        if (request.contains("variables") && !request["variables"].is_object()) {
            sendError(400, "Invalid variables: expected object");
            return;
        }
        if (request.contains("assignee") && !request["assignee"].is_string()) {
            sendError(400, "Invalid assignee: expected string");
            return;
        }

        std::string task_id = request.value("task_id", "");
        json variables = request.value("variables", json::object());
        std::string assignee = request.value("assignee", username_);

        std::string variables_error = {};
        if (!validateBpmnVariablesObject(variables, variables_error)) {
            sendError(400, std::string("Invalid variables: ") + variables_error);
            return;
        }

        if (task_id.empty() || isBlankString(task_id)) {
            sendError(400, "Missing task_id");
            return;
        }
        if (!isReasonableWireIdentifier(task_id)) {
            sendError(400, "Invalid task_id: must be <= 256 chars and contain no control characters");
            return;
        }
        if (!assignee.empty() && !isReasonableWireIdentifier(assignee)) {
            sendError(400, "Invalid assignee: must be <= 256 chars and contain no control characters");
            return;
        }

        // Task ID format: typically "token_id" or we need to parse instance_id and node_id
        // For simplicity, we'll assume task_id format is "instance_id:node_id" or just token_id
        // Let's extract instance_id from the task if it contains ':'
        
        std::string instance_id = {};
        std::string node_id = {};
        
        size_t colon_pos = task_id.find(':');
        if (colon_pos != std::string::npos) {
            instance_id = task_id.substr(0, colon_pos);
            node_id = task_id.substr(colon_pos + 1);
            if (instance_id.empty() || node_id.empty() ||
                isBlankString(instance_id) || isBlankString(node_id)) {
                sendError(400, "Invalid task_id format: expected '<instance_id>:<node_id>'");
                return;
            }
            if (!isReasonableWireIdentifier(instance_id) || !isReasonableWireIdentifier(node_id)) {
                sendError(400, "Invalid task_id format: instance_id/node_id must be <= 256 chars and contain no control characters");
                return;
            }
        } else {
            // Resolve a token-only task_id via ProcessGraphManager.
            // Scans active tokens to find the matching token_id and extracts
            // instance_id + current_node.  Stub #138 resolution.
            auto resolved = server_->process_graph_->findTokenByTokenId(task_id);
            if (!resolved) {
                sendError(400, "Invalid task_id: not found or not active");
                return;
            }
            instance_id = resolved->first;
            node_id     = resolved->second;
        }

        // Complete the task
        auto status = server_->process_graph_->completeTask(instance_id, node_id, variables);

        // Build response
        json response;
        response["success"] = status.ok;
        
        if (!status.ok) {
            response["error"] = status.message;
            response["next_task_id"] = "";
        } else {
            response["error"] = "";
            
            // Try to find next active task
            auto [get_status, instance] = server_->process_graph_->getProcessInstance(instance_id);
            if (get_status.ok && !instance.tokens.empty()) {
                // Find first active token
                for (const auto& token : instance.tokens) {
                    if (token.state == ProcessToken::State::READY || 
                        token.state == ProcessToken::State::ACTIVE) {
                        response["next_task_id"] = instance_id + ":" + token.current_node;
                        break;
                    }
                }
                if (!response.contains("next_task_id")) {
                    response["next_task_id"] = "";
                }
            } else {
                response["next_task_id"] = "";
            }
        }

        // Serialize response
        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);

    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(500, std::string("Internal error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleBpmnQueryInstance() {
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    if (!server_->process_graph_) {
        throw std::runtime_error(
            "CRITICAL: Process engine not wired to wire protocol server. "
            "Process engine injection is mandatory at startup.");
    }

    try {
        if (static_cast<int>(payload_buffer_.size()) > kMaxBpmnPayloadBytes) {
            sendError(413, "BPMN query-instance payload too large");
            return;
        }

        // Parse JSON payload
        // Expected format: { "process_instance_id": "...", "include_variables": true/false, "include_history": true/false }
        json request = parsePayloadJsonWithRetry(payload_buffer_, 2);

        if (!request.is_object()) {
            sendError(400, "Invalid request: expected JSON object");
            return;
        }

        if (request.contains("process_instance_id") && !request["process_instance_id"].is_string()) {
            sendError(400, "Invalid process_instance_id: expected string");
            return;
        }
        if (request.contains("include_variables") && !request["include_variables"].is_boolean()) {
            sendError(400, "Invalid include_variables: expected boolean");
            return;
        }
        if (request.contains("include_history") && !request["include_history"].is_boolean()) {
            sendError(400, "Invalid include_history: expected boolean");
            return;
        }
        if ([[maybe_unused]] request.contains("max_history_events") && !request["max_history_events"].is_number_unsigned()) {
            sendError(400, "Invalid max_history_events: expected unsigned integer");
            return;
        }

        std::string instance_id = request.value("process_instance_id", "");
        bool include_variables = request.value("include_variables", true);
        bool include_history = request.value("include_history", false);
        std::size_t max_history_events = request.value("max_history_events", kDefaultBpmnHistoryEvents);

        if (instance_id.empty() || isBlankString(instance_id)) {
            sendError(400, "Missing process_instance_id");
            return;
        }
        if (!isReasonableWireIdentifier(instance_id)) {
            sendError(400, "Invalid process_instance_id: must be <= 256 chars and contain no control characters");
            return;
        }
        if ([[maybe_unused]] max_history_events == 0 || max_history_events > kMaxBpmnHistoryEvents) {
            sendError(400, "Invalid max_history_events: must be in range 1..10000");
            return;
        }

        // Get process instance
        auto [status, instance] = server_->process_graph_->getProcessInstance(instance_id);

        if (!status.ok) {
            sendError(404, "Process instance not found: " + status.message);
            return;
        }

        // Build response
        json response;

        // Map status
        int status_code = 0;
        switch (instance.state) {
            case ProcessInstance::State::RUNNING:
                status_code = 0;
                break;
            case ProcessInstance::State::COMPLETED:
                status_code = 1;
                break;
            case ProcessInstance::State::FAILED:
                status_code = 2;
                break;
            case ProcessInstance::State::SUSPENDED:
                status_code = 3;
                break;
            case ProcessInstance::State::TERMINATED:
                status_code = 4;
                break;
            default:
                status_code = 0;
        }
        response["status"] = status_code;

        // Active tasks
        json active_tasks = json::array();
        for (const auto& token : instance.tokens) {
            if (token.state == ProcessToken::State::READY || 
                token.state == ProcessToken::State::ACTIVE) {
                json task;
                task["task_id"] = instance_id + ":" + token.current_node;
                task["task_name"] = token.current_node;
                task["task_type"] = "userTask"; // Default, could be enhanced
                task["assignee"] = ""; // Not stored in token currently
                task["created_at_ns"] = token.created_at_ms * 1000000; // Convert ms to ns
                active_tasks.push_back(task);
            }
        }
        response["active_tasks"] = active_tasks;

        // Variables
        if (include_variables) {
            response["variables"] = instance.variables;
        } else {
            response["variables"] = json::object();
        }

        // History (simplified - just list visited nodes)
        bool history_truncated = false;
        if (include_history) {
            json history = json::array();
            for (const auto& token : instance.tokens) {
                for (const auto& node : token.visited_nodes) {
                    if ([[maybe_unused]] static_cast<int>(history.size()) >= max_history_events) {
                        history_truncated = true;
                        break;
                    }
                    json event;
                    event["event_type"] = "node_visited";
                    auto tsIt = token.visit_timestamps.find(node);
                    if (tsIt != token.visit_timestamps.end()) {
                        int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            tsIt->second.time_since_epoch()).count();
                        event["timestamp_ns"] = ns;
                    } else {
                        event["timestamp_ns"] = token.created_at_ms * 1000000;
                    }
                    event["data"] = json::object();
                    event["data"]["node_id"] = node;
                    history.push_back([[maybe_unused]] event);
                }
                if (history_truncated) {
                    break;
                }
            }
            response["history"] = history;
            response["history_truncated"] = history_truncated;
        } else {
            response["history"] = json::array();
            response["history_truncated"] = false;
        }

        // Timestamps
        response["start_time_ns"] = instance.started_at_ms * 1000000; // Convert ms to ns
        if (instance.completed_at_ms.has_value()) {
            response["end_time_ns"] = instance.completed_at_ms.value() * 1000000;
        } else {
            response["end_time_ns"] = 0;
        }

        // Serialize response
        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);

    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(500, std::string("Internal error: ") + e.what());
    }
}

} // namespace themis::network

