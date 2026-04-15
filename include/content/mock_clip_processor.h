/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mock_clip_processor.h                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:44:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     49                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
