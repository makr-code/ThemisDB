# Developer LLM Wiki — Build/Test/CI/Operations

> Author: ThemisDB Team
> Created: 2026-10-26
> Last Updated: 2026-10-30
> Status: active
> AI_Used: true
> AI_Models: unknown
> AI_Contributor: GitHub Copilot
> Human_Review: false
> Human_Author: ThemisDB Team
>
> Exact model backend not verifiable from final document text alone.

Datum: 2026-10-26
Status: Active

## CI repair context (2026-10-30)

The generated source inventory below and `WIKI_STATUS.json` describe the
2026-10-26 snapshot (7,989 sources), not a fresh validation of current workflows.
Its workflow entries contain hashes and unreadable titles rather than operational
guidance; use workflow sources and concrete run logs as evidence.

- Summary jobs must check out the repository before invoking the local
  `write-run-summary` action. Checkouts in other jobs do not share a workspace.
  See [Workflow Guidelines](../../.github/WORKFLOW_GUIDELINES.md) and
  [boundary guard](../../tools/ci/check_workflow_boundaries.py).
- Wave-4 explicitly configures the `linux-release` preset with benchmarks enabled,
  builds and executes `bench_w4a_release_gates` in the same job so its shared
  runtime libraries remain available. Only
  `benchmark-artifacts/w4a_release_gates.json` is transferred for baseline comparison. Source:
  [benchmark-gate.yml](../../.github/workflows/benchmark-gate.yml).
