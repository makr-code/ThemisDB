#pragma once

/**
 * @file kernel_registry.h
 * @brief Convenience header for the central kernel registry.
 *
 * Issue: #5382 — GPU/VRAM Layer: Refactor, vereinheitlichen und
 *                Shader-Vollständigkeit sichern
 *
 * This header provides the complete KernelRegistry, KernelCoverage, and
 * ValidationReport API.  All declarations live in compute_backend.h alongside
 * the other acceleration module types; this header simply re-exports them for
 * callers that want to include only the kernel-registry surface.
 *
 * Usage:
 * @code
 *   #include "acceleration/kernel_registry.h"
 *   using namespace themis::acceleration;
 *
 *   // Register a dispatch table (typically called once per backend at startup):
 *   KernelRegistry reg;
 *   reg.registerANNDispatch(BackendType::CUDA, cudaANNDispatch);
 *   reg.registerGeoDispatch(BackendType::CUDA, cudaGeoDispatch);
 *
 *   // Validate completeness (CI assertion):
 *   ValidationReport r = reg.validate();
 *   assert(r.allComplete());
 *
 *   // Resolve with CPU fallback:
 *   ANNKernelDispatch d = reg.lookupANNWithFallback(BackendType::CUDA);
 *
 *   // Per BackendRegistry singleton (after initializeRuntime()):
 *   ValidationReport report = BackendRegistry::instance().validateKernels();
 * @endcode
 */

#include "acceleration/compute_backend.h"
