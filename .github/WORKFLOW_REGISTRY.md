# ThemisDB Workflow Registry (Comprehensive Governance)

> Author: ThemisDB Contributors
> Created: 2026-09-09
> Last Updated: 2026-09-27
> Update Note: Parameterized test/benchmark quality lanes via reusable-cmake-build.yml
> Status: active
> Total Workflows: 83 (verified 2026-09-27)

## Zielbild

Dieses Repository nutzt ein bewusst strukturiertes, release-orientiertes CI/CD-System.
Alle Workflows sind dokumentiert und validiert gegen WORKFLOW_GUIDELINES.md.

## Leitprinzipien

- **Keine Schatten-CI:** Alle 83 Workflows sind in diesem Registry dokumentiert.
- **Klare Verantwortlichkeit:** Jeder Workflow hat definierte Trigger und Zweck.
- **Konsistente Governance:** Alle Workflows folgen Naming-, Concurrency- und Trigger-Policies.
- **Drift Prevention:** Registry-Update ist erforderlich bei neuen Workflows (Pre-Commit-Hook in Phase 3).

## Aktiver Workflow-Kern: Alle 83 Workflows

### 1. BUILD & MAINLINE (13 Workflows)

| Workflow | Purpose | Status |
|----------|---------|--------|
| `build-mainline.yml` | Build: Mainline (All platforms, multi-matrix) | active |
| `build-clang-fast.yml` | Build: Clang Fast (Quick clang-only validation) | active |
| `build-benchmarks.yml` | Build: Benchmarks (Performance regression suite, 64-matrix) | active |
| `build-sanitizer-nightly.yml` | Build: Sanitizer Nightly (AddressSanitizer + UBSan) | active |
| `build-wave-a-gpu.yml` | Build: Wave A GPU (CUDA/GPU feature validation) | active |
| `build-wave-b-transaction.yml` | Build: Wave B Transaction (Transactional subsystem) | active |
| `build-wave-b-llm-benchmarks.yml` | Build: Wave B LLM Benchmarks (LLM inference performance) | active |
| `build-llm-inference.yml` | Build: LLM Inferencing (TinyLlama + doku.db RAG + AdaLoRA) | active |
| `build-content-regression.yml` | Build: Content Regression [Wave D] (Content processing validation) | active |
| `build-widget.yml` | Build: WinGet E2E Validation (Windows installer end-to-end) | active |
| `build-ollama-router.yml` | Build: Copilot Ollama Router (VS Code extension build) | active |
| `wave-a-gpu-ci-execution.yml` | Wave A GPU CI Execution (GPU + CPU Fallback matrix) | active |
| `wave-a-gpu-phase3-baseline-execution.yml` | Wave A GPU Phase 3: Baseline Capture & Measurement | active |

### 2. GATES: PR VALIDATION (21 Workflows)

