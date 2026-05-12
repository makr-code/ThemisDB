# PERFORMANCE_EXPECTATIONS — src/auth

## Scope
- Modul: `src/auth`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_auth_token_validation.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| AUT-1 | Siehe Zielbeschreibung: LDAP Bind P99 | `BM_JWT_ValidToken_RS256` |
| AUT-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_JWT_ValidToken_WithBlacklist` |
| AUT-3 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_JWT_ValidToken_RS256` |
| AUT-4 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_TokenBlacklist_IsRevoked_Hit` |
| AUT-5 | Siehe Zielbeschreibung: Redis Token Revocation P99 | `BM_TokenBlacklist_IsRevoked_Miss` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| AUTHG-1 | <= 12 ms (JWT Validierung P95) | p95 aus `BM_JWT_ValidToken_RS256` |
| AUTHG-2 | >= 30000 ops/s (Blacklist Hit/Miss Throughput) | mean aus `BM_TokenBlacklist_IsRevoked_Hit` und `BM_TokenBlacklist_IsRevoked_Miss` |
| AUTHG-3 | <= 18 ms (Token Revocation P99) | p99 aus `BM_TokenBlacklist_IsRevoked_Miss` |
| AUTHG-4 | Regression <= 7 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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