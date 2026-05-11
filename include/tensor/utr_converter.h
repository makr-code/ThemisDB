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
 * - `fromDocument()` now uses normalized token features plus hashed character
 *   trigram features for each segment before HT decomposition.  A learned
 *   sentence encoder / discourse-guided topology is still deferred.
 *
 * - `fromImage()` now performs non-overlapping patch aggregation (mean +
 *   stddev per channel) before TT decomposition.  Learned semantic image
 *   embeddings remain deferred to Q4 2028.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"
#include "tensor/ht_train.h"
#include "tensor/hyper_index_builder.h"

#include <cstddef>
#include <cstdint>
#include <functional>
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
 * - **EmbedFn** (`setEmbedFn`) — replaces the FNV-1a hash-projection embedding
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
     * Values are normalised to [0, 1] and TT-decomposed directly.
     *
     * @param pixels  Flat HWC float values (length = h × w × c).
     * @param h       Image height in pixels.
     * @param w       Image width in pixels.
     * @param c       Number of channels (1 = grayscale, 3 = RGB, etc.)
     * @param cfg     UTR configuration.
     * @return TT-train encoding the image tensor.
     *
     * @throws std::invalid_argument if h, w, or c is 0 or pixels.size() != h*w*c.
     *
      * @note Uses patch statistics today; learned semantic image embeddings are
      *       still deferred to Q4 2028.
     */
    [[nodiscard]] static storage::TTTrain
    fromImage(const std::vector<float>& pixels,
              std::size_t h, std::size_t w, std::size_t c,
              const UTRConfig& cfg = {});

    /**
     * @brief Encode a document as a hierarchical TT (HTTrain).
     *
     * The document is split into segments (paragraphs or sentences depending on
      * `hint`).  Each segment is encoded as a fixed-length embedding vector via
      * normalized token + hashed trigram features.  The resulting
      * segment × embed_dim matrix
     * is decomposed via HT-SVD into an HTTrain.
     *
     * @param text   Raw document text (UTF-8).
     * @param hint   Segmentation strategy.
     * @param cfg    UTR configuration.
     * @return HTTrain encoding the hierarchical document structure.
     *
     * @throws std::invalid_argument if text is empty.
     *
      * @note Learned sentence encoders / discourse-graph HT topology remain
      *       deferred to Q4 2028.
     */
    [[nodiscard]] static tensor::HTTrain
    fromDocument(const std::string&       text,
                 DocumentStructureHint    hint = DocumentStructureHint::PARAGRAPHS,
                 const UTRConfig&         cfg  = {});
};

} // namespace tensor
} // namespace themis
