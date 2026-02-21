/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_mock_clip.cpp                                 ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:18:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
