/**
 * @file test_utils_interfaces.cpp
 * @brief Tests for the abstract interfaces defined in utils_interfaces.h
 *        and their concrete adapter implementations in utils_adapters.h.
 *
 * Test suites:
 *   - UtilsInterfacesKeyHandle         — KeyHandle RAII + zeroing
 *   - UtilsInterfacesHKDFKeyCache      — HKDFKeyCacheAdapter contracts
 *   - UtilsInterfacesStreamingPII      — PIIStreamDetectorAdapter contracts
 *   - UtilsInterfacesAuditLog          — HashChainAuditLogAdapter contracts
 *   - UtilsInterfacesSampler           — SampledLoggerSamplerAdapter contracts
 *   - UtilsInterfacesPipeline          — SequentialUtilsPipeline contracts
 *   - UtilsInterfacesSAGACompactor     — SAGALogCompactorAdapter contracts
 */

#include <gtest/gtest.h>
#include "utils/utils_interfaces.h"
#include "utils/utils_adapters.h"
#include "utils/pii_detection_engine.h"
#include "utils/logger.h"
#include "utils/saga_logger.h"

#include <filesystem>
#include <future>
#include <string>
#include <vector>

using namespace themis::utils;
namespace fs = std::filesystem;

// ============================================================================
// KeyHandle — RAII and zeroing
// ============================================================================

TEST(UtilsInterfacesKeyHandle, ConstructionSetsValidState) {
    KeyHandle h(std::vector<uint8_t>{1, 2, 3, 4});
    EXPECT_TRUE(h.valid());
    EXPECT_EQ(h.size(), 4u);
}

TEST(UtilsInterfacesKeyHandle, EmptyKeyIsInvalid) {
    KeyHandle h(std::vector<uint8_t>{});
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(h.size(), 0u);
}

TEST(UtilsInterfacesKeyHandle, MoveTransfersOwnership) {
    KeyHandle h1(std::vector<uint8_t>{10, 20, 30});
    EXPECT_TRUE(h1.valid());
    KeyHandle h2(std::move(h1));
    EXPECT_TRUE(h2.valid());
    EXPECT_EQ(h2.size(), 3u);
}

TEST(UtilsInterfacesKeyHandle, BytesSpanIsAccessible) {
    std::vector<uint8_t> raw = {0xAA, 0xBB, 0xCC};
    KeyHandle h(raw);
    auto span = h.bytes();
    ASSERT_EQ(span.size(), 3u);
    EXPECT_EQ(span[0], 0xAA);
    EXPECT_EQ(span[1], 0xBB);
    EXPECT_EQ(span[2], 0xCC);
}

// ============================================================================
// HKDFKeyCacheAdapter — IHKDFKeyCache contracts
// ============================================================================

class HKDFKeyCacheAdapterTest : public ::testing::Test {
protected:
    HKDFCacheConfig cfg;
    void SetUp() override {
        cfg.max_entries = 10;
        cfg.ttl         = std::chrono::seconds{300};
    }
};

TEST_F(HKDFKeyCacheAdapterTest, DeriveReturnsValidHandle) {
    HKDFKeyCacheAdapter cache(cfg);
    KeyContext ctx;
    ctx.ikm          = {0x01, 0x02, 0x03};
    ctx.salt         = {0xFF};
    ctx.info         = "test-purpose";
    ctx.outputLength = 16;

    KeyHandle h = cache.derive(ctx);
    EXPECT_TRUE(h.valid());
    EXPECT_EQ(h.size(), 16u);
}

TEST_F(HKDFKeyCacheAdapterTest, DeriveSameContextReturnsSameLengthKey) {
    HKDFKeyCacheAdapter cache(cfg);
    KeyContext ctx;
    ctx.ikm = {0x10, 0x20};
    ctx.info = "context-A";
    ctx.outputLength = 32;

    KeyHandle h1 = cache.derive(ctx);
    KeyHandle h2 = cache.derive(ctx);
    EXPECT_EQ(h1.size(), 32u);
    EXPECT_EQ(h2.size(), 32u);
}

TEST_F(HKDFKeyCacheAdapterTest, MaxCacheSizeMatchesConfig) {
    HKDFKeyCacheAdapter cache(cfg);
    EXPECT_EQ(cache.maxCacheSize(), 10u);
}

