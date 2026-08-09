# Module Documentation Remediation Checklist (2026-08-09)

**Datum:** 2026-08-09  
**Status:** Phase 3 Documentation Gap Analysis  
**Scope:** 66 tracked modules in ROADMAP.md vs existing src/<module>/README.md files  
**Priority:** High Impact — Phase 6 Documentation Acceptance Gate

---

## Executive Summary

- **Total tracked modules (ROADMAP.md):** 66
- **Existing src/<module>/README.md files:** 67 (includes non-module directories)
- **Modules with missing or minimal Level-1 documentation:** ~4 priority cases identified
- **Estimated remediation effort:** 8-12 hours across 3 priority tiers

---

## Priority Tier 1: PRODUCTION_CANDIDATE Modules (High Impact)

All 15 PRODUCTION_CANDIDATE modules should have complete Level-1 README.md per governance rules.

| Module | README.md Status | Last Synced | Priority Action |
|--------|-----------------|-------------|-----------------|
| **analytics** | ✅ EXISTS | TBD | Verify Phase 1-6 completeness |
| **graph** | ✅ EXISTS | TBD | Verify L0 verification status reflected |
| **index** | ✅ EXISTS | 2026-08-02 | Current (INDEX_MODULE_STATUS referenced) |
| **llm** | ✅ EXISTS | TBD | Verify LLM Wiki plugin SDK integration |
| **network** | ✅ EXISTS | TBD | Verify io_uring batched send coverage |
| **observability** | ✅ EXISTS | TBD | Verify metrics/tracing/alerting sections |
| **prompt_engineering** | ✅ EXISTS | TBD | Verify injection detection & self-optimization |
| **query** | ✅ EXISTS | TBD | Verify 100+ built-in functions documented |
| **rag** | ✅ EXISTS | TBD | Verify evaluation (faithfulness/relevance) |
| **replication** | ✅ EXISTS | TBD | Verify multi-master patterns |
| **server** | ✅ EXISTS | TBD | Verify 40+ API handlers documented |
| **sharding** | ✅ EXISTS | TBD | Verify Raft/Paxos/Gossip coverage |
| **temporal** | ✅ EXISTS | TBD | Verify conflict resolution & compression |
| **training** | ✅ EXISTS | TBD | Verify LoRA fine-tuning & knowledge graph |
| **transaction** | ✅ EXISTS | TBD | Verify SAGA & branching patterns |

**Remediation Plan:** Spot-check each for Phase 6 completeness (1-2 hours)

---

## Priority Tier 2: HARDENING Modules (46 modules, sampling)

Representative sample of HARDENING modules requiring Level-1 documentation verification:

| Module | README.md Status | Last Synced | Priority Action |
|--------|-----------------|-------------|-----------------|
| **auth** | ✅ EXISTS | 2026-07-27 | Verify 12 error codes & principal contract |
| **base** | ✅ EXISTS | TBD | Verify module loader documentation |
| **cache** | ✅ EXISTS | TBD | Verify semantic/query/embedding cache sections |
| **failover** | ✅ EXISTS | 2026-07-29 | Verify Phase 2-3 state machine & DR |
| **geo** | ✅ EXISTS | TBD | Verify CUDA distance kernels & containment |
| **llama_cpp** | ✅ EXISTS | TBD | Verify streaming/batch inference coverage |
| **maintenance** | ⚠️ MINIMAL | TBD | Centralized maintenance orchestration not fully documented |
| **process** | ✅ EXISTS | 2026-08-06 | Verify Phase 1-6 completion snapshot |
| **security** | ✅ EXISTS | TBD | Verify RLS, HSM/Vault, audit logging |
| **updates** | ✅ EXISTS | TBD | Verify error codes [7400-7499] & diagnostics |
| **user_storage_encrypted** | ✅ EXISTS | TBD | Verify GocryptFS & key rotation |
| **utils** | ✅ EXISTS | TBD | Verify logging, PII detection, compression |
| **voice** | ✅ EXISTS | TBD | Verify voice assistant integration |

**Remediation Plan:** Targeted updates for HARDENING modules with Phase 1-6 delivery evidence (3-4 hours)

