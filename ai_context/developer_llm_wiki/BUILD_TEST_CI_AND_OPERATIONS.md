---
Author: "ThemisDB Team"
Created: "2026-11-02"
Last Updated: "2026-11-05"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Developer LLM Wiki — Build/Test/CI/Operations

Datum: 2026-11-02
Status: Active

## macOS LLVM backend build requirements

- Sources: `cmake/Dependencies.cmake`, `tests/test_deployment_workflow_regressions.py`; Actions run `41960105333`, job `127024660181`.
- The existing Apple `ggml-base` C++ `<cerrno>` force-include resolves the earlier GGUF compilation failure. The newer run exposes a missing `<memory>` include in `ggml-metal.cpp` and `elaborated-enum-base` errors from the Xcode 15.4 SDK headers used by Accelerate.
- Apple `ggml-metal` C++ compilation privately force-includes `<memory>`. For Apple builds using upstream/Homebrew Clang, a compiler-checked `-Wno-error=elaborated-enum-base` on `ggml-cpu` and `ggml-blas` C++ compilation retains the SDK warning without treating it as fatal. C/Objective-C sources and downstream consumers are unchanged; Metal and Accelerate remain enabled.
- Focused host CMake/compiler regressions exercise missing-header failure, successful compilation, platform/compiler guards, missing targets, and private C++ flag scope. They do not replace a native macOS ARM64 build; native SDK and packaging validation remain pending.
- Wiki health metadata dates from 2026-11-02 and workflow summaries are incomplete. Current source and concrete Actions logs take precedence.
## Windows release compiler-memory mitigation

- Sources: `.github/workflows/release-build-matrix.yml`, `cmake/ModularBuild.cmake`, and [Actions run 41861219716, job 126684560606](https://github.com/makr-code/ThemisDB/actions/runs/41861219716/job/126684560606). The task-provided log evidence reports concurrent `policy_api_handler.cpp` and `monitoring_api_handler.cpp` failing with MSVC C1060 heap exhaustion using the x64 compiler; monitoring already has the low-memory `/Od` setting.
- The companion workflow correction changes only the `windows-amd64` `build_command`: `CMAKE_BUILD_PARALLEL_LEVEL=1` and explicit `--parallel 1` (both previously `2`). Configure-time vcpkg concurrency stays at `2`; other platforms and production optimization settings remain unchanged. Sequential compilation avoids overlapping heavy translation units without establishing a successful Windows build.
- `tests/test_windows_release_build_memory.py` executes matrix preparation and the generated build shell with stub `cmake` to verify concurrency and pipeline failure propagation through the logging pipe. Stub execution is not MSVC build evidence.
- The Linux sandbox cannot run MSVC. A **new Actions run on corrected source** is required; retries retain the original source SHA. No Windows build success is claimed.
- `WIKI_STATUS.json` was generated on 2026-11-02 with 8066 sources; workflow summaries marked “binary or unreadable” are incomplete. Current source and Actions logs take precedence over generated wiki metadata.

## Global transaction durability regression coverage

- Canonical sources: `tests/transaction/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/test_global_transaction_manager.cpp`, `src/transaction/global_transaction_manager.cpp`, `include/sharding/wal_manager.h`.
- Modular builds must register the existing global transaction regression suite as well as non-modular builds. Enable `THEMIS_BUILD_TESTS` when configuring the focused validation build; release packaging configurations that disable tests do not provide this evidence.
- The coordinator reports internal errors with `GlobalTxnResult::FAILURE` and checks `WALManager::flushDurable()` through its WAL helper. The void `flush()` API is not a durability success indicator.
- Verify focused tests before accepting a new native ARM64 Release build and its packages. Re-running an older failed Actions run retains its original source commit; use a new Mainline dispatch on corrected `develop`.
- Wiki health metadata was generated on 2026-11-02 and contains incomplete workflow summaries marked “binary or unreadable”; current source and Actions logs take precedence. No ARM64 build or packaging success is inferred from this documentation.

## Docker ARM64 bootstrap preflight

Exact model backend not verifiable from final document text alone.

- Sources: `.github/workflows/reusable-docker-image-platform.yml`, `docker/Dockerfile.unified`, `tests/test_docker_runtime_package_regressions.py`.
- ARM64 platform builds require the pinned `bootstrap-vcpkg.sh`, which builds vcpkg natively on ARM64; the `vcpkg-glibc` release binary is x86-64-only.
- Every selected release tag (including alpha, beta, rc, and nightly tags) is checked before platform dispatch, even when registry publication and dry-run are disabled. This includes tags resolved implicitly from the version files for publication: the parent preflight and override notice also check `steps.version.outputs.source_is_tag`. Untagged developer-track runs retain platform-level validation. `override_docker_source_preflight` skips dependency/runtime source checks but not the release-source ARM64 bootstrap check.
- Set `dry_run=true` to build both platforms, run Trivy vulnerability scans, and execute runtime smoke checks without registry login/publication or release-status tracker updates. Tagged dry-runs also run the source preflight. Dry-run takes precedence over `push_to_registry`.
- The reusable platform workflow enforces the same dry-run no-push behavior, runs the ARM64 bootstrap check and Trivy against the locally loaded image, then executes runtime validation. Plain validation runs without `dry_run` keep their existing build/runtime behavior.
- Run `41900183288`, AMD64 job `126830955426`, selected obsolete `v2.4.0-alpha` source (`3cfe59e6c5e7093cec8391291b5fc405dfdab246`) and failed all eight dependency/runtime source checks before Docker compilation. The stable-only caller condition had allowed the alpha tag to reach platform dispatch; tagged prereleases now receive the same early check.
- Run `41984551595`, ARM64 job `127107984594`, selected the same obsolete tag implicitly (`push_to_registry=true`, empty `tag_name`) and failed the native-bootstrap check. Checking only explicit tag inputs missed this path; the resolved-source flag closes it without changing the selected commit or bypassing ARM64 validation.
- If a release tag fails source preflight, publish a **new release tag** containing the corrected Dockerfile; the workflow does not move tags or substitute newer source for the selected release commit. To validate current workflow source instead, set `build_from_workflow_source=true`; this forces dry-run and disables registry publication.

## Windows compiler memory and ARM SIMD guard

- Sources: `.github/workflows/release-build-matrix.yml`, `cmake/CompilerOptions.cmake`, `cmake/ModularBuild.cmake`, `cmake/README.md`, `tests/test_mainline_compiler_regressions.py`.
- The Windows release matrix uses dependency/configure concurrency 1 and build `--parallel 1`: `C1060` occurred even with monitoring's `/Od` workaround. Monitoring, index and prompt handlers share `THEMIS_MSVC_HEAVY_HANDLER_LOW_MEMORY` (default `ON`, `/Od` outside Debug); opting out retains `/bigobj` and unity exclusion.
- `CompilerOptions.cmake` runs before platform detection. AVX2/FMA flags and `THEMIS_HAS_AVX2` require an x64 target processor, Release and AVX2 enabled; QNAP's x86 baseline is likewise target-guarded. An x64 host must not inject x86 flags into ARM64 targets.
- Focused Python tests configure CMake policies without native compiler execution. They do not prove native Windows/ARM64 compilation or package success. Dispatch a new Mainline run containing the corrected source on `develop`; retrying the original failed run retains its original SHA.

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
- Hash: `4924c5315f997ad1`
- Titel: (binary or unreadable)

## .github/workflows/build-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `49b371583baa11bf`
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
- Hash: `729447d9b4865f07`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-llm-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `06d4bf8f34a54be2`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-transaction.yml
- Kategorie: build_test_ci
- Hash: `c0bd97312e911678`
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
- Hash: `564a9b0c3ee3a8ef`
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
- Hash: `a0125bd989f1b129`
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
- Hash: `dd96084910101341`
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
- Hash: `9b0644fce3451072`
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
- Hash: `eeee4a037355adac`
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
- Hash: `f381b8463cbb9f58`
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
- Hash: `5b6062bbb310ee7c`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline.yml
- Kategorie: build_test_ci
- Hash: `ae6d60e49c09472e`
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
- Hash: `c8b9869e1c8c0342`
- Titel: (binary or unreadable)

## .github/workflows/release-wordpress-press.yml
- Kategorie: build_test_ci
- Hash: `641b3d6490580dca`
- Titel: (binary or unreadable)

## .github/workflows/reusable-benchmark-build-run-report.yml
- Kategorie: build_test_ci
- Hash: `fb6c9e997f8c0092`
- Titel: (binary or unreadable)