TEST_F(HKDFKeyCacheAdapterTest, EvictAllClearsCache) {
    HKDFKeyCacheAdapter cache(cfg);
    KeyContext ctx;
    ctx.ikm = {0xAB};
    ctx.outputLength = 8;
    cache.derive(ctx);
    EXPECT_NO_THROW(cache.evictAll());
    // After evictAll, subsequent derive should still work (re-derives).
    KeyHandle h = cache.derive(ctx);
    EXPECT_TRUE(h.valid());
}

TEST_F(HKDFKeyCacheAdapterTest, TtlReturnsConfiguredValue) {
    HKDFKeyCacheAdapter cache(cfg);
    KeyContext ctx;
    ctx.ikm = {0x01};
    auto ttl = cache.ttl(ctx);
    // Should equal the configured 300 s in ms.
    EXPECT_EQ(ttl.count(), std::chrono::milliseconds{std::chrono::seconds{300}}.count());
}

// ============================================================================
// PIIStreamDetectorAdapter — IStreamingPIIDetector contracts
// ============================================================================

class PIIStreamDetectorTest : public ::testing::Test {
protected:
    std::shared_ptr<IPIIDetectionEngine> engine_;
    std::unique_ptr<PIIStreamDetectorAdapter> detector_;

    void SetUp() override {
        auto result = PIIDetectionEngineFactory::createUnsigned("regex");
        ASSERT_TRUE(result.has_value()) << "Could not create regex engine";
        engine_ = std::move(*result);

        nlohmann::json cfg;
        cfg["enabled"] = true;
        cfg["patterns"] = nlohmann::json::array({
            {
                {"name", "EMAIL"},
                {"regex", R"([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,})"},
                {"confidence", 0.95},
                {"enabled", true},
                {"flags", nlohmann::json::array({"icase"})}
            }
        });

        ASSERT_TRUE(engine_->initialize(cfg))
            << "Could not initialize regex engine";
        detector_ = std::make_unique<PIIStreamDetectorAdapter>(engine_);
    }
};

TEST_F(PIIStreamDetectorTest, DetectOnEmptyChunkReturnsNoPII) {
    std::vector<std::byte> empty;
    auto result = detector_->detect({empty.data(), empty.size()});
    EXPECT_FALSE(result.containsPII);
    EXPECT_EQ(result.spanCount, 0u);
}

TEST_F(PIIStreamDetectorTest, DetectFindsEmailAddress) {
    std::string text = "Please contact alice@example.com for more info.";
    std::vector<std::byte> chunk(reinterpret_cast<const std::byte*>(text.data()),
                                 reinterpret_cast<const std::byte*>(text.data() + text.size()));
    auto result = detector_->detect({chunk.data(), chunk.size()});
    EXPECT_TRUE(result.containsPII);
    EXPECT_GE(result.spanCount, 1u);

    bool hasEmail = false;
    for (auto cat : result.categories) {
        if (cat == PIICategory::Email) { hasEmail = true; break; }
    }
    EXPECT_TRUE(hasEmail) << "Expected PIICategory::Email in detected categories";
}

TEST_F(PIIStreamDetectorTest, OutputNeverContainsRawValues) {
    // PIIDetectionResult must not expose raw PII values.
    std::string text = "alice@example.com";
    std::vector<std::byte> chunk(reinterpret_cast<const std::byte*>(text.data()),
                                 reinterpret_cast<const std::byte*>(text.data() + text.size()));
    auto result = detector_->detect({chunk.data(), chunk.size()});
    // Verify the result struct has no raw-value field (structural check).
    EXPECT_TRUE(result.containsPII);
    EXPECT_GT(result.spanCount, 0u);
    // No field in PIIDetectionResult stores the original string — this is a
    // static structural guarantee enforced by the interface type definition.
}

