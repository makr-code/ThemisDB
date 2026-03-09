# Scheduler-Modul

**Stand:** 9. März 2026
**Version:** 1.5.0
**Kategorie:** Scheduler
**Validated:** 2026-03-09 (9168268)
**Status:** current

---

## Übersicht

Das Scheduler-Modul implementiert ThemisDBs vollständige Task-Scheduling- und Automatisierungsinfrastruktur: periodische AQL-Query-Ausführung, benutzerdefinierte Funktionen, ein 3-stufiges Zeitreihendaten-Lifecycle-Management (HybridRetentionManager), verteilte Koordination mit Leader-Election, DAG-basierte Workflow-Ausführung, CDC-Event-Trigger, durchsuchbares Audit-Log und externe Scheduler-Integration (Kubernetes CronJob, Apache Airflow).

**Primäre Dokumentation:** [`src/scheduler/README.md`](../../../src/scheduler/README.md)
**Roadmap:** [`src/scheduler/ROADMAP.md`](../../../src/scheduler/ROADMAP.md)
**Geplante Erweiterungen:** [`src/scheduler/FUTURE_ENHANCEMENTS.md`](../../../src/scheduler/FUTURE_ENHANCEMENTS.md)
**Fehlende Implementierungen:** [`docs/de/scheduler/missing-implementations.md`](./missing-implementations.md)

---

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| TaskScheduler | `task_scheduler.h` | `task_scheduler.cpp` | Kern-Engine: Thread-Pool, Retry-Policies, DAG-Ausführung, SLA-Alerting |
| HybridRetentionManager | `hybrid_retention_manager.h` | `hybrid_retention_manager.cpp` | 3-stufiger Zeitreihen-Lifecycle (Gorilla, Downsampling, Tagesaggregate) |
| DistributedTaskCoordinator | `distributed_task_coordinator.h` | `distributed_task_coordinator.cpp` | Cluster-weite Leader-Election und Task-Koordination |
| EventTrigger / EventTriggerManager | `event_trigger.h` | `event_trigger.cpp` | CDC-Changefeed → Task-Ausführungs-Bridge mit Circuit Breaker |
| ExternalSchedulerAdapter | `external_scheduler_adapter.h` | `external_scheduler_adapter.cpp` | Kubernetes CronJob + Apache Airflow DAG Generator |
| TaskAuditManager | `task_audit_manager.h` | `task_audit_manager.cpp` | Durchsuchbares Audit-Log (queryAuditEvents, JSONL-Export, SIEM) |
| TaskAuditEvent | `task_audit_event.h` | `task_audit_event.cpp` | Strukturierte Audit-Event-Typen |
| TaskAnomalyDetector | `task_anomaly_detector.h` | `task_anomaly_detector.cpp` | Statistische Anomalieerkennung für Ausführungsmuster |
| TaskResultStore | `task_result_store.h` | `task_result_store.cpp` | Persistente Speicherung von Task-Ausgaben in RocksDB |
| CronExpression | `utils/cron_parser.h` | `utils/cron_parser.cpp` | Vollständiger Cron-Parser (v1.5.0): Aliases, @-specials, Jahr-Feld, Timezone |

**Gesamt:** 9 Header + 9 Source-Dateien in `src/scheduler/` + `utils/cron_parser`

---

## Komponenten-Detail

### TaskScheduler

Kern-Task-Scheduling-Engine mit konfigurierbarem Thread-Pool. Unterstützt:

- Periodische AQL-Query-Ausführung und Custom-Function-Callbacks
- Mehrere Retry-Strategien: `FIXED_DELAY`, `EXPONENTIAL_BACKOFF`, `LINEAR_BACKOFF`, `JITTER_BACKOFF`, `FIBONACCI_BACKOFF`
- DAG-Ausführung (`executeDAG`) mit konditionaler Verzweigung (`on_condition`-Prädikat)
- SLA-Deadline-Monitoring mit Alertmanager-Integration
- Aufgaben-Persistenz und Recovery via RocksDB
- OpenTelemetry Distributed Tracing

**Performance:** < 1 % CPU-Overhead, 50–200 ms Task-Startlatenz

### HybridRetentionManager

Dreistufiges Zeitreihen-Lifecycle-Management:

| Stage | Zeitfenster | Strategie | Reduktion |
|-------|------------|-----------|-----------|
| 1 | 0–7 Tage | Gorilla-Kompression (XOR delta-of-delta) | 10–20× |
| 2 | 7–365 Tage | Varianzbasiertes Downsampling (LTTB-inspiriert) | Konfigurierbar |
| 3 | > 1 Jahr | Tagesaggregate | 99,9 % |

