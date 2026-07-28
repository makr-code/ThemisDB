/**
 * @file evaluation.h
 * @brief Umbrella include for the ThemisDB evaluation and benchmarking module.
 *
 * Stable public entry-point for the evaluation module.  Include this header
 * to access the benchmark matrix, metric collectors, and scoring primitives.
 */

// ThemisDB — EPIC 2 Evaluation & Benchmarking
// Canonical umbrella header: include/evaluation/evaluation.h
//
// This header is the stable public entry-point for the evaluation module.
// Implementation headers are in src/evaluation/include/ and exposed via the
// themis_evaluation CMake target (THEMIS_BUILD_EVALUATION=ON).
//
// Usage:
//   #include "evaluation/evaluation.h"
//   #include "evaluation/benchmark_matrix.h"   // direct implementation header
//   #include "evaluation/hardware_profile.h"
//   #include "evaluation/query_planner.h"
//
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

// Forward-include implementation headers:
// #include "evaluation/benchmark_matrix.h"
// #include "evaluation/hardware_profile.h"
// #include "evaluation/query_planner.h"