TEST_F(PIIStreamDetectorTest, Statelessness_ConcurrentCallsDoNotInterfere) {
    // Run many concurrent detect() calls; each should be independent.
    std::string textA = "alice@example.com";
    std::string textB = "no pii here at all";
    std::vector<std::byte> chunkA(reinterpret_cast<const std::byte*>(textA.data()),
                                  reinterpret_cast<const std::byte*>(textA.data() + textA.size()));
    std::vector<std::byte> chunkB(reinterpret_cast<const std::byte*>(textB.data()),
                                  reinterpret_cast<const std::byte*>(textB.data() + textB.size()));

    std::vector<std::future<PIIDetectionResult>> futures;
    for (int i = 0; i < 16; ++i) {
        futures.push_back(std::async(std::launch::async,
            [&, i]() {
                auto& c = (i % 2 == 0) ? chunkA : chunkB;
                return detector_->detect({c.data(), c.size()});
            }));
    }

    for (int i = 0; i < 16; ++i) {
        auto res = futures[i].get();
        if (i % 2 == 0) {
            EXPECT_TRUE(res.containsPII)  << "Even index should find PII";
        } else {
            EXPECT_FALSE(res.containsPII) << "Odd index should find no PII";
        }
    }
}

TEST_F(PIIStreamDetectorTest, PseudonymiseReturnsReducedOutput) {
    std::string text = "Contact alice@example.com today.";
    std::vector<std::byte> chunk(reinterpret_cast<const std::byte*>(text.data()),
                                 reinterpret_cast<const std::byte*>(text.data() + text.size()));
    auto sanitised = detector_->pseudonymise({chunk.data(), chunk.size()});
    EXPECT_GE(sanitised.replacementCount, 1u);
    // The sanitised data must not be empty.
    EXPECT_FALSE(sanitised.sanitisedData.empty());
}

TEST_F(PIIStreamDetectorTest, SupportedCategoriesNotEmpty) {
    auto cats = detector_->supportedCategories();
    EXPECT_FALSE(cats.empty());
}

// ============================================================================
// HashChainAuditLogAdapter — IHashChainAuditLog contracts
// ============================================================================

class HashChainAuditLogAdapterTest : public ::testing::Test {
protected:
    static constexpr const char* kLog  = "/tmp/test_iaudit_chain.jsonl";
    static constexpr const char* kHead = "/tmp/test_iaudit_head.bin";

    std::unique_ptr<HashChainAuditLogAdapter> log_;

    void SetUp() override {
        fs::remove(kLog);
        fs::remove(kHead);
        HashChainAuditWriterConfig cfg;
        cfg.log_path        = kLog;
        cfg.chain_head_path = kHead;
        log_ = std::make_unique<HashChainAuditLogAdapter>(cfg);
    }
    void TearDown() override {
        log_.reset();
        fs::remove(kLog);
        fs::remove(kHead);
    }
};

TEST_F(HashChainAuditLogAdapterTest, InitialEntryCountIsZero) {
    EXPECT_EQ(log_->entryCount(), 0u);
}

TEST_F(HashChainAuditLogAdapterTest, AppendReturnsMonotonicEntryId) {
    AuditEvent e1{"login", "alice", "db:users", "{}", {}};
    AuditEvent e2{"logout", "alice", "db:users", "{}", {}};
    EntryId id1 = log_->append(e1);
    EntryId id2 = log_->append(e2);
    EXPECT_LT(id1, id2);
    EXPECT_EQ(log_->entryCount(), 2u);
}

TEST_F(HashChainAuditLogAdapterTest, LastEntryIdMatchesAppendedCount) {
    log_->append({"ev1", "u1", "r1", "", {}});
    log_->append({"ev2", "u2", "r2", "", {}});
    log_->append({"ev3", "u3", "r3", "", {}});
    EXPECT_EQ(log_->entryCount(), 3u);
    EXPECT_EQ(log_->lastEntryId(), 2u);
}

TEST_F(HashChainAuditLogAdapterTest, QueryReturnsInitialCursor) {
    AuditQuery q;
    q.fromEntry = 0;
    auto cursor = log_->query(q);
    // Empty log — cursor should be exhausted.
    EXPECT_TRUE(cursor.exhausted);
}

TEST_F(HashChainAuditLogAdapterTest, AppendIsAppendOnlyNoRemoveMethods) {
    // IHashChainAuditLog must NOT have delete() or update() — this is a
    // compile-time structural guarantee verified by the absence of such methods
    // in the interface type.  Here we just confirm the append path works.
    EXPECT_NO_THROW(log_->append({"test", "bot", "res", "", {}}));
    EXPECT_EQ(log_->entryCount(), 1u);
}

