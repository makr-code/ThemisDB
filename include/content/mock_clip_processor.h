/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mock_clip_processor.h                              ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:43:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
