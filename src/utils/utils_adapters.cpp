/**
 * @file utils_adapters.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/utils_adapters.h"
#include "utils/lek_manager.h"

#include <openssl/sha.h>

#include <algorithm>
#include <fstream>
#include <future>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace themis {
namespace utils {

// ============================================================================
// PIIStreamDetectorAdapter
// ============================================================================

const std::vector<PIICategory> PIIStreamDetectorAdapter::kAllCategories = {
    PIICategory::Email,
    PIICategory::Phone,
    PIICategory::SocialSecurityNumber,
    PIICategory::CreditCard,
    PIICategory::IBAN,
    PIICategory::IPAddress,
    PIICategory::URL,
    PIICategory::PersonName,
    PIICategory::Location,
    PIICategory::Organization,
    PIICategory::Other,
};

PIIStreamDetectorAdapter::PIIStreamDetectorAdapter(
        std::shared_ptr<IPIIDetectionEngine> engine,
        size_t lookaheadBytes)
    : engine_(std::move(engine))
    , lookaheadBytes_(lookaheadBytes)
{}

PIIDetectionResult PIIStreamDetectorAdapter::detect(
        std::span<const std::byte> chunk) const
{
    // Stateless: create a fresh scanner per call.
    PIIStreamScannerConfig cfg;
    cfg.lookahead_bytes = lookaheadBytes_;
    PIIStreamScanner scanner(engine_, cfg);

    std::string_view sv(reinterpret_cast<const char*>(chunk.data()),static_cast<int>(chunk.size()));
    auto findings = scanner.scan_chunk(sv, /*is_last=*/true);

    PIIDetectionResult result;
    result.containsPII = !findings.empty();
    result.spanCount   = findings.size();

    // Collect distinct categories without exposing raw values.
    for (const auto& f : findings) {
        PIICategory cat = PIICategory::Other;
        switch (f.type) {
            case PIIType::EMAIL:        cat = PIICategory::Email;             break;
            case PIIType::PHONE:        cat = PIICategory::Phone;             break;
            case PIIType::SSN:          cat = PIICategory::SocialSecurityNumber; break;
            case PIIType::CREDIT_CARD:  cat = PIICategory::CreditCard;        break;
            case PIIType::IBAN:         cat = PIICategory::IBAN;              break;
            case PIIType::IP_ADDRESS:   cat = PIICategory::IPAddress;         break;
            case PIIType::URL:          cat = PIICategory::URL;               break;
            case PIIType::PERSON_NAME:  cat = PIICategory::PersonName;        break;
            case PIIType::LOCATION:     cat = PIICategory::Location;          break;
            case PIIType::ORGANIZATION: cat = PIICategory::Organization;      break;
            default:                    cat = PIICategory::Other;             break;
        }
        // Add only if not already present.
        if (std::find(result.categories.begin(), result.categories.end(), cat)
                == result.categories.end()) {
            result.categories.push_back(cat);
        }
    }

    return result;
}

SanitisedChunk PIIStreamDetectorAdapter::pseudonymise(
        std::span<const std::byte> chunk) const
{
    // Stateless: create a fresh scanner per call.
    PIIStreamScannerConfig cfg;
    cfg.lookahead_bytes = lookaheadBytes_;
    PIIStreamScanner scanner(engine_, cfg);

    std::string_view sv(reinterpret_cast<const char*>(chunk.data()),static_cast<int>(chunk.size()));
    auto findings = scanner.scan_chunk(sv, /*is_last=*/true);

    // Build the sanitised output by masking each finding.
    std::string text(sv);
    size_t offset_shift = 0; // track length changes from replacements
    for (const auto& f : findings) {
        // Replace the span with a safe placeholder token.
        const std::string placeholder = "[REDACTED]";
        size_t start = f.start_offset + offset_shift;
        size_t len   = f.end_offset - f.start_offset;
        if (start <static_cast<int>(text.size()) && start + len <= text.size()) {
            text.replace(start, len, placeholder);
            offset_shift += static_cast<int>(placeholder.size()) - len;
        }
    }

    SanitisedChunk result;
    result.sanitisedData.resize(text.size());
    std::transform(text.begin(), text.end(), result.sanitisedData.begin(),
                   [](char c) { return static_cast<std::byte>(c); });
    result.replacementCount  = findings.size();
    result.pseudonymMap.id   = 0; // Opaque placeholder.

    return result;
}

std::span<const PIICategory> PIIStreamDetectorAdapter::supportedCategories() const {
    return {kAllCategories.data(),static_cast<int>(kAllCategories.size())};
}

// ============================================================================
// HashChainAuditLogAdapter
// ============================================================================

HashChainAuditLogAdapter::HashChainAuditLogAdapter(
        HashChainAuditWriterConfig cfg,
        const std::string& chainSeed)
    : cfg_(cfg)
    , writer_(cfg, chainSeed)
{}

