---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Developer LLM Wiki — Build/Test/CI/Operations

Datum: 2026-10-31
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
  See the [repair scope](../../.github/WORKFLOW_REGISTRY.md#ci-remediation-2026-10-31).
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
- COMMUNITY Docker builds keep fmt and spdlog together in Ubuntu's
  `libfmt-dev`/`libspdlog-dev` packages. Do not add a standalone vcpkg fmt
  dependency to `docker/vcpkg-community.json`: CMake prefers that fmt over the
  system package, but Ubuntu's compiled spdlog uses the system fmt ABI.
  [ARM64 job 123993876135](https://github.com/makr-code/ThemisDB/actions/runs/41034175988/job/123993876135)
  failed linking `themis_server` with vcpkg fmt v11 and Ubuntu spdlog 1.12.
  The manifest now leaves both libraries to apt; the provider regression check
  is in `tests/test_docker_runtime_package_regressions.py` and runs before Buildx
  setup in `reusable-docker-image-platform.yml` for both architectures.
  Retry job 124024436376 still used the old manifest with explicit vcpkg fmt;
  its link command mixed Ubuntu spdlog 1.12 with vcpkg fmt 11.0.2.
  A successful full ARM64 image build with the corrected manifest still requires
  CI verification.
  AMD64 job [124204379008](https://github.com/makr-code/ThemisDB/actions/runs/41034175988/job/124204379008)
  also built the old `910fe92` revision: it installed vcpkg fmt 11.0.2 alongside
  Ubuntu spdlog 1.12 and failed linking `themis_server` with unresolved
  `spdlog::details::log_msg` symbols using `fmt::v11`. The current COMMUNITY
  manifest already excludes fmt. The Docker dependency stage now rejects
  vcpkg fmt/spdlog headers for COMMUNITY, including transitive installations,
  before compiling the server. Shell regression tests cover both architectures.
  Re-running the old commit cannot validate this fix; a new run on the corrected
  revision is required for full-image verification.
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
- Linux AMD64 release job
  [124033550929](https://github.com/makr-code/ThemisDB/actions/runs/40996218967/job/124033550929)
  failed inside vcpkg compiler detection before the outer Ninja diagnostic.
  The run did not retain the nested error logs, so its underlying compiler failure
  is unconfirmed. The reusable CMake workflow now prints fresh compiler-detection
  output/error logs before checking for recoverable sccache errors. Cache-disabled
  configure retries clear inherited launcher environment variables as well as the
  CMake cache entries, so nested vcpkg probes do not reuse a failing launcher.
  Other compiler errors remain fatal; a full release CI run is still required.
- Linux AMD64 release job
  [124425510353](https://github.com/makr-code/ThemisDB/actions/runs/41163259618/job/124425510353)
  recovered from a compiler-probe sccache failure, then failed building Arrow 25.0.1
  when Arrow rediscovered sccache and contacted the same unavailable GitHub cache
  backend. Clearing CMake launchers alone does not prevent dependency-side discovery.
  The existing configure/build recovery now exports `SCCACHE_GHA_ENABLED=false`
  and persists it through `GITHUB_ENV`; rediscovered sccache can use the configured
  local cache rather than the failed remote backend. Source:
  [reusable-cmake-build.yml](../../.github/workflows/reusable-cmake-build.yml).
  Shell regression coverage in
  [test_check_workflow_boundaries.py](../../tools/tests/test_check_workflow_boundaries.py)
  verifies backend isolation, persistence, and fatal retry errors. A full release
  run on the corrected revision is still required.
- Retry job
  [124572236426](https://github.com/makr-code/ThemisDB/actions/runs/41163259618/job/124572236426)
  ran the old recovery logic: it cleared compiler launchers after a compiler-probe
  sccache failure but did not disable the GitHub cache backend. Arrow 25.0.1 then
  rediscovered sccache and failed with `cache storage failed to read` because the
  remote service was unavailable. Ninja successfully started Arrow's build; the
  outer missing-Ninja message followed the failed vcpkg install and was not the
  root cause. The current workflow already disables that backend during recovery,
  and the release matrix selects local caching for both Linux architectures.
  The command-level regression test now covers AMD64 and ARM64, through both
  architecture-only and OS-filtered matrix selection. This verifies workflow
  behavior, not a successful full release build; validation requires a new run
  on the corrected revision rather than another retry of the old commit.
- Evidence window: 2026-10-29 05:23 to 2026-10-30 05:23 UTC.
  Representative runs:
  [missing summary checkout](https://github.com/makr-code/ThemisDB/actions/runs/41003835208),
  [nested permissions](https://github.com/makr-code/ThemisDB/actions/runs/40954398766),
  [coverage prerequisite](https://github.com/makr-code/ThemisDB/actions/runs/40990478535),
  [expired approval](https://github.com/makr-code/ThemisDB/actions/runs/40960673012).
  Local validation is not evidence that these historical runs have turned green.

## CI repair context (2026-10-31)

- Run [41166806169](https://github.com/makr-code/ThemisDB/actions/runs/41166806169)
  failed in Linux AMD64 and ARM64 release lanes when the GitHub Actions artifact
  cache returned an outage response to sccache while vcpkg compiled Arrow.
- Linux release lanes now disable the remote sccache GHA backend while retaining
  sccache's local cache directory persisted by `actions/cache`. The release matrix
  passes the per-lane boolean directly; a fallback must not turn explicit `false`
  into `true`.
- ARM64 job [124497145202](https://github.com/makr-code/ThemisDB/actions/runs/41163259618/job/124497145202)
  failed for the same reason: sccache's `ghac` backend returned HTTP 400 during
  Arrow compilation. Ninja was executing those compilation commands; the later
  outer CMake Ninja diagnostic was secondary. This run used the older
  `0f6f0c9def30b95adff8475a143432876e6ae56d` revision, not the current backend fix.
  Executable regression coverage in `tools/tests/test_check_workflow_boundaries.py`
  checks both ARM64 selectors, apt-provided sccache, and local-backend environment
  inheritance through the generated configure/build commands. Full native ARM64
  release verification still requires a new CI run on the corrected revision.

## .github/workflows/benchmark-gate.yml
- Kategorie: build_test_ci
- Hash: `aca5342a1a4719e8`
- Titel: (binary or unreadable)

## .github/workflows/benchmark-performance-gate.yml
- Kategorie: build_test_ci
- Hash: `0dda1e5dc15d07b0`
- Titel: (binary or unreadable)

## .github/workflows/benchmark-thematic-suites.yml
- Kategorie: build_test_ci
- Hash: `905adfbc3b58957e`
- Titel: (binary or unreadable)

## .github/workflows/build-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `71a3ed0d828b1264`
- Titel: (binary or unreadable)

## .github/workflows/build-clang-fast.yml
- Kategorie: build_test_ci
- Hash: `e3dc5830b7156750`
- Titel: (binary or unreadable)

## .github/workflows/build-content-regression.yml
- Kategorie: build_test_ci
- Hash: `7d5b8aeba93e383d`
- Titel: (binary or unreadable)

## .github/workflows/build-llm-inference.yml
- Kategorie: build_test_ci
- Hash: `cdbebf0d599d8288`
- Titel: (binary or unreadable)

## .github/workflows/build-mainline.yml
- Kategorie: build_test_ci
- Hash: `19f575aa854103d9`
- Titel: (binary or unreadable)

## .github/workflows/build-ollama-router.yml
- Kategorie: build_test_ci
- Hash: `0f6f7251a556c299`
- Titel: (binary or unreadable)

## .github/workflows/build-sanitizer-nightly.yml
- Kategorie: build_test_ci
- Hash: `18496274c11a21a3`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-a-gpu.yml
- Kategorie: build_test_ci
- Hash: `687a7b26386e4b3b`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-llm-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `06d4bf8f34a54be2`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-transaction.yml
- Kategorie: build_test_ci
- Hash: `8de8b1d5b9e3ceda`
- Titel: (binary or unreadable)

## .github/workflows/build-widget.yml
- Kategorie: build_test_ci
- Hash: `42880b1ffb7f94f7`
- Titel: (binary or unreadable)

## .github/workflows/codeql.yml
- Kategorie: build_test_ci
- Hash: `e578581a55abf58d`
- Titel: (binary or unreadable)

## .github/workflows/compliance-governance-gates.yml
- Kategorie: build_test_ci
- Hash: `5fbaf5d0c69ad1b8`
- Titel: (binary or unreadable)

## .github/workflows/compliance-supply-chain.yml
- Kategorie: build_test_ci
- Hash: `bc00b2b0ce899fbe`
- Titel: (binary or unreadable)

## .github/workflows/copilot-code-review.yml
- Kategorie: build_test_ci
- Hash: `67df9c187f101b65`
- Titel: (binary or unreadable)

## .github/workflows/coverage-report-nightly.yml
- Kategorie: build_test_ci
- Hash: `7e846289258c5dc7`
- Titel: (binary or unreadable)

## .github/workflows/crda.yml
- Kategorie: build_test_ci
- Hash: `bb919180ff98a17b`
- Titel: (binary or unreadable)

## .github/workflows/edition-hyperscaler-ci.yml
- Kategorie: build_test_ci
- Hash: `b74bf19be67a21e6`
- Titel: (binary or unreadable)

## .github/workflows/ga-promotion-signoff.yml
- Kategorie: build_test_ci
- Hash: `5578bfa9a2422da4`
- Titel: (binary or unreadable)

## .github/workflows/gate-copilot-regression.yml
- Kategorie: build_test_ci
- Hash: `26b5d8224705f9b9`
- Titel: (binary or unreadable)

## .github/workflows/gate-distributed-knowledge.yml
- Kategorie: build_test_ci
- Hash: `8cd46108cf2bbc01`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-benchmark-ctest.yml
- Kategorie: build_test_ci
- Hash: `2bea96d431b64131`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-community-failclosed.yml
- Kategorie: build_test_ci
- Hash: `a2ab1944bb366743`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-core.yml
- Kategorie: build_test_ci
- Hash: `e35592e861eb5198`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doc-metadata.yml
- Kategorie: build_test_ci
- Hash: `aa6b2ce8ae57fc0f`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doxygen-governance.yml
- Kategorie: build_test_ci
- Hash: `54b2aa7fb8588485`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-edition-license.yml
- Kategorie: build_test_ci
- Hash: `90502118f5c9afd8`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-final-runtime-api.yml
- Kategorie: build_test_ci
- Hash: `4c068252ac381b33`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-hash-sbom.yml
- Kategorie: build_test_ci
- Hash: `dd91b24239ddcf0e`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-merge-readiness.yml
- Kategorie: build_test_ci
- Hash: `95568a45e75c8174`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-module-doxygen-xml.yml
- Kategorie: build_test_ci
- Hash: `900637cefc00be9b`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-plugin-boundary.yml
- Kategorie: build_test_ci
- Hash: `a6ea80c1053c5585`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-primary-doc-structure.yml
- Kategorie: build_test_ci
- Hash: `fb2c7a7f3d0493f1`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-eval.yml
- Kategorie: build_test_ci
- Hash: `e1f94059dac3e030`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase10.yml
- Kategorie: build_test_ci
- Hash: `1681c0bb656c122b`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase7.yml
- Kategorie: build_test_ci
- Hash: `df421c7580604c25`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase8.yml
- Kategorie: build_test_ci
- Hash: `a4974e1d52615e80`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase9.yml
- Kategorie: build_test_ci
- Hash: `6e8421e255ac7397`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-security.yml
- Kategorie: build_test_ci
- Hash: `2c3a03877b8a4233`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-version.yml
- Kategorie: build_test_ci
- Hash: `c6460c02ca88e685`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-version-targeting.yml
- Kategorie: build_test_ci
- Hash: `fca23062af58da44`
- Titel: (binary or unreadable)

## .github/workflows/gate-release-benchmark-validation.yml
- Kategorie: build_test_ci
- Hash: `20c69f30312f2c38`
- Titel: (binary or unreadable)

## .github/workflows/gate-wave-closure.yml
- Kategorie: build_test_ci
- Hash: `4b71d91f930e92a8`
- Titel: (binary or unreadable)

## .github/workflows/gate-workflow-guidelines.yml
- Kategorie: build_test_ci
- Hash: `5afc2a42edbf47bb`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-ai-working.yml
- Kategorie: build_test_ci
- Hash: `26be4b7773d0202a`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-architecture-ci.yml
- Kategorie: build_test_ci
- Hash: `145c04435220c227`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-build-issues.yml
- Kategorie: build_test_ci
- Hash: `28cc8bae4f130a25`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-ci-health.yml
- Kategorie: build_test_ci
- Hash: `eeaaa0fef5ea8fb6`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-compendium-sync.yml
- Kategorie: build_test_ci
- Hash: `c715e87395b0e850`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-docs-db-build.yml
- Kategorie: build_test_ci
- Hash: `c902cb537996f423`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-docs.yml
- Kategorie: build_test_ci
- Hash: `a54b7b05d42b1d71`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-housekeeping.yml
- Kategorie: build_test_ci
- Hash: `21c263ba1be0aa9a`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-issues.yml
- Kategorie: build_test_ci
- Hash: `3eac9240951febcf`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-pr-failure-diagnosis.yml
- Kategorie: build_test_ci
- Hash: `79e30b70f70ed0ef`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-soll-ist-gap-issues.yml
- Kategorie: build_test_ci
- Hash: `82ad329604e11db4`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-workflow-guardrails-observe.yml
- Kategorie: build_test_ci
- Hash: `b624de438556a4b0`
- Titel: (binary or unreadable)

## .github/workflows/msvc.yml
- Kategorie: build_test_ci
- Hash: `90395a27d5c2fffd`
- Titel: (binary or unreadable)

## .github/workflows/nightly-governance-audit.yml
- Kategorie: build_test_ci
- Hash: `ba578246c0558de1`
- Titel: (binary or unreadable)

## .github/workflows/publish-wiki.yml
- Kategorie: build_test_ci
- Hash: `cfbf27bb80fc0f6d`
- Titel: (binary or unreadable)

## .github/workflows/release-build-matrix.yml
- Kategorie: build_test_ci
- Hash: `14500612c5a032b4`
- Titel: (binary or unreadable)

## .github/workflows/release-changelog.yml
- Kategorie: build_test_ci
- Hash: `d97f477879f14fd3`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-approval.yml
- Kategorie: build_test_ci
- Hash: `2aef3be51ad35c5c`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-image.yml
- Kategorie: build_test_ci
- Hash: `c90c79e3a5308859`
- Titel: (binary or unreadable)

## .github/workflows/release-evidence-chain.yml
- Kategorie: build_test_ci
- Hash: `7f277b8a7015e3df`
- Titel: (binary or unreadable)

## .github/workflows/release-linux-distribution.yml
- Kategorie: build_test_ci
- Hash: `b57790a94b20eeeb`
- Titel: (binary or unreadable)

## .github/workflows/release-linux-distro-approval.yml
- Kategorie: build_test_ci
- Hash: `0d32919331508507`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline-approval.yml
- Kategorie: build_test_ci
- Hash: `1bd45850e1e6a3ba`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline.yml
- Kategorie: build_test_ci
- Hash: `e156f24778d96463`
- Titel: (binary or unreadable)

## .github/workflows/release-nightly.yml
- Kategorie: build_test_ci
- Hash: `d30426adfb7a8ed0`
- Titel: (binary or unreadable)

## .github/workflows/release-promote.yml
- Kategorie: build_test_ci
- Hash: `a92910e7ccc60f25`
- Titel: (binary or unreadable)

## .github/workflows/release-rollback.yml
- Kategorie: build_test_ci
- Hash: `9d4c1cac6420e19c`
- Titel: (binary or unreadable)

## .github/workflows/release-windows-distribution.yml
- Kategorie: build_test_ci
- Hash: `1e353b6b8f278370`
- Titel: (binary or unreadable)

## .github/workflows/release-windows-distro-approval.yml
- Kategorie: build_test_ci
- Hash: `956b7f63c2049670`
- Titel: (binary or unreadable)

## .github/workflows/release-winget-approval.yml
- Kategorie: build_test_ci
- Hash: `bb4f1e6838c842b8`
- Titel: (binary or unreadable)

## .github/workflows/release-winget.yml
- Kategorie: build_test_ci
- Hash: `d97ea10c5b94d9f8`
- Titel: (binary or unreadable)

## .github/workflows/release-wordpress-press.yml
- Kategorie: build_test_ci
- Hash: `641b3d6490580dca`
- Titel: (binary or unreadable)

## .github/workflows/reusable-benchmark-build-run-report.yml
- Kategorie: build_test_ci
- Hash: `21d1cddc7405c381`
- Titel: (binary or unreadable)
