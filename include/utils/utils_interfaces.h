/**
 * @file utils_interfaces.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace themis {
namespace utils {

// ============================================================================
// IStreamingPIIDetector — supporting types
// ============================================================================

/**
 * @brief PII category used by IStreamingPIIDetector.
 *
 * Coarse-grained category labels exposed by the interface; raw entity values
 * are never included in interface output types.
 */
enum class PIICategory {
    Email,
    Phone,
    SocialSecurityNumber,
    CreditCard,
    IBAN,
    IPAddress,
    URL,
    PersonName,
    Location,
    Organization,
    Other,
};

/**
 * @brief Result returned by IStreamingPIIDetector::detect().
 *
 * Never contains original PII values — only category labels and counts.
 */
struct PIIDetectionResult {
    bool containsPII{false};
    std::vector<PIICategory> categories; ///< Distinct categories found in the chunk.
    size_t spanCount{0};                 ///< Total number of detected PII spans.
};

/**
 * @brief Opaque handle for the pseudonym map produced by pseudonymise().
 *
 * Consumers may pass this handle back to the implementation for correlation
 * or logging; raw pseudonym-to-original mappings are never exposed.
 */
struct PseudonymMapHandle {
    uint64_t id{0}; ///< Opaque identifier — implementation-defined.
};

/**
 * @brief Result returned by IStreamingPIIDetector::pseudonymise().
 *
 * Contains the sanitised bytes (with PII replaced by deterministic pseudonyms)
 * and metadata; original PII values are not included.
 */
struct SanitisedChunk {
    std::vector<std::byte> sanitisedData;    ///< Sanitised byte content for this chunk.
    size_t replacementCount{0};              ///< Number of PII spans replaced.
    PseudonymMapHandle pseudonymMap;         ///< Opaque handle; no raw values exposed.
};

/**
 * @brief Stateless, thread-safe streaming PII detection and pseudonymisation interface.
 *
 * Implementations MUST be stateless between calls: no state from one detect()
 * or pseudonymise() invocation may influence a subsequent one.  This contract
 * makes it safe to call the same instance concurrently from multiple threads.
 *
 * Output types (PIIDetectionResult, SanitisedChunk) never contain original PII
 * values; only category labels, span counts, and opaque handles are exposed.
 */
class IStreamingPIIDetector {
public:
    virtual ~IStreamingPIIDetector() = default;

    /**
     * @brief Detect PII in a byte chunk.
     *
     * Stateless: no state is retained between calls.  Safe to call concurrently.
     *
     * @param chunk  Byte span of the document chunk to inspect.
     * @return       Detection result (no raw PII values).
     */
    [[nodiscard]] virtual PIIDetectionResult detect(std::span<const std::byte> chunk) const = 0;

    /**
     * @brief Replace all PII spans in a byte chunk with deterministic pseudonyms.
     *
     * Stateless: no state is retained between calls.  Safe to call concurrently.
     *
     * @param chunk  Byte span of the document chunk to sanitise.
     * @return       Sanitised chunk (no raw PII values).
     */
    [[nodiscard]] virtual SanitisedChunk pseudonymise(std::span<const std::byte> chunk) const = 0;

    /**
     * @brief Return the set of PII categories this implementation can detect.
     */
    [[nodiscard]] virtual std::span<const PIICategory> supportedCategories() const = 0;
};


// ============================================================================
// IHashChainAuditLog — supporting types
// ============================================================================

/** @brief Monotonically-increasing sequence number identifying an audit entry. */
using EntryId = uint64_t;

/**
 * @brief An audit event to be appended to IHashChainAuditLog.
 *
 * The payload is implementation-defined structured data (e.g. JSON).
 */
struct AuditEvent {
    std::string eventType;    ///< Machine-readable event type (e.g. "auth.login").
    std::string actorId;      ///< Principal that triggered the event.
    std::string resourceId;   ///< Affected resource identifier.
    std::string payload;      ///< Serialised event body (JSON string recommended).
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
};

/**
 * @brief Result of IHashChainAuditLog::verifyChain().
 */
struct ChainVerifyResult {
    bool valid{true};                            ///< true when the full range is intact.
    std::optional<EntryId> firstTamperedEntry;   ///< Sequence number of first bad entry.
    size_t verifiedCount{0};                     ///< Number of entries successfully verified.
    std::string errorMessage;                    ///< Human-readable description on failure.
};

/**
 * @brief Cursor position for IHashChainAuditLog::query().
 *
 * Cursors are read-only and are invalidated by new appends.
 */
struct AuditCursor {
    EntryId position{0};   ///< Current sequence position.
    bool exhausted{false}; ///< true when no more entries remain.
};

