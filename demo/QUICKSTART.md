# ThemisDB Kickstarter Demo - Quick Start

## 📹 Sofort-Start für Demo-Video

### 1. Minute 0-2: Vorbereitung
```powershell
cd C:\Projects\ThemisDB

# Terminal öffnen und diese Datei ausführen:
.\demo\kickstarter_demo_script.ps1
```

Das war's! Das Script führt automatisch alle Demo-Schritte durch.

---

## 🎬 Video-Recording-Tools

### Empfohlen: OBS Studio
1. Download: https://obsproject.com/
2. "New Scene" -> "Display Capture"
3. Settings: 1920x1080, 60 FPS, H.264
4. Hit "Start Recording"
5. Run demo script
6. Hit "Stop Recording"
7. MP4 saved to your Videos folder

### Alternativ: Windows Snipping Tool
```powershell
# Windows 11/10
Win + Shift + R  # Starte Video-Aufnahme
```

---

## 📋 Was du zeigen solltest (Reihenfolge)

| Nummer | Feature | Command | Zeit |
|--------|---------|---------|------|
| 1 | **Schema Check** | `themisctl schema --host localhost:8765` | 30 sec |
| 2 | **Document Search** | `themisctl query "FOR doc IN demo_articles FILTER doc.title LIKE '%AI%' LIMIT 5 RETURN doc"` | 2 min |
| 3 | **Vector Search** | `themisctl query "FOR doc IN demo_embeddings LIMIT 5 RETURN doc"` | 2 min |
| 4 | **Graph Traversal** | `themisctl query "FOR node IN demo_graph ... LIMIT 8 RETURN node"` | 2 min |
| 5 | **RAG LLM Query** | `themisctl rag query --collection demo_articles "What is quantum computing?"` | 2 min |
| 6 | **Performance** | `themisctl admin stats` | 1 min |
| 7 | **Recommendations** | `themisctl index recommend --collection demo_articles` | 1 min |

**Gesamtdauer:** ~10-12 Minuten (optimal)

---

## 🚀 Schritt-für-Schritt Demoscript

### Terminal vorbereiten
```powershell
# Größere Font (Terminal -> Einstellungen -> Appearance)
# Fenster: Vollbild oder 1920x1080

# RECORDING STARTEN ⏺️

# Schritt 1: Intro
Write-Host "Starting ThemisDB Live Demo" -ForegroundColor Green
Write-Host ""

# Schritt 2: System Status
.\build\windows-release\bin\themisctl schema --host localhost:8765

# Schritt 3: Document Query
Write-Host "Demo: Full-Text Document Search" -ForegroundColor Cyan
.\build\windows-release\bin\themisctl query --host localhost:8765 `
  "FOR doc IN demo_articles FILTER doc.title LIKE '%AI%' SORT doc.published DESC LIMIT 5 RETURN doc.title"

# Schritt 4: Vector Query
Write-Host "Demo: Vector/Semantic Search" -ForegroundColor Cyan
.\build\windows-release\bin\themisctl query --host localhost:8765 `
  "FOR doc IN demo_embeddings LIMIT 5 RETURN doc"

# Schritt 5: Graph Query
Write-Host "Demo: Graph Traversal & Relationships" -ForegroundColor Cyan
.\build\windows-release\bin\themisctl query --host localhost:8765 `
  "FOR node IN demo_graph LIMIT 5 RETURN node"

# Schritt 6: RAG Demo
Write-Host "Demo: LLM-Powered Natural Language Query" -ForegroundColor Cyan
.\build\windows-release\bin\themisctl rag query `
  --collection demo_articles --top-k 3 `
  "What is machine learning and AI?"

# Schritt 7: Stats
Write-Host "System Performance Metrics" -ForegroundColor Cyan
.\build\windows-release\bin\themisctl admin stats --host localhost:8765

Write-Host "Demo Complete!" -ForegroundColor Green

# RECORDING STOPPEN ⏹️
```

---

## 💬 Was du sagen solltest (Sprechtext)

### Beim Starten:
> "This is ThemisDB - a production-grade multi-model database system. We're going to show you live demonstrations of its core capabilities."

### Bei Document Search:
> "First, traditional document search - like SQL queries. We're searching through millions of articles for papers about AI and machine learning."

### Bei Vector Search:
> "Next, vector search - semantic similarity. Our system can compare millions of embeddings against a query vector in milliseconds."

### Bei Graph Traversal:
> "Here's graph database functionality - finding relationships and paths through networks of data. We can traverse multi-hop relationships instantly."

### Bei RAG:
> "And now, the AI component - our LLM-powered RAG agent. You ask a natural language question, ThemisDB converts it to optimal queries and returns an AI-generated answer."

### Am Ende:
> "As you can see, ThemisDB is fully operational, handling all types of data models - documents, vectors, and graphs - in a single unified system."

---

## ✅ Checkliste vor Upload

- [ ] Video läuft 10-15 Minuten
- [ ] Font ist groß genug (mindestens 24pt)
- [ ] Mindestens 5 verschiedene Query-Typen
- [ ] LLM/RAG-Feature gezeigt
- [ ] Performance-Metriken sichtbar
- [ ] KEIN Schnitte oder Übergänge (ein Stück!)
- [ ] Sprache ist klar verständlich
- [ ] Auflösung min. 1280x720
- [ ] Format: MP4 oder WebM

---

## 📸 Screenshots für Kickstarter Seite

Nach dem Video, mache diese Screenshots:

```powershell
# 1. themisctl schema output
.\build\windows-release\bin\themisctl schema | Out-File -Encoding UTF8 schema_output.txt
# Screenshot davon

# 2. Query Results in JSON
.\build\windows-release\bin\themisctl query --output-format json "FOR doc IN demo_articles LIMIT 3 RETURN doc"
# Screenshot

# 3. Admin Stats
.\build\windows-release\bin\themisctl admin stats
# Screenshot

# 4. REPL Mode
.\build\windows-release\bin\themisctl repl
# Screenshot vom Prompt
```

---

## 🔧 Troubleshooting

### Server läuft nicht?
```powershell
# Prüfe ob Server läuft
$THEMISCTL = ".\build\windows-release\bin\themisctl.exe"
& $THEMISCTL --version
# Should return: ThemisDB CLI v1.x.x
```

### Collections existieren nicht?
```powershell
# Erstelle Test-Collections
.\build\windows-release\bin\themisdb-import \
  --collection demo_articles \
  --file data/sample_articles.jsonl
```

### Queries geben Fehler zurück?
- Prüfe Syntax (AQL Query Language)
- Prüfe ob Collections existieren
- Prüfe ob Server antwortet

---

## 📚 Weitere Ressourcen

- **AQL Query Language Docs**: [docs/aql.md](../../docs/en/query/aql.md)
- **themisctl Manual**: [docs/themisctl.md](../../docs/en/tools/themisctl.md)
- **RAG Features**: [docs/rag.md](../../docs/en/features/rag.md)

---

**Viel Erfolg! 🎬🚀**
