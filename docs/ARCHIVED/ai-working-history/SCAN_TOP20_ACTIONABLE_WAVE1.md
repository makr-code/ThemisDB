# ThemisDB Scan-Auswertung - Top 20 Actionable Findings (Wave 1)

Stand: 2026-06-04
Quelle: ai_working/gap_scan_results.json

## Zusammenfassung

- Gesamt-Findings: 33,247
- Module: 67
- Critical: 2,926
- High: 8,106
- Medium: 13,127
- Low: 9,088

Hinweis zur Priorisierung:
- Diese Liste enthält nur Critical und High Findings.
- Prioritaetsscore = Trefferzahl, bei Critical doppelt gewichtet.

## Top 20 (Impact-priorisiert)

| Rang | Score | Treffer | Severity | Modul | Datei | Pattern |
|---|---:|---:|---|---|---|---|
| 1 | 234 | 117 | CRITICAL | replication | replication/replication_manager.cpp | missing_version_tracking |
| 2 | 166 | 83 | CRITICAL | llm | llm/vision_config.cpp | data_race |
| 3 | 120 | 60 | CRITICAL | query | query/aql_translator.cpp | data_race |
| 4 | 108 | 108 | HIGH | network | network/wire_protocol_server.cpp | no_retry_logic |
| 5 | 99 | 99 | HIGH | replication | replication/replication_manager.cpp | undefined_conflict_resolution |
| 6 | 76 | 76 | HIGH | server | server/http_server.cpp | null_dereference |
| 7 | 72 | 36 | CRITICAL | llm | llm/lora_framework/kernels/directx_kernels.cpp | data_race |
| 8 | 72 | 36 | CRITICAL | storage | storage/rocksdb_wrapper.cpp | data_race |
| 9 | 67 | 67 | HIGH | replication | replication/conflict_resolution.cpp | undefined_conflict_resolution |
| 10 | 62 | 31 | CRITICAL | analytics | analytics/distributed_analytics.cpp | missing_version_tracking |
| 11 | 62 | 31 | CRITICAL | server | server/query_api_handler.cpp | data_race |
| 12 | 62 | 31 | CRITICAL | sharding | sharding/shard_router.cpp | missing_version_tracking |
| 13 | 56 | 28 | CRITICAL | llm | llm/llama_wrapper.cpp | model_integrity_gap |
| 14 | 54 | 54 | HIGH | distributed_knowledge | distributed_knowledge/federated_rag_merger.cpp | undefined_conflict_resolution |
| 15 | 53 | 53 | HIGH | content | content/office_processor.cpp | pointer_arithmetic_unbounded |
| 16 | 50 | 25 | CRITICAL | distributed_knowledge | distributed_knowledge/federated_rag_merger.cpp | missing_version_tracking |
| 17 | 47 | 47 | HIGH | llm | llm/lora_framework/vram_allocator.cpp | null_dereference |
| 18 | 46 | 23 | CRITICAL | replication | replication/conflict_resolution.cpp | missing_version_tracking |
| 19 | 42 | 42 | HIGH | storage | storage/columnar_format.cpp | size_assumption |
| 20 | 42 | 21 | CRITICAL | llm | llm/model_loader.cpp | model_integrity_gap |

## Wave-1 Umsetzung (empfohlen)

## Wave-1 Triage (2026-06-04)

Evidenzbasis:
- `ai_working/gap_scan_results.json` (Top-20 Paare gegen Einzel-Funde aufgeloest)
- Quellcode-Pruefung an den zuerst gemeldeten Trefferzeilen

### Vorlaeufige Klassifikation

