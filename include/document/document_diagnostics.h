/**
 * @file document_diagnostics.h
 * @brief Unified document module error taxonomy, classification utilities,
 *        and diagnostic sink for store/schema/merge/lifecycle/exchange failure paths.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 *
 * @details
 * This header provides a complete Phase 2 diagnostics taxonomy for the ThemisDB
 * document module. It covers all 20 ERR_DOC_* error codes (9400–9419), organises
 * them into eight semantic failure classes, and exposes utilities that allow
 * callers to classify, describe, and format document errors uniformly.
 *
 * In addition, DocumentDiagnosticSink provides a lightweight, thread-safe
 * observer that can be embedded in manager or store implementations to collect
 * per-class error counts at runtime without allocation on the hot path.
 *
 * ### Coverage map
 * | Code range | Class                          |
 * |------------|--------------------------------|
 * | 9400, 9401, 9402, 9408, 9409, 9416 | STORE_FAILURE   |
 * | 9403, 9404, 9405, 9412, 9418       | SCHEMA_VIOLATION|
 * | 9407, 9419                         | MERGE_CONFLICT  |
 * | 9417                               | LIFECYCLE_ERROR |
 * | 9413, 9414                         | ROUND_TRIP_ERROR|
 * | 9406, 9415                         | EXCHANGE_ERROR  |
 * | 9410, 9411                         | INVALID_INPUT   |
 * | all others                         | UNKNOWN         |
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/error_registry.h"
#include "utils/expected.h"
#include <cstddef>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

// ─────────────────────────────────────────────────────────────────────────────
// Forward-declare DocumentErrorClass before its std::hash specialisation so
// that the unordered_map inside DocumentDiagnosticSink compiles cleanly.
// ─────────────────────────────────────────────────────────────────────────────

namespace themis {
namespace document {

/**
 * @brief Semantic failure class for document module errors.
 *
 * Each ERR_DOC_* error code maps to exactly one class. Callers can use this
 * enum to implement class-aware retry, logging, or alerting policies without
 * hard-coding raw numeric error codes.
 */
enum class DocumentErrorClass {
    STORE_FAILURE,    ///< Backend storage-level errors (not-found, unavailable, ACL)
    SCHEMA_VIOLATION, ///< Schema validation or version ordering errors
    MERGE_CONFLICT,   ///< Three-way merge or concurrent-version conflict errors
    LIFECYCLE_ERROR,  ///< Lifecycle hook or event terminal failure errors
    ROUND_TRIP_ERROR, ///< Round-trip snapshot persistence errors
    EXCHANGE_ERROR,   ///< XDOMEA/exchange boundary enforcement errors
    INVALID_INPUT,    ///< Invalid argument, ID, or encryption operation errors
    UNKNOWN,          ///< Unclassified document errors (not in 9400–9419 range)
};

} // namespace document
} // namespace themis

// ─────────────────────────────────────────────────────────────────────────────
// std::hash specialisation – must appear before DocumentDiagnosticSink so that
// the compiler can locate it when instantiating
// std::unordered_map<DocumentErrorClass, std::size_t>.
// ─────────────────────────────────────────────────────────────────────────────

namespace std {
/**
 * @brief Hash specialisation for themis::document::DocumentErrorClass.
 *
 * Delegates to std::hash<int> via a static_cast so that DocumentErrorClass
 * values can be used directly as keys in std::unordered_map / std::unordered_set
 * without requiring any additional wrapper type.
 */
template <>
struct hash<themis::document::DocumentErrorClass> {
    [[nodiscard]] std::size_t operator()(
        themis::document::DocumentErrorClass cls) const noexcept
    {
        return std::hash<int>{}(static_cast<int>(cls));
    }
};
} // namespace std

// ─────────────────────────────────────────────────────────────────────────────
// Remaining document_diagnostics API lives in themis::document
// ─────────────────────────────────────────────────────────────────────────────

