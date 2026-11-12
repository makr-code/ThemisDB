---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-12"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Developer LLM Wiki — Build/Test/CI/Operations

Datum: 2026-11-12
Status: Active

## Governance maturity manifest verification

The canonical manifest is `docs/governance/MATURITY_EVIDENCE_MANIFEST.json`; its
gate records are grouped under `gates`, not `evidence_items`. The
`.github/scripts/verify_maturity_gates.py` verifier evaluates declared
`validation_rule` checks, resolves evidence paths inside the repository, and uses
Git history for file-freshness checks so checkout timestamps do not make stale
evidence appear current. Content patterns allow literals and the bounded `.*`
wildcard rather than unrestricted regular expressions. `PASS` entries with
unsupported rules fail closed.
`PENDING` entries remain visibly pending and are not counted as passed; current
workflow policy does not make pending entries block the run.

The
`.github/workflows/compliance-governance-gates.yml` workflow uploads the JSON
report as a run artifact and separately runs the hard 100%-maturity checker.
Release-candidate validation invokes the same verifier for Wave 7–9 groups.
Operational guidance is in
[`MATURITY_AUTOMATION_RUNBOOK.md`](../../docs/operations/MATURITY_AUTOMATION_RUNBOOK.md).

## Docker runtime APT mirror synchronization recovery

