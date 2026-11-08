---
Author: "ThemisDB Team"
Created: "2026-11-02"
Last Updated: "2026-11-08"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot; OpenAI API assistant"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Developer LLM Wiki — Build/Test/CI/Operations

> Exact model backend not verifiable from final document text alone.

Datum: 2026-11-02
Status: Active

## Docker Python configure prerequisites (2026-11-08)

- [Actions run `42457016399`, job `128661153265`](https://github.com/makr-code/ThemisDB/actions/runs/42457016399/job/128661153265)
  failed in the dependency stage while vcpkg ran `autoreconf -vfi` for Python
  3.11.10, before ThemisDB compilation. The pinned Python port requires
  `autoconf-archive` for `AX_C_FLOAT_WORDS_BIGENDIAN`; Autoconf alone does not
  provide that macro.
- The shared `base` stage in the canonical [Dockerfile](../../docker/Dockerfile.unified)
  now installs `autoconf-archive` alongside the existing Autoconf, Automake,
  Libtool and pkg-config tools. Runtime packages and dependency versions are unchanged.
- The existing [Docker regression suite](../../tests/test_docker_runtime_package_regressions.py)
  checks the Python configure prerequisites in the base stage. A fresh Actions
  build on corrected source is still required to establish full image build success.

## Docker documentation build context (2026-11-08)

- Task-provided evidence: [Actions run `42406530936`, job `128507711178`](https://github.com/makr-code/ThemisDB/actions/runs/42406530936/job/128507711178)
  failed before compilation at `COPY docs ./docs` and `COPY ai_context ./ai_context`;
  BuildKit could not find the checksum sources. Root [`.dockerignore`](../../.dockerignore)
  used deny-by-default `*` without exceptions for these directories.
- The corrected allowlist retains `*` and adds `!docs/`, `!docs/**`,
  `!ai_context/`, and `!ai_context/**`. Canonical [Dockerfile](../../docker/Dockerfile.unified)
  still requires both source trees, `THEMIS_DOCS_DB_MODE=BUILD`, the `docs_database`
  target, documentation installation, and runtime payload validation. Keep the
  documentation/model payload mandatory; do not substitute empty placeholder
  directories or bypass payload gates.
- Canonical [regression source](../../tests/test_docker_runtime_package_regressions.py):
  `test_documentation_payload_sources_are_in_build_context` checks deny-by-default,
  source directory existence, both COPY instructions, and directory/content
  exceptions without explicit re-exclusions. This is source coverage, not full
  Docker build evidence; validation is handled by the parent task.
- A **fresh Actions run on corrected source** is required. Retrying the old SHA
  does not apply the fix; no full build or packaging success is claimed.
  [Wiki status](WIKI_STATUS.json) remains generated 2026-11-02 / 8066 sources with
  incomplete workflow summaries; current canonical source and concrete run logs
  take precedence. This is a level2 build-test sync (`DOC-WEEKLY-2026-45`), not
  a wiki regeneration.

## Linux AMD64 release CPU portability (2026-11-07)

- Run `42346575119`, job `128325782592`, compiled successfully but failed validation
  when `import_docs_rocksdb` terminated with `Illegal instruction` before producing
  the documentation database. Configure selected ThemisDB and GGML `-march=native`;
  the lane restores compiler objects across hosted runners without CPU identity.
- The Linux AMD64 release matrix now passes `THEMIS_ENABLE_AVX2=OFF` and
  `GGML_NATIVE=OFF`, selecting ThemisDB's baseline `-march=x86-64`. GGML's default
  SSE4.2, AVX, AVX2, BMI2, FMA and F16C options are explicitly disabled as well.
  This removes runner-specific instruction requirements
  without bypassing the mandatory documentation/model payload or packaging gates.
  Other platforms and developer presets are unchanged.
- Sources: [release matrix](../../.github/workflows/release-build-matrix.yml),
  [architecture policy](../../cmake/platforms/ArchitectureOptimizations.cmake),
  [regression tests](../../tests/test_release_runtime_payload.py).
  Tests exercise matrix generation and the real CMake architecture policy.
- The crash log contains no instruction address or core backtrace; a fresh Actions
  run on corrected source must confirm importer execution and package validation.
  Wiki freshness remains November 2 / 8066 sources; this entry is a focused source sync.

## Windows dependency preparation before release compile (2026-11-07)

- Acceptance plan: isolate the enabled Windows release prerequisite; bound its
  configure process tree, save completed/partial binary archives before compile,
  fail closed on incomplete preparation, retain serial MSVC, and test/document
  cache diagnostics and source/feature parity.
- Sources: `.github/workflows/release-build-matrix.yml`,
  `.github/workflows/reusable-cmake-build.yml`,
  `scripts/prepare-windows-dependencies.py`,
  `tests/test_windows_dependency_preparation.py`, and
  [Windows operations guide](../../docs/de/guides/WINDOWS_BUILD_SETUP.md).
  Task-provided evidence: run `42278019978`, job `128119148701` spent six hours
  configuring (vcpkg gRPC 203/220; Debug ~84 minutes and Release >=84 minutes),
  was cancelled before ThemisDB compilation, and skipped the late cache-save guard.
- The selected Windows lane now has a separate `windows-dependencies` job using
  the same pinned source and exact configure command as the consumer. It performs
  real project configuration for manifest/default-feature parity, but no ThemisDB
  compile. Disabled Windows preparation is skipped; Linux/macOS still execute
  after a Windows preparation failure.
- Budget starts before checkout: a 300-minute job-relative deadline and at most
  270 minutes in dependency configure, PID-tree termination (`taskkill /T /F` on
  Windows), then cache save with a 30-minute upload limit inside the 360-minute
  job. Controlled timeout returns 124; ordinary errors retain their exit codes.
  Failure/timeout still schedules partial snapshot save, but does not authorize
  compilation. Hard cancellation/runner loss remains outside this guarantee.
- Preparation publishes a cache key only after successful configure and save.
  Windows compile requires exact snapshot restore and vcpkg binary-only installation;
  ABI drift, missing ZIPs or cache-service failure stops before compile rather than
  rebuilding dependencies without a bound. Prepared archives survive later MSVC
  failure. The opt-in `release-deps-v2` namespace is platform/toolchain/triplet
  separated, records manifest/overlay and configure hashes, vcpkg/MSVC versions and source SHA,
  and disables old build-tree restore. Windows paths are native-normalized.
- Snapshot keys remain unique per run/attempt. Prefix restore during preparation
  is reported separately from exact hits and no-cache, with matched key and ZIP
  count. Install `Restored N package(s)` output, not archive inventory, establishes
  ABI reuse. Missing output is “not reported”; zero sccache requests are not misses.
- Dependency parallelism defaults to 2 (input `windows_dependency_concurrency`,
  validated range 1–4). Actual ThemisDB MSVC remains `--parallel 1`. Unchanged
  `x64-windows` dual Debug/Release packages retain target/host linkage and ABI
  compatibility, including supported Debug release-matrix dispatches. No global
  release-only environment hack or unvalidated overlay triplet is introduced.
- Host regressions do not establish native Windows CI success. A new corrected-source
  dispatch must verify taskkill tree cleanup, partial upload, binary-only ABI restore,
  MSVC and packaging. Existing Apple-backend Python tests currently have unrelated
  source-extraction errors; they were not removed or modified for this change.
- Run `42396942036`, job `128476529660`, failed before configure because Python's
  `bash` lookup launched WSL instead of Git Bash. The workflow now passes its active
  Git Bash executable as a native Windows path in `THEMIS_DEPENDENCY_BASH` to the
  dependency supervisor. The helper validates this environment contract, not an
  unused `--bash` argument, and rejects missing/empty paths without a PATH fallback.
  The consumer job `128477281514` failed with an empty dependency cache key because
  preparation failed; its fail-closed prerequisite gate remains unchanged.
- Wiki status remains generated 2026-11-02 / 8066 sources with incomplete workflow
  summaries; source and concrete run logs take precedence.

## macOS LLVM backend build requirements

- Sources: `cmake/Dependencies.cmake`, `tests/test_deployment_workflow_regressions.py`; Actions run `41960105333`, job `127024660181`.
- The existing Apple `ggml-base` C++ `<cerrno>` force-include resolves the earlier GGUF compilation failure. The newer run exposes a missing `<memory>` include in `ggml-metal.cpp` and `elaborated-enum-base` errors from the Xcode 15.4 SDK headers used by Accelerate.
- Apple `ggml-metal` C++ compilation privately force-includes `<memory>`. For Apple builds using upstream/Homebrew Clang, a compiler-checked `-Wno-error=elaborated-enum-base` on `ggml-cpu` and `ggml-blas` C++ compilation retains the SDK warning without treating it as fatal. C/Objective-C sources and downstream consumers are unchanged; Metal and Accelerate remain enabled.
- Focused host CMake/compiler regressions exercise missing-header failure, successful compilation, platform/compiler guards, missing targets, and private C++ flag scope. They do not replace a native macOS ARM64 build; native SDK and packaging validation remain pending.
- Wiki health metadata dates from 2026-11-02 and workflow summaries are incomplete. Current source and concrete Actions logs take precedence.
## Windows release compiler-memory mitigation

- Sources: `.github/workflows/release-build-matrix.yml`, `cmake/ModularBuild.cmake`, `cmake/README.md`, and latest [Actions run 42155076805, job 127687580213](https://github.com/makr-code/ThemisDB/actions/runs/42155076805/job/127687580213). The task-provided log evidence reports MSVC C1060 heap exhaustion despite serial compilation and the handler `/Od` override.
- The `windows-amd64` ThemisDB build still sets `VCPKG_MAX_CONCURRENCY=1` and `CMAKE_BUILD_PARALLEL_LEVEL=1`, with explicit build `--parallel 1`. Dependency configure now has separate modest parallelism (see prerequisite section above). Serial compilation alone did not resolve the earlier C1060 failure.
- The scoped correction adds explicit `/Ob0` alongside `/Od` for monitoring, index and prompt handlers under the same non-Debug `THEMIS_MSVC_HEAVY_HANDLER_LOW_MEMORY` guard (default `ON`). `/Ob0` overrides inherited `/Ob2` inlining. Setting the option to `OFF` removes both overrides, restoring normal optimization and inherited inlining; `/bigobj`, unity exclusion and global `/Zm200` remain. Other translation units are unchanged.
- `tests/test_windows_release_build_memory.py` executes matrix preparation and the generated build shell with stub `cmake` to verify concurrency and pipeline failure propagation through the logging pipe. Stub execution is not MSVC build evidence.
- The Linux sandbox cannot run MSVC. A **new Actions run on corrected source** is required; retries retain the original source SHA. No Windows build success is claimed.
- `WIKI_STATUS.json` was generated on 2026-11-02 with 8066 sources; workflow summaries marked “binary or unreadable” are incomplete. Current source and Actions logs take precedence over generated wiki metadata.

## Configure-time source download recovery

- Sources: `.github/workflows/reusable-cmake-build.yml`, `tests/test_deployment_workflow_regressions.py`, and [Actions run 42174218456, job 127750342277](https://github.com/makr-code/ThemisDB/actions/runs/42174218456/job/127750342277).
- Windows AMD64 configuration failed when vcpkg exhausted three download attempts for the Arrow 25.0.1 archive from `archive.apache.org`. The later missing Ninja/compiler diagnostics followed the aborted dependency install; vcpkg had already detected MSVC successfully.
- The existing one-time configure retry now handles `Download timed out` on every runner architecture, retaining downloaded dependencies and the original vcpkg version/checksum verification. Existing ARM64 `BUILD_FAILED` recovery is unchanged. Successful configuration and unrelated Windows build failures are not retried; a failed retry propagates its exit status.
- Run [42457047962, job 128660718401](https://github.com/makr-code/ThemisDB/actions/runs/42457047962/job/128660718401) failed during fmt dependency configuration when GitHub returned HTTP 504 for its pinned patch download. The same bounded retry now recognizes vcpkg curl HTTP 5xx responses; regression tests cover 502/503/504 and confirm HTTP 404 is not retried.
- Focused shell regressions cover recovery, retry bounds, final failure propagation, non-network failures, and ARM64 behavior. They do not establish native Windows build or packaging success; a fresh Actions run containing the fix remains required.

## Global transaction durability regression coverage

- Canonical sources: `tests/transaction/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/test_global_transaction_manager.cpp`, `src/transaction/global_transaction_manager.cpp`, `include/sharding/wal_manager.h`.
- Modular builds must register the existing global transaction regression suite as well as non-modular builds. Enable `THEMIS_BUILD_TESTS` when configuring the focused validation build; release packaging configurations that disable tests do not provide this evidence.
- The coordinator reports internal errors with `GlobalTxnResult::FAILURE` and checks `WALManager::flushDurable()` through its WAL helper. The void `flush()` API is not a durability success indicator.
- Verify focused tests before accepting a new native ARM64 Release build and its packages. Re-running an older failed Actions run retains its original source commit; use a new Mainline dispatch on corrected `develop`.
- Wiki health metadata was generated on 2026-11-02 and contains incomplete workflow summaries marked “binary or unreadable”; current source and Actions logs take precedence. No ARM64 build or packaging success is inferred from this documentation.

## macOS pinned llama.cpp portability (2026-11-05)

- Evidence: the [failed macOS CI job](https://github.com/makr-code/ThemisDB/actions/runs/42075920970/job/127417305713) and CMake-Agent handoff identify missing `<memory>` for `std::unique_ptr` in pinned `ggml-metal.cpp`, plus Homebrew Clang's fixed-enum diagnostic in Xcode 15.4 SDK Accelerate headers included by `ggml-cpu`.
- Canonical implementation: [cmake/Dependencies.cmake](../../cmake/Dependencies.cmake) adds Apple-only, PRIVATE, C++-only `-include memory` to `ggml-metal`. For `ggml-cpu`, Apple plus upstream compiler ID `Clang` (not `AppleClang`) gates a capability probe using `-Werror=elaborated-enum-base`; only when supported is PRIVATE, C++-only `-Wno-error=elaborated-enum-base` applied.
- Keep other warnings-as-errors intact: no global warning weakening, no Metal/Accelerate disable, and no compiler downgrade. Retain the current Homebrew compiler headers/runtime needed for `std::jthread`.
- Regression sources: [fixture CMakeLists.txt](../../tests/cmake/ggml_portability/CMakeLists.txt) and [RunCase.cmake](../../tests/cmake/ggml_portability/RunCase.cmake) check target/language/platform/compiler guards, private options, missing targets, unsupported diagnostics, and negative controls retaining unrelated `-Werror` failures. They use synthetic sources, not a native Apple SDK build.
- CMake-Agent reported Linux results: fixture **11/11**, [deployment workflow regressions](../../tests/test_deployment_workflow_regressions.py) **30/30**, and existing [Ninja discovery tests](../../tests/test_cmake_ninja_discovery.py) **2/2** passed. These results were not rerun for this documentation-only update.
- Limitation: no native macOS build has validated the fix; a fresh macOS CI build is still required. Linux fixture success does not establish Metal/Accelerate compilation, linking, runtime, or packaging success.

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
- The Windows release matrix uses dependency/configure concurrency 1 and build `--parallel 1`; latest run `42155076805`, job `127687580213`, still hit `C1060` with `/Od`. Monitoring, index and prompt handlers now share `/Od` plus explicit `/Ob0` (overriding inherited `/Ob2`) under `THEMIS_MSVC_HEAVY_HANDLER_LOW_MEMORY` (default `ON`, outside Debug). Opting out removes both overrides while retaining `/bigobj` and unity exclusion; native validation remains pending.
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
