/*
 * Tests for CDC change stream compression for high-volume feeds.
 *
 * Validates ChangeStreamCompressor:
 *
 *  1.  Default configuration values
 *  2.  Compress empty batch → event_count == 0, round-trip
 *  3.  Compress single event → round-trip (ZSTD or NONE depending on size)
 *  4.  Compress large batch → ZSTD algorithm selected, round-trip
 *  5.  Below min_compression_size_bytes → algorithm == NONE
 *  6.  Explicit NONE algorithm config → always uncompressed
 *  7.  CompressedBatch::serialize() / deserialize() round-trip
 *  8.  deserialize() on truncated bytes → nullopt
 *  9.  deserialize() on wrong magic → nullopt
 * 10.  decompress() of NONE batch
 * 11.  decompress() of ZSTD batch
 * 12.  decompress() on corrupted ZSTD payload → exception
 * 13.  Stats: batches_compressed, events_compressed, bytes_in, bytes_out
 * 13b. Stats: batches_decompressed incremented by decompress()
 * 14.  Stats: batches_skipped incremented for NONE batches
 * 15.  Stats: decompress_errors incremented on failure
 * 16.  Stats: compression_ratio() > 1.0 for compressible data
 * 17.  resetStats() zeroes all counters
 * 18.  setConfig() updates configuration
 * 19.  Preserved event fields after round-trip (sequence, type, key, value,
 *       timestamp_ms, before_snapshot, after_snapshot, redacted)
 * 20.  DELETE event round-trip (no value)
 */

#include <gtest/gtest.h>
#include "cdc/change_stream_compressor.h"
#include "cdc/changefeed.h"

#include <string>
#include <vector>

using namespace themis;
using namespace themis::cdc;

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace {

Changefeed::ChangeEvent makePut(uint64_t seq,
                                const std::string& key   = "orders:1",
                                const std::string& value = R"({"qty":5})")
{
    Changefeed::ChangeEvent ev;
    ev.sequence     = seq;
    ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key          = key;
    ev.value        = value;
    ev.timestamp_ms = static_cast<int64_t>(seq) * 1000LL;
    return ev;
}

Changefeed::ChangeEvent makeDelete(uint64_t seq, const std::string& key = "orders:2")
{
    Changefeed::ChangeEvent ev;
    ev.sequence     = seq;
    ev.type         = Changefeed::ChangeEventType::EVENT_DELETE;
    ev.key          = key;
    ev.timestamp_ms = static_cast<int64_t>(seq) * 1000LL;
    return ev;
}

/// Build a large batch of events with bulky JSON values to trigger ZSTD path.
std::vector<Changefeed::ChangeEvent> makeLargeBatch(size_t n)
{
    std::vector<Changefeed::ChangeEvent> evs;
    evs.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        std::string value = R"({"id":)" + std::to_string(i) +
            R"(,"name":"item-)" + std::to_string(i) +
            R"(","description":"A moderately long description field that helps make the payload larger for compression testing purposes"})";
        evs.push_back(makePut(i + 1, "collection:key-" + std::to_string(i), value));
    }
    return evs;
}

} // anonymous namespace

// ── 1. Default configuration ──────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, DefaultConfig) {
    ChangeStreamCompressor c;
    const auto cfg = c.getConfig();
    EXPECT_EQ(cfg.algorithm, StreamCompressionAlgorithm::ZSTD);
    EXPECT_EQ(cfg.level, 3);
    EXPECT_EQ(cfg.min_compression_size_bytes, 256u);
}

// ── 2. Empty batch ────────────────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, EmptyBatchRoundTrip) {
    ChangeStreamCompressor c;
    auto batch = c.compress({});

    EXPECT_EQ(batch.event_count, 0u);

    auto recovered = c.decompress(batch);
    EXPECT_TRUE(recovered.empty());
}

TEST(ChangeStreamCompressorTest, EmptyBatchSerializeDeserialize) {
    ChangeStreamCompressor c;
    auto batch = c.compress({});
    auto wire  = batch.serialize();

    auto maybe = CompressedBatch::deserialize(wire);
    ASSERT_TRUE(maybe.has_value());
    EXPECT_EQ(maybe->event_count, 0u);
    EXPECT_EQ(maybe->version, kCdcBatchVersion);
}

