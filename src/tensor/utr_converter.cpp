/**
 * @file utr_converter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=9; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <memory>
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
// Encoder objects — ITextEncoder / IImageEncoder (higher priority)
// ============================================================================

namespace {
    std::mutex          g_embed_mtx;
    UTRConverter::EmbedFn g_embed_fn;          // null ⟹ use lexical fallback

    std::mutex               g_image_embed_mtx;
    UTRConverter::ImageEmbedFn g_image_embed_fn; // null ⟹ use raw-pixel fallback

    std::mutex                       g_text_encoder_mtx;
    std::shared_ptr<ITextEncoder>    g_text_encoder;   // null ⟹ use EmbedFn/lexical

    std::mutex                       g_image_encoder_mtx;
    std::shared_ptr<IImageEncoder>   g_image_encoder;  // null ⟹ use ImageEmbedFn/patch
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

// ============================================================================
// Encoder object registration — ITextEncoder / IImageEncoder
// ============================================================================

void UTRConverter::setTextEncoder(std::shared_ptr<ITextEncoder> encoder) {
    std::lock_guard<std::mutex> lk(g_text_encoder_mtx);
    g_text_encoder = std::move(encoder);
}

void UTRConverter::clearTextEncoder() {
    std::lock_guard<std::mutex> lk(g_text_encoder_mtx);
    g_text_encoder.reset();
}

std::shared_ptr<ITextEncoder> UTRConverter::getTextEncoder() {
    std::lock_guard<std::mutex> lk(g_text_encoder_mtx);
    return g_text_encoder;
}

void UTRConverter::setImageEncoder(std::shared_ptr<IImageEncoder> encoder) {
    std::lock_guard<std::mutex> lk(g_image_encoder_mtx);
    g_image_encoder = std::move(encoder);
}

void UTRConverter::clearImageEncoder() {
    std::lock_guard<std::mutex> lk(g_image_encoder_mtx);
    g_image_encoder.reset();
}

std::shared_ptr<IImageEncoder> UTRConverter::getImageEncoder() {
    std::lock_guard<std::mutex> lk(g_image_encoder_mtx);
    return g_image_encoder;
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
        const std::size_t rx = (t >> 1) & 1;
        const std::size_t ry = (t ^ rx) & 1;
        hilbertRotate(s, x, y, rx, ry);
        x += s * rx;
        y += s * ry;
        t >>= 2;
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
// Document segmentation helpers
// ============================================================================

/// Split text at double-newline boundaries (paragraph mode).
static std::vector<std::string> splitParagraphs(const std::string& text) {
    std::vector<std::string> segments;
    std::size_t start = 0;
    while (static_cast<size_t>(start) <static_cast<int>(text.size())) {
        const auto pos = text.find("\n\n", start);
        const auto end = (pos == std::string::npos) ?static_cast<int>(text.size()) : pos;
        const auto seg = text.substr(start, end - start);
        if (!seg.empty()) {
          segments.push_back(seg);
        }
        start = (pos == std::string::npos) ?static_cast<int>(text.size()) : (pos + 2);
    }
    if (segments.empty()) {
      segments.push_back(text);
    }
    return segments;
}

/// Split text at sentence boundaries (period + space or period + EOT heuristic).
static std::vector<std::string> splitSentences(const std::string& text) {
    std::vector<std::string> segments;
    std::size_t start = 0;
    for (std::size_t i = 0; i <static_cast<int>(text.size()); ++i) {
        if (text[i] == '.' &&
            (i + 1 >= text.size() || text[i + 1] == ' ' || text[i + 1] == '\n'))
        {
            const auto seg = text.substr(start, i + 1 - start);
            if (!seg.empty()) {
              segments.push_back(seg);
            }
            start = i + 1;
            while (start <static_cast<int>(text.size()) && text[start] == ' ') {
              ++start;
            }
        }
    }
    if (static_cast<int>(text.size()) > start) {
        const auto seg = text.substr(start);
        if (!seg.empty()) {
          segments.push_back(seg);
        }
    }
    if (segments.empty()) {
      segments.push_back(text);
    }
    return segments;
}

// ============================================================================
// Lexical feature embedding for documents (built-in degraded-mode fallback)
//
// STUB/SIMULATION NOTE:
// Purpose:           Built-in lexical encoder used when no ITextEncoder or
//                    EmbedFn bridge is registered.  Provides deterministic,
//                    library-free embeddings with LEXICAL quality.
// Activation:        Active when UTRConverter::getTextEncoder() == nullptr and
//                    UTRConverter::getEmbedFn() == nullptr.
// Production Delta:  Does not use learned weights; encodes unigrams, bigrams,
//                    and character trigrams with FNV-1a hashing + L2 norm.
//                    Semantic similarity is approximated, not learned.
// Removal Plan:      Superseded when a plugin registers an ITextEncoder with
//                    EncoderQuality::SEMANTIC.  This fallback is retained as
//                    the bottom tier for offline / zero-dependency operation.
// ============================================================================

/// FNV-1a 64-bit hash of a string view.
static uint64_t fnv1a(std::string_view s) noexcept {
    constexpr uint64_t kBasis = 14695981039346656037;
    constexpr uint64_t kPrime = 1099511628211;
    uint64_t h = kBasis;
    for (const unsigned char c : s) {
        h ^= static_cast<uint64_t>(c);
        h *= kPrime;
    }
    return h;
}

/**
 * @brief Scatter a hashed feature into the embedding vector via multi-lane projection.
 *
 * Uses a multi-lane hashing scheme to spread a single feature across up to 8
 * independent dimensions, reducing the probability of hash collisions causing
 * systematic cancellation.
 *
 * @param vec      Output embedding vector (must be non-empty).
 * @param feature  String feature to hash and scatter.
 * @param weight   Signed scalar weight applied to each projected dimension
 *                 (e.g. 1.0 for unigrams, 0.5 for bigrams, 0.35 for trigrams).
 *
 * @pre `vec.size() > 0`; if zero, the function returns without modification.
 */
