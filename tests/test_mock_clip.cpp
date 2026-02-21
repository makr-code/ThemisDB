/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_mock_clip.cpp                                 ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:46:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     49                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 1bd2f05cd  2025-11-12  Add tests for VaultKeyProvider retry logic, MockClipProce... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "content/mock_clip_processor.h"

using namespace themis::content;

TEST(MockClipTest, DeterministicEmbeddingSize) {
    MockClipProcessor p(128);
    std::string sample = "fake-image-bytes-12345";

    auto emb1 = p.generateEmbedding(sample);
    auto emb2 = p.generateEmbedding(sample);

    EXPECT_EQ(emb1.size(), 128u);
    EXPECT_EQ(emb2.size(), 128u);
    // Deterministic across calls
    EXPECT_EQ(emb1, emb2);
}

TEST(MockClipTest, Normalized) {
    MockClipProcessor p(64);
    auto emb = p.generateEmbedding("another-image");
    double sum = 0.0;
    for (float v : emb) sum += static_cast<double>(v) * v;
    EXPECT_NEAR(sum, 1.0, 1e-3);
}