| Workflow | Purpose | Gate Type |
|----------|---------|-----------|
| `gate-pr-core.yml` | Gate: PR Core (Primary PR gate: tests, boundaries, policies) | fast-path |
| `gate-pr-community-failclosed.yml` | Gate: PR Core [Community Fail-Closed] (Community-specific gate) | fail-closed |
| `gate-pr-edition-license.yml` | Gate: PR Core [Edition & License] (Edition compliance) | compliance |
| `gate-pr-hash-sbom.yml` | Gate: PR Core [Hash & SBOM Integrity] (Supply chain security) | compliance |
| `gate-pr-plugin-boundary.yml` | Gate: PR Core [Plugin Boundary] (Plugin boundary enforcement) | compliance |
| `gate-pr-primary-doc-structure.yml` | Gate: PR Primary Doc Structure (README/ARCHITECTURE validation) | docs |
| `gate-pr-doc-metadata.yml` | Gate: PR Doc Metadata (Markdown metadata & governance) | docs |
| `gate-pr-doxygen-governance.yml` | Gate: PR Doxygen Governance (C++ API doc validation) | docs |
| `gate-pr-module-doxygen-xml.yml` | Gate: PR Module Doxygen XML (Module-level Doxygen generation) | docs |
| `gate-pr-version-targeting.yml` | Gate: PR Version Targeting (PR body version field validation) | version-control |
| `gate-pr-merge-readiness.yml` | Gate: PR Merge Readiness (Pre-merge final validation) | pre-merge |
| `gate-pr-rag-eval.yml` | Gate: PR RAG Evaluation Contract (RAG system evaluation) | specialized |
| `gate-pr-rag-security.yml` | Gate: PR RAG Security Guardrails (RAG security validation) | security |
| `gate-pr-rag-version.yml` | Gate: PR RAG Embedding Version Governance (Embedding version tracking) | version-control |
| `gate-pr-rag-phase7.yml` | Gate: PR RAG Phase 7 (Freshness SLA) | specialized |
| `gate-pr-rag-phase8.yml` | Gate: PR RAG Phase 8 (Observability SLO) | specialized |
| `gate-pr-rag-phase9.yml` | Gate: PR RAG Phase 9 (Research Evaluation) | specialized |
| `gate-pr-rag-phase10.yml` | Gate: PR RAG Phase 10 (Cost Optimizer) | specialized |
| `gate-wave-closure.yml` | Gate: Wave Closure Governance (Wave exit criteria validation) | governance |
| `gate-distributed-knowledge.yml` | Gate: Module Validation [distributed_knowledge] (Module-specific gate) | module-specific |
| `gate-workflow-guidelines.yml` | Gate: Workflow Guidelines Compliance (Automated workflow policy validation) | governance |

### 3. RELEASE & PUBLICATION (20 Workflows)

| Workflow | Purpose | Stage |
|----------|---------|-------|
| `release-mainline.yml` | Release: Packaging (CPack orchestration, tag-triggered) | canonical |
| `release-build-matrix.yml` | Release: Build Matrix (Multi-platform build coordination) | build |
| `release-changelog.yml` | Release: Changelog (Release notes generation) | docs |
| `release-docker-image.yml` | Release: Docker Image Consumer (Docker Hub publish) | distribution |
| `release-docker-approval.yml` | Release: Docker Approval Gate (Docker release approval) | approval |
| `release-winget.yml` | Release: WinGet Consumer (WinGet publish) | distribution |
| `release-winget-approval.yml` | Release: WinGet Approval Gate (WinGet approval) | approval |
| `release-windows-distribution.yml` | Release: Windows Distribution Consumer (Windows MSI/ZIP) | distribution |
| `release-windows-distro-approval.yml` | Release: Windows Distro Approval Gate (Windows approval) | approval |
| `release-linux-distribution.yml` | Release: Linux Distribution Consumer (Linux packages) | distribution |
| `release-linux-distro-approval.yml` | Release: Linux Distro Approval Gate (Linux approval) | approval |
| `release-mainline-approval.yml` | Release: GitHub Release Approval Gate (GitHub release approval) | approval |
| `release-nightly.yml` | Release: Nightly (Daily snapshot builds) | automation |
| `release-promote.yml` | Release: Promote (Semi-Automatic edition promotion) | automation |
| `release-rollback.yml` | Release: Rollback (Release rollback orchestration) | recovery |
| `release-wordpress-press.yml` | Release: WordPress Press (Blog/announcement publish) | communication |
| `automation-community.yml` | Automation: Community (Community branch sync automation) | automation |
| `edition-hyperscaler-ci.yml` | Edition: Hyperscaler [CI] (Hyperscaler-edition specific CI) | edition-specific |
| `ga-promotion-signoff.yml` | Governance: GA Promotion Sign-Off (GA promotion approval workflow) | governance |
| `benchmark-performance-gate.yml` | Benchmark: Performance Gate (Performance regression detection) | quality |

### 4. MAINTENANCE & GOVERNANCE (19 Workflows)

