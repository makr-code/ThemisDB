# LLM Optimierungsebenen-Matrix — ThemisDB Laufzeit-Optimierung durch Semantik & Mustererkennung

**Forschungsdokument**
**Version:** 1.0
**Datum:** 2026-04-16
**Status:** 🔬 Design / Research
**Kategorie:** LLM-augmentierte Datenbankoptimierung

---

## Inhaltsverzeichnis

- [0. Kontext & Verhältnis zu den Loops 1–4](#0-kontext--verhältnis-zu-den-loops-14)
- [1. Gesamtübersicht der Ebenenmatrix](#1-gesamtübersicht-der-ebenenmatrix)
- [Ebene 5 — Transaktions-Semantik & Konfliktvoraussage](#ebene-5--transaktions-semantik--konfliktvoraussage)
- [Ebene 6 — Schema-Evolution & Datenmigrations-Regie](#ebene-6--schema-evolution--datenmigrations-regie)
- [Ebene 7 — Sicherheits-Anomalie-Erkennung via Semantik](#ebene-7--sicherheits-anomalie-erkennung-via-semantik)
- [Ebene 8 — Multi-Tenant Workload-Isolation & Ressourcenpolitik](#ebene-8--multi-tenant-workload-isolation--ressourcenpolitik)
- [Ebene 9 — Erklärbarkeit & DBA-Dialog](#ebene-9--erklärbarkeit--dba-dialog)
- [Ebene 10 — Speicher-Layout & Kompression auf Semantikebene](#ebene-10--speicher-layout--kompression-auf-semantikebene)
- [Ebene 11 — Verteiltes Wissens-Sharding (RAID-5 der Intelligenz)](#ebene-11--verteiltes-wissens-sharding-raid-5-der-intelligenz)
- [2. Schnittstellen zwischen den Ebenen](#2-schnittstellen-zwischen-den-ebenen)
- [3. Implementierungsreihenfolge & Quick-Wins](#3-implementierungsreihenfolge--quick-wins)
- [4. Offene Forschungsfragen](#4-offene-forschungsfragen)
- [5. Referenzen](#5-referenzen)

---

## 0. Kontext & Verhältnis zu den Loops 1–4

ThemisDB hat bereits vier selbstoptimierende Kreisläufe implementiert
(dokumentiert in `THEMISDB_LORA_RESEARCH_PAPER.md` und `THEMISDB_LORA_METRICS_AND_OVERVIEW.md`):

| Loop | Zeitskala | Kern-Signal | ThemisDB-Komponente |
|------|-----------|-------------|---------------------|
| 1 – Query Execution | ≤ 10 ms | Ausführungszeit je Plan | `WorkloadAdaptiveOptimizer`, BAO |
| 2 – Workload Adaptation | 60 s | Query-Klassen-Verteilung | `HnswParameterTuner`, `WorkloadProfile` |
| 3 – Index Lifecycle | Stunden–Tage | Indexnutzungsstatistiken | `AdaptiveIndex`, `SelectivityAnalyzer` |
| 4 – Adapter Improvement | Wöchentlich | DBA-Feedback, RLAIF | `IncrementalLoRATrainer`, `RLAIFTrainer` |

Die Ebenen 5–10 sind **orthogonale Optimierungsachsen** — sie operieren nicht entlang
der Latenz-Zeitachse der Loops, sondern entlang der **semantischen Tiefe** der Eingabedaten.
Das LLM bringt hier echtes Domänenwissen ein: über Transaktionssemantik, Schemastrukturen,
Sicherheitskontexte, Mandanten-Workloads und physische Datenlayouts.

---

## 1. Gesamtübersicht der Ebenenmatrix

```
┌────────┬──────────────────────────────────────┬────────────┬──────────────────────────┬───────────────┐
│ Ebene  │ Optimierungsziel                     │ Zeitskala  │ Semantik-Input           │ Autonomie     │
├────────┼──────────────────────────────────────┼────────────┼──────────────────────────┼───────────────┤
│ 1–4    │ Query/Index/Adapter                  │ ms–Wochen  │ Query-Pattern, Metriken  │ hoch          │
│ 5      │ Transaktions-Konfliktvoraussage       │ < 10 ms    │ Tx-Inhalt, Entitäts-Map  │ mittel        │
│ 6      │ Schema-Evolution-Regie               │ Tage–Wochen│ Nutzungsmuster, Typen    │ Advisory      │
│ 7      │ Sicherheits-Anomalie via Semantik    │ < 100 ms   │ Session-Kontext, Intent  │ mittel → hoch │
│ 8      │ Multi-Tenant Workload-Isolation       │ Sekunden   │ Workload-Fingerprint     │ hoch          │
│ 9      │ Erklärbarkeit & DBA-Dialog           │ on-demand  │ Alle Ebenen-Signale      │ Advisory      │
│ 10     │ Layout & Kompression (Semantik)      │ Stunden    │ Semantischer Datentyp    │ Advisory      │
└────────┴──────────────────────────────────────┴────────────┴──────────────────────────┴───────────────┘
```

**Legende Autonomie:**
- `Advisory` — LLM generiert Empfehlung, DBA/System bestätigt explizit
- `mittel` — LLM handelt autonom innerhalb konfigurierter Guardrails
- `hoch` — LLM handelt autonom, nur Monitoring-Alert bei Abweichung

---

## Ebene 5 — Transaktions-Semantik & Konfliktvoraussage

### Motivation

Der `DeadlockPredictor` in ThemisDB lernt bereits aus historischen Transaktions-Ereignissen.
Bisher beschränkt sich dieses Lernen auf strukturelle Muster (Entitäts-IDs, Lock-Reihenfolgen).
Das LLM kann die **inhaltliche Semantik** einer Transaktion verstehen und damit
Konflikte voraussagen, noch bevor der Lock-Manager involviert wird.

### Signal-Quellen

| Signal | ThemisDB-Quelle | Format |
|--------|-----------------|--------|
| Entitätsmenge je Transaktion | `TransactionManager::activeTransactions()` | Set\<entity_id\> |
| Write-Intent-Pattern | `WriteBatch` Inhalt (Collection + Keys) | JSON diff |
| Historische Konfliktraten | `DeadlockPredictor::Config::conflict_history` | Zeitreihe |
| Session-Affinität | `TransactionAuditor` Logs | session_id → entity_clusters |
| Graph-Nachbarschaft | `GraphIndexManager` (Chimera-Adapter) | Adjazenzliste |

### LLM-Aufgaben auf dieser Ebene

**5a — Batch-Affinität:**
Das LLM erkennt, dass mehrere Transaktionen semantisch zur selben Domäne gehören
(z.B. alle Updates einer User-Session, alle Preis-Updates eines Produktkatalogs)
und empfiehlt, sie in einen `TransactionBatcher`-Slot zu gruppieren.

```
Input:  Tx-Menge { Write(user:42, email), Write(user:42, name), Write(user:42, prefs) }
Output: BatchHint { session_affinity: "user:42", suggested_batch_window_ms: 5 }
```

**5b — Optimistic-Lock-Eskalation:**
Wenn das LLM einen hohen semantischen Overlap zweier Transaktionen erkennt
(überlappende Entity-Cluster im Chimera-Graphen), eskaliert es frühzeitig von
`OPTIMISTIC` auf `PESSIMISTIC` Isolation, bevor ein Retry-Cycle auftritt.

```
Input:  Tx_A modifies {Invoice:X, Order:X}, Tx_B modifies {Order:X, Payment:X}
Output: IsolationHint { tx_a: PESSIMISTIC, reason: "Order:X semantic overlap" }
```

**5c — Write-Amplification-Vorhersage:**
Das LLM erkennt kaskadierende Update-Effekte im Graphen.
Beispiel: Ein Update auf `Product:42` löst via `ON_UPDATE_CASCADE`-Semantik
Updates auf 1.200 abhängige Knoten aus.

```
Input:  Write(Product:42, price=19.99)
Output: AmplificationWarning { fan_out_estimate: 1200, suggested_strategy: BATCH_DEFERRED }
```

### ThemisDB-Komponenten

```
include/transaction/deadlock_predictor.h     → history input
include/transaction/transaction_manager.h    → lock escalation hook
include/transaction/transaction_batcher.h    → batch grouping output
include/transaction/lock_manager.h           → pessimistic lock gate
include/chimera/themisdb_adapter.h           → graph neighborhood lookup
include/llm/ai_orchestrator.h               → LLM inference pipeline
```

### Implementierungsphasen

**Phase 1 — API-Vertrag (2 Wochen)**
- Neues Interface `ITransactionSemanticAdvisor` in `include/transaction/`
- Methoden: `batchHint(txBatch)`, `isolationHint(txPair)`, `amplificationWarn(write)`
- Signal-Adapter für `DeadlockPredictor` → LLM-Context

**Phase 2 — Core-Implementierung (4 Wochen)**
- `LLMTransactionAdvisor` implementiert `ITransactionSemanticAdvisor`
- RAG-Kontext aus `TransactionAuditor`-Logs (letzte 1.000 Tx)
- Chimera-Adapter liefert Entity-Cluster als JSON für LLM-Prompt

**Phase 3 — Fehlerbehandlung (1 Woche)**
- Timeout-Fallback: kein LLM-Advice → `DeadlockPredictor` regiert allein
- Circular-Dependency-Erkennung im Graphen vor Cluster-Expansion

**Phase 4 — Tests**
- Unit: BatchHint korrekt für bekannte Session-Affinität
- Integration: Pessimistic-Eskalation verhindert messbare Retry-Cycles
- Property-based: Kein Correctness-Verlust bei jeder Advisory-Entscheidung

**Phase 5 — Performance-Ziele**
- LLM-Advice-Latenz ≤ 5 ms (cached token prefix für Tx-Pattern)
- Batch-Affinität-Erkennung: ≥ 80 % Precision für Session-Pattern
- Retry-Cycle-Reduktion: ≥ −20 % vs. Baseline (`DeadlockPredictor` allein)

### Sicherheit & Zuverlässigkeit
- LLM-Advice darf **nie** eine Transaktion blockieren — nur Hint, nicht Gate
- Alle Advisory-Entscheidungen werden in `TransactionAuditor` geloggt
- Rollback-Mechanismus: Feature-Flag `THEMIS_LLM_TX_ADVISOR_ENABLED`

---

## Ebene 6 — Schema-Evolution & Datenmigrations-Regie

### Motivation

ThemisDB hat bereits `OnlineSchemaMigration` (Online-DDL) und
`DocumentSchemaEvolution` (versioniertes Schema-Register). Bisher sind
beide reaktiv — ein DBA muss die Migration auslösen. Das LLM kann
Migrations-Kandidaten **proaktiv identifizieren** und priorisieren,
indem es Nutzungsmuster über Zeit analysiert.

### Signal-Quellen

| Signal | ThemisDB-Quelle | Format |
|--------|-----------------|--------|
| Feld-Zugriffsfrequenz | `WorkloadAdaptiveOptimizer::WorkloadProfile.hot_tables` | Collection → Feld → Count |
| Typ-Koercion-Anomalien | AQL-Parser (`AQLQueryBuilder`) | Cast-Ops je Feld |
| Ungenutzte Felder | `SelectivityAnalyzer` (Abfrage-Coverage) | Feld → last_read_timestamp |
| Join-Pattern | `LLMAQLHandler` Query-Logs | Collection-Paare mit Häufigkeit |
| GDPR-Retention | `GdprSubjectRightsManager` | Data-Retention-Policy je Collection |

### LLM-Aufgaben auf dieser Ebene

**6a — Denormalisierungsempfehlung:**
LLM erkennt, dass zwei Collections immer gemeinsam in JOINs erscheinen
und empfiehlt eine materialisierte Ansicht oder Inline-Embedding.

```
Input:  Query-Log zeigt: 94 % aller AQL-Queries joinen Orders mit Customers
Output: DenormalizationHint {
  collections: ["Orders", "Customers"],
  suggested_action: EMBED_FIELD,
  fields: ["customer_name", "customer_email"],
  estimated_query_speedup: "−35 % p99",
  migration_risk: LOW
}
```

**6b — Dead-Weight-Detektion:**
LLM erkennt Felder/Collections die seit ≥ 90 Tagen nicht gelesen wurden.
Empfehlung: Archivierung, GDPR-Prüfung oder DROP.

```
Input:  Field "legacy_notes" in Orders: last_read = 2025-12-01, size = 4.2 GB
Output: ArchivingCandidate {
  field: "Orders.legacy_notes",
  last_accessed: "2025-12-01",
  storage_saved_gb: 4.2,
  gdpr_check_required: true,
  suggested_action: ARCHIVE_TO_COLD_TIER
}
```

**6c — Typ-Migrations-Vorschlag:**
LLM erkennt strukturelle Typ-Inkonsistenzen (Feld gespeichert als `string`,
aber 99,8 % der Werte sind numerisch).

```
Input:  Field "price" in Products: stored_type=STRING, avg_cast_ops=2847/min
Output: TypeMigrationHint {
  field: "Products.price",
  current_type: STRING,
  recommended_type: DECIMAL(10,2),
  aql_cast_ops_eliminated: 2847,
  migration_plan: ONLINE_DDL_ADD_COLUMN
}
```

### ThemisDB-Komponenten

```
include/storage/online_schema_migration.h        → DDL execution
include/document/document_schema_evolution.h     → schema version registry
include/governance/gdpr_subject_rights.h         → GDPR pre-check
include/performance/workload_adaptive_optimizer.h → field access patterns
include/aql/aql_ingestion_bridge.h              → query log enrichment
include/llm/ai_orchestrator.h                   → LLM inference
```

### Implementierungsphasen

**Phase 1 — API-Vertrag (2 Wochen)**
- Interface `ISchemaAdvisor` mit: `detectDeadWeight()`, `suggestDenormalization()`, `suggestTypeMigration()`
- Adapter für `WorkloadProfile.hot_tables` → Schema-Nutzungs-Matrix

**Phase 2 — Core-Implementierung (4 Wochen)**
- `LLMSchemaAdvisor` mit RAG-Kontext (Schema-History + Zugriffsstatistiken)
- GDPR-Pre-Check-Hook: Jede Archivierungsempfehlung durchläuft `GdprSubjectRightsManager`
- Advisory-Output: JSON-Bericht mit Migrations-Risiko-Score (LOW/MEDIUM/HIGH)

**Phase 3 — Sicherheitsebene (1 Woche)**
- Kein automatisches DDL — nur Advisory-Ausgabe in `LLMSchemaAdvisorReport`
- Manueller Bestätigungs-Gate für MEDIUM/HIGH-Risiko-Migrationen
- Dry-Run-Modus für Migrationsschätzungen

**Phase 4 — Tests**
- Unit: DeadWeight-Erkennung korrekt für bekannte Zugriffshistorien
- Integration: Denormalisierungs-Hint verbesserter Query-Plan-Kosten
- GDPR-Test: Archivierungskandidat mit GDPR-Pflicht wird korrekt geflaggt

**Phase 5 — Performance-Ziele**
- Schema-Advisor-Analyse: ≤ 2 s (Batch, nicht im kritischen Pfad)
- Dead-Weight-Detektion Precision: ≥ 90 % (kein False-Positive-DROP)
- Denormalisierungs-Vorschläge: ≥ 75 % DBA-Akzeptanzrate (gemessen via `FeedbackCollector`)

---

## Ebene 7 — Sicherheits-Anomalie-Erkennung via Semantik

### Motivation

`ZeroTrustPolicyEnforcer` und `AccessControl` prüfen derzeit Regeln und Rollen.
Sie verstehen nicht den *Intent* einer Abfrage. Das LLM kann den semantischen
Kontext einer AQL-Abfrage im Verhältnis zu historischen Session-Mustern bewerten
und bösartige oder unerwünschte Zugriffe erkennen, die regelbasierte Systeme nicht
detektieren können (z.B. langsame Exfiltration durch viele kleine, valide Abfragen).

### Signal-Quellen

| Signal | ThemisDB-Quelle | Format |
|--------|-----------------|--------|
| Abfrage-Intent | `LLMAQLHandler` (AQL → LLM-Intent) | Intent-Klasse + Konfidenz |
| Session-History | `ZeroTrustContext` (last_verified_at, session_risk_score) | Zeitreihe |
| Feld-Zugriffs-Muster | `AccessControl` Audit-Log | Session × Feld × Zeitstempel |
| Volumen-Anomalie | `MLAnomalyDetector` (ARIMA/Prophet) | Std-Abweichung je Metrik |
| Privilege-Zustand | `SessionConfig.mfa_required_roles` | Aktive Rollen je Session |

### LLM-Aufgaben auf dieser Ebene

**7a — Query-Intent-Klassifikation:**
LLM bewertet eine AQL-Abfrage nicht nur syntaktisch (ist sie valide?), sondern
semantisch: Entspricht der Intent dem historischen Muster dieser Session?

```
Input:
  Session: user_id=42, role=["readonly"], historical_avg_result_size=12 docs
  Query: FOR d IN Customers RETURN d  (result_size_estimate: 180.000 docs)
Output: IntentAlert {
  anomaly_type: BULK_EXTRACTION,
  risk_score: 0.94,
  recommended_action: RATE_LIMIT_AND_ALERT,
  reason: "Session hat historisch max. 12 Dokumente abgefragt; dieser Bulk-Select ist 15.000× größer"
}
```

**7b — Privilege-Creep-Erkennung:**
LLM erkennt, dass ein Service-Account plötzlich Felder abfragt,
die er in den letzten 90 Tagen nie angefragt hat.

```
Input:
  Service: api_service_v2, last_90_days_fields: ["order_id", "status", "total"]
  Current query accesses: ["order_id", "status", "total", "customer_ssn", "payment_card_hash"]
Output: PrivilegeAlert {
  new_fields: ["customer_ssn", "payment_card_hash"],
  gdpr_sensitive: true,
  recommended_action: BLOCK_AND_NOTIFY_SECURITY_TEAM
}
```

**7c — Semantische AQL-Injection-Erkennung:**
Über syntaktische Prüfungen hinaus: LLM erkennt Semantik-Anomalien
wie Time-Based-Blind-Injection-Muster (ungewöhnliche Sleep/Delay-Konstrukte)
oder UNION-Tricks in AQL-Parametern.

```
Input:  AQL-Fragment aus User-Input: "... LET x = SLEEP(3) RETURN x ..."
Output: InjectionAlert {
  pattern: TIME_BASED_BLIND,
  sanitized_query: "<BLOCKED>",
  recommended_action: BLOCK_IMMEDIATELY
}
```

### ThemisDB-Komponenten

```
include/security/zero_trust_policy_enforcer.h   → session_risk_score update
include/security/access_control.h               → audit log input
include/observability/ml_anomaly_detector.h     → volume anomaly baseline
include/observability/metric_anomaly_detector.h → field-level anomalies
include/llm/lora_security_validator.h           → LLM output integrity check
include/prompt_engineering/feedback_collector.h  → security event feedback
include/governance/gdpr_subject_rights.h        → GDPR-sensitive field detection
```

### Implementierungsphasen

**Phase 1 — API-Vertrag (2 Wochen)**
- Interface `IQueryIntentAdvisor` mit: `classifyIntent(query, session)`, `detectPrivilegeCreep(service, fields)`, `detectInjection(aqlFragment)`
- Integration Hook in `AccessControl::checkPermission()` (nach syntaktischer Prüfung, vor Execution)

**Phase 2 — Core-Implementierung (6 Wochen)**
- `LLMQueryIntentClassifier`: LoRA-Adapter `DomainType::SECURITY_MONITOR` (neu)
- Baseline-Profil aus 90-Tage-Window via `ZeroTrustContext`-History
- GDPR-sensitive-Fields-Lookup via `GdprSubjectRightsManager::getSensitiveFields()`

**Phase 3 — Fehlerbehandlung (1 Woche)**
- Fail-Open vs. Fail-Closed konfigurierbar: `THEMIS_LLM_SECURITY_FAIL_CLOSED=true`
- Latenz-Budget: > 50 ms → Fallback auf regelbasierte `AccessControl`
- False-Positive-Rate ≤ 0,1 % (sonst zu viele DBA-Alerts)

**Phase 4 — Tests**
- Unit: Intent-Klassifikation korrekt für Exfiltrations-, Normal- und Analyse-Pattern
- Red-Team: 20 bekannte AQL-Injection-Patterns werden erkannt
- Regression: Kein False-Positive für legitime Bulk-Exports (Backup-Jobs)

**Phase 5 — Performance-Ziele**
- Klassifikationslatenz ≤ 30 ms (p99) im kritischen Abfragepfad
- Precision (Anomalie-Erkennung) ≥ 95 %
- Recall (kein False-Negative bei kritischen GDPR-Feldern) ≥ 99 %

### Sicherheit & Zuverlässigkeit
- LLM-Output wird via `LoRASecurityValidator` auf Integrität geprüft
- Alle Security-Alerts werden in `AuditLogger` geschrieben (tamper-evident)
- Post-Quantum-Sicherung des Audit-Logs via `SphincsPlus`

---

## Ebene 8 — Multi-Tenant Workload-Isolation & Ressourcenpolitik

### Motivation

`TenantManager` verwaltet derzeit statische Quotas
(max_storage_bytes, requests_per_second, max_concurrent_queries).
Das LLM kann **dynamische Workload-Identitäten** erkennen und
Ressourcenzuteilung zur Laufzeit adaptieren — ohne manuelle Quota-Anpassungen.

### Signal-Quellen

| Signal | ThemisDB-Quelle | Format |
|--------|-----------------|--------|
| Workload-Typ | `WorkloadAdaptiveOptimizer::WorkloadType` | OLTP/OLAP/GRAPH/VECTOR |
| Query-Burst-Pattern | `TenantConfig.requests_per_second` Metrik | Zeitreihe je Tenant |
| Resource-Verbrauch | `Prometheus`-Metriken (CPU, IO, VRAM) | Counter/Gauge je Tenant |
| Shard-Affinität | `AdaptiveShardRouter` Routing-Protokoll | Tenant → Shard-Verteilung |
| Anomalie-Signal | `MLAnomalyDetector` | Abweichung vom erwarteten Profil |

### LLM-Aufgaben auf dieser Ebene

**8a — Workload-Fingerprint-Klassifikation:**
Das LLM erhält den aktuellen Query-Mix eines Tenants und klassifiziert
ihn in eine von vier Workload-Klassen mit spezifischen Ressourcenprofilen.

```
Input:  Tenant A: 90 % OLAP-Scans > 1M Docs, 10 % OLTP-Writes
Output: WorkloadFingerprint {
  tenant: "A",
  class: ANALYTICAL_BATCH,
  recommended_profile: {
    thread_pool_size: 16,
    cache_size_mb: 2048,
    io_priority: LOW,         // gibt OLTP-Tenants Vorrang
    vector_search_disabled: false,
    query_timeout_ms: 30000
  }
}
```

**8b — Cross-Tenant-Ressourcen-Arbitration:**
Wenn Tenant B einen OLTP-Burst startet, während Tenant A einen
OLAP-Background-Scan läuft, reduziert das LLM proaktiv
die IO-Priorität von A, ohne dass Quotas manuell angepasst werden müssen.

```
Input:
  Tenant A: OLAP scan (in progress, 45 % complete, low urgency)
  Tenant B: OLTP burst (SLA: p99 < 10 ms, currently at 9.8 ms)
Output: ResourceArbitration {
  reduce_io_for: "A",
  boost_io_for: "B",
  duration_estimate_s: 12,
  rationale: "Tenant B SLA critical, Tenant A batch tolerant"
}
```

**8c — Privacy-Safe Cross-Tenant-Lernübertragung:**
Das LLM lernt aus Optimierungserfolgen bei Tenant A (z.B. ein bestimmtes
HNSW-efSearch bei Embedding-Suchen) und überträgt das **Muster** (nicht die Daten)
auf Tenant B mit ähnlichem Workload-Fingerprint.

```
Input:  Tenant A: efSearch=128 → p99 reduction 22 ms→14 ms (VECTOR workload)
        Tenant B: VECTOR workload, efSearch currently=64
Output: TransferHint {
  tenant_target: "B",
  recommendation: "Increase efSearch to 128",
  basis: "Pattern-transfer from similar-fingerprint tenant (anonymised)",
  privacy_guarantee: "NO_DATA_SHARED"
}
```

### ThemisDB-Komponenten

```
include/server/tenant_manager.h                   → quota runtime update
include/performance/workload_adaptive_optimizer.h  → WorkloadType classification
include/sharding/adaptive_shard_router.h           → tenant → shard routing
include/index/hnsw_parameter_tuner.h              → efSearch cross-tenant transfer
include/observability/ml_anomaly_detector.h        → workload anomaly detection
include/llm/ai_orchestrator.h                     → inference pipeline
```

### Implementierungsphasen

**Phase 1 — API-Vertrag (2 Wochen)**
- Interface `IMultiTenantWorkloadAdvisor` in `include/server/`
- Methoden: `fingerprint(tenantId)`, `arbitrate(tenantPair)`, `transferPattern(src, dst)`
- Privacy-Guard: `transferPattern` operiert ausschließlich auf anonymisierten Metriken

**Phase 2 — Core-Implementierung (5 Wochen)**
- `LLMTenantWorkloadAdvisor` mit Prometheus-Metrik-Adapter
- `TenantConfig`-Runtime-Update via `TenantManager::updateConfig()` (bereits implementiert)
- Privacy-Layer: Tenant-IDs werden vor LLM-Inferenz durch UUID-Tokens ersetzt

**Phase 3 — Tests**
- Unit: Fingerprint-Klassifikation korrekt für OLTP, OLAP, VECTOR, GRAPH
- Integration: OLTP-SLA eingehalten während OLAP-Scan läuft
- Privacy: Zero-Knowledge-Test — kein Tenant-Klartextname im LLM-Prompt

**Phase 4 — Performance-Ziele**
- Arbitration-Reaktionszeit ≤ 2 s (nicht im kritischen Abfragepfad)
- SLA-Verletzungsrate durch OLTP-Tenants: ≥ −40 % vs. statische Quotas
- Cross-Tenant-Transfer-Akzeptanzrate (DBA-bestätigt): ≥ 70 %

---

## Ebene 9 — Erklärbarkeit & DBA-Dialog

### Motivation

Die Ebenen 1–8 produzieren Optimierungsentscheidungen — aber ohne Erklärung
verliert der DBA das Vertrauen in das System. Das LLM ist die einzige Schicht,
die alle vorherigen Entscheidungen **in natürlicher Sprache kontextualisieren** kann.
Ebene 9 ist kein eigenständiger Optimierer, sondern die **Explainability-Brücke**
zwischen allen anderen Ebenen und dem menschlichen Administrator.

### Signal-Quellen

Ebene 9 aggregiert **alle Signale der Ebenen 1–10**:

| Signal-Typ | Quelle |
|------------|--------|
| Loop-1–4 Entscheidungen | `WorkloadAdaptiveOptimizer`, `AdaptiveIndex`, `LoRATrainer` |
| Tx-Advisor-Decisions (E5) | `LLMTransactionAdvisor` |
| Schema-Hints (E6) | `LLMSchemaAdvisor` |
| Security-Alerts (E7) | `LLMQueryIntentClassifier` |
| Tenant-Arbitration (E8) | `LLMTenantWorkloadAdvisor` |
| Layout-Hints (E10) | `LLMStorageLayoutAdvisor` |
| DBA-Feedback | `FeedbackCollector`, `RLAIFTrainer` |

### LLM-Aufgaben auf dieser Ebene

**9a — Entscheidungs-Erklärung (Decision Rationale):**
Jede autonome Entscheidung aller Ebenen erhält ein `reason`-Feld
in strukturiertem Deutsch/Englisch.

```
Entscheidung: "Index 'idx_order_customer' wurde automatisch gelöscht"
Reason: "Dieser Index wurde in den letzten 47 Tagen in 0 von 284.193 Abfragen
         verwendet. Das Löschen spart 2,1 GB Speicher und eliminiert
         den Write-Overhead bei Inserts (~12 ms/Insert). Ähnliche Entscheidung
         wurde am 2026-03-12 für 'idx_product_legacy' getroffen (DBA bestätigt)."
```

**9b — Kausalitäts-Erklärung bei Leistungsproblemen:**

```
DBA-Frage: "Warum ist p99 der Abfrage Q42 von 8 ms auf 31 ms gestiegen?"
LLM-Antwort: "Drei Ursachen identifiziert:
  1. [Hauptursache, 68 %] Cache-Eviction: Tenant C hat gestern einen 3-GB-
     OLAP-Scan ausgeführt, der den Shared Buffer-Pool geleert hat. → Ebene 8
     hat noch kein Arbitration-Event ausgelöst, da C nicht als OLAP klassifiziert war.
  2. [Nebenursache, 22 %] HNSW-efSearch wurde von 64 auf 32 gesenkt (Loop 2,
     2026-04-15 03:12 UTC) um VRAM zu sparen. Q42 ist eine Vector-Lookup-Query.
  3. [Hintergrund, 10 %] Erhöhte Write-Concurrency durch Backup-Job (Montag 03:00)."
```

**9c — DBA-Chat & Gegenvorschlag-Workflow:**
Der DBA kann im natürlichsprachlichen Dialog Vorschläge ablehnen und
Begründungen geben, die direkt als RLAIF-Training-Signal verwendet werden.

```
DBA: "Der Vorschlag, idx_order_status zu löschen, ist falsch —
      dieser Index wird von unserem Monitoring-Job verwendet."
LLM: "Verstanden. Ich aktualisiere mein Profil für idx_order_status:
      'Batch-Job-Nutzung außerhalb AQL-Query-Log'. Soll ich
      zukünftig Batch-Job-Queries in die Nutzungsstatistik einbeziehen?
      [Ja / Nein / Nur für diese Collection]"
→ DBA-Feedback wird via FeedbackCollector als PreferencePair in RLAIFTrainer gespeist.
```

### ThemisDB-Komponenten

```
include/prompt_engineering/feedback_collector.h  → DBA-Feedback Storage
include/rag/rlaif_trainer.h                      → Preference-Pair Training
include/llm/ai_orchestrator.h                    → Dialog-Management
include/llm/ai_decision_auditor.h                → Decision-Log (tamper-evident)
include/observability/metric_anomaly_detector.h  → Kausalitätssignale
include/rag/continuous_learning_orchestrator.h   → RLAIF-Feedback → Adapter
```

### Implementierungsphasen

**Phase 1 — API-Vertrag (2 Wochen)**
- `DecisionRecord` struct: `{layer, timestamp, action, reason, confidence, dba_accepted}`
- `IExplainabilityBridge` mit: `explain(decisionId)`, `askCausal(metricName, timerange)`, `submitFeedback(decisionId, accepted, rationale)`

**Phase 2 — Core-Implementierung (6 Wochen)**
- Decision-Log-Aggregator: Alle Ebenen schreiben in `AIDecisionAuditor`
- Kausalitäts-Graph: LLM baut bei Performance-Fragen einen Ursachen-Baum
- DBA-Chat-API: REST-Endpoint `POST /api/llm/dba-dialog` (streaming response)

**Phase 3 — Tests**
- Unit: `explain(decisionId)` liefert korrekte Felder für alle Entscheidungstypen
- Integration: DBA-Feedback fließt innerhalb von 24h in neuen LoRA-Adapter
- Acceptance: DBA-User-Study — ≥ 80 % bewerten Erklärungen als "hilfreich"

**Phase 4 — Performance-Ziele**
- Erklärungslatenz: ≤ 500 ms für einfache Decisions
- Kausalitätsanalyse: ≤ 3 s für 7-Tage-Fenster
- RLAIF-Loop: DBA-Feedback → Adapter-Update ≤ 24 h

---

## Ebene 10 — Speicher-Layout & Kompression auf Semantikebene

### Motivation

ThemisDB wählt derzeit Speicher-Backends (RocksDB/LSM, B-Tree, Columnar, HNSW)
anhand von statischen Konfigurationen und einfachen Heuristiken. Das LLM kann
die **Semantik der gespeicherten Daten** verstehen und datentyp-spezifische
Optimierungen vorschlagen: Partitionierungsschema, Kompressionsalgorithmus,
Hot/Cold-Tier-Zuordnung.

### Signal-Quellen

| Signal | ThemisDB-Quelle | Format |
|--------|-----------------|--------|
| Datentyp-Verteilung | AQL-Schema-Analyse | Collection → Feldtypen-Histogramm |
| Kompressionsraten | RocksDB `CompactionStats` | Collection → Kompressions-Ratio |
| Access-Pattern | `WorkloadAdaptiveOptimizer` | Read/Write-Mix je Collection |
| Embedding-Dimensionen | `VectorIndexManager` | Dimension × Abstandsmetrik |
| Retention-Policy | `GdprSubjectRightsManager` | Pflichtretention je Feld |
| Zeitreihen-Granularität | `TimeSeriesManager` | Tick-Interval × Window-Größe |

### LLM-Aufgaben auf dieser Ebene

**10a — Semantische Partitionierung:**
LLM erkennt, dass bestimmte Datentypen columnar statt row-oriented
gespeichert werden sollten.

```
Input:
  Collection "Metrics" (1.2 TB):
    Fields: timestamp (int64), sensor_id (string), value (float64), unit (string)
    Query-Pattern: 98 % range-scans auf [timestamp] mit FILTER sensor_id
Output: LayoutHint {
  collection: "Metrics",
  recommended_layout: COLUMNAR,
  partition_key: "timestamp",
  secondary_cluster_key: "sensor_id",
  estimated_compression_improvement: "+340 %",
  query_speedup_estimate: "−45 % scan time"
}
```

**10b — Domänen-spezifische Kompression:**
LLM empfiehlt pro Collection-Typ den optimalen Kompressionsalgorithmus.

```
Collection-Typ              → Algorithmus              → Begründung
─────────────────────────────────────────────────────────────────────
Float-Embedding-Vectors    → Product Quantization (PQ) → 4–16× Kompression, recall ~95 %
Timestamps                 → Delta-Encoding + Gorilla   → 10–90× bei monotonen Zeitreihen
Freitext-Felder            → Zstandard (Zstd-19)        → 2–3× ohne Strukturverlust
JSON-Dokumente             → Snappy + Schema-aware       → 1.5–2× mit Feld-Indizierung
Log-Messages               → LZ4 + Rot-Dictionary        → 2–4× bei repetitivem Text
```

**10c — Hot/Cold-Tier-Semantik:**
Über Last-Access-Timestamps hinaus: LLM bewertet den *semantischen Wert*
von Daten für die Tier-Zuordnung.

```
Input:
  Collection "LegalCases" (500 GB): last_access = 18 Monate, case_status = "CLOSED"
  GDPR-Tag: "data_retention_required_7_years"
Output: TierHint {
  collection: "LegalCases",
  recommended_tier: COLD_WRITE_ONCE,  // nicht DELETE, weil GDPR
  rationale: "Rechtliche Aufbewahrungspflicht 7 Jahre aktiv. Cold-Tier-Move spart ~380 GB warm storage.",
  gdpr_policy: "DO_NOT_DELETE_BEFORE_2032-04-16",
  allowed_action: MOVE_TO_COLD_TIER
}
```

### ThemisDB-Komponenten

```
include/storage/online_schema_migration.h         → Layout-Migration Execution
include/index/hnsw_parameter_tuner.h             → Vector-PQ-Empfehlung
include/governance/gdpr_subject_rights.h         → Retention-Policy-Check
include/timeseries/anomaly_detection.h           → Zeitreihen-Pattern
include/sharding/gpu_erasure_coder.h             → Erasure-Coding für Cold-Tier
include/llm/ai_orchestrator.h                    → LLM-Inference
include/storage/online_schema_migration.h        → DDL für Layout-Änderungen
```

### Implementierungsphasen

**Phase 1 — API-Vertrag (2 Wochen)**
- Interface `IStorageLayoutAdvisor` mit: `analyzeLayout(collection)`, `suggestCompression(collection)`, `suggestTierPlacement(collection)`
- `LayoutHint` struct mit: `recommended_layout`, `compression_algo`, `tier`, `gdpr_gate`, `risk_level`

**Phase 2 — Core-Implementierung (6 Wochen)**
- `LLMStorageLayoutAdvisor` mit Schema-Analyse-Adapter
- GDPR-Gate: Jede Tier-Empfehlung durchläuft `GdprSubjectRightsManager::canArchive()`
- Kompressionsratings: Benchmark-Daten aus `BENCHMARK_ANALYSIS.md` als RAG-Kontext

**Phase 3 — Tests**
- Unit: Columnar-Empfehlung korrekt für Zeitreihen-Collections
- Integration: PQ-Empfehlung für Embedding-Collections ohne Recall-Verlust > 2 %
- GDPR-Test: Cold-Tier-Move mit aktiver Retention wird korrekt blockiert

**Phase 4 — Performance-Ziele**
- Layout-Analyse ≤ 10 s (Batch-Job, nicht im kritischen Pfad)
- Kompressionsverbesserung nach Advisory ≥ +50 % für Zeitreihen-Collections
- Kein fehlerhafter DELETE bei GDPR-geschützten Daten (0-Fehler-Ziel)

---

## Ebene 11 — Verteiltes Wissens-Sharding (RAID-5 der Intelligenz)

> **Vollständige Dokumentation:** `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md`

### Motivation

Die Ebenen 5–10 operieren **shard-lokal** — jeder Shard optimiert sich allein.
Ebene 11 ist die Infrastruktur, die Optimierungseinsichten **shard-übergreifend**
propagiert, ohne Rohdaten die Shard-Grenzen überschreiten zu lassen.

**Analogie:** Ebenen 5–10 sind die Daten in einem RAID-Array.
Ebene 11 ist der RAID-5-Controller, der die Parität (= verteiltes Wissen) verwaltet.

### Position in der Matrix

```
┌────────┬──────────────────────────────────────┬────────────┬──────────────────────────┬───────────────┐
│ Ebene  │ Optimierungsziel                     │ Zeitskala  │ Semantischer Input       │ Autonomie     │
├────────┼──────────────────────────────────────┼────────────┼──────────────────────────┼───────────────┤
│ 1–4    │ Query/Index/Adapter (lokal)          │ ms–Wochen  │ Query-Muster, Metriken   │ hoch          │
│ 5      │ Transaktionskonflikt-Voraussage       │ < 10 ms    │ Tx-Inhalt, Entity-Map    │ mittel        │
│ 6      │ Schema-Evolutions-Regie              │ Tage–Wochen│ Nutzungsmuster, Typen    │ Advisory      │
│ 7      │ Sicherheits-Anomalie via Semantik    │ < 100 ms   │ Session-Kontext, Intent  │ mittel → hoch │
│ 8      │ Multi-Tenant Workload-Isolation      │ Sekunden   │ Workload-Fingerprint     │ hoch          │
│ 9      │ Erklärbarkeit & DBA-Dialog           │ On-demand  │ Alle Ebenen-Signale      │ Advisory      │
│ 10     │ Layout & Kompression (Semantik)      │ Stunden    │ Semantischer Datentyp    │ Advisory      │
├────────┼──────────────────────────────────────┼────────────┼──────────────────────────┼───────────────┤
│ **11** │ **Verteiltes Wissens-Sharding**       │ **Stunden–**│ **Gradienten, Embeddings,**│ **Infrastruktur**│
│        │ **RAID-5 für Intelligenz**            │ **Tage**   │ **anonyme Metriken**     │               │
└────────┴──────────────────────────────────────┴────────────┴──────────────────────────┴───────────────┘
```

### Die vier Verbindungsebenen

| Verbindungs-Ebene | Mechanismus | Neue Komponente | Basis-Komponente |
|---|---|---|---|
| **A — Adapter-Discovery** | Gossip-Payload | `AdapterCapabilityAnnouncement` | `GossipProtocol` |
| **B — Federated LoRA** | FedAvg + DP | `LoRAFederationCoordinator` | `FederatedAggregator` |
| **C — Federated RAG** | RRF-Merge | `FederatedRAGMerger` | `QueryFederation` + `RAGIngestionBridge` |
| **D — Federated RLAIF** | Embedding-Gossip | `CrossShardFeedbackSync` | `FeedbackCollector` + `RLAIFTrainer` |

### Cross-Shard-Erweiterung der Ebenen 5–10

| Ebene | Shard-lokal (heute) | Cross-Shard mit Ebene 11 |
|---|---|---|
| E5 Tx-Semantik | Batch-Hints je Shard | `CrossShardTransaction`-Hints via `QueryFederation` |
| E6 Schema | Dead-Weight-Report je Shard | Aggregiert über alle Shards — kein saisonaler Feldverlust |
| **E7 Security** | IntentAlert je Shard | **Gossip-Propagation: Anomalie-Shard warnt alle sofort** |
| E8 Multi-Tenant | WorkloadFingerprint je Shard | Cross-Shard-Transfer bei ähnlichem Tenant-Fingerprint |
| E9 Explainability | AIDecisionAuditor je Shard | `FederatedAIDecisionAuditor` — globale Timeline aller Shards |
| E10 Layout | LayoutHint je Shard | LayoutHint via Gossip — shard-übergreifende Komprimierung |

### Differential-Privacy-Kern

Federated LoRA (Ebene 11B) verwendet den **Gaussian-Mechanismus**
(Dwork & Roth 2014):

```
σ = Δf · √(2·ln(1.25/δ)) / ε
```

Empfohlene Konfiguration: `ε = 0.1`, `δ = 1e-5`, max. `T = 50` Runden
→ `ε_total = 5.0` (praktisch akzeptabel, Dwork & Roth §3.5).

### Neue Akzeptanzkriterien für Ebene 11

| Kriterium | Schwellenwert |
|---|---|
| Gradient-Accuracy-Delta nach Runde | ≥ +0 % (kein Rückschritt) |
| DP-Budget-Verbrauch je Runde | ε_round ≤ 0.1 |
| Adapter-Routing-Qualität (Precision@3) | ≥ 80 % für domain_hint-Queries |
| RAG-Federated-Recall | ≥ +15 % vs. shard-lokal |
| DBA-Feedback-Propagations-Latenz | ≤ 2 × Gossip-Intervall |

### Offene Forschungsfragen für Ebene 11

**RQ-E11-1** — Wie viele Trainings-Samples benötigt ein Shard mindestens vor dem Beitrag?
*(Hypothese: n_k ≥ 500 — McMahan §4)*

**RQ-E11-2** — Konvergiert FedAvg bei stark heterogener Shard-Spezialisierung?
*(Hypothese: FedProx mit μ = 0.01 verhindert Divergenz bis 5× Heterogenität)*

**RQ-E11-3** — Verletzt Gradient-Transfer über EU/Non-EU-Shards Art. 44 DSGVO?
*(Hypothese: Nein — nur anonymisierte numerische Gradienten, kein personenbezogener Inhalt)*

---

## 2. Schnittstellen zwischen den Ebenen

Die **sieben** Ebenen (5–11) sind nicht isoliert. Sie teilen Signal-Quellen und
erzeugen gegenseitige Inputs:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     LLM Optimierungsebenen — Signalfluss                 │
│                                                                         │
│  Ebene 5 (Tx-Semantik)         ──►  Ebene 9 (Explainability)            │
│  Ebene 6 (Schema-Evolution)    ──►  Ebene 9                             │
│  Ebene 7 (Security)            ──►  Ebene 9                             │
│  Ebene 8 (Multi-Tenant)        ──►  Ebene 9                             │
│  Ebene 10 (Layout)             ──►  Ebene 9                             │
│                                                                         │
│  Ebene 7 (Security)            ──►  Ebene 5 (Tx-Block bei Anomalie)     │
│  Ebene 6 (Schema)              ──►  Ebene 10 (Typ → Kompression)        │
│  Ebene 8 (Tenant-Fingerprint)  ──►  Ebene 5 (Batch-Affinität je Tenant) │
│  Ebene 9 (DBA-Feedback)        ──►  Ebene 4 (RLAIF Loop)                │
│                                                                         │
│  Loops 1–4 (bekannt)           ──►  Ebene 9 (alle Decisions erklärbar)  │
│                                                                         │
│  Ebene 11 (Distributed Knowledge) — Cross-Shard-Transport für 5–10:    │
│  Ebene 7 (IntentAlert)         ──►  E11-Gossip ──► alle Shards          │
│  Ebene 6 (Dead-Weight)         ──►  E11-FedLoRA ──► globaler Report     │
│  Ebene 9 (AIDecision)          ──►  E11-Audit  ──► Federated Timeline   │
│  Ebene 4 (RLAIF)               ──►  E11-D      ──► Cross-Shard Feedback │
│  Ebene 3/4 (LoRA-Training)     ──►  E11-B      ──► FedAvg + DP(ε,δ)    │
└─────────────────────────────────────────────────────────────────────────┘
```

**Gemeinsamer AIDecisionAuditor:**
Alle Ebenen schreiben strukturierte `DecisionRecord`-Einträge in `AIDecisionAuditor`.
Ebene 9 liest daraus für Erklärungen. Ebene 4 (RLAIF) liest daraus für Training.
**Ebene 11** erweitert dies zum `FederatedAIDecisionAuditor` — DBA sieht Entscheidungen
aller Shards in einer globalen Timeline.

**Gemeinsame Guardrail-Schicht:**
Alle Ebenen mit `mittel`/`hoch` Autonomie durchlaufen den **Autonomie-Gate**
(dokumentiert in `THEMISDB_LORA_METRICS_AND_OVERVIEW.md` §5):
- ECE < 0,05
- Hot-Pattern-Coverage ≥ 85 %
- DBA-Akzeptanzrate ≥ 75 %

---

## 3. Implementierungsreihenfolge & Quick-Wins

**Reihenfolge nach ROI/Aufwand:**

| Priorität | Ebene | Begründung | Quick-Win |
|-----------|-------|------------|-----------|
| 1 | **E7 Security** | Vorhandene Signale (`ZeroTrustContext`, `MLAnomalyDetector`), kein neues Training-Signal nötig | Intent-Klassifikation mit bestehendem LLM-Adapter |
| 2 | **E9 Explainability** | Hoher DBA-Trust-Gewinn, keine Autonomie-Risiken, liefert RLAIF-Daten | Decision-Log + Kausalitäts-API |
| 3 | **E8 Multi-Tenant** | `TenantManager` bereits vorhanden, Workload-Profile bereits via `WorkloadAdaptiveOptimizer` | Workload-Fingerprint für beste Tenant-Isolation |
| 4 | **E6 Schema** | Online-DDL bereits vorhanden, Advisory-Only = kein Risiko | Dead-Weight-Report |
| 5 | **E5 Transaction** | `DeadlockPredictor` als Basis vorhanden | Batch-Affinität für Session-Pattern |
| 6 | **E10 Layout** | Langfristiger ROI (Speicherkosten), aufwändigere Migration | Columnar-Empfehlung für Zeitreihen |
| 7a | **E11-A Adapter-Gossip** | Quick Win: kein Training, sofortiges Domain-Routing | `AdapterCapabilityAnnouncement` broadcasten |
| 7b | **E11-C Federated RAG** | Nutzt vorhandene `QueryFederation` + `RAGIngestionBridge` | LLM sieht Wissen aller Shards |
| 7c | **E11-B Federated LoRA** | Kern der verteilten Intelligenz | FedAvg + DP über Shard-Gradienten |
| 7d | **E11-D Cross-Shard RLAIF** | DBA-Feedback wird global wirksam | Embedding-Gossip für Feedback-Summaries |

---

## 4. Offene Forschungsfragen

**RQ-E5-1** — Wie groß ist der tatsächliche Retry-Cycle-Gain von semantischer
Konfliktvoraussage vs. `DeadlockPredictor` allein?
(Hypothese: +15–25 % bei Graph-intensiven Workloads)

**RQ-E6-1** — Ab welcher Nutzungs-Lebensdauer kann ein Feld sicher als
"Dead Weight" eingestuft werden, ohne saisonale Muster zu verletzen?
(Hypothese: 180-Tage-Rolling-Window mit Saisonalitäts-Korrektur via ARIMA)

**RQ-E7-1** — Wie hoch ist die Falsch-Positiv-Rate von LLM-basierter
Intent-Klassifikation für Bulk-Export-Szenarien (Backup-Jobs)?
(Hypothese: < 0,1 % mit Session-Kontext-Conditionierung)

**RQ-E8-1** — Verletzt Privacy-Safe Cross-Tenant-Lernübertragung
GDPR-Grundsätze (Datentrennung Artikel 32 DSGVO)?
(Hypothese: Nein, wenn ausschließlich Muster — keine Rohdaten — transferiert werden)

**RQ-E9-1** — Erhöht Erklärbarkeit die DBA-Akzeptanzrate für autonome
Entscheidungen? (Hypothese: +20–35 pp vs. Empfehlung ohne Reason-Feld)

**RQ-E10-1** — Wie hoch ist der tatsächliche Recall-Verlust bei
Product-Quantization für hochdimensionale Embeddings (1536 Dim)?
(Hypothese: < 2 % Recall-Verlust bei 8×-Kompression — basierend auf JDH17)

**RQ-E11-1** — Wie viele Trainings-Samples benötigt ein Shard mindestens,
bevor sein Gradient-Beitrag das globale Modell nicht verschlechtert?
(Hypothese: n_k ≥ 500 — McMahan et al. §4)

**RQ-E11-2** — Konvergiert FedAvg bei stark heterogener Shard-Spezialisierung
(Security-Shard vs. Schema-Shard) oder divergiert das globale Modell?
(Hypothese: FedProx mit μ = 0.01 verhindert Divergenz bis 5× Heterogenität)

**RQ-E11-3** — Verletzt Gradient-Transfer über EU/Non-EU-Shard-Grenzen Art. 44 DSGVO?
(Hypothese: Nein — nur anonymisierte numerische Gradienten, kein personenbezogener Inhalt)

**RQ-E11-4** — Wie groß ist der Federated-RAG-Recall-Gewinn (RRF) gegenüber
rein shard-lokalem Retrieval?
(Hypothese: ≥ +15 % Recall@10 bei domänen-übergreifenden Queries)

---

## 5. Referenzen

- Van Aken et al. (2017). Automatic Database Management System Tuning Through Large-scale Machine Learning. ACM SIGMOD.
- Marcus et al. (2021). Bao: Learning to Steer Query Optimizers. SIGMOD.
- Bai et al. (2022). Constitutional AI: Harmlessness from AI Feedback. arXiv:2212.08073.
- Lee et al. (2023). RLAIF: Scaling RL from Human Feedback with AI Feedback. arXiv:2309.00267.
- Johnson et al. (2017). Billion-scale similarity search with GPUs. arXiv:1702.08734. [JDH17]
- Ding et al. (2020). Self-Managing Database Systems with AI Planning. VLDB.
- Pavlo et al. (2017). Self-Driving Database Management Systems. CIDR.
- Negi et al. (2023). Robust Query Driven Cardinality Estimation under Changing Workloads. VLDB.
- **McMahan, H.B. et al. (2017). Communication-Efficient Learning of Deep Networks from Decentralised Data. AISTATS.** *(Neu: Ebene 11)*
- **Li, T. et al. (2020). Federated Optimization in Heterogeneous Networks (FedProx). MLSys.** *(Neu: Ebene 11)*
- **Dwork, C. & Roth, A. (2014). The Algorithmic Foundations of Differential Privacy.** *(Neu: Ebene 11)*
- **Cormack, G.V. et al. (2009). Reciprocal Rank Fusion. ACM SIGIR.** *(Neu: Ebene 11)*
- ThemisDB: `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md` *(Neu: vollständige Ebene-11-Dokumentation)*
- ThemisDB: `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md` *(Neu: englische Version)*
- ThemisDB: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md`
- ThemisDB: `docs/en/research/THEMISDB_LORA_METRICS_AND_OVERVIEW.md`
- ThemisDB: `docs/de/research/MULTI_LAYER_FEEDBACK_LEARNING.md`
- ThemisDB: `docs/de/research/HYBRID_KONZEPT_THEMISDB.md`

---

## 6. Laufzeit-Einflussmechanismen: 7 Klassen

> **Querbezug:** `PERFORMANCE_EXPECTATIONS.md §14.1` ·
> `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md §12` ·
> `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12`

Diese sieben Klassen klassifizieren jeden Mechanismus, mit dem LLM-Infrastruktur
und AdaLoRA die SLOs (Ebenen 5–11) zur **Laufzeit** beeinflussen.

| # | Klasse | Semantik | Beispiele (Ebenen 5–11) |
|---|---|---|---|
| 1 | **Switch** | Binär ON/OFF — deterministischer Codepfad-Wechsel | `enable_draft_kv_cache`, `hot_swap.enabled`, `importance_pruning.enabled` |
| 2 | **Fader** | Kontinuierlich signiert −x…0…+x — Hot-Reload via SIGHUP | `acceptance_threshold` (0.6–0.75–0.9), `total_rank_budget` (128–512–1024), `speculative_tokens` (3–6–10) |
| 3 | **Optimizer** | Löst Zielfunktion (min/max) — keine Umgebungswahrnehmung | `WorkloadFingerprintEngine`, FedAvg Rank-Aggregation, TIES-Merge SVD |
| 4 | **Agentic Solver** | Wahrnehmung → Entscheidung → Aktion — autonom | `SelfImprovementModule`, LLM Intent Classifier (Ebene 7), `CrossShardFeedbackSync` |
| 5 | **Closed Loop** | Ausgabe gemessen → Korrektursignal zurückgeführt | AdaLoRA Rank-Allokation, CI SLO-Gate, RLAIF Quality-Loop |
| 6 | **Open Loop** | Aktion durch Input, kein Feedback-Pfad | SIGHUP hot-reload, Gossip-Broadcast, LoRA Hot-Swap |
| 7 | **Kausalkette** | Gerichtete Mehrschritt-Wirkungssequenz ohne Rückpfad | WorkloadFingerprintEngine → Rank-Budget → FedAvg → TTFT P99↓ |

Vollständige Tabellen mit ThemisDB-Instanzen: `PERFORMANCE_EXPECTATIONS.md §14.1` und
`docs/de/research/VERTEILTES_WISSEN_FEDERATION.md §12`.

**Operational Resilience — Querschnittsdimensionen**

Die fünf Dimensionen sind keine eigenständigen Klassen — sie instanziieren
die sieben Klassen oben mit konkreten Resilienz-Mustern. Sie gelten quer
über alle sechs semantischen Optimierungsebenen (L5–L10) und den gemeinsamen
LoRA/AdaLoRA-Stack.
Kanonische vollständige Tabellen:
`VERTEILTES_WISSEN_FEDERATION.md §12.8` · `DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12.8`.

### Backpressure

| Mechanismus | Klasse | Downstream-Signal | Upstream-Reaktion | SLO |
|---|---|---|---|---|
| Inference-Request-Queue | **Fader** | `max_pending_requests` überschritten | Ingestion-Rate gedrosselt | Dispatch-Latenz P99 |
| Kafka Semantic-Layer Event Lag | **Closed Loop** | Topic-Lag-Metrik | Consumer-Rate angepasst | Throughput |
| HTTP 429 (Inference Endpoint) | **Open Loop** | 429-Antwort | Exponential Backoff | TTFT |
| LLM-Queue Hard-Drop | **Switch** | Queue voll | Request abgelehnt (503) | Verfügbarkeit |

### Timeout / Circuit Breaker

| Mechanismus | Klasse | Auslöser | Aktion | Config-Key |
|---|---|---|---|---|
| Inference-Timeout | **Fader** | Deadline überschritten | Request abgebrochen | `inference_timeout_ms` |
| LoRA Hot-Swap Timeout | **Switch** | Swap > 5 s | Rollback auf vorherigen Adapter | `hot_swap.timeout_ms` |
| Circuit Breaker OPEN | **Closed Loop** | `failure_rate ≥ failure_threshold` | Pfad gesperrt, Probe-Requests | `circuit_breaker.failure_threshold` |
| gRPC-Deadline-Propagation | **Kausalkette** | Client setzt Deadline | Deadline durch alle Ebenen propagiert | gRPC-Metadata |

### Errors / Warnings

| Signal | Klasse | Quelle | Konsument | Wirkung |
|---|---|---|---|---|
| L5 Transaction Conflict WARN | **Kausalkette** | `TransactionSemanticAdvisor` | `DeadlockPredictor` → Re-Index | Conflict-Graph aktualisiert |
| L6 Schema Dead-Weight WARN | **Kausalkette** | `SchemaDeadWeightDetector` | `DocumentSchemaEvolution` → Advisory | Archivierungs-Kandidat markiert |
| L7 IntentClassifier Risiko=HIGH | **Kausalkette** | `IntentClassifier` | ZeroTrust → AuditLog → SIEM | Session gesperrt |
| Importance-Score NaN | **Kausalkette** | AdaLoRA-Layer | PruningEngine → Pruning deaktiviert | Rank-Budget fixiert bis Neustart |
| P99 > Baseline + 20 % | **Closed Loop** | SLO-Monitor | CI-Gate | Deployment geblockt |

### Security

| Mechanismus | Klasse | ThemisDB-Instanz | Bezug |
|---|---|---|---|
| TLS erzwingen | **Switch** | `tls.enforce` | `docker/admin-ui/nginx.ssl.conf` |
| MFA für Admin/Operator | **Switch** | `mfa_required_roles: [admin, operator]` | `include/security/access_control.h` |
| RBAC-Strenge | **Fader** | `rbac.policy_version` | `src/security/access_control.cpp` |
| Rate-Limiting Login | **Fader** | 5 r/m → 30 r/m (nginx) | `docker/admin-ui/nginx.conf` |
| ZeroTrust Session-Risk-Regelkreis | **Closed Loop** | `session_risk_score` → Dauer-Verifikation | `include/security/zero_trust_policy_enforcer.h` |
| Sicherheits-Anomalie → SIEM (Ebene 7) | **Kausalkette** | `IntentClassifier` → ZeroTrust → SIEM | `VERTEILTES_WISSEN_FEDERATION.md §12.7` |
| CSRF-Nonce-Validierung | **Switch** | `csrf_validation.enabled` | `docker/admin-ui/nginx.conf` |

### Hardening

| Maßnahme | Klasse | Mechanismus | Aktivierung |
|---|---|---|---|
| Plaintext-API ablehnen | **Switch** | `security.deny_plaintext_api` | ON in Production |
| Audit-Log-Verbosität | **Fader** | `audit.log_level` (INFO → DEBUG → TRACE) | SIGHUP |
| Dependency-Pinning + SBOM | **Open Loop** | CI-Scan bei jedem Build | GitHub Actions |
| IPv6-CIDR-Whitelist | **Fader** | `network_policy.cidr_allowlist` | `include/security/zero_trust_policy_enforcer.h` |
| Secret-Scan-Gate | **Closed Loop** | Alert → PR geblockt | GitHub Actions |
| GDPR Erase-Target (Ebenen 5–10) | **Closed Loop** | `GdprSubjectRightsManager` → per-Modul-ACK | `include/governance/gdpr_subject_rights.h` |
| AIDecisionAuditor-Abdeckung (alle 6 Ebenen) | **Open Loop** | L5–L10 schreiben `DecisionRecord` | `include/llm/ai_decision_auditor.h` |

> **Implementations-Arbeitspaket:** `docs/issues/distributed_knowledge/DK-OR-operational-resilience.md`