---

## Priority Tier 3: EXPERIMENTAL & THIN/PLACEHOLDER (5 modules)

| Module | README.md Status | Last Synced | Priority Action |
|--------|-----------------|-------------|-----------------|
| **llama_cpp** (EXPERIMENTAL) | ✅ EXISTS | TBD | Document streaming/batch inference |
| **stable_diffusion** (EXPERIMENTAL) | ✅ EXISTS | TBD | Document image generation + sanitizer |
| **ai_working** (THIN/PLACEHOLDER) | ⚠️ MISSING | — | Not applicable (meta-directory) |
| **distributed_tensor** (THIN/PLACEHOLDER) | ⚠️ MINIMAL/MISSING | — | Deferred; Phase 2+ work |
| **retrieval** (THIN/PLACEHOLDER) | ⚠️ MINIMAL/MISSING | — | Deferred; hybrid retrieval rollout |

**Remediation Plan:** Defer non-critical; experimental modules to be verified (1-2 hours post-GA)

---

## Critical Gaps Requiring Immediate Attention

### 1. **Maintenance Module** (src/maintenance/)
- **Current Status:** HARDENING with minimal documentation
- **Issue:** Centralized maintenance orchestration (cron scheduling, MVCC cleanup, storage compaction) underdocumented
- **Required:** Update README.md with DatabaseMaintenanceOrchestrator, MaintenanceRegistry, MvccCleanupHandler, StorageCompactionHandler
- **Effort:** 2 hours
- **Phase 6 Gate:** YES (HARDENING module Phase 1-6 acceptance)

### 2. **Wave-1 Private Plugin Public SDKs**
- **Current Status:** Only llm_wiki SDK documented in include/llm_wiki/
- **Issue:** ethics_ai, user_storage_encrypted, importer plugins lack public SDK documentation
- **Required:** Create Level-1 docs for public interfaces + private submodule reference architecture
- **Effort:** 4-6 hours
- **Phase 6 Gate:** YES (Wave-1 private plugin governance enforcement)

### 3. **Multi-Edition Feature Matrix Documentation**
- **Current Status:** RELEASE_STRATEGY.md has edition matrix; module-level docs lack per-edition coverage
- **Issue:** MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER, MILITARY feature sets not consistently documented per module
- **Required:** Add edition-availability matrix to key module READMEs (storage, query, llm, security)
- **Effort:** 3-4 hours
- **Phase 6 Gate:** YES (Release lane promotion requirements)

---

## Phase 6 Documentation Acceptance Criteria Mapping

All module README.md updates must satisfy Phase 6 gate requirements:

| Acceptance Criterion | Related Modules | Evidence File | Status |
|---------------------|-----------------|---------------|--------|
| Level-1 documentation current | All 66 tracked | src/<module>/README.md | PARTIAL (67 exist, ~4 gaps) |
| Phase 1-6 completion snapshots | 15 PRODUCTION_CANDIDATE + HARDENING | ROADMAP.md + README.md | PARTIAL (Phase completion notes added to ARCHITECTURE.md) |
| Error codes & diagnostics documented | auth, updates, security, others | src/<module>/README.md | PARTIAL (auth/updates shown in ARCHITECTURE.md) |
| GPU/acceleration backend coverage | geo, index, query, llm, acceleration | src/<module>/README.md | PARTIAL (needs Module-level details) |
| Edition matrix per module | All modules | src/<module>/README.md | MINIMAL (only root RELEASE_STRATEGY.md current) |
| Private plugin SDK boundaries | ethics_ai, storage, importer, llm_wiki | include/<module>/, plugins/private/ | PARTIAL (llm_wiki SDK exists; others minimal) |

---

## Recommended Remediation Sequence (Priority Order)

### Stage 1: Critical Infrastructure (1-2 sessions, ~6 hours)
1. ✅ Wave-1 private plugin submodule governance (Phase 2 → ai_context/INDEX_MODULE_STATUS_2026_08_09.md)
2. [ ] maintenance module README.md completion
3. [ ] Multi-edition feature matrix (5 key modules: storage, query, llm, security, server)
4. [ ] Private plugin SDK documentation (ethics_ai, user_storage_encrypted, importer)

