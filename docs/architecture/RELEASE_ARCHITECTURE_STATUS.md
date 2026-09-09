# Release Architecture Status

## Version and Governance Signals

| Signal | Current reading | Evidence |
|---|---|---|
| Repository version | `v2.4.0-alpha` | `ROADMAP.md`, `VERSION` |
| Current roadmap posture | substantial implementation base, but not GA-ready | `ROADMAP.md` (`Current Status`, `Current real wave posture`, `Release-critical blockers`) |
| Branch/release target | hardening and GA gating continue on `develop` | `ROADMAP.md`, `BRANCHING_STRATEGY.md`, `RELEASE_STRATEGY.md` |
| Cross-module evidence set | module inventory, include-derived coupling, repository-wide symbol usage, runtime-critical execution paths, and release posture are now split across dedicated architecture and source-root evidence docs | `docs/architecture/MODULE_ARCHITECTURE.md`, `docs/architecture/MODULE_INTEGRATION_CONTRACTS.md`, `docs/architecture/DATA_FLOW_PATHS.md`, `src/MODULE_FUNCTION_USAGE_MAP.md`, `src/CROSS_MODULE_INTEGRATION.md` |

## Footprint Posture Across `src/`

| Posture | Count | Module paths | Interpretation |
|---|---:|---|---|
| Runtime code-bearing module paths | 69 | all top-level `src/` modules except `ai_working`, `llm_streaming`, `vector_search` | runtime code exists locally and should be release-reviewed through its owning source/test/header files |
| Docs-only module paths with production-oriented claims | 2 | `src/llm_streaming/`, `src/vector_search/` | release claims require canonical runtime mapping outside the local module directory before they should be used as evidence |
| Source-root evidence / working path | 1 | `src/ai_working/` | documentation and execution-artifact support path, not a runtime subsystem |

## Cross-Module Release Hubs

| Hub | Why it matters | Evidence |
|---|---|---|
| `server` | highest direct outgoing coupling in the include graph (35 module dependencies) and main remote ingress point | `src/server/http_server.cpp`, `src/server/query_api_handler.cpp`, `src/server/llm_api_handler.cpp` |
| `llm` | second-largest dependency fan-out and canonical home of active streaming/runtime AI wiring | `src/llm/`, `include/llm/`, `src/llm/streaming_handler.cpp` |
| `query` | central read/write orchestration point for AQL, indexing, storage, distributed, and LLM-assisted execution | `src/query/query_engine.cpp`, `src/query/aql_runner.cpp`, `include/query/query_engine.h` |
| `storage` + `utils` | deepest shared infrastructure sinks in the include graph | `src/storage/`, `include/storage/`, `src/utils/`, `include/utils/` |

## Cross-Module Evidence Reading Order

| Step | Artifact | Purpose |
|---|---|---|
| 1 | `docs/architecture/MODULE_ARCHITECTURE.md` | confirm the full 72-path inventory and footprint posture |
| 2 | `docs/architecture/MODULE_INTEGRATION_CONTRACTS.md` | inspect compile-time module coupling and SCC findings |
| 3 | `src/MODULE_FUNCTION_USAGE_MAP.md` | widen review to repository-wide symbol consumers and example call sites |
| 4 | `docs/architecture/DATA_FLOW_PATHS.md` | confirm the release-critical runtime chains and orchestration boundaries |

## Current Release-Critical Cross-Module Caveats

| Caveat | Architectural impact | Evidence |
|---|---|---|
| Transaction Wave A CI evidence still open | blocks confidence in end-to-end crash recovery and distributed commit posture | `ROADMAP.md`, `src/transaction/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`, `docs/architecture/DATA_FLOW_PATHS.md` |
| GPU hardening / representative hardware still open | AI/search acceleration claims remain below strict GA confidence | `ROADMAP.md`, `src/gpu/ROADMAP.md`, `src/acceleration/ROADMAP.md` |
| Query FTS benchmark gate still open | search pipeline is implemented but release performance evidence is incomplete | `ROADMAP.md`, `src/query/ROADMAP.md`, `src/search/ROADMAP.md` |
| `llm_streaming` and `vector_search` are docs-only module paths | source-traceability must follow canonical runtime files before release decisions | `src/llm_streaming/ROADMAP.md`, `src/vector_search/ROADMAP.md`, `docs/architecture/DATA_FLOW_PATHS.md` |

## Contract and Maturity Signals

| Signal type | Canonical location | Use in review |
|---|---|---|
| Public API / header contracts | `include/<module>/**` | confirm exported interfaces and ownership/failure behavior |
| Module-local architecture contracts | `src/<module>/ARCHITECTURE.md` | confirm intended subsystem role and integration points |
| Module-local maturity / release notes | `src/<module>/ROADMAP.md` | confirm phase status, blockers, and acceptance evidence |
| Cross-module architecture evidence | `docs/architecture/*.md`, `src/MODULE_FUNCTION_USAGE_MAP.md`, `src/CROSS_MODULE_INTEGRATION.md` | review compile-time coupling together with repository-wide symbol usage, runtime path overlays, SCCs, and docs-only caveats before release gating |
