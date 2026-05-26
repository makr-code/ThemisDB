## Update: Große Stub-/Simulation-Umsetzungen priorisiert (Core/Security/Tensor)

Neu verlinkte Issues:
- #4920 Core/Server Feature-Gates
- #4921 Security/Sharding SignedRequestVerifier
- #4922 Security/HSM PKCS#11
- #4923 Tensor TNSRTask Persistenz
- #4924 TensorRouter Template-Topologie
- #4925 HissReshaper pure-binary QTT
- #4926 HyperIndexBuilder FK-Propagation
- #4927 UTR semantische Encoder

### Priorisierung
1. P1 (Sicherheits- und Betriebsblocker)
- #4921
- #4922
- #4920

2. P2 (Persistenz-/Konsistenzkern Tensor)
- #4923
- #4924

3. P3 (Qualitäts-/Ranking-/Semantikverbesserungen Tensor)
- #4925
- #4926
- #4927

### Reihenfolgebegründung
- P1 zuerst wegen direkter Sicherheits- und Produktionsauswirkung.
- P2 danach, weil Persistenz-/Topologie-Korrektheit Grundlage für belastbare Tensor-Pipelines ist.
- P3 anschließend für Qualitäts- und Retrieval-Verbesserung auf stabiler Basis.

---

## Doxygen-Audit 2026-05-11 (maschinell ausgewertet)

Quelle:
- build/doxygen/doxygen-warnings.log

Kennzahlen:
- TOTAL: 716
- UNDOC ("is not documented"): 31
- PARAM (kaputte/inkonsistente @param): 152
- UNSUPPORTED_TAG (XML/HTML-Tags in Kommentaren): 241

Top-Module nach Warnungsvolumen:
- query: 69
- index: 54
- plugins: 40
- rag: 39
- content: 38
- analytics: 35
- utils: 30
- server: 27
- process: 26
- storage: 24
- llm: 24
- auth: 21
- search: 20
- temporal: 19
- graph: 16

## TODO-Backlog fuer KI-Agents (Copilot/Ollama)

Status-Legende:
- [ ] offen
- [~] in Bearbeitung
- [x] erledigt

### P1: Doxygen-Kommentarsyntax korrigieren (schneller, hoher Hebel)

- [x] Unsupported XML/HTML Tags bereinigen (Target: Q2 2026)
	- Fokusdatei-Cluster: include/query/*
	- Regel: Platzhalter wie NAME/COLLECTION/AQL_BODY statt Winkelklammer-Tags verwenden.
	- Ergebnis (2026-05-11): query-spezifisch 19 -> 0 Unsupported-Tags.
	- Akzeptanz: erreicht.

- [~] @param-Inkonsistenzen reparieren - Phase 1 (Target: Q2 2026)
	- PHASE 1 COMPLETE: 152 -> 121 violations (-31, 20% reduction) ✅
	- Patched files (5): rag_judge.h, vram_secure_clear.h, graph_query_optimizer.h, timeseries.h, query_federation.h
	- Strategy: Split overloaded function docs to match individual signatures
	- Remaining (121): param_mismatch=61, too_many=44, no_args_with_param=15
	- Phase 2: Target next 20 high-priority files, reduce to ≤50 violations

### P2: Fehlende API-Dokumentation in Hotspot-Modulen

- [ ] Modul query: Public API-Doku vervollstaendigen (Target: Q2 2026)
	- Scope: include/query/*.h
	- Akzeptanz: Keine "is not documented"-Warnungen fuer query.

- [ ] Modul index: Public API-Doku vervollstaendigen (Target: Q2 2026)
	- Scope: include/index/*.h
	- Akzeptanz: Keine "is not documented"-Warnungen fuer index.

- [ ] Modul server: Public API-Doku vervollstaendigen (Target: Q2 2026)
	- Scope: include/server/*.h
	- Akzeptanz: Keine "is not documented"-Warnungen fuer server.

### P3: Restliche Module in Batches normalisieren

- [ ] Batch A: plugins, rag, content, analytics (Target: Q3 2026)
- [ ] Batch B: utils, process, storage, llm (Target: Q3 2026)
- [ ] Batch C: auth, search, temporal, graph (Target: Q3 2026)

## Agent-Runbook (direkt kopierbare Arbeitsauftraege)

Hinweis Routing in diesem Repo:
- C++-Header/Source-Aufgaben bevorzugt lokal via @ollama.
- Architektur/Review-Freigaben via Standard-Copilot.

Prompt 1 (Syntax-Fix, query):
- "@ollama /local Repariere in include/query/*.h alle Doxygen-Warnungen vom Typ 'Unsupported xml/html tag'. Ersetze problematische <...>-Token durch doxygen-sichere Darstellung. Keine Verhaltensaenderung am Code. Liefere Patch + Liste behobener Warnungen."

Prompt 2 (@param-Fix, modulweit):
- "@ollama /local Pruefe in include/**/*.h alle Doxygen-Kommentare auf @param-Konsistenz. Entferne ueberzaehlige @param, korrigiere Namen exakt gemaess Signatur und ergaenze fehlende Parameterdoku nur bei Public APIs. Liefere Patch + Vorher/Nachher-Zaehlung."

Prompt 3 (UNDOC, server):
- "@ollama /local Dokumentiere in include/server/*.h alle oeffentlichen Klassen/Methoden/Struct-Felder mit Doxygen (@brief, @param, @return, @throws wo relevant). Kein Stub-Text, sondern konkrete Semantik, Fehlerfaelle und Ownership-Hinweise."

Prompt 4 (Review-Gate):
- "Fuehre Code-Review auf Doxygen-Aenderungen durch: Fokus auf fachlich korrekte Semantik in Kommentaren, keine API-Aenderung, keine falschen Guarantees. Markiere Blocker."

## Re-Run / Verifikation

- [ ] Audit erneut laufen lassen und Kennzahlen aktualisieren (Target: Q2 2026)
	- Kommando: C:/Program Files/doxygen/bin/doxygen.exe Doxyfile.audit
	- Danach maschinell pruefen:
		- Count("Unsupported xml/html tag") == 0
		- Count("too many @param") == 0
		- Count("argument .* @param") == 0
		- UNDOC pro Zielmodul == 0