/**
 * @brief Query parameters for IHashChainAuditLog::query().
 */
struct AuditQuery {
    std::optional<EntryId> fromEntry;         ///< Start of range (inclusive); nullopt = start.
    std::optional<EntryId> toEntry;           ///< End of range (inclusive); nullopt = end.
    std::optional<std::string> eventTypeFilter; ///< Optional event-type prefix filter.
    size_t pageSize{100};                     ///< Maximum entries per page.
};

/**
 * @brief Tamper-evident append-only audit log interface.
 *
 * Each appended entry's hash includes the hash of the previous entry, forming
 * an HMAC-SHA-256 chain.  The interface is append-only: no delete or update
 * methods exist.
 */
class IHashChainAuditLog {
public:
    virtual ~IHashChainAuditLog() = default;

    /**
     * @brief Append an audit event to the log.
     *
     * Thread-safe.  The returned EntryId is the sequence number of the new entry.
     *
     * @param event  The event to append.
     * @return       Sequence number assigned to the new entry.
     */
    [[nodiscard]] virtual EntryId append(const AuditEvent& event) = 0;

    /**
     * @brief Verify the hash chain over a range of entries.
     *
     * @param from  First entry to verify (inclusive).
     * @param to    Last entry to verify (inclusive).
     * @return      Verification result.
     */
    [[nodiscard]] virtual ChainVerifyResult verifyChain(EntryId from, EntryId to) const = 0;

    /**
     * @brief Open a paginated, forward-only cursor for reading entries.
     *
     * The cursor is invalidated by subsequent calls to append().
     *
     * @param query  Query parameters (range + optional filter).
     * @return       Cursor positioned at the first matching entry.
     */
    [[nodiscard]] virtual AuditCursor query(const AuditQuery& query) const = 0;

    /** @brief Total number of entries appended since the log was created. */
    [[nodiscard]] virtual size_t entryCount() const = 0;

    /**
     * @brief Sequence number of the most recently appended entry.
     *
     * Returns 0 when the log is empty.
     */
    [[nodiscard]] virtual EntryId lastEntryId() const = 0;
};


// ============================================================================
// IHKDFKeyCache — supporting types
// ============================================================================

/**
 * @brief Context for a single HKDF derivation.
 *
 * Same IKM with a different purpose or identity field yields a distinct key,
 * preventing cross-context key reuse.
 */
struct KeyContext {
    std::vector<uint8_t> ikm;     ///< Input key material.
    std::vector<uint8_t> salt;    ///< Optional salt.
    std::string info;             ///< Context/purpose string.
    size_t outputLength{32};      ///< Desired key length in bytes.
};

/**
 * @brief Move-only RAII handle for a cached derived key.
 *
 * The destructor zeroes the key bytes in memory before releasing them,
 * ensuring key material does not linger on the heap.
 *
 * Raw key bytes are never exposed through the public interface.
 */
class KeyHandle {
public:
    /** @brief Construct with derived key bytes (implementation use only). */
    explicit KeyHandle(std::vector<uint8_t> keyBytes) noexcept
        : bytes_(std::move(keyBytes)) {}

    KeyHandle(const KeyHandle&)            = delete;
    KeyHandle& operator=(const KeyHandle&) = delete;

    KeyHandle(KeyHandle&&) noexcept            noexcept = default;
    KeyHandle& operator=(KeyHandle&&) noexcept noexcept = default;

    ~KeyHandle() {
        // Zeroize key material before release.
        for (auto& b : bytes_) {
            b = 0;
        }
    }

    /** @brief Length of the derived key in bytes. */
    size_t size() const noexcept { return bytes_.size(); }

    /** @brief true when the handle holds a valid (non-empty) key. */
    bool valid() const noexcept { return !bytes_.empty(); }

    /**
     * @brief Provide read-only access to the key bytes for cryptographic use.
     *
     * The returned span is valid only for the lifetime of this handle.
     * Do NOT log, serialise, or retain the raw bytes beyond the call site.
     */
    std::span<const uint8_t> bytes() const noexcept {
        return {bytes_.data(), bytes_.size()};
    }

private:
    std::vector<uint8_t> bytes_;
};

/**
 * @brief TTL-enforced HKDF key-derivation cache interface.
 *
 * derive() returns a cached key when the TTL has not expired; it evicts and
 * re-derives when the TTL has expired, ensuring stale key material is never
 * served.  KeyHandle's destructor zeroes key bytes on release.
 *
 * No method on this interface exposes raw key bytes beyond KeyHandle::bytes().
 */
class IHKDFKeyCache {
public:
    virtual ~IHKDFKeyCache() = default;

