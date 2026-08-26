# Developer LLM Wiki Context for GitHub Copilot

Use this module when GitHub Copilot on GitHub is implementing, refactoring, reviewing, or debugging code in ThemisDB.

## Goal

- Make the curated Developer LLM Wiki a first-class context source for GitHub Copilot coding tasks.
- Reduce ad-hoc repository scanning when relevant wiki syntheses already exist.
- Keep Copilot anchored to source-of-truth precedence instead of treating wiki pages as independent truth.

## Required Read Order

1. [../../AI_WIKI_INTEGRATION_PLAYBOOK.md](../../AI_WIKI_INTEGRATION_PLAYBOOK.md)
2. [../../ai_context/developer_llm_wiki/INDEX.md](../../ai_context/developer_llm_wiki/INDEX.md)
3. One or more task-specific wiki pages:
   - [../../ai_context/developer_llm_wiki/MODULES_AND_APIS.md](../../ai_context/developer_llm_wiki/MODULES_AND_APIS.md)
   - [../../ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md](../../ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md)
   - [../../ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md](../../ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md)
4. [../../ai_context/developer_llm_wiki/WIKI_STATUS.json](../../ai_context/developer_llm_wiki/WIKI_STATUS.json) as freshness/sync evidence

## File Selection Rules

### Module and API work
- Start with `MODULES_AND_APIS.md`
- Cross-check with module-local `src/<module>/ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `README.md`
- Then consult API contract pages under `ai_context/api_contracts/` when the change touches public interfaces:
  - `api.md`
  - `llm.md`
  - `index.md`
  - `storage.md`
  - `transaction.md`
  - `auth.md`
- Prefer module/API wiki context before broad repository search when the task asks to implement, extend, or review an API contract.

#### Transport and external API work
- Prioritize `ai_context/api_contracts/api.md` after `MODULES_AND_APIS.md`.
- Default file order:
  1. `developer_llm_wiki/MODULES_AND_APIS.md`
  2. `ai_context/api_contracts/api.md`
  3. `ai_context/FUNCTION_CLASSIFICATION.md`
  4. transport-facing headers under `include/api/**`
  5. transport tests and `src/api/*.md`
- Must verify: request/response contract, fail-closed behavior, external error mapping, version/deprecation impact, and documented thread-safety.

#### Storage and index API contract work
- Prioritize `ai_context/api_contracts/storage.md` and `ai_context/api_contracts/index.md` after `MODULES_AND_APIS.md`.
- Default file order:
  1. `developer_llm_wiki/MODULES_AND_APIS.md`
  2. `ai_context/api_contracts/storage.md` and/or `index.md`
  3. `ai_context/FUNCTION_CLASSIFICATION.md`
  4. public storage/index headers and module docs
  5. focused tests and benchmarks
- Must verify: key/value or query contract, ordering/result guarantees, snapshot/isolation expectations, durability/performance gates, and compatibility of any option changes.

#### Auth and identity API work
- Prioritize `ai_context/api_contracts/auth.md` after `MODULES_AND_APIS.md`.
- Default file order:
  1. `developer_llm_wiki/MODULES_AND_APIS.md`
  2. `ai_context/api_contracts/auth.md`
  3. `developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md` when policy or surface rules change
  4. auth-facing headers and module docs
  5. auth tests and security-sensitive callers
- Must verify: principal/credential contract, tenant/scope isolation, revocation/expiry semantics, client-visible error policy, and no weakening of security defaults.

#### LLM and model API work
- Prioritize `ai_context/api_contracts/llm.md` after `MODULES_AND_APIS.md`.
- Default file order:
  1. `developer_llm_wiki/MODULES_AND_APIS.md`
  2. `ai_context/api_contracts/llm.md`
  3. `ai_context/FUNCTION_CLASSIFICATION.md`
  4. public LLM headers and module docs
  5. inference tests / benchmarks / timeout-sensitive callers
- Must verify: model/input/output contract, timeout and cancellation semantics, ownership/lifetime of returned data, concurrency model, and behavior across model switching.

#### Transaction and consistency API work
- Prioritize `ai_context/api_contracts/transaction.md` after `MODULES_AND_APIS.md`.
- Default file order:
  1. `developer_llm_wiki/MODULES_AND_APIS.md`
  2. `ai_context/api_contracts/transaction.md`
  3. `ai_context/FUNCTION_CLASSIFICATION.md`
  4. transaction-facing headers and module docs
  5. isolation / rollback / recovery tests
- Must verify: isolation-level semantics, commit/rollback guarantees, timeout behavior, savepoint/saga stability, and that consistency guarantees remain explicit.

## API Subtype Routing Heuristics

- If the task changes HTTP/gRPC/GraphQL request handling, version routing, middleware, or client-visible transport errors, treat it as **Transport and external API work**.
- If the task changes storage engine CRUD, snapshotting, scans, index search/insert/delete, or result ordering guarantees, treat it as **Storage and index API contract work**.
- If the task changes authentication, authorization, principals, tokens, revocation, tenant isolation, or security-sensitive caller contracts, treat it as **Auth and identity API work**.
- If the task changes embeddings, generation, model selection, inference options, or cancellation behavior, treat it as **LLM and model API work**.
- If the task changes transactions, isolation levels, commit/rollback, 2PC/SAGA, savepoints, or consistency guarantees, treat it as **Transaction and consistency API work**.
- If multiple API subtypes apply, read all relevant contract pages and use the strictest compatibility, security, and documentation requirements among them.

### Build, test, CI, and operations work
- Start with `BUILD_TEST_CI_AND_OPERATIONS.md`
- Cross-check with `.github/workflows/*.yml`, `scripts/*`, and root build docs
- For CI/build failures, inspect the failing GitHub Actions run/job first, then use the wiki to narrow the relevant workflow/build files.
- Prioritize workflow/package/dependency sections in the wiki before scanning unrelated modules.

### Governance, roadmap, release, and process work
- Start with `GOVERNANCE_AND_ROADMAP.md`
- Cross-check with `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `RELEASE_STRATEGY.md`, `BRANCHING_STRATEGY.md`, `VERSIONING.md`
- Treat root governance files as the final authority for branch names, release lanes, documentation sync, and acceptance gates.
- Use the wiki as an index into those root governance sources, not as a replacement for them.

### C and C++ implementation work
- Start with `MODULES_AND_APIS.md`
- Then consult:
  - `../../ai_context/memory_management_policy.md`
  - `../../ai_context/OOP_AND_SOC_PRINCIPLES.md`
  - `../../ai_context/FUNCTION_CLASSIFICATION.md`
- Cross-check with:
  - `../../.github/instructions/cpp-best-practices.instructions.md`
  - `../../.github/instructions/cpp-language-service-tools.instructions.md`
  - `../../.github/instructions/documentation-enforcement.instructions.md`
- For public C++ API changes, also read the relevant file in `../../ai_context/api_contracts/` before editing.
- Prefer the wiki and instruction files to establish invariants, ownership, and test expectations before touching `.cpp`/`.h`/`.hpp` files.

#### Public C++ API work
- Prioritize `ai_context/api_contracts/*.md` after `MODULES_AND_APIS.md`.
- Verify public/header-level stability, ownership, thread-safety, deprecation, and documentation expectations before editing `include/**`.
- Cross-check with `FUNCTION_CLASSIFICATION.md` for public API level and `documentation-enforcement.instructions.md` for Doxygen obligations.
- Default file order:
  1. `developer_llm_wiki/MODULES_AND_APIS.md`
  2. relevant `ai_context/api_contracts/*.md`
  3. `ai_context/FUNCTION_CLASSIFICATION.md`
  4. public headers in `include/**`
  5. affected tests and module docs
- Must verify: ownership/lifetime contract, `@thread_safety`, deprecation/versioning impact, and whether the change expands the supported public surface.

#### Internal core C++ work
- Prioritize `memory_management_policy.md` and `OOP_AND_SOC_PRINCIPLES.md` after `MODULES_AND_APIS.md`.
- Focus on invariants, ownership transfer, adapter boundaries, and whether logic belongs in `include/**`, `src/**`, or `detail::`.
- Verify that any internal helper/API drift does not accidentally widen the public surface.
- Default file order:
  1. `developer_llm_wiki/MODULES_AND_APIS.md`
  2. `ai_context/memory_management_policy.md`
  3. `ai_context/OOP_AND_SOC_PRINCIPLES.md`
  4. internal headers / `src/**`
  5. focused module tests
- Must verify: RAII ownership, `detail::` boundaries, exception/result-handling pattern, and no accidental promotion of internal helpers into public headers.

#### Concurrency and performance C++ work
- Prioritize `FUNCTION_CLASSIFICATION.md` and `memory_management_policy.md` after `MODULES_AND_APIS.md`.
- Check thread-safety class, locking expectations, timeout/cancellation behavior, and hot-path criticality before editing.
- Cross-check with `cpp-best-practices.instructions.md` for lock ordering, atomics, false-sharing, and profiling-first constraints.
- Default file order:
  1. `developer_llm_wiki/MODULES_AND_APIS.md`
  2. `ai_context/FUNCTION_CLASSIFICATION.md`
  3. `ai_context/memory_management_policy.md`
  4. C++ best-practice instruction file
  5. relevant benchmarks / concurrency tests / target sources
- Must verify: thread-safety class, lock ordering, atomic vs mutex choice, timeout/cancellation semantics, and whether the path is P0/P1/P2.

#### Plugin boundary and extensibility C++ work
- Prioritize `OOP_AND_SOC_PRINCIPLES.md` and `GOVERNANCE_AND_ROADMAP.md` after `MODULES_AND_APIS.md`.
- Verify public/private boundary rules, adapter/plugin interface contracts, and that no private implementation details leak into Community/Minimal surfaces.
- Treat plugin-related interface additions as governance-sensitive changes, not just local refactors.
- Default file order:
  1. `developer_llm_wiki/MODULES_AND_APIS.md`
  2. `ai_context/OOP_AND_SOC_PRINCIPLES.md`
  3. `developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md`
  4. plugin interface headers/docs
  5. affected governance/release docs when surface policy changes
- Must verify: adapter boundary, edition/public-private boundary, repository-vs-plugin ownership, and whether the change belongs in public SDK docs or private plugin space.

## C++ Subtype Routing Heuristics

- If the task edits `include/**` or changes documented external behavior, treat it as **Public C++ API work**.
- If the task edits `src/**`, `detail::`, or internal helper headers without intended external contract change, treat it as **Internal core C++ work**.
- If the task changes mutexes, atomics, scheduling, batching, benchmarks, latency-sensitive code, or timeout behavior, treat it as **Concurrency and performance C++ work**.
- If the task changes adapters, plugin interfaces, extensibility seams, SDK exposure, or edition boundaries, treat it as **Plugin boundary and extensibility C++ work**.
- If multiple C++ subtypes apply, read all relevant subtype inputs and use the strictest boundary/documentation requirements among them.

### General onboarding or multi-area tasks
- Start with `INDEX.md`
- Follow its links before falling back to broad repository search

## Task-Type Priority Matrix

| Task type | Read first | Then verify against |
|---|---|---|
| API / module implementation | `developer_llm_wiki/MODULES_AND_APIS.md` | `src/<module>/*.md`, `ai_context/api_contracts/*.md`, headers/tests |
| API transport / external surface | `developer_llm_wiki/MODULES_AND_APIS.md` | `ai_context/api_contracts/api.md`, `FUNCTION_CLASSIFICATION.md`, `include/api/**`, transport tests |
| API storage / index contracts | `developer_llm_wiki/MODULES_AND_APIS.md` | `ai_context/api_contracts/storage.md`, `index.md`, module docs, tests/benchmarks |
| API auth / identity | `developer_llm_wiki/MODULES_AND_APIS.md` | `ai_context/api_contracts/auth.md`, auth docs/tests, governance-sensitive policy sources |
| API LLM / model | `developer_llm_wiki/MODULES_AND_APIS.md` | `ai_context/api_contracts/llm.md`, `FUNCTION_CLASSIFICATION.md`, llm docs/tests |
| API transaction / consistency | `developer_llm_wiki/MODULES_AND_APIS.md` | `ai_context/api_contracts/transaction.md`, transaction docs/tests |
| CI / build / workflow triage | `developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md` | GitHub Actions logs, `.github/workflows/*.yml`, `scripts/*` |
| Governance / roadmap / release | `developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md` | `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `RELEASE_STRATEGY.md`, `BRANCHING_STRATEGY.md`, `VERSIONING.md` |
| C++ public API | `developer_llm_wiki/MODULES_AND_APIS.md` | `ai_context/api_contracts/*.md`, `FUNCTION_CLASSIFICATION.md`, headers/tests |
| C++ internal core | `developer_llm_wiki/MODULES_AND_APIS.md` | `memory_management_policy.md`, `OOP_AND_SOC_PRINCIPLES.md`, module sources/tests |
| C++ concurrency / performance | `developer_llm_wiki/MODULES_AND_APIS.md` | `FUNCTION_CLASSIFICATION.md`, `memory_management_policy.md`, C++ instruction files, benchmarks/tests |
| C++ plugin boundary | `developer_llm_wiki/MODULES_AND_APIS.md` | `OOP_AND_SOC_PRINCIPLES.md`, `GOVERNANCE_AND_ROADMAP.md`, plugin docs/interfaces |
| Multi-area / discovery | `developer_llm_wiki/INDEX.md` | linked task-specific wiki pages and primary sources |

## Freshness and Trust Rules

- Check `WIKI_STATUS.json` for:
  - `generated_at`
  - `source_count`
  - delta metadata (`added_count`, `removed_count`, `changed_count`)
- Treat the Developer LLM Wiki as compiled guidance, not primary truth.
- If root SOT, module docs, tests, or code disagree with the wiki, prefer the primary source and report wiki drift.
- If the needed wiki page is missing, stale, or irrelevant, state that explicitly in the task output.

## GitHub Copilot Behavior Expectations

- On GitHub coding tasks, cite the consulted wiki page(s) when they materially informed the implementation.
- Do not claim the wiki was used unless the referenced files were actually consulted.
- Use the wiki to narrow search space, identify likely files, and recover module/build/governance context faster.
- Still verify behavior against code, tests, and workflows before making changes.
