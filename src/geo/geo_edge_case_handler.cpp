// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file geo_edge_case_handler.cpp
 * @brief Implementation of GeoEdgeCaseHandler — deterministic mixed-backend
 *        and precision-mode edge-case handling for the geo module.
 *
 * @see include/geo/geo_edge_case_handler.h
 * @see src/geo/ROADMAP.md — Phase 2/3 Q4 2026 items
 */

#include "geo/geo_edge_case_handler.h"

#include <cmath>
#include <limits>

namespace themis {
namespace geo {

// ============================================================================
// § 1  Construction
// ============================================================================

GeoEdgeCaseHandler::GeoEdgeCaseHandler(
        BackendCombination combo,
        GeoPrecisionMode   precision,
        IncidentCallback   on_incident) noexcept
    : combo_(combo),
      precision_(precision),
      on_incident_(std::move(on_incident)) {
}

// ============================================================================
// § 2  Dispatch
// ============================================================================

GeoEdgeCaseResult GeoEdgeCaseHandler::dispatch(
        std::function<double()>                cpu_fn,
        std::function<std::optional<double>()> gpu_fn) noexcept {

    // ULTRA precision forces CPU path regardless of backend availability.
    const bool force_cpu = (precision_ == GeoPrecisionMode::ULTRA)
                         || hasCapabilityDrift();

    // CPU-only path.
    if (combo_ == BackendCombination::CPU_ONLY || force_cpu || !gpu_fn) {
        try {
            double result = cpu_fn();
            cpu_dispatch_count_.fetch_add(1, std::memory_order_relaxed);
            return {GeoErrorCode::OK, result};
        } catch (...) {
            return {GeoErrorCode::BACKEND_UNAVAILABLE, 0.0};
        }
    }

    // Attempt GPU path first.
    std::optional<double> gpu_result;
    try {
        gpu_result = gpu_fn();
    } catch (...) {
        gpu_result = std::nullopt;
    }

    if (!gpu_result.has_value()) {
        // GPU unavailable: silent fallback to CPU (per contract §2a).
        fallback_count_.fetch_add(1, std::memory_order_relaxed);
        emitIncident("GEO-ECH-FALLBACK", "GPU path unavailable; falling back to CPU");
        try {
            double result = cpu_fn();
            cpu_dispatch_count_.fetch_add(1, std::memory_order_relaxed);
            return {GeoErrorCode::OK, result};
        } catch (...) {
            return {GeoErrorCode::BACKEND_UNAVAILABLE, 0.0};
        }
    }

    // GPU returned a value.  For CPU_AND_GPU, cross-validate.
    gpu_dispatch_count_.fetch_add(1, std::memory_order_relaxed);

    if (combo_ == BackendCombination::CPU_AND_GPU) {
        double cpu_result = 0.0;
        try {
            cpu_result = cpu_fn();
        } catch (...) {
            // CPU failure: accept GPU result.
            return {GeoErrorCode::OK, *gpu_result};
        }
        cpu_dispatch_count_.fetch_add(1, std::memory_order_relaxed);

        const double tol = toleranceForMode(precision_);
        const double diff = std::abs(*gpu_result - cpu_result);
        const double scale = std::max(std::abs(cpu_result), 1.0);
        if (diff / scale > tol) {
            // Parity mismatch: record drift, return authoritative CPU result.
            uint64_t drift = drift_event_count_.fetch_add(1, std::memory_order_acq_rel) + 1;
            emitIncident("GEO-ECH-DRIFT",
                "Cross-backend parity mismatch detected; CPU result used");
            if (drift >= kDriftThreshold) {
                emitIncident("GEO-ECH-DRIFT-PERSISTENT",
                    "Persistent backend drift: GPU path suppressed until reset");
            }
            return {GeoErrorCode::OK, cpu_result};
        }
    }

    return {GeoErrorCode::OK, *gpu_result};
}

// ============================================================================
// § 3  Geometry Boundary Validation
// ============================================================================

GeoErrorCode GeoEdgeCaseHandler::validateCoordinateBoundary(
        double lon, double lat) const noexcept {

    // Degenerate: NaN or Inf.
    if (!std::isfinite(lon) || !std::isfinite(lat)) {
        return GeoErrorCode::GEOMETRY_INVALID;
    }
    // Out-of-bounds.
    if (lon < kWgs84LonMin || lon > kWgs84LonMax ||
        lat < kWgs84LatMin || lat > kWgs84LatMax) {
        return GeoErrorCode::COORDINATE_OUT_OF_BOUNDS;
    }
    return GeoErrorCode::OK;
}

GeoErrorCode GeoEdgeCaseHandler::validateRing(
        const std::vector<std::pair<double, double>>& ring) const noexcept {

    // Minimum 4 points (3 unique + closing point).
    if (static_cast<int>(ring.size()) < 4) {
        return GeoErrorCode::GEOMETRY_INVALID;
    }
    // Ring must be closed: first == last.
    if (ring.front().first  != ring.back().first ||
        ring.front().second != ring.back().second) {
        return GeoErrorCode::GEOMETRY_INVALID;
    }
    // All coordinates must pass WGS84 validation.
    for (const auto& [lon, lat] : ring) {
        auto ec = validateCoordinateBoundary(lon, lat);
        if (ec != GeoErrorCode::OK) {
            return ec;
        }
    }
    return GeoErrorCode::OK;
}

// ============================================================================
// § 4  Capability Drift Detection
// ============================================================================

bool GeoEdgeCaseHandler::hasCapabilityDrift() const noexcept {
    return drift_event_count_.load(std::memory_order_acquire) >= kDriftThreshold;
}

void GeoEdgeCaseHandler::resetDriftCounter() noexcept {
    drift_event_count_.store(0, std::memory_order_release);
}

// ============================================================================
// § 5  Diagnostic Counters
// ============================================================================

uint64_t GeoEdgeCaseHandler::cpuDispatchCount() const noexcept {
    return cpu_dispatch_count_.load(std::memory_order_relaxed);
}

uint64_t GeoEdgeCaseHandler::gpuDispatchCount() const noexcept {
    return gpu_dispatch_count_.load(std::memory_order_relaxed);
}

uint64_t GeoEdgeCaseHandler::fallbackCount() const noexcept {
    return fallback_count_.load(std::memory_order_relaxed);
}

uint64_t GeoEdgeCaseHandler::driftEventCount() const noexcept {
    return drift_event_count_.load([[maybe_unused]] std::memory_order_acquire);
}

// ============================================================================
// § 6  Internal Helpers
// ============================================================================

void GeoEdgeCaseHandler::emitIncident(
        std::string_view id, std::string_view desc) const noexcept {
    if (on_incident_) {
        try { on_incident_(id, desc); } catch (...) {}
    }
}

} // namespace geo
} // namespace themis