    /**
     * @brief Return a (possibly cached) derived key for the given context.
     *
     * If an unexpired cache entry exists for @p ctx, the cached KeyHandle is
     * returned.  Otherwise a fresh HKDF derivation is performed, the result
     * is cached, and the new handle is returned.
     *
     * @param ctx  Derivation context (IKM, salt, info, length).
     * @return     Move-only handle wrapping the derived key bytes.
     */
    [[nodiscard]] virtual KeyHandle derive(const KeyContext& ctx) = 0;

    /**
     * @brief Explicitly invalidate the cache entry for @p ctx.
     *
     * A no-op when no entry exists.
     */
    virtual void evict(const KeyContext& ctx) = 0;

    /**
     * @brief Evict all cache entries (emergency key-cache flush).
     */
    virtual void evictAll() = 0;

    /**
     * @brief Remaining TTL for the cached entry for @p ctx.
     *
     * @return Remaining lifetime in milliseconds; 0 when expired or absent.
     */
    [[nodiscard]] virtual std::chrono::milliseconds ttl(const KeyContext& ctx) const = 0;

    /** @brief Current number of live (unexpired) cache entries. */
    [[nodiscard]] virtual size_t cacheSize() const = 0;

    /** @brief Maximum number of entries the cache will hold before evicting LRU entries. */
    [[nodiscard]] virtual size_t maxCacheSize() const = 0;
};


// ============================================================================
// IStructuredLogSampler — supporting types
// ============================================================================

/**
 * @brief Event classification used by IStructuredLogSampler.
 *
 * Security events MUST always be sampled (shouldSample() == true) regardless
 * of the configured rate.
 */
enum class EventClass {
    Operational,  ///< Normal operational log entry — subject to sampling.
    Performance,  ///< Performance/metrics log entry — subject to sampling.
    Debug,        ///< Debug-only entry — subject to highest sampling suppression.
    Security,     ///< Security / compliance entry — NEVER sampled away.
};

/**
 * @brief A log entry passed to IStructuredLogSampler for a sampling decision.
 */
struct LogEntry {
    EventClass eventClass{EventClass::Operational};
    int level{0};             ///< Numeric log level (e.g. spdlog level values).
    const char* file{nullptr}; ///< Source file (for per-call-site buckets).
    int line{0};               ///< Source line (for per-call-site buckets).
    std::string message;      ///< Log message (may be inspected by adaptive samplers).
};

/**
 * @brief Structured log-sampling and rate-limiting interface.
 *
 * All methods that make or record a sampling decision are noexcept; log
 * sampling must never throw or abort the calling thread.
 *
 * Contract: shouldSample() MUST return true for any entry with
 * eventClass == EventClass::Security, regardless of the current rate.
 * Conforming implementations should enforce this via a short-circuit check
 * before consulting the rate limiter.
 */
class IStructuredLogSampler {
public:
    virtual ~IStructuredLogSampler() = default;

    /**
     * @brief Decide whether @p entry should be emitted.
     *
     * @return true  — caller should emit the log entry.
     * @return false — caller should suppress the log entry.
     *
     * noexcept: never throws; must not abort the caller.
     *
     * Security contract: returns true unconditionally for Security-class entries.
     */
    [[nodiscard]] virtual bool shouldSample(const LogEntry& entry) noexcept = 0;

    /**
     * @brief Provide feedback on whether @p entry was actually emitted.
     *
     * Adaptive rate-limiting implementations use this to adjust future rates.
     *
     * noexcept: never throws; must not abort the caller.
     */
    virtual void recordDecision(const LogEntry& entry, bool sampled) noexcept = 0;

    /**
     * @brief Current effective sampling rate as a fraction in [0.0, 1.0].
     *
     * noexcept.
     */
    [[nodiscard]] virtual double currentRate() const noexcept = 0;

    /**
     * @brief Request a new target sampling rate (fraction in [0.0, 1.0]).
     *
     * noexcept.
     */
    virtual void setTargetRate(double rate) noexcept = 0;

    /**
     * @brief Total number of log entries passed to shouldSample() that were sampled.
     *
     * noexcept.
     */
    [[nodiscard]] virtual size_t sampledCount() const noexcept = 0;

    /**
     * @brief Total number of log entries passed to shouldSample() that were dropped.
     *
     * noexcept.
     */
    [[nodiscard]] virtual size_t droppedCount() const noexcept = 0;
};


// ============================================================================
// ISAGALogCompactor — supporting types
// ============================================================================

/**
 * @brief Identifies a range of SAGA log segments by their boundary transaction IDs.
 */
struct SegmentRange {
    std::string fromTxnId; ///< Start boundary (inclusive).
    std::string toTxnId;   ///< End boundary (inclusive).
};

/** @brief Opaque identifier for a single compacted SAGA log segment. */
using SegmentId = std::string;

