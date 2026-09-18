/**
 * @file w3c_trace_context_propagator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_context.h"

#include <algorithm>
#include <map>
#include <string>
#include <string_view>

namespace themis {
namespace core {
namespace concerns {

class W3CTraceContextPropagator {
public:
    static IContextPtr extract(
            const std::map<std::string, std::string>& headers,
            IContextPtr parent = nullptr) {

        auto ctx = parent ? parent->createChild() : SimpleContext::create();

        std::string traceparent = headerValueCI(headers, "traceparent");
        if (!traceparent.empty()) {
            std::string trace_id, parent_id;
            if (parseTraceparent(traceparent, trace_id, parent_id)) {
                ctx->set(context_keys::kTraceId, trace_id);
                ctx->set(context_keys::kSpanId,  parent_id);
            }
        }

        std::string tracestate = headerValueCI(headers, "tracestate");
        if (!tracestate.empty()) {
            ctx->set("w3c.tracestate", tracestate);
        }

        return ctx;
    }

    static void inject(
            const IContext& ctx,
            std::map<std::string, std::string>& headers) {

        auto trace_id = ctx.get(context_keys::kTraceId).value_or("");
        auto span_id  = ctx.get(context_keys::kSpanId).value_or("");

        if (isValidHex(trace_id, 32) && isValidHex(span_id, 16)) {
            headers["traceparent"] = "00-" + trace_id + "-" + span_id + "-01";
        }

        auto tracestate = ctx.get("w3c.tracestate").value_or("");
        if (!tracestate.empty()) {
            headers["tracestate"] = tracestate;
        }
    }

private:
    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    static std::string headerValueCI(
            const std::map<std::string, std::string>& headers,
            std::string_view name) {
        // Exact match first (common case: lowercase headers).
        {
            auto it = headers.find(std::string(name));
            if (it != headers.end()) {
              return it->second;
            }
        }
        // Fallback: case-insensitive linear scan.
        for (const auto& [k, v] : headers) {
            if (k.size() == name.size() &&
                std::equal(k.begin(), k.end(), name.begin(),
                           [](unsigned char a, unsigned char b) {
                               return std::tolower(a) == std::tolower(b);
                           })) {
                return v;
            }
        }
        return {};
    }

    /**
     * @brief Is Valid Hex.
     * @param[in] s Input parameter.
     * @param[in] expected_len Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: size(), std::isxdigit().
     */
    static bool isValidHex(const std::string& s, std::size_t expected_len) {
        if (s.size() != expected_len) {
          return false;
        }
        bool non_zero = false;
        for (unsigned char c : s) {
            if (!std::isxdigit(c)) {
              return false;
            }
            if (c != '0') {
              non_zero = true;
            }
        }
        return non_zero;
    }

    /**
     * @brief Parse Traceparent.
     * @param[in] value Input parameter.
     * @param[in,out] trace_id Identifier of the trace.
     * @param[in,out] parent_id Identifier of the parent.
     * @return True when the operation succeeds.
     * @details Calls: size(), fromHexDigit(), substr(), isValidHex(), std::move().
     */
    static bool parseTraceparent(
            const std::string& value,
            std::string& trace_id,
            std::string& parent_id) {

        // Minimum length: 2+1+32+1+16+1+2 = 55
        if (value.size() < 55) {
          return false;
        }
        if (value[2] != '-' || value[35] != '-' || value[52] != '-') {
          return false;
        }

        // Parse version byte (2 hex chars).
        uint8_t ver_hi, ver_lo;
        if (!fromHexDigit(value[0], ver_hi) || !fromHexDigit(value[1], ver_lo)) {
          return false;
        }
        uint8_t version = static_cast<uint8_t>((ver_hi << 4) | ver_lo);

        // version "ff" is explicitly reserved as invalid per the W3C spec.
        if (version == 0xff) {
          return false;
        }

        std::string tid = value.substr(3, 32);
        std::string pid = value.substr(36, 16);

        if (!isValidHex(tid, 32) || !isValidHex(pid, 16)) {
          return false;
        }

        trace_id  = std::move(tid);
        parent_id = std::move(pid);
        return true;
    }

    /**
     * @brief From Hex Digit.
     * @param[in] c Input parameter.
     * @param[in,out] out Input/output parameter.
     * @return True when the operation succeeds.
     * @details Implements fromHexDigit without additional internal calls.
     */
    static bool fromHexDigit(char c, uint8_t& out) {
        if (c >= '0' && c <= '9') { out = static_cast<uint8_t>(c - '0');      return true; }
        if (c >= 'a' && c <= 'f') { out = static_cast<uint8_t>(c - 'a' + 10); return true; }
        if (c >= 'A' && c <= 'F') { out = static_cast<uint8_t>(c - 'A' + 10); return true; }
        return false;
    }

    // Non-instantiable utility class.
    W3CTraceContextPropagator() = delete;
};

} // namespace concerns
} // namespace core
} // namespace themis
