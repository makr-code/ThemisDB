/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ws_handler.cpp                                     ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:48:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     184                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 899b3070e4  2026-03-14  fix(api/ws): clarify url_decode comment, document semanti... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "api/ws_handler.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"

#include <charconv>
#include <cctype>
#include <string_view>

namespace themis {
namespace api {

namespace http = boost::beast::http;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

WsChangeHandler::WsChangeHandler(std::shared_ptr<AuthMiddleware> auth,
                                 Changefeed* changefeed)
    : auth_(std::move(auth))
    , changefeed_(changefeed)
{}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

bool WsChangeHandler::isChangeStreamPath(std::string_view path) {
    return path == "/v2/changes" || path == "/v2/cdc/stream";
}

/// Decode a percent-encoded URL path/query component per RFC 3986.
/// Only %XX sequences are expanded; '+' is left as a literal '+' (this is a
/// raw URL query string, not application/x-www-form-urlencoded).
/// Malformed sequences (e.g. bare '%', truncated '%3', non-hex '%ZZ') pass
/// through unchanged.
/// Note: HTTP form-param decoding in http_type_adapter.cpp intentionally uses
/// different semantics ('+' → ' ') for application/x-www-form-urlencoded bodies.
static std::string url_decode(const std::string& encoded) {
    std::string result;
    result.reserve(encoded.size());
    for (std::size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size() &&
            std::isxdigit(static_cast<unsigned char>(encoded[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(encoded[i + 2])))
        {
            unsigned int val = 0;
            std::from_chars(&encoded[i + 1], &encoded[i + 3], val, 16);
            result += static_cast<char>(val);
            i += 2;
        } else {
            result += encoded[i];
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// validate()
// ---------------------------------------------------------------------------

WsChangeHandler::UpgradeDecision
WsChangeHandler::validate(const http::request<http::string_body>& req) const
{
    UpgradeDecision decision;

    // ── 1. Path check ────────────────────────────────────────────────────────
    const std::string target(req.target());
    const auto qmark = target.find('?');
    const std::string path_only  = (qmark == std::string::npos)
                                       ? target
                                       : target.substr(0, qmark);
    const std::string query_str  = (qmark == std::string::npos)
                                       ? std::string{}
                                       : target.substr(qmark + 1);

    if (!isChangeStreamPath(path_only)) {
        decision.reject_status = http::status::not_found;
        decision.reject_reason = "Not Found";
        return decision;
    }

    // ── 2. Bearer token / JWT authentication ─────────────────────────────────
    // Auth middleware is optional (may be nullptr in test environments).
    if (auth_) {
        const auto auth_hdr = req[http::field::authorization];
        std::string token;

        if (!auth_hdr.empty()) {
            const std::string auth_str(auth_hdr);
            constexpr std::string_view kBearer = "Bearer ";
            if (auth_str.size() > kBearer.size() &&
                auth_str.substr(0, kBearer.size()) == kBearer)
            {
                token = auth_str.substr(kBearer.size());
            } else {
                token = auth_str;
            }
        }

        // "cdc:subscribe" scope is required for the change-stream endpoint.
        const auto result = auth_->authorize(token, "cdc:subscribe");
        if (!result.authorized) {
            THEMIS_WARN("WsChangeHandler: auth rejected for /v2/changes – {}",
                        result.reason);
            decision.reject_status = http::status::unauthorized;
            decision.reject_reason = "Unauthorized: " + result.reason;
            return decision;
        }

        decision.user_id   = result.user_id;
        decision.tenant_id = result.tenant_id;
    }

    // ── 3. Extract CDC filter parameters from the query string ───────────────
    // Supported parameters:
    //   from_sequence=<uint64>   – start delivering events from this sequence
    //   key_prefix=<string>      – only deliver events whose key starts with
    //                              this prefix (empty = all keys)

    auto parse_param = [&](const std::string& qs,
                           const std::string& key) -> std::string
    {
        const std::string search = key + "=";
        const auto pos = qs.find(search);
        if (pos == std::string::npos) return {};
        const auto val_start = pos + search.size();
        const auto val_end   = qs.find('&', val_start);
        const std::string raw = (val_end == std::string::npos)
                   ? qs.substr(val_start)
                   : qs.substr(val_start, val_end - val_start);
        return url_decode(raw);
    };

    const std::string from_seq_str = parse_param(query_str, "from_sequence");
    if (!from_seq_str.empty()) {
        uint64_t v = 0;
        const auto [ptr, ec] = std::from_chars(
            from_seq_str.data(),
            from_seq_str.data() + from_seq_str.size(),
            v);
        if (ec == std::errc{}) {
            decision.from_sequence = v;
        } else {
            THEMIS_WARN("WsChangeHandler: invalid from_sequence '{}', using 0",
                        from_seq_str);
        }
    }

    decision.key_prefix = parse_param(query_str, "key_prefix");

    decision.should_upgrade = true;
    return decision;
}

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET

