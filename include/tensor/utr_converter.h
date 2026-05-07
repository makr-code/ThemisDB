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
 * ## Stub status
 *
 * - STUB #256: `fromGeospatial()` — raster grid encoded as TT via row-major
 *   unfolding + TT-SVD.  Neighborhood preservation is structural (locality of
 *   grid coordinates in row-major order) rather than curvature-aware.
 *   Full topology-preserving encoding (geomorphic TT mode ordering) deferred to
 *   Q3 2028.
 *
 * - STUB #257: `fromDocument()` — hierarchical sentence embedding; uses uniform
 *   paragraph splits rather than a learned segmentation model.  Full HT encoding
 *   with discourse-graph-guided tree topology deferred to Q4 2028.
 *
 * - STUB #258: `fromImage()` — 3-D / 4-D TT encoding of pixel data in
 *   H×W×C mode ordering.  No semantic alignment or patch embedding performed;
 *   pixel values are normalised to [0, 1] and decomposed directly.
 *   Patch-based structural similarity embedding deferred to Q4 2028.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"
#include "tensor/ht_train.h"
#include "tensor/hyper_index_builder.h"

#include <cstddef>
#include <cstdint>
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
 */
class UTRConverter {
public:
    /**
     * @brief Encode a geospatial raster grid as a TT-train.
     *
     * The grid is treated as a 2-D tensor T ∈ ℝ^{rows × cols} and decomposed
     * via TT-SVD.  Spatial locality is preserved by row-major mode ordering
     * (adjacent cells map to adjacent index tuples).
     *
     * @param grid   Input raster grid (rows × cols scalar values).
     * @param cfg    UTR configuration (eps, max_rank).
     * @return TT-train encoding the geospatial field.
     *
     * @throws std::invalid_argument if grid is empty or cell_size_deg ≤ 0.
     *
     * @note STUB #256 — topology-preserving mode ordering deferred to Q3 2028.
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
     * @note STUB #258 — patch-based structural similarity embedding deferred to Q4 2028.
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
     * a hash-projection (STUB #257).  The resulting segment × embed_dim matrix
     * is decomposed via HT-SVD into an HTTrain.
     *
     * @param text   Raw document text (UTF-8).
     * @param hint   Segmentation strategy.
     * @param cfg    UTR configuration.
     * @return HTTrain encoding the hierarchical document structure.
     *
     * @throws std::invalid_argument if text is empty.
     *
     * @note STUB #257 — discourse-graph HT topology deferred to Q4 2028.
     */
    [[nodiscard]] static tensor::HTTrain
    fromDocument(const std::string&       text,
                 DocumentStructureHint    hint = DocumentStructureHint::PARAGRAPHS,
                 const UTRConfig&         cfg  = {});
};

} // namespace tensor
} // namespace themis
