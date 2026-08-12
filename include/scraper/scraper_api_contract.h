/**
 * @file scraper_api_contract.h
 * @brief Frozen scraper pipeline and metadata writer API contract for v1.x.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Defines the normative contract for the scraper module covering source seeding,
 * fetch/render operations, content extraction, quality evaluation, and
 * provenance-aware metadata write behaviour.
 *
 * ## §API Contracts
 *
 * Key behavioural invariants:
 *   1. Fetch failures are always surfaced as kFetchFailed; the scraper never
 *      silently returns empty content for a failed fetch.
 *   2. JS render operations are bounded by kDefaultRenderTimeoutMs; expiry
 *      produces kRenderTimeout and the partially-rendered content is discarded.
 *   3. Parse errors do not abort an entire crawl run; they produce kParseError
 *      for the affected page and the run continues.
 *   4. LLM evaluator failures produce kEvaluationFailed; content is NOT written
 *      to the metadata store unless evaluation succeeds (fail-closed on quality).
 *   5. Metadata writes are atomic; a partial write failure leaves no orphaned
 *      records and returns kMetadataWriteFailed.
 *
 * ## §Error Taxonomy
 *
 * | Code  | Constant              | Meaning                                           |
 * |-------|-----------------------|---------------------------------------------------|
 * | 0     | kSuccess              | Operation completed without error                 |
 * | 8500  | kFetchFailed          | HTTP/network fetch operation failed               |
 * | 8501  | kRenderTimeout        | JS render exceeded allowed time budget            |
 * | 8502  | kParseError           | Content parse returned structural error           |
 * | 8503  | kEvaluationFailed     | LLM quality evaluator returned failure            |
 * | 8504  | kMetadataWriteFailed  | Provenance/metadata write operation failed        |
 * | 8505  | kSourceNotFound       | Referenced source URL not in catalog              |
 * | 8506  | kPaginationLimit      | Crawl hit configured pagination depth limit       |
 * | 8507  | kInternalError        | Unclassified internal error; always deny          |
 *
 * ## §Threading Guarantees
 *
 * - Scraper pipeline stages are executed on a bounded thread pool; callers
 *   receive results via futures or callbacks.
 * - The metadata writer is thread-safe; concurrent writes from multiple
 *   pipeline workers are serialized internally.
 * - The LLM evaluator is stateless per call; concurrent invocations are safe.
 *
 * ## §Contract Freeze
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/scraper/ROADMAP.md — Phase 1 item
 */

#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace themis {
namespace scraper {

// ============================================================================
// § 1  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the scraper module.
 *
 * All scraper pipeline operations return or throw with one of these codes.
 * Values are in the reserved range [8500, 8599].
 */
enum class ScraperError : int32_t {
    kSuccess             = 0,
    kFetchFailed         = 8500, ///< HTTP/network fetch operation failed.
    kRenderTimeout       = 8501, ///< JS render exceeded time budget.
    kParseError          = 8502, ///< Content parse returned structural error.
    kEvaluationFailed    = 8503, ///< LLM quality evaluator returned failure.
    kMetadataWriteFailed = 8504, ///< Provenance/metadata write failed.
    kSourceNotFound      = 8505, ///< Source URL not in catalog.
    kPaginationLimit     = 8506, ///< Crawl hit pagination depth limit.
    kInternalError       = 8507, ///< Unclassified internal error.
};

// ============================================================================
// § 2  Pipeline constraints
// ============================================================================

/// Default JS render timeout in milliseconds.
inline constexpr std::chrono::milliseconds kDefaultRenderTimeout{30'000};

/// Maximum allowed JS render timeout (operator-configurable upper bound).
inline constexpr std::chrono::milliseconds kMaxRenderTimeout{120'000};

/// Default maximum pagination depth per crawl run.
inline constexpr uint32_t kDefaultMaxPaginationDepth = 50;

/// Maximum URL length in bytes.
inline constexpr std::size_t kMaxUrlBytes = 8192;

/// Maximum content size the scraper will process per page (10 MiB).
inline constexpr std::size_t kMaxPageContentBytes = 10u * 1024u * 1024u;

// ============================================================================
// § 3  Supporting structs
// ============================================================================

/**
 * @brief Descriptor for a single scrape request.
 */
struct ScrapeRequest {
    std::string  source_url;         ///< Target URL to scrape (max kMaxUrlBytes).
    bool         enable_js_render{false}; ///< Whether to use the JS renderer.
    uint32_t     max_pagination_depth{kDefaultMaxPaginationDepth};
    std::chrono::milliseconds render_timeout{kDefaultRenderTimeout};
};

/**
 * @brief Lightweight result descriptor returned from a scrape operation.
 */
struct ScrapeResult {
    std::string  source_url;         ///< URL that was scraped.
    bool         success{false};     ///< Whether the overall scrape succeeded.
    ScraperError error{ScraperError::kSuccess}; ///< Error code on failure.
    uint32_t     pages_scraped{0};   ///< Number of pages successfully processed.
    std::string  metadata_record_id; ///< Persisted metadata record ID (if written).
};

// ============================================================================
// § 4  Fail-closed contract
// ============================================================================

/**
 * @brief Returns true when the given error mandates fail-closed denial of write.
 */
[[nodiscard]] inline constexpr bool isScraperFailClosed(ScraperError e) noexcept {
    return e == ScraperError::kEvaluationFailed
        || e == ScraperError::kInternalError
        || e == ScraperError::kMetadataWriteFailed;
}

} // namespace scraper
} // namespace themis
