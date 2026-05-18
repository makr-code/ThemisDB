/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/utr_converter.h                             ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 7 (Q3–Q4 2028)                      ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/utr_converter.h
 * @brief UTRConverter — unified tensor representation pipeline for heterogeneous data.
 *
 * ## Overview
 *
 * `UTRConverter` converts geospatial, relational, image, and document data into
 * `TTTrain` or `HTTrain` objects that are directly indexable by
 * `TensorIndexManager`.
 *
 * All outputs preserve the following invariants:
 * - Dense round-trip RMSE ≤ the configured `eps` threshold.
 * - No tenant data mixing — each converter method is scoped to a single dataset.
 * - Coordinate precision loss ≤ 1e-7 for geospatial inputs.
 *
 * ## Current implementation status
 *
 * - `fromGeospatial()` encodes raster values in Hilbert-curve traversal order
 *   (with power-of-two square padding) before TT-SVD, improving locality
 *   retention for non-axis-aligned neighbors.
 *
 * - `fromDocument()` uses a lexical encoder that combines unigram, bigram, and
 *   character-trigram features with L2 normalization.  A learned sentence
 *   encoder backend can be registered via `setTextEncoder()` to replace the
 *   built-in lexical fallback.
 *
 * - `fromImage()` performs non-overlapping patch aggregation (mean + stddev per
 *   channel) before TT decomposition.  A learned patch-embedding backend can be
 *   registered via `setImageEncoder()` to replace the built-in patch statistics.
 *
 * ## Encoder Priority Chain
 *
 * For both `fromDocument()` and `fromImage()`, the active encoder is selected
 * according to the following priority (highest to lowest):
 *
 * 1. Registered `ITextEncoder` / `IImageEncoder` (if `isAvailable()` is true)
 * 2. Injected `EmbedFn` / `ImageEmbedFn` bridge function (STUB #257 / #258)
 * 3. Built-in lexical / patch-statistics encoder (degraded mode)
 *
 * When the built-in fallback is active, the encoder quality is `EncoderQuality::LEXICAL`
 * for documents and `EncoderQuality::HASH` for images.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"
#include "tensor/encoder_interface.h"
#include "tensor/ht_train.h"
#include "tensor/hyper_index_builder.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// RasterGrid — geospatial input type
// ============================================================================

/**
 * @brief A regular raster grid of scalar observations.
 *
 * Cell (row, col) carries a scalar value such as flood risk, temperature, or
 * population density.  Coordinates are WGS84 degrees.
 *
 * Precision contract: lat/lon stored as double; round-trip via UTR must not
 * lose > 1e-7 degrees (checked by `fromGeospatial`).
 */
struct RasterGrid {
    std::size_t         rows         = 0;   ///< Number of latitude rows
    std::size_t         cols         = 0;   ///< Number of longitude columns
    double              lat_min      = 0.0; ///< Bottom-left latitude (WGS84)
    double              lon_min      = 0.0; ///< Bottom-left longitude (WGS84)
    double              cell_size_deg= 0.0; ///< Cell size in degrees (uniform)
    std::vector<float>  values;             ///< Row-major scalar values (rows × cols)
};

// ============================================================================
// DocumentStructureHint — optional segmentation hints for fromDocument()
// ============================================================================

enum class DocumentStructureHint : uint8_t {
    NONE       = 0,  ///< No structural information; uniform paragraph splits
    PARAGRAPHS = 1,  ///< Splits at double-newlines
    SENTENCES  = 2   ///< Splits at sentence boundaries (period+space heuristic)
};

// ============================================================================
// UTRConfig — shared configuration for all UTRConverter methods
// ============================================================================

struct UTRConfig {
    double      eps           = 0.01;  ///< TT/HT reconstruction error tolerance
    std::size_t max_rank      = 16;    ///< Hard cap on TT/HT rank
    std::size_t embed_dim     = 64;    ///< Embedding dimension for document tokens
    std::size_t max_segments  = 16;    ///< Maximum text segments per document
    std::size_t bucket_count  = 8;     ///< Buckets per column for tabular data
};

// ============================================================================
// UTRConverter
// ============================================================================

/**
 * @brief Converts heterogeneous data sources into tensor-native (TT/HT) formats.
 *
 * All methods are static and stateless.  Thread-safe.
 *
 * ### Bridge Injection APIs (STUB #257 / #258)
 *
 * `UTRConverter` exposes two injectable bridges so that real encoder backends
 * can replace the built-in fallback implementations without changing callers:
 *
 * - **EmbedFn** (`setEmbedFn`) — replaces the built-in lexical embedding
 *   used in `fromDocument()`.  The callable receives `(segment, embed_dim)` and
 *   must return a `std::vector<float>` of exactly `embed_dim` elements.
 *
 * - **ImageEmbedFn** (`setImageEmbedFn`) — replaces the raw-pixel TT encoding
 *   in `fromImage()`.  The callable receives the flat HWC pixel buffer and the
 *   three dimensions plus the UTR config, and must return a valid `TTTrain`.
 *
 * Both bridges are stored in static `std::mutex`-guarded slots.  Call
 * `clearEmbedFn()` / `clearImageEmbedFn()` to revert to the built-in fallback.
 * The fallback is retained when no bridge is set (fail-open for encode quality,
 * not for correctness — structural invariants are always maintained).
 *
 * ### Encoder Object APIs (preferred over raw bridge functions)
 *
 * Higher-level plugins should implement `ITextEncoder` / `IImageEncoder` and
 * register them via `setTextEncoder()` / `setImageEncoder()`.  These encoder
 * objects take priority over the raw `EmbedFn` / `ImageEmbedFn` bridges.
 * If `isAvailable()` returns false on the registered encoder, the system
 * falls back to the bridge function tier (if any), then to the built-in
 * lexical/patch encoder.
 */
class UTRConverter {
public:
    // -------------------------------------------------------------------------
    // STUB #257 bridge — sentence/document embedding
    // -------------------------------------------------------------------------

    /**
     * @brief Callable type for text-segment embedding.
     *
     * @param segment   UTF-8 text segment.
     * @param embed_dim Required output dimensionality.
     * @return Float vector of length `embed_dim`.
     *
     * The returned vector MUST have exactly `embed_dim` elements; if it does
     * not, `fromDocument()` throws `std::runtime_error`.
     */
    using EmbedFn = std::function<std::vector<float>(const std::string& segment,
                                                      std::size_t        embed_dim)>;

    /// Replace the FNV-1a hash-projection fallback with a real encoder.
    static void setEmbedFn(EmbedFn fn);

    /// Revert `fromDocument()` to the built-in FNV-1a fallback.
    static void clearEmbedFn();

    /// Returns the currently installed EmbedFn, or an empty std::function if none.
    static EmbedFn getEmbedFn();

    // -------------------------------------------------------------------------
    // STUB #258 bridge — image encoding
    // -------------------------------------------------------------------------

    /**
     * @brief Callable type for image-to-TTTrain encoding.
     *
     * @param pixels  Flat HWC float buffer (raw, not normalised).
     * @param h, w, c Image dimensions.
     * @param cfg     UTR configuration.
     * @return A valid TTTrain for the given image.
     */
    using ImageEmbedFn = std::function<storage::TTTrain(
                                const std::vector<float>& pixels,
                                std::size_t h,
                                std::size_t w,
                                std::size_t c,
                                const UTRConfig& cfg)>;

    /// Replace the raw-pixel TT encoding with a real patch-embedding backend.
    static void setImageEmbedFn(ImageEmbedFn fn);

    /// Revert `fromImage()` to the built-in raw-pixel fallback.
    static void clearImageEmbedFn();

    /// Returns the currently installed ImageEmbedFn, or an empty std::function.
    static ImageEmbedFn getImageEmbedFn();

    // -------------------------------------------------------------------------
    // Encoder object registration — preferred over raw bridge functions
    // -------------------------------------------------------------------------

    /**
     * @brief Register a learned or high-quality text encoder.
     *
     * The encoder replaces the built-in lexical fallback in `fromDocument()`.
     * If `encoder->isAvailable()` returns false at call time, the system falls
     * back to the `EmbedFn` bridge (if set) or the built-in lexical encoder.
     *
     * Pass `nullptr` to clear any previously registered encoder.
     *
     * @param encoder  Shared pointer to an `ITextEncoder` implementation, or
     *                 nullptr to clear.
     */
    static void setTextEncoder(std::shared_ptr<ITextEncoder> encoder);

    /// Clear the registered text encoder; `fromDocument()` reverts to the
    /// `EmbedFn` bridge tier or the built-in lexical encoder.
    static void clearTextEncoder();

    /// Returns the currently registered `ITextEncoder`, or nullptr if none.
    [[nodiscard]] static std::shared_ptr<ITextEncoder> getTextEncoder();

    /**
     * @brief Register a learned or high-quality image encoder.
     *
     * The encoder replaces the built-in patch-statistics fallback in
     * `fromImage()`.  If `encoder->isAvailable()` returns false at call time,
     * the system falls back to the `ImageEmbedFn` bridge (if set) or the
     * built-in patch-statistics encoder.
     *
     * Pass `nullptr` to clear any previously registered encoder.
     *
     * @param encoder  Shared pointer to an `IImageEncoder` implementation, or
     *                 nullptr to clear.
     */
    static void setImageEncoder(std::shared_ptr<IImageEncoder> encoder);

    /// Clear the registered image encoder; `fromImage()` reverts to the
    /// `ImageEmbedFn` bridge tier or the built-in patch-statistics encoder.
    static void clearImageEncoder();

    /// Returns the currently registered `IImageEncoder`, or nullptr if none.
    [[nodiscard]] static std::shared_ptr<IImageEncoder> getImageEncoder();

    /**
     * @brief Encode a geospatial raster grid as a TT-train.
     *
     * The grid is treated as a 2-D tensor T ∈ ℝ^{rows × cols} and decomposed
     * via TT-SVD after Hilbert-curve reordering on a power-of-two square grid.
     *
     * @param grid   Input raster grid (rows × cols scalar values).
     * @param cfg    UTR configuration (eps, max_rank).
     * @return TT-train encoding the geospatial field.
     *
     * @throws std::invalid_argument if grid is empty or cell_size_deg ≤ 0.
     */
    [[nodiscard]] static storage::TTTrain
    fromGeospatial(const RasterGrid& grid, const UTRConfig& cfg = {});

    /**
     * @brief Encode tabular data as a HyperIndexTensor (TT-encoded co-occurrence).
     *
     * Delegates to `HyperIndexBuilder::fromSchema()`.
     *
     * @param tenant_id  Owning tenant.
     * @param schema     Column descriptors.
     * @param rows       Data rows.
     * @param cfg        UTR configuration.
     * @return HyperIndexTensor encoding cross-column relationships.
     *
     * @throws std::invalid_argument if schema has < 2 columns or rows is empty.
     */
    [[nodiscard]] static HyperIndexTensor
    fromTabular(const std::string&               tenant_id,
                const std::vector<ColumnSchema>& schema,
                const std::vector<TableRow>&     rows,
                const UTRConfig&                 cfg = {});

    /**
     * @brief Encode pixel data as a TT-train.
     *
     * Input shape is (h × w × c) stored in row-major HWC order.
     * Values are normalised to [0, 1] before patch-statistics extraction and
     * TT decomposition.
     *
     * If a learned `IImageEncoder` is registered and `isAvailable()`, it is
     * used in preference to the built-in patch-statistics encoder.
     *
     * @param pixels  Flat HWC float values (length = h × w × c).
     * @param h       Image height in pixels.
     * @param w       Image width in pixels.
     * @param c       Number of channels (1 = grayscale, 3 = RGB, etc.)
     * @param cfg     UTR configuration.
     * @return TT-train encoding the image tensor.
     *
     * @throws std::invalid_argument if h, w, or c is 0 or pixels.size() != h*w*c.
     * @throws std::runtime_error if a registered `IImageEncoder` produces an
     *         empty TTTrain (fail-closed on encoder contract violation).
     */
    [[nodiscard]] static storage::TTTrain
    fromImage(const std::vector<float>& pixels,
              std::size_t h, std::size_t w, std::size_t c,
              const UTRConfig& cfg = {});

    /**
     * @brief Encode a document as a hierarchical TT (HTTrain).
     *
     * The document is split into segments (paragraphs or sentences depending on
     * `hint`).  Each segment is encoded as a fixed-length embedding vector.
     * The encoding tier is selected by priority:
     * 1. Registered `ITextEncoder` (if `isAvailable()`)
     * 2. Injected `EmbedFn` bridge (STUB #257)
     * 3. Built-in lexical encoder (unigram + bigram + char-trigram features,
     *    L2-normalised) — degraded mode, `EncoderQuality::LEXICAL`
     *
     * The resulting segment × embed_dim matrix is decomposed via HT-SVD into
     * an HTTrain.
     *
     * @param text   Raw document text (UTF-8).
     * @param hint   Segmentation strategy.
     * @param cfg    UTR configuration.
     * @return HTTrain encoding the hierarchical document structure.
     *
     * @throws std::invalid_argument if text is empty or produces no segments.
     * @throws std::runtime_error if the active `EmbedFn` or `ITextEncoder`
     *         returns a vector of the wrong size (fail-closed on mismatch).
     */
    [[nodiscard]] static tensor::HTTrain
    fromDocument(const std::string&       text,
                 DocumentStructureHint    hint = DocumentStructureHint::PARAGRAPHS,
                 const UTRConfig&         cfg  = {});
};

} // namespace tensor
} // namespace themis