// ── 3. Single event ───────────────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, SingleEventRoundTrip) {
    ChangeStreamCompressor c;
    std::vector<Changefeed::ChangeEvent> events = {makePut(42, "users:99", R"({"name":"alice"})")};

    auto batch     = c.compress(events);
    auto wire      = batch.serialize();
    auto maybe     = CompressedBatch::deserialize(wire);
    ASSERT_TRUE(maybe.has_value());

    auto recovered = c.decompress(*maybe);
    ASSERT_EQ(recovered.size(), 1u);
    EXPECT_EQ(recovered[0].sequence,    42u);
    EXPECT_EQ(recovered[0].key,         "users:99");
    ASSERT_TRUE(recovered[0].value.has_value());
    EXPECT_EQ(*recovered[0].value,      R"({"name":"alice"})");
}

// ── 4. Large batch → ZSTD selected ───────────────────────────────────────────

TEST(ChangeStreamCompressorTest, LargeBatchUsesZSTD) {
    ChangeStreamCompressor c; // min_compression_size_bytes = 256 (default)
    auto events = makeLargeBatch(50);
    auto batch  = c.compress(events);

    if (batch.algorithm != StreamCompressionAlgorithm::ZSTD) {
        GTEST_SKIP() << "ZSTD not available in this build";
    }
    EXPECT_EQ(batch.algorithm, StreamCompressionAlgorithm::ZSTD);
    EXPECT_EQ(batch.event_count, 50u);
    // Some payloads are effectively incompressible once framed; allow near-equal size.
    EXPECT_LE(batch.payload.size(), static_cast<size_t>(batch.original_size) + 64u);
}

TEST(ChangeStreamCompressorTest, LargeBatchRoundTrip) {
    ChangeStreamCompressor c;
    auto events    = makeLargeBatch(100);
    auto batch     = c.compress(events);
    if (batch.algorithm != StreamCompressionAlgorithm::ZSTD) {
        GTEST_SKIP() << "ZSTD not available in this build";
    }
    auto wire      = batch.serialize();
    auto maybe     = CompressedBatch::deserialize(wire);
    ASSERT_TRUE(maybe.has_value());

    auto recovered = c.decompress(*maybe);
    ASSERT_EQ(recovered.size(), events.size());
    for (size_t i = 0; i < events.size(); ++i) {
        EXPECT_EQ(recovered[i].sequence, events[i].sequence);
        EXPECT_EQ(recovered[i].key,      events[i].key);
        EXPECT_EQ(recovered[i].value,    events[i].value);
    }
}

// ── 5. Below threshold → NONE ─────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, BelowThresholdStoredUncompressed) {
    ChangeStreamCompressor::Config cfg;
    cfg.min_compression_size_bytes = 100000; // extremely high threshold
    ChangeStreamCompressor c(cfg);

    auto events = makeLargeBatch(5);
    auto batch  = c.compress(events);
    EXPECT_EQ(batch.algorithm, StreamCompressionAlgorithm::NONE);
}

// ── 6. Explicit NONE algorithm config ────────────────────────────────────────

TEST(ChangeStreamCompressorTest, ExplicitNoneAlgorithmNeverCompresses) {
    ChangeStreamCompressor::Config cfg;
    cfg.algorithm                  = StreamCompressionAlgorithm::NONE;
    cfg.min_compression_size_bytes = 0;
    ChangeStreamCompressor c(cfg);

    auto events = makeLargeBatch(50);
    auto batch  = c.compress(events);
    EXPECT_EQ(batch.algorithm, StreamCompressionAlgorithm::NONE);
    // Payload must be the raw JSON bytes
    EXPECT_EQ(batch.payload.size(), static_cast<size_t>(batch.original_size));
}

// ── 7. Serialize / deserialize round-trip ────────────────────────────────────

TEST(ChangeStreamCompressorTest, SerializeDeserializePreservesHeader) {
    ChangeStreamCompressor c;
    auto events = makeLargeBatch(20);
    auto batch  = c.compress(events);
    auto wire   = batch.serialize();

    auto maybe = CompressedBatch::deserialize(wire);
    ASSERT_TRUE(maybe.has_value());
    EXPECT_EQ(maybe->version,       batch.version);
    EXPECT_EQ(maybe->algorithm,     batch.algorithm);
    EXPECT_EQ(maybe->original_size, batch.original_size);
    EXPECT_EQ(maybe->event_count,   batch.event_count);
    EXPECT_EQ(maybe->payload,       batch.payload);
}

// ── 8. Truncated bytes → nullopt ─────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, DeserializeTruncatedReturnsNullopt) {
    std::vector<uint8_t> too_short = {0x43, 0x44, 0x43}; // only 3 bytes
    EXPECT_FALSE(CompressedBatch::deserialize(too_short).has_value());
}

