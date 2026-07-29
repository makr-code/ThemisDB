// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file updates_api_contract.h
 * @brief Frozen API contract for the ThemisDB updates module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * The updates module manages software lifecycle operations: blue/green
 * deployments, canary rollouts, build verification, and live patch
 * application.  All update operations MUST be reversible (rollback) unless
 * the operation is explicitly marked as irreversible.
 *
 * @section contracts API Contracts
 *
 * ### BlueGreenDeployment
 * - `switchTraffic()` completes atomically from the caller's perspective;
 *   traffic is never split between old and new versions mid-switch.
 * - `rollback()` reverts to the prior healthy slot; calling rollback with no
 *   prior switch → UPD_NO_ROLLBACK_TARGET.
 *
 * ### CanaryRollout
 * - `advancePercentage()` increments traffic in configured steps; never
 *   exceeds 100 % in a single step unless explicitly forced.
 * - Canary abort reverts traffic to 0 % without requiring explicit rollback.
 *
 * ### BuildVerifier
 * - `verify()` returns a signed verification token on success.
 * - Signature mismatch → UPD_CHECKSUM_MISMATCH (NEVER silent pass-through).
 * - Missing build artifact → UPD_ARTIFACT_MISSING.
 *
 * ### LivePatcher
 * - Patches are applied in-place without restarting the server process.
 * - Incompatible patch for running version → UPD_PATCH_INCOMPATIBLE.
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                      | Meaning                                      |
 * |---------------------------|----------------------------------------------|
 * | UPD_NO_ROLLBACK_TARGET    | Rollback requested but no prior slot exists  |
 * | UPD_CHECKSUM_MISMATCH     | Build artifact checksum validation failed    |
 * | UPD_ARTIFACT_MISSING      | Required build artifact not found            |
 * | UPD_PATCH_INCOMPATIBLE    | Patch not compatible with running version    |
 * | UPD_SWITCH_IN_PROGRESS    | Traffic switch already in progress           |
 * | UPD_CANARY_ABORTED        | Canary rollout aborted due to error threshold|
 *
 * @section threading Threading Guarantees
 * - `BlueGreenDeployment` state machine is protected by an internal mutex.
 * - `CanaryRollout` percentage updates are atomic.
 * - `BuildVerifier` is stateless and fully thread-safe.
 *
 * @section contract_freeze Contract Freeze
 * Frozen for ThemisDB v2.x.
 *
 * @see src/updates/ROADMAP.md — Phase 1 gate
 * @see benchmarks/updates/bench_updates_release_gates.cpp
 * @see tests/updates/test_updates_contract_hardening_focused.cpp
 */

#include <cstdint>
#include <string>

namespace themis::updates {

/// @brief Error codes for the updates module.
enum class UpdatesError : int32_t {
    kNoRollbackTarget  = 7400, ///< No prior deployment slot to roll back to
    kChecksumMismatch  = 7401, ///< Artifact checksum validation failed
    kArtifactMissing   = 7402, ///< Required build artifact not found
    kPatchIncompatible = 7403, ///< Patch incompatible with running version
    kSwitchInProgress  = 7404, ///< Traffic switch already in progress
    kCanaryAborted     = 7405, ///< Canary rollout aborted
};

/// @brief Deployment slot identifier.
enum class DeploymentSlot : int32_t { kBlue = 0, kGreen = 1 };

/// @brief Result of a build verification.
struct VerificationResult {
    bool        valid{false};
    std::string signatureToken; ///< Non-empty if valid == true
    UpdatesError error{UpdatesError::kChecksumMismatch};
};

} // namespace themis::updates
