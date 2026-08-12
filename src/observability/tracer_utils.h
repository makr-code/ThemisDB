/**
 * @file tracer_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// tracer_utils.h — internal helpers shared by tracer.cpp and opentelemetry_tracer.cpp
//
// These functions are intentionally NOT part of the public API.
// They are placed here to avoid ODR violations in Unity Builds.
#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <utility>

namespace themis {
namespace observability {
namespace detail {

/// Generate a 128-bit random hex string for use as a W3C trace-id.
inline std::string generateTraceId() {
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    const uint64_t hi = dist(rng);
    const uint64_t lo = dist(rng);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << hi
        << std::setw(16) << lo;
    return oss.str();
}

/// Generate a 64-bit random hex string for use as a W3C span-id.
inline std::string generateSpanId() {
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << dist(rng);
    return oss.str();
}

/// Probabilistic sampling decision for a given rate in [0.0, 1.0].
inline bool shouldSample(double rate) {
    if (rate >= 1.0) return true;
    if (rate <= 0.0) return false;
    thread_local std::mt19937_64 rng{std::random_device{}()};
    thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng) < rate;
}

/// Parse a W3C traceparent of the form
///   "00-<32hexTraceId>-<16hexSpanId>-<2hexFlags>"
/// Returns {trace_id, span_id} or {"", ""} on failure.
inline std::pair<std::string, std::string> parseTraceparent(
    const std::string& value) noexcept
{
    if (value.size() < 55) return {"", ""};
    if (value[2] != '-' || value[35] != '-' || value[52] != '-')
        return {"", ""};

    std::string trace_id  = value.substr(3,  32);
    std::string parent_id = value.substr(36, 16);

    auto isHex = [](const std::string& s) {
        return std::all_of(s.begin(), s.end(),
                           [](unsigned char c) { return std::isxdigit(c) != 0; });
    };

    if (!isHex(trace_id) || !isHex(parent_id)) return {"", ""};
    return {trace_id, parent_id};
}

/// Build a W3C traceparent header value.
inline std::string buildTraceparent(const std::string& trace_id,
                                     const std::string& span_id,
                                     bool sampled = true) {
    return "00-" + trace_id + "-" + span_id + (sampled ? "-01" : "-00");
}

/// Case-insensitive header lookup.
inline std::string findHeader(const std::map<std::string, std::string>& headers,
                               const std::string& lower_key) {
    for (const auto& [k, v] : headers) {
        std::string lk = k;
        std::transform(lk.begin(), lk.end(), lk.begin(),
                       [](unsigned char c) {
                           return static_cast<char>(std::tolower(c));
                       });
        if (lk == lower_key) return v;
    }
    return {};
}

} // namespace detail
} // namespace observability
} // namespace themis
