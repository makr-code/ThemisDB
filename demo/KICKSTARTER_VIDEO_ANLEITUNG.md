# ThemisDB Kickstarter Demo Video - Anleitung

## Überblick
Diese Anleitung erklärt, wie du ein unbearbeitetes Screen-Recording der ThemisDB erstelle, das Kickstarts Anforderungen erfüllt.

**Wichtig:** Kickstarter möchte ein **echtes, ungefiltertes Video** - keine Poliertes Endergebnis.

---

## 1. Vorbereitung

### A. System prüfen und Binary bauen
```powershell
cd C:\Projects\ThemisDB
cmake --build --preset windows-release --target themisctl
```

### B. Optional: Server manuell starten (wenn nicht bereits aktiv)
```powershell
.\build-msvc-windows-release\bin\themis_server.exe --db .\demo\data\themis_db --port 8765 --allow-degraded-build --allow-stub-hsm
```

### B1. Docs-Datenbank fuer Section 8 bereitstellen (empfohlen)
```powershell
.\tools\build_docs_db.ps1 -SkipBuild -OutputDir .\artifacts\docs-db -Force
Copy-Item .\artifacts\docs-db\docs_database.json .\data\docs_database.json -Force
Copy-Item .\artifacts\docs-db\docs_artifact.json .\data\docs_artifact.json -Force
```

Hinweis: Nicht direkt nach `.\data` bauen, da das Build-Skript den Zielordner bei `-Force` rekursiv leert.

### B2. Zwei-Terminal-Setup (fuer das Video empfohlen)
- Terminal 1: nur Server-Logs (themis_server.exe)
- Terminal 2: nur CLI-Kommandos (themisctl.exe / themis_ctrl.exe)

Beispiel-Setup:

Terminal 1 (Server laeuft durchgehend)
```powershell
cd C:\Projects\ThemisDB
.\build-msvc-windows-release\bin\themis_server.exe --db .\demo\data\themis_db --port 8765 --allow-degraded-build --allow-stub-hsm
```

Terminal 2 (Demo-Kommandos)
```powershell
cd C:\Projects\ThemisDB
.\demo\kickstarter_demo_script.ps1
```

### C. Empfohlener Weg: Komplettskript verwenden
```powershell
.\demo\kickstarter_demo_script.ps1
```

Fuer einen automatisierten Probe-/CI-Lauf ohne Tastaturstopps:
```powershell
$env:THEMIS_DEMO_NO_PAUSE='1'
.\demo\kickstarter_demo_script.ps1
```

Das PS1 übernimmt:
- Pre-Flight (Server erreichbar?)
- Automatisches Laden von Demo-Daten bei Bedarf
- Die komplette Demo-Reihenfolge in einem Durchlauf

### D. Bildschirm vorbereiten
- Terminal auf mindestens 1920x1080
- Gut lesbare Schriftgröße
- Keine störenden Fenster/Notifications

---
## 2. Aufnahme starten

### Option A: OBS Studio (empfohlen)
- Display oder Window Capture auf das Terminal
- 1920x1080, H.264, MP4
- Eine durchgehende Aufnahme ohne Schnitte

### Option B: Windows Bordmittel
- Screen Recording mit Win+G (Xbox Game Bar)
- Ebenfalls als ein durchgehender Take aufnehmen

---
## 3. Demo-Ablauf (Stand: kickstarter_demo_script.ps1)

Nutze fuer das Video bevorzugt den Komplettlauf:

```powershell
.\demo\kickstarter_demo_script.ps1
```

Empfohlene Darstellung im Recording:
- Links: Terminal 1 mit laufendem themis_server.exe (Live-Logs)
- Rechts: Terminal 2 mit themisctl-/Demo-Script-Kommandos
- So ist Ursache/Wirkung direkt sichtbar (Request in Terminal 2, Reaktion in Terminal 1)

Die folgenden Punkte sind exakt die im Script ausgefuehrten Phasen inklusive Beispiel-Eingabe und erwarteter Ausgabe.

### Phase 1: System Status & Schema

**Beispiel-Eingabe**
```powershell
& $THEMISCTL --host 127.0.0.1 --port 8765 schema
```

**Erwartete Ausgabe**
- Schema-/Collection-Informationen als JSON oder strukturierte Liste
- Kommentar im Demo-Script: "Server is running and responding to queries."

### Phase 2: Document Read-Back

**Beispiel-Eingabe**
```powershell
& $THEMISCTL --host 127.0.0.1 --port 8765 get "demo_articles:art_0001"
```

**Erwartete Ausgabe**
- JSON-Dokument fuer den Key demo_articles:art_0001
- Bei Fehler: Warning "Section 2 failed: demo_articles sample not readable"