static void scatterFeature(std::vector<float>& vec,
                            std::string_view    feature,
                            float               weight) {
    const std::size_t embed_dim = vec.size();
    if (embed_dim == 0) return; // guard: no-op for zero-dim vectors
    const uint64_t h = fnv1a(feature);
    for (std::size_t lane = 0; lane < 8 && lane < embed_dim; ++lane) {
        const auto shift = (lane % 4) * 16;
        const auto bits  = static_cast<uint16_t>((h >> shift) & 0xFFFFULL);
        const auto dim   = static_cast<std::size_t>((bits + lane * 131) % embed_dim);
        const auto sign  = ((h >> (shift + 7)) & 1) ? 1.0f : -1.0f;
        vec[dim] += sign * weight;
    }
}

/**
 * @brief Normalise a raw token for embedding: lowercase + remove non-alphanumeric characters.
 *
 * Passed by value so callers can move lvalue strings in; the modified value is returned.
 *
 * @param token  Raw token (may contain punctuation, mixed case).
 * @return Lowercased, alphanumeric-only version of the input.
 */
static std::string normalizeToken(std::string token) {
    token.erase(std::remove_if(token.begin(), token.end(),
                               [](unsigned char c) {
                                   return !std::isalnum(static_cast<unsigned char>(c));
                               }),
                token.end());
    std::transform(token.begin(), token.end(), token.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return token;
}

/// @brief Delimiter byte separating the two tokens in a bigram feature key.
/// Uses ASCII SOH (0x01) — a control character that cannot appear in
/// normalised (alphanumeric-only, lowercase) tokens.
constexpr char kBigramDelimiter = '\x01';

/**
 * @brief Built-in lexical embedding for a text segment.
 *
 * Combines:
 * - Unigram features (token-level FNV-1a projection)
 * - Bigram features (consecutive token pairs, weight 0.5)
 * - Character trigram features (weight 0.35, only for tokens ≥ 3 chars)
 *
 * The final vector is L2-normalised to unit length, making cosine similarity
 * directly applicable.
 *
 * @param segment   Input text segment.
 * @param embed_dim Output dimensionality.
 * @return L2-normalised float vector of length `embed_dim`.
 */
static std::vector<float> lexicalEmbed(const std::string& segment,
                                        std::size_t        embed_dim) {
    std::vector<float> vec(embed_dim, 0.0f);

    // Tokenize
    std::vector<std::string> tokens;
    {
        std::istringstream iss(segment);
        std::string raw = {};
        while (iss >> raw) {
            auto tok = normalizeToken(std::move(raw));
            if (!tok.empty()) {
              tokens.push_back(std::move(tok));
            }
        }
    }

    if (tokens.empty()) {
        return vec; // all-zero embedding for empty/whitespace-only segments
    }

    // Unigram features
    for (const auto& tok : tokens) {
        scatterFeature(vec, tok, 1.0f);
        // Character trigrams (for tokens ≥ 3 chars)
        if (static_cast<int>(tok.size()) > = 3) {
            for (std::size_t i = 0; i + 3 <= tok.size(); ++i) {
                scatterFeature(vec, std::string_view{tok.data() + i, 3}, 0.35f);
            }
        }
    }

    // Bigram features (consecutive word pairs)
    for (std::size_t i = 0; i + 1 <static_cast<int>(tokens.size()); ++i) {
        const std::string bigram = tokens[i] + kBigramDelimiter + tokens[i + 1];
        scatterFeature(vec, bigram, 0.5f);
    }

    // L2 normalisation to unit length
    float norm_sq = 0.0f;
    for (const auto v : vec) {
      norm_sq += v * v;
    }
    if (norm_sq > 0.0f) {
        const float inv_norm = 1.0f / std::sqrt(norm_sq);
        for (auto& v : vec) {
          v *= inv_norm;
        }
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
    if (static_cast<int>(grid.values.size()) != expected) {
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
    for (std::size_t d = 0; d <static_cast<int>(hilbert_ordered.size()); ++d) {
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
    if (static_cast<int>(pixels.size()) != expected) {
        throw std::invalid_argument(
            "pixels.size() (" + std::to_string(pixels.size()) +
            ") != h*w*c (" + std::to_string(expected) + ")");
    }

    // Priority 1: registered IImageEncoder
    {
        std::shared_ptr<IImageEncoder> enc;
        {
            std::lock_guard<std::mutex> lk(g_image_encoder_mtx);
            enc = g_image_encoder;
        }
        if (enc && enc->isAvailable()) {
            auto result = enc->encode(pixels, h, w, c, cfg);
            if (result.cores.empty()) {
                throw std::runtime_error(
                    "UTRConverter::fromImage: registered IImageEncoder ('" +
                    std::string(enc->description()) +
                    "') returned an empty TTTrain");
            }
            return result;
        }
    }

    // Priority 2: raw ImageEmbedFn bridge
    ImageEmbedFn image_embed_fn;
    {
        std::lock_guard<std::mutex> lk(g_image_embed_mtx);
        image_embed_fn = g_image_embed_fn;
    }
    if (image_embed_fn) {
        return image_embed_fn(pixels, h, w, c, cfg);
    }

    // Priority 3: built-in patch-statistics encoder (degraded mode)
    // STUB/SIMULATION NOTE:
    // Purpose:           Fallback when no IImageEncoder or ImageEmbedFn is set.
    // Activation:        getImageEncoder() == nullptr && getImageEmbedFn() == nullptr.
    // Production Delta:  Uses simple per-patch mean+stddev statistics; no learned
    //                    spatial or semantic features.
    // Removal Plan:      Superseded when a plugin registers an IImageEncoder.
    const auto patch_h = clampPatchExtent(h);
    const auto patch_w = clampPatchExtent(w);
    const auto patch_rows = (h + patch_h - 1) / patch_h;
    const auto patch_cols = (w + patch_w - 1) / patch_w;
    const auto patch_feature_dim = c * 2; // mean + stddev per channel

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
    [[fallthrough]];\n    case DocumentStructureHint::NONE:
    [[fallthrough]];\n    default:
        segments = splitParagraphs(text);
        break;
    }

    // Limit to max_segments to bound the tensor size
    if (static_cast<int>(segments.size()) > cfg.max_segments) {
        segments.resize(cfg.max_segments);
    }
    if (segments.empty()) {
        throw std::invalid_argument("document produced no segments after splitting");
    }

    // 2. Select embedding function — priority chain:
    //    ITextEncoder (registered) > EmbedFn bridge > built-in lexical encoder
    const std::size_t num_segs = segments.size();
    const std::size_t embed_dim = cfg.embed_dim;

    // Snapshot encoder state once to guarantee consistency across all segments
    std::shared_ptr<ITextEncoder> text_encoder;
    {
        std::lock_guard<std::mutex> lk(g_text_encoder_mtx);
        text_encoder = g_text_encoder;
    }
    UTRConverter::EmbedFn embed_fn;
    {
        std::lock_guard<std::mutex> lk(g_embed_mtx);
        embed_fn = g_embed_fn;
    }

    // Resolve active encoder tier
    const bool use_text_encoder = text_encoder && text_encoder->isAvailable();
    const bool use_embed_fn     = !use_text_encoder && static_cast<bool>(embed_fn);

    std::vector<float> segment_matrix;
    segment_matrix.reserve(num_segs * embed_dim);
    for (const auto& seg : segments) {
        std::vector<float> emb = {};

        if (use_text_encoder) {
            // Priority 1: registered ITextEncoder
            emb = text_encoder->encode(seg, embed_dim);
            if (static_cast<int>(emb.size()) != embed_dim) {
                throw std::runtime_error(
                    "UTRConverter::fromDocument: registered ITextEncoder ('" +
                    std::string(text_encoder->description()) +
                    "') returned " + std::to_string(emb.size()) +
                    " elements but embed_dim=" + std::to_string(embed_dim));
            }
        } else if (use_embed_fn) {
            // Priority 2: raw EmbedFn bridge
            emb = embed_fn(seg, embed_dim);
            if (static_cast<int>(emb.size()) != embed_dim) {
                throw std::runtime_error(
                    "UTRConverter::fromDocument: injected EmbedFn returned " +
                    std::to_string(emb.size()) +
                    " elements but embed_dim=" + std::to_string(embed_dim));
            }
        } else {
            // Priority 3: built-in lexical encoder (degraded mode)
            emb = lexicalEmbed(seg, embed_dim);
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
