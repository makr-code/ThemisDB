// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file projects_api_contract.h
 * @brief Frozen API contract for the ThemisDB projects module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * The projects module manages collaborative database projects: user
 * membership, audit logging, resource quotas, and project lifecycle.
 *
 * @section contracts API Contracts
 *
 * ### CollaborationManager
 * - `addMember()` is idempotent; adding an existing member is a no-op with OK.
 * - `removeMember()` on a non-member → PROJ_MEMBER_NOT_FOUND.
 * - Audit log is written before the membership change takes effect.
 *
 * ### InMemoryProjectAuditLog
 * - `append()` is thread-safe and non-blocking.
 * - `entries()` returns a stable snapshot; subsequent appends do not modify
 *   the snapshot.
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                     | Meaning                                       |
 * |--------------------------|-----------------------------------------------|
 * | PROJ_MEMBER_NOT_FOUND    | Requested member does not exist               |
 * | PROJ_PROJECT_NOT_FOUND   | Project ID not found                          |
 * | PROJ_QUOTA_EXCEEDED      | Project resource quota exceeded               |
 * | PROJ_AUDIT_OVERFLOW      | In-memory audit log at capacity               |
 *
 * @section threading Threading Guarantees
 * - `CollaborationManager` is thread-safe via internal shared_mutex.
 * - `InMemoryProjectAuditLog` append/read are thread-safe.
 *
 * @section contract_freeze Contract Freeze
 * Frozen for ThemisDB v2.x.
 */

#include <cstdint>
#include <string>

namespace themis::projects {

enum class ProjError : int32_t {
    kMemberNotFound  = 7700,
    kProjectNotFound = 7701,
    kQuotaExceeded   = 7702,
    kAuditOverflow   = 7703,
};

} // namespace themis::projects
