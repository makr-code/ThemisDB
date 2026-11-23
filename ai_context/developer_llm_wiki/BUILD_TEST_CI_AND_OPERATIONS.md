---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-23"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Developer LLM Wiki — Build/Test/CI/Operations

Datum: 2026-11-23
Status: Active

## macOS ARM64 Accelerate SDK compilation

- Actions job [127417305713](https://github.com/makr-code/ThemisDB/actions/runs/42075920970/job/127417305713)
  failed in GGML Metal, CPU, and BLAS compilation. The Metal `<memory>`
  force-include is already scoped to that target's C++ sources.
- `cmake/Dependencies.cmake` probes Homebrew Clang's
  `elaborated-enum-base` diagnostic and demotes only that Xcode 15.4 Accelerate
  header diagnostic for existing `ggml-cpu` and `ggml-blas` C++ targets.
  BLAS does not require the CPU target to be present. C sources, consumers,
  non-Apple platforms, and unrelated warning-as-error checks are unchanged.
- `tests/cmake/ggml_portability` and the backend tests in
  `tests/test_deployment_workflow_regressions.py` verify target/language scope,
  compiler probing, missing targets, and negative compilation controls.
  Local fixture success does not establish a successful macOS release run.

## PR gate scope and large campaigns (2026-11-23)

- `gate-pr-core.yml` retains one required release-critical build/test path; its
  duplicate quality build was removed. Registration and WAL-report contracts
  remain fast Python preflight checks.
- `gate-pr-merge-readiness.yml` runs the Final Runtime API gate once. It is no
  longer duplicated inside `reusable-pr-build-components-gate.yml`.
- `build-sanitizer-nightly.yml` and `build-wave-b-transaction.yml` run on their
  schedules or by manual dispatch, not on PRs. Their sanitizer, chaos, and
  recovery evidence remains separate from per-PR results.
- `build-mainline.yml` remains scheduled/manual for platform/runtime builds and
  calls `reusable-mainline-validation-gates.yml` for Linux CTest, benchmarks,
  and macOS kqueue tests. It does not execute CTest or benchmarks inline.
  Scheduled success is not a result for an individual PR commit.
- Local YAML, policy, and workflow-contract checks do not prove GitHub Actions
  runtime or runner-minute savings; compare post-change job durations after
  successful scheduled and PR runs.

## DIS-10 real-transport process recovery

- `.github/workflows/build-wave-b-transaction.yml` builds the Linux-only
  `test_dis10_real_transport_process_recovery` target and selects its
  `DIS-11`-labeled CTest registration for three repeats with the chaos/recovery
  artifact. The registration remains named
  `DIS10RealTransportProcessRecoveryTests`.
- The target covers gRPC/mTLS participant processes, WAL recovery, directional
  TCP loss, bounded request latency, and crash/recovery transactions with three,
  four, and five participants. The five-participant transaction runs across six
  processes including the coordinator. It also injects application-level
  duplicate COMMIT/reordered ABORT RPCs; encrypted TCP/TLS bytes are not altered.
  Its WAL history assertions are
  terminal-outcome consistency checks, not a general serializability checker.
- The 11-case registered target passed once and in three repeated CTest runs in
  Ubuntu 24.04/Linux Debug. This local result does not verify a GitHub Actions
  run/artifact, Windows/macOS transport, 72-hour chaos, or release-platform
  evidence.
- The same process target is the bounded DIS-11 test suite, not a second or
  simulated transport. Select it with
  `ctest --test-dir <build-dir> -L '^DIS-11$' --output-on-failure`. On
  2026-11-23, the strict target build and
  `ctest --test-dir /tmp/themis-dis11-network -L '^DIS-11$' --repeat until-fail:3
  --output-on-failure` passed all three runs in Ubuntu 24.04/Linux Debug. This
  local result is separate from workflow, platform, and maintainer evidence.
- The workflow integration is not a passing CI result until a run URL and
  uploaded artifact are verified. It provides no Windows/macOS process
  coverage or 72-hour chaos evidence.
- `build-wave-b-transaction.yml` now executes the DIS-11 CTest label three
  separate times with `--no-tests=error`, writes one JUnit file per run, and
  validates all three results with `tools/ci/validate_dis11_evidence.py`.
  Artifact collection rejects missing, failed, skipped, wrong-test, or
  commit-mismatched DIS-11 evidence.
- The portable `DIS10ConcurrentSerializabilityTests` target overlaps four
  transactions across two durable participant objects, records read/write
  events, checks a conflict-precedence graph, and requires a serial replay that
  matches the observed reads. A synthetic opposing-order fixture checks cycle
  rejection. Wave B repeats the test and validates its JSON artifact against
  the commit and workflow run ID. The release matrix runs and uploads this
  focused in-process test on Linux AMD64/ARM64, Windows AMD64, and macOS ARM64;
  those lanes do not establish cross-platform gRPC/process transport.
- The portable target passed three repeated local CTest runs on 2026-11-22.
  This is in-process Linux validation only; successful CI artifacts and
  Windows/macOS release-platform lanes remain open.
- A recent Wave B PR run
  [45059649533](https://github.com/makr-code/ThemisDB/actions/runs/45059649533)
  ended `action_required` with zero jobs. No successful workflow artifact or
  release-platform test pass has yet been verified.

## Config and package version integrity gate

- Canonical CI entry point: [`.github/workflows/test-helm-chart.yml`](../../.github/workflows/test-helm-chart.yml).
- Config and supported package metadata changes run the fail-closed checker at [`scripts/check_config_version_integrity.py`](../../scripts/check_config_version_integrity.py); its focused regressions are in [`tests/test_config_version_integrity.py`](../../tests/test_config_version_integrity.py).
- The checker lints shipped TSA, RAID, and replication examples plus production YAML profiles. It requires TSA certificate verification, TLS/mTLS for externally bound services and remote replication, and rejects external plaintext HTTP endpoints.
- The version matrix compares supported package metadata with root `VERSION`; Debian CPack is derived from `VERSION` and `RELEASE_TYPE`. Any mismatch fails the gate; no implicit package-version exception is applied.
- CI uploads `config-lint.txt` and `version-matrix.json` as the `config-version-integrity-<run-id>` artifact. Helm chart changes independently run chart lint, render tests, and the Kind smoke test.

## LLM real-model CI lane

- The scheduled/manual `.github/workflows/build-llm-inference.yml` lane prepares TinyLlama through the reusable model cache and the SHA256-pinned downloader.
- The verified model is passed to the inference, embedding, LoRA adapter, RAG, AdaLoRA, and Wiki Phase B suites. Each test invocation enforces an `llm` skip budget of zero, and the final report fails if any required suite is skipped or unsuccessful.
- Focused binaries and the RAG golden dataset are transferred under `build/bin/tests_by_module/llm/Release`; the generated LLM CTest file is restored under `build/cmake/tests/llm`. Suite jobs run CTest from that directory and emit GoogleTest output so the zero-skip checker can verify actual test execution.
- A passing run is evidence only for those executed suites; it does not establish Vision/Whisper coverage, the repository-wide `GTEST_SKIP` target, or a successful run until workflow evidence exists.

## Gate workflow failure-tolerance guard

- [`tools/ci/check_workflow_fail_open.py`](../../tools/ci/check_workflow_fail_open.py) scans gate workflows and their transitively invoked local reusable workflows for `continue-on-error` and `|| true`.
- Each remaining exception is matched exactly in [`tools/ci/workflow_fail_open_allowlist.json`](../../tools/ci/workflow_fail_open_allowlist.json) and requires a rationale, owner, expiry, and occurrence count. Gate callers that enable reusable `*_continue_on_error` inputs are also inventoried.
- Focused regressions: [`tools/tests/test_workflow_fail_open.py`](../../tools/tests/test_workflow_fail_open.py). This policy check does not itself provide platform gate-run evidence or maintainer sign-off.

## Platform-independent release artifacts

- [`release-build-matrix.yml`](../../.github/workflows/release-build-matrix.yml) validates packages per selected OS/architecture lane. If a lane fails before producing its package artifact, the runtime gate skips that lane rather than treating the missing package as a validation failure.
- `release-build-matrix.yml` defaults to `run_ctest: true` for existing release callers. `build-mainline.yml` sets `run_ctest: false` and runs CTest/benchmarks only through its separate reusable validation gates.
- Mainline packaging and nightly GitHub Release publication may proceed when at least one package exists and every produced package passes the runtime gate. A runtime validation failure still blocks publication, and a release with no distributable package fails closed.
- A partial platform release does not hide the failed build lane: the aggregate matrix and final release status remain failed. Nightly Docker manifest promotion still requires both validated `amd64` and `arm64` image digests.
## .github/workflows/benchmark-gate.yml
- Kategorie: build_test_ci
- Hash: `ff58415b236088a7`
- Titel: (binary or unreadable)

## .github/workflows/benchmark-performance-gate.yml
- Kategorie: build_test_ci
- Hash: `8e6a4182f3878296`
- Titel: (binary or unreadable)

## .github/workflows/benchmark-thematic-suites.yml
- Kategorie: build_test_ci
- Hash: `4924c5315f997ad1`
- Titel: (binary or unreadable)

## .github/workflows/build-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `913e742004afdfc2`
- Titel: (binary or unreadable)

## .github/workflows/build-clang-fast.yml
- Kategorie: build_test_ci
- Hash: `7d5985d376be02c2`
- Titel: (binary or unreadable)

## .github/workflows/build-content-regression.yml
- Kategorie: build_test_ci
- Hash: `55fe0fa1379b63c3`
- Titel: (binary or unreadable)

## .github/workflows/build-llm-inference.yml
- Kategorie: build_test_ci
- Hash: `f5cc675a0dda3302`
- Titel: (binary or unreadable)

## .github/workflows/build-mainline.yml
- Kategorie: build_test_ci
- Hash: `b080ee84d2ad128b`
- Titel: (binary or unreadable)

## .github/workflows/build-ollama-router.yml
- Kategorie: build_test_ci
- Hash: `0f6f7251a556c299`
- Titel: (binary or unreadable)

## .github/workflows/build-sanitizer-nightly.yml
- Kategorie: build_test_ci
- Hash: `ed65f0eca6070be6`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-a-gpu.yml
- Kategorie: build_test_ci
- Hash: `904fdcb9fe301ca7`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-llm-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `06d4bf8f34a54be2`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-transaction.yml
- Kategorie: build_test_ci
- Hash: `bf79b0ee296c0336`
- Titel: (binary or unreadable)

## .github/workflows/build-widget.yml
- Kategorie: build_test_ci
- Hash: `6289f57417da9f3e`
- Titel: (binary or unreadable)

## .github/workflows/codeql.yml
- Kategorie: build_test_ci
- Hash: `e578581a55abf58d`
- Titel: (binary or unreadable)

## .github/workflows/compliance-governance-gates.yml
- Kategorie: build_test_ci
- Hash: `beb84fc18b4711d6`
- Titel: (binary or unreadable)

## .github/workflows/compliance-supply-chain.yml
- Kategorie: build_test_ci
- Hash: `73a5549755905006`
- Titel: (binary or unreadable)

## .github/workflows/copilot-code-review.yml
- Kategorie: build_test_ci
- Hash: `67df9c187f101b65`
- Titel: (binary or unreadable)

## .github/workflows/coverage-report-nightly.yml
- Kategorie: build_test_ci
- Hash: `7fbf3390118b7219`
- Titel: (binary or unreadable)

## .github/workflows/crda.yml
- Kategorie: build_test_ci
- Hash: `39624fa4a8b31ebd`
- Titel: (binary or unreadable)

## .github/workflows/edition-hyperscaler-ci.yml
- Kategorie: build_test_ci
- Hash: `04ea5d998e55b00f`
- Titel: (binary or unreadable)

## .github/workflows/ga-promotion-signoff.yml
- Kategorie: build_test_ci
- Hash: `12754aec171561e4`
- Titel: (binary or unreadable)

## .github/workflows/gate-copilot-regression.yml
- Kategorie: build_test_ci
- Hash: `fcd22f67a2a2b916`
- Titel: (binary or unreadable)

## .github/workflows/gate-distributed-knowledge.yml
- Kategorie: build_test_ci
- Hash: `da815ebe2fd31684`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-artifact-size.yml
- Kategorie: build_test_ci
- Hash: `4cf20be878c11a7d`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-benchmark-ctest.yml
- Kategorie: build_test_ci
- Hash: `cae2121c15ac636a`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-community-failclosed.yml
- Kategorie: build_test_ci
- Hash: `7514579b278e50e6`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-core.yml
- Kategorie: build_test_ci
- Hash: `2868abbfc13dd19b`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doc-metadata.yml
- Kategorie: build_test_ci
- Hash: `ca290ddda4457344`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doxygen-governance.yml
- Kategorie: build_test_ci
- Hash: `3e86708c58c4eccb`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-edition-license.yml
- Kategorie: build_test_ci
- Hash: `8c508978f04664d7`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-file-size.yml
- Kategorie: build_test_ci
- Hash: `2ea528b8ada5fe2c`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-final-runtime-api.yml
- Kategorie: build_test_ci
- Hash: `a2ccf0ac464fb452`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-hash-sbom.yml
- Kategorie: build_test_ci
- Hash: `32e1915ebe79c5b8`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-merge-readiness.yml
- Kategorie: build_test_ci
- Hash: `da04d82813e21ea2`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-module-doxygen-xml.yml
- Kategorie: build_test_ci
- Hash: `eed6be4c50b24f77`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-plugin-boundary.yml
- Kategorie: build_test_ci
- Hash: `893726259c7569a2`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-primary-doc-structure.yml
- Kategorie: build_test_ci
- Hash: `c958a6779c21a03a`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-eval.yml
- Kategorie: build_test_ci
- Hash: `40775771c777a109`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase10.yml
- Kategorie: build_test_ci
- Hash: `c026f3a21a714495`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase7.yml
- Kategorie: build_test_ci
- Hash: `c56513ae1f6df7d9`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase8.yml
- Kategorie: build_test_ci
- Hash: `2d36ad77b92e4ef3`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase9.yml
- Kategorie: build_test_ci
- Hash: `556d618b16f14bfe`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-security.yml
- Kategorie: build_test_ci
- Hash: `118f656434a9d21f`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-version.yml
- Kategorie: build_test_ci
- Hash: `d3925741c9c25dc4`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-version-targeting.yml
- Kategorie: build_test_ci
- Hash: `75b4fe420b06a5a3`
- Titel: (binary or unreadable)

## .github/workflows/gate-release-benchmark-validation.yml
- Kategorie: build_test_ci
- Hash: `1fccc8ad4a941593`
- Titel: (binary or unreadable)

## .github/workflows/gate-wave-closure.yml
- Kategorie: build_test_ci
- Hash: `75a93752df76c2d2`
- Titel: (binary or unreadable)

## .github/workflows/gate-workflow-guidelines.yml
- Kategorie: build_test_ci
- Hash: `ab48765994565c25`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-ai-working.yml
- Kategorie: build_test_ci
- Hash: `d640d7edd4a934ff`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-architecture-ci.yml
- Kategorie: build_test_ci
- Hash: `a0d13a7f0ed1f3db`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-build-issues.yml
- Kategorie: build_test_ci
- Hash: `85c3af80afb63a72`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-ci-health.yml
- Kategorie: build_test_ci
- Hash: `32405ec99fc880ca`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-compendium-sync.yml
- Kategorie: build_test_ci
- Hash: `2dae37d283b05287`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-docs-db-build.yml
- Kategorie: build_test_ci
- Hash: `c902cb537996f423`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-docs.yml
- Kategorie: build_test_ci
- Hash: `ce164cf7a95894ee`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-housekeeping.yml
- Kategorie: build_test_ci
- Hash: `e0f55d4d8cbb60e0`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-issues.yml
- Kategorie: build_test_ci
- Hash: `0283193cb8979091`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-pr-failure-diagnosis.yml
- Kategorie: build_test_ci
- Hash: `c370e662f1a2a7aa`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-soll-ist-gap-issues.yml
- Kategorie: build_test_ci
- Hash: `11e52c2b47b31d1c`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-workflow-guardrails-observe.yml
- Kategorie: build_test_ci
- Hash: `583f638ec15bf4a5`
- Titel: (binary or unreadable)

## .github/workflows/msvc.yml
- Kategorie: build_test_ci
- Hash: `09fa053e07e96668`
- Titel: (binary or unreadable)

## .github/workflows/nightly-governance-audit.yml
- Kategorie: build_test_ci
- Hash: `29d8c7f0b32a2565`
- Titel: (binary or unreadable)

## .github/workflows/publish-wiki.yml
- Kategorie: build_test_ci
- Hash: `a401e774bbe005b4`
- Titel: (binary or unreadable)

## .github/workflows/release-build-matrix.yml
- Kategorie: build_test_ci
- Hash: `878efca4ec2d080f`
- Titel: (binary or unreadable)

## .github/workflows/release-changelog.yml
- Kategorie: build_test_ci
- Hash: `dfec5228b322e601`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-approval.yml
- Kategorie: build_test_ci
- Hash: `95e888ff0d589849`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-image.yml
- Kategorie: build_test_ci
- Hash: `f9ad6daeb2c1a6a4`
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
- Hash: `970d76fb7ecbb48e`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline-approval.yml
- Kategorie: build_test_ci
- Hash: `a00cac6dd0110921`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline.yml
- Kategorie: build_test_ci
- Hash: `cedb6f723ecc52b5`
- Titel: (binary or unreadable)

## .github/workflows/release-nightly.yml
- Kategorie: build_test_ci
- Hash: `1dfd2df4871ae65e`
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
- Hash: `773f6bf1c474b91e`
- Titel: (binary or unreadable)

## .github/workflows/release-winget-approval.yml
- Kategorie: build_test_ci
- Hash: `8ba6dea6be6f9de6`
- Titel: (binary or unreadable)

## .github/workflows/release-winget.yml
- Kategorie: build_test_ci
- Hash: `930ab8ec4a9bfa90`
- Titel: (binary or unreadable)