| Workflow | Purpose | Category |
|----------|---------|----------|
| `maintenance-pr-failure-diagnosis.yml` | Maintenance: PR Failure Diagnosis [Recommend-Only] (Auto-diagnosis on PR failure) | support |
| `maintenance-ci-health.yml` | Maintenance: CI Health Dashboard (CI health metrics aggregation) | observability |
| `maintenance-build-issues.yml` | Maintenance: Build Error Issues (Automatic build error tracking) | issue-tracking |
| `maintenance-issues.yml` | Maintenance: Issues [GS3 + Security Alerts] (GS3 security alert handling) | security |
| `maintenance-docs.yml` | Maintenance: Docs (Documentation health and drift detection) | docs |
| `maintenance-housekeeping.yml` | Maintenance: Housekeeping (Repository cleanup and maintenance) | maintenance |
| `maintenance-architecture-ci.yml` | Maintenance: Architecture CI (Architecture documentation validation) | docs |
| `maintenance-workflow-guardrails-observe.yml` | Maintenance: Workflow Guardrails [Observe] (Workflow policy observability) | governance |
| `maintenance-soll-ist-gap-issues.yml` | Maintenance: SOLL-IST Gap Issues (Roadmap vs. reality tracking) | governance |
| `maintenance-ai-working.yml` | Maintenance: AI Working Cleanup - LLM Wiki (AI context cleanup) | automation |
| `maintenance-docs-db-build.yml` | Maintenance: Docs-to-ThemisDB Database Build (Documentation database sync) | automation |
| `maintenance-compendium-sync.yml` | Maintenance: Compendium Sync (Compendium documentation sync) | automation |
| `wiki-pr-gate.yml` | Maintenance: Wiki PR Gate (GitHub Wiki PR validation) | docs |
| `wiki-publish-from-issue.yml` | Wiki: Publish from Issue Approval (Wiki auto-publish from issues) | automation |
| `publish-wiki.yml` | Publish: GitHub Wiki (Wiki content publishing) | publication |
| `compliance-governance-gates.yml` | Compliance: Governance Gates (Multi-layer compliance validation) | compliance |
| `compliance-supply-chain.yml` | Compliance: Supply Chain (Supply chain security validation) | compliance |
| `gate-copilot-regression.yml` | Gate: Copilot Regression (Copilot code review regression detection) | quality |
| `copilot-code-review.yml` | Copilot: Code Review Setup (Copilot code review initialization) | tooling |

### 5. SECURITY (6 Workflows)

| Workflow | Purpose | Frequency |
|----------|---------|-----------|
| `security-codeql.yml` | Security: CodeQL (CodeQL static analysis) | scheduled |
| `security-consolidated.yml` | Security: Consolidated Scans (Multi-tool security aggregation) | scheduled |
| `security-dast-zap.yml` | Security: OWASP ZAP DAST (Dynamic application security testing) | scheduled |
| `security-fortify.yml` | Security: Fortify AST Scan [Scheduled] (Fortify static analysis) | scheduled |
| `security-fuzzing.yml` | Security: Fuzzing (Fuzz testing suite) | scheduled |
| `security-pentest-quarterly.yml` | Security: Quarterly Pentest Cadence (Quarterly penetration testing) | quarterly |

### 6. REUSABLE WORKFLOWS (4 Workflows)

| Workflow | Purpose | Consumers |
|----------|---------|-----------|
| `reusable-cmake-build.yml` | Reusable CMake Build Pipeline (Build orchestration reusable) | mainline, gates, release |
| `reusable-benchmark-runner.yml` | Reusable: Benchmark Runner (Benchmark execution reusable) | build-benchmarks |
| `reusable-docs-db-builder.yml` | Reusable: Docs-to-ThemisDB Database Builder (Docs DB generation) | maintenance-docs-db-build |
| `reusable-status-flags-and-issues.yml` | Reusable: Status Flags and Issues (Status/issue automation) | maintenance, release |

