// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file toolbox_api_contract.h
 * @brief Frozen API contract for the ThemisDB toolbox module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * The toolbox module provides utility services for content operations:
 * fingerprinting, bridging to content processing pipelines, and general
 * data transformation helpers.
 *
 * @section contracts API Contracts
 *
 * ### ContentFingerprinter
 * - `fingerprint()` is deterministic for identical byte sequences.
 * - Empty input → TOOLBOX_EMPTY_INPUT (never returns zero-hash silently).
 * - Fingerprint length is fixed at 32 bytes (SHA-256).
 *
 * ### ContentToolboxBridge
 * - `process()` forwards content to registered processors in priority order.
 * - No processor registered → TOOLBOX_NO_PROCESSOR.
 * - Processing failure in any processor → abort chain + TOOLBOX_PROCESSOR_FAILED.
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                       | Meaning                                      |
 * |----------------------------|----------------------------------------------|
 * | TOOLBOX_EMPTY_INPUT        | Input data is empty or null                  |
 * | TOOLBOX_NO_PROCESSOR       | No processor registered for content type     |
 * | TOOLBOX_PROCESSOR_FAILED   | A registered processor returned failure      |
 * | TOOLBOX_ENCODING_UNSUPPORTED | Content encoding not supported              |
 *
 * @section threading Threading Guarantees
 * - `ContentFingerprinter` is stateless and fully thread-safe.
 * - `ContentToolboxBridge` processor list is protected by a shared_mutex.
 *
 * @section contract_freeze Contract Freeze
 * Frozen for ThemisDB v2.x.
 */

#include <cstdint>
#include <array>
#include <string>
#include <vector>

namespace themis::toolbox {

/// @brief Error codes for the toolbox module.
enum class ToolboxError : int32_t {
    kEmptyInput           = 7500,
    kNoProcessor          = 7501,
    kProcessorFailed      = 7502,
    kEncodingUnsupported  = 7503,
};

/// @brief Fixed-size fingerprint (32 bytes = SHA-256).
using Fingerprint = std::array<uint8_t, 32>;

} // namespace themis::toolbox
