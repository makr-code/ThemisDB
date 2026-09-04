// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file scraper_render_contract.h
 * @brief Header-only contract helpers for JS renderer timeout enforcement
 *        and API pagination limit detection.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Provides two small, side-effect-free utilities that centralise the decision
 * logic required by the scraper pipeline:
 *
 *  1. **enforceRenderTimeout** — maps a raw `JsRenderResult` onto a
 *     `JsRenderContractResult` with an explicit `ScraperError` code.
 *     Timeout conditions (elapsed ≥ budget **or** renderer error message
 *     containing "timeout") produce `ScraperError::kRenderTimeout`; other
 *     failures produce `ScraperError::kFetchFailed`.  The default time budget
 *     is `kDefaultRenderTimeout` (30 000 ms).
 *
 *  2. **isPaginationLimitReached** — returns `true` when a crawl has consumed
 *     all allowed pages (`current_page >= max_depth`), matching the contract
 *     defined by `kDefaultMaxPaginationDepth`.
 *
 * Both functions are `noexcept` and have no external dependencies beyond the
 * two included headers.
 *
 * @see include/scraper/scraper_api_contract.h  — error taxonomy and constants
 * @see include/scraper/scraper_js_renderer.h   — JsRenderResult definition
 */

#pragma once

#include "scraper/scraper_api_contract.h"
#include "scraper/scraper_js_renderer.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

namespace themis {
namespace scraper {

// ============================================================================
// § 1  Contract result wrapper
// ============================================================================

/**
 * @brief Wraps a `JsRenderResult` together with an explicit `ScraperError`
 *        code and a convenience `timed_out` flag.
 *
 * Constructed exclusively by `enforceRenderTimeout`; never built directly by
 * pipeline code.
 *
 * | render outcome  | error                    | timed_out | html     |
 * |-----------------|--------------------------|-----------|----------|
 * | success         | kSuccess                 | false     | original |
 * | timeout         | kRenderTimeout           | true      | cleared  |
 * | other failure   | kFetchFailed             | false     | cleared  |
 */
struct JsRenderContractResult {
    /// The underlying render result (html is cleared on any failure).
    JsRenderResult render_result;

    /// Canonical scraper error code; kSuccess when rendering succeeded.
    ScraperError   error{ScraperError::kSuccess};

    /// True when the failure was specifically a render timeout.
    bool           timed_out{false};
};

// ============================================================================
// § 2  Helpers (internal linkage)
// ============================================================================

namespace detail {

/// Case-insensitive substring search.
inline bool containsIgnoreCase(const std::string& haystack,
                                const std::string& needle) noexcept {
    if (needle.empty()) {
      return true;
    }
    if (haystack.size() < needle.size()) {
      return false;
    }

    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(),   needle.end(),
        [](unsigned char a, unsigned char b) noexcept {
            return std::tolower(a) == std::tolower(b);
        });
    return it != haystack.end();
}

} // namespace detail

// ============================================================================
// § 3  enforceRenderTimeout
// ============================================================================

/**
 * @brief Maps a raw `JsRenderResult` onto a `JsRenderContractResult` with an
 *        authoritative `ScraperError` code.
 *
 * Decision table:
 *  - `raw.success == true`
 *      → `error = kSuccess`, `timed_out = false`, html preserved.
 *  - `raw.success == false` **AND** (`raw.elapsed_ms >= timeout_ms` **OR**
 *    `raw.error` contains "timeout" case-insensitively):
 *      → `error = kRenderTimeout`, `timed_out = true`, html cleared.
 *  - `raw.success == false` (other failure):
 *      → `error = kFetchFailed`, `timed_out = false`, html cleared.
 *
 * The default `timeout_ms` value matches
 * `kDefaultRenderTimeout` (30 000 ms).
 *
 * @param raw        Raw result returned by `IScraperJSRenderer::render()`.
 * @param timeout_ms Time budget in milliseconds used to classify elapsed time.
 *                   Defaults to `kDefaultRenderTimeout.count()` (30 000).
 * @return           A `JsRenderContractResult` with html cleared on any failure.
 *
 * @note This function is `noexcept` and safe to call from any pipeline stage.
 */
[[nodiscard]] inline JsRenderContractResult enforceRenderTimeout(
    const JsRenderResult& raw,
    int timeout_ms = static_cast<int>(kDefaultRenderTimeout.count())) noexcept
{
    JsRenderContractResult out;
    out.render_result = raw;

    if (raw.success) {
        // Happy path: propagate the result unchanged.
        out.error      = ScraperError::kSuccess;
        out.timed_out  = false;
        return out;
    }

    // Failure path: determine whether this is a timeout.
    const bool elapsed_exceeded = (raw.elapsed_ms >= static_cast<long>(timeout_ms));
    const bool error_is_timeout = detail::containsIgnoreCase(raw.error, "timeout");

    if (elapsed_exceeded || error_is_timeout) {
        out.error             = ScraperError::kRenderTimeout;
        out.timed_out         = true;
        out.render_result.html.clear();
    } else {
        out.error             = ScraperError::kFetchFailed;
        out.timed_out         = false;
        out.render_result.html.clear();
    }

    return out;
}

// ============================================================================
// § 4  isPaginationLimitReached
// ============================================================================

/**
 * @brief Returns `true` when the crawl has reached or exceeded the configured
 *        maximum pagination depth.
 *
 * This enforces the contract from `scraper_api_contract.h`:
 * > Crawl hits configured pagination depth limit → `kPaginationLimit`.
 *
 * The caller is responsible for emitting `ScraperError::kPaginationLimit`
 * once this returns `true`.  The default depth limit is
 * `kDefaultMaxPaginationDepth` (50).
 *
 * @param current_page  Zero- or one-based page index (consistent with caller).
 * @param max_depth     Maximum number of pages allowed (inclusive boundary).
 * @return              `true` if `current_page >= max_depth`.
 *
 * @note Pure predicate: no side-effects, always `noexcept`.
 */
[[nodiscard]] inline constexpr bool isPaginationLimitReached(
    uint32_t current_page,
    uint32_t max_depth) noexcept
{
    return current_page >= max_depth;
}

} // namespace scraper
} // namespace themis