## Compliance Scorecard

| Category | Total | Status |
|----------|-------|--------|
| **BUILD & MAINLINE** | 13 | ✅ All documented |
| **GATES: PR VALIDATION** | 21 | ✅ All documented |
| **RELEASE & PUBLICATION** | 20 | ✅ All documented |
| **MAINTENANCE & GOVERNANCE** | 19 | ✅ All documented |
| **SECURITY** | 6 | ✅ All documented |
| **REUSABLE WORKFLOWS** | 4 | ✅ All documented |
| **TOTAL** | **83** | ✅ **100% Registry Sync** |

## Complete Workflow Redesign and Consolidation Plan

> Status: active redesign target for the canonical CI/CD topology.
> Goal: one reusable build core, one release orchestrator, one artifact contract, and one approval policy layer.

### 1. Design principles

- Keep one canonical reusable engine for CMake builds: [reusable-cmake-build.yml](.github/workflows/reusable-cmake-build.yml)
- Keep one canonical release orchestrator: [release-mainline.yml](.github/workflows/release-mainline.yml)
- Keep downstream distribution workflows as consumers only, not as independent artifact producers
- Remove duplicate setup/configure/build logic from the top-level matrix files and standardize on a single parameterized contract
- Split quality validation into named build lines that reuse the same engine with different parameters: PR release-critical tests and benchmark execution/reporting
- Treat release approval and release packaging as separate concerns: approval = authorization; packaging = artifact assembly
- Make gate workflows remain fast, non-blocking when appropriate, and scoped by file or module

### 2. Canonical layered topology

#### Layer 0 — policy and gate

- [gate-pr-core.yml](.github/workflows/gate-pr-core.yml)
- [gate-pr-merge-readiness.yml](.github/workflows/gate-pr-merge-readiness.yml)
- [gate-workflow-guidelines.yml](.github/workflows/gate-workflow-guidelines.yml)
- [compliance-governance-gates.yml](.github/workflows/compliance-governance-gates.yml)

Purpose:
- enforce repo policy
- verify workflow compliance and metadata
- gate PR merge and release readiness without heavy package work

#### Layer 1 — build orchestration

- [build-mainline.yml](.github/workflows/build-mainline.yml)
- [reusable-cmake-build.yml](.github/workflows/reusable-cmake-build.yml)
- [build-clang-fast.yml](.github/workflows/build-clang-fast.yml)
- [build-content-regression.yml](.github/workflows/build-content-regression.yml)
- specialized build lanes such as GPU/LLM/benchmarks remain in dedicated build workflows but must call the same reusable engine

Purpose:
- define build cost boundaries
- reuse one build pipeline contract for all platform and architecture lanes
- avoid copy-paste setup scripts for each entrypoint

#### Layer 2 — release orchestration

- [release-mainline.yml](.github/workflows/release-mainline.yml)
- [release-build-matrix.yml](.github/workflows/release-build-matrix.yml)
- [release-changelog.yml](.github/workflows/release-changelog.yml)
- [release-promote.yml](.github/workflows/release-promote.yml)
- [release-rollback.yml](.github/workflows/release-rollback.yml)

Purpose:
- define one official release pipeline
- package the canonical release artifacts
- validate tag and edition semantics before publication

#### Layer 3 — downstream delivery consumers

- [release-docker-image.yml](.github/workflows/release-docker-image.yml)
- [release-winget.yml](.github/workflows/release-winget.yml)
- [release-linux-distribution.yml](.github/workflows/release-linux-distribution.yml)
- [release-windows-distribution.yml](.github/workflows/release-windows-distribution.yml)

Purpose:
- consume final package outputs from the release orchestrator
- not create a second artifact identity or build path
- remain downstream of the canonical release contract

#### Layer 4 — security and compliance

- [security-codeql.yml](.github/workflows/security-codeql.yml)
- [security-consolidated.yml](.github/workflows/security-consolidated.yml)
- [security-dast-zap.yml](.github/workflows/security-dast-zap.yml)
- [security-fortify.yml](.github/workflows/security-fortify.yml)
- [security-fuzzing.yml](.github/workflows/security-fuzzing.yml)
- [security-pentest-quarterly.yml](.github/workflows/security-pentest-quarterly.yml)
- [compliance-supply-chain.yml](.github/workflows/compliance-supply-chain.yml)

Purpose:
- keep security scanning isolated from product build speed
- avoid mixing release build logic with scheduled security scans

#### Layer 5 — maintenance and governance

- [maintenance-ci-health.yml](.github/workflows/maintenance-ci-health.yml)
- [maintenance-build-issues.yml](.github/workflows/maintenance-build-issues.yml)
- [maintenance-housekeeping.yml](.github/workflows/maintenance-housekeeping.yml)
- [maintenance-ai-working.yml](.github/workflows/maintenance-ai-working.yml)
- [maintenance-docs.yml](.github/workflows/maintenance-docs.yml)

Purpose:
- operational housekeeping
- build issue tracking and registry governance
- repository hygiene without impacting build correctness

### 3. Consolidation targets

The following patterns are explicitly identified as duplicate or drift-prone and should be normalized into the canonical model above.

#### 3.1 Build lane consolidation

Current duplication:
- [build-mainline.yml](.github/workflows/build-mainline.yml) defines repeated platform setup blocks in `step2-*` jobs
- [release-build-matrix.yml](.github/workflows/release-build-matrix.yml) repeats a second matrix definition with largely parallel setup/configure/build commands

Target:
- one build matrix contract with `platform`, `preset`, `runner`, `setup_script`, `configure_command`, `build_command`, `artifact_name`
- all platform jobs route through [reusable-cmake-build.yml](.github/workflows/reusable-cmake-build.yml)
- no second shell-native configure logic outside the reusable engine

#### 3.2 Release approval consolidation

Current duplication:
- [release-mainline-approval.yml](.github/workflows/release-mainline-approval.yml)
- [release-docker-approval.yml](.github/workflows/release-docker-approval.yml)
- [release-winget-approval.yml](.github/workflows/release-winget-approval.yml)
- [release-linux-distro-approval.yml](.github/workflows/release-linux-distro-approval.yml)
- [release-windows-distro-approval.yml](.github/workflows/release-windows-distro-approval.yml)

Target:
- canonical approval gateway with a single `approval_policy` and a shared artifact gate
- downstream distribution jobs remain consumers only
- downstream release consumers should not carry independent release-signoff logic

#### 3.3 Artifact contract normalization

Current drift:
- artifact naming is split between `build-*`, `packages-*`, and `dist/*` flows
- release packaging logic is spread between release orchestration and downstream package jobs

Target:
- canonical artifact naming:
  - `themisdb-<edition>-<version>-<platform>-<arch>` for final release bundles
  - `packages-<preset>-<run_number>` for package staging artifacts
  - `build-<preset>-<run_number>` for raw build outputs
- all downstream jobs must fetch from the canonical release artifact set only

#### 3.4 Concurrency and trigger tightening

The repo must enforce the following standard:
- PR gate workflows: `cancel-in-progress: true`
- release orchestrators: `cancel-in-progress: false`
- scheduled maintenance jobs: low-frequency only and structured by day/time bucket
- `paths` filters must be narrow and module- or file-specific; avoid broad repository-wide patterns unless explicitly justified

### 4. Consolidated workflow map (target state)

#### Canonical build and release chain