EntryId HashChainAuditLogAdapter::append(const AuditEvent& event) {
    nlohmann::json record;
    record["event_type"]  = event.eventType;
    record["actor_id"]    = event.actorId;
    record["resource_id"] = event.resourceId;
    record["payload"]     = event.payload;
    record["ts_epoch_ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                                event.timestamp.time_since_epoch()).count();

    writer_.write(record);
    return writer_.sequenceNumber() - 1; // write() post-increments seq
}

ChainVerifyResult HashChainAuditLogAdapter::verifyChain(
        EntryId /*from*/, EntryId /*to*/) const
{
    auto raw = verifier_.verify_chain(cfg_.log_path);
    // AuditVerifyResult → ChainVerifyResult
    ChainVerifyResult result;
    result.valid         = raw.ok;
    result.verifiedCount = static_cast<size_t>(raw.entries_ok);
    result.errorMessage  = raw.error_message;
    if (!raw.ok && raw.first_bad_seq != UINT64_MAX) {
        result.firstTamperedEntry = static_cast<EntryId>(raw.first_bad_seq);
    }
    return result;
}

AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
    AuditCursor cursor;
    cursor.position  = query.fromEntry.value_or(0);
    cursor.exhausted = (cursor.position >= writer_.sequenceNumber());
    return cursor;
}

size_t HashChainAuditLogAdapter::entryCount() const {
    return static_cast<size_t>(writer_.sequenceNumber());
}

EntryId HashChainAuditLogAdapter::lastEntryId() const {
    uint64_t seq = writer_.sequenceNumber();
    return seq > 0 ? seq - 1 : 0;
}

// ============================================================================
// HKDFKeyCacheAdapter
// ============================================================================

HKDFKeyCacheAdapter::HKDFKeyCacheAdapter(HKDFCacheConfig cfg)
    : cache_(cfg), cfg_(cfg)
{}

KeyHandle HKDFKeyCacheAdapter::derive(const KeyContext& ctx) {
    auto bytes = cache_.derive_cached(ctx.ikm, ctx.salt, ctx.info, ctx.outputLength);
    return KeyHandle(std::move(bytes));
}

void HKDFKeyCacheAdapter::evict(const KeyContext& ctx) {
    cache_.purge_by_ikm_hash(ikmHash(ctx.ikm));
}

void HKDFKeyCacheAdapter::evictAll() {
    cache_.clear();
}

std::chrono::milliseconds HKDFKeyCacheAdapter::ttl(const KeyContext& /*ctx*/) const {
    // HKDFCache does not expose per-entry remaining TTL; return the configured
    // maximum as a conservative upper bound.  Returns 0 when TTL is disabled.
    if (cfg_.ttl.count() == 0) return std::chrono::milliseconds{0};
    return std::chrono::duration_cast<std::chrono::milliseconds>(cfg_.ttl);
}

size_t HKDFKeyCacheAdapter::cacheSize() const {
    // HKDFCache::stats() tracks evictions but not live entry count.
    // Return 0 as a safe default; implementations can override for precision.
    return 0;
}

size_t HKDFKeyCacheAdapter::maxCacheSize() const {
    return cfg_.max_entries;
}

// static
std::string HKDFKeyCacheAdapter::ikmHash(const std::vector<uint8_t>& ikm) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(ikm.data(),static_cast<int>(ikm.size()), digest);

    std::ostringstream oss = {};
    for (unsigned char byte : digest) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte);
    }
    return oss.str();
}

// ============================================================================
// SampledLoggerSamplerAdapter
// ============================================================================

SampledLoggerSamplerAdapter::SampledLoggerSamplerAdapter(
        std::shared_ptr<Logger>  underlying,
        SampledLoggerConfig      cfg)
    : sampler_(std::move(underlying), cfg)
    , cfg_(cfg)
    , targetRate_(cfg.info_sample_rate)
{}

bool SampledLoggerSamplerAdapter::shouldSample(const LogEntry& entry) noexcept {
    // Security contract: never suppress security events.
    if (entry.eventClass == EventClass::Security) {
        ++sampled_;
        return true;
    }

    // Derive effective rate from configured per-level rates.
    double rate = targetRate_.load(std::memory_order_relaxed);
    // If rate == 1.0 always sample; if rate == 0.0 always drop.
    bool accepted = (rate >= 1.0);
    if (!accepted && rate > 0.0) {
        // Counter-based approximate sampling: accept every ceil(1/rate)-th call.
        uint64_t c = callCounter_.fetch_add(1, std::memory_order_relaxed);
        uint64_t period = static_cast<uint64_t>(1.0 / rate + 0.5);
        if (period == 0) {
          period = 1;
        }
        accepted = (c % period == 0);
    }

    if (accepted) {
        ++sampled_;
    } else {
        ++dropped_;
    }
    return accepted;
}