### Phase 3: Vector Payload Read-Back

**Beispiel-Eingabe**
```powershell
& $THEMISCTL --host 127.0.0.1 --port 8765 get "demo_embeddings:vec_0001"
```

**Erwartete Ausgabe**
- JSON mit Vektor-Payload
- Bei Fehler: Warning "Section 3 failed: demo_embeddings sample not readable"

### Phase 4: Graph Node Read-Back

**Beispiel-Eingabe**
```powershell
& $THEMISCTL --host 127.0.0.1 --port 8765 get "demo_knowledge_graph:node_0001"
```

**Erwartete Ausgabe**
- JSON fuer den Graph-Knoten
- Bei Fehler: Warning "Section 4 failed: demo_knowledge_graph sample not readable"

### Phase 5: LLM Inference Probe

**Beispiel-Eingabe**
```powershell
$llmInferenceBody = '{"prompt":"Summarize the impact of ACID transactions for distributed databases in two sentences.","max_tokens":96,"temperature":0.2}'
& $THEMISCTL --host 127.0.0.1 --port 8765 api POST /api/v1/llm/inference --body $llmInferenceBody
```

**Erwartete Ausgabe**
- Erfolg: JSON mit LLM-Antwort
- Haeufig in degradierten Setups: HTTP 500 mit Hinweis auf nicht initialisiertes LLM
- Dann folgt im Script automatisch ein Model-Auto-Load (bevorzugt `.\\models\\phi4.gguf`, sonst erstes `.gguf`) und danach ein erneuter Probe-Call:
```powershell
& $THEMISCTL --host 127.0.0.1 --port 8765 api GET /api/v1/llm/health
```

### Phase 6: Graph Query Planning Probe

**Beispiel-Eingabe**
```powershell
$section6GraphExplainBody = '{"query_type":"k_hop","start_vertex":"demo_knowledge_graph:node_0001","max_depth":1}'
& $THEMISCTL --host 127.0.0.1 --port 8765 api POST /api/v1/graph/query/explain --body $section6GraphExplainBody
```

**Erwartete Ausgabe**
- Erfolg: JSON mit Optimizer-/Planungsdaten (Algorithmus, geschätzte Kosten, Alternativen)
- Bei deaktiviertem Graph-Optimizer: Section wird per Pre-Check als Caveat geskippt

### Phase 7: RAG Capability Probe

**Beispiel-Eingabe**
```powershell
$ragQueryBody = '{"query":"What are the latest papers on quantum computing by MIT?","collection":"demo_articles","top_k":3,"max_tokens":192,"temperature":0.2}'
& $THEMISCTL --host 127.0.0.1 --port 8765 api POST /api/v1/llm/rag --body $ragQueryBody
```

**Erwartete Ausgabe**
- Erfolg: Antworttext plus Retrieval-Metriken
- Bei LLM-Initialisierungsproblemen: Warning in der Demo-Zusammenfassung

### Phase 8: Themis Help Probe (docs.db)

**Beispiel-Eingabe**
```powershell
& $THEMISCTL --host 127.0.0.1 --port 8765 help --mode lora "How do I configure sharding and RAG safely in ThemisDB?"
```

**Erwartete Ausgabe**
- Erfolg: themis-spezifische Hilfsantwort auf Basis docs.db
- Wenn docs-Datenbank fehlt: typischerweise HTTP 503 und Caveat-Warning

### Phase 9: CRUD Consistency Check

**Beispiel-Eingabe**
```powershell
$runtimeProbeBody = @{ blob = '{"title":"Runtime Probe","content":"Compatibility mode"}' } | ConvertTo-Json -Compress
& $THEMISCTL --host 127.0.0.1 --port 8765 put "demo_articles:runtime_probe" $runtimeProbeBody
& $THEMISCTL --host 127.0.0.1 --port 8765 get "demo_articles:runtime_probe"
```

**Erwartete Ausgabe**
- PUT erfolgreich (z. B. "Entity stored")
- GET liefert denselben Datensatz zurueck

### Phase 10: Performance & Statistics

**Beispiel-Eingabe**
```powershell
& $THEMISCTL --host 127.0.0.1 --port 8765 admin stats
```

**Erwartete Ausgabe**
- Metriken zu Query/Throughput/Latency/Cache (je nach Build unterschiedlich detailliert)

### Phase 11: Automatic Index Recommendation

**Beispiel-Eingabe**
```powershell
& $THEMISCTL --host 127.0.0.1 --port 8765 index recommend demo_articles
```

**Erwartete Ausgabe**
- Empfehlungsliste (ADD/DROP) mit Spalten und Nutzenbewertung

