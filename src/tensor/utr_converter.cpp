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
#include <bit>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// Static bridge slots — STUB #257 (EmbedFn) / STUB #258 (ImageEmbedFn)
// ============================================================================

namespace {
    std::mutex          g_embed_mtx;
    UTRConverter::EmbedFn g_embed_fn;          // null ⟹ use FNV-1a fallback

    std::mutex               g_image_embed_mtx;
    UTRConverter::ImageEmbedFn g_image_embed_fn; // null ⟹ use raw-pixel fallback
} // namespace

void UTRConverter::setEmbedFn(UTRConverter::EmbedFn fn) {
    std::lock_guard<std::mutex> lk(g_embed_mtx);
    g_embed_fn = std::move(fn);
}

void UTRConverter::clearEmbedFn() {
    std::lock_guard<std::mutex> lk(g_embed_mtx);
    g_embed_fn = nullptr;
}

UTRConverter::EmbedFn UTRConverter::getEmbedFn() {
    std::lock_guard<std::mutex> lk(g_embed_mtx);
    return g_embed_fn;
}

void UTRConverter::setImageEmbedFn(UTRConverter::ImageEmbedFn fn) {
    std::lock_guard<std::mutex> lk(g_image_embed_mtx);
    g_image_embed_fn = std::move(fn);
}

void UTRConverter::clearImageEmbedFn() {
    std::lock_guard<std::mutex> lk(g_image_embed_mtx);
    g_image_embed_fn = nullptr;
}

UTRConverter::ImageEmbedFn UTRConverter::getImageEmbedFn() {
    std::lock_guard<std::mutex> lk(g_image_embed_mtx);
    return g_image_embed_fn;
}

namespace {

constexpr std::size_t kMinPatchExtent = 1;
constexpr std::size_t kMaxPatchExtent = 4;

// Round up to the next power-of-two side length.
// v=0 and v=1 both map to 1.
[[nodiscard]] std::size_t roundUpPowerOfTwo(std::size_t v) {
    return v <= 1 ? 1 : std::bit_ceil(v);
}

// Hilbert helper rotation/reflection step.
// rx/ry are quadrant bits derived from the Hilbert index.
void hilbertRotate(std::size_t n, std::size_t& x, std::size_t& y, std::size_t rx, std::size_t ry) {
    if (ry == 0) {
        if (rx == 1) {
            x = n - 1 - x;
            y = n - 1 - y;
        }
        std::swap(x, y);
    }
}

// Convert a Hilbert distance d into (x,y) coordinates on an n x n grid.
[[nodiscard]] std::pair<std::size_t, std::size_t> hilbertIndexToXY(std::size_t n, std::size_t d) {
    std::size_t x = 0;
    std::size_t y = 0;
    for (std::size_t s = 1, t = d; s < n; s <<= 1) {
        const std::size_t rx = (t >> 1U) & 1U;
        const std::size_t ry = (t ^ rx) & 1U;
        hilbertRotate(s, x, y, rx, ry);
        x += s * rx;
        y += s * ry;
        t >>= 2U;
    }
    return {x, y};
}

// ============================================================================
// Shared helpers
// ============================================================================

/// Clamp a float to [lo, hi].
static float clampf(float v, float lo, float hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

static std::size_t clampPatchExtent(std::size_t extent) noexcept {
    return std::max<std::size_t>(kMinPatchExtent,
                                 std::min<std::size_t>(kMaxPatchExtent, extent));
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
// Lexical feature embedding for documents
// ============================================================================

/// FNV-1a 64-bit hash of a string view.
static uint64_t fnv1a(std::string_view s) noexcept {
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
                                   [](unsigned char c) {
                                       return !std::isalnum(static_cast<unsigned char>(c));
                                   }),
                    token.end());
        std::transform(token.begin(), token.end(), token.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return token;
    };

    auto scatter = [&](std::string_view feature, float weight) {
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
                // Pass a view over 3 chars to avoid a heap allocation per n-gram.
                scatter(std::string_view{token.data() + i, 3}, 0.35f);
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

    const std::size_t hilbert_side = roundUpPowerOfTwo(std::max(grid.rows, grid.cols));
    std::vector<float> hilbert_ordered(hilbert_side * hilbert_side, 0.0f);
    for (std::size_t d = 0; d < hilbert_ordered.size(); ++d) {
        const auto [x, y] = hilbertIndexToXY(hilbert_side, d);
        if (y < grid.rows && x < grid.cols) {
            hilbert_ordered[d] = normalised[y * grid.cols + x];
        }
    }

    auto decomposed = decomposer.decompose(hilbert_ordered, {hilbert_side, hilbert_side}, tt_cfg);
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

    ImageEmbedFn image_embed_fn;
    {
        std::lock_guard<std::mutex> lk(g_image_embed_mtx);
        image_embed_fn = g_image_embed_fn;
    }
    if (image_embed_fn) {
        return image_embed_fn(pixels, h, w, c, cfg);
    }

    const auto patch_h = clampPatchExtent(h);
    const auto patch_w = clampPatchExtent(w);
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
                const auto mean = sample_count > 0
                    ? sum[channel] / static_cast<double>(sample_count)
                    : 0.0;
                const auto variance = sample_count > 0
                    ? std::max(
                          0.0,
                          (sum_sq[channel] / static_cast<double>(sample_count)) -
                              (mean * mean))
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

    // Obtain the embedding function: injected bridge takes priority over FNV-1a.
    UTRConverter::EmbedFn embed_fn;
    {
        std::lock_guard<std::mutex> lk(g_embed_mtx);
        embed_fn = g_embed_fn;
    }

    std::vector<float> segment_matrix;
    segment_matrix.reserve(num_segs * embed_dim);
    for (const auto& seg : segments) {
        std::vector<float> emb;
        if (embed_fn) {
            emb = embed_fn(seg, embed_dim);
            if (emb.size() != embed_dim) {
                throw std::runtime_error(
                    "UTRConverter::fromDocument: injected EmbedFn returned " +
                    std::to_string(emb.size()) +
                    " elements but embed_dim=" + std::to_string(embed_dim));
            }
        } else {
            emb = hashEmbed(seg, embed_dim);
        }
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
