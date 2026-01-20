---
name: 📚 Documentation: Dual Namespace Architecture
about: Add inline documentation explaining dual namespace approach
title: "[DOCS] Document Dual Namespace Rationale in Code"
labels: priority:P3, type:documentation, area:core, effort:small
assignees: ''
---

## ✅ Low Priority Enhancement

**Current Status:** Dual namespace approach is documented but not explained in code  
**Priority:** P3 (Low)  
**Effort:** 1-2 hours  
**Target Version:** v1.4.0  
**Related Audit:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md` Section 5.1

---

## 📋 Background

ThemisDB uses a **dual namespace approach**:
- **`themis`** - Primary namespace (90%+ of codebase)
- **`themisdb`** - Extended namespace for specific subsystems

This is **intentional and architecturally correct** per `docs/de/architecture/namespace-architektur.md`.

However, developers encountering `namespace themisdb` without context may be confused about why two namespaces exist.

---

## 🎯 Objective

Add clarifying comments in key header files to explain the namespace rationale, preventing confusion and unnecessary "fix the namespace" PRs.

---

## 📝 Files to Update

Add comments to the following 17 files that use `namespace themisdb`:

### Sharding Module (RAFT-related)
- [ ] `include/sharding/raft_consensus.h`
- [ ] `include/sharding/raft_log.h`
- [ ] `include/sharding/raft_state.h`
- [ ] `include/sharding/raft_wal_integration.h`
- [ ] `include/sharding/hot_spare_manager.h`
- [ ] `include/sharding/predictive_detector.h`
- [ ] `include/sharding/quorum_manager.h`
- [ ] `include/sharding/partition_detector.h`
- [ ] `include/sharding/operational_metrics.h`
- [ ] `include/sharding/redundancy_strategy.h`
- [ ] `include/sharding/auto_recovery_manager.h`
- [ ] `include/sharding/backpressure_protocol.h`

### Replication Module
- [ ] `include/replication/replication_manager.h`
- [ ] `include/replication/multi_master_replication.h`

### Storage Module
- [ ] `include/storage/blob_redundancy_manager.h`

### Temporal Module
- [ ] `include/temporal/temporal_conflict_resolver.h`

### Server Module (investigate)
- [ ] `include/server/buffer_binary_protocol.h` - Verify if this should use `themis::server` instead

---

## ✏️ Comment Template

Add this comment block at the top of each file (after copyright):

```cpp
/**
 * NAMESPACE RATIONALE:
 * 
 * This file uses `namespace themisdb` (not `themis`) for extended/specialized
 * components as per the documented architecture in:
 * docs/de/architecture/namespace-architektur.md
 * 
 * Dual Namespace Approach:
 * - `themis` = Primary namespace for core functionality (90%+ of codebase)
 * - `themisdb` = Extended namespace for specific subsystems:
 *   * RAFT consensus components (themisdb::sharding)
 *   * Temporal conflict resolution (themisdb::temporal)
 *   * Replication orchestration (themisdb::replication)
 *   * Streaming protocols (themisdb::streaming)
 * 
 * This separation is intentional and should NOT be changed to `themis`.
 * See NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md for details.
 */
```

### Example for RAFT Files

```cpp
// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * NAMESPACE RATIONALE:
 * 
 * RAFT consensus components use `namespace themisdb::sharding` to distinguish
 * them from standard sharding components in `themis::sharding`.
 * 
 * This separation allows:
 * - Clear distinction between RAFT-specific vs general sharding code
 * - Independent evolution of RAFT subsystem
 * - Easier maintenance and testing of consensus logic
 * 
 * Architecture Reference: docs/de/architecture/namespace-architektur.md
 * Audit Reference: NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md Section 1.3
 */

#ifndef THEMISDB_SHARDING_RAFT_CONSENSUS_H
#define THEMISDB_SHARDING_RAFT_CONSENSUS_H

namespace themisdb {
namespace sharding {
    // RAFT implementation...
}}

#endif
```

---

## ✅ Acceptance Criteria

- [ ] All 17 files have namespace rationale comments
- [ ] Comments explain WHY `themisdb` is used (not just THAT it's used)
- [ ] Comments reference architecture documentation
- [ ] `buffer_binary_protocol.h` investigated (migrate to `themis::server` if no good reason)
- [ ] No functional code changes
- [ ] Documentation style consistent across all files

---

## 🧪 Testing Requirements

- [ ] Code compiles without errors
- [ ] No functional changes (git diff shows only comment additions)
- [ ] All existing tests pass

---

## 📚 References

- **Architecture Doc:** `docs/de/architecture/namespace-architektur.md`
- **Audit Report:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md` Section 1
- **Files List:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md` Appendix A

---

## 🎯 Benefit

**Prevents Confusion:**
- New contributors understand namespace design
- Reduces "why are there two namespaces?" questions
- Prevents incorrect "fix" PRs trying to unify namespaces

**Low Effort, High Clarity:**
- < 2 hours work
- Improves codebase understandability
- Self-documenting code

---

## 📋 Optional Enhancement

Update `docs/de/architecture/namespace-architektur.md` to explicitly recommend adding these comments for all `themisdb` namespace files.

---

**Created:** Based on Namespace Implementation Audit (2026-01-20)  
**Audit Section:** 5.1 Namespace-Konsistenz Recommendations  
**Related New Requirement:** "Erkläre Dual namespace approach is intentional and documented"