**Wissenschaftliche Basis:** Pelkonen et al. (2015) Gorilla-Algorithmus; Steinarsson (2013) LTTB

### DistributedTaskCoordinator

Koordiniert Task-Ausführung über mehrere Cluster-Nodes. Nutzt `DistributedCoordinator` (Sharding-Modul) für Leader-Election. Aktiviert/deaktiviert den lokalen `TaskScheduler` automatisch je nach Leader-Status.

### CronExpression Parser (v1.5.0)

Vollständiger POSIX-konformer Cron-Parser (IEEE Std 1003.1-2017):

```
Syntax:  minute hour day month weekday [year]
Beispiel: 0 9 * * MON-FRI 2025
```

Unterstützt: Wildcards `*`, Ranges `1-5`, Listen `1,3,5`, Steps `*/15`, Start/Step `5/15`, Monatsnamen `JAN`–`DEC`, Wochentagsnamen `MON`–`SUN`, @-Specials (`@daily`, `@hourly`, `@monthly`, `@weekly`, `@yearly`, `@reboot`), optionales 6-Feld-Format mit Jahr-Constraint, Timezone-aware (`tz_offset_seconds`).

### TaskAuditManager

Durchsuchbares Audit-Log für alle Task-Ausführungsereignisse:

```cpp
AuditQueryParams params;
params.task_id = "my-task";
params.from_time = /* ... */;
auto events = audit_manager->queryAuditEvents(params);
```

JSONL-Export für SIEM-Integration. Tamper-evident via `utils::AuditLogger`.

---

## Test-Abdeckung

| Test-Datei | Beschreibung |
|-----------|-------------|
| `test_task_scheduler.cpp` | Kern-Scheduler-Tests (DAG, Retry, Persistence) |
| `test_task_scheduler_api_handler.cpp` | REST-API-Handler-Tests |
| `test_task_scheduler_triggers.cpp` | Event-Trigger-Tests |
| `test_task_scheduler_siem_integration.cpp` | SIEM/Audit-Integrationstests |
| `test_external_scheduler_adapter.cpp` | K8s CronJob + Airflow DAG Generator |
| `test_scheduler_integration.cpp` | End-to-End-Integrationstests |
| `test_continuous_batch_scheduler.cpp` | Batch-Scheduler-Tests |
| `test_chaos_scheduler.cpp` | Chaos-Engineering-Tests |
| `test_governance_review_scheduler.cpp` | Governance-Review-Tests |
| `test_gpu_time_slice_scheduler.cpp` | GPU-Time-Slice-Tests |

---

## Bekannte Einschränkungen

- **Web-UI-Frontend:** REST-API (`task_scheduler_api_handler`) ist produktionsbereit. Ein Web-Frontend (Issue [#2445](https://github.com/ThemisDB/ThemisDB/issues/2445)) ist noch in Entwicklung.
- **Dynamisches Task-Scaling:** Automatische Skalierung basierend auf Queue-Tiefe ist geplant (Issue [#2269](https://github.com/ThemisDB/ThemisDB/issues/2269)).
- **DistributedTaskCoordinator** benötigt das Sharding-Modul (`DistributedCoordinator`) für Leader-Election in einer Cluster-Deployment-Konfiguration.

---

## Wissenschaftliche Grundlagen

1. **Gorilla-Kompression:** Pelkonen et al. (2015), VLDB Endowment 8(12). DOI: [10.14778/2824032.2824078](https://doi.org/10.14778/2824032.2824078)
2. **Work-Stealing Thread Pool:** Blumofe & Leiserson (1999), JACM 46(5). DOI: [10.1145/324133.324234](https://doi.org/10.1145/324133.324234)
3. **LTTB Downsampling:** Steinarsson (2013), Reykjavik University M.Sc. thesis. URL: http://skemman.is/stream/get/1946/15343/37285/3/SS_MSthesis.pdf
4. **POSIX Cron-Syntax:** IEEE Std 1003.1-2017. URL: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html
5. **Dapper Distributed Tracing:** Sigelman et al. (2010), Google Technical Report. URL: https://research.google/pubs/pub36356/

---

## Verwandte Dokumentation

- [Primäre Docs: src/scheduler/README.md](../../../src/scheduler/README.md)
- [ROADMAP](../../../src/scheduler/ROADMAP.md)
- [Fehlende Implementierungen](./missing-implementations.md)
- [HybridRetentionManager (Detail)](./HYBRID_RETENTION_MANAGER.md)
- [TaskScheduler (Detail)](./TASK_SCHEDULER.md)
- [Storage-Modul](../storage/README.md)
- [Query-Modul](../query/README.md)

---

*Validated: 2026-03-09 | Next Review: v1.6.0 Milestone*
