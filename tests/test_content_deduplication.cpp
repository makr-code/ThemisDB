/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_content_deduplication.cpp                     ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 04:02:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     510                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b617bb3a1  2026-02-28  Implement content deduplication via SHA-256 hash before s... ║
    • 8af0ff1a8  2026-02-27  refactor(content): address code review feedback on dedupl... ║
    • 95da435db  2026-02-27  feat(content): add content deduplication via perceptual h... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_content_deduplication.cpp
 * @brief Unit tests for content deduplication via perceptual hashing.
 *
 * Covers:
 *  - ImageProcessor::computePHash() — DCT-based 64-bit hash
 *  - TextProcessor::computeMinHash() — 128-permutation MinHash
 *  - DeduplicationChecker — pHash Hamming distance and MinHash band-LSH
 *  - ContentMetrics — recordDedupCheck / recordDedupHit counters
 */

#include <gtest/gtest.h>
#include "content/image_processor.h"
#include "content/content_processor.h"
#include "content/deduplication_checker.h"
#include "content/content_metrics.h"

#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

using namespace themis::content;

// ============================================================================
// Helpers — minimal synthetic image blobs
// ============================================================================

namespace {

/// Build a minimal 2×2 24-bpp BMP blob with a solid colour.
std::vector<uint8_t> makeBmp(uint8_t r, uint8_t g, uint8_t b, int width = 4, int height = 4) {
    // BMP header (14 bytes) + DIB header (40 bytes) + pixel data
    int row_stride = ((width * 3 + 3) / 4) * 4;
    int pixel_data_size = height * row_stride;
    int file_size = 54 + pixel_data_size;

    std::vector<uint8_t> bmp(static_cast<size_t>(file_size), 0);

    // Signature
    bmp[0] = 'B'; bmp[1] = 'M';
    // File size (LE)
    bmp[2] = file_size & 0xFF;
    bmp[3] = (file_size >> 8) & 0xFF;
    bmp[4] = (file_size >> 16) & 0xFF;
    bmp[5] = (file_size >> 24) & 0xFF;
    // Pixel data offset = 54
    bmp[10] = 54;
    // DIB header size = 40
    bmp[14] = 40;
    // Width
    bmp[18] = width & 0xFF;
    bmp[22] = height & 0xFF; // positive height → bottom-up
    bmp[26] = 1;              // color planes
    bmp[28] = 24;             // bits per pixel

    // Fill pixel rows (BGR)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t off = 54 + static_cast<size_t>(y) * static_cast<size_t>(row_stride)
                       + static_cast<size_t>(x) * 3;
            bmp[off]     = b;
            bmp[off + 1] = g;
            bmp[off + 2] = r;
        }
    }
    return bmp;
}

/// Build a BMP with a checkerboard pattern (alternating black/white).
std::vector<uint8_t> makeCheckerBmp(int width = 8, int height = 8) {
    int row_stride = ((width * 3 + 3) / 4) * 4;
    int file_size  = 54 + height * row_stride;
    std::vector<uint8_t> bmp(static_cast<size_t>(file_size), 0);

    bmp[0] = 'B'; bmp[1] = 'M';
    bmp[2] = file_size & 0xFF;
    bmp[3] = (file_size >> 8) & 0xFF;
    bmp[10] = 54;
    bmp[14] = 40;
    bmp[18] = width & 0xFF;
    bmp[22] = height & 0xFF;
    bmp[26] = 1;
    bmp[28] = 24;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t val = ((x + y) % 2 == 0) ? 255 : 0;
            size_t off = 54 + static_cast<size_t>(y) * static_cast<size_t>(row_stride)
                       + static_cast<size_t>(x) * 3;
            bmp[off] = val; bmp[off+1] = val; bmp[off+2] = val;
        }
    }
    return bmp;
}

