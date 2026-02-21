/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mock_clip_processor.h                              ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "content/content_processor.h"
#include <string>

namespace themis {
namespace content {

// Mock CLIP-like image processor for deterministic embeddings used in tests.
class MockClipProcessor : public IContentProcessor {
public:
    MockClipProcessor(int dim = 512) : dim_(dim) {}
    ~MockClipProcessor() override = default;

    ExtractionResult extract(const std::string& blob, const ContentType& content_type) override;
    std::vector<nlohmann::json> chunk(const ExtractionResult& extraction_result, int chunk_size, int overlap) override;
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;
    std::string getName() const override { return "MockClipProcessor"; }
    std::vector<ContentCategory> getSupportedCategories() const override { return {ContentCategory::IMAGE}; }

private:
    int dim_ = 512;
    std::vector<float> computeMockEmbedding_(const std::string& data) const;
};

} // namespace content
} // namespace themis
