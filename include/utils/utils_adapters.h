/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            utils_adapters.h                                   ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-14 06:58:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     280                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 071d9ee897  2026-03-20  feat(utils): implement abstract interfaces for the utils ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file utils_adapters.h
 * @brief Concrete adapter classes that bridge existing utils implementations
 *        to the abstract interfaces defined in utils_interfaces.h.
 *
 * Adapters:
 *   - PIIStreamDetectorAdapter  — IStreamingPIIDetector over IPIIDetectionEngine
 *   - HashChainAuditLogAdapter  — IHashChainAuditLog over HashChainAuditWriter
 *   - HKDFKeyCacheAdapter       — IHKDFKeyCache over HKDFCache
 *   - SampledLoggerSamplerAdapter — IStructuredLogSampler over SampledLogger
 *   - SAGALogCompactorAdapter   — ISAGALogCompactor over SAGALogCompactor/Replayer
 *   - SequentialUtilsPipeline   — IUtilsPipeline (concrete sequential runner)
 */

#pragma once

#include "utils/utils_interfaces.h"
#include "utils/pii_detection_engine.h"
#include "utils/audit_logger.h"
#include "utils/hkdf_cache.h"
#include "utils/logger.h"
#include "utils/saga_logger.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace utils {

// ============================================================================
// PIIStreamDetectorAdapter — IStreamingPIIDetector
// ============================================================================

/**
 * @brief Stateless IStreamingPIIDetector adapter backed by IPIIDetectionEngine.
 *
 * Each detect() or pseudonymise() call creates a private PIIStreamScanner
 * instance so that no state is shared between invocations.  Thread-safe.
 */
class PIIStreamDetectorAdapter : public IStreamingPIIDetector {
public:
    /**
     * @param engine            Detection engine used for scanning.
     * @param lookaheadBytes    Cross-chunk lookahead buffer size (bytes).
     *                          Defaults to 256 (same as kDefaultLookaheadBytes
     *                          in pii_detection_engine.h).
     */
    explicit PIIStreamDetectorAdapter(
            std::shared_ptr<IPIIDetectionEngine> engine,
            size_t lookaheadBytes = 256);

    PIIDetectionResult detect(std::span<const std::byte> chunk) const override;

    SanitisedChunk pseudonymise(std::span<const std::byte> chunk) const override;

    std::span<const PIICategory> supportedCategories() const override;

private:
    std::shared_ptr<IPIIDetectionEngine> engine_;
    size_t lookaheadBytes_;
    /// Static list of all categories this adapter can detect.
    static const std::vector<PIICategory> kAllCategories;
};

// ============================================================================
// HashChainAuditLogAdapter — IHashChainAuditLog
// ============================================================================

/**
 * @brief IHashChainAuditLog adapter backed by HashChainAuditWriter.
 *
 * Provides append(), verifyChain(), query(), entryCount(), and lastEntryId()
 * on top of the existing HashChainAuditWriter + AuditLogVerifier pair.
 *
 * Thread-safe for concurrent appends.
 */
class HashChainAuditLogAdapter : public IHashChainAuditLog {
public:
    /**
     * @param cfg        Writer configuration (log path, chain-head path, fsync).
     * @param chainSeed  Hex seed for the genesis hash (HKDF-derived in production).
     */
    explicit HashChainAuditLogAdapter(
            HashChainAuditWriterConfig cfg = {},
            const std::string& chainSeed  = "");

    EntryId append(const AuditEvent& event) override;

    ChainVerifyResult verifyChain(EntryId from, EntryId to) const override;

    AuditCursor query(const AuditQuery& query) const override;

    size_t entryCount() const override;

    EntryId lastEntryId() const override;

private:
    HashChainAuditWriterConfig cfg_;
    HashChainAuditWriter writer_;
    AuditLogVerifier     verifier_;
};

// ============================================================================
// HKDFKeyCacheAdapter — IHKDFKeyCache
// ============================================================================

/**
 * @brief IHKDFKeyCache adapter backed by HKDFCache.
 *
 * Translates KeyContext to the ikm/salt/info/length signature expected by
 * HKDFCache::derive_cached().  TTL and max-size information is sourced from
 * the HKDFCacheConfig passed at construction time.
 */
