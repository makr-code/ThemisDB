// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file themis_api_contract.h
 * @brief Edition identifiers and error codes for the ThemisDB engine core.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * This header declares only the `ThemisError` and `Edition` enumerations in
 * `themis::engine`. It does not declare build metadata accessors or runtime
 * capability-discovery functions. See `themis/build_info.h` and
 * `themis/edition.h` for those separate interfaces.
 *
 * @section contracts API Contracts
 *
 * `ThemisError` names the edition-mismatch (7800) and unknown-feature (7801)
 * error categories. `Edition` identifies MINIMAL (1), COMMUNITY (2),
 * ENTERPRISE (3), HYPERSCALER (4), and MILITARY (5). These declarations do not
 * implement runtime validation or report errors by themselves.
 *
 * @section error_taxonomy Error Taxonomy
 * | Enumerator                    | Meaning |
 * |------------------------------|---------|
 * | `ThemisError::kEditionMismatch` | Error category for an edition mismatch |
 * | `ThemisError::kFeatureUnknown`   | Error category for an unknown feature |
 *
 * @note This header contains no functions or runtime state. Compatibility
 *       expectations are governed by the repository versioning policy.
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