- [reusable-cmake-build.yml](.github/workflows/reusable-cmake-build.yml) — universal build primitive
- [gate-pr-core.yml](.github/workflows/gate-pr-core.yml) — parameterized PR release-critical test quality line consumer
- [benchmark-performance-gate.yml](.github/workflows/benchmark-performance-gate.yml) — parameterized benchmark quality line consumer
- [build-mainline.yml](.github/workflows/build-mainline.yml) — mainline build lane on develop
- [release-mainline.yml](.github/workflows/release-mainline.yml) — release orchestrator
- [release-build-matrix.yml](.github/workflows/release-build-matrix.yml) — matrix implementation of the release build contract
- [release-docker-image.yml](.github/workflows/release-docker-image.yml) — downstream packaging consumer
- [release-winget.yml](.github/workflows/release-winget.yml) — downstream delivery consumer
- [release-linux-distribution.yml](.github/workflows/release-linux-distribution.yml) — downstream Linux package consumer
- [release-windows-distribution.yml](.github/workflows/release-windows-distribution.yml) — downstream Windows package consumer

#### Canonical governance and support chain

- [gate-pr-core.yml](.github/workflows/gate-pr-core.yml)
- [gate-pr-merge-readiness.yml](.github/workflows/gate-pr-merge-readiness.yml)
- [gate-workflow-guidelines.yml](.github/workflows/gate-workflow-guidelines.yml)
- [maintenance-ci-health.yml](.github/workflows/maintenance-ci-health.yml)
- [maintenance-build-issues.yml](.github/workflows/maintenance-build-issues.yml)
- [compliance-governance-gates.yml](.github/workflows/compliance-governance-gates.yml)

### 5. Deprecation and retirement policy

The following patterns are considered non-canonical and should be reduced or merged over time:

- near-duplicate setup blocks across multiple top-level workflows
- parallel release approval workflows with almost identical gating logic
- release package assembly distributed across both central and downstream workflows
- mixed build/release responsibilities in one file for packaging, governance, and publishing
- any workflow that creates a second artifact lineage without a clear consumer contract

### 6. Cutover rules

1. No new top-level workflow may be introduced unless it fills a missing responsibility not already covered by the canonical layers.
2. If a workflow duplicates another workflow's build or packaging logic, it must be merged or retired before release gating is considered complete.
3. Any consolidation change must keep [WORKFLOW_GUIDELINES.md](.github/WORKFLOW_GUIDELINES.md) and [WORKFLOW_REGISTRY.md](.github/WORKFLOW_REGISTRY.md) in sync.
4. Reusable workflows must remain the default choice for build and packaging logic; direct copy-paste implementation is a review blocker.
5. Downstream delivery jobs must consume from the canonical release artifact set, not generate a distinct, competing release identity.

### 7. Final target statement

The intended end-state is a single, clear CI/CD topology:

- fast policy gates at the top,
- one reusable build engine underneath,
- one canonical release orchestrator,
- downstream delivery consumers only,
- dedicated security and maintenance layers kept separate from runtime build and release logic.

This is the reference design for future workflow changes and should remain the default baseline for all workflow refactors.

## Critical Governance Rules (from WORKFLOW_GUIDELINES.md)

1. **Trigger Policy**: All workflows must constrain `push.tags` with `branches:` guard
2. **Concurrency**: All workflows must use qualified `group` names or `cancel-in-progress: false`
3. **Naming Convention**: Workflows must follow `Build:`, `Gate:`, `Release:`, `Maintenance:`, `Security:` prefixes
4. **Action Pinning**: All GitHub Actions must use SHA pinning (not `@latest` or `@main`)
5. **Registry Sync**: New workflows require WORKFLOW_REGISTRY.md update (enforce via pre-commit hook)

## Recent Updates

- **2026-09-26**: Comprehensive inventory of all 83 workflows; added categorization and governance scorecard
- **2026-09-09**: Initial lean registry (76 workflows documented)

## Next Steps: Pre-Commit Hook (Phase 3)

A pre-commit hook will enforce Registry sync on `git commit --allow-empty .github/workflows/*.yml`:
1. Scan for new workflow files
2. Verify all workflows are documented in REGISTRY
3. Block commit if discrepancy > 0 workflows

See `.github/pre-commit-hooks/workflow-registry-sync.sh` (Phase 3 implementation).
