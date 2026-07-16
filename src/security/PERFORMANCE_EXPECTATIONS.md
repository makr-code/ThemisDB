# PERFORMANCE_EXPECTATIONS — src/security

## Scope
- Modul: `src/security`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_security.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| SEC-1 | Siehe Zielbeschreibung: AES-256-GCM (AES-NI) | `BM_AES256GCM_Encrypt_1MB` |
| SEC-2 | Siehe Zielbeschreibung: AES-256-GCM medium payload latency | `BM_AES256GCM_Encrypt_64KB` |
| SEC-3 | Siehe Zielbeschreibung: Kyber-1024 Key Encapsulation | `BM_PostQuantum_KyberKeyGen_1024` |
| SEC-4 | Siehe Zielbeschreibung: Dilithium-5 Signing | `BM_PostQuantum_DilithiumSign_5` |
| SEC-5 | Siehe Zielbeschreibung: RBAC Role-Hierarchy validation latency | `BM_RBAC_RoleHierarchyValidation` |
| SEC-6 | Siehe Zielbeschreibung: RBAC Policy Eval (100 Rollen) P99 | `BM_RBAC_PermissionCheck_ManyRoles` |
| SEC-7 | Siehe Zielbeschreibung: AQL injection detection overhead | `BM_AQLInjection_MaliciousQuery` |
| SEC-8 | Siehe Zielbeschreibung: Audit Log Write P99 | `BM_AuditLog_TamperEvidentAppend` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| SECG-1 | >= 600 MB/s (AES-256-GCM Encrypt Throughput) | mean aus `BM_AES256GCM_Encrypt_1MB` |
| SECG-2 | <= 45 ms (TLS/Audit kritische P99-Latenz) | p99 aus `BM_AES256GCM_Encrypt_64KB` und `BM_FieldEncryption_SmallDocument` |
| SECG-3 | <= 20 ms (RBAC Evaluation P95) | p95 aus `BM_RBAC_PermissionCheck_ManyRoles` |
| SECG-4 | Regression <= 7 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.

## Numerische Mindestziele (Release Gate)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| NG-1 Latenz P95 | <= 50 ms | p95 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-2 Latenz P99 | <= 100 ms | p99 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-3 Throughput-Stabilitaet | Regression <= 10 % gegen letzte Baseline | `(current - baseline) / baseline` |

Hinweis:
- Diese Mindestziele gelten als moduluebergreifende Release-Grenzen solange kein strengeres, modulspezifisches Ziel hinterlegt ist.
- Bei `proxy` oder `not_measurable` bleibt das Ziel numerisch gueltig, wird aber ueber den dokumentierten Proxy-Pfad verifiziert.

## Sourcecode Verification (Module: security/performance)

- Gepruefte Benchmark-Quelle:
  - `benchmarks/bench_security.cpp`
- Gepruefte Ziel-Fall-Zuordnung:
  - AES-GCM throughput/latency (`BM_AES256GCM_*`)
  - RBAC policy and hierarchy checks (`BM_RBAC_*`)
  - Post-quantum keygen/sign operations (`BM_PostQuantum_*`)
  - Injection detection overhead (`BM_AQLInjection_*`)
  - Tamper-evident audit append performance (`BM_AuditLog_*`)
- Ergebnis:
  - Alle referenzierten Benchmark-Faelle existieren im aktuellen Benchmark-Source.
  - Release-Gates bleiben an reproduzierbare Messlaeufe im Release-Profil gebunden.