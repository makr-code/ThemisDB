/**
 * @file retrieval.h
 * @brief Umbrella include for the ThemisDB retrieval module.
 *
 * Stable public entry-point for the retrieval subsystem including
 * vector search, keyword search, and hybrid-ranking combiners.
 */

// ThemisDB — EPIC 1 Hybrid Knowledge Retrieval
// Canonical umbrella header: include/retrieval/retrieval.h
//
// This header is the stable public entry-point for the retrieval module.
// Implementation headers are in src/retrieval/include/ and exposed via the
// themis_retrieval CMake target (THEMIS_BUILD_RETRIEVAL=ON).
//
// Usage:
//   #include "retrieval/retrieval.h"
//   #include "retrieval/lora_package.h"   // direct implementation header
//
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

// Forward-include implementation headers:
// #include "retrieval/lora_package.h"
