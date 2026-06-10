# Module Gaps Execution Plan (2026-06-08)

## Ziel

Nach dem Erreichen von `MISSING_EXECUTABLES=0` wird die Arbeitsweise von reaktiver Build-Reparatur auf modulgetriebene Gap-Reduktion umgestellt.

Eingangssignale fuer die Priorisierung:
- `src/*/MODULE_GAPS.md` fuer modulnahe, scannerbasierte Findings
- `ai_working/HIGH_GAP_SPRINT_BACKLOG_2026-05-25.md` fuer moduluebergreifende Prioritaeten
- bestehende Fokus-Tests und die jetzt wieder vollstaendige CTest-Registry

## Ausgangslage

### Bereits erreicht
- CTest-Registry konsolidiert
- `ctest --preset windows-release -N` liefert aktuell keine `Could not find executable`-Eintraege
- Fokus-Targets koennen jetzt wieder systematisch zur Validierung von Gap-Fixes genutzt werden

### Wichtigste Planungsquellen
- `ai_working/README.md`
- `ai_working/HIGH_GAP_SPRINT_BACKLOG_2026-05-25.md`
- `src/network/MODULE_GAPS.md`
- `src/rag/MODULE_GAPS.md`

## Prioritaetsmodell

### Prioritaet A: Module mit guter Testabdeckung und hoher Reparaturhebelwirkung
1. `network`
   - 356 actionable Findings
   - frische Fokus-Targets wurden gerade erfolgreich gebaut
   - gute Ausgangslage fuer schnelle Delta-Verbesserungen
2. `rag`
   - 248 actionable Findings
   - Build-/Testpfade sind bereits aktiv genutzt worden
   - nahe an Retrieval-/LLM-/Evaluation-Pfaden mit hoher Systemwirkung

### Prioritaet B: High-Gap-Kernmodule aus Sprint-Backlog
3. `llm`
4. `server`
5. `query`
6. `sharding`
7. `index`

## Aktionsstrategie pro Modul

### Phase 1: Triagieren
- Top-3 Dateien mit den meisten actionable Findings bestimmen
- Nur Findings priorisieren, die
  - CRITICAL oder HIGH sind
  - durch existierende Fokus-Tests abgesichert werden koennen
  - keine unklaren Legacy-/Kompatibilitaetspfade erfordern

### Phase 2: Reparieren
- Kleine, semantisch saubere C++-Aenderungen
- Dokumentation der betroffenen Public APIs mitziehen
- Keine Stub-/Legacy-Pfade einfuehren

### Phase 3: Validieren
- passenden Focus-Test oder kleines Testpaket bauen/laufen lassen
- wenn sinnvoll gezielten Modul-Build erneut ausfuehren
- Delta in `ai_working/` und bei Bedarf in Modul-Roadmap/Arbeitslog dokumentieren

### Phase 4: Rueckkopplung
- nach einer Fix-Welle gezielten Modul-Rescan vorbereiten
- Delta gegen alte `MODULE_GAPS.md`-Snapshot-Werte erfassen

## Startmodul: network

### Warum network zuerst
- hohes actionable Volumen
- frische CTest-Faehigkeit durch die abgeschlossene Executable-Kampagne
- mehrere klare CRITICAL-Themen mit guter Testnaehe

### Erste Ziel-Dateien
1. `src/network/wire_protocol_server.cpp`
   - CRITICAL: `no_timeout`
   - CRITICAL: `thread_join_no_timeout`
   - CRITICAL: `data_race`
   - CRITICAL: `sensitive_data_logging`
2. `src/network/wire_protocol_connection_pool.cpp`
3. `src/network/quic_server.cpp`
4. `src/network/udp_server.cpp`

### Erste konkrete Aufgaben
- `wire_protocol_server.cpp`: blocking/join-Pfade inventarisieren und timeout-/shutdown-vertraeglich machen
- `wire_protocol_server.cpp`: Logging auf sensible Token-/PII-Ausgaben pruefen und haerten
- `udp_server.cpp`: nach dem ebenen Link-Fix jetzt auch die in `MODULE_GAPS.md` markierten Findings sichten
- passende Network-Fokus-Tests fuer Wire/QUIC/UDP als Validierungsset festlegen

## Zweitmodul: rag

### Warum direkt danach
- hoher Systemhebel fuer Retrieval-/Bewertungsstrecken
- gute Ueberlappung mit existierenden Focus-Tests (`RagContextAssembler`, `MultiStepRAG`, etc.)

### Erste Ziel-Dateien
1. `src/rag/continuous_learning_orchestrator.cpp`
2. `src/rag/rag_ingestion_bridge.cpp`
3. `src/rag/multi_step_rag.cpp`
4. `src/rag/reranker.cpp`

### Erste konkrete Aufgaben
- Timeout-/Threading-/Resource-Themen vor Performance-Themen ziehen
- `multi_step_rag.cpp` und Nachbarpfade auf weitere scannerrelevante Guards nachziehen
- vorhandene Focus-Targets als Regression-Absicherung verwenden

## Arbeitsregel fuer kommende Turns
- Nicht mehr nur nach Buildfehlern iterieren
- Jeden groesseren Turn an einem Modul aus `MODULE_GAPS.md` ausrichten
- Vor jeder nicht-trivialen Codewelle einen kurzen Plan/Arbeitsstand in `ai_working/` halten
- Nach jeder Welle den Validierungsstatus und das erwartete Rescan-Delta festhalten

## Naechster konkreter Schritt
- `network` als erstes Gap-Reduktionsmodul bearbeiten
- zuerst `src/network/wire_protocol_server.cpp` gegen die hoechsten CRITICAL-Findings analysieren
- danach einen kleinen, testbaren Fixblock umsetzen und mit passenden Network-Fokus-Targets validieren
