# Scheduler-Modul – Sekundärdokumentation (DE)

<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/scheduler/ und ../../../include/scheduler/ -->

**Modul:** `scheduler`  
**Version:** v1.5.0  
**Status:** ✅ Produktionsreif  
**Geprüft:** 2026-03-11 (Commit `1d4165d`)

---

## Inhalt dieses Verzeichnisses

| Datei | Beschreibung |
|-------|-------------|
| [TASK_SCHEDULER.md](TASK_SCHEDULER.md) | Übersicht: TaskScheduler-Architektur, Sicherheitshinweise, REST-API-Kurzreferenz |
| [HYBRID_RETENTION_MANAGER.md](HYBRID_RETENTION_MANAGER.md) | 3-Stufen-Lifecycle (Gorilla, Downsampling, Tagesaggregate) |
| [ADAPTIVE_VS_TIME_BASED_RETENTION.md](ADAPTIVE_VS_TIME_BASED_RETENTION.md) | Vergleich: Gorilla vs. zeitbasierte vs. adaptive Retention |
| [AQL_RETENTION_EXTENSIONS.md](AQL_RETENTION_EXTENSIONS.md) | AQL-Erweiterungen für Retention-Abfragen |
| [DATA_RETENTION_DOWNSAMPLING.md](DATA_RETENTION_DOWNSAMPLING.md) | Downsampling-Algorithmen und -Konfiguration |
| [SYSTEM_IMPACT_ANALYSIS.md](SYSTEM_IMPACT_ANALYSIS.md) | Systemauswirkungsanalyse des Schedulers auf ThemisDB |
| [IMPLEMENTATION_SUMMARY.md](implementation-history/summaries/IMPLEMENTATION_SUMMARY.md) | Historische Implementierungszusammenfassung (v0.0.x) |
| [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md) | Reality-Check-Befunde: fehlende oder unvollständige Implementierungen |

---

## Schnellübersicht

Der **TaskScheduler** ist die zentrale Komponente für zeitgesteuerte und ereignisbasierte
Aufgaben in ThemisDB. Er unterstützt:

- **Cron-Ausdrücke** (6-Felder, Zeitzone, @-Spezialformen)
- **Feste Intervalle** (`interval`-basiert)
- **CDC-Ereignis-Trigger** (`EventTriggerManager`)
- **Manuelle Ausführung** (`executeTaskNow`, `executeDAG`)
- **DAG-Abhängigkeiten** mit bedingtem Branching (`branch_condition`)
- **Retry-Strategien** (FIXED, EXPONENTIAL, LINEAR, JITTER, FIBONACCI)
- **SLA-Überwachung** und Alertmanager-Integration
- **Dynamische Nebenläufigkeitsskalierung** (`enable_dynamic_scaling`)
- **Web-UI** für Task-Management (`GET /api/v1/scheduler/ui`)

### Primärquellen (maßgeblich für Details)

| Quelle | Zweck |
|--------|-------|
| [`src/scheduler/README.md`](../../../src/scheduler/README.md) | Entwickler-Einstieg, Architektur, Komponenten |
| [`src/scheduler/ROADMAP.md`](../../../src/scheduler/ROADMAP.md) | Feature-Status und Roadmap |
| [`src/scheduler/ARCHITECTURE.md`](../../../src/scheduler/ARCHITECTURE.md) | Komponentendiagramm, Datenpfade |
| [`src/scheduler/FUTURE_ENHANCEMENTS.md`](../../../src/scheduler/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen mit wissenschaftlichen Referenzen |
| [`include/scheduler/README.md`](../../../include/scheduler/README.md) | Vollständige Public-API-Dokumentation |
| [`include/scheduler/FUTURE_ENHANCEMENTS.md`](../../../include/scheduler/FUTURE_ENHANCEMENTS.md) | API-Entwürfe für geplante Features |

---

## Fehlende Implementierungen

Beim Reality-Check wurden Abweichungen zwischen Dokumentation und Sourcecode identifiziert.
Details: → [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md)

---

## Verwandte Themen

- [CDC-Modul (Changefeed)](../../cdc/README.md) – Event-Trigger-Quelle
- [Performance-Dokumentation](../../performance/) – Scheduler-Benchmarks
- [Security-Dokumentation](../../security/README.md) – AQL-Injection-Schutz, RBAC
