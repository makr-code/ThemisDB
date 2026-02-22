/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_mock_clip.cpp                                 ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