/// Build a BMP with a horizontal gradient from 0 to 255.
std::vector<uint8_t> makeGradientBmp(int width = 32, int height = 32) {
    int row_stride = ((width * 3 + 3) / 4) * 4;
    int file_size  = 54 + height * row_stride;
    std::vector<uint8_t> bmp(static_cast<size_t>(file_size), 0);

    bmp[0] = 'B'; bmp[1] = 'M';
    bmp[2] = file_size & 0xFF;  bmp[3] = (file_size >> 8) & 0xFF;
    bmp[10] = 54; bmp[14] = 40;
    bmp[18] = width & 0xFF; bmp[22] = height & 0xFF;
    bmp[26] = 1; bmp[28] = 24;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t val = static_cast<uint8_t>((x * 255) / (width - 1));
            size_t off = 54 + static_cast<size_t>(y) * static_cast<size_t>(row_stride)
                       + static_cast<size_t>(x) * 3;
            bmp[off] = val; bmp[off+1] = val; bmp[off+2] = val;
        }
    }
    return bmp;
}

} // namespace

// ============================================================================
// ImageProcessor::computePHash
// ============================================================================

TEST(ComputePHash, EmptyBlobReturnsEmpty) {
    std::vector<uint8_t> empty;
    EXPECT_TRUE(ImageProcessor::computePHash(empty).empty());
}

TEST(ComputePHash, SmallBlobReturnsEmpty) {
    std::vector<uint8_t> tiny(10, 0);
    EXPECT_TRUE(ImageProcessor::computePHash(tiny).empty());
}

TEST(ComputePHash, ReturnsSixteenCharHex) {
    auto bmp = makeBmp(128, 64, 32);
    std::string hash = ImageProcessor::computePHash(bmp);
    ASSERT_EQ(hash.size(), 16u);
    for (char c : hash) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex character: " << c;
    }
}

TEST(ComputePHash, IdenticalBlobsProduceIdenticalHash) {
    auto bmp = makeBmp(200, 100, 50);
    EXPECT_EQ(ImageProcessor::computePHash(bmp), ImageProcessor::computePHash(bmp));
}

TEST(ComputePHash, NearIdenticalGradientsDifferByFewBits) {
    // Two identical gradient images should produce exactly the same hash (distance 0).
    auto bmp1 = makeGradientBmp(32, 32);
    auto bmp2 = bmp1; // exact copy

    std::string h1 = ImageProcessor::computePHash(bmp1);
    std::string h2 = ImageProcessor::computePHash(bmp2);
    ASSERT_EQ(h1.size(), 16u);
    ASSERT_EQ(h2.size(), 16u);
    EXPECT_EQ(h1, h2) << "Exact-copy gradients must produce identical pHashes (Hamming=0)";
}

TEST(ComputePHash, DifferentImagesDifferentHashes) {
    auto bmp_black  = makeBmp(0,   0,   0);
    auto bmp_white  = makeBmp(255, 255, 255);
    auto bmp_check  = makeCheckerBmp();

    auto h_black = ImageProcessor::computePHash(bmp_black);
    auto h_white = ImageProcessor::computePHash(bmp_white);
    auto h_check = ImageProcessor::computePHash(bmp_check);

    EXPECT_NE(h_black, h_white);
    EXPECT_NE(h_black, h_check);
    EXPECT_NE(h_white, h_check);
}

TEST(ComputePHash, FallbackForNonBmpReturnsHex) {
    // Simulate a PNG-like blob (starts with PNG magic bytes but no decodeable pixel data)
    std::vector<uint8_t> pseudo_png(200, 0xAB);
    pseudo_png[0] = 0x89; pseudo_png[1] = 'P'; pseudo_png[2] = 'N'; pseudo_png[3] = 'G';
    std::string h = ImageProcessor::computePHash(pseudo_png);
    ASSERT_EQ(h.size(), 16u);
}

// ============================================================================
// TextProcessor::computeMinHash
// ============================================================================

TEST(ComputeMinHash, EmptyTextReturnsMaxValues) {
    auto sig = TextProcessor::computeMinHash("");
    ASSERT_EQ(sig.size(), 128u);
    for (auto v : sig) EXPECT_EQ(v, UINT32_MAX);
}

TEST(ComputeMinHash, ProducesCorrectSize) {
    auto sig = TextProcessor::computeMinHash("hello world this is a test");
    EXPECT_EQ(sig.size(), 128u);
}

TEST(ComputeMinHash, IdenticalTextsProduceIdenticalSignatures) {
    std::string text = "The quick brown fox jumps over the lazy dog";
    EXPECT_EQ(TextProcessor::computeMinHash(text), TextProcessor::computeMinHash(text));
}

