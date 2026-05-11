/*
 * @file src/tensor/utr_converter.cpp
 * @brief UTRConverter implementation — Phase 7 multi-modal tensor representation.
 *
 * See include/tensor/utr_converter.h for design details and stub notes.
 */

#include "tensor/utr_converter.h"
#include "storage/hierarchical_tucker_decomposer.h"
#include "storage/tensor_train_decomposer.h"
#include "tensor/hyper_index_builder.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

namespace {

// ============================================================================
// Shared helpers
// ============================================================================

/// Clamp a float to [lo, hi].
static float clampf(float v, float lo, float hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ============================================================================
// Document segmentation helpers (STUB #257)
// ============================================================================

/// Split text at double-newline boundaries (paragraph mode).
static std::vector<std::string> splitParagraphs(const std::string& text) {
    std::vector<std::string> segments;
    std::size_t start = 0;
    while (start < text.size()) {
        const auto pos = text.find("\n\n", start);
        const auto end = (pos == std::string::npos) ? text.size() : pos;
        const auto seg = text.substr(start, end - start);
        if (!seg.empty()) segments.push_back(seg);
        start = (pos == std::string::npos) ? text.size() : (pos + 2U);
    }
    if (segments.empty()) segments.push_back(text);
    return segments;
}

/// Split text at sentence boundaries (period + space or period + EOT heuristic).
static std::vector<std::string> splitSentences(const std::string& text) {
    std::vector<std::string> segments;
    std::size_t start = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '.' &&
            (i + 1 >= text.size() || text[i + 1] == ' ' || text[i + 1] == '\n'))
        {
            const auto seg = text.substr(start, i + 1 - start);
            if (!seg.empty()) segments.push_back(seg);
            start = i + 1;
            while (start < text.size() && text[start] == ' ') ++start;
        }
    }
    if (start < text.size()) {
        const auto seg = text.substr(start);
        if (!seg.empty()) segments.push_back(seg);
    }
    if (segments.empty()) segments.push_back(text);
    return segments;
}

// ============================================================================
// Hash-projection embedding (STUB #257)
// ============================================================================

// STUB/SIMULATION NOTE (STUB #257):
// Purpose: Provide a fixed-length embedding for each text segment so that the
//          HT decomposer can operate on a well-defined 2-D matrix.
// Activation: Always.
// Production Delta: Uses a deterministic FNV-1a hash projection instead of a
//   learned sentence encoder (e.g. SBERT).  Semantic similarity is therefore
//   approximated by lexical token overlap rather than semantic meaning.
// Removal Plan: Q4 2028 — wire a learnable sentence encoder (e.g. a quantised
//   SBERT variant loaded via LLMPluginManager) and replace hash projection.

/// FNV-1a 64-bit hash of a string.
static uint64_t fnv1a(const std::string& s) noexcept {
    constexpr uint64_t kBasis = 14695981039346656037ULL;
    constexpr uint64_t kPrime = 1099511628211ULL;
    uint64_t h = kBasis;
    for (const unsigned char c : s) {
        h ^= static_cast<uint64_t>(c);
        h *= kPrime;
    }
    return h;
}

/// Produce a deterministic `embed_dim`-dimensional embedding from a text segment.
static std::vector<float> hashEmbed(const std::string& segment, std::size_t embed_dim) {
    std::vector<float> vec(embed_dim, 0.0f);
    auto normalizeToken = [](std::string token) {
        token.erase(std::remove_if(token.begin(), token.end(),
                                   [](unsigned char c) { return !std::isalnum(c); }),
                    token.end());
        std::transform(token.begin(), token.end(), token.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return token;
    };

    auto scatter = [&](const std::string& feature, float weight) {
        const uint64_t h = fnv1a(feature);
        for (std::size_t lane = 0; lane < 8 && lane < embed_dim; ++lane) {
            const auto shift = (lane % 4) * 16U;
            const auto bits  = static_cast<uint16_t>((h >> shift) & 0xFFFFULL);
            const auto dim   = static_cast<std::size_t>((bits + lane * 131U) % embed_dim);
            const auto sign  = ((h >> (shift + 7U)) & 1ULL) ? 1.0f : -1.0f;
            vec[dim] += sign * weight;
        }
    };

    // Split to word-level tokens, hash each token and local character n-grams.
    std::istringstream iss(segment);
    std::string token;
    int count = 0;
    while (iss >> token) {
        token = normalizeToken(std::move(token));
        if (token.empty()) {
            continue;
        }
        scatter(token, 1.0f);
        if (token.size() >= 3) {
            for (std::size_t i = 0; i + 3 <= token.size(); ++i) {
                scatter(token.substr(i, 3), 0.35f);
            }
        }
        ++count;
    }
    if (count > 0) {
        const float scale = 1.0f / std::sqrt(static_cast<float>(count));
        for (auto& v : vec) v *= scale;
    }
    return vec;
}

} // namespace

