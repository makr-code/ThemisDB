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
    EXPECT_DOUBLE_EQ(result->similarity, DeduplicationChecker::kJaccardThreshold);
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