- Coverage requires real RocksDB; thematic benchmarks require Boost.Filesystem.
  MSVC requires nlohmann-json headers and importer generation before analysis.
  See the [repair scope](../../.github/WORKFLOW_REGISTRY.md#ci-remediation-2026-10-30).
- Invalid nested-workflow permissions and expired execution approvals are different
  failure classes. Only the former are fixed by workflow code; approvals require
  maintainer intervention. MSVC analyzer C1060 remains a separate resource risk.
- Core preflight accepts local `uses` references and inline SHA comments; new
  pinning/self-push/non-blocking-step violations are checked on added diff lines.
  Housekeeping and promotion cancel superseded PR runs without cancelling manual work.
- Docker's consumer, platform builds and nested model cache use distinct concurrency
  namespaces; platform groups include the target architecture. The inherited
  `github.workflow` value alone does not distinguish caller and callee.
  [Run 41026987740](https://github.com/makr-code/ThemisDB/actions/runs/41026987740)
  still used the old `develop` revision and never reached an actual image build:
  summary checkout failures and cancelled model preparation occurred first.
  The branch verification passed both model-cache preparations and their summaries,
  then exposed Docker export rejecting attestation manifest lists with `--load`.
  Registry publications retain provenance/SBOM; local exports omit those unsupported
  attestations. Platform builds select native AMD64/ARM64 runners.
- Follow-up PR evidence exposed separate cache/permission defects: RAG phases 7–10
  need PR-write permission in both caller and callee for labels/comments, even when
  compilation passed. The reusable CMake build no longer registers an unprepared
  NuGet binary source. Its existing sccache recovery disables the forced launcher
  via `THEMIS_ENABLE_COMPILER_CACHE=OFF`, preserves the cached toolchain and fails
  on reconfigure errors. The release-critical job budget is 120 minutes; this is
  not proof that its full build passes. Nightly/packaging Ninja discovery and MSVC
  analyzer memory failures still need successful full-build evidence.
  Custom configure callers declare `build_directory` explicitly, including release
  matrix lanes; cache-disabled recovery uses that cache's original source directory.
- Evidence window: 2026-10-29 05:23 to 2026-10-30 05:23 UTC.
  Representative runs:
  [missing summary checkout](https://github.com/makr-code/ThemisDB/actions/runs/41003835208),
  [nested permissions](https://github.com/makr-code/ThemisDB/actions/runs/40954398766),
  [coverage prerequisite](https://github.com/makr-code/ThemisDB/actions/runs/40990478535),
  [expired approval](https://github.com/makr-code/ThemisDB/actions/runs/40960673012).
  Local validation is not evidence that these historical runs have turned green.

## .github/workflows/benchmark-gate.yml
- Kategorie: build_test_ci
- Hash: `2ed12a75ae4d88dd`
- Titel: (binary or unreadable)

## .github/workflows/benchmark-performance-gate.yml
- Kategorie: build_test_ci
- Hash: `1a928691320f72d4`
- Titel: (binary or unreadable)

## .github/workflows/benchmark-thematic-suites.yml
- Kategorie: build_test_ci
- Hash: `182626faa1ddc164`
- Titel: (binary or unreadable)

## .github/workflows/build-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `71a3ed0d828b1264`
- Titel: (binary or unreadable)

## .github/workflows/build-clang-fast.yml
- Kategorie: build_test_ci
- Hash: `4aecf3a233329958`
- Titel: (binary or unreadable)

## .github/workflows/build-content-regression.yml
- Kategorie: build_test_ci
- Hash: `c51a96ee1048ccec`
- Titel: (binary or unreadable)

## .github/workflows/build-llm-inference.yml
- Kategorie: build_test_ci
- Hash: `c7547a6187686262`
- Titel: (binary or unreadable)

## .github/workflows/build-mainline.yml
- Kategorie: build_test_ci
- Hash: `52172f1c21512439`
- Titel: (binary or unreadable)

## .github/workflows/build-ollama-router.yml
- Kategorie: build_test_ci
- Hash: `f800d02427d19323`
- Titel: (binary or unreadable)

## .github/workflows/build-sanitizer-nightly.yml
- Kategorie: build_test_ci
- Hash: `c33bd2146a991020`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-a-gpu.yml
- Kategorie: build_test_ci
- Hash: `3af9c5ba95550907`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-llm-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `a195c2dd33667e40`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-transaction.yml
- Kategorie: build_test_ci
- Hash: `f9f1e151e548cb58`
- Titel: (binary or unreadable)

## .github/workflows/build-widget.yml
- Kategorie: build_test_ci
- Hash: `b1ced56788a1901a`
- Titel: (binary or unreadable)

## .github/workflows/codeql.yml
- Kategorie: build_test_ci
- Hash: `695a9366679051b9`
- Titel: (binary or unreadable)

## .github/workflows/compliance-governance-gates.yml
- Kategorie: build_test_ci
- Hash: `5fbaf5d0c69ad1b8`
- Titel: (binary or unreadable)

## .github/workflows/compliance-supply-chain.yml
- Kategorie: build_test_ci
- Hash: `2bdeddf89793c973`
- Titel: (binary or unreadable)

## .github/workflows/copilot-code-review.yml
- Kategorie: build_test_ci
- Hash: `0d1a0649b0997348`
- Titel: (binary or unreadable)

## .github/workflows/coverage-report-nightly.yml
- Kategorie: build_test_ci
- Hash: `f58981429be9c05d`
- Titel: (binary or unreadable)

## .github/workflows/crda.yml
- Kategorie: build_test_ci
- Hash: `97e0d0c08b28c0d6`
- Titel: (binary or unreadable)

## .github/workflows/edition-hyperscaler-ci.yml
- Kategorie: build_test_ci
- Hash: `c84cf8571fc357bb`
- Titel: (binary or unreadable)

## .github/workflows/ga-promotion-signoff.yml
- Kategorie: build_test_ci
- Hash: `5578bfa9a2422da4`
- Titel: (binary or unreadable)

## .github/workflows/gate-copilot-regression.yml
- Kategorie: build_test_ci
- Hash: `7aa9ccd3a7b9db42`
- Titel: (binary or unreadable)

## .github/workflows/gate-distributed-knowledge.yml
- Kategorie: build_test_ci
- Hash: `11f5867f80fa1340`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-benchmark-ctest.yml
- Kategorie: build_test_ci
- Hash: `2bea96d431b64131`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-community-failclosed.yml
- Kategorie: build_test_ci
- Hash: `09dc8cf8ab669cbd`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-core.yml
- Kategorie: build_test_ci
- Hash: `d90e793a97d618ff`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doc-metadata.yml
- Kategorie: build_test_ci
- Hash: `ee49b6c923f6ebb9`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doxygen-governance.yml
- Kategorie: build_test_ci
- Hash: `ca146c1cdd81da59`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-edition-license.yml
- Kategorie: build_test_ci
- Hash: `897b92df7b05a427`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-final-runtime-api.yml
- Kategorie: build_test_ci
- Hash: `955179ae7e93eeff`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-hash-sbom.yml
- Kategorie: build_test_ci
- Hash: `5958152d528c8613`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-merge-readiness.yml
- Kategorie: build_test_ci
- Hash: `3cc0761932e8c077`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-module-doxygen-xml.yml
- Kategorie: build_test_ci
- Hash: `aa1eb79b9e6324dd`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-plugin-boundary.yml
- Kategorie: build_test_ci
- Hash: `1f900fdaeebb02f8`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-primary-doc-structure.yml
- Kategorie: build_test_ci
- Hash: `fb2c7a7f3d0493f1`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-eval.yml
- Kategorie: build_test_ci
- Hash: `168f2d1cfe884ae7`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase10.yml
- Kategorie: build_test_ci
- Hash: `27140536dcf45e72`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase7.yml
- Kategorie: build_test_ci
- Hash: `950304e9b459c29e`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase8.yml
- Kategorie: build_test_ci
- Hash: `d6924ed3455fcce3`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase9.yml
- Kategorie: build_test_ci
- Hash: `049fe9568de0c3a5`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-security.yml
- Kategorie: build_test_ci
- Hash: `5f2ed0a100b984e1`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-version.yml
- Kategorie: build_test_ci
- Hash: `2efec1f10f872e5c`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-version-targeting.yml
- Kategorie: build_test_ci
- Hash: `cc4abbc2ba324d80`
- Titel: (binary or unreadable)

## .github/workflows/gate-release-benchmark-validation.yml
- Kategorie: build_test_ci
- Hash: `20c69f30312f2c38`
- Titel: (binary or unreadable)

## .github/workflows/gate-wave-closure.yml
- Kategorie: build_test_ci
- Hash: `e6e34aadd596225f`
- Titel: (binary or unreadable)

## .github/workflows/gate-workflow-guidelines.yml
- Kategorie: build_test_ci
- Hash: `fda45221825af777`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-ai-working.yml
- Kategorie: build_test_ci
- Hash: `26be4b7773d0202a`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-architecture-ci.yml
- Kategorie: build_test_ci
- Hash: `3bce3d16ac55c655`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-build-issues.yml
- Kategorie: build_test_ci
- Hash: `8d6cd2e8dbfd28ce`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-ci-health.yml
- Kategorie: build_test_ci
- Hash: `eeaaa0fef5ea8fb6`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-compendium-sync.yml
- Kategorie: build_test_ci
- Hash: `7ce9c504f18ef119`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-docs-db-build.yml
- Kategorie: build_test_ci
- Hash: `e7d6836cd74794b5`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-docs.yml
- Kategorie: build_test_ci
- Hash: `cae537e04e5bf088`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-housekeeping.yml
- Kategorie: build_test_ci
- Hash: `de3d14c60b5ce30a`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-issues.yml
- Kategorie: build_test_ci
- Hash: `3eac9240951febcf`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-pr-failure-diagnosis.yml
- Kategorie: build_test_ci
- Hash: `6c395f7c16a2d63f`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-soll-ist-gap-issues.yml
- Kategorie: build_test_ci
- Hash: `23201f8ab2bf1e13`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-workflow-guardrails-observe.yml
- Kategorie: build_test_ci
- Hash: `b624de438556a4b0`
- Titel: (binary or unreadable)

## .github/workflows/msvc.yml
- Kategorie: build_test_ci
- Hash: `b982946bc0d19bf1`
- Titel: (binary or unreadable)

## .github/workflows/nightly-governance-audit.yml
- Kategorie: build_test_ci
- Hash: `ba578246c0558de1`
- Titel: (binary or unreadable)

## .github/workflows/publish-wiki.yml
- Kategorie: build_test_ci
- Hash: `b742aa6ffe6ac9ce`
- Titel: (binary or unreadable)

## .github/workflows/release-build-matrix.yml
- Kategorie: build_test_ci
- Hash: `f39f60b713d0f444`
- Titel: (binary or unreadable)

## .github/workflows/release-changelog.yml
- Kategorie: build_test_ci
- Hash: `d97f477879f14fd3`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-approval.yml
- Kategorie: build_test_ci
- Hash: `bc2aa312fe262501`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-image.yml
- Kategorie: build_test_ci
- Hash: `a62d8846c0cb6e65`
- Titel: (binary or unreadable)

## .github/workflows/release-evidence-chain.yml
- Kategorie: build_test_ci
- Hash: `7f277b8a7015e3df`
- Titel: (binary or unreadable)

## .github/workflows/release-linux-distribution.yml
- Kategorie: build_test_ci
- Hash: `f672eaf5ce2b3257`
- Titel: (binary or unreadable)

## .github/workflows/release-linux-distro-approval.yml
- Kategorie: build_test_ci
- Hash: `7cb1f9c0e8f91147`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline-approval.yml
- Kategorie: build_test_ci
- Hash: `635081477b6cee78`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline.yml
- Kategorie: build_test_ci
- Hash: `2bb6f74eeac8c896`
- Titel: (binary or unreadable)

## .github/workflows/release-nightly.yml
- Kategorie: build_test_ci
- Hash: `6a0ddb740eb262a5`
- Titel: (binary or unreadable)

## .github/workflows/release-promote.yml
- Kategorie: build_test_ci
- Hash: `b7f541ba60b5c75f`
- Titel: (binary or unreadable)

## .github/workflows/release-rollback.yml
- Kategorie: build_test_ci
- Hash: `521def4a55065f59`
- Titel: (binary or unreadable)

## .github/workflows/release-windows-distribution.yml
- Kategorie: build_test_ci
- Hash: `4ed1a67590a8166e`
- Titel: (binary or unreadable)

## .github/workflows/release-windows-distro-approval.yml
- Kategorie: build_test_ci
- Hash: `0e299ecfbf225e28`
- Titel: (binary or unreadable)

## .github/workflows/release-winget-approval.yml
- Kategorie: build_test_ci
- Hash: `54c1b3a806901136`
- Titel: (binary or unreadable)

## .github/workflows/release-winget.yml
- Kategorie: build_test_ci
- Hash: `7e12c4fac4d6db6a`
- Titel: (binary or unreadable)

## .github/workflows/release-wordpress-press.yml
- Kategorie: build_test_ci
- Hash: `641b3d6490580dca`
- Titel: (binary or unreadable)

## .github/workflows/reusable-benchmark-build-run-report.yml
- Kategorie: build_test_ci
- Hash: `203b7384daa8ef87`
- Titel: (binary or unreadable)