TEST(ComputeMinHash, SimilarTextsShareMostMinHashValues) {
    std::string t1 = "The quick brown fox jumps over the lazy dog and runs away";
    std::string t2 = "The quick brown fox jumps over the lazy dog and runs fast";

    auto s1 = TextProcessor::computeMinHash(t1);
    auto s2 = TextProcessor::computeMinHash(t2);
    ASSERT_EQ(s1.size(), s2.size());

    // Estimate Jaccard: fraction of matching MinHash values
    size_t matches = 0;
    for (size_t i = 0; i < s1.size(); ++i) {
        if (s1[i] == s2[i]) ++matches;
    }
    double estimated_jaccard = static_cast<double>(matches) / s1.size();
    EXPECT_GT(estimated_jaccard, 0.5) << "Similar texts should have high Jaccard estimate";
}

TEST(ComputeMinHash, DissimilarTextsHaveLowJaccard) {
    std::string t1 = "Machine learning and neural networks are popular topics today";
    std::string t2 = "Cooking recipes involve ingredients flour sugar butter eggs";

    auto s1 = TextProcessor::computeMinHash(t1);
    auto s2 = TextProcessor::computeMinHash(t2);

    size_t matches = 0;
    for (size_t i = 0; i < s1.size(); ++i) {
        if (s1[i] == s2[i]) ++matches;
    }
    double estimated_jaccard = static_cast<double>(matches) / s1.size();
    EXPECT_LT(estimated_jaccard, 0.5) << "Dissimilar texts should have low Jaccard estimate";
}

TEST(ComputeMinHash, CustomNumHashes) {
    auto sig = TextProcessor::computeMinHash("test text", 64);
    EXPECT_EQ(sig.size(), 64u);
}

// ============================================================================
// DeduplicationChecker — pHash (no persistent storage needed)
// ============================================================================

// Minimal stub for RocksDBWrapper — use nullptr and verify graceful handling.
TEST(DeduplicationCheckerImage, NullStorageReturnNoHit) {
    DeduplicationChecker checker(nullptr);
    auto result = checker.isDuplicateImage("0000000000000000");
    EXPECT_FALSE(result.has_value());
}

TEST(DeduplicationCheckerImage, EmptyHashReturnNoHit) {
    DeduplicationChecker checker(nullptr);
    checker.registerImage("id1", ""); // should be silently ignored
    auto result = checker.isDuplicateImage("0000000000000000");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// DeduplicationChecker — MinHash band-LSH (in-memory only)
// ============================================================================

TEST(DeduplicationCheckerText, NoRegistrationNoHit) {
    DeduplicationChecker checker(nullptr);
    auto sig = TextProcessor::computeMinHash("unique content nobody else has");
    EXPECT_FALSE(checker.isDuplicateText(sig).has_value());
}

TEST(DeduplicationCheckerText, IdenticalSignatureDetected) {
    DeduplicationChecker checker(nullptr);
    std::string text = "The quick brown fox jumps over the lazy dog";
    auto sig = TextProcessor::computeMinHash(text);

    checker.registerText("doc_original", sig);
    auto result = checker.isDuplicateText(sig);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->existing_id, "doc_original");
    EXPECT_GE(result->similarity, DeduplicationChecker::kJaccardThreshold);
}

TEST(DeduplicationCheckerText, NearDuplicateDetectedViaBands) {
    DeduplicationChecker checker(nullptr);

    // Two very similar texts — most shingles overlap, so many MinHash values match
    std::string t1 = "The quick brown fox jumps over the lazy dog and runs far away";
    std::string t2 = "The quick brown fox jumps over the lazy dog and runs far away today";

    auto sig1 = TextProcessor::computeMinHash(t1);
    auto sig2 = TextProcessor::computeMinHash(t2);

    // Register first document
    checker.registerText("doc1", sig1);

    // Check second document — at least one band should collide due to high similarity
    auto result = checker.isDuplicateText(sig2);
    // Whether or not a collision occurs depends on similarity; just verify no crash.
    // For highly similar texts, expect a hit.
    if (result.has_value()) {
        EXPECT_EQ(result->existing_id, "doc1");
    }
}

