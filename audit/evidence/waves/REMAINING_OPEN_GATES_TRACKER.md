---
Author: ThemisDB Maintainers
Created: 2026-09-15
Last Updated: 2026-11-09
Status: active
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---
# Remaining Open Gates Tracker (Wave A→D)

Status: active
Scope: `develop`
Last Updated: 2026-11-09
Purpose: track unresolved benchmark/closure evidence that remains explicitly open in manifests.

## Wave A revalidation (2026-11-09)

Observed `develop` HEAD: `b55c75f19d48c830c298933b734ff5bec71213d5`. Available lane runs below are older than that HEAD and are recorded as attempts, not passes. The Wave A exit manifest contains the full machine-readable lane details.

| Lane / gate | Current state | Latest available run and commit | Revalidation blocker |
|---|---|---|---|
| `A_exit_release_critical_green` | `pending` | [Current run 42638110351](https://github.com/makr-code/ThemisDB/actions/runs/42638110351), `b55c75f19d48c830c298933b734ff5bec71213d5` — queued | Latest completed predecessor [42635125188](https://github.com/makr-code/ThemisDB/actions/runs/42635125188), `fc1dbd540541fd4616786b923621a6fe37644e9c`, failed and skipped the release-critical test job |
| Transaction | `pending` | [42562739998](https://github.com/makr-code/ThemisDB/actions/runs/42562739998), `b3634fc907b6fdfe29b163112b01f577162010ec` — workflow success | Phase-4 baseline, full suite, and chaos jobs skipped; `benchmarks/wave8/TRANSACTION_BASELINES_LATEST.json` absent |
| GPU `A4_representative_hardware` | `skipped` | [42559714050](https://github.com/makr-code/ThemisDB/actions/runs/42559714050), `8550ebc98499db562aa975e176c19d12745540af` — workflow failure | GPU test compilation failed; manual-only representative job was not enabled, so runner availability is unverified |
| Voice | `pending` | [42441375803](https://github.com/makr-code/ThemisDB/actions/runs/42441375803), `74f899e4d44f44bae43b07f7c799a19580debe9d` — workflow failure; voice job succeeded | Hosted CPU benchmark only; no representative-hardware p95/p99 artifact |
| Sharding | `pending` | [42441375803](https://github.com/makr-code/ThemisDB/actions/runs/42441375803), `74f899e4d44f44bae43b07f7c799a19580debe9d` — workflow failure | Wave A flow failed before benchmarks; release-gate job skipped |
| Replication | `pending` | [42441375803](https://github.com/makr-code/ThemisDB/actions/runs/42441375803), `74f899e4d44f44bae43b07f7c799a19580debe9d` — workflow failure | No replication-specific representative run or current artifact; shared sharding/transaction flow failed |

No fresh representative-hardware metrics or CPU-fallback comparison were produced. Keep every missing lane pending/skipped, and do not promote root-roadmap status until fresh evidence and the Wave A validator pass.

## Historical status changes (last recorded 2026-09-15)

| Manifest | Gate | Previous status | Resolution |
|---|---|---|---|
| `manifests/wave_b_module_hardening_closure_manifest.json` | `B_modules_rag` | `pending` | ✅ → `success` — RAG Phase 5-6 complete (270+ tests, 4 benchmarks, src/rag/ROADMAP.md Wave B exit ✅ SATISFIED 2026-08-18) |
| `manifests/wave_b_module_hardening_closure_manifest.json` | `B_modules_updates` | `pending` | ✅ → `success` — v1.1.0 GA; UPDP-4..7 PASS; src/updates/MODULE_EVIDENCE.md source-backed |
| `manifests/security_wave_c_closure_manifest.json` | `C2_fail_closed_boundary` | `in_progress` | ✅ → `success` — Vault/HSM/PKI fail-closed matrix tests PASS 2026-08-17; HSM stub opt-in enforced |
| `manifests/operability_wave_d_closure_manifest.json` | `D1_runbooks` | `in_progress` | ✅ → `success` — 5 runbooks delivered; GA D-1 PASS 2026-08-04 |
| `manifests/transaction_wave_b_closure_manifest.json` | `B4_phase4_baseline` | `skipped` | Previously reported `success`; reopened 2026-11-09 because the latest Phase-4 baseline job was skipped, the referenced file is absent, and no baseline artifact was uploaded |

## Advanced since last update (2026-09-15)

| Manifest | Gate | Previous status | New status | Action |
|---|---|---|---|---|
| `manifests/wave_b_module_hardening_closure_manifest.json` | `B_modules_llm_wiki` | `pending` | `in_progress` | LLM Wiki Phase 3-4 complete; Wave B closure bundle present |
| `manifests/wave_b_module_hardening_closure_manifest.json` | `B_modules_llm` | `pending` | `in_progress` | LLM Wiki Phase A+B complete; distributed hardening in_progress |
| `manifests/wave_ab_gap_closure_manifest.json` | `G4_unapproved_stub_mock_sim_blockers` | `pending` | `in_progress` | HSM default stub removed; remaining paths under audit |
| `manifests/security_wave_c_closure_manifest.json` | `C4_compliance_signoff_bundle` | `pending` | `in_progress` | Sanitizer + pentest evidence closed; final assembly + §9 human sign-off pending |
| `manifests/operability_wave_d_closure_manifest.json` | `D2_observability_gates` | `pending` | `in_progress` | Phase 2A/2B/2C verification docs present |

## Open gates needing further evidence

| Manifest | Gate | Current status | Missing evidence / action |
|---|---|---|---|
| `manifests/gpu_wave_a_closure_manifest.json` | `A4_representative_hardware` | `skipped` | Run 42559714050 skipped the manual-only `self-hosted,gpu-cuda` job because its input was not enabled; runner availability is unverified; no p95/p99 artifact |
| `manifests/wave_a_exit_closure_manifest.json` | `A_exit_rep_hardware_p95_p99` | `pending` | Transaction, GPU, voice, sharding, and replication lane references and individual blockers are recorded in the exit manifest; no lane has current representative-hardware metrics |
| `manifests/wave_a_exit_closure_manifest.json` | `A_exit_transaction_gpu_artifacts` | `in_progress` | Transaction Phase-4 baseline and GPU representative hardware evidence are missing; latest lane run references are behind observed `develop` HEAD |
| `manifests/wave_b_module_hardening_closure_manifest.json` | `B_modules_query` | `in_progress` | Representative-hardware baseline + cross-feature integration tests |
| `manifests/wave_b_module_hardening_closure_manifest.json` | `B_modules_llm_wiki` | `in_progress` | Wiki routing cost-signal, security/governance runtime gates, policy-loader evidence |
| `manifests/wave_b_module_hardening_closure_manifest.json` | `B_modules_llm` | `in_progress` | Distributed collectives, multi-tenant isolation, speculative/TARG cross-module wiring |
| `manifests/wave_b_module_hardening_closure_manifest.json` | `B_modules_acceleration` | `pending` | Result validation hardening (320 gaps), Phase B advisory-only evidence |
| `manifests/wave_b_module_hardening_closure_manifest.json` | `B_modules_index` | `pending` | Buffer lifecycle RAII, concurrency ThreadSanitizer, ANN+GPU validation |
| `manifests/wave_b_integration_closure_manifest.json` | `B_integration_query_index_storage` | `in_progress` | Cross-module Query↔Index↔Storage integration evidence package |
| `manifests/wave_b_integration_closure_manifest.json` | `B_integration_llm_rag_llm_wiki` | `in_progress` | Cross-chain integration test execution + representative-hardware baseline |
| `manifests/wave_b_integration_closure_manifest.json` | `B_integration_rep_hardware_refresh` | `pending` | Representative-hardware baseline refresh for both integration chains |
| `manifests/wave_ab_gap_closure_manifest.json` | `G2_security_critical_real_gaps` | `in_progress` | Close remaining 41 real security gaps (audit/MARKER_GAP_CLASSIFICATION_2026-08-31.md) |
| `manifests/wave_ab_gap_closure_manifest.json` | `G3_runtime_critical_real_gaps` | `in_progress` | Close chimera(38), llama_cpp(26), tensor(25) real runtime-critical gaps |
| `manifests/wave_ab_gap_closure_manifest.json` | `G4_unapproved_stub_mock_sim_blockers` | `in_progress` | Complete audit of remaining unapproved non-production paths |
| `manifests/security_wave_c_closure_manifest.json` | `C3_sustained_load_evidence` | `pending` | Sustained-load security evidence on representative scope |
| `manifests/security_wave_c_closure_manifest.json` | `C4_compliance_signoff_bundle` | `in_progress` | Final compliance sign-off bundle assembly + §9 human sign-off |
| `manifests/operability_wave_d_closure_manifest.json` | `D2_observability_gates` | `in_progress` | Observability gate sign-off from final execution evidence (Phase 2A/2B/2C) |
| `manifests/operability_wave_d_closure_manifest.json` | `D3_soak_recovery` | `in_progress` | 4 soak tests (24–48h each) scheduled Q1 2027; execution TBD |
| `manifests/operability_wave_d_closure_manifest.json` | `D4_operator_signoff` | `pending` | Human operator readiness sign-off — GA_PROMOTION_SIGN_OFF.md §9 |

## Evidence-fill cadence

1. Update manifest(s) with latest real run IDs and artifact references.
2. Keep unresolved gates explicitly `pending` / `in_progress` / `skipped`.
3. Run wave validator (`gate-wave-closure.yml` or local validator script).
4. Sync root status only after PASS.
