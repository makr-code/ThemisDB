# ThemisDB Kickstarter Demo Environment

Vollständiges Setup für die Video-Demo der Kickstarter-Kampagne.

**Updated:** Mai 17, 2026 - Complete themisctl-based demo with automated scripts

## 📁 Struktur

```
demo/
├── kickstarter_demo_script.ps1    ⭐ RUN THIS - Vollständige automatisierte Demo
├── kickstarter_demo_script.sh     # Bash-Version für Linux/macOS
├── QUICKSTART.md                  ⭐ START HERE - 5-Minuten Quick-Start
├── DEMO_QUERIES.md                # 50+ Copy-Paste ready AQL Queries
├── KICKSTARTER_VIDEO_ANLEITUNG.md # Ausführliche Deutsche Anleitung
├── README.md                       # Diese Datei
├── setup/
│   ├── init-demo.ps1              # Legacy setup script (erstellt Daten+Queries)
│   └── setup-demo.ps1             # Erweiterte Setup-Optionen
├── queries/                        # 6 Demo-Queries (alle Use-Cases)
│   ├── 1_relational_join.sql       # SQL: Relational mit JOIN
│   ├── 2_vector_search.sql         # Vector: Semantic Search
│   ├── 3_graph_relationships.aql   # Graph: Beziehungen
│   ├── 4_timeseries_aggregation.sql # TimeSeries: Aggregation
│   ├── 5_llm_inference.aql         # LLM: Native Inference (kein Cloud!)
│   └── 6_rag_hallucination_detection.aql # RAG: mit Hallucination-Check
├── data/                           # Sample-Daten (automatisch erstellt)
│   ├── customers.csv               # 5 Kunden
│   ├── products.json               # 5 Produkte mit Embeddings
│   ├── orders.csv                  # 6 Bestellungen
│   └── themis.db                   # Datenbank (beim Start erstellt)
└── videos/
    ├── VIDEO_RECORDING_CHECKLIST.txt     # Schritt-für-Schritt Anleitung
    ├── SCREENSHOT_GUIDE.txt              # 6 Screenshots für Kickstarter
    └── screenshots/                      # Speichern Sie 6 PNGs hier
```

## 🚀 Quick Start (3 Schritte)

### ⭐ NEW: Automated Demo Script

**Run this for immediate full demo:**

```powershell
cd C:\Projects\ThemisDB
.\demo\kickstarter_demo_script.ps1
```

Das läuft automatisch durch alle Demo-Phasen:
- ✅ Server-Status Check
- ✅ Document/Text Search
- ✅ Vector Search (Semantic)
- ✅ Graph Traversal
- ✅ RAG/LLM Agent
- ✅ Performance Stats
- ✅ Index Recommendations

**Total Zeit:** ~12 Minuten (perfekt für Kickstarter!)

---

### 1. Demo-Daten generieren & importieren (3 Min)

```powershell
# Stelle sicher, dass ThemisDB Server läuft (Port 8765)
# Dann:

.\demo\setup\setup_demo_data.ps1
```

Das erstellt automatisch:
- ✅ **13 Research Articles** (AI, ML, Quantum, Databases, etc.)
- ✅ **13 Vector Embeddings** (128-dimensional)
- ✅ **Knowledge Graph** (10 Researchers, 7 Papers, 4 Conferences + Edges)
- ✅ Alle Collections in ThemisDB importiert
- ✅ Daten verifiziert und bereit zur Demo

**Details:** [setup/SETUP_INSTRUCTIONS.md](setup/SETUP_INSTRUCTIONS.md)

### 2. Video aufnehmen (5 Min)

**Schritt A: Video-Checklist lesen**
```
cat demo\videos\VIDEO_RECORDING_CHECKLIST.txt
```

**Schritt B: ThemisDB starten**
```bash
docker compose up -d
# oder: themis_server --data-dir ./demo/data --port 8765
```

**Schritt C: Queries nacheinander ausführen**
- Öffnen Sie die 6 Query-Dateien
- Kopieren Sie jede Query in den ThemisDB CLI
- Sprechen Sie während die Query läuft (nicht danach)

**Timeline (Single-Take, ~4-5 Min):**
- 0:00-0:30  → Intro ("Willkommen zu ThemisDB")
- 0:30-1:30  → Query 1: SQL Join
- 1:30-2:30  → Query 2: Vector Search
- 2:30-3:30  → Query 3: Graph Relationships
- 3:30-4:30  → Query 4+5: LLM + RAG
- 4:30-5:00  → Summary (Performance, Features)

### 3. Screenshots erstellen (5 Min)

**Schritt A: Screenshot-Anleitung lesen**
```
cat demo\videos\SCREENSHOT_GUIDE.txt
```

**Schritt B: 6 Screenshots**
1. `01-sql-query-results.png` - SQL Query mit Ergebnissen
2. `02-vector-search-results.png` - Vector Search mit Scores
3. `03-graph-relationships.png` - Graph mit Nodes+Edges
4. `04-llm-inference-local.png` - LLM Output
5. `05-performance-metrics.png` - Performance Stats
6. `06-multi-model-architecture.png` - Alle 6 Model-Types

**Speichern unter:** `demo/videos/screenshots/`

## 📊 Demo Scenarios

### 1️⃣ Relational (SQL)
**Was zeigt es:** ACID-Transaktionen, JOINs über multiple Tabellen

