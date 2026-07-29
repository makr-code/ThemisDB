/*
 * ThemisDB | File: geo_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen geo module API contracts for the active v1.x major line.
 */

/**
 * @file geo_api_contract.h
 * @brief Frozen geo module API contracts for the active v1.x major line.
 *
 * This header defines the normative, binding contract for the ThemisDB geo
 * module covering:
 *   - GeoJSON validation (input contract, coordinate bounds enforcement)
 *   - Backend dispatch (CPU/GPU selection, fallback order, accuracy guarantees)
 *   - Spatial index (insert/query contract, bounding-box consistency)
 *   - Spatial join (result completeness for intersect/contains/within)
 *   - Canonical error taxonomy
 *
 * ## Contract Scope
 *
 * These contracts are binding for all v1.x implementations:
 *   - Geometry processors (GeoJsonGeometry, GeoOpsExt, GeoMath)
 *   - Spatial backends (SpatialBackend, GpuKernelDispatcher, DeviceDetector)
 *   - Spatial indexes (GeoRtree, RtreeCursor)
 *   - Spatial join (SpatialJoin, SpatialJoinFilter)
 *   - Clustering (GeoClustering)
 *   - Temporal-spatial query (TemporalSpatialQuery)
 *
 * ## Versioning
 *
 * Stable within v1.x.  Breaking changes require v2.0 with migration notes.
 *
 * @see src/geo/ROADMAP.md — Phase 1 frozen contract items
 * @see include/geo/geo_json_geometry.h  — Geometry types
 * @see include/geo/spatial_backend.h   — Backend interface
 * @see include/geo/geo_rtree.h         — Spatial index interface
 * @see include/geo/spatial_join.h      — Spatial join interface
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

namespace themis {
namespace geo {

// ============================================================================
// § 1  GeoJSON Validation Contract
//
// All geometry inputs MUST pass the following validation before being used in
// any spatial operation:
//
//   a) VALID GEOJSON: Geometry objects must conform to RFC 7946.  Invalid
//      objects (null coordinates, wrong type, malformed arrays) → GEOMETRY_INVALID.
//
//   b) WGS84 BOUNDS: Longitude must be in [-180, +180]; latitude in [-90, +90].
//      Out-of-bounds coordinates → COORDINATE_OUT_OF_BOUNDS.
//
//   c) RING CLOSURE: Polygon exterior and interior rings MUST be closed
//      (first coordinate == last coordinate).  Open rings → GEOMETRY_INVALID.
//
//   d) MINIMUM VERTICES: A valid LineString requires ≥ 2 points; a Polygon
//      exterior ring requires ≥ 4 points (including the closing point).
//
//   e) NULL REJECTION: Null or empty geometry inputs are rejected with
//      GEOMETRY_INVALID before any backend is invoked.
// ============================================================================

/// Maximum number of coordinates in a single geometry object.
/// Geometries exceeding this limit return GEOMETRY_TOO_LARGE.
inline constexpr std::size_t kMaxGeometryCoordinates = 10'000'000;

/// Maximum nesting depth for GeometryCollection objects.
inline constexpr int kMaxGeometryCollectionDepth = 16;

/// WGS84 longitude bounds.
inline constexpr double kWgs84LonMin = -180.0;
inline constexpr double kWgs84LonMax = +180.0;

/// WGS84 latitude bounds.
inline constexpr double kWgs84LatMin = -90.0;
inline constexpr double kWgs84LatMax = +90.0;

// ============================================================================
// § 2  Backend Dispatch Contract
//
// Spatial operations use a two-tier dispatch hierarchy:
//
//   Primary: GPU (CUDA/HIP) — selected when a GPU is present and the input
//            size exceeds kGpuDispatchMinBatchSize.
//   Fallback: CPU — always available; used when GPU is absent or unavailable.
//
// Invariants:
//   a) GPU unavailable → automatic, silent fallback to CPU.
//   b) Fallback does NOT produce different results; numeric accuracy is
//      equivalent within kBackendAccuracyTolerance.
//   c) BACKEND_UNAVAILABLE is returned only when BOTH GPU and CPU fail; this
//      state is considered an internal error.
//   d) Backend selection latency overhead must not exceed kBackendSelectionBudget.
// ============================================================================

/// Minimum batch size before the GPU backend is preferred over CPU.
inline constexpr std::size_t kGpuDispatchMinBatchSize = 1024;

/// Maximum acceptable numeric difference between CPU and GPU results for the
/// same spatial predicate (relative tolerance).
inline constexpr double kBackendAccuracyTolerance = 1e-9;

/// Maximum time the backend selection decision may consume.
inline constexpr std::chrono::microseconds kBackendSelectionBudget{100};

// ============================================================================
// § 3  Spatial Index Contract
//
// The spatial index (R-tree or equivalent) MUST satisfy:
//
//   a) INSERT DURABILITY: Once insert() returns success, the entry is queryable
//      immediately from the same index instance.
//
//   b) BOUNDING-BOX CONSISTENCY: A query with bounding box B MUST return all
//      entries whose MBR intersects B.  It MUST NOT return entries whose MBR
//      does not intersect B.
//
//   c) CONCURRENT SAFETY: Index operations are safe from multiple threads when
//      using the documented locking contract (read-many, write-exclusive).
//
//   d) MAX CAPACITY: Inserting beyond kSpatialIndexMaxEntries returns
//      INDEX_CAPACITY_EXCEEDED.
// ============================================================================

/// Maximum number of entries in a single spatial index instance.
inline constexpr std::size_t kSpatialIndexMaxEntries = 100'000'000;  ///< 100M.

/// Maximum query result set size returned by a single spatial query.
/// Larger result sets are paginated; callers use a cursor for continuation.
inline constexpr std::size_t kSpatialQueryMaxResults = 100'000;

// ============================================================================
// § 4  Spatial Join Contract
//
// Spatial join operations (intersect, contains, within) MUST satisfy:
//
//   a) COMPLETENESS: The result set for predicate P on sets A and B contains
//      ALL pairs (a ∈ A, b ∈ B) for which P(a, b) is true.  No qualifying
//      pair may be silently omitted.
//
//   b) SOUNDNESS: The result set contains ONLY pairs for which P(a, b) is
//      true.  False positives from bounding-box pre-filters MUST be eliminated
//      by exact predicate evaluation before inclusion in the result.
//
//   c) INVALID INPUT: If either input set contains GEOMETRY_INVALID entries,
//      the join returns an error rather than silently skipping those entries
//      (unless the caller sets the skip_invalid flag explicitly).
//
//   d) EMPTY SET: A spatial join with an empty input set returns an empty
//      result set, not an error.
// ============================================================================

/// Maximum number of input geometries per side of a spatial join.
inline constexpr std::size_t kSpatialJoinMaxInputSize = 10'000'000;

// ============================================================================
// § 5  Error Taxonomy
//
// Codes 1–99: geometry validation; 100–199: backend; 200–299: index;
// 300–399: join/query; 9xxx: internal.
// ============================================================================

/**
 * @brief Canonical error codes for the ThemisDB geo module.
 *
 * All geo operation failures MUST map to one of these codes before being
 * returned to callers or emitted in metrics/audit events.
 */
