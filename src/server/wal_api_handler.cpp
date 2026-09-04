/**
 * @file wal_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/wal_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "sharding/wal_applier.h"
#include "sharding/wal_manager.h"
#include "sharding/replication_coordinator.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/zstd_codec.h"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <chrono>

namespace themis {
namespace server {

using themis::AuthMiddleware;

WALApiHandler::WALApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<sharding::WALApplier> wal_applier,
    std::shared_ptr<sharding::WALManager> wal_manager,
    std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator,
    std::shared_ptr<AuthMiddleware> auth,
    const std::string& wal_shared_secret,
    const std::string& wal_hmac_secret
)
    : storage_(std::move(storage))
    , wal_applier_(std::move(wal_applier))
    , wal_manager_(std::move(wal_manager))
    , replication_coordinator_(std::move(replication_coordinator))
    , auth_(std::move(auth))
    , wal_shared_secret_(wal_shared_secret)
    , wal_hmac_secret_(wal_hmac_secret)
{
}

http::response<http::string_body> WALApiHandler::handleApply(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleApply");
    // HMAC/shared-secret auth (optional)
    if (!wal_shared_secret_.empty()) {
        auto hdr = req.find("X-WAL-Auth");
        if (hdr == req.end() || hdr->value() != wal_shared_secret_) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
    }

    if (!wal_hmac_secret_.empty()) {
        auto hdr = req.find("X-WAL-HMAC");
        if (hdr == req.end()) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        std::string expected = hmacSha256Hex(wal_hmac_secret_, req.body());
        if (expected.empty() || !timingSafeEqual(expected, std::string(hdr->value()))) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
    }

    auto apply_start = std::chrono::steady_clock::now();

    if (!wal_applier_) {
        return makeErrorResponse(http::status::service_unavailable, "WAL applier not configured", req);
    }
    auto& wal_applier = *wal_applier_;

    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(req.body());
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("Invalid JSON: ") + e.what(), req);
    }

    // Accept either raw entries or compressed payload from shipper
    nlohmann::json entries_json;
    if (payload.contains("entries") && payload["entries"].is_array()) {
        entries_json = payload["entries"];
    } else if (payload.contains("entries_compressed")) {
        // entries_compressed is stored as binary in JSON (nlohmann::json::binary)
        try {
            const auto& bin = payload["entries_compressed"].get_binary();
            std::vector<uint8_t> compressed(bin.begin(), bin.end());
            auto decompressed = utils::zstd_decompress(compressed);
            entries_json = nlohmann::json::parse(decompressed.begin(), decompressed.end());
        } catch (const std::exception& e) {
            return makeErrorResponse(http::status::bad_request, 
                std::string("Failed to decode compressed entries: ") + e.what(), req);
        }
    } else {
        return makeErrorResponse(http::status::bad_request, 
            "Missing 'entries' array (or entries_compressed)", req);
    }

    std::vector<sharding::WALEntry> entries = {};

    entries.reserve(entries_json.size());

    try {
        for (const auto& item : entries_json) {
            sharding::WALEntry e;
            if (item.contains("lsn")) {
                e.lsn = sharding::LSN::fromString(item.value("lsn", std::string("0/0")));
            }
            e.type = static_cast<sharding::WALEntryType>(item.value("type", 0));
            e.timestamp = item.value("timestamp", uint64_t(0));
            e.transaction_id = item.value("transaction_id", std::string());
            if (item.contains("data")) {
                e.data = item["data"]; // assumes valid JSON object/array
            }
            entries.push_back(std::move(e));
        }
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::bad_request, 
            std::string("Invalid WAL entry: ") + e.what(), req);
    }

    auto result = wal_applier.applyBatch(entries);
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - apply_start).count();
    recordLatency(elapsed_us);
    
    if (!result.success) {
        wal_apply_fail_.fetch_add(1, std::memory_order_relaxed);
        nlohmann::json body = {
            {"error", true},
            {"message", "Apply failed"},
            {"errors", result.errors},
            {"status_code", static_cast<int>(http::status::internal_server_error)}
        };
        return makeResponse(http::status::internal_server_error, body.dump(), req);
    }

    wal_apply_success_.fetch_add(1, std::memory_order_relaxed);
    {
        std::unique_lock<std::shared_mutex> lock(wal_metrics_mutex_);
        wal_last_applied_lsn_ = result.last_applied_lsn.toString();
    }

    nlohmann::json body = {
        {"success", true},
        {"entries_applied", result.entries_applied},
        {"last_applied_lsn", result.last_applied_lsn.toString()}
    };
    return makeResponse(http::status::ok, body.dump(), req);
}

http::response<http::string_body> WALApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> WALApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

void WALApiHandler::recordLatency([[maybe_unused]] int64_t elapsed_us) {
    wal_apply_latency_sum_us_.fetch_add(static_cast<uint64_t>(elapsed_us), std::memory_order_relaxed);
    wal_apply_latency_count_.fetch_add(1, std::memory_order_relaxed);
    if (elapsed_us <= 50'000) {
        wal_apply_latency_le_50ms_.fetch_add(1, std::memory_order_relaxed);
    } else if (elapsed_us <= 200'000) {
        wal_apply_latency_le_200ms_.fetch_add(1, std::memory_order_relaxed);
    } else if (elapsed_us <= 1'000'000) {
        wal_apply_latency_le_1000ms_.fetch_add(1, std::memory_order_relaxed);
    } else {
        wal_apply_latency_gt_1000ms_.fetch_add(1, std::memory_order_relaxed);
    }
}

std::string WALApiHandler::hmacSha256Hex(const std::string& key, const std::string& data) {
    unsigned int len = 0;
    unsigned char* result = HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
                                 reinterpret_cast<const unsigned char*>(data.data()), data.size(), nullptr, &len);
    if (!result || len == 0) {
        return {};
    }
    static constexpr char hex_digits[] = "0123456789abcdef";
    std::string hex = {};
    hex.reserve(len * 2);
    for (unsigned int i = 0; i < len; ++i) {
        hex.push_back(hex_digits[(result[i] >> 4) & 0x0F]);
        hex.push_back(hex_digits[result[i] & 0x0F]);
    }
    return hex;
}

bool WALApiHandler::timingSafeEqual(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
      return false;
    }
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return diff == 0;
}

} // namespace server
} // namespace themis
