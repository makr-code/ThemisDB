# Phase Dependency Graph

**Document:** Cross-Module Phase Prerequisites and Dependencies  
**Status:** Phase 2 Automation (2026-Q3)  
**Version:** 1.0  
**Owner:** Platform Release / @makr-code  
**Last Updated:** 2026-08-10  
**Auto-Generated:** Weekly by `.github/scripts/module_phase_verifier.py`

---

## Purpose

This document tracks which modules must reach which phases before dependent modules can progress. It enables automated validation that phase prerequisites are met before allowing phase advancement.

**Principle:** *No module can advance past a phase if its prerequisite modules haven't reached their required phases.*

---

## Dependency Matrix

### Core Infrastructure Tier

**Dependencies:** Modules that many others depend on

| Module | Phase | Dependents | Prerequisite Modules | Min Phase |
|--------|-------|-----------|----------------------|-----------|
| `base` | 6 | all | (none) | N/A |
| `core` | 6 | all | `base` | 6 |
| `auth` | 6 | server, sharding, query, transaction | `base`, `core` | 4 |
| `security` | 6 | server, query, llm, rag, transaction | `base`, `core`, `auth` | 4 |
| `network` | 5 | server, sharding | `base`, `core` | 4 |

### Storage & Transaction Tier

**Dependencies:** Storage layer and consistency guarantees

| Module | Phase | Dependents | Prerequisite Modules | Min Phase |
|--------|-------|-----------|----------------------|-----------|
| `storage` | 6 | index, transaction, sharding, query | `base`, `core`, `security` | 5 |
| `transaction` | 5 | query, sharding, execution | `storage`, `auth` | 5 |
| `index` | 6 | query, sharding | `storage` | 5 |
| `failover` | 6 | sharding, server | `transaction`, `storage` | 5 |

### Query & Execution Tier

**Dependencies:** Query processing and execution engine

| Module | Phase | Dependents | Prerequisite Modules | Min Phase |
|--------|-------|-----------|----------------------|-----------|
| `query` | 5 | execution, sharding, llm | `index`, `storage`, `transaction`, `auth` | 5 |
| `execution` | 5 | sharding | `query`, `storage` | 5 |
| `aql` | 5 | query, execution | `query` | 4 |
| `search` | 5 | query, rag | `index`, `query` | 4 |

### Distribution & Scaling Tier

**Dependencies:** Distributed execution and scaling

| Module | Phase | Dependents | Prerequisite Modules | Min Phase |
|--------|-------|-----------|----------------------|-----------|
| `sharding` | 5 | server | `query`, `transaction`, `execution`, `failover`, `network` | 6 |
| `process` | 6 | execution, sharding | `core`, `transaction` | 5 |
| `metadata` | 5 | sharding, query | `storage`, `transaction` | 4 |

### AI/ML & Retrieval Tier

**Dependencies:** LLM and RAG functionality

| Module | Phase | Dependents | Prerequisite Modules | Min Phase |
|--------|-------|-----------|----------------------|-----------|
| `llm` | 5 | rag, prompt_engineering | `base`, `security`, `auth` | 4 |
| `rag` | 5 | execution | `llm`, `search`, `index` | 4 |
| `prompt_engineering` | 4 | llm, rag | `llm` | 3 |
| `evaluation` | 4 | llm, rag | `llm` | 2 |

### Data Processing Tier

**Dependencies:** Ingestion, export, CDC

| Module | Phase | Dependents | Prerequisite Modules | Min Phase |
|--------|-------|-----------|----------------------|-----------|
| `ingestion` | 5 | storage, sharding | `storage` | 4 |
| `exporters` | 5 | storage | `storage`, `transaction` | 4 |
| `cdc` | 5 | ingestion, exporters | `storage`, `transaction` | 4 |
| `content` | 5 | ingestion, search, rag | `storage`, `ingestion` | 4 |
| `scraper` | 4 | ingestion, content | `ingestion`, `content` | 3 |

### GPU & Acceleration Tier

**Dependencies:** Hardware acceleration and tensor operations

| Module | Phase | Dependents | Prerequisite Modules | Min Phase |
|--------|-------|-----------|----------------------|-----------|
| `gpu` | 4 | acceleration, tensor, search | `base`, `core` | 3 |
| `tensor` | 4 | execution, distributed_tensor | `gpu`, `base` | 3 |
| `distributed_tensor` | 3 | sharding | `tensor`, `sharding` | 4 |
| `acceleration` | 4 | query, execution | `gpu` | 3 |

### Operations & Governance Tier

**Dependencies:** Operational readiness, monitoring, compliance

| Module | Phase | Dependents | Prerequisite Modules | Min Phase |
|--------|-------|-----------|----------------------|-----------|
| `failover` | 6 | (no direct) | `transaction`, `storage` | 5 |
| `chaos` | 5 | (no direct) | `failover`, `sharding` | 5 |
| `ethics_ai` | 3 | llm, rag, prompt_engineering | `base`, `security` | 2 |
| `performance` | 5 | (observability) | all core modules | 4 |

---

## Dependency Graph (Textual DAG)

