// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file themis_api_contract.h
 * @brief Frozen API contract for the ThemisDB themis (engine core) module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * The themis module exposes the engine's build metadata, edition identifiers,
 * and capability discovery.  It is the authoritative source of version and
 * edition information at runtime.
 *
 * @section contracts API Contracts
 *
 * ### BuildInfo
 * - `version()` returns a non-empty semver string.
 * - `commitHash()` returns a non-empty 40-char SHA-1 or "unknown" in
 *   development builds.
 * - `buildTimestamp()` is in ISO 8601 format.
 *
 * ### Edition
 * - `current()` returns the compiled edition; it CANNOT be changed at runtime.
 * - `isFeatureEnabled()` is deterministic for a given edition + feature key.
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                      | Meaning                                      |
 * |---------------------------|----------------------------------------------|
 * | THEMIS_EDITION_MISMATCH   | Runtime edition differs from compiled        |
 * | THEMIS_FEATURE_UNKNOWN    | Feature key not registered                   |
 *
 * @section threading Threading Guarantees
 * - All methods are const/stateless; fully thread-safe.
 *
 * @section contract_freeze Contract Freeze
 * Frozen for ThemisDB v2.x.
 */

#include <cstdint>
#include <string>

namespace themis::engine {

enum class ThemisError : int32_t {
    kEditionMismatch = 7800,
    kFeatureUnknown  = 7801,
};

enum class Edition : int32_t {
    kMinimal     = 1,
    kCommunity   = 2,
    kEnterprise  = 3,
    kHyperscaler = 4,
    kMilitary    = 5,
};

} // namespace themis::engine
