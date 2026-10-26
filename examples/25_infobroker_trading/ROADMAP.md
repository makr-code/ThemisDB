> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# ROADMAP

## Current Status
- [~] Basisversion für News-Ingestion, Signalerzeugung, Backtest und ThemisDB-Export ist implementiert.
- [~] Hybrid-Forecast-Layer (Event-Sentiment + Preisverlauf) mit Label-Evaluierung ist als Validierungsstufe integriert.
- [~] Automatisierter Abruf globaler RSS/Atom-Nachrichtenquellen mit Regionenfilter und CSV-Normalisierung ist integriert.

## In Progress
- [ ] Erweiterte Risiko- und Ausführungsregeln für marktspezifische Handelslogik integrieren (Target: 2026-Q4)

## Planned Features
- [ ] Multi-Source-News-Fusion mit Quellgewichtung und Konfliktauflösung erweitern (Target: 2027-Q1)
- [ ] Event- und regimebasierte Positionsgrößensteuerung ergänzen (Target: 2027-Q1)
- [ ] Portfolio- und Multi-Asset-Optimierung für ETF/Fonds/Aktien-Mix hinzufügen (Target: 2027-Q2)

## Implementation Phases
### Phase 1: Design / API-Vertrag
- [ ] Einheitliches Contract-Schema für News, Market Bars, Signals und Execution-Events finalisieren (Target: 2026-Q4)
- [ ] Eingabevalidierung und Fehlersemantik für fehlerhafte Feeds dokumentieren (Target: 2026-Q4)

### Phase 2: Core-Implementierung
- [ ] Exchange-spezifische Handelskalender und Session-Zeiten integrieren (Target: 2027-Q1)
- [ ] Signal-Ensemble mit zusätzlicher Event-Klassifikation (Earnings/M&A/Makro) ausbauen (Target: 2027-Q1)

### Phase 3: Fehlerbehandlung & Edge Cases
- [ ] Feed-Ausfälle, verspätete Ticks und inkonsistente Timestamps robust behandeln (Target: 2027-Q1)
- [ ] Fail-closed Guardrails für geringe Datenqualität und niedrige Signal-Confidence verankern (Target: 2027-Q1)

### Phase 4: Tests
- [ ] Unit- und Integrations-Tests für Profilwechsel (HFT vs. Long-Term) sowie Short/Long-Pfade erweitern (Target: 2027-Q1)
- [ ] Historische Regressionssuite mit reproduzierbaren Datenschnitten etablieren (Target: 2027-Q2)

### Phase 5: Performance/Hardening
- [ ] Batch-Backtest auf größere Symboluniversen mit Laufzeit- und Speicherbudgets optimieren (Target: 2027-Q2)
- [ ] Security-Hardening für Importpfade und externe Newsquellen abschließen (Target: 2027-Q2)

### Phase 6: Dokumentation & Abnahme
- [ ] Betriebsdokumentation inklusive Risk Controls und Manual Override-Runbook vervollständigen (Target: 2027-Q2)
- [ ] Modulabnahme über Kennzahlen-Gates (Drawdown, Win-Rate, Sharpe-like) durchführen (Target: 2027-Q2)

## Production Readiness Checklist
- [ ] Datenqualitäts- und Feed-Integritätsprüfungen aktiv
- [ ] Risikolimits, Positions- und Exposure-Regeln implementiert
- [ ] Historische Regressionsdaten und Backtest-Baselines versioniert
- [ ] Security- und Compliance-Prüfung für Live-Datenanbindung abgeschlossen
- [ ] Runbooks für Failover und manuelle Eingriffe vorhanden

## Known Issues & Limitations
- [!] Die aktuelle Scoring-Logik ist deterministisch und dient als reproduzierbare LLM-Surrogatbasis für historische Validierung.
- [!] Live-Order-Routing ist absichtlich nicht enthalten.

## Breaking Changes
- Keine bekannten Breaking Changes dokumentiert.
