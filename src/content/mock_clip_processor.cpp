/** @file mock_clip_processor.cpp @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata. @author makr-code @version 0.0.1 @date 2026-09-18 21:43:08 @note Maturity: 🟢 PRODUCTION-READY @note Score: 96/100 @note Lines: 98 @note Module Context: src/content @note Ownership Scope: production-code @note Primary Symbols: none-detected @note PR History (last 5): #5950 [CP-1] Content Module Batch... (2026-08-15) | #3619 fix(content): build system ... (2026-03-12) @note Governance: BranchModel=develop-first; CanonicalBranches=develop,community,military @note Release Context: GateModel=WaveA→B→C→D on develop @note Status: Production Ready @note Generator: .github/scripts/code_maturity_header_writer.py @note This block is auto-generated and will be overwritten. */

#include "content/mock_clip_processor.h"

#include <cmath>
#include <functional>

#include "utils/logger.h"

namespace themis {
namespace content {

/**
 * @brief Extract.
 * @param[in] blob Input parameter.
 * @param[in] content_type Input parameter.
 * @return Return value.
 * @details Calls: nlohmann::json::object(), size(), computeMockEmbedding_().
 */
ExtractionResult MockClipProcessor::extract(const std::string &blob, const ContentType &content_type) {
    ExtractionResult res;
    res.ok                              = true;
    res.metadata                        = nlohmann::json::object();
    res.metadata["mime_type"]           = content_type.mime_type;
    res.metadata["original_size_bytes"] = blob.size();

    // For images we don't extract text; instead produce a mock embedding
    res.embedding = computeMockEmbedding_(blob);
    return res;
}

/**
 * @brief Chunk.
 * @param[in] extraction_result Input parameter.
 * @param[in] int Input parameter.
 * @param[in] int Input parameter.
 * @return Return value.
 * @details Calls: push_back().
 */
std::vector<nlohmann::json> MockClipProcessor::chunk(const ExtractionResult &extraction_result, int /*chunk_size*/,
                                                     int /*overlap*/) {
    std::vector<nlohmann::json> out;
    nlohmann::json chunk;
    chunk["text"]        = "";
    chunk["seq_num"]     = 0;
    chunk["token_count"] = 0;
    chunk["embedding"]   = extraction_result.embedding;
    out.push_back(chunk);
    return out;
}

/**
 * @brief Generate Embedding.
 * @param[in] chunk_data Input parameter.
 * @return Return value.
 * @details Calls: computeMockEmbedding_().
 */
std::vector<float> MockClipProcessor::generateEmbedding(const std::string &chunk_data) {
    return computeMockEmbedding_(chunk_data);
}

std::vector<float> MockClipProcessor::computeMockEmbedding_(const std::string &data) const {
    std::vector<float> v(dim_, 0.0f);
    if (data.empty()) {
        return v;
    }

    // Deterministic hash-based pseudo-embedding
    std::hash<std::string> hasher;
    size_t h = hasher(data);

    // Spread influence across dimensions
    for (int i = 0; i < dim_; ++i) {
        // mix hash with index for some variance
        uint64_t mixed = h ^ (static_cast<uint64_t>(i) * 0x9e3779b97f4a7c15ULL);
        // convert to float in [-1,1]
        float val = static_cast<int64_t>(mixed % 100000) / 100000.0f;
        val       = (val * 2.0f) - 1.0f;
        v[i]      = val;
    }

    // L2 normalize
    double sum = 0.0;
    for (float x : v) {
        sum += static_cast<double>(x) * x;
    }
    double norm = std::sqrt(sum);
    if (norm > 1e-6) {
        for (float &x : v) {
            x = static_cast<float>(x / norm);
        }
    }

    return v;
}

} // namespace content
} // namespace themis