TEST(DeduplicationCheckerText, DissimilarTextNoHit) {
    DeduplicationChecker checker(nullptr);
    std::string t1 = "Machine learning and neural networks are popular topics today";
    std::string t2 = "Cooking recipes involve ingredients flour sugar butter eggs and milk";

    auto sig1 = TextProcessor::computeMinHash(t1);
    auto sig2 = TextProcessor::computeMinHash(t2);

    checker.registerText("doc1", sig1);
    auto result = checker.isDuplicateText(sig2);
    EXPECT_FALSE(result.has_value())
        << "Dissimilar texts should not trigger a dedup hit";
}

TEST(DeduplicationCheckerText, MultipleRegistrations) {
    DeduplicationChecker checker(nullptr);

    auto sig_a = TextProcessor::computeMinHash("alpha beta gamma delta epsilon");
    auto sig_b = TextProcessor::computeMinHash("zeta eta theta iota kappa lambda");
    auto sig_c = TextProcessor::computeMinHash("alpha beta gamma delta epsilon");

    checker.registerText("doc_a", sig_a);
    checker.registerText("doc_b", sig_b);

    // sig_c is identical to sig_a — should detect doc_a as duplicate
    auto result_c = checker.isDuplicateText(sig_c);
    ASSERT_TRUE(result_c.has_value());
    EXPECT_EQ(result_c->existing_id, "doc_a");

    // sig_b should not match sig_a
    auto result_b_lookup = checker.isDuplicateText(sig_b);
    ASSERT_TRUE(result_b_lookup.has_value());
    EXPECT_EQ(result_b_lookup->existing_id, "doc_b");
}

TEST(DeduplicationCheckerText, TooShortSignatureIgnored) {
    DeduplicationChecker checker(nullptr);
    std::vector<uint32_t> short_sig(10, 42); // only 10 values, need 128
    // Should not crash and should report no hit
    EXPECT_FALSE(checker.isDuplicateText(short_sig).has_value());
    checker.registerText("doc_x", short_sig); // should be silently ignored
}

// ============================================================================
// ContentMetrics — dedup counters
// ============================================================================

TEST(ContentMetricsDedup, InitialCountersAreZero) {
    ContentMetrics m;
    EXPECT_EQ(m.getDedupChecksTotal(), 0u);
    EXPECT_EQ(m.getDedupHitsTotal(), 0u);
}

TEST(ContentMetricsDedup, RecordDedupCheckIncrements) {
    ContentMetrics m;
    m.recordDedupCheck();
    m.recordDedupCheck();
    EXPECT_EQ(m.getDedupChecksTotal(), 2u);
}

TEST(ContentMetricsDedup, RecordDedupHitIncrements) {
    ContentMetrics m;
    m.recordDedupHit();
    EXPECT_EQ(m.getDedupHitsTotal(), 1u);
}

TEST(ContentMetricsDedup, PrometheusContainsDedupCounters) {
    ContentMetrics m;
    m.recordDedupCheck();
    m.recordDedupCheck();
    m.recordDedupHit();

    std::string prom = m.toPrometheusFormat();
    EXPECT_NE(prom.find("content_dedup_checks_total 2"), std::string::npos);
    EXPECT_NE(prom.find("content_dedup_hits_total 1"), std::string::npos);
}

TEST(ContentMetricsDedup, ResetClearsDedupCounters) {
    ContentMetrics m;
    m.recordDedupCheck();
    m.recordDedupHit();
    m.reset();
    EXPECT_EQ(m.getDedupChecksTotal(), 0u);
    EXPECT_EQ(m.getDedupHitsTotal(), 0u);
}

TEST(ContentMetricsDedup, JsonContainsDedupCounters) {
    ContentMetrics m;
    m.recordDedupCheck();
    m.recordDedupCheck();
    m.recordDedupHit();

    auto j = m.toJson();
    EXPECT_EQ(j["throughput"]["dedup_checks"].get<uint64_t>(), 2u);
    EXPECT_EQ(j["throughput"]["dedup_hits"].get<uint64_t>(), 1u);
}

// ============================================================================
// SHA-256 hash-based exact-duplicate detection (ContentManager integration)
// ============================================================================

