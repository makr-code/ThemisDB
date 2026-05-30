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
| 1 | **Health Check** | `themisctl --host 127.0.0.1 --port 8765 health` | 30 sec |
| 2 | **Deterministic Retrieval** | `themisctl api GET /entities/demo_articles:art_0001` | 1 min |
| 3 | **Graph Vernetzung** | `themisctl api POST /graphql --stdin --content-type application/json` | 2 min |
| 4 | **LLM Model Load** | `themisctl api POST /api/v1/llm/models/load --stdin --content-type application/json` | 1 min |
| 5 | **LLM Zusammenfassung** | `themisctl api POST /api/v1/llm/inference --stdin --content-type application/json` | 2 min |
| 6 | **RAG LLM Query** | `themisctl api POST /api/v1/llm/rag --stdin --content-type application/json` | 2 min |
| 7 | **Entities Readback (Safe Anchor)** | `themisctl api GET /entities/demo_articles:art_0001` | 1 min |
| 8 | **Recommendations** | `themisctl --host 127.0.0.1 --port 8765 index recommend demo_articles` | 1 min |

Hinweis zu Schritt 1:
- In aktuellen `themisctl`-Builds kann `schema` bei Erfolg ohne Ausgabe enden.
- Für sichtbare Reachability-Ausgabe nutze alternativ:

```powershell
.\build-msvc-windows-release\bin\themisctl.exe --host 127.0.0.1 --port 8765 health
# Erwartet: liveness: healthy / readiness: healthy
```

**Gesamtdauer:** ~10-12 Minuten (optimal)

---

## 🚀 Schritt-für-Schritt Demoscript

### Terminal vorbereiten
```powershell
# Größere Font (Terminal -> Einstellungen -> Appearance)
# Fenster: Vollbild oder 1920x1080

# RECORDING STARTEN ⏺️

# Empfohlen: offizielles Runbook starten (Step-by-Step mit erwarteter Ausgabe)
Get-Content .\demo\DEMO_QUERIES.md

# Danach die Schritte 3-13 in Reihenfolge aus DEMO_QUERIES.md ausführen.

# RECORDING STOPPEN ⏹️
```

---

## 💬 Was du sagen solltest (Sprechtext)

### Beim Starten:
> "This is ThemisDB - a production-grade multi-model database system. We're going to show you live demonstrations of its core capabilities."

### Bei Document Search:
> "First, we validate server reachability and core API flow. All further steps use reproducible API calls from the live runbook."

### Bei Vector Search:
> "Next, we run LLM inference and then graph explain to demonstrate low-latency planning plus AI-assisted output in one system."

### Bei Graph Traversal:
> "Then we show GraphQL schema introspection and an advanced GraphQL query with variables and aliases."

### Bei RAG:
> "Finally, we run a RAG endpoint call that combines retrieval and generation with measurable response metrics."

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
.\build-msvc-windows-release\bin\themisctl schema | Out-File -Encoding UTF8 schema_output.txt
# Screenshot davon

# 2. Graph Explain Result in JSON
'{"query_type":"k_hop","start_vertex":"demo_knowledge_graph:node_0001","max_depth":1}' | .\build-msvc-windows-release\bin\themisctl.exe --host 127.0.0.1 --port 8765 api POST /api/v1/graph/query/explain --stdin --content-type application/json
# Screenshot

# 3. GraphQL Schema (SDL)
.\build-msvc-windows-release\bin\themisctl.exe --host 127.0.0.1 --port 8765 api GET /graphql/schema
# Screenshot

# 4. LLM Inference Result
'{"prompt":"Summarize ACID in two sentences.","max_tokens":48,"temperature":0.2}' | .\build-msvc-windows-release\bin\themisctl.exe --timeout 180 --host 127.0.0.1 --port 8765 api POST /api/v1/llm/inference --stdin --content-type application/json
# Screenshot der JSON-Antwort
```

---

## 🔧 Troubleshooting

### Server läuft nicht?
```powershell
# Prüfe ob Server läuft
$THEMISCTL = ".\build-msvc-windows-release\bin\themisctl.exe"
& $THEMISCTL --version
# Should return: ThemisDB CLI v1.x.x
```

### Collections existieren nicht?
```powershell
# Demo-Daten erneut generieren/importieren
.\demo\setup\setup_demo_data.ps1
```

### Queries geben Fehler zurück?
- Prüfe zuerst Reachability: `themisctl --host 127.0.0.1 --port 8765 health`
- Nutze fuer den Live-Call bevorzugt die im Runbook dokumentierten stabilen Endpunkte.

---

## 📚 Weitere Ressourcen

- **themisctl Manual**: [docs/themisctl.md](../../docs/en/tools/themisctl.md)
- **RAG Features**: [docs/rag.md](../../docs/en/features/rag.md)

---

**Viel Erfolg! 🎬🚀**