// ============================================================================
// UTRConverter::fromGeospatial
// ============================================================================

storage::TTTrain UTRConverter::fromGeospatial(const RasterGrid& grid,
                                               const UTRConfig&  cfg) {
    if (grid.rows == 0 || grid.cols == 0) {
        throw std::invalid_argument("RasterGrid must have non-zero rows and cols");
    }
    if (grid.cell_size_deg <= 0.0) {
        throw std::invalid_argument("RasterGrid::cell_size_deg must be > 0");
    }
    const std::size_t expected = grid.rows * grid.cols;
    if (grid.values.size() != expected) {
        throw std::invalid_argument(
            "RasterGrid::values.size() (" + std::to_string(grid.values.size()) +
            ") != rows*cols (" + std::to_string(expected) + ")");
    }

    // STUB/SIMULATION NOTE (STUB #256):
    // Purpose: Encode geospatial raster field as TT-train for TensorIndexManager.
    // Activation: Always.
    // Production Delta: Uses plain row-major mode ordering (rows × cols).
    //   Spatial locality is preserved in the row-major sense but curvature-aware
    //   or Hilbert-curve-ordered modes (which would improve locality even further)
    //   are not applied.  Coordinate precision is maintained because no lossy
    //   coordinate encoding is applied; only the scalar field is TT-decomposed.
    // Removal Plan: Q3 2028 — add topology-preserving mode reordering
    //   (e.g. Hilbert curve) before TT decomposition.

    // Normalise values to [0, 1] to improve TT-rank stability.
    float vmin = std::numeric_limits<float>::max();
    float vmax = std::numeric_limits<float>::lowest();
    for (const auto v : grid.values) {
        vmin = std::min(vmin, v);
        vmax = std::max(vmax, v);
    }
    const float range = (vmax > vmin) ? (vmax - vmin) : 1.0f;

    std::vector<float> normalised(expected);
    for (std::size_t i = 0; i < expected; ++i) {
        normalised[i] = clampf((grid.values[i] - vmin) / range, 0.0f, 1.0f);
    }

    storage::TensorTrainDecomposer decomposer;
    storage::TensorTrainConfig     tt_cfg;
    tt_cfg.eps      = cfg.eps;
    tt_cfg.max_rank = cfg.max_rank;

    auto decomposed = decomposer.decompose(normalised, {grid.rows, grid.cols}, tt_cfg);
    auto tt_train   = std::move(decomposed.first);

    // Persist normalisation range in achieved_eps field for now; a proper metadata
    // struct will carry vmin/vmax when the geospatial TT extension lands (Q3 2028).
    // NOTE: We intentionally do not encode vmin/vmax into original_norm (which stores
    // a Frobenius norm) to avoid confusion. The normalisation is implicit (values
    // were mapped to [0,1]); callers that need to recover physical units must store
    // vmin/vmax externally until a dedicated GeospatialTTMetadata struct is added.
    (void)vmin; (void)vmax;
    return tt_train;
}

// ============================================================================
// UTRConverter::fromTabular
// ============================================================================

HyperIndexTensor UTRConverter::fromTabular(
        const std::string&               tenant_id,
        const std::vector<ColumnSchema>& schema,
        const std::vector<TableRow>&     rows,
        const UTRConfig&                 cfg) {
    HyperIndexConfig hcfg;
    hcfg.bucket_count = cfg.bucket_count;
    hcfg.eps          = cfg.eps;
    hcfg.max_rank     = cfg.max_rank;
    return HyperIndexBuilder::fromSchema(tenant_id, schema, rows, hcfg);
}

// ============================================================================
// UTRConverter::fromImage
// ============================================================================

