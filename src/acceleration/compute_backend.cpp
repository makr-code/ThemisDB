/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compute_backend.cpp                                ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:48:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     102                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * Acceleration module — Base Compute Backend + Safe Batch Dispatch
 * ================================================================
 * This translation unit provides the concrete default implementations
 * for the IVectorBackend abstract interface declared in compute_backend.h.
 *
 * Dispatch chain position
 * -----------------------
 *   BackendRegistry::getSelectedVectorBackend()
 *       └─► IVectorBackend::batchKnnSearchSafe()   ← implemented here
 *               ├─ validates each query for NaN / Inf (per-element scan)
 *               ├─ marks invalid queries with AccelerationErrorCode::InputRangeViolation
 *               └─► IVectorBackend::batchKnnSearch()  (concrete backend impl)
 *                       └─► GPU kernel or CPU SIMD path
 *
 * Key interfaces implemented
 * ---------------------------
 *   IVectorBackend::batchKnnSearchSafe()  — NaN/Inf guard; delegates valid queries to batchKnnSearch()
 *
 * Related files
 * -------------
 *   include/acceleration/compute_backend.h     — full interface declaration + PartialBatchResult
 *   include/acceleration/batch_validator.h     — shape/dtype/range validators used by concrete backends
 *   include/acceleration/error_codes.h         — AccelerationErrorCode taxonomy
 *   src/acceleration/backend_registry.cpp      — selects and caches the concrete backend
 *   src/acceleration/ARCHITECTURE.md           — error handling strategy (Section 10)
 */
namespace acceleration {

// Default implementation of batchKnnSearchSafe for IVectorBackend.
//
// Each query vector is validated for NaN/Inf values before being dispatched.
// Queries that contain non-finite values receive AccelerationErrorCode::InputRangeViolation
// in their KnnQueryResult and an empty neighbors list.  Valid queries are forwarded
// to batchKnnSearch (which guarantees deterministic tie-breaking by lower index).
//
// Backends may override this method to integrate validation more tightly (e.g.
// to avoid a host-side copy when running on a GPU backend).
PartialBatchResult IVectorBackend::batchKnnSearchSafe(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    size_t k,
    bool useL2
) {
    PartialBatchResult result;
    result.queryResults.resize(numQueries);

    // Identify valid and invalid queries
    std::vector<size_t> validIndices;
    validIndices.reserve(numQueries);

    for (size_t q = 0; q < numQueries; ++q) {
        const float* queryVec = queries + q * dim;
        bool valid = true;
        for (size_t d = 0; d < dim; ++d) {
            if (std::isnan(queryVec[d]) || std::isinf(queryVec[d])) {
                valid = false;
                break;
            }
        }
        if (valid) {
            validIndices.push_back(q);
        } else {
            KnnQueryResult& qr = result.queryResults[q];
            qr.status       = AccelerationErrorCode::InputRangeViolation;
            qr.errorMessage = "Query vector contains non-finite value (NaN or Inf)";
            ++result.failureCount;
        }
    }

    if (validIndices.empty()) {
        return result;
    }

    // Build a contiguous buffer of valid query vectors
    std::vector<float> validQueries(validIndices.size() * dim);
    for (size_t i = 0; i < validIndices.size(); ++i) {
        const float* src = queries + validIndices[i] * dim;
        std::copy(src, src + dim, validQueries.data() + i * dim);
    }

    // Dispatch valid queries through the deterministic batchKnnSearch
    auto batchResults = batchKnnSearch(
        validQueries.data(), validIndices.size(), dim,
        vectors, numVectors, k, useL2
    );

    // Map results back to the original query indices
    for (size_t i = 0; i < validIndices.size(); ++i) {
        KnnQueryResult& qr = result.queryResults[validIndices[i]];
        qr.neighbors = std::move(batchResults[i]);
        qr.status    = AccelerationErrorCode::Success;
        ++result.successCount;
    }

    return result;
}

} // namespace acceleration
} // namespace themis
