/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            batch_validator.h                                  ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-13 20:19:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     237                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • e86b6edc79  2026-02-23  feat(acceleration): add BatchValidator and strict input v... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include <cstddef>
#include <cstdint>
#include <functional>

namespace themis {
namespace acceleration {

// ---------------------------------------------------------------------------
// BatchValidator — shared strict input-validation helpers for acceleration
//                  backends.
//
// Each validate_* function checks one invariant and, on failure, calls
// setError() on the owning backend via the provided error sink callback,
// then returns false.  On success it returns true without touching the sink.
//
// Usage pattern (mirrors CUDA backend guards):
//
//   using BV = BatchValidator;
//   auto sink = [this](ErrorContext e){ setError(std::move(e)); };
//
//   if (!BV::validateVectorBatch(backendName, queries, numQueries, dim,
//                                vectors, numVectors, sink)) return {};
// ---------------------------------------------------------------------------
struct BatchValidator {
    using ErrorSink = std::function<void(ErrorContext)>;

    // -----------------------------------------------------------------------
    // Vector batch: computeDistances / batchKnnSearch
    // -----------------------------------------------------------------------

    /// Validate pointer and dimension arguments for a distance or KNN batch.
    /// @returns true if all invariants hold; false (+ error set) otherwise.
    static bool validateVectorBatch(
        const char*   backendName,
        const float*  queries,
        size_t        numQueries,
        size_t        dim,
        const float*  vectors,
        size_t        numVectors,
        const ErrorSink& onError)
    {
        if (queries == nullptr || vectors == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "queries and vectors pointers must be non-null"));
            return false;
        }
        if (numQueries == 0 || numVectors == 0 || dim == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "numQueries, numVectors, and dim must all be > 0"));
            return false;
        }
        return true;
    }

    /// Additional check for k in batchKnnSearch.
    static bool validateK(
        const char*      backendName,
        size_t           k,
        const ErrorSink& onError)
    {
        if (k == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "k must be > 0"));
            return false;
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // Geo batch: batchDistances / batchPointInPolygon
    // -----------------------------------------------------------------------

    /// Validate pointer and count arguments for a geo distance batch.
    static bool validateGeoBatch(
        const char*    backendName,
        const double*  lats1,
        const double*  lons1,
        const double*  lats2,
        const double*  lons2,
        size_t         count,
        const ErrorSink& onError)
    {
        if (lats1 == nullptr || lons1 == nullptr ||
            lats2 == nullptr || lons2 == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "latitude/longitude pointers must be non-null"));
            return false;
        }
        if (count == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "count must be > 0"));
            return false;
        }
        return true;
    }

    /// Validate pointer and count arguments for a point-in-polygon batch.
    static bool validatePointInPolygonBatch(
        const char*    backendName,
        const double*  pointLats,
        const double*  pointLons,
        size_t         numPoints,
        const double*  polygonCoords,
        size_t         numPolygonVertices,
        const ErrorSink& onError)
    {
        if (pointLats == nullptr || pointLons == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "point latitude/longitude pointers must be non-null"));
            return false;
        }
        if (polygonCoords == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "polygonCoords pointer must be non-null"));
            return false;
        }
        if (numPoints == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "numPoints must be > 0"));
            return false;
        }
        if (numPolygonVertices < 3) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "numPolygonVertices must be >= 3 to form a valid polygon"));
            return false;
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // Graph batch: batchBFS / batchShortestPath
    // -----------------------------------------------------------------------

    /// Validate pointer and count arguments for a graph BFS batch.
    static bool validateGraphBFSBatch(
        const char*      backendName,
        const uint32_t*  adjacency,
        size_t           numVertices,
        const uint32_t*  startVertices,
        size_t           numStarts,
        const ErrorSink& onError)
    {
        if (adjacency == nullptr || startVertices == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "adjacency and startVertices pointers must be non-null"));
            return false;
        }
        if (numVertices == 0 || numStarts == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "numVertices and numStarts must both be > 0"));
            return false;
        }
        return true;
    }

    /// Validate pointer and count arguments for a shortest-path batch.
    static bool validateShortestPathBatch(
        const char*      backendName,
        const uint32_t*  adjacency,
        const float*     weights,
        size_t           numVertices,
        const uint32_t*  startVertices,
        const uint32_t*  endVertices,
        size_t           numPairs,
        const ErrorSink& onError)
    {
        if (adjacency == nullptr || weights == nullptr ||
            startVertices == nullptr || endVertices == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "adjacency, weights, startVertices, and endVertices pointers "
                "must be non-null"));
            return false;
        }
        if (numVertices == 0 || numPairs == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "numVertices and numPairs must both be > 0"));
            return false;
        }
        return true;
    }
};

} // namespace acceleration
} // namespace themis
