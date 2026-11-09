---
Author: "ThemisDB Team"
Created: "2026-11-09"
Last Updated: "2026-11-09"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Marker gap classification — 2026-11-09 source triage

## Scope and source precedence

This is a current triage checkpoint for the commit-pinned raw inventory in [`MARKER_LOCATIONS_2026-11-09.md`](MARKER_LOCATIONS_2026-11-09.md). The inventory contains 131 marker-bearing source lines, but marker text is not proof of a runtime gap. The source review below classifies only examples whose activation or behavior was verified; it does **not** claim a complete line-by-line classification or promote Wave AB status.

Canonical release posture remains in [`ROADMAP.md`](../ROADMAP.md). The closure requirements are in [`evidence/waves/WAVE_CLOSURE_BACKLOG.md`](evidence/waves/WAVE_CLOSURE_BACKLOG.md), §§75–99.

## Source-verified classifications

| Candidate | Classification | Evidence and disposition |
|---|---|---|
| No-OpenSSL `TimestampAuthority` fallback | **Source-verified security/runtime gap at the scanned develop commit** | `src/security/timestamp_authority.cpp:150-153,181-183,326-328,454-461` allows `THEMIS_ALLOW_TSA_STUB=1` to override production-mode rejection or select permissive eIDAS validation. `tests/test_timestamp_authority.cpp:590-604` encoded success in production as expected behavior. This work package hardens the production guard and adds focused regression coverage; keep G2/G4 open until that change and its tests are verified. Owner: open audit work package [#7814](https://github.com/makr-code/ThemisDB/issues/7814). |
| HSM providers | **Source-verified security-critical fail-open verification paths; remediation in progress** | The software-only branch in `src/security/hsm_provider.cpp` rejects canonical `ProductionMode` during initialization, but `verify()` can accept its deterministic pseudo-signature without requiring successful initialization or non-production mode. The `THEMIS_ENABLE_HSM_REAL` branch in `src/security/hsm_provider_pkcs11.cpp` can initialize successfully after PKCS#11 is unavailable or no usable key is found, and its software verification fallback accepts `Base64(SHA-256(data))`; software crypto fallback routes also require explicit production-mode review. Focused changes are underway; do not count them as closed until the final diff and regression behavior are verified. Owner: open audit work package [#7814](https://github.com/makr-code/ThemisDB/issues/7814). |
| `FieldEncryption::createDefault()` test-build path | **Potential release-visible mock-key path; remediation/evidence in progress** | `src/security/field_encryption.cpp:368-378` selects `MockKeyProvider` when `THEMIS_TEST_BUILD` is defined. The test-build definition is applied by the CMake test configuration, including a Release preset with tests enabled. Verify the resulting library/factory behavior and require an explicit test-only activation rather than assuming test-enabled builds are production-safe. Owner: open audit work package [#7814](https://github.com/makr-code/ThemisDB/issues/7814). |
| llama.cpp synthetic response mode | **Test seam in focused targets; no production no-model fallback found** | `src/llama_cpp/llama_cpp_plugin.cpp:383-422` puts synthetic generation behind `THEMIS_LLAMA_CPP_STUB_MODE`; without that definition, an unloaded model returns `success=false`, and the registrar's no-model reload path fails. The focused test targets define the macro in `tests/llama_cpp/CMakeLists.txt`. The macro match count is not a production gap count; open issue #7526 covers test-depth evidence. |
| Tensor encoder/repository paths | **Production-reachable degraded/approximate behavior; semantic acceptance remains open** | `src/tensor/utr_converter.cpp` uses lexical encoding when model-backed encoding is unavailable, and `src/tensor/adapter_repository.cpp` includes heap deserialization and fingerprint-graph similarity paths. These are active, non-empty implementations rather than empty stubs, but their output quality, caller expectations, and acceptable production conditions have not been established. Keep runtime suitability under #7814; do not treat closed issue #5301 or old marker counts as acceptance evidence. |
| Remaining current marker lines | **Unclassified** | The raw scan includes comments, literals, compile-time branches, test references, and implementation markers. No source/test review sufficient to classify every candidate was completed in this pass. They remain open for the required candidate-by-candidate review. |

## Historical example reconciliation

| Module | Historical snapshot | Current source evidence | Issue/work-package state |
|---|---|---|---|
| `security` | 41 real gaps in the 2026-08-31 classification; later snapshots also predate this scan | The exact commit-pinned inventory command reproduces 46 raw marker-bearing lines in the current `src/security/` scope; these are not 46 findings. TSA and HSM fail-open verification behavior are source-verified at the scanned commit. | Security policy work #7496 is closed via merged PR #7803; Wave C evidence #7812 remains open but is not code remediation for these findings. #7814 remains the current Wave AB audit owner. |
| `chimera` | 38 real gaps in the 2026-08-31 classification | No marker hits in the current `src/chimera/` scan. This does not prove that the module has no runtime fallback or integration gap. | Historical gap issue #5264 is closed as `not_planned`; its old count is not a current disposition. Reconcile runtime/build behavior under #7814 before closing G3. |
| `llama_cpp` | 26 real gaps in the 2026-08-31 classification and 26 STUB/MOCK markers on 2026-10-01 | 19 raw marker lines in this snapshot, including test-only macro uses. The production no-model path returns failure when stub mode is not compiled. | Historical gap issue #5281 is closed as `not_planned`. Open issue #7526 covers llama.cpp test depth, not closure of every runtime marker. |
| `tensor` | 25 real gaps in the 2026-08-31 classification and 4 STUB/MOCK markers on 2026-10-01 | 4 raw marker lines in this snapshot; current encoder and repository fallbacks require behavior-level review. | Historical gap issue #5301 is closed as `not_planned`; do not treat it or the old count as current closure evidence. Track unresolved triage in #7814 or a bounded owner issue. |

## Closure state

- Candidate-by-candidate classification: **in progress**; raw counts are not closure evidence.
- G2 security-critical gaps: **in progress**; TSA and HSM production-mode/fail-open verification paths are source-verified. TSA remediation is present but its build/test execution was unavailable; HSM remediation is not yet verified.
- G3 runtime-critical gaps: **in progress**; llama.cpp's synthetic mode is test-target-only in the reviewed configuration, while tensor has active degraded/approximate production behavior whose suitability is unresolved.
- G4 unapproved non-production paths: **in progress**; `FieldEncryption::createDefault()` selects a mock provider under `THEMIS_TEST_BUILD`; complete activation, fail-closed, and approval evidence is still required across candidates.
- No root `ROADMAP.md` or release status is promoted by this audit checkpoint.