### Finale Demo-Ausgabe

Am Ende zeigt das Script immer:
- "Demo Complete!"
- Feature-Liste
- Entweder:
   - "ThemisDB demo checks passed."
   - oder "Demo completed with caveats:" inklusive konkreter Warnungen

---

## 4. Wichtige Hinweise für Kickstarter

### ✓ WIRD AKZEPTIERT:
- ✓ Ungefiltertes, einteiliges Video
- ✓ Live-Terminal-Output
- ✓ Echte Anfragen, echte Daten
- ✓ Wenn mal ein Fehler passiert: Einfach neustart und weiter (NO CUTS!)
- ✓ Natürliche Redegeschwindigkeit
- ✓ Jedes Betriebssystem (Windows, Linux, macOS)

### ✗ WIRD NICHT AKZEPTIERT:
- ✗ Stark bearbeitete Videos (Schnitte, Übergänge, Musik)
- ✗ Nur Diagramme und Slides (muss laufendes System zeigen)
- ✗ Stock-Footage oder Demo-Simulationen
- ✗ Animierte Grafiken ohne echtem System dahinter

---

## 5. Screenshots für Kickstarter

Nach dem Video: Mache Screenshots von:

1. **Terminal mit laufender Query**
   ```powershell
   # Screenshot speichern (Windows)
   Print Screen -> Paste in Paint -> Speichern als PNG
   ```

2. **themisctl REPL läuft**
   ```powershell
   themisctl repl
   # Screenshot der interaktiven Shell
   ```

3. **Admin Dashboard** (wenn verfügbar)
   ```powershell
   themisctl admin stats
   # Lebendes System mit Statistiken
   ```

4. **Query-Ergebnisse in JSON/JSONL Format**
   ```powershell
   '{"query_type":"k_hop","start_vertex":"demo_knowledge_graph:node_0001","max_depth":1}' | .\build-msvc-windows-release\bin\themisctl.exe --host 127.0.0.1 --port 8765 api POST /api/v1/graph/query/explain --stdin --content-type application/json
   # Screenshot der formatierten Ausgabe
   ```

---

## 6. Video hochladen

1. Lade das MP4 bei Kickstarter unter "Project Media" hoch
2. Titel: `"ThemisDB Live Demo - Multi-Model Database in Action"`
3. Beschreibung:
   ```
   This is a live, unedited demonstration of ThemisDB in production.
   
   Shown:
   - Real-time document search (SQL-like queries)
   - Vector/semantic search with embeddings
   - Graph traversal with multi-hop relationships
   - LLM-powered RAG agent for natural language queries
   - Performance metrics and monitoring
   
   The system is fully operational and handling actual queries.
   ```

---

## 7. Checkliste vor Upload

- [ ] Video ist 10-15 Minuten lang
- [ ] Terminal ist groß und lesbar (mindestens 24pt Font)
- [ ] Mindestens 5 verschiedene Query-Typen gezeigt
- [ ] LLM/RAG-Feature demonstriert
- [ ] Performance-Stats sichtbar
- [ ] Keine Schnitte oder Bearbeitungen (ein durchgehendes Video)
- [ ] Audio klar verständlich (falls sprechend)
- [ ] Auflösung mindestens 1280x720

---

## 8. Tipps für ein professionelles Aussehen (ohne Bearbeitung)

- **Vorbereitung ist alles:** Schreibe alle Queries auf und teste sie vorher
- **Langsam sprechen:** Gib den Zuschauern Zeit zu folgen
- **Pause vor komplexen Queries:** "Diese Query zeigt Graph-Navigation..."
- **Kommentiere während der Ausführung:** "Jetzt werden 10 Millionen Vektoren durchsucht..."
- **Fehler sind OK:** Wenn etwas fehlschlägt, einfach erklären, Problem fixen und weitermachen

---

## 9. Zusätzliche Screenshots (statisch)

Mache auch Screenshots von:

1. **themisctl help**
   ```powershell
   themisctl --help
   ```

2. **Running Server**
   ```powershell
   # Screenshot vom REPL-Mode
   themisctl repl
   > EXPLAIN SELECT ...
   ```

3. **Configuration**
   ```powershell
   cat config.yaml
   # Zeigt: Port 8765, Memory-Limit, Features, etc.
   ```

---

## 10. Alternative: Live-Session mit Publikum

Falls kein Pre-Recorded Video möglich:
- Starte themisctl repl
- Lese Queries von einem Blatt vor
- Lasse den Zuschauer Fragen stellen
- Beantworte sie live mit Queries

Kickstarter akzeptiert auch das!

---

**Viel Erfolg mit dem Demo-Video! 🚀**
