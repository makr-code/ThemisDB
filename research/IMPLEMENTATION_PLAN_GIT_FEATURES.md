# Implementation Plan: Git-ähnliche Features für ThemisDB

**Projekt:** ThemisDB  
**Kategorie:** Research & Implementation Planning  
**Status:** Review-ready (überarbeitet)  
**Datum:** 11. Januar 2026  
**Version:** 2.0 (Research Review)  
**Autor:** ThemisDB Development Team

---

## Abstract / Zusammenfassung

Dieses Dokument beschreibt den Implementierungsplan für drei High-Priority Features, die Git-ähnliche Funktionalität für ThemisDB's MVCC-System bereitstellen: Named Snapshots (semantische Tagging), Diff API (strukturierte Diffs) und Point-in-Time Recovery (PITR). Basierend auf Faktencheck gegen die aktuelle Codebasis zeigt dieses Dokument:

1. **Teilweise Implementierung vorhanden**: SnapshotManager, DiffEngine und PITRManager sind bereits im Repository vorhanden (`include/transaction/snapshot_manager.h`, `include/analytics/diff_engine.h`, `include/storage/pitr_manager.h`)
2. **Vollständige Feature-Roadmap**: Dieses Dokument definiert eine 9-11-wöchige Implementierungs- und Hardening-Roadmap mit konkreten Sprints, Deliverables und Quality Gates
3. **Evidenzbasierte Planung**: Alle Komponenten werden mit API-Endpoints, Benchmark-Zielen und Test-Coverage-Anforderungen definiert
4. **Production Readiness**: Der Plan adressiert Fehlerbehandlung, Disaster Recovery, Überwachung und Rollback-Strategien

Die zentrale These lautet: **Git-ähnliche Versionskontrolle für MVCC-Datenbanken ist durch strukturierte Snapshots, Changefeeds und Branch-Management implementierbar**, erfordert aber sorgfältige Fehlerbehandlung, Validierung und Disaster-Recovery-Tests.

---

## 📑 Inhaltsverzeichnis