class HKDFKeyCacheAdapter : public IHKDFKeyCache {
public:
    /**
     * @param cfg  Cache configuration (max_entries, ttl).
     */
    explicit HKDFKeyCacheAdapter(HKDFCacheConfig cfg = HKDFCacheConfig{});

    KeyHandle derive(const KeyContext& ctx) override;

    void evict(const KeyContext& ctx) override;

    void evictAll() override;

    std::chrono::milliseconds ttl(const KeyContext& ctx) const override;

    size_t cacheSize() const override;

    size_t maxCacheSize() const override;

private:
    HKDFCache      cache_;
    HKDFCacheConfig cfg_;

    /// Compute the SHA-256 hex string of the IKM bytes for purge_by_ikm_hash().
    static std::string ikmHash(const std::vector<uint8_t>& ikm);
};

// ============================================================================
// SampledLoggerSamplerAdapter — IStructuredLogSampler
// ============================================================================

/**
 * @brief IStructuredLogSampler adapter backed by SampledLogger.
 *
 * Security-class entries (EventClass::Security) always return true from
 * shouldSample(), bypassing the underlying rate limiter entirely.
 *
 * All interface methods are noexcept as required by the interface contract.
 */
class SampledLoggerSamplerAdapter : public IStructuredLogSampler {
public:
    /**
     * @param underlying  Logger instance to delegate sampled entries to.
     * @param cfg         Initial sampling configuration.
     */
    explicit SampledLoggerSamplerAdapter(
            std::shared_ptr<Logger>  underlying,
            SampledLoggerConfig      cfg = {});

    bool shouldSample(const LogEntry& entry) noexcept override;

    void recordDecision(const LogEntry& entry, bool sampled) noexcept override;

    double currentRate() const noexcept override;

    void setTargetRate(double rate) noexcept override;

    size_t sampledCount() const noexcept override;

    size_t droppedCount() const noexcept override;

private:
    SampledLogger        sampler_;
    SampledLoggerConfig  cfg_;
    std::atomic<size_t>  sampled_{0};
    std::atomic<size_t>  dropped_{0};
    std::atomic<double>  targetRate_{1.0};
    std::atomic<uint64_t> callCounter_{0};
};

// ============================================================================
// SAGALogCompactorAdapter — ISAGALogCompactor
// ============================================================================

/**
 * @brief Concrete ReplayIterator over a pre-loaded vector of SAGALogEntry.
 *
 * The entries are read eagerly from the WAL at construction time so that the
 * iterator is independent of the underlying file handles.
 */
class VectorReplayIterator : public ReplayIterator {
public:
    explicit VectorReplayIterator(std::vector<SAGALogEntry> entries);

    bool        hasNext() const override;
    SAGALogEntry next() override;
    void        reset() override;

private:
    std::vector<SAGALogEntry> entries_;
    size_t pos_{0};
};

/**
 * @brief ISAGALogCompactor adapter backed by SAGALogCompactor and SAGALogReplayer.
 *
 * compact() launches the compaction on a background thread and returns a future.
 * replay() reads all entries for the given segment ID into memory and returns a
 * VectorReplayIterator.
 */
class SAGALogCompactorAdapter : public ISAGALogCompactor {
public:
    explicit SAGALogCompactorAdapter(const SAGALoggerConfig& cfg);

    std::future<CompactionResult> compact(SegmentRange range) override;

    std::unique_ptr<ReplayIterator> replay(SegmentId segmentId) override;

private:
    SAGALoggerConfig cfg_;
};

// ============================================================================
// SequentialUtilsPipeline — IUtilsPipeline
// ============================================================================

/**
 * @brief Concrete IUtilsPipeline that runs stages sequentially on a background thread.
 *
 * run() dispatches a background task that executes all registered stages in
 * registration order.  shutdown() tears them down in reverse order and is
 * noexcept.
 */
class SequentialUtilsPipeline : public IUtilsPipeline {
public:
    SequentialUtilsPipeline() = default;
    ~SequentialUtilsPipeline() override;

    void registerStage(std::unique_ptr<IUtilsStage> stage) override;

    std::future<PipelineResult> run() override;

    void shutdown() noexcept override;

private:
    mutable std::mutex           mu_;
    std::vector<std::unique_ptr<IUtilsStage>> stages_;
    bool                         shutdownCalled_{false};
};

} // namespace utils
} // namespace themis
