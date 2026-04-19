> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Coding Platform - Schritt-für-Schritt Anleitung

Diese Anleitung zeigt Ihnen, wie Sie die ThemisDB Coding Platform verwenden, um Code-Snippets zu verwalten, aus dem Internet zu scrapen und mit VSCode zu integrieren.

## 📋 Inhaltsverzeichnis

1. [Erste Schritte](#erste-schritte)
2. [Code-Snippets verwalten](#code-snippets-verwalten)
3. [Web Scraping einrichten](#web-scraping-einrichten)
4. [Semantic Search verwenden](#semantic-search-verwenden)
5. [VSCode Integration](#vscode-integration)
6. [Projekte importieren](#projekte-importieren)
7. [Erweiterte Features](#erweiterte-features)

## 1. Erste Schritte

### ThemisDB Server starten

```bash
# Docker Container starten
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  themisdb/themisdb:latest

# Warten bis Server bereit ist
curl http://localhost:8080/health
```

**Erwartete Ausgabe**:
```json
{"status": "healthy", "version": "1.3.0"}
```

### Desktop-Anwendung starten

```bash
cd examples/21_coding_platform
pip install -r requirements.txt
python main.py
```

**Was Sie sehen sollten**:
- Ein Fenster mit mehreren Tabs
- "Snippets", "Projects", "Search", "Scraper"
- Status-Anzeige unten: "Connected to ThemisDB"

### Erster Health-Check

1. Klicken Sie auf **"Connection"** → **"Test Connection"**
2. Sie sollten eine Bestätigung sehen: ✅ "ThemisDB connected successfully"

## 2. Code-Snippets verwalten

### Neues Snippet erstellen

**Schritt 1**: Tab "Snippets" öffnen

**Schritt 2**: Button "New Snippet" klicken

**Schritt 3**: Formular ausfüllen:
```
Title:       Fast File Reading in Python
Language:    python
Description: Efficient way to read large files line by line
Tags:        file-io, performance, python
```

**Schritt 4**: Code eingeben:
```python
def read_large_file(file_path):
    with open(file_path, 'r') as file:
        for line in file:
            # Process line by line
            yield line.strip()

# Usage
for line in read_large_file('large_file.txt'):
    print(line)
```

**Schritt 5**: Button "Save Snippet" klicken

✅ **Erfolg**: Snippet erscheint in der Liste

### Snippet bearbeiten

1. **Snippet auswählen** aus der Liste
2. **Edit-Button** klicken
3. **Änderungen vornehmen**
4. **"Update Snippet"** klicken

### Snippet löschen

1. **Snippet auswählen**
2. **Delete-Button** klicken
3. **Bestätigen** im Dialog

### Snippet-Details anzeigen

**Doppelklick** auf Snippet in der Liste zeigt:
- Vollständigen Code
- Metadata (Autor, Datum, Quelle)
- Statistiken (Views, Copies, Likes)
- Ähnliche Snippets

## 3. Web Scraping einrichten

### GitHub Personal Access Token erstellen

**Warum benötigt?** GitHub API-Rate-Limit ist ohne Token sehr niedrig (60 req/hour vs 5000 req/hour).

**Schritt 1**: GitHub Settings öffnen
- Gehen Sie zu https://github.com/settings/tokens
- Klicken Sie "Generate new token" → "Generate new token (classic)"

**Schritt 2**: Token konfigurieren
- Name: `ThemisDB Scraper`
- Scopes auswählen:
  - ✅ `public_repo` (Zugriff auf öffentliche Repos)
  - ✅ `read:user` (Benutzerinfo lesen)
- Klicken Sie "Generate token"

**Schritt 3**: Token speichern
```bash
# .env Datei im examples/21_coding_platform/ erstellen
echo "GITHUB_TOKEN=ghp_your_token_here" > .env
```

### GitHub Repository scrapen

**Schritt 1**: Tab "Scraper" öffnen

**Schritt 2**: "GitHub Repository" auswählen

**Schritt 3**: URL eingeben:
```
Repository URL: https://github.com/fastapi/fastapi
```

**Schritt 4**: Optionen konfigurieren:
```
Max Files:       100
File Patterns:   *.py, *.md
Exclude:         test_*, *_test.py
Min Stars:       -
Branch:          main
```

**Schritt 5**: Button "Start Scraping" klicken

**Was passiert**:
1. ⏳ Job wird erstellt und gestartet
2. 📊 Progress Bar zeigt Fortschritt
3. 📝 Log zeigt Details:
   ```
   [INFO] Cloning repository...
   [INFO] Found 234 Python files
   [INFO] Processing: fastapi/main.py
   [INFO] Created snippet: FastAPI Application Setup
   [INFO] Processing: fastapi/routing.py
   ...
   ```
4. ✅ Job completed: "45 snippets created, 12 duplicates skipped"

### Stack Overflow Fragen scrapen

**Schritt 1**: "Stack Overflow" auswählen

**Schritt 2**: Tags eingeben:
```
Tags:       python, asyncio
Min Score:  10
Max Posts:  50
```

**Schritt 3**: Button "Start Scraping" klicken

**Ergebnis**: 
- Fragen mit hohen Scores werden importiert
- Code-Blöcke aus Antworten werden als Snippets gespeichert
- Links zu Original-Posts werden in Metadata gespeichert

### Dokumentation crawlen

**Schritt 1**: "Documentation" auswählen

**Schritt 2**: URL eingeben:
```
Base URL:    https://docs.python.org/3/library/asyncio.html
Max Depth:   2
```

**Schritt 3**: "Start Crawling" klicken

**Ergebnis**:
- Dokumentations-Seiten werden gecrawlt
- Code-Beispiele werden extrahiert
- Verlinkungen zwischen Seiten werden als Graph gespeichert

## 4. Semantic Search verwenden

### Text-basierte Suche

**Schritt 1**: Tab "Search" öffnen

**Schritt 2**: Suchtyp "Semantic Search" auswählen

**Schritt 3**: Query eingeben:
```
Query: asynchronous HTTP requests in Python
```

**Schritt 4**: Enter drücken oder "Search" klicken

**Ergebnisse**:
```
1. Async HTTP with aiohttp (Similarity: 0.92)
   import aiohttp
   async def fetch(url):
       async with aiohttp.ClientSession() as session:
           ...

2. Concurrent requests with asyncio (Similarity: 0.87)
   import asyncio
   import requests
   ...

3. FastAPI async endpoints (Similarity: 0.82)
   from fastapi import FastAPI
   @app.get("/async")
   async def read_async():
       ...
```

### Code-Similarity Suche

**Schritt 1**: Tab "Search" öffnen

**Schritt 2**: "Find Similar Code" auswählen

**Schritt 3**: Code-Beispiel eingeben:
```python
async def download_file(url, filename):
    async with aiohttp.ClientSession() as session:
        async with session.get(url) as resp:
            with open(filename, 'wb') as fd:
                async for chunk in resp.content.iter_chunked(1024):
                    fd.write(chunk)
```

**Schritt 4**: "Find Similar" klicken

**Ergebnisse**: Ähnliche Code-Snippets mit Similarity-Score

### Filter kombinieren

**Advanced Search**:
```
Query:           HTTP requests
Language:        python
Framework:       aiohttp
Min Stars:       50
Tags:            async, networking
Date Range:      Last 6 months
```

## 5. VSCode Integration

### Extension installieren

**Schritt 1**: Extension kompilieren
```bash
cd vscode_extension
npm install
npm run compile
```

**Schritt 2**: Extension laden
- Öffnen Sie VSCode
- Drücken Sie `F5` (Extension Development Host)
- Oder: Extension als VSIX paketieren
  ```bash
  npm run package
  code --install-extension themisdb-snippets-0.0.1.vsix
  ```

**Schritt 3**: Extension konfigurieren
- `Ctrl+,` → Settings öffnen
- "ThemisDB" suchen
- Konfigurieren:
  ```json
  {
    "themisdb.apiUrl": "http://localhost:8080",
    "themisdb.autoSync": true,
    "themisdb.suggestOnType": true
  }
  ```

### Snippets durchsuchen

**Methode 1: Command Palette**
1. `Ctrl+Shift+P` drücken
2. "ThemisDB: Search Snippets" eingeben
3. Suchbegriff eingeben
4. Snippet auswählen → wird an Cursor-Position eingefügt

**Methode 2: Sidebar**
1. ThemisDB Icon in Activity Bar klicken
2. Hierarchische Ansicht nach Sprachen:
   ```
   📁 Python
     📁 FastAPI
       📄 JWT Authentication
       📄 Async Endpoints
     📁 Django
       📄 Custom Middleware
   📁 JavaScript
     📁 React
       📄 Custom Hooks
   ```
3. Snippet anklicken → Code-Preview
4. "Insert" klicken oder Drag & Drop

**Methode 3: Context Menu**
1. Text im Editor markieren
2. Rechtsklick → "ThemisDB: Find Similar Code"
3. Ähnliche Snippets werden angezeigt

### Auto-Suggestions

Wenn `themisdb.suggestOnType: true`:

**Beispiel**: Sie schreiben:
```python
# JWT authentication in F
```

**Auto-Suggest** erscheint:
```
💡 ThemisDB Suggestion:
   "JWT Authentication in FastAPI"
   
   @app.post("/login")
   async def login(credentials: LoginSchema):
       ...
```

Drücken Sie `Tab` zum Einfügen.

### Snippet synchronisieren

**Lokales Snippet zu ThemisDB pushen**:
1. Code im Editor markieren
2. `Ctrl+Shift+P` → "ThemisDB: Save as Snippet"
3. Titel und Tags eingeben
4. Enter → Snippet wird zu ThemisDB hochgeladen

## 6. Projekte importieren

### Von GitHub importieren

**Schritt 1**: Tab "Projects" öffnen

**Schritt 2**: "Import from GitHub" klicken

**Schritt 3**: Repository URL eingeben:
```
URL: https://github.com/tiangolo/fastapi
```

**Schritt 4**: Import-Optionen:
```
Import as:        Single Project
Include:          *.py, *.md, *.txt, requirements.txt
Exclude:          tests/, docs/, .github/
Max File Size:    1 MB
Analyze:          Yes (create embeddings)
```

**Schritt 5**: "Import" klicken

**Ergebnis**:
- Gesamtes Projekt wird importiert
- Datei-Struktur wird als Tree gespeichert
- Code-Dateien werden analysiert
- Dependencies werden erkannt
- README wird als Dokumentation gespeichert

### Projekt lokal exportieren

**Schritt 1**: Projekt auswählen

**Schritt 2**: "Export" → "Export to Folder" klicken

**Schritt 3**: Zielordner auswählen

**Schritt 4**: Export-Optionen:
```
☑ Include all files
☑ Preserve structure
☑ Include metadata (as .themisdb.json)
☐ Include dependencies
```

**Ergebnis**: Projekt wird in Ordner exportiert, bereit zur Verwendung

## 7. Erweiterte Features

### Batch-Import von lokalen Dateien

**Schritt 1**: "File" → "Batch Import" klicken

**Schritt 2**: Ordner auswählen

**Schritt 3**: Optionen konfigurieren:
```
File Types:      *.py, *.js, *.java, *.cpp
Recursive:       Yes
Auto-Tag:        Yes (based on path and content)
Create Project:  No (import as individual snippets)
```

**Ergebnis**: Alle Code-Dateien werden als Snippets importiert

### Custom Scrapers schreiben

**Beispiel**: GitLab Scraper

```python
# scrapers/gitlab_scraper.py
from web_scraper import BaseScraper
import requests

class GitLabScraper(BaseScraper):
    def __init__(self, token=None):
        self.base_url = "https://gitlab.com/api/v4"
        self.token = token
    
    def scrape_project(self, project_id):
        headers = {"PRIVATE-TOKEN": self.token} if self.token else {}
        
        # Get project files
        url = f"{self.base_url}/projects/{project_id}/repository/tree"
        response = requests.get(url, headers=headers)
        files = response.json()
        
        snippets = []
        for file in files:
            if file['type'] == 'blob' and file['name'].endswith('.py'):
                content = self.get_file_content(project_id, file['path'])
                snippet = self.create_snippet(
                    title=file['name'],
                    code=content,
                    language='python',
                    source_url=file['path']
                )
                snippets.append(snippet)
        
        return snippets
```

**Verwendung**:
```python
from scrapers.gitlab_scraper import GitLabScraper

scraper = GitLabScraper(token="your_gitlab_token")
snippets = scraper.scrape_project(project_id=12345)
```

### Embeddings regenerieren

Wenn ein besseres Embedding-Modell verfügbar ist:

```python
from code_indexer import CodeIndexer

indexer = CodeIndexer()

# Alle Snippets neu indexieren
indexer.reindex_all(
    model_name="microsoft/graphcodebert-base",
    batch_size=32
)
```

### Analytics Dashboard

**Statistiken anzeigen**:
1. Tab "Analytics" öffnen
2. Übersicht:
   ```
   Total Snippets:      1,234
   Total Projects:      45
   Total Docs:          89
   Languages:           Python (45%), JavaScript (30%), Java (15%)
   Most Used Tags:      async, api, database, authentication
   Storage Used:        234 MB
   ```

3. **Charts**:
   - Snippets über Zeit
   - Sprachen-Verteilung
   - Top Tags
   - Scraping-Aktivität

### Kollaboration

**Snippets teilen**:
1. Snippet auswählen
2. "Share" → "Generate Link" klicken
3. Link kopieren: `themisdb://snippet/abc123`
4. Kollege öffnet Link in seiner ThemisDB App
5. Snippet wird automatisch importiert

## 💡 Tipps & Tricks

### Hotkeys

| Aktion | Shortcut |
|--------|----------|
| Neue Suche | `Ctrl+F` |
| Neues Snippet | `Ctrl+N` |
| Snippet speichern | `Ctrl+S` |
| Snippet bearbeiten | `Ctrl+E` |
| Snippet löschen | `Del` |
| Syntax-Highlighting umschalten | `Ctrl+H` |
| Preview | `Space` |

### Workflow-Empfehlungen

**1. Code-Learning**:
```
1. GitHub Repos von Top-Projekten scrapen
2. Semantic Search für spezifische Patterns
3. Ähnliche Code-Stücke vergleichen
4. Best Practices identifizieren
```

**2. Team Knowledge Base**:
```
1. Team-Projekte importieren
2. Interne Standards als Snippets dokumentieren
3. VSCode Extension im Team verteilen
4. Snippets über ThemisDB teilen
```

**3. Interview Preparation**:
```
1. Stack Overflow Top-Antworten scrapen
2. Nach Algorithmen-Kategorien organisieren
3. Regelmäßig durchgehen mit Spaced Repetition
4. Eigene Lösungen als Snippets speichern
```

## 🐛 Häufige Probleme

### "Connection refused"
**Lösung**: ThemisDB Server läuft nicht
```bash
docker ps -a | grep themisdb
docker start themisdb
```

### "Rate limit exceeded" beim Scraping
**Lösung**: Warten oder Token konfigurieren
```bash
# GitHub Token in .env setzen
echo "GITHUB_TOKEN=ghp_xxx" >> .env
```

### Embeddings langsam
**Lösung**: GPU aktivieren oder kleineres Modell
```python
# In code_indexer.py
MODEL_NAME = "sentence-transformers/all-MiniLM-L6-v2"  # Schneller, kleinere Vektoren
```

### VSCode Extension funktioniert nicht
**Lösung**: 
```bash
cd vscode_extension
rm -rf node_modules
npm install
npm run compile
```

## 🎓 Weiterführende Guides

- [WEB_SCRAPING.md](WEB_SCRAPING.md) - Detaillierter Scraping-Guide
- [VSCODE_INTEGRATION.md](VSCODE_INTEGRATION.md) - VSCode Extension Development
- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Architektur verstehen

---

**Viel Erfolg mit der ThemisDB Coding Platform!** 🚀

Für weitere Fragen: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
