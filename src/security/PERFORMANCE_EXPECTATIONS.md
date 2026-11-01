---
Author: ThemisDB Contributors
Created: unknown
Creation_Date_Note: Original document creation date is not established; no date is inferred.
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: themisdb-doc-orchestrator
Human_Review: false
Human_Author: unknown
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - benchmarks/security/CMakeLists.txt
  - benchmarks/CMakeLists.txt
  - benchmarks/security/bench_security.cpp
  - benchmarks/security/bench_security_release_gates.cpp
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS — src/security

## Scope
- Modul: `src/security`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle fuer die aktuellen Release-Gates ist `benchmarks/security/bench_security_release_gates.cpp`; Phase-2/3-Spezialgates liegen in den fokussierten Security-Benchmarkdateien.

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/security/bench_security_release_gates.cpp`
  - `benchmarks/security/bench_security_phase2_crypto_gates.cpp`
  - `benchmarks/security/bench_security_phase3_policy_gates.cpp`
  - `benchmarks/security/bench_security.cpp` (breitere Modul-Baseline; source exists, but is not registered by the security subdirectory's auto-glob)

`bench_security.cpp` is explicitly skipped by
`benchmarks/security/CMakeLists.txt`'s auto-discovery loop. When benchmarks are
enabled, `benchmarks/CMakeLists.txt` separately registers the source as target
`bench_security`. Thus the source exists and is a broad baseline, but it is not
an auto-registered target from the security subdirectory.

## Harte Release-Gates (aktuell)

| Gate-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| SRG-01 | p99 <= 1 ms | Policy evaluation hot path |
| SRG-02 | p99 <= 500 us | JWT token signature verify |
| SRG-03 | p99 <= 100 us | Key lookup (in-memory) |
| SRG-04 | p99 <= 500 us | Audit write (mock in-memory) |
| SRG-05 | p99 <= 200 us | RBAC permission check |
| SRG-06 | p99 <= 2 ms | Certificate validation overhead |

## Focused hardening benchmark families

| Family | Evidence file | Coverage |
|---|---|---|
| Phase 2 Crypto | `benchmarks/security/bench_security_phase2_crypto_gates.cpp` | key lifecycle, provider failover, crypto error-path guardrails |
| Phase 3 Policy | `benchmarks/security/bench_security_phase3_policy_gates.cpp` | RLS, policy merge, deny-by-default, masking guardrails |
| Release gates | `benchmarks/security/bench_security_release_gates.cpp` | promotion-blocking hot-path latency limits |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.

## Hinweise zur Bewertung

- Die harten `SRG-*`-Grenzen sind die aktuell verbindlichen release-blocking Ziele fuer das Modul.
- Phase-2/3-Benchmarks sind Härtungs- und Regressionsgates; sie ergänzen die Release-Hot-Path-Grenzen statt sie zu ersetzen.
- Wave-C-Produktionsvalidierung (`tests/security/test_security_wavec_production_validation_focused.cpp`) liefert Integrations- und Fehlermatrix-Evidenz, ist aber kein eigener Benchmark-Ersatz.

## Sourcecode Verification (Module: security/performance)

- Gepruefte Benchmark-Quelle:
  - `benchmarks/security/bench_security_release_gates.cpp`
  - `benchmarks/security/bench_security_phase2_crypto_gates.cpp`
  - `benchmarks/security/bench_security_phase3_policy_gates.cpp`
  - `benchmarks/security/bench_security.cpp`
- Gepruefte Target-Registrierung:
  - `benchmarks/security/CMakeLists.txt` excludes `bench_security` from its auto-glob.
  - `benchmarks/CMakeLists.txt` separately creates target `bench_security` when the source exists.
- Gepruefte Ziel-Fall-Zuordnung:
  - `SRG-01..SRG-06` release-gate mappings
  - Phase-2 crypto gate mappings
  - Phase-3 policy gate mappings
- Ergebnis:
  - Die aktuell dokumentierten fokussierten Security-Benchmarkdateien existieren im Repository.
  - Release-Gates bleiben an reproduzierbare Messlaeufe im Release-Profil gebunden.