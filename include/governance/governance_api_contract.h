// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file governance_api_contract.h
 * @brief Frozen API contract for the ThemisDB governance module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * The governance module enforces data-residency rules (GDPR, CCPA, LGPD),
 * cross-border transfer controls, compliance reporting, and consent
 * management.  This contract defines observable behaviour, error codes,
 * and threading guarantees for all public interfaces.
 *
 * @section contracts API Contracts
 *
 * ### ComplianceReporter
 * - `generateReport()` returns a complete, non-empty report string on success.
 * - Reports are idempotent for the same time-window and dataset snapshot.
 * - If the requested regulation is not configured → GOV_REGULATION_UNKNOWN.
 *
 * ### CcpaRules
 * - `canProcess()` returns false (deny) on ambiguous consent; never silently
 *   allows processing.
 * - Rule evaluation is synchronous; no background threads are spawned.
 *
 * ### CrossBorderTransfer
 * - `isTransferAllowed()` checks destination jurisdiction against configured
 *   allowlist; missing destination → GOV_JURISDICTION_BLOCKED.
 * - Transfer decisions are logged immutably via the audit sink.
 *
 * ### ComplianceReporting (batch)
 * - Batch export honours maximum record limits; exceeding limit returns
 *   GOV_EXPORT_LIMIT_EXCEEDED rather than silently truncating.
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                        | Meaning                                      |
 * |-----------------------------|----------------------------------------------|
 * | GOV_REGULATION_UNKNOWN      | Requested regulation not configured          |
 * | GOV_CONSENT_MISSING         | Required consent record absent               |
 * | GOV_JURISDICTION_BLOCKED    | Destination jurisdiction not allowed         |
 * | GOV_EXPORT_LIMIT_EXCEEDED   | Export exceeds configured record limit       |
 * | GOV_AUDIT_WRITE_FAILED      | Audit log write failed                       |
 * | GOV_RULE_CONFLICT           | Conflicting compliance rules detected        |
 *
 * @section threading Threading Guarantees
 * - All public methods are thread-safe via internal shared mutex.
 * - Audit log writes are serialised to prevent interleaving.
 *
 * @section contract_freeze Contract Freeze
 * Frozen for ThemisDB v2.x.
 *
 * @see src/governance/ROADMAP.md — Phase 1 gate
 * @see benchmarks/governance/bench_governance_release_gates.cpp
 * @see tests/governance/test_governance_contract_hardening_focused.cpp
 */

#include <cstdint>
#include <string>
#include <string_view>

namespace themis::governance {

/// @brief Error codes for the governance module.
enum class GovError : int32_t {
    kRegulationUnknown   = 7200, ///< Requested regulation not configured
    kConsentMissing      = 7201, ///< Required consent record absent
    kJurisdictionBlocked = 7202, ///< Destination jurisdiction not allowed
    kExportLimitExceeded = 7203, ///< Export exceeds record limit
    kAuditWriteFailed    = 7204, ///< Audit log write failed
    kRuleConflict        = 7205, ///< Conflicting compliance rules
};

/// @brief Result of a compliance check.
struct ComplianceCheckResult {
    bool     allowed{false};
    GovError reason{GovError::kConsentMissing};
    std::string justification; ///< Human-readable explanation
};

/// @brief Supported regulations.
enum class Regulation : int32_t {
    kGDPR = 1,
    kCCPA = 2,
    kLGPD = 3,
    kHIPAA = 4,
};

} // namespace themis::governance
