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
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
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
    // Split to word-level tokens, hash each, scatter into the embedding.
    std::istringstream iss(segment);
    std::string token;
    int count = 0;
    while (iss >> token) {
        const uint64_t h = fnv1a(token);
        // Use multiple 16-bit lanes of the hash value to scatter contributions.
        for (std::size_t lane = 0; lane < 4 && lane < embed_dim; ++lane) {
            const auto shift = lane * 16U;
            const auto bits  = static_cast<uint16_t>((h >> shift) & 0xFFFFULL);
            const auto dim   = static_cast<std::size_t>(bits) % embed_dim;
            // Signed contribution (+1 / -1) from the high bit of the next lane
            const auto sign  = ((h >> (shift + 8U)) & 1ULL) ? 1.0f : -1.0f;
            vec[dim] += sign;
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
    // Activation: Always when no ImageEmbedFn bridge is installed.
    // Production Delta: Normalises pixel values to [0, 1] and decomposes directly
    //   in HWC mode order.  No patch embedding, semantic alignment, or frequency-
    //   domain transform is applied.  Similarity is therefore pixel-level, not
    //   semantic.
    // Removal Plan: Q4 2028 — add patch-based structural embedding (non-overlapping
    //   patches → learned linear projection → TT decompose in patch space).

    // Delegate to an injected encoder when available.
    {
        std::lock_guard<std::mutex> lk(g_image_embed_mtx);
        if (g_image_embed_fn) {
            return g_image_embed_fn(pixels, h, w, c, cfg);
        }
    }

    // Normalise to [0, 1]
    std::vector<float> normed(pixels.size());
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        normed[i] = clampf(pixels[i], 0.0f, 255.0f) / 255.0f;
    }

    storage::TensorTrainDecomposer decomposer;
    storage::TensorTrainConfig     tt_cfg;
    tt_cfg.eps      = cfg.eps;
    tt_cfg.max_rank = cfg.max_rank;

    std::vector<std::size_t> shape;
    if (c == 1) {
        shape = {h, w};
    } else {
        shape = {h, w, c};
    }

    auto decomposed = decomposer.decompose(normed, shape, tt_cfg);
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
