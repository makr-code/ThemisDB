/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mock_clip_processor.h                              ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     53                                             ║
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
