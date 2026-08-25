# Linux-ThemisDB Gate Decision Sheet

Status: Draft
Datum: 2026-08-21
Scope: Numerische Gate-Parameter fuer Production-Readiness (SLO, RTO/RPO, Backlog, Restart, Recovery)
Verweis: LINUX_THEMISDB_INTEGRATION_MATCHING.md

## Zweck

Dieses Dokument operationalisiert die Go/No-Go Gates aus dem Linux-ThemisDB Matching als ausfuellbares Entscheidungsblatt.

Nutzung:

1. Zielumgebung benennen.
2. Numerische Schwellwerte je Gate eintragen.
3. Messlauf und Evidence verlinken.
4. Gate-Entscheidung (PASS/FAIL/BLOCKED) dokumentieren.

## Entscheidungsprofil

| Feld | Wert |
|---|---|
| Umgebung | TBD |
| Deployment-Modell | systemd / container / k8s (TBD) |
| Hardwareklasse | TBD |
| Datenprofil | TBD |
| KI-Lastprofil | TBD |
| Testfenster | TBD |
| Entscheiderkreis | TBD |
| Datum Gate-Review | TBD |

## Gate G1: Ressourcenstabilitaet (24h Mixed-Load)

### Ziel

Stabiler Dauerbetrieb ohne Ressourcenkollaps unter Mischlast (Core DB + AdaLoRA + CDC + Reindex + Repair).

### Schwellwerte (eintragen)

| Metrik | Zielwert | Maximalwert (Hard Limit) | Istwert | Ergebnis |
|---|---|---|---|---|
| OOM-Kills (24h) | 0 | 0 | TBD | TBD |
| Deadlocks | 0 | 0 | TBD | TBD |
| Unkontrollierte Restart-Loops | 0 | 0 | TBD | TBD |
| CPU Sättigung core-db (p95) | TBD | TBD | TBD | TBD |
| Memory Pressure sustained > N min | TBD | TBD | TBD | TBD |

### Evidence

- Testlauf-ID: TBD
- Artefakt/Log-Pfad: TBD
- Bemerkungen: TBD

### Gate-Entscheidung

- Status: PASS / FAIL / BLOCKED
- Begruendung: TBD

## Gate G2: Latenzstabilitaet unter Mischlast

### Ziel

Stabile Service-Latenzen bei gleichzeitiger Last durch Query, Inference und CDC.

### Schwellwerte (eintragen)

| Metrik | SLO Ziel | Hard Limit | Istwert | Ergebnis |
|---|---|---|---|---|
| Query p99 | TBD ms | TBD ms | TBD | TBD |
| Inference p95 | TBD ms | TBD ms | TBD | TBD |
| CDC End-to-End p95 | TBD ms | TBD ms | TBD | TBD |
| Queue-Latenz CDC->RAG p95 | TBD ms | TBD ms | TBD | TBD |

### Evidence

- Testlauf-ID: TBD
- Artefakt/Log-Pfad: TBD
- Bemerkungen: TBD

### Gate-Entscheidung

- Status: PASS / FAIL / BLOCKED
- Begruendung: TBD

## Gate G3: Degradationstoleranz (RAID-Rebuild + Shard-Repair)

### Ziel

Kontrollierter Betrieb bei degraded Storage und paralleler Hintergrundarbeit.

### Schwellwerte (eintragen)

| Metrik | SLO Ziel | Hard Limit | Istwert | Ergebnis |
|---|---|---|---|---|
| Datenverlust-Ereignisse | 0 | 0 | TBD | TBD |
| Query p99 waehrend Rebuild | TBD ms | TBD ms | TBD | TBD |
| Repair-Backlog Peak | TBD | TBD | TBD | TBD |
| SLA-Verletzungen gesamt | TBD | TBD | TBD | TBD |

### Evidence

- Testlauf-ID: TBD
- Artefakt/Log-Pfad: TBD
- Bemerkungen: TBD

### Gate-Entscheidung

- Status: PASS / FAIL / BLOCKED
- Begruendung: TBD

## Gate G4: Recovery (Crash + AdaLoRA Rollback)

### Ziel

Deterministische Wiederherstellung und geordneter Rollback im Lastbetrieb.

### Schwellwerte (eintragen)

