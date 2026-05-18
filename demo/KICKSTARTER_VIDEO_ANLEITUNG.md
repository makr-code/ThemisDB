# ThemisDB Kickstarter Demo Video - Anleitung

## Überblick
Diese Anleitung erklärt, wie du ein unbearbeitetes Screen-Recording der ThemisDB erstelle, das Kickstarts Anforderungen erfüllt.

**Wichtig:** Kickstarter möchte ein **echtes, ungefiltertes Video** - keine Poliertes Endergebnis.

---

## 1. Vorbereitung

### A. System prüfen
```powershell
# 1. ThemisDB Server starten
cd C:\Projects\ThemisDB
cmake --build --preset windows-release --target themisctl

# 2. Server läuft (in separatem Terminal)
build\windows-release\bin\themisctl --version
```

### B. Test-Daten laden
```powershell
# Stelle sicher, dass die Demo-Collections existieren:
# - demo_articles (vollständige Dokumente mit Text)
# - demo_embeddings (Vektordaten, Semantic Search)
# - demo_knowledge_graph (Graph mit Nodes: researcher, paper)

# Daten ggf. per Import-Tool laden:
.\build\windows-release\bin\themisdb-import --collection demo_articles data/demo_articles.jsonl
```

### C. Bildschirm vorbereiten
- Terminal auf **hohe Auflösung** stellen (mindestens 1920x1080)
- Font-Größe erhöhen (Terminal leserlich, aber nicht zu groß)
- Farbschema auf Hell oder Dunkel (konsistent)
- Alle anderen Fenster schließen oder minimieren

---

## 2. Video-Recording

### Option A: OBS Studio (kostenlos, empfohlen)
```bash
# 1. OBS Studio herunterladen: https://obsproject.com/
# 2. Neue Scene erstellen
# 3. Source: "Display Capture" oder "Window Capture" (VS Code Terminal)
# 4. Canvas size: 1920x1080 oder 1280x720
# 5. Encoding: H.264, 60 FPS, höhere Bitrate (6000 kbps)
# 6. Output: MP4 Format
# 7. Recording starten
```

### Option B: Windows Screenshot-Tool
```powershell
# Einfache Alternative (Windows 10+)
# Win + Shift + S -> Video Recording starten
# Terminal-Output wird aufgezeichnet
```

---

## 3. Demo-Ablauf (ca. 10-15 Minuten)

### **Phase 1: Start & Setup (1-2 Min)**
```powershell
# Zeige das Terminal
# Erkläre: "Das ist ThemisDB - ein Multi-Model Database System"

# Starte den REPL:
.\build\windows-release\bin\themisctl repl --host localhost:8765

# Oder direkt die Demo:
.\build\windows-release\bin\themisctl schema --host localhost:8765
```

**Was zu sagen ist:**
- "ThemisDB läuft bereits und akzeptiert Anfragen."
- "Das Schema zeigt unsere verschiedenen Datenmodelle: Dokumente, Vektoren, Graphen."

---

### **Phase 2: Document Search (2-3 Min)**
```powershell
# Ausführe die erste Query
themisctl query --host localhost:8765 \
  "FOR doc IN demo_articles \
   FILTER doc.title LIKE '%AI%' OR doc.content LIKE '%machine learning%' \
   SORT doc.published DESC \
   LIMIT 5 \
   RETURN { title: doc.title, published: doc.published }"
```

**Was zu sagen ist:**
- "Das ist eine klassische Dokumentsuche - ähnlich wie SQL."
- "ThemisDB sucht Millionen von Dokumenten nach Text-Filtern."
- "Die Ergebnisse kommen live zurück."

---

### **Phase 3: Vector Search - Semantic (2-3 Min)**
```powershell
themisctl query --host localhost:8765 \
  "FOR doc IN demo_embeddings \
   LET similarity = COSINE_SIMILARITY(doc.embedding, @query) \
   FILTER similarity > 0.7 \
   SORT similarity DESC \
   LIMIT 5 \
   RETURN { title: doc.title, similarity: similarity }"
```

**Was zu sagen ist:**
- "Das ist Vector Search - semantische Ähnlichkeit, nicht Text-Matching."
- "ThemisDB vergleicht Millionen von Embeddings mit hoher Geschwindigkeit."
- "Dies ist ideal für AI-Modelle und Similarity-Suche."

---

### **Phase 4: Graph Traversal (2-3 Min)**
```powershell
themisctl query --host localhost:8765 \
  "FOR researcher IN demo_knowledge_graph \
   FILTER researcher.type == 'researcher' \
   FOR paper IN 1..2 OUTBOUND researcher._id graph_edges \
     FILTER paper.type == 'paper' \
   RETURN { \
     researcher: researcher.name, \
     paper: paper.title \
   } \
   LIMIT 8"
```

**Was zu sagen ist:**
- "Das ist Graph-Navigation - Multi-Hop-Beziehungen."
- "ThemisDB findet Pfade zwischen Knoten (z.B. Forscher -> Paper -> Konferenzen)."
- "Das funktioniert mit Millionen von Nodes und Edges."

---

### **Phase 5: RAG + LLM Features (2-3 Min)**
```powershell
themisctl rag query \
  --collection demo_articles \
  --top-k 3 \
  "What are the latest papers on quantum computing by MIT?"
```

**Was zu sagen ist:**
- "Das ist unser RAG (Retrieval-Augmented Generation) Agent."
- "Der LLM versteht die natürlichsprachige Frage, nicht SQL."
- "ThemisDB findet die relevantesten Daten und gibt eine AI-generierte Antwort."

---

### **Phase 6: Performance & Stats (1-2 Min)**
```powershell
themisctl admin stats --host localhost:8765
```

**Was zu sagen ist:**
- "Live-Statistiken zeigen Durchsatz, Latenz, Cache-Hit-Rate."
- "ThemisDB verarbeitet Tausende Queries pro Sekunde."
- "System ist in Produktion stabil und skalierbar."

---

### **Phase 7: Index Recommendations (1 Min)**
```powershell
themisctl index recommend --host localhost:8765 --collection demo_articles
```

**Was zu sagen ist:**
- "ThemisDB analysiert Query-Muster automatisch."
- "Es empfiehlt Optimierungen und Indizes zur Speedup."

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
   themisctl query --output-format json ...
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
