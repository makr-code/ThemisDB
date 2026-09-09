# RELEASE_ARCHITECTURE_STATUS

## Scope

Release posture for architecture integration, based on source and module-roadmap evidence.

## Status Buckets

### A) Production-ready contract surfaces (GA contract signals frozen in headers)

| Area | Evidence |
|---|---|
| Query core interface | `include/themis/base/interfaces/query_interface.h:1-10` |
| Storage core interface | `include/themis/base/interfaces/storage_interface.h:1-10` |
| Index manager interface | `include/themis/base/interfaces/index_interface.h:1-10` |
| Server ingress contract | `include/server/http_server.h:1-9` |
| 2PC core contracts | `include/transaction/distributed_transaction_manager.h:1-10`, `include/sharding/two_phase_commit_coordinator.h:1-10` |

### B) Hardening wave modules (implementation substantial, wave-gating still open)

| Module/Area | Open gate | Evidence |
|---|---|---|
| GPU | Phase C reduction + representative-hardware gates pending | `src/gpu/ROADMAP.md:34-44`, `src/gpu/ROADMAP.md:149-167` |
| Transaction | CI execution / representative-hardware closure pending | `src/transaction/ROADMAP.md:18-25`, `src/transaction/ROADMAP.md:114-121` |
| Root release path | GA still blocked by Wave-A/B evidence + human sign-off | `ROADMAP.md:7`, `ROADMAP.md:50-57`, `ROADMAP.md:102-108` |

### C) Docs-only module paths

| Module path | Current state evidence | Impact |
|---|---|---|
| `src/llm_streaming/` | Docs-only files in directory; no colocated `.cpp/.h` implementation | Delivery claims must map to canonical implementation/test paths before GA decisions. |
| `src/vector_search/` | Docs-only files in directory; no colocated `.cpp/.h` implementation | Same: avoid using module-local claims as standalone implementation proof. |

Directory evidence:
- `src/llm_streaming/` -> `.gitkeep`, `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`
- `src/vector_search/` -> `.gitkeep`, `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`
- Roadmap caveat in-module: `src/llm_streaming/ROADMAP.md:18-22`, `src/vector_search/ROADMAP.md:18-22`

## Release Blockers (source-backed)

1. **Wave A/B hardening evidence not fully closed on release lane**
   - `ROADMAP.md:50-57`, `ROADMAP.md:102-108`
2. **GPU Wave A gating incomplete**
   - `src/gpu/ROADMAP.md:34-44`, `src/gpu/ROADMAP.md:149-154`
3. **Transaction CI/hardware evidence incomplete**
   - `src/transaction/ROADMAP.md:18-25`, `src/transaction/ROADMAP.md:114-117`
4. **Docs-only module-path claims need canonical source mapping**
   - `src/llm_streaming/ROADMAP.md:20-21`, `src/vector_search/ROADMAP.md:20-21`

## Decision Guidance

- Treat `ROADMAP.md` root wave gates as authoritative release gate context.
- Treat docs-only module directories as documentation artefacts unless a direct source/test/benchmark path is cited.
- Use `MODULE_ARCHITECTURE.md` and `DATA_FLOW_PATHS.md` as architecture evidence index for PR/release reviews.
