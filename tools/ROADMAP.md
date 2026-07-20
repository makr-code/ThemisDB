> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# ROADMAP

## Current Status
- [~] Scanner-Tuning-Programm für Gap-Analyse ist aktiv: Baseline eingefroren (TP 24%, CRITICAL-TP 50%) und Prioritätskategorien sind definiert.
- [~] Fokus ist auf `legacy_duplication`, `smart_ptr_misuse`, `memory_order`, `uncaught_exception` gesetzt; High-FP-Kategorien sind für Backlog-Eingang gedrosselt.

## In Progress
- [~] Scanner-Orchestrierung und Tuning-Policy stabilisieren (Target: 2026-Q3)
- [~] Reproduzierbare 50er-Stratifizierung + Validierungsberichte automatisieren (Target: 2026-Q3)
- [~] Validierten Remediation-Backlog mit Modulpriorität `llm -> server -> sharding` erzeugen (Target: 2026-Q3)

## Planned Features
- [ ] Batch-Gates für Delivery-Wellen vollständig in Scanner-Artefakte integrieren (Target: 2026-Q3)
- [ ] KPI-Wochentracking (Total, TP%, FP%, Severity/Kategorie/Modul) als Standardausgabe absichern (Target: 2026-Q3)
- [ ] Dokument-Sync mit Root-Governance (`ROADMAP`, `FUTURE_ENHANCEMENTS`, Release-Artefakte) fortlaufend verankern (Target: 2026-Q4)

## Implementation Phases
### Phase 1: Design / API-Vertrag
- [x] Analyse-Baseline als verbindliche Policy erfassen (TP 24%, CRITICAL 50%) (Target: 2026-Q3)
- [x] Prioritäts- und Defer-Listen für Kategorien definieren (Target: 2026-Q3)
- [ ] API-Vertrag für Scan→Sample→Analyse→Backlog-Pipeline finalisieren (Target: 2026-Q3)

### Phase 2: Core-Implementierung
- [x] Scanner-Tuning-Policy in Orchestrierung integrieren (Target: 2026-Q3)
- [x] Safe-Pattern-Whitelist und Signal-basierte HIGH/MEDIUM-Härtung implementieren (Target: 2026-Q3)
- [ ] Vollscan-Runtime für identische Datenbasis als Standard-Runbook absichern (Target: 2026-Q3)

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] High-FP-Kategorien für Remediation-Backlog-Eingang explizit drosseln (Target: 2026-Q3)
- [ ] Edge-Cases in Kategorie-/Signalheuristiken gegen Fehlklassifikation absichern (Target: 2026-Q3)
- [ ] Fallback-Verhalten bei fehlenden Files/Line-Mapping dokumentieren und testen (Target: 2026-Q3)

### Phase 4: Tests
- [~] Script-Validierung für Scanner/Sampling/Analyse automatisieren (Target: 2026-Q3)
- [ ] Reproduzierbare Sample-Regression (Seed=42) in CI-Checks aufnehmen (Target: 2026-Q3)
- [ ] Delta-Tracking gegen vorherige Reports in Pipeline automatisieren (Target: 2026-Q3)

### Phase 5: Performance/Hardening
- [ ] Scanner-Durchlaufzeit und FP-Reduktionswirkung pro Wave messbar machen (Target: 2026-Q4)
- [ ] Batch-Gates erzwingen: keine neuen CRITICAL, Tests grün, Recovery-Semantik dokumentiert (Target: 2026-Q4)
- [ ] Wave-7-Stabilität + Wave-8-Fault-Injection-Signoff in Delivery-Fluss verankern (Target: 2026-Q4)

### Phase 6: Dokumentation & Abnahme
- [~] Wöchentliche KPI-Artefakte aus Scanner-Ausgaben erzeugen (Target: 2026-Q3)
- [ ] `tools/ROADMAP.md` und `tools/FUTURE_ENHANCEMENTS.md` bei jedem Tuning-Batch synchron halten (Target: 2026-Q4)
- [ ] Batch-Abschlussreport (erledigt/offen/risiko) als Pflichtartefakt definieren (Target: 2026-Q4)

## Production Readiness Checklist
- [ ] Vollscan und Re-Scan mit identischer Datenbasis reproduzierbar ausführbar
- [ ] 50er-Stratifizierung reproduzierbar (Seed dokumentiert)
- [ ] Gesamt-TP-Ziel >= 50% erreicht; CRITICAL-Präzision >= 50% gehalten
- [ ] Validierter Remediation-Backlog priorisiert nach `llm -> server -> sharding`
- [ ] Batch-Gates pro Welle nachweislich eingehalten
- [ ] KPI- und Governance-Artefakte wöchentlich aktualisiert

## Known Issues & Limitations
- [!] Scanner-Landschaft ist zwischen `tools/` und `tools/legacy/` verteilt; Orchestrierungsimporte müssen beide Pfade berücksichtigen.
- [!] Heuristische TP/FP-Klassifikation bleibt probabilistisch und benötigt zyklische Nachkalibrierung.

## Breaking Changes
- Keine bekannten Breaking Changes dokumentiert.