```sql
SELECT o.order_id, c.name, p.name, o.total_amount
FROM orders o
JOIN customers c ON o.customer_id = c.customer_id
JOIN products p ON o.product_id = p.product_id
WHERE c.country = 'DE';
```

**Voiceover:** "Hier sehen Sie klassische relationale Daten mit ACID-Garantien."

---

### 2️⃣ Vector Search (Semantik)
**Was zeigt es:** Semantische Suche ohne exakte Keywords

```sql
SELECT * FROM vector_search(
  query='renewable energy battery storage',
  top_k=5
);
```

**Voiceover:** "Die Vector-Suche versteht Bedeutung. Statt Keywords suchen wir semantisch."

---

### 3️⃣ Graph (Beziehungen)
**Was zeigt es:** Netzwerk-Analysen, Recommendation Engines

```
MATCH (c:Customer)-[:PLACED]->(o:Order)-[:CONTAINS]->(p:Product)
RETURN c.name, COUNT(o) AS order_count, SUM(o.total_amount) AS spent;
```

**Voiceover:** "Die Graph-Engine zeigt Beziehungen zwischen Kunden, Bestellungen und Produkten."

---

### 4️⃣ Time-Series (Aggregation)
**Was zeigt es:** Zeitbasierte Analysen, Trends

```sql
SELECT DATE_TRUNC('week', order_date) AS week,
       COUNT(*) AS order_count,
       SUM(total_amount) AS revenue
FROM orders
GROUP BY week;
```

**Voiceover:** "Time-Series zeigt Trends über Zeit."

---

### 5️⃣ LLM Inference (LOKAL!)
**Was zeigt es:** Native AI ohne Cloud, volle Datenkontrolle

```
SELECT llm_generate(
  prompt='Write marketing copy for: Solar Panel 400W'
);
```

**Voiceover:** "ThemisDB hat NATIVE LLM-Inferenz! Keine Cloud-Calls, volle Datensicherheit."

---

### 6️⃣ RAG + Hallucination Detection
**Was zeigt es:** Sichere AI mit Fact-Checking

```
SELECT rag_answer(
  question='What energy solutions do we offer?',
  check_hallucination=true,
  confidence_threshold=0.75
);
```

**Voiceover:** "RAG-Pipeline mit automatischer Hallucination-Detection."

---

## 💡 Recording Tips

### ✅ TUN:

- Langsam sprechen (nicht gehetzt)
- Erklären WÄHREND du Input gibst (nicht danach)
- Ein Fehler ist OK (authentic)
- Font vergrößern (14-16pt)
- Terminal maximiert
- Clear audio (keine Hintergrundgeräusche)

### ❌ NICHT TUN:

- Keine Bearbeitung (must be single-take!)
- Keine Musik/Sound-Effekte
- Keine AI-generierte Voiceover
- Keine schnellen Cuts
- Nicht zu schnell klicken

## 📹 Video Specs

- **Format:** MP4 (H.264)
- **Resolution:** 1280x720 (HD) oder höher
- **Duration:** 4-5 Minuten
- **Audio:** Clear, 44.1kHz, English
- **File size:** < 500MB
- **Save to:** `demo/videos/themisdb-demo-video.mp4`

## 🎯 Kickstarter Submission

Nach Video + Screenshots:

1. ✅ Kopieren Sie Video zu Kickstarter (Demo Video section)
2. ✅ Kopieren Sie 6 Screenshots zu Kickstarter (Gallery)
3. ✅ Fügen Sie English-Transkription/Subtitles hinzu
4. ✅ Ergänzen Sie "Use of AI" section
5. ✅ Fügen Sie vollständige English-Übersetzungen hinzu (Story, Rewards, Captions)
6. ✅ Reichen Sie Projekt erneut ein

## ❓ Troubleshooting

| Problem | Lösung |
|---------|--------|
| ThemisDB startet nicht | `cmake --build --preset msvc-ninja-release` zuerst ausführen |
| Port 8765 bereits belegt | Andere Prozesse beenden oder anderer Port |
| Daten werden nicht geladen | CSV-Format prüfen (UTF-8, richtige Trennzeichen) |
| LLM-Queries schlagen fehl | llama.cpp models herunterladen (SETUP.md) |
| Screenshots zu klein | Terminal vergrößern, Font 14-16pt |

## 📚 Weitere Ressourcen

- [Kickstarter Prototype Guide](https://www.kickstarter.com/blog/pointers-for-sharing-your-prototype)
- [Kickstarter Creator Handbook](https://www.kickstarter.com/help/handbook)
- ThemisDB Docs: [QUICKSTART.md](../../QUICKSTART.md)
- ThemisDB Examples: [examples/](../../examples/)

## ✨ Success Checklist

- [ ] ✅ Demo-Umgebung initialisiert (`init-demo.ps1` ausgeführt)
- [ ] 📹 Video aufgenommen (4-5 Min, single-take, MP4)
- [ ] 📸 6 Screenshots erstellt (HD, PNG)
- [ ] 📝 English-Transkription hinzugefügt
- [ ] 🤖 AI Disclosure section komplett
- [ ] 🇬🇧 Story, Rewards, Captions ins English übersetzt
- [ ] ✅ Video + Screenshots zu Kickstarter hochgeladen
- [ ] ✅ Projekt erneut eingereicht

---

**Status:** ✅ READY FOR DEMO

Viel Erfolg! 🚀
