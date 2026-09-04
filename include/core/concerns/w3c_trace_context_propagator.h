/**
 * @file w3c_trace_context_propagator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief W3C Trace Context propagator for distributed context propagation.
 *
 * Implements the W3C Trace Context Level 1 specification for propagating
 * trace context across service boundaries using the `traceparent` and
 * `tracestate` HTTP headers (https://www.w3.org/TR/trace-context/).
 *
 * This class bridges the W3C HTTP-header representation of distributed trace
 * context to/from the `IContext`/`ContextPropagation` key-value store so that
 * all downstream code (logging, metrics, nested calls) can access the trace
 * identifiers without being aware of HTTP headers.
 *
 * ### Typical usage at an HTTP request entry point
 * @code
 *   // Inbound request handler
 *   auto ctx = W3CTraceContextPropagator::extract(request.headers);
 *   ContextScope scope(ctx);
 *
 *   // ctx->get(context_keys::kTraceId) → "4bf92f3577b34da6a3ce929d0e0e4736"
 *   // ctx->get(context_keys::kSpanId)  → "00f067aa0ba902b7"
 *
 *   // Outbound request to downstream service
 *   std::map<std::string, std::string> out_headers;
 *   W3CTraceContextPropagator::inject(*ctx, out_headers);
 *   // out_headers["traceparent"] == "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"
 * @endcode
 *
 * ### traceparent format (W3C Trace Context Level 1)
 * @code
 *   version-traceid-parentid-traceflags
 *   00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
 * @endcode
 * Fields:
 *  - version   : 2 hex chars (currently "00")
 *  - trace-id  : 32 hex chars (128-bit); all-zeros is invalid
 *  - parent-id : 16 hex chars (64-bit);  all-zeros is invalid
 *  - flags     : 2 hex chars  (bit 0 = sampled)
 *
 * ### Context keys populated by extract()
 * | context_keys constant | Value                                      |
 * |-----------------------|--------------------------------------------|
 * | kTraceId              | 32-char hex trace-id from traceparent      |
 * | kSpanId               | 16-char hex parent-id from traceparent     |
 *
 * ### Thread safety
 * All methods are stateless and thread-safe (no shared mutable state).
 */
class W3CTraceContextPropagator {
public:
    /**
     * @brief Extract W3C TraceContext from inbound HTTP headers into an
     *        `IContext`.
     *
     * Reads the `traceparent` header (case-insensitive) from @p headers.
     * When the header is present and valid:
     *  - `kTraceId` is set to the 32-char hex trace-id.
     *  - `kSpanId`  is set to the 16-char hex parent-id.
     *
     * When `tracestate` is present, it is stored in the raw entry
     * `"w3c.tracestate"` for passthrough to downstream services.
     *
     * If the `traceparent` header is absent or invalid per the W3C spec
     * (wrong length, all-zeros trace-id, all-zeros parent-id), a new empty
     * root context is returned so callers always receive a non-null
     * `IContextPtr`.
     *
     * W3C Baggage extraction is intentionally delegated to
     * `themis::Baggage::extract()` (called separately when a tracer adapter
     * is used) to keep concerns separated.
     *
     * @param headers  Incoming HTTP headers (case-insensitive key lookup).
     * @param parent   Optional parent context; when non-null the returned
     *                 context is a child that inherits all parent attributes.
     * @return A non-null `IContextPtr` populated with trace identifiers.
     */
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

    /**
     * @brief Inject W3C TraceContext from an `IContext` into outgoing HTTP
     *        headers.
     *
     * Reads `kTraceId` and `kSpanId` from @p ctx and writes a well-formed
     * `traceparent` header (sampling flag set to 01 = sampled) into
     * @p headers.
     *
     * If `kTraceId` or `kSpanId` are absent, or if either value is not a
     * valid 32-/16-char hex string respectively, no `traceparent` header is
     * written (headers are left unchanged).
     *
     * If `w3c.tracestate` is present in @p ctx it is forwarded as the
     * `tracestate` header.
     *
     * @param ctx     Source context.
     * @param headers Outgoing HTTP headers map to populate.
     */
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

    /// Case-insensitive header lookup.
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

    /// Return true iff @p s consists of exactly @p expected_len hex
    /// characters and is not all zeros.
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
     * @brief Parse a W3C `traceparent` header value.
     *
     * Valid format: `<version>-<32-hex>-<16-hex>-<2-hex>`
     * Length must be at least 55 characters.
     * Version byte "ff" is explicitly invalid per the W3C spec.
     * All-zeros trace-id and all-zeros parent-id are explicitly invalid.
     *
     * Forward compatibility: future versions (01-fe) must be accepted and
     * parsed as long as the header contains at least 55 characters in the
     * expected field positions.  Callers should ignore any trailing data.
     *
     * @param value      Raw header value.
     * @param trace_id   Output: 32-char hex trace-id on success.
     * @param parent_id  Output: 16-char hex parent-id on success.
     * @return true if the header is valid and the outputs are set.
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

    /// Convert a single hex character to its 4-bit value; returns false on non-hex input.
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