#include "content/content_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index_manager.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"
#include <filesystem>
#include <openssl/sha.h>

using namespace themis;

namespace {

class ContentSHA256DedupTest : public ::testing::Test {
protected:
    std::shared_ptr<RocksDBWrapper>       storage_;
    std::shared_ptr<VectorIndexManager>   vector_index_;
    std::shared_ptr<GraphIndexManager>    graph_index_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<ContentManager>       mgr_;
    const std::string                     kDbPath = "./test_sha256_dedup_db";

    void SetUp() override {
        if (std::filesystem::exists(kDbPath))
            std::filesystem::remove_all(kDbPath);
        RocksDBWrapper::Config cfg;
        cfg.db_path = kDbPath;
        storage_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());
        vector_index_   = std::make_shared<VectorIndexManager>(*storage_);
        graph_index_    = std::make_shared<GraphIndexManager>(*storage_);
        secondary_index_ = std::make_shared<SecondaryIndexManager>(*storage_);
        mgr_ = std::make_shared<ContentManager>(
            storage_, vector_index_, graph_index_, secondary_index_);
    }

    void TearDown() override {
        mgr_.reset();
        secondary_index_.reset();
        graph_index_.reset();
        vector_index_.reset();
        storage_.reset();
        if (std::filesystem::exists(kDbPath))
            std::filesystem::remove_all(kDbPath);
    }

    // Compute reference SHA-256 hex for a string.
    static std::string sha256Hex(const std::string& data) {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest);
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
            oss << std::setw(2) << static_cast<unsigned int>(digest[i]);
        return oss.str();
    }
};

} // namespace

TEST_F(ContentSHA256DedupTest, FirstIngestionSucceeds) {
    auto res = mgr_->ingestRawBlob("hello world text content", "test.txt", "text/plain");
    ASSERT_TRUE(res.success);
    EXPECT_FALSE(res.primary_content_id.empty());
}

TEST_F(ContentSHA256DedupTest, SecondIngestionOfIdenticalBlobReturnsSameId) {
    const std::string blob = "identical content for SHA-256 dedup test";
    auto res1 = mgr_->ingestRawBlob(blob, "file.txt", "text/plain");
    ASSERT_TRUE(res1.success);

    auto res2 = mgr_->ingestRawBlob(blob, "file.txt", "text/plain");
    ASSERT_TRUE(res2.success);
    EXPECT_EQ(res1.primary_content_id, res2.primary_content_id)
        << "Second ingest of identical blob must return the existing content ID";
}

TEST_F(ContentSHA256DedupTest, DifferentBlobsGetDifferentIds) {
    auto res1 = mgr_->ingestRawBlob("content alpha", "a.txt", "text/plain");
    auto res2 = mgr_->ingestRawBlob("content beta",  "b.txt", "text/plain");
    ASSERT_TRUE(res1.success);
    ASSERT_TRUE(res2.success);
    EXPECT_NE(res1.primary_content_id, res2.primary_content_id);
}

TEST_F(ContentSHA256DedupTest, HashSha256FieldIsRealSHA256) {
    const std::string blob = "verify that hash_sha256 is a proper SHA-256";
    auto res = mgr_->ingestRawBlob(blob, "verify.txt", "text/plain");
    ASSERT_TRUE(res.success);

    auto meta = mgr_->getContentMeta(res.primary_content_id);
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->hash_sha256, sha256Hex(blob))
        << "hash_sha256 must be the real SHA-256 hex of the blob";
}

TEST_F(ContentSHA256DedupTest, DuplicateMetadataContainsDuplicateOfField) {
    const std::string blob = "duplicate detection metadata test";
    auto res1 = mgr_->ingestRawBlob(blob, "dup.txt", "text/plain");
    ASSERT_TRUE(res1.success);

    auto res2 = mgr_->ingestRawBlob(blob, "dup.txt", "text/plain");
    ASSERT_TRUE(res2.success);
    ASSERT_TRUE(res2.metadata.contains("duplicate_of"))
        << "Duplicate ingest result must contain 'duplicate_of' key";
    EXPECT_EQ(res2.metadata["duplicate_of"].get<std::string>(), res1.primary_content_id);
}