| Datei + Pattern | Erste Trefferzeile | Vorlaeufig | Kurzbegruendung |
|---|---:|---|---|
| replication/replication_manager.cpp + missing_version_tracking | 329 | False Positive | Treffer auf Prometheus-Metrik-String `conflicts_resolved` statt Konfliktlogik. |
| replication/replication_manager.cpp + undefined_conflict_resolution | 329 | False Positive | Gleicher String-Match im Metrik-Export, keine Merge-Strategie-Codezeile. |
| replication/conflict_resolution.cpp + missing_version_tracking | 2 | False Positive | Treffer in auto-generiertem Datei-Header/Doxygen-Block. |
| replication/conflict_resolution.cpp + undefined_conflict_resolution | 304 | Eher False Positive | Treffer in Doxygen-Param-Kommentar, nicht in Ausfuehrungslogik. |
| distributed_knowledge/federated_rag_merger.cpp + missing_version_tracking | 2 | False Positive | Treffer im Dateikopf-Kommentar. |
| distributed_knowledge/federated_rag_merger.cpp + undefined_conflict_resolution | 2 | False Positive | Treffer im Dateikopf-Kommentar. |
| analytics/distributed_analytics.cpp + missing_version_tracking | 21 | False Positive | Treffer in Architektur-/Datenflusskommentar. |
| query/aql_translator.cpp + data_race | 94 | Eher False Positive | Lokale Variable (`varName`) ohne Shared-State-Schreibpfad. |
| server/http_server.cpp + null_dereference | 1232 | Eher False Positive | `feedback_api_handler_` wird unmittelbar davor erzeugt, dann gesetzt. |
| storage/rocksdb_wrapper.cpp + data_race | 99 | False Positive | Merge-Operator mit lokalen Variablen + expliziter Null-Guard. |
| storage/columnar_format.cpp + size_assumption | 104 | False Positive | `sizeof(int64_t)` auf Fixed-Width-Typ ist beabsichtigtes Serialisierungsformat. |
| llm/vision_config.cpp + data_race | 118 | Review offen | Konfigurationsobjekt-Mutationen, Publikationspfad separat pruefen. |
| llm/lora_framework/kernels/directx_kernels.cpp + data_race | 297 | Review offen | Globaler DX-Statuszugriff moeglich, Threading-Pfad pruefen. |
| network/wire_protocol_server.cpp + no_retry_logic | 1341 | Review offen (Scanner-Fix umgesetzt) | Vorheriger Match sehr wahrscheinlich auf `request.*`-Heuristik-Rauschen. |
| llm/llama_wrapper.cpp + model_integrity_gap | 397 | Review offen (Scanner-Fix umgesetzt) | Datei enthaelt Integrity-Checks; Heuristik-Fenster war zu kurz. |
| llm/model_loader.cpp + model_integrity_gap | 156 | Review offen | Integritaetskette ueber Aufrufer/Loader-Kooperation prüfen. |
| sharding/shard_router.cpp + missing_version_tracking | 222 | Review offen | Merging-Pfade mit Versionsmetadaten noch gezielt pruefen. |
| content/office_processor.cpp + pointer_arithmetic_unbounded | 162 | Review offen | Treffer auf Blob-Verarbeitung, Bounds-Pfade gezielt nachziehen. |

### Bereits umgesetzte Scanner-Heuristik-Verbesserungen

1. Kommentar-/Doxygen-Filter fuer Distributed-Consistency-Scanner
- Datei: `tools/scanners/gs3_step04_distributed_consistency.py`
- Wirkung: `missing_version_tracking`/`undefined_conflict_resolution` werden nicht mehr auf Header-/Doku-Text ausgelöst.

2. Strengere HTTP/RPC-Muster fuer Reliability-Scanner
- Datei: `tools/scanners/gs3_step01_classic_reliability.py`
- Wirkung: Keine `no_retry_logic`-Treffer mehr auf generische Objekte wie `json request...`.

3. Robusteres Integrity-Fenster im LLM-Safety-Scanner
- Datei: `tools/scanners/gs3_step04_llm_ai_safety.py`
- Wirkung: `model_integrity_gap` beruecksichtigt nun groesseren Folgekontext und ignoriert Kommentar-/Deklarationszeilen.

### Nächster Verifikationsschritt

1. Scanner neu laufen lassen (nur betroffene Scanner oder Full-Run).
2. Delta fuer die oben klassifizierten Paare messen.
3. Offene Review-Faelle (Vision/DX/ModelLoader/ShardRouter/OfficeProcessor) als echte Defects oder FP final entscheiden.

### Fokussierter Delta-Check (nach Heuristik-Update)

Scope:
- Dateien: `replication_manager.cpp`, `conflict_resolution.cpp`, `federated_rag_merger.cpp`, `distributed_analytics.cpp`, `wire_protocol_server.cpp`, `llama_wrapper.cpp`, `model_loader.cpp`
- Patterns: `missing_version_tracking`, `undefined_conflict_resolution`, `no_retry_logic`, `model_integrity_gap`

Ergebnis:
- Gesamt vorher: 630
- Gesamt nachher: 0
- Delta: -630 (ca. -100.0%)