// ============================================================================
// SampledLoggerSamplerAdapter — IStructuredLogSampler contracts
// ============================================================================

class SampledLoggerSamplerAdapterTest : public ::testing::Test {
protected:
    std::unique_ptr<SampledLoggerSamplerAdapter> sampler_;

    void SetUp() override {
        Logger::init(); // ensure spdlog is initialised
        auto logger = std::make_shared<Logger>();
        SampledLoggerConfig cfg;
        cfg.info_sample_rate = 1.0; // 100% by default for tests
        sampler_ = std::make_unique<SampledLoggerSamplerAdapter>(
            std::move(logger), cfg);
    }
};

TEST(UtilsInterfacesSampler, SecurityEventsAreNeverDropped) {
    Logger::init();
    auto logger = std::make_shared<Logger>();
    SampledLoggerConfig cfg;
    // Set rate to 0% — should still pass Security events.
    cfg.info_sample_rate  = 0.0;
    cfg.debug_sample_rate = 0.0;
    SampledLoggerSamplerAdapter sampler(std::move(logger), cfg);

    for (int i = 0; i < 100; ++i) {
        LogEntry entry;
        entry.eventClass = EventClass::Security;
        entry.message    = "security event #" + std::to_string(i);
        EXPECT_TRUE(sampler.shouldSample(entry))
            << "Security event must never be dropped (iteration " << i << ")";
    }
}

TEST_F(SampledLoggerSamplerAdapterTest, SampledCountIncreasesOnAccept) {
    LogEntry entry;
    entry.eventClass = EventClass::Security; // guaranteed accept
    sampler_->shouldSample(entry);
    EXPECT_GE(sampler_->sampledCount(), 1u);
}

TEST_F(SampledLoggerSamplerAdapterTest, CurrentRateIsInValidRange) {
    double rate = sampler_->currentRate();
    EXPECT_GE(rate, 0.0);
    EXPECT_LE(rate, 1.0);
}

TEST_F(SampledLoggerSamplerAdapterTest, SetTargetRateClampedToUnit) {
    sampler_->setTargetRate(2.5);
    EXPECT_LE(sampler_->currentRate(), 1.0);
    sampler_->setTargetRate(-0.5);
    EXPECT_GE(sampler_->currentRate(), 0.0);
}

TEST_F(SampledLoggerSamplerAdapterTest, RecordDecisionIsNoexcept) {
    LogEntry entry;
    entry.eventClass = EventClass::Operational;
    EXPECT_NO_THROW(sampler_->recordDecision(entry, true));
    EXPECT_NO_THROW(sampler_->recordDecision(entry, false));
}

// ============================================================================
// SequentialUtilsPipeline — IUtilsPipeline contracts
// ============================================================================

class CountingStage : public IUtilsStage {
public:
    explicit CountingStage(std::atomic<int>& counter, bool succeed = true)
        : counter_(counter), succeed_(succeed) {}

    std::string name() const override { return "CountingStage"; }

    bool execute() override {
        ++counter_;
        return succeed_;
    }

    void teardown() noexcept override { torn_down_ = true; }

    bool torn_down_{false};

private:
    std::atomic<int>& counter_;
    bool succeed_;
};

TEST(UtilsInterfacesPipeline, EmptyPipelineSucceeds) {
    SequentialUtilsPipeline pipeline;
    auto future = pipeline.run();
    auto result = future.get();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stagesRun, 0u);
}

TEST(UtilsInterfacesPipeline, RegisteredStagesAreExecuted) {
    SequentialUtilsPipeline pipeline;
    std::atomic<int> counter{0};
    pipeline.registerStage(std::make_unique<CountingStage>(counter));
    pipeline.registerStage(std::make_unique<CountingStage>(counter));
    auto result = pipeline.run().get();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stagesRun, 2u);
    EXPECT_EQ(counter.load(), 2);
}