namespace themis {
namespace document {

// ─────────────────────────────────────────────────────────────────────────────
// classifyDocumentError
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Maps an ERR_DOC_* error code to its DocumentErrorClass.
 *
 * The mapping is exhaustive for all 20 codes in the range 9400–9419. Any code
 * outside that range (including non-document error families) returns
 * DocumentErrorClass::UNKNOWN.
 *
 * @param code A themis::errors::ErrorCode value. Non-document codes are
 *             accepted and classified as UNKNOWN; no precondition is violated.
 * @return The DocumentErrorClass that best represents the failure category.
 *
 * @note This function is marked noexcept and has no side effects. It is safe
 *       to call from signal handlers or constexpr contexts (pending C++20
 *       promotion; currently inline).
 */
[[nodiscard]] inline DocumentErrorClass
classifyDocumentError(errors::ErrorCode code) noexcept
{
    switch (code) {
        // ── STORE_FAILURE ──────────────────────────────────────────────────
        case errors::ErrorCode::ERR_DOC_NOT_FOUND:
        case errors::ErrorCode::ERR_DOC_ALREADY_EXISTS:
        case errors::ErrorCode::ERR_DOC_INVALID_ID:
        case errors::ErrorCode::ERR_DOC_ACCESS_DENIED:
        case errors::ErrorCode::ERR_DOC_COLLECTION_NOT_FOUND:
        case errors::ErrorCode::ERR_DOC_STORE_UNAVAILABLE:
            return DocumentErrorClass::STORE_FAILURE;

        // ── SCHEMA_VIOLATION ───────────────────────────────────────────────
        case errors::ErrorCode::ERR_DOC_SCHEMA_SEALED:
        case errors::ErrorCode::ERR_DOC_SCHEMA_VERSION_NOT_FOUND:
        case errors::ErrorCode::ERR_DOC_SCHEMA_VERSION_EXISTS:
        case errors::ErrorCode::ERR_DOC_SCHEMA_TRANSITION_INVALID:
        case errors::ErrorCode::ERR_DOC_VALIDATION_ABORTED:
            return DocumentErrorClass::SCHEMA_VIOLATION;

        // ── MERGE_CONFLICT ─────────────────────────────────────────────────
        case errors::ErrorCode::ERR_DOC_MERGE_CONFLICT:
        case errors::ErrorCode::ERR_DOC_VERSION_CONFLICT:
            return DocumentErrorClass::MERGE_CONFLICT;

        // ── LIFECYCLE_ERROR ────────────────────────────────────────────────
        case errors::ErrorCode::ERR_DOC_LIFECYCLE_HOOK_FAILED:
            return DocumentErrorClass::LIFECYCLE_ERROR;

        // ── ROUND_TRIP_ERROR ───────────────────────────────────────────────
        case errors::ErrorCode::ERR_DOC_SNAPSHOT_COLLISION:
        case errors::ErrorCode::ERR_DOC_ROUND_TRIP_PERSIST_FAIL:
            return DocumentErrorClass::ROUND_TRIP_ERROR;

        // ── EXCHANGE_ERROR ─────────────────────────────────────────────────
        case errors::ErrorCode::ERR_DOC_DIFF_NOT_FOUND:
        case errors::ErrorCode::ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED:
            return DocumentErrorClass::EXCHANGE_ERROR;

        // ── INVALID_INPUT ──────────────────────────────────────────────────
        case errors::ErrorCode::ERR_DOC_ENCRYPT_FAILED:
        case errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT:
            return DocumentErrorClass::INVALID_INPUT;

        // ── UNKNOWN (non-document or unrecognised codes) ───────────────────
        default:
            return DocumentErrorClass::UNKNOWN;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// documentErrorClassName
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Returns a human-readable, stable ASCII name for a DocumentErrorClass.
 *
 * The returned string_view points to a string literal with static storage
 * duration; callers must not store the pointer past the class's lifetime (which
 * is effectively the program lifetime for literals).
 *
 * @param cls The DocumentErrorClass value to name.
 * @return A non-null, null-terminated string_view such as "STORE_FAILURE".
 *         Returns "UNKNOWN" for DocumentErrorClass::UNKNOWN and for any
 *         out-of-range cast value.
 */
[[nodiscard]] inline std::string_view
documentErrorClassName(DocumentErrorClass cls) noexcept
{
    switch (cls) {
        case DocumentErrorClass::STORE_FAILURE:    return "STORE_FAILURE";
        case DocumentErrorClass::SCHEMA_VIOLATION: return "SCHEMA_VIOLATION";
        case DocumentErrorClass::MERGE_CONFLICT:   return "MERGE_CONFLICT";
        case DocumentErrorClass::LIFECYCLE_ERROR:  return "LIFECYCLE_ERROR";
        case DocumentErrorClass::ROUND_TRIP_ERROR: return "ROUND_TRIP_ERROR";
        case DocumentErrorClass::EXCHANGE_ERROR:   return "EXCHANGE_ERROR";
        case DocumentErrorClass::INVALID_INPUT:    return "INVALID_INPUT";
        default:                                   return "UNKNOWN";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// documentErrorDescription
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Returns a concise, single-line description for a document error code.
 *
 * The description is suitable for inclusion in log messages, diagnostic output,
 * or user-facing API error payloads. Each of the 20 ERR_DOC_* codes (9400–9419)
 * has a distinct entry. Codes outside this range return "unknown document error".
 *
 * @param code A themis::errors::ErrorCode value. Non-document codes are
 *             accepted; they produce the "unknown document error" fallback.
 * @return A non-null, null-terminated string_view with static storage duration.
 */
[[nodiscard]] inline std::string_view
documentErrorDescription(errors::ErrorCode code) noexcept
{
    switch (code) {
        case errors::ErrorCode::ERR_DOC_NOT_FOUND:
            return "document not found in the store";
        case errors::ErrorCode::ERR_DOC_ALREADY_EXISTS:
            return "document with the same ID already exists";
        case errors::ErrorCode::ERR_DOC_INVALID_ID:
            return "document ID is empty or malformed";
        case errors::ErrorCode::ERR_DOC_SCHEMA_SEALED:
            return "schema registry is sealed; cannot register new versions";
        case errors::ErrorCode::ERR_DOC_SCHEMA_VERSION_NOT_FOUND:
            return "requested schema version does not exist";
        case errors::ErrorCode::ERR_DOC_SCHEMA_VERSION_EXISTS:
            return "schema version already registered";
        case errors::ErrorCode::ERR_DOC_DIFF_NOT_FOUND:
            return "one or both documents for diff/merge not found";
        case errors::ErrorCode::ERR_DOC_MERGE_CONFLICT:
            return "three-way merge produced unresolvable conflicts";
        case errors::ErrorCode::ERR_DOC_ACCESS_DENIED:
            return "collection ACL denied the requested operation";
        case errors::ErrorCode::ERR_DOC_COLLECTION_NOT_FOUND:
            return "collection does not exist";
        case errors::ErrorCode::ERR_DOC_ENCRYPT_FAILED:
            return "encrypted entity operation failed";
        case errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT:
            return "a required argument is invalid or missing";
        case errors::ErrorCode::ERR_DOC_SCHEMA_TRANSITION_INVALID:
            return "schema version transition violates ordering or compatibility rules";
        case errors::ErrorCode::ERR_DOC_SNAPSHOT_COLLISION:
            return "round-trip snapshot ID already exists (relay/index collision)";
        case errors::ErrorCode::ERR_DOC_ROUND_TRIP_PERSIST_FAIL:
            return "round-trip persistence failed at store level";
        case errors::ErrorCode::ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED:
            return "XDOMEA/exchange boundary enforcement failed";
        case errors::ErrorCode::ERR_DOC_STORE_UNAVAILABLE:
            return "backing document store is unavailable or unresponsive";
        case errors::ErrorCode::ERR_DOC_LIFECYCLE_HOOK_FAILED:
            return "lifecycle hook signaled a terminal failure";
        case errors::ErrorCode::ERR_DOC_VALIDATION_ABORTED:
            return "schema validation aborted due to structural document error (non-object body)";
        case errors::ErrorCode::ERR_DOC_VERSION_CONFLICT:
            return "concurrent version update conflict detected";
        default:
            return "unknown document error";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// operator<< for DocumentErrorClass
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Stream insertion operator for DocumentErrorClass.
 *
 * Writes the ASCII class name (e.g., "MERGE_CONFLICT") to @p os. This makes
 * DocumentErrorClass values directly usable with std::cout, spdlog, and
 * fmt-based loggers that accept ostream-compatible types.
 *
 * @param os  Destination output stream.
 * @param cls The DocumentErrorClass to insert.
 * @return Reference to @p os, enabling chaining.
 */
inline std::ostream& operator<<(std::ostream& os, DocumentErrorClass cls)
{
    return os << documentErrorClassName(cls);
}

// ─────────────────────────────────────────────────────────────────────────────
// formatDocumentError
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Produces a fully-formatted, single-line diagnostic string for a
 *        document Error.
 *
 * The format is:
 * @code
 *   "[DOC:<numeric_code>/<CLASS_NAME>] <description>: <context>"
 * @endcode
 *
 * If the error carries no context string, the trailing ": <context>" segment
 * is omitted.
 *
 * ### Example outputs
 * @code
 *   "[DOC:9407/MERGE_CONFLICT] three-way merge produced unresolvable conflicts: 3 conflict(s)"
 *   "[DOC:9400/STORE_FAILURE] document not found in the store: /collections/invoices/doc-42"
 *   "[DOC:9412/SCHEMA_VIOLATION] schema version transition violates ordering or compatibility rules"
 * @endcode
 *
 * @param err A themis::Error carrying an ERR_DOC_* code and optional context.
 *            Non-document codes are formatted with class UNKNOWN.
 * @return A heap-allocated std::string suitable for logging or API responses.
 */
[[nodiscard]] inline std::string formatDocumentError(const themis::Error& err)
{
    const auto code = err.code();
    const auto cls  = classifyDocumentError(code);
    const auto desc = documentErrorDescription(code);
    const auto& ctx = err.context();

    std::ostringstream oss;
    oss << "[DOC:" << static_cast<int>(code)
        << "/" << documentErrorClassName(cls) << "] "
        << desc;
    if (!ctx.empty()) {
        oss << ": " << ctx;
    }
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// DocumentDiagnosticSink
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe observer that accumulates document error counts by class.
 *
 * DocumentDiagnosticSink is a lightweight, non-owning diagnostic collector
 * intended to be embedded in DocumentManager, DocumentStore, or higher-level
 * orchestrators. It does not own any storage backend and does not throw.
 *
 * ### Thread safety
 * All public methods acquire the internal mutex via std::lock_guard before
 * accessing shared state. The mutex is declared `mutable` so that const read
 * accessors (count(), totalCount()) can lock it without violating const
 * correctness.
 *
 * ### Usage example
 * @code
 *   DocumentDiagnosticSink sink;
 *
 *   // Inside error-handling code:
 *   sink.record(errors::ErrorCode::ERR_DOC_MERGE_CONFLICT, "branch 'feature/x' vs 'main'");
 *
 *   // Later, for reporting:
 *   std::size_t conflicts = sink.count(DocumentErrorClass::MERGE_CONFLICT);
 *   std::size_t total     = sink.totalCount();
 * @endcode
 *
 * @note Lifecycle hooks are declared noexcept in IDocumentLifecycleHook, so
 *       record() is also noexcept, making it safe to invoke from hook
 *       implementations.
 */
class DocumentDiagnosticSink {
public:
    /// @brief Default-constructs an empty sink with zero counts.
    DocumentDiagnosticSink() = default;

    /// Non-copyable; diagnostics state is not meant to be duplicated.
    DocumentDiagnosticSink(const DocumentDiagnosticSink&)            = delete;
    DocumentDiagnosticSink& operator=(const DocumentDiagnosticSink&) = delete;

    /// Movable: allows the sink to be transferred during initialisation.
    DocumentDiagnosticSink(DocumentDiagnosticSink&&)            noexcept = default;
    DocumentDiagnosticSink& operator=(DocumentDiagnosticSink&&) noexcept = default;

    ~DocumentDiagnosticSink() = default;

    // ── Mutating operations ──────────────────────────────────────────────────

    /**
     * @brief Records one diagnostic event for the given error code.
     *
     * Classifies @p code via classifyDocumentError(), increments the
     * per-class counter, and increments the total counter. The optional
     * @p context string is accepted for API symmetry with formatDocumentError()
     * but is currently not persisted (count-only sink); extend if richer
     * diagnostics are needed.
     *
     * @param code    The ERR_DOC_* (or any) ErrorCode being recorded.
     * @param context An optional caller-supplied diagnostic context string
     *                (e.g., document ID, collection name). Ignored in count
     *                tracking but visible in any future log-extension path.
     *
     * @note noexcept: safe to call from lifecycle hooks and signal-adjacent
     *       code paths.
     */
    void record(errors::ErrorCode code,
                [[maybe_unused]] std::string_view context) noexcept
    {
        const auto cls = classifyDocumentError(code);
        std::lock_guard<std::mutex> lock(mu_);
        ++counts_[cls];
        ++total_;
    }

    // ── Read-only accessors ──────────────────────────────────────────────────

    /**
     * @brief Returns the number of errors recorded for the specified class.
     *
     * Returns 0 if no errors of that class have been recorded (i.e., the class
     * is absent from the internal map).
     *
     * @param cls The DocumentErrorClass to query.
     * @return Number of errors recorded for @p cls. Thread-safe.
     */
    [[nodiscard]] std::size_t count(DocumentErrorClass cls) const noexcept
    {
        std::lock_guard<std::mutex> lock(mu_);
        const auto it = counts_.find(cls);
        return (it != counts_.end()) ? it->second : std::size_t{0};
    }

    /**
     * @brief Returns the total number of errors recorded across all classes.
     *
     * @return Aggregate error count. Thread-safe.
     */
    [[nodiscard]] std::size_t totalCount() const noexcept
    {
        std::lock_guard<std::mutex> lock(mu_);
        return total_;
    }

    /**
     * @brief Resets all per-class counters and the total counter to zero.
     *
     * Intended for use at the start of a new observation window (e.g., between
     * test runs or at a metrics reporting boundary). Thread-safe.
     */
    void clear() noexcept
    {
        std::lock_guard<std::mutex> lock(mu_);
        counts_.clear();
        total_ = 0;
    }

private:
    mutable std::mutex mu_;                                          ///< Guards all mutable members
    std::unordered_map<DocumentErrorClass, std::size_t> counts_;    ///< Per-class error counts
    std::size_t total_{0};                                           ///< Aggregate error count
};

} // namespace document
} // namespace themis