void SampledLoggerSamplerAdapter::recordDecision(
        const LogEntry& /*entry*/, [[maybe_unused]] bool sampled) noexcept
{
    // No adaptive feedback in this implementation; stats already tracked in
    // shouldSample().  If sampled is inconsistent with internal tracking the
    // caller's explicit feedback takes precedence.
}

double SampledLoggerSamplerAdapter::currentRate() const noexcept {
    return targetRate_.load(std::memory_order_relaxed);
}

void SampledLoggerSamplerAdapter::setTargetRate([[maybe_unused]] double rate) noexcept {
    rate = std::max(0.0, std::min(1.0, rate));
    targetRate_.store(rate, std::memory_order_relaxed);

    SampledLoggerConfig updated = cfg_;
    updated.debug_sample_rate = rate;
    updated.info_sample_rate  = rate;
    sampler_.set_config(updated);
}

size_t SampledLoggerSamplerAdapter::sampledCount() const noexcept {
    return sampled_.load(std::memory_order_relaxed);
}

size_t SampledLoggerSamplerAdapter::droppedCount() const noexcept {
    return dropped_.load(std::memory_order_relaxed);
}

// ============================================================================
// VectorReplayIterator
// ============================================================================

VectorReplayIterator::VectorReplayIterator(std::vector<SAGALogEntry> entries)
    : entries_(std::move(entries)), pos_(0)
{}

bool VectorReplayIterator::hasNext() const {
    return static_cast<bool>(pos_  < static_cast<int>(entries_.size()));
}

SAGALogEntry VectorReplayIterator::next() {
    return entries_[pos_++];
}

void VectorReplayIterator::reset() {
    pos_ = 0;
}

// ============================================================================
// SAGALogCompactorAdapter
// ============================================================================

SAGALogCompactorAdapter::SAGALogCompactorAdapter(const SAGALoggerConfig& cfg)
    : cfg_(cfg)
{}

std::future<CompactionResult> SAGALogCompactorAdapter::compact(SegmentRange range) {
    return std::async(std::launch::async, [this, range]() {
        auto start = std::chrono::steady_clock::now();
        CompactionResult result;
        try {
            SAGALogCompactor compactor(cfg_);
            // Use toTxnId as the upper boundary for the existing API.
            size_t archived = compactor.compact(range.toTxnId);
            result.retainedEntries   = archived; // archived ≈ retained after compaction
            result.compactedSegments = 1;
            result.success           = true;
        } catch (const std::exception& e) {
            result.success      = false;
            result.errorMessage = e.what();
        }
        auto end = std::chrono::steady_clock::now();
        result.durationMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        return result;
    });
}

std::unique_ptr<ReplayIterator> SAGALogCompactorAdapter::replay(
        SegmentId /*segmentId*/)
{
    std::vector<SAGALogEntry> entries;
    SAGALogReplayer replayer(cfg_);
    replayer.replay_incomplete([&entries](const SAGAStep& step) {
        SAGALogEntry entry;
        entry.sagaId      = step.saga_id;
        entry.stepName    = step.step_name;
        entry.action      = step.action;
        entry.status      = step.status;
        entry.payloadJson = step.payload.dump();
        entry.timestamp   = step.timestamp;
        entries.push_back(std::move(entry));
    });
    return std::make_unique<VectorReplayIterator>(std::move(entries));
}

// ============================================================================
// SequentialUtilsPipeline
// ============================================================================

SequentialUtilsPipeline::~SequentialUtilsPipeline() {
    shutdown();
}

void SequentialUtilsPipeline::registerStage(std::unique_ptr<IUtilsStage> stage) {
    std::lock_guard<std::mutex> lock(mu_);
    stages_.push_back(std::move(stage));
}

std::future<PipelineResult> SequentialUtilsPipeline::run() {
    // Snapshot stage pointers under the lock; execution is off-lock.
    std::vector<IUtilsStage*> snapshot;
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& s : stages_) {
          snapshot.push_back(s.get());
        }
    }

    return std::async(std::launch::async, [snapshot]() {
        PipelineResult result;
        result.success = true;
        for (IUtilsStage* stage : snapshot) {
            if (!stage->execute()) {
                result.success      = false;
                result.errorMessage = "Stage '" + stage->name() + "' failed.";
                break;
            }
            ++result.stagesRun;
        }
        return result;
    });
}

void SequentialUtilsPipeline::shutdown() noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    if (shutdownCalled_) {
      return;
    }
    shutdownCalled_ = true;
    // Tear down in reverse registration order.
    for (auto it = stages_.rbegin(); it != stages_.rend(); ++it) {
        (*it)->teardown();
    }
}

} // namespace utils
} // namespace themis