enum class GeoErrorCode : int {
    // ── Geometry Validation ───────────────────────────────────────────────────
    /// Geometry object is structurally invalid (null coords, bad type, etc.).
    GEOMETRY_INVALID            = 1,
    /// Coordinate value is outside WGS84 bounds.
    COORDINATE_OUT_OF_BOUNDS    = 2,
    /// Geometry has too many coordinates (exceeds kMaxGeometryCoordinates).
    GEOMETRY_TOO_LARGE          = 3,
    /// Geometry type is recognised by the spec but not supported in this build.
    UNSUPPORTED_GEOMETRY_TYPE   = 4,
    /// Numeric precision of the geometry exceeds representable range.
    PRECISION_EXCEEDED          = 5,

    // ── Backend ───────────────────────────────────────────────────────────────
    /// Both GPU and CPU backends are unavailable (internal error).
    BACKEND_UNAVAILABLE         = 100,
    /// GPU backend failed; CPU fallback was used (informational, not fatal).
    GPU_FALLBACK_TO_CPU         = 101,
    /// Backend result is inconsistent with the other backend's result.
    BACKEND_RESULT_MISMATCH     = 102,

    // ── Spatial Index ─────────────────────────────────────────────────────────
    /// Spatial index data structure is corrupted.
    INDEX_CORRUPTED             = 200,
    /// Spatial index has reached kSpatialIndexMaxEntries.
    INDEX_CAPACITY_EXCEEDED     = 201,
    /// Query returned more results than kSpatialQueryMaxResults; use cursor.
    QUERY_RESULT_TRUNCATED      = 202,

    // ── Spatial Join / Query ──────────────────────────────────────────────────
    /// One or more input geometries for the spatial join are invalid.
    JOIN_INVALID_INPUT          = 300,
    /// Input set exceeds kSpatialJoinMaxInputSize.
    JOIN_INPUT_TOO_LARGE        = 301,
    /// Query timed out before completion.
    QUERY_TIMEOUT               = 302,

    // ── Generic ───────────────────────────────────────────────────────────────
    /// Operation succeeded.
    OK                          = 0,
    /// Unclassified internal geo error.
    INTERNAL_ERROR              = 9999,
};

// ============================================================================
// § 6  Fail-Closed / Fail-Safe Classification Helpers
// ============================================================================

/**
 * @brief Returns true when @p code is a geometry validity error that prevents
 *        the operation from proceeding (fail-closed for invalid input).
 */
[[nodiscard]] inline constexpr bool isGeometryError(GeoErrorCode code) noexcept {
    return code == GeoErrorCode::GEOMETRY_INVALID
        || code == GeoErrorCode::COORDINATE_OUT_OF_BOUNDS
        || code == GeoErrorCode::GEOMETRY_TOO_LARGE
        || code == GeoErrorCode::UNSUPPORTED_GEOMETRY_TYPE;
}

/**
 * @brief Returns true when @p code represents a backend fallback event
 *        (informational — the operation may still succeed via CPU).
 */
[[nodiscard]] inline constexpr bool isBackendFallback(GeoErrorCode code) noexcept {
    return code == GeoErrorCode::GPU_FALLBACK_TO_CPU;
}

// ============================================================================
// § 7  Contract Conformance Notes
//
// All geo module implementations MUST:
//   1. Validate geometry against § 1 before invoking any backend operation.
//   2. Apply CPU fallback automatically when GPU is unavailable (§ 2).
//   3. Eliminate bounding-box false positives with exact predicate checks (§ 4).
//   4. Return GEOMETRY_INVALID (not a silent skip) for null coordinates.
//   5. Return GeoErrorCode values (or Expected<T, GeoErrorCode>).
// ============================================================================

}  // namespace geo
}  // namespace themis
