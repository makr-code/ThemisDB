> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

# FUTURE_ENHANCEMENTS

## examples/25_infobroker_trading

### Scope
- Ausbau zu einem mehrbörsigen Entscheidungs- und Validierungssystem für Aktien, ETF und Fonds.
- Ausbau von Signalqualität, Risiko-Management und Backtest-Tiefe für HFT- und Long-Term-Modi.

### Design Constraints
- Deterministische Reproduzierbarkeit identischer Datenläufe bleibt Pflicht.
- Keine Order-Ausführung ohne explizite Risk- und Compliance-Gates.
- Datenquellen müssen versioniert, nachvollziehbar und auditierbar bleiben.

### Required Interfaces
- News-Contract: `id,timestamp,source,symbol,exchange,headline,body,source_reliability`.
- Price-Contract: `symbol,exchange,timestamp,close`.
- Label-Contract: `news_id,symbol,exchange,event_timestamp,horizon_days,future_return_pct,label,confidence,rationale`.
- Signal-Contract: Aktion, Score, Confidence, Holding-Period, Rationale.
- Export-Contract: JSONL für Document/Vector/Graph/Time-Series Importpfade.

### Implementation Notes
- Signalmodul als austauschbare Policy-Schicht halten, damit echte LLM-Inferenz oder Ensemble-Modelle ohne Contract-Bruch integriert werden können.
- Backtest-Pipeline strikt vom Exportpfad trennen (Validierung vor Persistierung).
- Exchange-Kalender, Slippage und Gebührenmodell als optionale Erweiterungspunkte kapseln.

### Test Strategy
- Unit-Tests für positive/negative Ereignisse und Confidence-Schwellen.
- Integrations-Tests für vollständigen Datenlauf (CSV -> Signal -> Backtest -> Export).
- Regressionssuite mit festen Daten-Snapshots und festen Kennzahlengrenzen.

### Performance Targets
- Backtest auf 1 Mio. MarketBars in <= 90 Sekunden auf Standard-Entwicklerhardware.
- Speicherverbrauch <= 2 GB bei 1 Mio. MarketBars und 100k News-Events.
- Exportdurchsatz >= 50k JSONL-Einträge/Sekunde im Batch-Mode.

### Security / Reliability
- Input-Sanitization für externe Newsfeeds und CSV-Ingestion.
- Fail-closed bei unvollständigen Zeitreihen oder ungültigen Symbol-Mappings.
- Auditierbares Logging für alle Signal- und Trade-Entscheidungen.