```
[base] (6)
  ├─→ [core] (6)
  │    ├─→ [auth] (6)
  │    │    ├─→ [security] (6)
  │    │    │    ├─→ [query] (5) ◄─┐
  │    │    │    ├─→ [llm] (5) ◄──┐│
  │    │    │    └─→ [rag] (5) ◄─┐││
  │    │    └─→ [server]* (5)     │││
  │    │
  │    ├─→ [network] (5) ──→ [sharding] (5)
  │    │
  │    └─→ [storage] (6)
  │         ├─→ [index] (6)
  │         │    └─→ [query] (5) ◄────────────┐
  │         │         ├─→ [execution] (5)     │
  │         │         │    └─→ [sharding] (5)─┤
  │         │         └─→ [search] (5) ◄──┐   │
  │         │              └─→ [rag] (5) ◄─┼───┘
  │         │
  │         ├─→ [transaction] (5)
  │         │    ├─→ [query] (5)
  │         │    ├─→ [failover] (6)
  │         │    │    └─→ [sharding] (5)
  │         │    └─→ [process] (6)
  │         │
  │         ├─→ [ingestion] (5)
  │         │    ├─→ [content] (5)
  │         │    └─→ [cdc] (5)
  │         │
  │         └─→ [exporters] (5)
  │
  ├─→ [gpu] (4)
  │    ├─→ [tensor] (4)
  │    │    └─→ [distributed_tensor] (3)
  │    └─→ [acceleration] (4)
  │
  └─→ [chaos] (5) ◄─ requires [failover] (6), [sharding] (5)

[ethics_ai] (3)
  └─→ [llm] (5), [rag] (5), [prompt_engineering] (4)

[prompt_engineering] (4)
  ←─ [llm] (5)
  └─→ [rag] (5)

[evaluation] (4)
  ←─ [llm] (5)
  └─→ [rag] (5)

* [server] - indirect dependency via [auth], [security], [network]
```

---

## Validation Rules

### Phase Advancement Gate

**Condition:** When a module attempts to advance Phase N → Phase N+1

**Check:**
```python
for each prerequisite_module in module.prerequisites:
    if prerequisite_module.current_phase < prerequisite_module.required_phase:
        FAIL: Cannot advance <module> Phase <N+1>
              Prerequisite <prerequisite_module> at Phase <current> 
              but requires Phase <required>
        BLOCKING_MODULE = prerequisite_module
        return BLOCKED(BLOCKING_MODULE)
else:
    PASS: All prerequisites met
    return ALLOWED
```

### Cyclic Dependency Detection

**Rule:** Dependency graph must be acyclic (DAG)

**Automated Check:** Weekly workflow runs:
```bash
python .github/scripts/phase_dependency_validator.py --detect-cycles
```

**If cycle detected:**
- Auto-create GitHub issue: `[governance] Cyclic phase dependency detected`
- List modules and cycle path
- Block all phase advancement until cycle broken
- Alert release lead

---

## Current Status (2026-08-10)

### Ready for Release (Phase 6)

```
✅ [x] base (6)
✅ [x] core (6)
✅ [x] auth (6)
✅ [x] security (6)
✅ [x] storage (6)
✅ [x] index (6)
✅ [x] failover (6)
✅ [x] process (6)
```

### In Progress (Phase 5)

```
🟡 [~] network (5)
🟡 [~] transaction (5)
🟡 [~] query (5)
🟡 [~] execution (5)
🟡 [~] sharding (5)
🟡 [~] ingestion (5)
🟡 [~] exporters (5)
🟡 [~] cdc (5)
🟡 [~] chaos (5)
🟡 [~] llm (5)
🟡 [~] rag (5)
🟡 [~] search (5)
🟡 [~] aql (5)
🟡 [~] performance (5)
```

### Pending (Phase ≤ 4)

```
🟡 [~] content (5)
🟡 [~] metadata (5)
🟡 [~] gpu (4)
🟡 [~] tensor (4)
⬜ [ ] distributed_tensor (3)
⬜ [ ] prompt_engineering (4)
⬜ [ ] evaluation (4)
⬜ [ ] acceleration (4)
⬜ [ ] ethics_ai (3)
⬜ [ ] scraper (4)
```

### Blocking Dependencies (as of 2026-08-10)

None detected; all active phases have prerequisites MET.

---

## Dependency Changes & Review

### Last Review

**Date:** 2026-08-10  
**Reviewed By:** Phase 1 Foundation Delivery  
**Changes:**
- Aligned dependency graph with Phase 1 Evidence Registry
- Added GPU/acceleration tier
- Added ethics_ai prerequisites

### Next Review

**Schedule:** Weekly (every Monday 03:00 UTC)  
**Validation:** `.github/workflows/10-governance_maturity-verification.yml` runs dependency check

---

## References

- `RELEASE_PROMOTION_GATE_POLICY.md` — Tier 0 gate: phase dependency graph must be acyclic
- `PHASE_CLOSURE_POLICY.md` — Phase advancement blocked until prerequisites met
- `.github/scripts/module_phase_verifier.py` — Generates this document weekly
- `ROADMAP.md` — Current phase state for all modules

