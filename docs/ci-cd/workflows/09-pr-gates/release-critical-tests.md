# Release-Critical Test Gate (`09-pr-gates_release-critical-tests.yml`)

## Scope

Der Gate-Workflow erzwingt release-kritische Integrationsflüsse für `develop` und Editions-Release-Branches (`community`, `enterprise`, `hyperscaler`, `military`).

## Verbindliche Gate-Regeln

1. Build nur der priorisierten Release-Critical Targets.
2. CTest-Label `release_critical` muss mindestens einen Test selektieren.
3. Suite läuft mit `--repeat until-fail:5` zur Flake-Erkennung.
4. Logs (`release-critical-selection.log`, `release-critical-ctest.log`) werden immer als Artifact publiziert.

## Priorisierte Flows (Owner/Diagnostik)

| Test Target | Zweck | Owner | Diagnostik-Fokus |
|---|---|---|---|
| `query_execution_pipeline_test` | Query/Auth/Cache-Pfad | Query + Runtime | `401/400/404` Fehlercodes, Success-/Error-Metriken |
| `ingestion_pipeline_test` | Ingest + CDC + Checkpoint | Ingestion | Schema-/Content-Fehler ohne Teilartefakte |
| `transaction_replication_pipeline_test` | WAL/Replica/Saga/Failover | Storage + Replication | Commit/Abort-Status, WAL-Rückstand |
| `security_pipeline_test` | Auth/RBAC/Encryption/Rotation | Security | Datenleck-freie 401/403 Pfade, Audit-Masking |
| `rag_ai_pipeline_test` | RAG Degradation/Caching | AI Runtime | Embedding/Inference Fallback, Cache-Hits |
| `application_profile_pipeline_test` | Multi-Tenant + Timeout + Circuit Breaker | Application Profile | Retry/Circuit/Fallback Konsistenz |

## Repro-Strategie (lokal)

```bash
cmake --preset community-release
cmake --build --preset community-release --parallel 4 --target \
  query_execution_pipeline_test ingestion_pipeline_test rag_ai_pipeline_test \
  transaction_replication_pipeline_test security_pipeline_test application_profile_pipeline_test
ctest --preset community-release -L release_critical --output-on-failure --repeat until-fail:5
```

## Restlücken / Follow-ups

- Gate ist aktuell Linux-zentriert (`community-release`); plattformübergreifende Re-Checks (Windows) bleiben Follow-up.
- Flake-Loop ist auf 5 Wiederholungen begrenzt (schneller PR-Feedback-Kompromiss).