### Stage 2: PRODUCTION_CANDIDATE Verification (2-3 sessions, ~4 hours)
1. [ ] Spot-check all 15 PRODUCTION_CANDIDATE README.md for Phase 6 completeness
2. [ ] Add phase completion snapshots where missing
3. [ ] Verify GPU/acceleration sections (index, geo, llm)

### Stage 3: HARDENING Module Sampling (2-3 sessions, ~4 hours)
1. [ ] Auth, Failover, Updates, Process: verify recent Phase 2-6 completions are reflected
2. [ ] Geo, Utils, Voice, User_Storage: verify Level-1 documentation currency
3. [ ] Flag remaining stale or minimal docs for Phase 7 (post-GA)

### Stage 4: Future Work (Post-GA, Q4 2026+)
1. [ ] Experimental (llama_cpp, stable_diffusion): expand documentation
2. [ ] Thin/Placeholder (distributed_tensor, retrieval): defer until roadmap activation
3. [ ] Legacy documentation archival: consolidate ai_working/ phase summaries

---

## Governance Enforcement Points (Phase 6 Gate Checks)

### Mandatory Check 1: README.md Completeness
- **Rule:** All tracked modules in ROADMAP.md must have Level-1 README.md
- **Enforcement:** CI gate on PR that adds/removes module from ROADMAP.md
- **Evidence:** ai_context/MODULE_DOCUMENTATION_REMEDIATION_CHECKLIST.md (this file)
- **Status:** 4 critical gaps identified; 62/66 complete

### Mandatory Check 2: Phase 1-6 Completion Snapshots
- **Rule:** HARDENING modules with Phase 1-6 complete must include snapshot in README.md or ROADMAP.md
- **Enforcement:** Documentation review on module hardening PR merge
- **Evidence:** src/<module>/PHASE_6_ACCEPTANCE_CHECKLIST.md
- **Status:** Process, Auth, Failover complete; others TBD on Phase completion

### Mandatory Check 3: Edition Matrix
- **Rule:** All public API modules (query, storage, server, security) must document edition availability
- **Enforcement:** RELEASE_STRATEGY.md edition matrix must be synchronized with module docs
- **Evidence:** src/<module>/README.md + RELEASE_STRATEGY.md comparison
- **Status:** Needs implementation (low priority for GA, required for v2.5.0)

### Mandatory Check 4: Private Plugin SDK Boundaries
- **Rule:** Each private plugin must have public C++ SDK interface in include/<plugin>/
- **Enforcement:** .github/pull_request_template.md includes plugin SDK documentation checklist
- **Evidence:** include/<plugin>/*_interface.h + ai_context/INDEX_MODULE_STATUS_2026_08_09.md
- **Status:** LLM Wiki complete (include/llm_wiki/llm_wiki_plugin_interface.h); others minimal

---

## Implementation Notes

### a) Documentation Format & Standards
- Use consistent Level-1 structure: Overview, Architecture, Components, Testing, Performance, Security, Roadmap
- Reference style: Inline links to ROADMAP.md, FUTURE_ENHANCEMENTS.md, src/<module>/AUDIT.md
- Evidence markers: [✅ Phase X complete], [🟡 Phase X in progress], [❌ Phase X blocked]

### b) Phase 6 Sign-Off Process
1. Module owner creates PR with README.md updates + phase completion evidence
2. Documentation reviewer verifies against this checklist
3. If all checks pass → Phase 6 sign-off documented in PR
4. If gaps remain → Defer to Phase 7 post-GA (non-blocking for GA itself)

### c) Related Governance Files
- DOCUMENTATION_GOVERNANCE.md (Level 1-4 strategy)
- BRANCHING_STRATEGY.md (edition-to-branch mapping)
- RELEASE_STRATEGY.md (edition feature matrix)
- src/<module>/ROADMAP.md (authoritative module state)

---

**Next Review:** 2026-08-10 (daily during Phase 6 hardening)  
**Integration Point:** ROADMAP.md §Phase 6 — Documentation & Acceptance (line ~100)
