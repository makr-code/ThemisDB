/**
 * @file llm_correlation_context.h
 * @brief LLMCorrelationContext — lightweight W3C-compatible correlation context
 *        for end-to-end observability across the ThemisDB LLM pipeline.
 *
 * ## Purpose (P1.3)
 *
 * A single LLM inference request traverses multiple subsystems:
 *   HTTP server → AQL handler → DB retrieval → prompt assembly → inference → response
 *
 * Without a shared correlation context, log records from these subsystems
 * cannot be joined, making incident triage slow and reproducibility impossible.
 *
 * `LLMCorrelationContext` is a plain value type (no heap allocation, no mutex)
 * that can be cheaply passed by value or const-reference through every layer.
 * Each layer is responsible for copying the context into its own structures
 * (e.g. `InferenceRequest::trace_id`, `AIDecisionAudit::trace_id`).
 *
 * ## W3C Traceparent compatibility
 *
 * Fields follow the W3C Trace Context specification (https://www.w3.org/TR/trace-context/):
 *   - `trace_id` : 128-bit identifier, 32 lower-case hex chars
 *   - `span_id`  : 64-bit identifier, 16 lower-case hex chars
 *
 * ## Propagation contract
 *
 * 1. The HTTP server (or AQL entry point) generates a context from the incoming
 *    `traceparent` header, or synthesises one if absent.
 * 2. The context is passed to `LLMAQLHandler::handleRequest()`.
 * 3. The handler forwards `trace_id` / `span_id` into:
 *    - `LLMQueryContext` (retrieval snapshot)
 *    - `InferenceRequest` (llama.cpp call)
 *    - `AIDecisionAudit` (governance log)
 * 4. The response carries the same `trace_id` in the `X-Trace-ID` response header.
 *
 * @see include/aql/llm_query_context.h       — MVCC snapshot with trace fields
 * @see include/llm/ai_decision_auditor.h     — Audit record with trace fields
 * @see include/llm/llm_plugin_interface.h    — InferenceRequest::trace_id
 */

#pragma once

#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <iomanip>

namespace themis {
namespace llm {

/**
 * @brief Lightweight correlation context for a single LLM pipeline invocation.
 *
 * Intended to be created once per request at the topmost entry point and
 * threaded downward through all pipeline stages by value.  Copy is intentionally
 * cheap (two `std::string` fields of at most 32 characters each).
 */
struct LLMCorrelationContext {
    /**
     * @brief W3C traceparent trace-id: 128-bit, 32 lower-case hex characters.
     *
     * Empty string means the context was created without a parent trace (i.e.
     * this is the root span).  Callers MUST NOT treat an empty string as valid.
     * Always use `isValid()` or `ensure()` before reading.
     */
    std::string trace_id;

    /**
     * @brief W3C traceparent parent-span-id: 64-bit, 16 lower-case hex characters.
     *
     * Identifies the specific span (call site) that originated this request.
     * May be empty for synthetic/generated contexts.
     */
    std::string span_id;

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * @return true when trace_id has exactly 32 lower-case hex characters AND
     *         span_id has exactly 16 lower-case hex characters.
     */
    [[nodiscard]] bool isValid() const noexcept {
        return trace_id.size() == 32 && span_id.size() == 16;
    }

    // -----------------------------------------------------------------------
    // Factory helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Create a context from an incoming W3C `traceparent` header value.
     *
     * Parses the `traceparent` header format:
     *   `{version}-{trace-id}-{parent-id}-{flags}`
     *
     * @param traceparent  Raw header value, e.g.
     *                     `"00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"`.
     * @return Populated context if parsing succeeded; empty context otherwise.
     */
    [[nodiscard]] static LLMCorrelationContext fromTraceparent(
        const std::string& traceparent) noexcept
    {
        // Format: version(2)-trace_id(32)-parent_id(16)-flags(2), separated by '-'.
        LLMCorrelationContext ctx;
        if (traceparent.size() < 55) return ctx;  // minimal valid length
        const auto p1 = traceparent.find('-');
        if (p1 == std::string::npos) return ctx;
        const auto p2 = traceparent.find('-', p1 + 1);
        if (p2 == std::string::npos) return ctx;
        const auto p3 = traceparent.find('-', p2 + 1);
        if (p3 == std::string::npos) return ctx;

        ctx.trace_id = traceparent.substr(p1 + 1, p2 - p1 - 1);
        ctx.span_id  = traceparent.substr(p2 + 1, p3 - p2 - 1);
        if (!ctx.isValid()) {
            ctx.trace_id.clear();
            ctx.span_id.clear();
        }
        return ctx;
    }

    /**
     * @brief Generate a new root context with random trace_id and span_id.
     *
     * Used when no incoming traceparent header is present.
     *
     * @note Uses `std::mt19937_64` with `std::random_device` seeding.  This is
     *       not cryptographically strong; use only for observability identifiers.
     */
    [[nodiscard]] static LLMCorrelationContext generate() noexcept {
        try {
            std::random_device rd;
            std::mt19937_64 rng{rd()};
            std::uniform_int_distribution<uint64_t> dist;

            const uint64_t hi = dist(rng);
            const uint64_t lo = dist(rng);
            const uint64_t sp = dist(rng);

            auto toHex = [](uint64_t v, int width) {
                std::ostringstream ss;
                ss << std::setw(width) << std::setfill('0') << std::hex << v;
                return ss.str();
            };

            LLMCorrelationContext ctx;
            ctx.trace_id = toHex(hi, 16) + toHex(lo, 16);
            ctx.span_id  = toHex(sp, 16);
            return ctx;
        } catch (...) {
            // Fallback: all-zero IDs — better than crashing on observability init.
            LLMCorrelationContext ctx;
            ctx.trace_id = std::string(32, '0');
            ctx.span_id  = std::string(16, '0');
            return ctx;
        }
    }

    /**
     * @brief Return a valid context: the current one if valid, otherwise a
     *        freshly generated one.
     */
    [[nodiscard]] LLMCorrelationContext ensure() const noexcept {
        return isValid() ? *this : generate();
    }

    // -----------------------------------------------------------------------
    // Serialisation helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Produce a W3C `traceparent` header value for the version-0 format.
     *
     * @return `"00-{trace_id}-{span_id}-00"` when the context is valid;
     *         an empty string otherwise.
     */
    [[nodiscard]] std::string toTraceparent() const noexcept {
        if (!isValid()) return {};
        return "00-" + trace_id + "-" + span_id + "-00";
    }
};

} // namespace llm
} // namespace themis
