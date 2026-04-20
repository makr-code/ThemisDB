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
| SEC-2 | Siehe Zielbeschreibung: RSA-4096 Signaturprüfung P99 | `BM_RBAC_PermissionCheck_ManyRoles` |
| SEC-3 | Siehe Zielbeschreibung: Kyber-1024 Key Encapsulation | `BM_PostQuantum_KyberKeyGen_1024` |
| SEC-4 | Siehe Zielbeschreibung: Dilithium-5 Signing | `BM_RBAC_RoleHierarchyValidation` |
| SEC-5 | Siehe Zielbeschreibung: TLS 1.3 Handshake P99 | `BM_AES256GCM_Encrypt_64KB` |
| SEC-6 | Siehe Zielbeschreibung: RBAC Policy Eval (100 Rollen) P99 | `BM_RBAC_PermissionCheck_ManyRoles` |
| SEC-7 | Siehe Zielbeschreibung: HSM-Backed RSA-2048 Sign P99 | `BM_RBAC_PermissionCheck_SingleRole` |
| SEC-8 | Siehe Zielbeschreibung: Audit Log Write P99 | `BM_FieldEncryption_SmallDocument` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
