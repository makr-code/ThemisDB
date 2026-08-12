/**
 * @file batch_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include <cstddef>
#include <cstdint>
#include <functional>

namespace themis {
namespace acceleration {

/// @brief Shared strict input-validation helpers for acceleration backends.
///
/// Provides static validation functions for common batch operation invariants.
/// Each validate_* function checks one invariant and, on failure, calls
/// setError() on the owning backend via the provided error sink callback,
/// then returns false. On success it returns true without touching the sink.
///
/// ## Usage Pattern
/// @code
///   using BV = BatchValidator;
///   auto sink = [this](ErrorContext e){ setError(std::move(e)); };
///
///   if (!BV::validateVectorBatch(backendName, queries, numQueries, dim,
///                                vectors, numVectors, sink)) return {};
///   if (!BV::validateK(backendName, k, sink)) return {};
/// @endcode
///
/// ## Error Handling
/// All validation failures are communicated through the ErrorSink callback.
/// Backends implement setError() to store and log errors for later retrieval
/// via getLastError().
struct BatchValidator {
    /// @brief Error sink callback type.
    /// Called when a validation check fails. Receives an ErrorContext describing
    /// the validation error. The backend stores this error for later retrieval.
    using ErrorSink = std::function<void(ErrorContext)>;

    // -----------------------------------------------------------------------
    // Vector batch: computeDistances / batchKnnSearch
    // -----------------------------------------------------------------------

    /// @brief Validate pointer and dimension arguments for a distance or KNN batch.
    ///
    /// Checks that all pointers are non-null and all counts are positive.
    /// This is the common validation for distance computation and KNN search.
    ///
    /// @param backendName   Name of the backend (for error messages)
    /// @param queries       Query matrix pointer (must be non-null)
    /// @param numQueries    Number of queries (must be > 0)
    /// @param dim           Vector dimensionality (must be > 0)
    /// @param vectors       Database vector matrix pointer (must be non-null)
    /// @param numVectors    Number of database vectors (must be > 0)
    /// @param onError       Callback to invoke on validation failure
    /// @return true if all invariants hold; false (+ error set) otherwise
    ///
    /// @note On failure, onError() is called exactly once with an
    /// AccelerationErrorCode::InvalidInputShape error.
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

    /// @brief Additional check for k in batchKnnSearch.
    ///
    /// Validates that k (top-k count) is positive. Should be called after
    /// validateVectorBatch() for KNN operations.
    ///
    /// @param backendName   Name of the backend (for error messages)
    /// @param k             Number of top neighbors to retrieve (must be > 0)
    /// @param onError       Callback to invoke on validation failure
    /// @return true if k > 0; false (+ error set) otherwise
    ///
    /// @note On failure, onError() is called exactly once with an
    /// AccelerationErrorCode::InvalidInputShape error.
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

    /// @brief Validate pointer and count arguments for a geo distance batch.
    ///
    /// Checks that all coordinate pointers are non-null and count is positive.
    /// Used before geospatial distance computation.
    ///
    /// @param backendName   Name of the backend (for error messages)
    /// @param lats1        First set of latitudes pointer (must be non-null)
    /// @param lons1        First set of longitudes pointer (must be non-null)
    /// @param lats2        Second set of latitudes pointer (must be non-null)
    /// @param lons2        Second set of longitudes pointer (must be non-null)
    /// @param count        Number of point pairs (must be > 0)
    /// @param onError      Callback to invoke on validation failure
    /// @return true if all invariants hold; false (+ error set) otherwise
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

    /// @brief Validate pointer and count arguments for a point-in-polygon batch.
    ///
    /// Checks pointers are non-null, counts are positive, and polygon has
    /// at least 3 vertices (forms a valid triangle).
    ///
    /// @param backendName         Name of the backend (for error messages)
    /// @param pointLats           Test point latitudes pointer (must be non-null)
    /// @param pointLons           Test point longitudes pointer (must be non-null)
    /// @param numPoints           Number of test points (must be > 0)
    /// @param polygonCoords       Polygon vertex coordinates pointer (must be non-null)
    /// @param numPolygonVertices  Number of polygon vertices (must be >= 3)
    /// @param onError             Callback to invoke on validation failure
    /// @return true if all invariants hold; false (+ error set) otherwise
    ///
    /// @note The polygon must have at least 3 vertices to form a valid polygon.
    /// For edge cases (numPolygonVertices == 3), the polygon is a degenerate
    /// triangle but still valid for containment testing.
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

    /// @brief Validate pointer and count arguments for a graph BFS batch.
    ///
    /// Checks adjacency and start vertex pointers are non-null and counts are positive.
    ///
    /// @param backendName     Name of the backend (for error messages)
    /// @param adjacency       Graph adjacency matrix pointer (must be non-null)
    /// @param numVertices     Total number of vertices (must be > 0)
    /// @param startVertices   Starting vertex indices pointer (must be non-null)
    /// @param numStarts       Number of starting vertices (must be > 0)
    /// @param onError         Callback to invoke on validation failure
    /// @return true if all invariants hold; false (+ error set) otherwise
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

    /// @brief Validate pointer and count arguments for a shortest-path batch.
    ///
    /// Checks all pointers are non-null and counts are positive. Used before
    /// Dijkstra or similar shortest-path computations.
    ///
    /// @param backendName     Name of the backend (for error messages)
    /// @param adjacency       Graph adjacency matrix pointer (must be non-null)
    /// @param weights         Edge weights matrix pointer (must be non-null)
    /// @param numVertices     Total number of vertices (must be > 0)
    /// @param startVertices   Source vertex indices pointer (must be non-null)
    /// @param endVertices     Destination vertex indices pointer (must be non-null)
    /// @param numPairs        Number of (source, destination) pairs (must be > 0)
    /// @param onError         Callback to invoke on validation failure
    /// @return true if all invariants hold; false (+ error set) otherwise
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