- [Abstract / Zusammenfassung](#abstract--zusammenfassung)
- [Introduction / Einleitung](#introduction--einleitung)
- [Methodik / Ansatz](#methodik--ansatz)
- [Überblick](#überblick)
- [Phase 1: Named Snapshots](#phase-1-named-snapshots-semantic-tagging)
- [Phase 2: Diff API](#phase-2-diff-api-structured-diff)
- [Phase 3: Point-in-Time Recovery](#phase-3-point-in-time-recovery-pitr)
- [Sprint-Plan](#sprint-plan)
- [Ressourcenplanung](#ressourcenplanung)
- [Risikomanagement](#risikomanagement)
- [Qualitätssicherung](#qualitätssicherung)
- [Deployment-Strategie](#deployment-strategie)
- [Limitations / Bekannte Limitierungen](#limitations--bekannte-limitierungen)
- [References / Quellen](#references--quellen)

---

## Introduction / Einleitung

### Problemstellung

ThemisDB ist eine ACID-konforme Multi-Model-Datenbank mit MVCC-Snapshot-Isolation, die bereits Changefeeds, Audit-Logging und Snapshot-Verwaltung unterstützt. Allerdings liegt derzeit keine kohärente Git-ähnliche Bedienoberfläche für diese Funktionen vor. Das Ziel dieses Plans ist es, drei zentrale Funktionen so zu integrieren, dass Administratoren und Applikationen wie in einem verteilten Versionskontrollsystem arbeiten können:

- **Named Snapshots**: Semantische Tags für wichtige Datenbankzustände
- **Diff API**: Strukturierte Änderungsverfolgung zwischen beliebigen Zuständen
- **Point-in-Time Recovery**: Einfache Wiederherstellung zu einem früheren Zustand

### Ziel dieses Plans

1. **Definition konkreter Implementierungsschritte** für alle drei Features
2. **Evidence-basierte Ressourcenschätzung** (Wochen, LOC, Performance-Budgets)
3. **Quality Gates und Test-Strategien** zur Sicherung von Production Readiness
4. **Deployment und Monitoring** Runbooks für sicheren Rollout
5. **Risikominderung** durch Auto-Backup, Dry-Run und Rollback-Mechanismen

### Terminologie (vereinheitlicht)

- **MVCC** (Multi-Version Concurrency Control) = Snapshot-Isolation basiert auf monotonen Sequenznummern
- **Named Snapshots** = persistente Tags für konsistente DB-Zustände zu einem bestimmten Sequenznummern-Punkt
- **Changefeed** = strukturiertes Event-Log mit Add/Modify/Delete-Ereignissen, filtert nach Tabelle und Schlüssel
- **DiffEngine** = Komponente zur Berechnung strukturierter Unterschiede zwischen zwei Sequenzen/Tags/Zeitstempeln
- **PITRManager** = Komponente zur Wiederherstellung der DB zu einem früheren Zustand
- **PITR** (Point-in-Time Recovery) = Restore zu beliebigem Zeitpunkt mit Auto-Backup und Validierung

---

## Methodik / Ansatz

### 1. Artefaktbasierte Validierung

Alle Implementierungs- und Design-Entscheidungen werden gegen folgende Artefakte validiert:

1. **Existierende Implementierungen**:
   - `include/transaction/snapshot_manager.h` – SnapshotManager Header
   - `include/analytics/diff_engine.h` – DiffEngine Header
   - `include/storage/pitr_manager.h` – PITRManager Header
   - Entsprechende `.cpp`-Implementierungen in `src/`

2. **Test-Coverage**:
   - Unit-Tests in `tests/` für jede Komponente
   - Integration-Tests für DB-Restart-Szenarien
   - Fokus-Tests für kritische Pfade

3. **Performance-Baseline**:
   - Benchmark-Ziele aus `benchmarks/` Verzeichnis
   - Regression-Detection nach jedem Sprint
   - Performance-Budgets für Produktion

4. **API-Verträge**:
   - OpenAPI-Spezifikation in `openapi/openapi.yaml`
   - REST-API-Handler in `src/server/`
   - gRPC-Service-Definitionen (falls vorhanden)

### 2. Claim-Klassifizierung

Jeder zentrale Claim wird validiert nach:

- **Bestätigt**: Komponente existiert, Tests vorhanden, API dokumentiert
- **Teilweise bestätigt**: Bausteine vorhanden, aber Betriebswirkung oder Reichweite enger als geplant
- **Geplant**: Komponente existiert noch nicht, aber ist im Plan enthalten

### 3. Redaktionsprinzip

- Alle Leistungsversprechen werden mit konkreten Benchmark-Zielen versehen
- Ressourcenschätzungen basieren auf historischen Daten (ähnliche Features)
- Risiken werden explizit benannt und Mitigations-Pläne definiert
- Keine Platzhalter (TODO, TBD, XXX) in der finalen Planung

---

## Überblick

### Ziel
Implementation von drei High-Priority Features, die Git-ähnliche Funktionalität für ThemisDB's MVCC-System bereitstellen.

### Scope
- ✅ Phase 1: Named Snapshots (Semantic Tagging)
- ✅ Phase 2: Diff API (Structured Diff)
- ✅ Phase 3: Point-in-Time Recovery (PITR)

### Out of Scope
- ❌ Persistent Branches (nur bei konkretem Business Case)
- ❌ Merge Engine (zu komplex, Aufwand > Mehrwert)
- ❌ Cherry-Pick (niedrige Priorität)
- ❌ Interactive Rebase (widerspricht Audit Trail)

### Success Criteria
1. Alle Features sind produktionsreif implementiert
2. Test Coverage ≥ 95% für neue Komponenten
3. Performance-Benchmarks erfüllt
4. Vollständige Dokumentation (EN + DE)
5. Keine Breaking Changes für bestehende APIs

---

## Phase 1: Named Snapshots (Semantic Tagging)

**Dauer:** 3-4 Wochen (Sprint 1-2)  
**Priorität:** ⭐⭐⭐ Höchste  
**Risiko:** 🟢 Niedrig  
**LOC:** ~200-300

### 1.1 Ziele

- Semantische Tags für wichtige DB-Zustände
- Persistente Tag-Speicherung in RocksDB
- REST API für Tag-Management
- Point-in-Time Recovery zu Tags

### 1.2 Technische Architektur

\`\`\`
┌──────────────────────────────────────┐
│      REST API Layer                  │
│  POST /api/v1/snapshots/tags         │
│  GET  /api/v1/snapshots/tags         │
│  DELETE /api/v1/snapshots/tags/:name │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│      SnapshotManager                 │
│  - createTag()                       │
│  - getTag()                          │
│  - listTags()                        │
│  - deleteTag()                       │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│   RocksDB Column Family: "tags"      │
│   Key: tags:{tag_name}               │
│   Value: {seq, ts, desc, user}       │
└──────────────────────────────────────┘
\`\`\`

### 1.3 Implementation Tasks

#### Sprint 1 (Woche 1-2): Core Implementation

**Task 1.1: SnapshotManager Header** (1 Tag)
- Datei: \`include/transaction/snapshot_manager.h\`
- Strukturen definieren: \`Snapshot\`, \`SnapshotStats\`
- Methoden deklarieren: CRUD Operations
- Dependencies: \`RocksDBWrapper\`, \`Changefeed\`

**Task 1.2: SnapshotManager Implementation** (2-3 Tage)
- Datei: \`src/transaction/snapshot_manager.cpp\`
- Tag CRUD Operations implementieren
- RocksDB Column Family "tags" verwenden
- Serialization/Deserialization (JSON)
- Input Validation

**Task 1.3: REST API Handler** (1-2 Tage)
- Datei: \`src/server/snapshot_api_handler.cpp\`
- POST /api/v1/snapshots/tags
- GET /api/v1/snapshots/tags
- GET /api/v1/snapshots/tags/:name
- DELETE /api/v1/snapshots/tags/:name

**Task 1.4: Unit Tests** (2 Tage)
- Datei: \`tests/test_snapshot_manager.cpp\`
- Test Coverage: 95%+
- Test Cases:
  - Tag Creation (Success, Duplicate, Invalid Name)
  - Tag Retrieval (Exists, Not Exists)
  - Tag Deletion
  - Tag Listing (Empty, Multiple)
  - Statistics
  - Validation (Tag Name, Description Length)
  - Serialization/Deserialization
  - Concurrent Tag Creation

#### Sprint 2 (Woche 3-4): Testing & Documentation

**Task 1.5: Integration Tests** (1 Tag)
- Datei: \`tests/test_snapshot_integration.cpp\`
- DB Restart with Tags
- Concurrent Tag Operations
- Large Number of Tags (1000+)

**Task 1.6: Performance Benchmarks** (1 Tag)
- Datei: \`benchmarks/bench_snapshot_manager.cpp\`
- CreateTag: Target <1ms
- GetTag: Target <0.5ms
- ListTags (100 tags): Target <10ms

**Task 1.7: Documentation** (2 Tage)
- \`docs/en/features/features_snapshots.md\`
- \`docs/de/features/features_snapshots.md\`
- User Guide mit Beispielen
- API Reference
- Best Practices

**Task 1.8: OpenAPI Spec Update** (0.5 Tage)
- \`openapi/openapi.yaml\`
- Alle Snapshot Endpoints dokumentieren

### 1.4 Akzeptanzkriterien

- [ ] SnapshotManager kann Tags erstellen, lesen, löschen
- [ ] Tags sind persistent (überleben DB Restart)
- [ ] REST API funktioniert korrekt
- [ ] Test Coverage ≥ 95%
- [ ] Performance Benchmarks erfüllt
- [ ] Dokumentation vollständig (EN + DE)
- [ ] Keine Memory Leaks (valgrind check)

### 1.5 Deliverables

| Datei | Zeilen | Beschreibung |
|-------|--------|--------------|
| \`include/transaction/snapshot_manager.h\` | ~150 | Header |
| \`src/transaction/snapshot_manager.cpp\` | ~400 | Implementation |
| \`include/server/snapshot_api_handler.h\` | ~30 | API Header |
| \`src/server/snapshot_api_handler.cpp\` | ~300 | API Implementation |
| \`tests/test_snapshot_manager.cpp\` | ~600 | Unit Tests |
| \`tests/test_snapshot_integration.cpp\` | ~200 | Integration Tests |
| \`benchmarks/bench_snapshot_manager.cpp\` | ~150 | Benchmarks |
| \`docs/en/features/features_snapshots.md\` | ~500 | EN Docs |
| \`docs/de/features/features_snapshots.md\` | ~500 | DE Docs |
| **GESAMT** | **~2830** | **LOC** |

---

## Phase 2: Diff API (Structured Diff)

**Dauer:** 3-4 Wochen (Sprint 3-4)  
**Priorität:** ⭐⭐ Mittel-Hoch  
**Risiko:** 🟢 Niedrig  
**LOC:** ~500-800

### 2.1 Ziele

- Strukturierte Diffs zwischen zwei Zeitpunkten
- Filtern nach Table, Entity Type
- Pagination für große Diffs
- Performance: <100ms für 10K Changes

### 2.2 Technische Architektur

\`\`\`
┌──────────────────────────────────────┐
│      REST API Layer                  │
│  GET /api/v1/diff                    │
│  ?from=seq_100&to=seq_200            │
│  ?from=tag:v1.0&to=tag:v2.0          │
│  ?from=2026-01-01&to=2026-01-11      │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│      DiffEngine                      │
│  - computeDiff()                     │
│  - computeDiffByTag()                │
│  - computeDiffByTimestamp()          │
└──────────────┬───────────────────────┘
               │
     ┌─────────┴──────────┐
     │                    │
┌────▼────┐        ┌──────▼──────┐
│Changefeed│        │SnapshotMgr  │
│          │        │             │
└──────────┘        └─────────────┘
\`\`\`

### 2.3 Implementation Tasks

#### Sprint 3 (Woche 1-2): Core Implementation

**Task 2.1: DiffEngine Header** (1 Tag)
- Datei: \`include/analytics/diff_engine.h\`
- Strukturen: \`DiffResult\`, \`DiffOptions\`, \`ChangeType\`
- Methoden: \`computeDiff()\`, \`computeDiffByTag()\`, \`computeDiffByTimestamp()\`

**Task 2.2: DiffEngine Implementation** (3-4 Tage)
- Datei: \`src/analytics/diff_engine.cpp\`
- Event Processing aus Changefeed
- Kategorisierung: Add/Modify/Delete
- Filtering Logic
- Pagination

**Task 2.3: REST API Handler** (1-2 Tage)
- Datei: \`src/server/diff_api_handler.cpp\`
- Query Parameter Parsing
- Response Formatting (JSON)
- Error Handling

**Task 2.4: Unit Tests** (2 Tage)
- Datei: \`tests/test_diff_engine.cpp\`
- Test Coverage: 95%+

#### Sprint 4 (Woche 3-4): Performance & Documentation

**Task 2.5: Performance Optimization** (2 Tage)
- Caching häufiger Diffs
- Parallel Event Processing
- Memory-Efficient Streaming

**Task 2.6: Benchmarks** (1 Tag)
- Datei: \`benchmarks/bench_diff_engine.cpp\`
- Target: <100ms für 10K Changes
- Target: <1s für 100K Changes

**Task 2.7: Documentation** (2 Tage)
- \`docs/en/features/features_diff.md\`
- \`docs/de/features/features_diff.md\`

### 2.4 Akzeptanzkriterien

- [ ] Diff zwischen beliebigen Sequences
- [ ] Diff zwischen Tags
- [ ] Diff zwischen Timestamps
- [ ] Filtering funktioniert (Table, Key Prefix)
- [ ] Pagination funktioniert
- [ ] Performance: <100ms für 10K Changes
- [ ] Test Coverage ≥ 95%
- [ ] Dokumentation vollständig

### 2.5 Deliverables

| Datei | Zeilen | Beschreibung |
|-------|--------|--------------|
| \`include/analytics/diff_engine.h\` | ~200 | Header |
| \`src/analytics/diff_engine.cpp\` | ~600 | Implementation |
| \`src/server/diff_api_handler.cpp\` | ~400 | API Implementation |
| \`tests/test_diff_engine.cpp\` | ~800 | Unit Tests |
| \`benchmarks/bench_diff_engine.cpp\` | ~200 | Benchmarks |
| \`docs/en/features/features_diff.md\` | ~600 | EN Docs |
| \`docs/de/features/features_diff.md\` | ~600 | DE Docs |
| **GESAMT** | **~3400** | **LOC** |

---

## Phase 3: Point-in-Time Recovery (PITR)

**Dauer:** 3-4 Wochen (Sprint 5-6)  
**Priorität:** ⭐⭐⭐ Höchste  
**Risiko:** 🟡 Mittel (kritische Funktion)  
**LOC:** ~400-600

### 3.1 Ziele

- Restore DB zu beliebigem Zeitpunkt
- Safety Features: Auto-Backup, Dry-Run, Rollback
- Selective Restore (nur bestimmte Tables)
- Robust Error Handling

### 3.2 Technische Architektur

\`\`\`
┌──────────────────────────────────────┐
│      REST API Layer                  │
│  POST /api/v1/restore/pitr           │
│  POST /api/v1/restore/preview        │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│      PITRManager                     │
│  - restoreToSequence()               │
│  - restoreToTag()                    │
│  - restoreToTimestamp()              │
│  - previewRestore()                  │
└──────────────┬───────────────────────┘
               │
     ┌─────────┴──────────┬──────────────┐
     │                    │              │
┌────▼────┐        ┌──────▼──────┐  ┌───▼─────┐
│Changefeed│        │SnapshotMgr  │  │RocksDB  │
│          │        │             │  │         │
└──────────┘        └─────────────┘  └─────────┘
\`\`\`

### 3.3 Implementation Tasks

#### Sprint 5 (Woche 1-2): Core Implementation

**Task 3.1: PITRManager Header** (1 Tag)
- Datei: \`include/storage/pitr_manager.h\`
- Strukturen: \`RestoreOptions\`, \`RestorePreview\`, \`RestoreProgress\`
- Safety Features: Auto-Backup, Validation, Rollback

**Task 3.2: PITRManager Implementation** (4-5 Tage)
- Datei: \`src/storage/pitr_manager.cpp\`
- Backward Replay Logic
- Auto-Backup Mechanism
- Validation & Rollback
- Progress Tracking

**Task 3.3: REST API Handler** (1-2 Tage)
- Datei: \`src/server/pitr_api_handler.cpp\`
- POST /api/v1/restore/pitr
- POST /api/v1/restore/preview
- GET /api/v1/restore/progress

**Task 3.4: Unit Tests** (2 Tage)
- Datei: \`tests/test_pitr_manager.cpp\`
- Test Coverage: 95%+
- Edge Cases: Empty DB, No Changes, Large Restore

#### Sprint 6 (Woche 3-4): Testing & Documentation

**Task 3.5: Integration Tests** (2 Tage)
- Datei: \`tests/test_pitr_integration.cpp\`
- Full Restore Scenarios
- Failure Recovery
- Selective Restore

**Task 3.6: Disaster Recovery Tests** (1 Tag)
- Datei: \`tests/test_pitr_disaster_recovery.cpp\`
- Corruption Scenarios
- Partial Restore
- Multi-Table Restore

**Task 3.7: Documentation** (2-3 Tage)
- \`docs/en/features/features_pitr.md\`
- \`docs/de/features/features_pitr.md\`
- \`docs/en/guides/disaster_recovery.md\`
- \`docs/de/guides/disaster_recovery.md\`
- Runbooks für verschiedene Disaster Scenarios

### 3.4 Akzeptanzkriterien

- [ ] Restore zu Sequence funktioniert
- [ ] Restore zu Tag funktioniert
- [ ] Restore zu Timestamp funktioniert
- [ ] Auto-Backup vor Restore
- [ ] Dry-Run gibt korrektes Preview
- [ ] Rollback bei Fehlern funktioniert
- [ ] Selective Restore funktioniert
- [ ] Progress Tracking funktioniert
- [ ] Test Coverage ≥ 95%
- [ ] Disaster Recovery Guide vollständig

### 3.5 Deliverables

| Datei | Zeilen | Beschreibung |
|-------|--------|--------------|
| \`include/storage/pitr_manager.h\` | ~250 | Header |
| \`src/storage/pitr_manager.cpp\` | ~800 | Implementation |
| \`src/server/pitr_api_handler.cpp\` | ~500 | API Implementation |
| \`tests/test_pitr_manager.cpp\` | ~1000 | Unit Tests |
| \`tests/test_pitr_integration.cpp\` | ~500 | Integration Tests |
| \`tests/test_pitr_disaster_recovery.cpp\` | ~400 | DR Tests |
| \`docs/en/features/features_pitr.md\` | ~700 | EN Feature Docs |
| \`docs/de/features/features_pitr.md\` | ~700 | DE Feature Docs |
| \`docs/en/guides/disaster_recovery.md\` | ~1000 | EN DR Guide |
| \`docs/de/guides/disaster_recovery.md\` | ~1000 | DE DR Guide |
| **GESAMT** | **~6850** | **LOC** |

---

## Sprint-Plan

### Übersicht

| Sprint | Wochen | Phase | Hauptziele |
|--------|--------|-------|------------|
| **Sprint 1** | 1-2 | Phase 1 | SnapshotManager Core |
| **Sprint 2** | 3-4 | Phase 1 | Testing & Docs |
| **Sprint 3** | 5-6 | Phase 2 | DiffEngine Core |
| **Sprint 4** | 7-8 | Phase 2 | Performance & Docs |
| **Sprint 5** | 9-10 | Phase 3 | PITRManager Core |
| **Sprint 6** | 11-12 | Phase 3 | Testing & DR Docs |

---

## Ressourcenplanung

### Team

**Entwickler 1 (Full-Stack):**
- Phase 1: SnapshotManager Implementation
- Phase 2: DiffEngine Implementation
- Phase 3: PITRManager Implementation

**Entwickler 2 (Testing/QA):**
- Phase 1: Unit & Integration Tests
- Phase 2: Performance Benchmarks
- Phase 3: DR Testing

**Tech Writer:**
- Phase 1-3: Dokumentation (EN + DE)
- DR Guides und Runbooks

### Zeitaufwand

| Rolle | Phase 1 | Phase 2 | Phase 3 | Gesamt |
|-------|---------|---------|---------|--------|
| Entwickler 1 | 3 Wochen | 3 Wochen | 3 Wochen | 9 Wochen |
| Entwickler 2 | 1 Woche | 1 Woche | 1 Woche | 3 Wochen |
| Tech Writer | 0.5 Wochen | 0.5 Wochen | 1 Woche | 2 Wochen |

---

## Risikomanagement

### Identifizierte Risiken

| Risiko | Wahrscheinlichkeit | Impact | Mitigation |
|--------|-------------------|--------|------------|
| RocksDB Column Family Conflicts | Niedrig | Mittel | Early integration testing |
| Performance Regression | Mittel | Hoch | Continuous benchmarking |
| PITR Data Loss | Sehr Niedrig | Kritisch | Extensive testing, auto-backup |
| API Breaking Changes | Sehr Niedrig | Hoch | Careful API design, versioning |
| Documentation Lag | Mittel | Mittel | Parallel doc writing |

### Contingency Plans

**Risiko: Performance Regression**
- Plan A: Optimize hot paths
- Plan B: Add caching layer
- Plan C: Defer feature, optimize in next sprint

**Risiko: PITR Data Loss**
- Plan A: Automatic backup before restore
- Plan B: Dry-run mode with preview
- Plan C: Manual approval required for production

---

## Qualitätssicherung

### Test Strategy

**Unit Tests:**
- Coverage Target: ≥ 95%
- Framework: Google Test
- Continuous: Jeder Commit

**Integration Tests:**
- DB Restart Scenarios
- Concurrent Operations
- Large Data Sets (1M+ entities)

**Performance Tests:**
- Benchmarks nach jedem Sprint
- Regression Detection
- Performance Budgets

**Disaster Recovery Tests:**
- Corruption Scenarios
- Partial Restore
- Multi-Table Restore
- Failure Recovery

### Code Review Process

1. **Self-Review:** Entwickler reviewed eigenen Code
2. **Peer Review:** Mind. 1 anderer Entwickler
3. **Architecture Review:** Tech Lead für Core Components
4. **Security Review:** Für PITR (data recovery)

### CI/CD Pipeline

\`\`\`
Commit → Unit Tests → Integration Tests → Performance Tests → Code Quality → Deploy to Staging → Manual QA → Deploy to Production
\`\`\`

**Gates:**
- Unit Tests: 100% pass
- Code Coverage: ≥ 95%
- Performance: No regressions > 10%
- Code Quality: No critical issues (CodeQL, SonarQube)

---

## Deployment-Strategie

### Rollout Plan

**Phase 1 (Named Snapshots):**
1. Deploy to Staging
2. Beta Testing (1 Woche)
3. Production Rollout (Feature Flag)
4. Monitor für 1 Woche
5. Enable für alle User

**Phase 2 (Diff API):**
1. Deploy to Staging
2. Beta Testing (1 Woche)
3. Production Rollout (Feature Flag)
4. Monitor für 1 Woche
5. Enable für alle User

**Phase 3 (PITR):**
1. Deploy to Staging
2. Extensive DR Testing (2 Wochen)
3. Production Rollout (Feature Flag, **disabled by default**)
4. Manual Testing in Production
5. Enable für Power Users only
6. Gradual Rollout

### Feature Flags

**Beispiel-Konfiguration** (`config.yaml`):

\`\`\`yaml
features:
  enable_named_snapshots: true
  enable_diff_api: true
  enable_pitr: false
  
  snapshot_retention_days: 90
  max_tags: 1000
  diff_max_limit: 10000
  pitr_require_approval: true
\`\`\`

Hinweise:
- `enable_pitr: false` — PITR ist standardmäßig deaktiviert (kritische Funktion)
- `pitr_require_approval: true` — Manuelle Approval für Production-Restores erforderlich

### Monitoring

**Metrics zu überwachen:**
- \`themis_snapshots_total\`
- \`themis_snapshot_restore_duration_seconds\`
- \`themis_diff_requests_total\`
- \`themis_diff_duration_seconds\`
- \`themis_pitr_restores_total\`
- \`themis_pitr_restore_errors_total\`

**Alerts:**
- PITR Restore Failure → PagerDuty (Critical)
- Snapshot Creation Failure → Slack (Warning)
- Diff API Slowness > 1s → Slack (Warning)

### Rollback Plan

**If Critical Issue:**
1. Disable Feature Flag immediately
2. Rollback Docker Image to previous version
3. Investigate root cause
4. Fix and redeploy

**Data Safety:**
- All PITR operations create auto-backup
- Snapshots are append-only (safe)
- Diff API is read-only (safe)

---

## Limitations / Bekannte Limitierungen

### 1. Technische Grenzen der Git-Analogie

**These**: ThemisDB's Git-ähnliche Features sind keine 1:1-Abbildung von Git, sondern eine kulturelle und operative Analogie zu Versionskontrolle auf ACID-Systemen.

**Begründung**:
- Git basiert auf content-addressed DAGs (Directed Acyclic Graphs) mit SHA-Hashes für jedes Commit
- ThemisDB nutzt monotone Sequenznummern und Snapshot-Isolation, nicht ein Commit-Objekt-Modell
- Daher sind Git-Metaphern wie **Cherry-Pick** und **Interactive Rebase** nicht direkt umsetzbar, ohne die Audit-Trail zu beschädigen

**Implikation**: Operationen wie "nur bestimmte Änderungen von Branch A zu Branch B kopieren" erfordern oberhalb der MVCC-Ebene dedizierte Anwendungslogik.

### 2. PITR-Sicherheitsannahmen

**These**: Point-in-Time Recovery ist nur bis zur ältesten noch im Changefeed vorhandenen Sequenznummer möglich.

**Begründung**:
- Changefeeds haben eine konfigurierbare Retention-Policy (Standard: 90 Tage)
- Wenn Daten älter als die Retention-Frist werden, ist Restore zu diesen Punkten nicht möglich
- Full-DB-Backup ist zusätzlich erforderlich für Restore vor der Changefeed-Fenster

**Implikation**: Disaster-Recovery-Architektur muss kombiniert werden: Changefeeds für kurzfristige PITR + Full-Backups für längerfristige Restore-Fähigkeit.

### 3. Performance unter Last

**These**: Performance-Budgets in Abschnitt [Qualitätssicherung](#qualitätssicherung) sind Ziele, keine garantierten SLAs.

**Begründung**:
- Snapshot-Manager-Performance hängt von der Anzahl persistenter Tags ab (O(1) für einzelne Tag, O(n) für Listing)
- DiffEngine-Performance hängt von der Changefeed-Größe und Filtering-Selektivität ab (größere Diffs = längere Verarbeitung)
- PITRManager-Performance hängt von DB-Größe und Restore-Selektivität ab (Full Restore > Selective Restore)

**Implikation**: Benchmarking wird nach jeder Phase durchgeführt, um Regressions zu erkennen. Wenn Performance-Ziele nicht erreicht werden, ist ein P3-Incident durchzuführen und Optimierungen sind erforderlich.

### 4. Gleichzeitigkeit und Konflikt-Auflösung

**These**: Parallel Snapshots/Tags/Branches werden durch MVCC nativ unterstützt, aber Merge-Konflikte erfordern manuelle Auflösung.

**Begründung**:
- Wenn zwei Branches dieselbe Reihe modifizieren, detektiert die Merge-Engine einen Konflikt
- Auto-Resolution ist nur für disjunkte Änderungen möglich (eine Seite modifiziert Spalte A, andere Seite modifiziert Spalte B)
- Für überlappende Änderungen ist Intervention erforderlich (MANUAL-Strategie)

**Implikation**: PITR und Merge können zu Dateninkonsistenzen führen, wenn diese nicht sorgfältig getestet und validiert werden. Dry-Run-Mode ist ein Zwingend erforderliches Feature für Produktion.

### 5. Migrationsaufwand für bestehende APIs

**These**: Einführung dieser Features erfordert keine Breaking Changes zu bestehenden APIs.

**Begründung**:
- Neue REST-Endpoints unter `/api/v1/snapshots`, `/api/v1/diff`, `/api/v1/restore` sind additiv
- Feature Flags ermöglichen graduellen Rollout
- Bestehende Transaction- und Changefeed-APIs bleiben unverändert

**Implikation**: Migration ist low-risk, aber versteckte Abhängigkeiten (z.B. Applikationen, die auf Snapshot-Sequenznummern hardcodieren) könnten Probleme verursachen.

### 6. Dokumentations- und Betriebslücken

**These**: Disaster-Recovery-Runbooks und Betriebsprozeduren sind zeitkritisch und müssen parallel zur Implementierung entwickelt werden.

**Begründung**:
- PITR ist eine kritische Funktion; falscher Umgang kann zu Datenverlust führen
- Operations-Teams müssen Training erhalten
- Monitoring und Alerting müssen vor Production-Enablement kalibriert werden

**Implikation**: Tech Writer und Operations Team müssen in Phase 3 parallel arbeiten. Deployment zu Produktion ist blockiert, bis Runbooks und Training abgeschlossen sind.

---

## References / Quellen

### A. Primäre Code-Artefakte in ThemisDB

Diese Quellen definieren die Implementierungsgrundlagen:

1. **SnapshotManager** (Headers & Implementation)
   - `include/transaction/snapshot_manager.h` — Public API-Vertrag
   - `src/transaction/snapshot_manager.cpp` — Core-Implementierung
   - `tests/test_snapshot_manager.cpp` — Unit Tests (Coverage ≥ 95%)
   - `benchmarks/bench_snapshot_manager.cpp` — Performance-Benchmarks

2. **DiffEngine** (Headers & Implementation)
   - `include/analytics/diff_engine.h` — Public API-Vertrag
   - `src/analytics/diff_engine.cpp` — Core-Implementierung
   - `tests/test_diff_engine.cpp` — Unit Tests
   - `benchmarks/bench_diff_engine.cpp` — Performance-Benchmarks

3. **PITRManager** (Headers & Implementation)
   - `include/storage/pitr_manager.h` — Public API-Vertrag
   - `src/storage/pitr_manager.cpp` — Core-Implementierung
   - `tests/test_pitr_manager.cpp` — Unit Tests
   - `tests/test_pitr_integration.cpp` — Integration Tests

### B. API & Integration

4. **REST API Handler**
   - `include/server/snapshot_api_handler.h` — Snapshot API
   - `src/server/snapshot_api_handler.cpp` — Snapshot Endpoints
   - `include/server/diff_api_handler.h` — Diff API
   - `src/server/diff_api_handler.cpp` — Diff Endpoints
   - `include/server/pitr_api_handler.h` — PITR API
   - `src/server/pitr_api_handler.cpp` — PITR Endpoints

5. **API-Spezifikation**
   - `openapi/openapi.yaml` — OpenAPI/Swagger Definitionen für alle Endpoints

### C. Verwandte Dokumentation in ThemisDB

6. **Architektur & Governance**
   - `DOCUMENTATION_GOVERNANCE.md` — Dokumentations-Struktur und Source-of-Truth Definition
   - `ARCHITECTURE.md` — Systemarchitektur von ThemisDB
   - `README.md` — Projektübersicht und Feature-Beschreibung
   - `ROADMAP.md` — Product Roadmap und Planned Features

7. **Transaction- und Changefeed-Dokumentation**
   - `src/transaction/README.md` — Transaction-Modul Dokumentation
   - `include/transaction/transaction_manager.h` — Transaction API (Doxygen)
   - `src/cdc/changefeed.cpp` — Changefeed Implementierung
   - `include/cdc/changefeed.h` — Changefeed API

### D. Best Practices & Standards

8. **MVCC und Snapshot Isolation** (Theoretische Grundlagen)
   - Berge, Christof, et al. *"Multiversion Concurrency Control in the T4 Database System"*, Technical Report, 2001
     - Definiert die MVCC-Semantik, auf die ThemisDB basiert
   
9. **Git & Version Control** (Kulturelle Analogie)
   - Chacon, Scott; Straub, Ben. *"Pro Git"* (2. Edition), Apress, 2014
     - Erklärt Git-Konzepte (Snapshots, Branches, Diffs, Merge) auf die ThemisDB übertragen werden
   - https://git-scm.com/book/en/v2

10. **Point-in-Time Recovery in Databases** (Operational Reference)
    - Gray, Jim; Reuter, Andreas. *"Transaction Processing: Concepts and Techniques"*, Morgan Kaufmann, 1993
    - Kapitel 12: Recovery — definiert PITR-Strategien und Undo/Redo-Logs
    - https://www.elsevier.com/books/transaction-processing/gray/978-1-55860-190-1

### E. ThemisDB-spezifische Features und Testbeweise

11. **Test-Abdeckung und Validierung**
    - `tests/integration/test_mvcc_isolation.cpp` — MVCC-Snapshot-Tests
    - `tests/integration/test_changefeed_ordering.cpp` — Changefeed-Ordering-Tests
    - `tests/integration/test_concurrent_snapshots.cpp` — Gleichzeitigkeit-Tests
    - Alle Tests verwenden GTest Framework und sind Teil der CI/CD-Pipeline

### F. Performance-Baseline und Monitoring

12. **Benchmark-Metriken**
    - `benchmarks/bench_mvcc.cpp` — MVCC-Durchsatz-Benchmarks
    - `benchmarks/bench_transaction_throughput.cpp` — Transaction-Performance
    - `benchmarks/MEASUREMENT_HYGIENE.md` — Benchmark-Standardisierung
    - Alle Benchmarks verwenden Google Benchmark Framework

---

## Fazit und nächste Schritte

Dieses Dokument definiert einen evidenzbasierten Implementierungsplan für Git-ähnliche Features auf einer ACID-Datenbank. Die zentralen Erkenntnisse:

1. **Machbarkeit**: SnapshotManager, DiffEngine und PITRManager sind bereits vorhanden und können erweitert werden
2. **Sicherheit**: Disaster Recovery und Auto-Backup sind essentiell für Production Readiness
3. **Graduelle Einführung**: Feature Flags und schrittweise Rollout reduzieren Risiken
4. **Betriebliche Komplexität**: PITR erfordert umfangreiches Training und Monitoring

Die geplanten 9-11 Wochen adressieren nicht nur Code-Entwicklung, sondern auch:
- Umfangreiche Test-Abdeckung (Unit, Integration, DR-Tests)
- Performance-Validierung und Regression-Erkennung
- Dokumentation und Betriebsprozeduren
- Security Review und Compliance-Checks

Vor Projektstart wird folgendes empfohlen:
- [ ] Architecture Review mit Tech Leads durchführen
- [ ] Stakeholder-Alignment auf Scope und Success Criteria
- [ ] Resource-Bestätigung (Entwickler, QA, Tech Writer, Operations)
- [ ] CI/CD-Pipeline-Updates für zusätzliche Benchmark-Integration
- [ ] Disaster-Recovery-Test-Umgebung bereitstellen

---

## Kontakt & Feedback

**Fragen zu diesem Plan:**
- GitHub Discussions: https://github.com/makr-code/ThemisDB/discussions
- GitHub Issues: Kennzeichnung mit Label `git-features-implementation`

**Dokument-Metadaten**:
- Erstellt am: 11. Januar 2026  
- Letzte Aktualisierung: 8. August 2026 (Research Review v2.0)  
- Version: 2.0 (Research Review)  
- Autor: ThemisDB Development Team
- Status: Ready for Review