/**
 * @brief Result produced by an async ISAGALogCompactor::compact() operation.
 */
struct CompactionResult {
    size_t compactedSegments{0};  ///< Number of segments processed.
    size_t bytesSaved{0};         ///< Approximate bytes reclaimed.
    size_t retainedEntries{0};    ///< Entries preserved (committed SAGA steps).
    uint64_t durationMs{0};       ///< Wall-clock duration of the compaction pass.
    bool success{false};          ///< true when compaction completed without error.
    std::string errorMessage;     ///< Non-empty when success == false.
};

/**
 * @brief Minimal SAGA log entry as surfaced by the replay iterator.
 *
 * Only a subset of fields from the full SAGAStep is exposed here to decouple
 * the interface from the concrete logger implementation.
 */
struct SAGALogEntry {
    std::string sagaId;      ///< SAGA transaction identifier.
    std::string stepName;    ///< Step name within the saga.
    std::string action;      ///< "forward" or "compensate".
    std::string status;      ///< "success", "failed", or "pending".
    std::string payloadJson; ///< Step payload serialised as JSON.
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Forward-only, read-only iterator over entries in a compacted segment.
 *
 * No random access; reset() rewinds to the first entry.
 */
class ReplayIterator {
public:
    virtual ~ReplayIterator() = default;

    /** @brief true when at least one more entry is available. */
    [[nodiscard]] virtual bool hasNext() const = 0;

    /**
     * @brief Return the next entry and advance the cursor.
     *
     * Behaviour is undefined when hasNext() == false.
     */
    [[nodiscard]] virtual SAGALogEntry next() = 0;

    /** @brief Rewind to the first entry in the segment. */
    virtual void reset() = 0;
};

/**
 * @brief Async, non-blocking SAGA log compaction interface.
 *
 * compact() returns immediately; compaction proceeds in the background.
 * The returned future resolves when the background pass completes.
 *
 * Compaction MUST preserve all committed SAGA entries; only uncommitted or
 * superseded entries may be removed.
 */
class ISAGALogCompactor {
public:
    virtual ~ISAGALogCompactor() = default;

    /**
     * @brief Begin background compaction over the given segment range.
     *
     * Returns immediately.  The caller can wait on the future, or discard it
     * for fire-and-forget semantics.
     *
     * @param range  Segment range to compact.
     * @return       Future that resolves to a CompactionResult.
     */
    [[nodiscard]] virtual std::future<CompactionResult> compact(SegmentRange range) = 0;

    /**
     * @brief Open a forward-only iterator over the entries in a compacted segment.
     *
     * @param segmentId  Identifier of the segment to replay.
     * @return           Owning pointer to a ReplayIterator; never null on success.
     */
    [[nodiscard]] virtual std::unique_ptr<ReplayIterator> replay(SegmentId segmentId) = 0;
};


// ============================================================================
// IUtilsPipeline — supporting types
// ============================================================================

/**
 * @brief Result of a completed IUtilsPipeline::run() operation.
 */
struct PipelineResult {
    bool success{false};        ///< true when all stages completed without error.
    size_t stagesRun{0};        ///< Number of stages that were executed.
    std::string errorMessage;   ///< Non-empty when success == false.
};

/**
 * @brief Abstract interface for a single composable stage in a IUtilsPipeline.
 */
class IUtilsStage {
public:
    virtual ~IUtilsStage() = default;

    /** @brief Human-readable stage name (for logging/diagnostics). */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief Execute this stage.
     *
     * @return true on success; false causes the pipeline to record an error.
     */
    [[nodiscard]] virtual bool execute() = 0;

    /**
     * @brief Called by the pipeline during shutdown to release resources.
     *
     * Implementations must not throw.
     */
    virtual void teardown() noexcept = 0;
};

/**
 * @brief Composable utility-stage lifecycle manager.
 *
 * Stages are registered in order; run() executes them sequentially.
 * shutdown() tears down all registered stages in reverse registration order.
 */
class IUtilsPipeline {
public:
    virtual ~IUtilsPipeline() = default;

    /**
     * @brief Register a stage to be executed by run().
     *
     * Stages are executed in the order they are registered.
     *
     * @param stage  Owning pointer to the stage.
     */
    virtual void registerStage(std::unique_ptr<IUtilsStage> stage) = 0;

    /**
     * @brief Execute all registered stages in order.
     *
     * @return Future that resolves to a PipelineResult.
     */
    [[nodiscard]] virtual std::future<PipelineResult> run() = 0;

    /**
     * @brief Tear down all registered stages (reverse registration order).
     *
     * Must not throw.
     */
    virtual void shutdown() noexcept = 0;
};

} // namespace utils
} // namespace themis