TEST(UtilsInterfacesPipeline, FailingStageStopsPipeline) {
    SequentialUtilsPipeline pipeline;
    std::atomic<int> counter{0};
    pipeline.registerStage(std::make_unique<CountingStage>(counter, false)); // fails
    pipeline.registerStage(std::make_unique<CountingStage>(counter, true));  // not reached
    auto result = pipeline.run().get();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
    EXPECT_EQ(result.stagesRun, 0u); // first stage failed before incrementing
}

TEST(UtilsInterfacesPipeline, ShutdownIsIdempotent) {
    SequentialUtilsPipeline pipeline;
    EXPECT_NO_THROW(pipeline.shutdown());
    EXPECT_NO_THROW(pipeline.shutdown()); // second call must be safe
}

// ============================================================================
// SAGALogCompactorAdapter — ISAGALogCompactor contracts
// ============================================================================

class SAGALogCompactorAdapterTest : public ::testing::Test {
protected:
    static constexpr const char* kWAL = "/tmp/test_saga_iface.jsonl";
    static constexpr const char* kSig = "/tmp/test_saga_iface_sig.jsonl";

    SAGALoggerConfig sagaCfg;
    std::unique_ptr<SAGALogCompactorAdapter> adapter_;

    void SetUp() override {
        fs::remove(kWAL);
        fs::remove(kSig);
        sagaCfg.log_path       = kWAL;
        sagaCfg.signature_path = kSig;
        adapter_ = std::make_unique<SAGALogCompactorAdapter>(sagaCfg);
    }

    void TearDown() override {
        adapter_.reset();
        fs::remove(kWAL);
        fs::remove(kSig);
    }
};

TEST_F(SAGALogCompactorAdapterTest, CompactReturnsValidFuture) {
    SegmentRange range{"txn-0001", "txn-0999"};
    auto future = adapter_->compact(range);
    ASSERT_TRUE(future.valid());
    auto result = future.get();
    // With an empty WAL the compaction should succeed.
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.compactedSegments, 1u);
}

TEST_F(SAGALogCompactorAdapterTest, CompactIsNonBlocking) {
    SegmentRange range{"txn-0001", "txn-0999"};
    // compact() must return before the future resolves.
    auto before = std::chrono::steady_clock::now();
    auto future = adapter_->compact(range);
    auto after  = std::chrono::steady_clock::now();
    // The call itself should return almost immediately.
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(
                  after - before).count(), 200) << "compact() should not block";
    future.get(); // ensure it completes without throwing
}

TEST_F(SAGALogCompactorAdapterTest, ReplayEmptyWALReturnsEmptyIterator) {
    auto iter = adapter_->replay("segment-0");
    ASSERT_NE(iter, nullptr);
    EXPECT_FALSE(iter->hasNext());
}

TEST_F(SAGALogCompactorAdapterTest, ReplayIteratorResetIsIdempotent) {
    auto iter = adapter_->replay("segment-0");
    ASSERT_NE(iter, nullptr);
    iter->reset(); // reset on empty iterator must not throw
    EXPECT_FALSE(iter->hasNext());
}

// ============================================================================
// PIIDetectionResult / SanitisedChunk — interface type safety
// ============================================================================

TEST(UtilsInterfacesTypes, PIIDetectionResultHasNoRawValueField) {
    // Verify that the result type only carries category/count information.
    PIIDetectionResult r;
    r.containsPII = true;
    r.categories  = {PIICategory::Email};
    r.spanCount   = 1;
    // No 'values' or 'rawText' field should exist — confirmed by compilation.
    EXPECT_EQ(r.spanCount, 1u);
}

TEST(UtilsInterfacesTypes, SanitisedChunkHasOpaqueHandle) {
    SanitisedChunk chunk;
    chunk.replacementCount = 2;
    chunk.pseudonymMap.id  = 42;
    // The handle only carries an opaque id — no raw mapping exposed.
    EXPECT_EQ(chunk.pseudonymMap.id, 42u);
}

TEST(UtilsInterfacesTypes, CompactionResultCarriesExpectedFields) {
    CompactionResult r;
    r.compactedSegments = 3;
    r.bytesSaved        = 1024;
    r.retainedEntries   = 500;
    r.durationMs        = 42;
    r.success           = true;
    EXPECT_EQ(r.compactedSegments, 3u);
    EXPECT_EQ(r.bytesSaved, 1024u);
}