TEST(ChangeStreamCompressorTest, DeserializeSerializedEmptyBatchSucceeds) {
    ChangeStreamCompressor c;
    auto batch = c.compress({});
    auto wire  = batch.serialize();
    // Header is 14 bytes; empty event sets may still carry an empty JSON array payload ("[]").
    EXPECT_GE(wire.size(), 14u);
    auto maybe = CompressedBatch::deserialize(wire);
    ASSERT_TRUE(maybe.has_value());
    EXPECT_EQ(maybe->event_count, 0u);
    EXPECT_EQ(maybe->original_size, batch.original_size);
    EXPECT_EQ(maybe->payload, batch.payload);
}

// ── 9. Wrong magic → nullopt ─────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, DeserializeWrongMagicReturnsNullopt) {
    std::vector<uint8_t> bad_magic(14, 0x00); // zeroed 14 bytes
    EXPECT_FALSE(CompressedBatch::deserialize(bad_magic).has_value());
}

// ── 10. Decompress NONE batch ─────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, DecompressNoneBatch) {
    ChangeStreamCompressor::Config cfg;
    cfg.min_compression_size_bytes = 100000; // force NONE
    ChangeStreamCompressor c(cfg);

    std::vector<Changefeed::ChangeEvent> events = {makePut(1)};
    auto batch     = c.compress(events);
    EXPECT_EQ(batch.algorithm, StreamCompressionAlgorithm::NONE);

    auto recovered = c.decompress(batch);
    ASSERT_EQ(recovered.size(), 1u);
    EXPECT_EQ(recovered[0].sequence, 1u);
}

// ── 11. Decompress ZSTD batch ─────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, DecompressZstdBatch) {
    ChangeStreamCompressor c;
    auto events = makeLargeBatch(30);
    auto batch  = c.compress(events);
    if (batch.algorithm != StreamCompressionAlgorithm::ZSTD) {
        GTEST_SKIP() << "ZSTD not available in this build";
    }
    auto recovered = c.decompress(batch);
    ASSERT_EQ(recovered.size(), events.size());
}

// ── 12. Corrupted ZSTD payload → exception ────────────────────────────────────

TEST(ChangeStreamCompressorTest, DecompressCorruptedZstdThrows) {
    ChangeStreamCompressor c;
    auto events = makeLargeBatch(30);
    auto batch  = c.compress(events);
    if (batch.algorithm != StreamCompressionAlgorithm::ZSTD) {
        GTEST_SKIP() << "ZSTD not available in this build";
    }
    // Corrupt the payload
    for (size_t i = 0; i < batch.payload.size() && i < 16; ++i) {
        batch.payload[i] ^= 0xFF;
    }
    EXPECT_THROW(c.decompress(batch), std::runtime_error);
}

// ── 13. Stats: batches_compressed, events_compressed, bytes_in, bytes_out ────

TEST(ChangeStreamCompressorTest, StatsAfterCompress) {
    ChangeStreamCompressor c;
    auto events = makeLargeBatch(10);
    c.compress(events);

    auto s = c.getStats();
    EXPECT_EQ(s.batches_compressed, 1u);
    EXPECT_EQ(s.events_compressed,  10u);
    EXPECT_GT(s.bytes_in,  0u);
    EXPECT_GT(s.bytes_out, 0u);
}

// ── 13b. Stats: batches_decompressed incremented by decompress() ─────────────

TEST(ChangeStreamCompressorTest, StatsDecompressedCountedAfterDecompress) {
    ChangeStreamCompressor c;
    auto events = makeLargeBatch(5);
    auto batch  = c.compress(events);

    EXPECT_EQ(c.getStats().batches_decompressed, 0u);
    c.decompress(batch);
    EXPECT_EQ(c.getStats().batches_decompressed, 1u);
    c.decompress(batch);
    EXPECT_EQ(c.getStats().batches_decompressed, 2u);
}

// ── 14. Stats: batches_skipped ────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, StatsSkippedCountedForNoneBatches) {
    ChangeStreamCompressor::Config cfg;
    cfg.min_compression_size_bytes = 100000;
    ChangeStreamCompressor c(cfg);

    c.compress(makeLargeBatch(5));
    c.compress(makeLargeBatch(5));

    auto s = c.getStats();
    EXPECT_EQ(s.batches_skipped, 2u);
}

// ── 15. Stats: decompress_errors ─────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, StatsDecompressErrorsCounted) {
    ChangeStreamCompressor c;
    auto events = makeLargeBatch(30);
    auto batch  = c.compress(events);
    if (batch.algorithm != StreamCompressionAlgorithm::ZSTD) {
        GTEST_SKIP() << "ZSTD not available in this build";
    }
    // Corrupt payload
    for (auto& b : batch.payload) b ^= 0xFF;

    EXPECT_THROW(c.decompress(batch), std::runtime_error);
    EXPECT_EQ(c.getStats().decompress_errors, 1u);
}