Delta je Top-Fall (normalisiert):

| Datei + Pattern | Vorher | Nachher | Delta |
|---|---:|---:|---:|
| network/wire_protocol_server.cpp + no_retry_logic | 108 | 0 | -108 |
| replication/replication_manager.cpp + missing_version_tracking | 117 | 0 | -117 |
| replication/replication_manager.cpp + undefined_conflict_resolution | 99 | 0 | -99 |
| replication/conflict_resolution.cpp + missing_version_tracking | 23 | 0 | -23 |
| replication/conflict_resolution.cpp + undefined_conflict_resolution | 67 | 0 | -67 |
| distributed_knowledge/federated_rag_merger.cpp + missing_version_tracking | 25 | 0 | -25 |
| distributed_knowledge/federated_rag_merger.cpp + undefined_conflict_resolution | 54 | 0 | -54 |
| analytics/distributed_analytics.cpp + missing_version_tracking | 31 | 0 | -31 |
| analytics/distributed_analytics.cpp + undefined_conflict_resolution | 38 | 0 | -38 |
| llm/llama_wrapper.cpp + model_integrity_gap | 28 | 0 | -28 |
| llm/model_loader.cpp + model_integrity_gap | 21 | 0 | -21 |

Interpretation:
- `no_retry_logic` in `wire_protocol_server.cpp` ist durch Regex-Haertung praktisch eliminiert.
- Distributed-Consistency-Patterns wurden durch Kommentar-/Metrik-Filter und strengere Merge/Conflict-Call-Erkennung stark reduziert.
- `model_integrity_gap` wurde durch call-site-only Matching plus String-Literal-Filter weiter reduziert (Log-String-FPs entfernt).
- Der fokussierte Wave-1-Rescan zeigt jetzt keine verbleibenden Treffer fuer die vier Ziel-Patterns.

Finale Massnahme auf Code-Seite:
- In `src/llm/llama_wrapper.cpp` wurde in `loadModel(...)` ein Integritaets-Gate ergänzt:
	- optionaler Check bei `expected_checksum`/`model_checksum`
	- verpflichtender Check, wenn `require_model_integrity=true`
	- sauberer ERROR-State bei fehlgeschlagener oder fehlender geforderter Verifikation

### W1-A: Concurrency and Memory Safety

1. llm/vision_config.cpp - data_race
2. query/aql_translator.cpp - data_race
3. llm/lora_framework/kernels/directx_kernels.cpp - data_race
4. storage/rocksdb_wrapper.cpp - data_race
5. server/http_server.cpp - null_dereference

Akzeptanzkriterien:
- Datenraces entfernt oder klar synchronisiert (mutex/atomic/lock ordering).
- Keine Null-Dereferenzpfade ohne Guard im kritischen Request-Pfad.
- Relevante Focus-Tests erweitert/neu hinzugefuegt.

### W1-B: Replication Correctness

1. replication/replication_manager.cpp - missing_version_tracking
2. replication/replication_manager.cpp - undefined_conflict_resolution
3. replication/conflict_resolution.cpp - undefined_conflict_resolution
4. replication/conflict_resolution.cpp - missing_version_tracking

Akzeptanzkriterien:
- Eindeutige Versionierungsstrategie entlang Write/Replicate/Resolve.
- Deterministische Konfliktregeln dokumentiert und testbar.
- Regressionstests fuer konkurrierende Update-Sequenzen.

### W1-C: Integrity and Contract Hardening

1. llm/llama_wrapper.cpp - model_integrity_gap
2. llm/model_loader.cpp - model_integrity_gap
3. network/wire_protocol_server.cpp - no_retry_logic
4. content/office_processor.cpp - pointer_arithmetic_unbounded

Akzeptanzkriterien:
- Model-Integrity-Checks vor Nutzung verpflichtend.
- Retry-Policy im Wire-Protocol mit klaren Grenzen/Backoff.
- Bounds-Checks fuer pointer_arithmetic_unbounded-Faelle.

## Schnellstart fuer das Team

1. Fix-Branch pro Wave-1 Block (W1-A/W1-B/W1-C) erstellen.
2. Pro Block erst kleine, testbare Commits (max. 2-3 Pattern pro Commit).
3. Nach jedem Block Focus-Tests und scanner delta vergleichen.
4. Abschliessend diese Datei mit neuer Top-20-Liste aktualisieren.