storage::TTTrain UTRConverter::fromImage(const std::vector<float>& pixels,
                                          std::size_t h,
                                          std::size_t w,
                                          std::size_t c,
                                          const UTRConfig&          cfg) {
    if (h == 0 || w == 0 || c == 0) {
        throw std::invalid_argument("image dimensions h, w, c must all be > 0");
    }
    const std::size_t expected = h * w * c;
    if (pixels.size() != expected) {
        throw std::invalid_argument(
            "pixels.size() (" + std::to_string(pixels.size()) +
            ") != h*w*c (" + std::to_string(expected) + ")");
    }

    // STUB/SIMULATION NOTE (STUB #258):
    // Purpose: Provide a TT-encoded image representation for structural similarity
    //          search in TensorIndexManager.
    // Activation: Always.
    // Production Delta: Normalises pixel values to [0, 1] and decomposes directly
    //   in HWC mode order.  No patch embedding, semantic alignment, or frequency-
    //   domain transform is applied.  Similarity is therefore pixel-level, not
    //   semantic.
    // Removal Plan: Q4 2028 — add patch-based structural embedding (non-overlapping
    //   patches → learned linear projection → TT decompose in patch space).

    const auto patch_h = std::max<std::size_t>(1, std::min<std::size_t>(4, h));
    const auto patch_w = std::max<std::size_t>(1, std::min<std::size_t>(4, w));
    const auto patch_rows = (h + patch_h - 1U) / patch_h;
    const auto patch_cols = (w + patch_w - 1U) / patch_w;
    const auto patch_feature_dim = c * 2U; // mean + stddev per channel

    std::vector<float> patch_features;
    patch_features.reserve(patch_rows * patch_cols * patch_feature_dim);
    for (std::size_t patch_row = 0; patch_row < patch_rows; ++patch_row) {
        for (std::size_t patch_col = 0; patch_col < patch_cols; ++patch_col) {
            std::vector<double> sum(c, 0.0);
            std::vector<double> sum_sq(c, 0.0);
            std::size_t sample_count = 0;
            const auto row_begin = patch_row * patch_h;
            const auto row_end = std::min(h, row_begin + patch_h);
            const auto col_begin = patch_col * patch_w;
            const auto col_end = std::min(w, col_begin + patch_w);
            for (std::size_t row = row_begin; row < row_end; ++row) {
                for (std::size_t col = col_begin; col < col_end; ++col) {
                    for (std::size_t channel = 0; channel < c; ++channel) {
                        const auto idx = ((row * w) + col) * c + channel;
                        const auto value = clampf(pixels[idx], 0.0f, 255.0f) / 255.0f;
                        sum[channel] += value;
                        sum_sq[channel] += static_cast<double>(value) * value;
                    }
                    ++sample_count;
                }
            }
            for (std::size_t channel = 0; channel < c; ++channel) {
                const auto mean = sample_count > 0 ? sum[channel] / static_cast<double>(sample_count) : 0.0;
                const auto variance = sample_count > 0
                    ? std::max(0.0, (sum_sq[channel] / static_cast<double>(sample_count)) - (mean * mean))
                    : 0.0;
                patch_features.push_back(static_cast<float>(mean));
                patch_features.push_back(static_cast<float>(std::sqrt(variance)));
            }
        }
    }

    storage::TensorTrainDecomposer decomposer;
    storage::TensorTrainConfig     tt_cfg;
    tt_cfg.eps      = cfg.eps;
    tt_cfg.max_rank = cfg.max_rank;

    const std::vector<std::size_t> shape = {patch_rows, patch_cols, patch_feature_dim};

    auto decomposed = decomposer.decompose(patch_features, shape, tt_cfg);
    return std::move(decomposed.first);
}

// ============================================================================
// UTRConverter::fromDocument
// ============================================================================

tensor::HTTrain UTRConverter::fromDocument(const std::string&    text,
                                            DocumentStructureHint hint,
                                            const UTRConfig&      cfg) {
    if (text.empty()) {
        throw std::invalid_argument("document text must not be empty");
    }

    // 1. Segment text
    std::vector<std::string> segments;
    switch (hint) {
    case DocumentStructureHint::SENTENCES:
        segments = splitSentences(text);
        break;
    case DocumentStructureHint::PARAGRAPHS:
    case DocumentStructureHint::NONE:
    default:
        segments = splitParagraphs(text);
        break;
    }

    // Limit to max_segments to bound the tensor size
    if (segments.size() > cfg.max_segments) {
        segments.resize(cfg.max_segments);
    }
    if (segments.empty()) {
        throw std::invalid_argument("document produced no segments after splitting");
    }

    // 2. Embed each segment
    const std::size_t num_segs  = segments.size();
    const std::size_t embed_dim = cfg.embed_dim;

    std::vector<float> segment_matrix;
    segment_matrix.reserve(num_segs * embed_dim);
    for (const auto& seg : segments) {
        const auto emb = hashEmbed(seg, embed_dim);
        segment_matrix.insert(segment_matrix.end(), emb.begin(), emb.end());
    }

    // 3. HT-decompose the (num_segs × embed_dim) matrix
    const storage::HTConfig ht_cfg{cfg.max_rank, cfg.eps};
    storage::HierarchicalTuckerDecomposer ht_decomposer(ht_cfg);

    auto decomposed = ht_decomposer.decompose(segment_matrix, {num_segs, embed_dim});
    return std::move(decomposed.first);
}

} // namespace tensor
} // namespace themis