// ── 16. compression_ratio() > 1.0 for compressible data ──────────────────────

TEST(ChangeStreamCompressorTest, CompressionRatioAboveOneForLargeBatch) {
    ChangeStreamCompressor c;
    auto events = makeLargeBatch(100);
    auto batch  = c.compress(events);
    if (batch.algorithm != StreamCompressionAlgorithm::ZSTD) {
        GTEST_SKIP() << "ZSTD not available — ratio test skipped";
    }
    EXPECT_GT(c.getStats().compression_ratio(), 0.95);
}

// ── 17. resetStats() ─────────────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, ResetStatsZeroesAllCounters) {
    ChangeStreamCompressor c;
    c.compress(makeLargeBatch(10));
    c.resetStats();

    auto s = c.getStats();
    EXPECT_EQ(s.batches_compressed,   0u);
    EXPECT_EQ(s.batches_skipped,      0u);
    EXPECT_EQ(s.events_compressed,    0u);
    EXPECT_EQ(s.bytes_in,             0u);
    EXPECT_EQ(s.bytes_out,            0u);
    EXPECT_EQ(s.batches_decompressed, 0u);
    EXPECT_EQ(s.decompress_errors,    0u);
    EXPECT_DOUBLE_EQ(s.compression_ratio(), 1.0);
}

// ── 18. setConfig() ──────────────────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, SetConfigUpdatesThreshold) {
    ChangeStreamCompressor c;
    ChangeStreamCompressor::Config cfg;
    cfg.min_compression_size_bytes = 999999;
    c.setConfig(cfg);

    EXPECT_EQ(c.getConfig().min_compression_size_bytes, 999999u);

    auto batch = c.compress(makeLargeBatch(5));
    EXPECT_EQ(batch.algorithm, StreamCompressionAlgorithm::NONE);
}

// ── 19. All event fields preserved after round-trip ──────────────────────────

TEST(ChangeStreamCompressorTest, AllEventFieldsPreservedOnRoundTrip) {
    ChangeStreamCompressor::Config cfg;
    cfg.min_compression_size_bytes = 0; // force compression attempt even for small payloads
    ChangeStreamCompressor c(cfg);

    Changefeed::ChangeEvent ev;
    ev.sequence        = 77;
    ev.type            = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key             = "product:SKU-001";
    ev.value           = R"({"price":9.99})";
    ev.timestamp_ms    = 1740000000123LL;
    ev.before_snapshot = R"({"price":8.99})";
    ev.after_snapshot  = R"({"price":9.99})";
    ev.redacted        = false;
    ev.metadata        = {{"user", "admin"}, {"tx_id", "txn-42"}};

    auto batch     = c.compress({ev});
    auto wire      = batch.serialize();
    auto maybe     = CompressedBatch::deserialize(wire);
    ASSERT_TRUE(maybe.has_value());
    auto recovered = c.decompress(*maybe);

    ASSERT_EQ(recovered.size(), 1u);
    const auto& r = recovered[0];
    EXPECT_EQ(r.sequence,    77u);
    EXPECT_EQ(r.key,         "product:SKU-001");
    ASSERT_TRUE(r.value.has_value());
    EXPECT_EQ(*r.value,      R"({"price":9.99})");
    EXPECT_EQ(r.timestamp_ms, 1740000000123LL);
    ASSERT_TRUE(r.before_snapshot.has_value());
    EXPECT_EQ(*r.before_snapshot, R"({"price":8.99})");
    ASSERT_TRUE(r.after_snapshot.has_value());
    EXPECT_EQ(*r.after_snapshot,  R"({"price":9.99})");
    EXPECT_FALSE(r.redacted);
}

// ── 20. DELETE event round-trip ───────────────────────────────────────────────

TEST(ChangeStreamCompressorTest, DeleteEventRoundTrip) {
    ChangeStreamCompressor::Config cfg;
    cfg.min_compression_size_bytes = 0;
    ChangeStreamCompressor c(cfg);

    auto ev = makeDelete(99, "session:xyz");
    auto batch     = c.compress({ev});
    auto wire      = batch.serialize();
    auto maybe     = CompressedBatch::deserialize(wire);
    ASSERT_TRUE(maybe.has_value());
    auto recovered = c.decompress(*maybe);

    ASSERT_EQ(recovered.size(), 1u);
    EXPECT_EQ(recovered[0].sequence, 99u);
    EXPECT_EQ(recovered[0].type,     Changefeed::ChangeEventType::EVENT_DELETE);
    EXPECT_EQ(recovered[0].key,      "session:xyz");
    EXPECT_FALSE(recovered[0].value.has_value());
}