| Metrik | Zielwert | Hard Limit | Istwert | Ergebnis |
|---|---|---|---|---|
| RTO (Crash Recovery) | TBD min | TBD min | TBD | TBD |
| RPO | TBD | TBD | TBD | TBD |
| Rollback Zeit Adapter-Release | TBD min | TBD min | TBD | TBD |
| Recovery-Fehlversuche | 0 | 0 | TBD | TBD |

### Evidence

- Testlauf-ID: TBD
- Artefakt/Log-Pfad: TBD
- Bemerkungen: TBD

### Gate-Entscheidung

- Status: PASS / FAIL / BLOCKED
- Begruendung: TBD

## Gate G5: Security fail-closed

### Ziel

Bei Policy/Secret/TLS Fehlern darf kein fail-open Verhalten auftreten.

### Schwellwerte (eintragen)

| Metrik | Zielwert | Hard Limit | Istwert | Ergebnis |
|---|---|---|---|---|
| fail-open Events | 0 | 0 | TBD | TBD |
| Unerlaubte Requests trotz Policy-Fehler | 0 | 0 | TBD | TBD |
| Secret/TLS Fehler ohne deny | 0 | 0 | TBD | TBD |
| Mean Time to Detect (MTTD) | TBD min | TBD min | TBD | TBD |

### Evidence

- Testlauf-ID: TBD
- Artefakt/Log-Pfad: TBD
- Bemerkungen: TBD

### Gate-Entscheidung

- Status: PASS / FAIL / BLOCKED
- Begruendung: TBD

## Gate G6: Multi-Region Konsistenz

### Ziel

Deterministische Konfliktaufloesung bei Partition/Rejoin/Clock-Skew und konsistente Modellversionen.

### Schwellwerte (eintragen)

| Metrik | Zielwert | Hard Limit | Istwert | Ergebnis |
|---|---|---|---|---|
| Konfliktaufloesung deterministisch | 100% | >= TBD% | TBD | TBD |
| Inkonsistente Modellversionen nach Rejoin | 0 | 0 | TBD | TBD |
| Clock-Drift ueber Schwellwert | TBD | TBD | TBD | TBD |
| Rejoin-Stabilisierung (Zeit) | TBD min | TBD min | TBD | TBD |

### Evidence

- Testlauf-ID: TBD
- Artefakt/Log-Pfad: TBD
- Bemerkungen: TBD

### Gate-Entscheidung

- Status: PASS / FAIL / BLOCKED
- Begruendung: TBD

## Gesamtentscheidung

### Gate-Status Uebersicht

| Gate | Status | Blocker | Owner | Deadline fuer Re-Test |
|---|---|---|---|---|
| G1 | TBD | TBD | TBD | TBD |
| G2 | TBD | TBD | TBD | TBD |
| G3 | TBD | TBD | TBD | TBD |
| G4 | TBD | TBD | TBD | TBD |
| G5 | TBD | TBD | TBD | TBD |
| G6 | TBD | TBD | TBD | TBD |

### Freigabelogik

- GO: Alle Gates PASS.
- CONDITIONAL GO: Keine FAIL-Gates, aber BLOCKED mit akzeptiertem Risiko und Terminplan.
- NO-GO: Mindestens ein FAIL-Gate.

### Finale Entscheidung

- Freigabestatus: GO / CONDITIONAL GO / NO-GO
- Entscheidung am: TBD
- Verantwortlich: TBD
- Auflagen: TBD

## Mapping zur Matching-Analyse

- G1 <-> Ressourcenstabilitaet und Linux-Isolation.
- G2 <-> Latenzstabilitaet unter Mischlast.
- G3 <-> Degradationstoleranz bei RAID/Sharding/Repair.
- G4 <-> Recovery und AdaLoRA-Rollback.
- G5 <-> Security fail-closed.
- G6 <-> Multi-Region Konfliktkonsistenz.

## Ausfuellhinweise

- Alle Werte in Zielumgebung messen, keine Labor-Schaetzwerte eintragen.
- Jede Kennzahl muss auf reproduzierbare Testlaeufe verweisen.
- Bei FAIL immer Root-Cause, Gegenmassnahme und Re-Test-Termin erfassen.
- Bei CONDITIONAL GO muessen Risiko und Ablaufdatum dokumentiert sein.
