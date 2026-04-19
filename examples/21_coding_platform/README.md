> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Coding Platform - ThemisDB als intelligente Code-Verwaltung mit VSCode Integration

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-complex-red)
![Duration](https://img.shields.io/badge/duration-90--120%20min-blue)

## 📝 Übersicht

Eine vollständige Coding-Plattform, die ThemisDB als Backend nutzt, um Code-Snippets, Projekte und Dokumentationen zu verwalten. Die Plattform bietet VSCode-Integration und kann Code-Beispiele aus dem Internet scrapen und intelligent indexieren.

## ✨ Features

### Core Features
- ✅ **Code-Snippet-Verwaltung** - Speichern und organisieren von Code-Fragmenten
- ✅ **Projekt-Management** - Vollständige Projekte mit Dateien und Strukturen
- ✅ **Dokumentations-Hub** - Technische Dokumentationen zentral verwalten
- ✅ **Tagging & Kategorisierung** - Flexible Organisation nach Sprachen, Frameworks, Topics

### Web Scraping & Ingestion
- ✅ **Internet-Quellen scrapen** - Automatisches Extrahieren von Code-Beispielen
- ✅ **GitHub-Integration** - Repositories und Gists importieren
- ✅ **Stack Overflow** - Fragen und Antworten crawlen
- ✅ **Dokumentations-Crawler** - ReadTheDocs, DevDocs, etc.
- ✅ **Smart Parsing** - Automatische Erkennung von Code-Blöcken und Sprachen
- ✅ **Duplikat-Erkennung** - Ähnliche Code-Snippets identifizieren

### Semantic Search & AI
- ✅ **Vector Search** - Semantische Code-Suche mit Embeddings
- ✅ **Code2Vec** - Code in hochdimensionale Vektoren konvertieren
- ✅ **Ähnlichkeits-Suche** - "Finde ähnlichen Code"
- ✅ **Natural Language Queries** - "Zeige mir Python Beispiele für async/await"
- ✅ **Code-Empfehlungen** - Basierend auf aktuellem Kontext

### VSCode Integration
- ✅ **Extension** - Native VSCode-Erweiterung
- ✅ **Snippet-Browser** - Code-Snippets direkt in VSCode durchsuchen
- ✅ **Quick Insert** - Snippets mit einem Klick einfügen
- ✅ **Context-Aware** - Vorschläge basierend auf aktuellem Code
- ✅ **Sync** - Lokale Snippets mit ThemisDB synchronisieren

### Tkinter Desktop App
- ✅ **Code-Editor** - Syntax-Highlighting und Bearbeitung
- ✅ **Tree-View** - Hierarchische Projektstruktur
- ✅ **Search Interface** - Leistungsstarke Suchfunktionen
- ✅ **Import/Export** - Batch-Import von Code-Dateien
- ✅ **Preview** - Live-Vorschau von Code-Rendering

## 📊 Datenmodell

### CodeSnippet
```python
{
    "id": "snippet_uuid",
    "title": "Async HTTP Request in Python",
    "description": "Example of async HTTP requests using aiohttp",
    "code": "import aiohttp\n...",
    "language": "python",
    "framework": "aiohttp",
    "tags": ["async", "http", "networking"],
    "embedding": [0.123, -0.456, ...],  # 512D code embedding
    "metadata": {
        "author": "max@example.com",
        "source_url": "https://github.com/...",
        "source_type": "github",
        "license": "MIT",
        "stars": 125,
        "created_at": "2025-12-22T10:00:00Z",
        "updated_at": "2025-12-22T15:30:00Z"
    },
    "stats": {
        "views": 245,
        "copies": 18,
        "likes": 12
    }
}
```

### Project
```python
{
    "id": "project_uuid",
    "name": "FastAPI REST API Example",
    "description": "Complete REST API with authentication",
    "language": "python",
    "framework": "fastapi",
    "files": [
        {
            "path": "main.py",
            "content": "from fastapi import FastAPI\n...",
            "language": "python"
        },
        {
            "path": "requirements.txt",
            "content": "fastapi==0.104.1\n...",
            "language": "text"
        }
    ],
    "structure": {
        "type": "tree",
        "root": {
            "name": "project_root",
            "children": [...]
        }
    },
    "dependencies": ["fastapi", "uvicorn", "pydantic"],
    "readme": "# FastAPI Example\n...",
    "tags": ["api", "rest", "authentication"],
    "metadata": {
        "source_url": "https://github.com/...",
        "source_type": "github_repo",
        "stars": 1500,
        "forks": 234
    }
}
```

### Documentation
```python
{
    "id": "doc_uuid",
    "title": "Python AsyncIO Guide",
    "content": "# AsyncIO\n\nAsync programming in Python...",
    "type": "guide",  # guide, tutorial, reference, api_doc
    "language": "python",
    "framework": "asyncio",
    "sections": [
        {
            "title": "Introduction",
            "content": "...",
            "code_examples": ["snippet_id_1", "snippet_id_2"]
        }
    ],
    "embedding": [0.234, -0.567, ...],
    "metadata": {
        "source_url": "https://docs.python.org/...",
        "source_type": "official_docs",
        "version": "3.12",
        "last_updated": "2025-12-01"
    },
    "related_snippets": ["snippet_uuid_1", "snippet_uuid_2"],
    "related_projects": ["project_uuid_1"]
}
```

### ScrapingJob
```python
{
    "id": "job_uuid",
    "type": "github_repo",  # github_repo, stackoverflow, docs_site
    "source_url": "https://github.com/user/repo",
    "status": "completed",  # pending, running, completed, failed
    "config": {
        "max_depth": 3,
        "file_patterns": ["*.py", "*.js", "*.md"],
        "exclude_patterns": ["test_*", "*_test.py"],
        "min_file_size": 100,
        "max_file_size": 100000
    },
    "results": {
        "snippets_created": 45,
        "projects_created": 1,
        "docs_created": 8,
        "duplicates_found": 12,
        "errors": 2
    },
    "started_at": "2025-12-22T10:00:00Z",
    "completed_at": "2025-12-22T10:15:23Z",
    "error_log": []
}
```

## 🔧 Installation

### Voraussetzungen

```bash
# Python 3.8+
python --version

# ThemisDB Server
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  themisdb/themisdb:latest

# Node.js für VSCode Extension (optional)
node --version  # v16+
```

### Setup

```bash
cd examples/21_coding_platform

# Python Dependencies installieren
pip install -r requirements.txt

# Embedding-Modell herunterladen (automatisch beim ersten Start)
python -c "from sentence_transformers import SentenceTransformer; SentenceTransformer('microsoft/codebert-base')"
```

### VSCode Extension installieren (Optional)

```bash
cd vscode_extension
npm install
npm run compile

# Extension in VSCode laden
# Drücke F5 in VSCode, um Extension Development Host zu starten
```

## 🚀 Verwendung

### Desktop-Anwendung starten

```bash
python main.py
```

### Web Scraping

```python
# GitHub Repository scrapen
from web_scraper import GitHubScraper

scraper = GitHubScraper()
job_id = scraper.scrape_repository(
    url="https://github.com/fastapi/fastapi",
    max_files=100,
    file_patterns=["*.py"]
)

# Status prüfen
status = scraper.get_job_status(job_id)
print(f"Scraped {status['results']['snippets_created']} snippets")
```

```python
# Stack Overflow scrapen
from web_scraper import StackOverflowScraper

scraper = StackOverflowScraper()
job_id = scraper.scrape_questions(
    tags=["python", "asyncio"],
    min_score=10,
    max_questions=50
)
```

```python
# Dokumentation crawlen
from web_scraper import DocsCrawler

crawler = DocsCrawler()
job_id = crawler.crawl_documentation(
    base_url="https://docs.python.org/3/library/asyncio.html",
    max_depth=2
)
```

### Semantic Code Search

```python
from code_indexer import CodeIndexer
from themis_client import ThemisClient

client = ThemisClient()
indexer = CodeIndexer(client)

# Natural Language Query
results = indexer.search_by_description(
    query="asynchronous HTTP requests in Python",
    limit=10
)

for snippet in results:
    print(f"{snippet['title']} - Similarity: {snippet['score']:.2f}")
    print(snippet['code'][:200])
    print("---")
```

### VSCode Integration

1. **Extension installieren**: Siehe Installation
2. **Command Palette öffnen** (`Ctrl+Shift+P`)
3. **"ThemisDB: Search Snippets"** eingeben
4. **Suchbegriff** eingeben
5. **Snippet auswählen** und automatisch einfügen

Oder:

- **Sidebar öffnen**: ThemisDB Icon in der Activity Bar
- **Snippets durchsuchen**: Hierarchische Ansicht nach Sprachen
- **Drag & Drop**: Snippet in Editor ziehen

## 📖 Detaillierte Guides

- [HOW_TO.md](HOW_TO.md) - Schritt-für-Schritt Bedienungsanleitung
- [VSCODE_INTEGRATION.md](VSCODE_INTEGRATION.md) - VSCode Extension Setup
- [WEB_SCRAPING.md](WEB_SCRAPING.md) - Web Scraping und Ingestion
- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Architektur und Design

## 🏗️ Architektur

```
┌─────────────────────────────────────────────────────────┐
│                    Tkinter Desktop App                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐  │
│  │  Editor  │  │  Search  │  │  Scraper Dashboard  │  │
│  └──────────┘  └──────────┘  └──────────────────────┘  │
└────────────────────┬────────────────────────────────────┘
                     │
         ┌───────────┴───────────┐
         │                       │
┌────────▼────────┐     ┌────────▼─────────┐
│  Code Indexer   │     │   Web Scraper    │
│  - Embeddings   │     │  - GitHub        │
│  - Vector DB    │     │  - StackOverflow │
│  - Similarity   │     │  - Docs Sites    │
└────────┬────────┘     └────────┬─────────┘
         │                       │
         └───────────┬───────────┘
                     │
            ┌────────▼────────┐
            │  ThemisDB API   │
            │  - Vector Model │
            │  - Doc Model    │
            │  - Graph Model  │
            └────────┬────────┘
                     │
            ┌────────▼────────┐
            │  ThemisDB Core  │
            └─────────────────┘

┌─────────────────────────────────────────┐
│         VSCode Extension                │
│  ┌──────────┐  ┌─────────────────────┐  │
│  │ Sidebar  │  │  Command Palette    │  │
│  │ Browser  │  │  - Search           │  │
│  │          │  │  - Insert           │  │
│  │          │  │  - Sync             │  │
│  └──────────┘  └─────────────────────┘  │
└────────────┬────────────────────────────┘
             │ REST API
             │
    ┌────────▼────────┐
    │  ThemisDB API   │
    └─────────────────┘
```

## 🎯 Use Cases

### 1. Persönliche Code-Bibliothek
Sammeln und organisieren Sie Ihre eigenen Code-Snippets:
```python
# Snippet speichern
client.create_snippet(
    title="JWT Authentication Decorator",
    code=my_decorator_code,
    language="python",
    tags=["auth", "jwt", "decorator"]
)

# Später finden
snippets = client.search_snippets(
    query="authentication decorator",
    language="python"
)
```

### 2. Team Knowledge Base
Teilen Sie Code-Wissen im Team:
```python
# Projekt importieren
project = client.import_project_from_github(
    url="https://github.com/company/internal-lib",
    visibility="team"
)

# Team-Mitglieder können suchen
results = client.search_projects(
    query="database connection pooling",
    team="backend"
)
```

### 3. Learning Platform
Lernen Sie aus echtem Code:
```python
# Beispiele von GitHub sammeln
scraper.scrape_repositories(
    search_query="fastapi authentication",
    min_stars=100,
    max_repos=20
)

# Tutorials generieren
tutorial = client.generate_tutorial(
    topic="FastAPI JWT Authentication",
    level="beginner"
)
```

### 4. Code Review Assistant
Finden Sie Best Practices:
```python
# Ähnlichen Code finden
similar = client.find_similar_code(
    code=my_implementation,
    language="python",
    min_similarity=0.7
)

# Best Practices identifizieren
best_practices = [s for s in similar if s['metadata']['stars'] > 500]
```

## 🛠️ Technologie-Stack

### Backend
- **ThemisDB** - Multi-Model Database
  - Vector Model für Code-Embeddings
  - Document Model für Snippets und Docs
  - Graph Model für Beziehungen
- **Python 3.8+** - Hauptsprache

### Frontend
- **Tkinter** - Desktop GUI
- **tkinter-code-editor** - Syntax-Highlighting

### Web Scraping
- **requests** - HTTP Client
- **beautifulsoup4** - HTML Parsing
- **PyGithub** - GitHub API Client
- **selenium** (optional) - JavaScript-rendered Seiten

### Code Analysis
- **tree-sitter** - Code Parsing
- **pygments** - Syntax-Highlighting
- **sentence-transformers** - Text-Embeddings
- **microsoft/codebert-base** - Code-Embeddings

### VSCode Extension
- **TypeScript** - Extension Sprache
- **VSCode Extension API** - Integration
- **axios** - API Client

## 📋 Datei-Struktur

```
21_coding_platform/
├── README.md                      # Diese Datei
├── HOW_TO.md                      # Bedienungsanleitung
├── VSCODE_INTEGRATION.md          # VSCode Setup
├── WEB_SCRAPING.md                # Scraping Guide
├── ARCHITECTURE.md                # System-Design
├── requirements.txt               # Python Dependencies
├── main.py                        # Desktop App Entry Point
├── themis_client.py               # ThemisDB Client
├── models.py                      # Datenmodelle
├── web_scraper.py                 # Web Scraping Module
├── code_indexer.py                # Code Embedding & Search
├── ui/
│   ├── main_window.py            # Hauptfenster
│   ├── editor_panel.py           # Code-Editor
│   ├── search_panel.py           # Suchoberfläche
│   ├── scraper_panel.py          # Scraper Dashboard
│   └── project_tree.py           # Projekt-Browser
├── scrapers/
│   ├── github_scraper.py         # GitHub Integration
│   ├── stackoverflow_scraper.py  # Stack Overflow
│   └── docs_crawler.py           # Dokumentations-Crawler
├── vscode_extension/
│   ├── package.json              # Extension Manifest
│   ├── src/
│   │   ├── extension.ts          # Main Extension
│   │   ├── snippetProvider.ts   # Snippet TreeView
│   │   └── api_client.ts        # ThemisDB API Client
│   └── README.md                 # Extension Docs
└── tests/
    ├── test_scraper.py
    ├── test_indexer.py
    └── test_client.py
```

## 🔒 Sicherheit & Best Practices

### API Keys
```python
# .env Datei verwenden
GITHUB_TOKEN=ghp_xxxxxxxxxxxxx
STACKOVERFLOW_KEY=xxxxxxxxxxxxx

# In Code laden
from dotenv import load_dotenv
load_dotenv()
```

### Rate Limiting
- GitHub API: Max 5000 requests/hour (authenticated)
- Stack Overflow: Max 300 requests/day (free tier)
- Implementiert mit exponential backoff

### Datenschutz
- Keine Speicherung von Passwörtern oder Tokens in ThemisDB
- Nur öffentlicher Code wird gescrapet
- Respektierung von robots.txt

## 🐛 Troubleshooting

### Scraper funktioniert nicht
```
Error: GitHub API rate limit exceeded
```
**Lösung**: GitHub Personal Access Token konfigurieren:
```bash
export GITHUB_TOKEN=ghp_your_token_here
```

### Embeddings zu langsam
```
Warning: Embedding generation taking > 5s per snippet
```
**Lösung**: GPU-Beschleunigung aktivieren:
```bash
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118
```

### VSCode Extension lädt nicht
```
Error: Cannot find module 'axios'
```
**Lösung**:
```bash
cd vscode_extension
npm install
npm run compile
```

## 📚 Weiterführende Ressourcen

- [ThemisDB Vector Search](../../docs/vector-search.md)
- [Code Embeddings Explained](https://huggingface.co/microsoft/codebert-base)
- [VSCode Extension API](https://code.visualstudio.com/api)
- [GitHub API Documentation](https://docs.github.com/en/rest)

## 🤝 Beitragen

Wollen Sie neue Scraper hinzufügen oder Features verbessern?

1. Forken Sie das Repository
2. Erstellen Sie einen Feature Branch
3. Implementieren Sie Ihre Änderungen
4. Fügen Sie Tests hinzu
5. Erstellen Sie einen Pull Request

### Neue Scraper hinzufügen

```python
# scrapers/my_scraper.py
from web_scraper import BaseScraper

class MyScraper(BaseScraper):
    def scrape(self, url: str) -> List[CodeSnippet]:
        # Ihre Implementierung
        pass
```

## 📄 Lizenz

Dieses Beispiel ist unter der MIT-Lizenz lizenziert - siehe [LICENSE](../../LICENSE) für Details.

## 🆘 Support

Bei Fragen oder Problemen:
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)

---

**Nächste Schritte**: 
1. Starten Sie mit [HOW_TO.md](HOW_TO.md) für eine detaillierte Anleitung
2. Lesen Sie [WEB_SCRAPING.md](WEB_SCRAPING.md) um Code aus dem Internet zu importieren
3. Installieren Sie die [VSCode Extension](VSCODE_INTEGRATION.md) für nahtlose Integration

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-24