Run [42762843024, job 129655324640](https://github.com/makr-code/ThemisDB/actions/runs/42762843024/job/129655324640)
failed in the runtime stage's `apt-get update`: the Noble security package index
had an unexpected size relative to its signed release metadata. Compilation and
runtime smoke validation were not reached. The runtime stage in
[`docker/Dockerfile.unified`](../../docker/Dockerfile.unified) now retries index
refreshes up to five times, clearing failed indexes and waiting 15 seconds
between attempts. `--error-on=any` rejects partial index refreshes; persistent
failures remain fatal and package signature/hash verification is unchanged.
The independent runtime stage does not inherit base-stage APT configuration.
[`tests/test_docker_runtime_package_regressions.py`](../../tests/test_docker_runtime_package_regressions.py)
executes the shell logic for immediate success, transient recovery, exhausted
retries, and package installation failure. A full platform image build remains
required in CI from a source revision containing this fix.

## Docker Python autoreconf prerequisite gate

The current Docker base stage installs `autoconf-archive` before vcpkg bootstrap. Release-source preflight, the reusable platform workflow, and the platform test enforce `test_base_installs_python_autoreconf_prerequisites`. For the compliance export, `docker-compose.yml` selects the repository-root [Dockerfile](../../Dockerfile), not `docker/Dockerfile.unified`. The earlier failed run [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686/job/128248301027) used the root Dockerfile before this prerequisite was added; the missing package matched vcpkg's warning and was a likely cause of its `python3:x64-linux` `autoreconf -vfi` failure, although stderr was not captured. Follow-up run [42761863772](https://github.com/makr-code/ThemisDB/actions/runs/42761863772) succeeded at SBOM and C5 manifest packaging/validation/upload, but the SOC 2 export failed during root Dockerfile apt package downloads from `security.ubuntu.com` (timeout/404) before vcpkg/Python. It therefore does not verify the prerequisite. SBOM signing and tag-only provenance were skipped, and a successful SOC 2 evidence-export build remains required. An older selected source without this package can still fail during `autoreconf`; rerunning that workflow does not change its source. Use a new release tag containing the fix, or set `build_from_workflow_source=true` for non-publishing validation. See the [operational guidance](../../docs/en/deployment/DOCKER_MULTI_EDITION_BUILD.md#troubleshooting) for details.

## C5 key-lifecycle persisted-event test

`tests/auth/test_c5_key_lifecycle_audit_evidence.cpp` exercises the real `JWTKeyRotationManager::rotateActiveKey()` API with an attached `AuditLogger` and asserts the persisted `KEY_ROTATED` JSONL record. It is registered as `release_critical`; the PR gate uploads its JUnit result and JSONL output, and mainline CTest evidence includes the audit-output directory. This is API-level coverage only: application-runtime logger wiring is unverified, the test has not yet passed CI, and five other required lifecycle event classes have no production emitter. The local configure attempt could not complete because RocksDB development dependencies were unavailable. See the canonical [C5 key lifecycle evidence map](../../audit/evidence/c5/KEY_LIFECYCLE_AUDIT_EVENTS.md).

## Windows dependency snapshot handoff

Run `42638257654`, job `129657233420` failed before compilation because the
completed dependency cache saved by preparation was no longer available on a
failed-job rerun, despite an identical key. The canonical
`.github/workflows/reusable-cmake-build.yml` now uploads the completed snapshot
as a 30-day run artifact; `release-build-matrix.yml` passes its immutable artifact
ID and source/configure/toolchain identity to the consumer. Consumption requires
that artifact and retains binary-only vcpkg installation and identity validation.
Caches accelerate preparation only; partial preparation snapshots are never
published as completed artifacts. Failed-job reruns reuse the original producer's
artifact ID, not the current attempt number. If the artifact expires, rerun
preparation too. Older workflow revisions require a new run containing this fix.
Focused coverage: `tests/test_windows_dependency_preparation.py`.

Run `42768142107`, job `129687972024` downloaded that artifact but failed
restoration because Git Bash's `tar` could not resolve the Windows-form
`GITHUB_WORKSPACE` path passed to `-C`. Snapshot creation and restoration now
normalize the workspace with `cygpath -u`; restoration also ensures the cache
directory exists. The round-trip regression models a Windows-form workspace and
executes real archive/extract commands with spaced paths, while retaining
identity-mismatch and missing-artifact rejection.

Attempt 2 of the same run, [job `129791530083`](https://github.com/makr-code/ThemisDB/actions/runs/42768142107/job/129791530083),
repeated this failure: artifact download succeeded, but the executed workflow
still passed `"$GITHUB_WORKSPACE/.cache"` directly to `tar -C`, which exited with
code 2 (`Cannot open: No such file or directory`). Configuration, compilation,
and CTest were skipped; the subsequent missing CTest evidence is a consequence,
not the root cause. The current workflow already normalizes the workspace and
creates the extraction directory. All 28 Windows dependency preparation tests
passed locally on 2026-11-10, including the archive round trip. Start a new
mainline run from a revision containing the corrected reusable workflow; another
failed-job rerun of this old run does not pick up that workflow fix. Native
Windows validation of the corrected workflow remains pending.

## MSVC monitoring route translation-unit split

Run `42638257654`, job `129343673144` failed with VS18 MSVC 14.51 `C1060` at
`monitoring_api_handler.cpp(2246)` despite serial compilation and
`/Od /Ob0 /GL- /bigobj /Zm200`. Only `MonitoringApiHandler::registerRoutes()`
(all 47 routes unchanged) is moved to `src/server/monitoring_api_routes.cpp`,
registered in both build layouts with the handler's existing MSVC properties.
No API or flag changes; modular non-Debug low-memory defaults and opt-out are
preserved. Follow-up run `42747840520`, job `129662241930` still failed at
`monitoring_api_routes.cpp(155)` with `C1060`. Registration now delegates to eight
file-local helpers (at most ten routes each), bounding per-function nested JSON
initializer pressure while preserving all annotations and registration order.
`tests/test_mainline_compiler_regressions.py` checks the batch bound and dispatch;
`tests/server/test_server_openapi_drift_focused.cpp` covers actual registration,
the nested license schema, and repeat-registration stability.
On 2026-11-11, [run `42972100442`, job `130366604809`](https://github.com/makr-code/ThemisDB/actions/runs/42972100442/job/130366604809)
confirmed the eight-helper split was insufficient: serial VS18 MSVC 14.51.36231
Hostx64/x64 with `/Od /Ob0 /GL- /bigobj /Zm200` failed with `C1060` at
`monitoring_api_routes.cpp(158)`, ending the single deeply nested license response
initializer. Only that helper now constructs properties, schema, and response in
separate shallow JSON statements (no raw JSON parsing). All nine properties,
examples, date format, response content, metadata, 47 routes, and registration
order remain unchanged; flags, source lists, workflows, and other routes are
untouched. The existing Python regression guards single-expression complexity;
the existing C++ license check compares the complete response/schema and metadata.
Local Linux validation on 2026-11-11 passed all 11 compiler regressions. The
Windows memory suite passed three tests but errored in two configure tests before
log creation: `cygpath` is unavailable and `THEMIS_CACHE_TOOLS_PREFIX` is unset.
The three build/parallelism tests passed explicitly too. The system-package
`community-release` configure with tests enabled failed at missing RocksDB;
the existing C++ focused target could not build without a generated `build.ninja`,
and C++ tests were not run.
Native Windows validation remains pending. See
[MSVC Handler Optimization](../../cmake/README.md#msvc-handler-optimization)
for current policy and preserved historical evidence.

## PR Merge Readiness failure: status-call concurrency and final runtime

Run [42964785412](https://github.com/makr-code/ThemisDB/actions/runs/42964785412) recorded `call-plugin-boundary=cancelled` and `call-final-runtime-api=failure` on parent job [130384787522](https://github.com/makr-code/ThemisDB/actions/runs/42964785412/job/130384787522). The actual plugin enforcement job [130364267816](https://github.com/makr-code/ThemisDB/actions/runs/42964785412/job/130364267816) succeeded; the optional `human-loop-resolution` status-call job [130365663917](https://github.com/makr-code/ThemisDB/actions/runs/42964785412/job/130365663917) was cancelled. Therefore the cancelled parent plugin call is not evidence that plugin enforcement failed; the final-runtime failure remains a separate readiness failure.

The reusable status workflow now scopes its concurrency group by operation plus a stable target identity (issue number, or marker/title when no issue number is available), while keeping `cancel-in-progress: false`. This avoids unrelated status calls from sharing a caller/ref group; a subsequent CI run is still required to verify the parent gate no longer inherits a cancelled label-resolution result.

## TinyLlama inference gate: missing modular EmbeddedLLM runtime

Run [42964785479, job 130387233834](https://github.com/makr-code/ThemisDB/actions/runs/42964785479/job/130387233834) failed five TinyLlama tests with `EmbeddedLLM: no backend configured`; modular inference had selected the API stub instead of the real facade. The current branch's `tests/llm/CMakeLists.txt` now requires `THEMIS_ENABLE_LLM=ON`, compiles `src/llm/embedded_llm.cpp` into the model-backed modular inference targets, and links `themis_llm`; the inference workflow enables LLM. The cited failure predates these changes and cannot verify them. A prior local CMake configure did not reach target generation because RocksDB development headers were unavailable. CI validation of this remediation remains pending; no current-branch test pass is claimed.

## Release-critical test artifact executable modes

In the same run, [job 130381306099](https://github.com/makr-code/ThemisDB/actions/runs/42964785479/job/130381306099) failed three restored CTest executables with `permission denied`. The build directory had been transferred directly as an Actions artifact, which did not preserve executable modes. `.github/workflows/gate-pr-core.yml` now transfers the build directory inside a tar archive and extracts it before CTest; a local tar round-trip verified executable-mode preservation. The latest PR Core run is `action_required`, so CI validation of this fix remains pending.

## Docker runtime APT mirror synchronization recovery

Run [42762843024, job 129655324640](https://github.com/makr-code/ThemisDB/actions/runs/42762843024/job/129655324640)
failed in the runtime stage's `apt-get update`: the Noble security package index
had an unexpected size relative to its signed release metadata. Compilation and
runtime smoke validation were not reached. The runtime stage in
[`docker/Dockerfile.unified`](../../docker/Dockerfile.unified) now retries index
refreshes up to five times, clearing failed indexes and waiting 15 seconds
between attempts. `--error-on=any` rejects partial index refreshes; persistent
failures remain fatal and package signature/hash verification is unchanged.
The independent runtime stage does not inherit base-stage APT configuration.
[`tests/test_docker_runtime_package_regressions.py`](../../tests/test_docker_runtime_package_regressions.py)
executes the shell logic for immediate success, transient recovery, exhausted
retries, and package installation failure. A full platform image build remains
required in CI from a source revision containing this fix.

## PostgreSQL SQL-dump importer linkage

[Run `42854980938`, job `129982571957`](https://github.com/makr-code/ThemisDB/actions/runs/42854980938/job/129982571957) failed while linking `module_importers_test_postgres_insert_token_progress_focused`, before CTest, with undefined `PostgreSQLImporter` constructor, `initialize`, `importData`, and destructor symbols. [ModularBuild.cmake](../../cmake/ModularBuild.cmake) now always includes `../src/importers/postgres_importer.cpp`, matching the [monolithic importer registration](../../cmake/CMakeLists.txt): SQL-dump import is independent of `THEMIS_ENABLE_POSTGRES_WIRE`. In the modular layout, `postgres_session.cpp` and `postgres_importer_mdm.cpp` remain gated by that option.

Reported specialist validation evaluated the extracted actual CMake source lists with wire ON/OFF: the importer appeared exactly once in both cases, while session and MDM sources remained conditional; the old OFF omission was reproduced in memory. All 10 existing [mainline compiler regression tests](../../tests/test_mainline_compiler_regressions.py) passed; no persistent test was added. Native C++ tests were not run locally because gtest, JSON, and spdlog headers were unavailable; successful native linking and CTest remain unverified.

## Docker Python autoreconf prerequisite gate

The current Docker base stage installs `autoconf-archive` before vcpkg bootstrap. Release-source preflight, the reusable platform workflow, and the platform test enforce `test_base_installs_python_autoreconf_prerequisites`. For the compliance export, `docker-compose.yml` selects the repository-root [Dockerfile](../../Dockerfile), not `docker/Dockerfile.unified`. The earlier failed run [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686/job/128248301027) used the root Dockerfile before this prerequisite was added; the missing package matched vcpkg's warning and was a likely cause of its `python3:x64-linux` `autoreconf -vfi` failure, although stderr was not captured. Follow-up run [42761863772](https://github.com/makr-code/ThemisDB/actions/runs/42761863772) succeeded at SBOM and C5 manifest packaging/validation/upload, but the SOC 2 export failed during root Dockerfile apt package downloads from `security.ubuntu.com` (timeout/404) before vcpkg/Python. It therefore does not verify the prerequisite. SBOM signing and tag-only provenance were skipped, and a successful SOC 2 evidence-export build remains required. An older selected source without this package can still fail during `autoreconf`; rerunning that workflow does not change its source. Use a new release tag containing the fix, or set `build_from_workflow_source=true` for non-publishing validation. See the [operational guidance](../../docs/en/deployment/DOCKER_MULTI_EDITION_BUILD.md#troubleshooting) for details.

## C5 key-lifecycle persisted-event test

`tests/auth/test_c5_key_lifecycle_audit_evidence.cpp` exercises the real `JWTKeyRotationManager::rotateActiveKey()` API with an attached `AuditLogger` and asserts the persisted `KEY_ROTATED` JSONL record. It is registered as `release_critical`; the PR gate uploads its JUnit result and JSONL output, and mainline CTest evidence includes the audit-output directory. This is API-level coverage only: application-runtime logger wiring is unverified, the test has not yet passed CI, and five other required lifecycle event classes have no production emitter. The local configure attempt could not complete because RocksDB development dependencies were unavailable. See the canonical [C5 key lifecycle evidence map](../../audit/evidence/c5/KEY_LIFECYCLE_AUDIT_EVENTS.md).

## Windows dependency snapshot handoff

Run `42638257654`, job `129657233420` failed before compilation because the
completed dependency cache saved by preparation was no longer available on a
failed-job rerun, despite an identical key. The canonical
`.github/workflows/reusable-cmake-build.yml` now uploads the completed snapshot
as a 30-day run artifact; `release-build-matrix.yml` passes its immutable artifact
ID and source/configure/toolchain identity to the consumer. Consumption requires
that artifact and retains binary-only vcpkg installation and identity validation.
Caches accelerate preparation only; partial preparation snapshots are never
published as completed artifacts. Failed-job reruns reuse the original producer's
artifact ID, not the current attempt number. If the artifact expires, rerun
preparation too. Older workflow revisions require a new run containing this fix.
Focused coverage: `tests/test_windows_dependency_preparation.py`.

Run `42768142107`, job `129687972024` downloaded that artifact but failed
restoration because Git Bash's `tar` could not resolve the Windows-form
`GITHUB_WORKSPACE` path passed to `-C`. Snapshot creation and restoration now
normalize the workspace with `cygpath -u`; restoration also ensures the cache
directory exists. The round-trip regression models a Windows-form workspace and
executes real archive/extract commands with spaced paths, while retaining
identity-mismatch and missing-artifact rejection.

Attempt 2 of the same run, [job `129791530083`](https://github.com/makr-code/ThemisDB/actions/runs/42768142107/job/129791530083),
repeated this failure: artifact download succeeded, but the executed workflow
still passed `"$GITHUB_WORKSPACE/.cache"` directly to `tar -C`, which exited with
code 2 (`Cannot open: No such file or directory`). Configuration, compilation,
and CTest were skipped; the subsequent missing CTest evidence is a consequence,
not the root cause. The current workflow already normalizes the workspace and
creates the extraction directory. All 28 Windows dependency preparation tests
passed locally on 2026-11-10, including the archive round trip. Start a new
mainline run from a revision containing the corrected reusable workflow; another
failed-job rerun of this old run does not pick up that workflow fix. Native
Windows validation of the corrected workflow remains pending.

## MSVC monitoring route translation-unit split

Run `42638257654`, job `129343673144` failed with VS18 MSVC 14.51 `C1060` at
`monitoring_api_handler.cpp(2246)` despite serial compilation and
`/Od /Ob0 /GL- /bigobj /Zm200`. Only `MonitoringApiHandler::registerRoutes()`
(all 47 routes unchanged) is moved to `src/server/monitoring_api_routes.cpp`,
registered in both build layouts with the handler's existing MSVC properties.
No API or flag changes; modular non-Debug low-memory defaults and opt-out are
preserved. Follow-up run `42747840520`, job `129662241930` still failed at
`monitoring_api_routes.cpp(155)` with `C1060`. Registration now delegates to eight
file-local helpers (at most ten routes each), bounding per-function nested JSON
initializer pressure while preserving all annotations and registration order.
`tests/test_mainline_compiler_regressions.py` checks the batch bound and dispatch;
`tests/server/test_server_openapi_drift_focused.cpp` covers actual registration,
the nested license schema, and repeat-registration stability.
Native Windows validation remains pending. See
[MSVC Handler Optimization](../../cmake/README.md#msvc-handler-optimization)
for current policy and preserved historical evidence.
## .github/workflows/benchmark-gate.yml
- Kategorie: build_test_ci
- Hash: `59518d7978eca388`
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
- Hash: `44818920cb9d5579`
- Titel: (binary or unreadable)

## .github/workflows/build-clang-fast.yml
- Kategorie: build_test_ci
- Hash: `e3dc5830b7156750`
- Titel: (binary or unreadable)

## .github/workflows/build-content-regression.yml
- Kategorie: build_test_ci
- Hash: `55fe0fa1379b63c3`
- Titel: (binary or unreadable)

## .github/workflows/build-llm-inference.yml
- Kategorie: build_test_ci
- Hash: `4321cbd09bad4a80`
- Titel: (binary or unreadable)

## .github/workflows/build-mainline.yml
- Kategorie: build_test_ci
- Hash: `3db25366a551d209`
- Titel: (binary or unreadable)

## .github/workflows/build-ollama-router.yml
- Kategorie: build_test_ci
- Hash: `0f6f7251a556c299`
- Titel: (binary or unreadable)

## .github/workflows/build-sanitizer-nightly.yml
- Kategorie: build_test_ci
- Hash: `e840eaa2420ff43b`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-a-gpu.yml
- Kategorie: build_test_ci
- Hash: `cee20f4d06d381c3`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-llm-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `06d4bf8f34a54be2`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-transaction.yml
- Kategorie: build_test_ci
- Hash: `475664dc69013385`
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
- Hash: `8ac5e390dff631d7`
- Titel: (binary or unreadable)

## .github/workflows/compliance-supply-chain.yml
- Kategorie: build_test_ci
- Hash: `d873e4f96db3e06c`
- Titel: (binary or unreadable)

## .github/workflows/copilot-code-review.yml
- Kategorie: build_test_ci
- Hash: `67df9c187f101b65`
- Titel: (binary or unreadable)

## .github/workflows/coverage-report-nightly.yml
- Kategorie: build_test_ci
- Hash: `ee69e063159edf1b`
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
- Hash: `b7a1fd48be5e29d8`
- Titel: (binary or unreadable)

## .github/workflows/gate-copilot-regression.yml
- Kategorie: build_test_ci
- Hash: `26b5d8224705f9b9`
- Titel: (binary or unreadable)

## .github/workflows/gate-distributed-knowledge.yml
- Kategorie: build_test_ci
- Hash: `718cedef92c2320a`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-artifact-size.yml
- Kategorie: build_test_ci
- Hash: `9fbb81a4b77620ce`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-benchmark-ctest.yml
- Kategorie: build_test_ci
- Hash: `0de984e6fb1a53a1`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-community-failclosed.yml
- Kategorie: build_test_ci
- Hash: `fa3a0808c501edda`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-core.yml
- Kategorie: build_test_ci
- Hash: `83ded0790348963d`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doc-metadata.yml
- Kategorie: build_test_ci
- Hash: `aa6b2ce8ae57fc0f`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doxygen-governance.yml
- Kategorie: build_test_ci
- Hash: `b52dcb9ed052e38d`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-edition-license.yml
- Kategorie: build_test_ci
- Hash: `32768597ff46e0b5`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-file-size.yml
- Kategorie: build_test_ci
- Hash: `2ea528b8ada5fe2c`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-final-runtime-api.yml
- Kategorie: build_test_ci
- Hash: `c6cb01cff1f5e22b`
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
- Hash: `728026c1c4ca87a5`
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
- Hash: `e5b4384d507be894`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-version.yml
- Kategorie: build_test_ci
- Hash: `21c36c69e408cb90`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-version-targeting.yml
- Kategorie: build_test_ci
- Hash: `718d81eb7f5493ec`
- Titel: (binary or unreadable)

## .github/workflows/gate-release-benchmark-validation.yml
- Kategorie: build_test_ci
- Hash: `22dd32a4b2caa773`
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
- Hash: `a1ee840f4b006e9e`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-architecture-ci.yml
- Kategorie: build_test_ci
- Hash: `202246b32941a015`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-build-issues.yml
- Kategorie: build_test_ci
- Hash: `c89ea426e7bc1d3d`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-ci-health.yml
- Kategorie: build_test_ci
- Hash: `21b4521f4ff35743`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-compendium-sync.yml
- Kategorie: build_test_ci
- Hash: `304d8fff04e2503c`
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
- Hash: `c16b15f57472e274`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-issues.yml
- Kategorie: build_test_ci
- Hash: `3eac9240951febcf`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-pr-failure-diagnosis.yml
- Kategorie: build_test_ci
- Hash: `7d6f105aab762de9`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-soll-ist-gap-issues.yml
- Kategorie: build_test_ci
- Hash: `2d438ffc797ba079`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-workflow-guardrails-observe.yml
- Kategorie: build_test_ci
- Hash: `4ec194cb73d21301`
- Titel: (binary or unreadable)

## .github/workflows/msvc.yml
- Kategorie: build_test_ci
- Hash: `9b0644fce3451072`
- Titel: (binary or unreadable)

## .github/workflows/nightly-governance-audit.yml
- Kategorie: build_test_ci
- Hash: `b090ce842f11bc23`
- Titel: (binary or unreadable)

## .github/workflows/publish-wiki.yml
- Kategorie: build_test_ci
- Hash: `1a0aa680cc4a2620`
- Titel: (binary or unreadable)

## .github/workflows/release-build-matrix.yml
- Kategorie: build_test_ci
- Hash: `48ce185c7fb057b8`
- Titel: (binary or unreadable)

## .github/workflows/release-changelog.yml
- Kategorie: build_test_ci
- Hash: `dfec5228b322e601`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-approval.yml
- Kategorie: build_test_ci
- Hash: `2aef3be51ad35c5c`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-image.yml
- Kategorie: build_test_ci
- Hash: `68ad3b872ce6af1f`
- Titel: (binary or unreadable)

## .github/workflows/release-evidence-chain.yml
- Kategorie: build_test_ci
- Hash: `b4d356065a311585`
- Titel: (binary or unreadable)

## .github/workflows/release-linux-distribution.yml
- Kategorie: build_test_ci
- Hash: `a0e14b32a6e8b350`
- Titel: (binary or unreadable)

## .github/workflows/release-linux-distro-approval.yml
- Kategorie: build_test_ci
- Hash: `53cda23976264669`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline-approval.yml
- Kategorie: build_test_ci
- Hash: `5b6062bbb310ee7c`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline.yml
- Kategorie: build_test_ci
- Hash: `00e1e7d73fa4883b`
- Titel: (binary or unreadable)

## .github/workflows/release-nightly.yml
- Kategorie: build_test_ci
- Hash: `0589e6a3191bf0d3`
- Titel: (binary or unreadable)

## .github/workflows/release-promote.yml
- Kategorie: build_test_ci
- Hash: `73f6461397803be4`
- Titel: (binary or unreadable)

## .github/workflows/release-rollback.yml
- Kategorie: build_test_ci
- Hash: `fa6423b3a69ad992`
- Titel: (binary or unreadable)

## .github/workflows/release-windows-distribution.yml
- Kategorie: build_test_ci
- Hash: `006a726c2df6368c`
- Titel: (binary or unreadable)

## .github/workflows/release-windows-distro-approval.yml
- Kategorie: build_test_ci
- Hash: `66941507e22929b5`
- Titel: (binary or unreadable)

## .github/workflows/release-winget-approval.yml
- Kategorie: build_test_ci
- Hash: `e130a8dbc1b1bc40`
- Titel: (binary or unreadable)

## .github/workflows/release-winget.yml
- Kategorie: build_test_ci
- Hash: `759ff11fe3c8efd0`
- Titel: (binary or unreadable)
