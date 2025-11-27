# RESPO Datenquellen für LLM-Training

## Übersicht: Quellen für Programmier-Grundverständnis

Dieses Dokument beschreibt alle verfügbaren Datenquellen für das Training eines Code-LLMs wie CodeLlama.

## 📊 Quellenübersicht

### 1. Code-Repositories (Hauptquelle für echten Code)

| Quelle | Scraper | Inhalt | Qualität |
|--------|---------|--------|----------|
| **GitHub** | `GitHubScraper` | Millionen Repos, alle Sprachen | ⭐⭐⭐⭐⭐ |
| **GitLab** | `GitLabScraper` | Open-Source Projekte | ⭐⭐⭐⭐ |
| **Bitbucket** | `BitbucketScraper` | Enterprise Projekte | ⭐⭐⭐ |

### 2. Paket-Registries (Qualitätsgeprüfter Code)

| Quelle | Scraper | Inhalt | Qualität |
|--------|---------|--------|----------|
| **PyPI** | `PyPIScraper` | Python Packages | ⭐⭐⭐⭐⭐ |
| **npm** | `NPMScraper` | JavaScript/Node Packages | ⭐⭐⭐⭐⭐ |
| **crates.io** | `CratesScraper` | Rust Crates | ⭐⭐⭐⭐⭐ |
| **Debian** | `DebianScraper` | Linux Systemtools | ⭐⭐⭐⭐ |

### 3. Q&A und Code-Snippets

| Quelle | Scraper | Inhalt | Qualität |
|--------|---------|--------|----------|
| **Stack Overflow** | `StackOverflowScraper` | Fragen + Antworten mit Code | ⭐⭐⭐⭐⭐ |
| **GitHub Gists** | `GistsScraper` | Code-Snippets | ⭐⭐⭐⭐ |

### 4. Dokumentation & Referenzen

| Quelle | Scraper | Inhalt | Qualität |
|--------|---------|--------|----------|
| **DevDocs** | `DevDocsScraper` | Aggregierte API Docs | ⭐⭐⭐⭐⭐ |
| **MDN** | `MDNScraper` | Web-Technologien | ⭐⭐⭐⭐⭐ |
| **cheat.sh** | `CheatShScraper` | Schnellreferenzen | ⭐⭐⭐⭐ |
| **tldr-pages** | `TLDRScraper` | Vereinfachte Man-Pages | ⭐⭐⭐⭐ |

### 5. Lernmaterial & Tutorials

| Quelle | Scraper | Inhalt | Qualität |
|--------|---------|--------|----------|
| **Learn X in Y Minutes** | `LearnXInYMinutesScraper` | Sprach-Einführungen | ⭐⭐⭐⭐⭐ |
| **Rosetta Code** | `RosettaCodeScraper` | Algorithmen in 700+ Sprachen | ⭐⭐⭐⭐⭐ |
| **LeetCode Solutions** | `LeetCodeScraper` | Algorithmen & Datenstrukturen | ⭐⭐⭐⭐⭐ |
| **Design Patterns** | `DesignPatternsScraper` | Software-Architektur | ⭐⭐⭐⭐⭐ |
| **Python PEPs** | `PythonPEPsScraper` | Python Best Practices | ⭐⭐⭐⭐⭐ |

---

## 🎯 Empfohlene Trainings-Strategie

### Phase 1: Grundverständnis (Pre-Training Data)

```
Reihenfolge für optimales Lernen:

1. Sprach-Tutorials (Learn X in Y Minutes)
   → Syntax, Keywords, Grundkonzepte

2. Dokumentation (DevDocs, MDN, cheat.sh)
   → API-Referenzen, Funktionssignaturen

3. Algorithmen (Rosetta Code, LeetCode)
   → Logik, Datenstrukturen, Patterns

4. Best Practices (PEPs, Design Patterns)
   → Idiomatischer Code, SOLID
```

### Phase 2: Echter Code (Fine-Tuning Data)

```
1. Top-Repositories (GitHub stars > 1000)
   → Produktionsqualität, guter Stil

2. Paket-Quellcode (PyPI, npm Top 1000)
   → Getesteter, dokumentierter Code

3. Stack Overflow (Top-Antworten)
   → Erklärungen + funktionierender Code
```

### Phase 3: Spezialisierung (LoRA Fine-Tuning)

```
Je nach Zielsprache:
- Python: PyPI + Django/Flask Repos + PEPs
- JavaScript: npm + React/Vue Repos + MDN
- Rust: crates.io + Servo/Tokio Repos
- Go: Go stdlib + Kubernetes Repos
```

---

## 📦 Datenmengen-Schätzung

| Kategorie | Geschätzte Größe | Dateien |
|-----------|------------------|---------|
| GitHub Top 10k Repos | ~50 GB | ~10M |
| PyPI Top 1000 | ~5 GB | ~500K |
| npm Top 1000 | ~8 GB | ~1M |
| Stack Overflow Code | ~20 GB | ~30M |
| Dokumentation | ~2 GB | ~100K |
| Tutorials & Patterns | ~500 MB | ~50K |
| **Gesamt (dedupliziert)** | **~60 GB** | **~15M** |

---

## 🔧 Nutzung

### Alle Scraper laden

```python
from respo.ingestion.sources import (
    # Git Platforms
    GitHubScraper, GitLabScraper, BitbucketScraper,
    # Package Registries
    PyPIScraper, NPMScraper, CratesScraper, DebianScraper,
    # Q&A
    StackOverflowScraper, GistsScraper,
    # Documentation
    DevDocsScraper, MDNScraper, CheatShScraper, TLDRScraper,
    # Factory
    get_scraper,
)

# Educational scrapers
from respo.ingestion.sources.educational import (
    LearnXInYMinutesScraper, RosettaCodeScraper,
    LeetCodeScraper, DesignPatternsScraper, PythonPEPsScraper,
)

# Oder via Factory
scraper = get_scraper("stackoverflow")
```

### Beispiel: Komplettes Training-Dataset erstellen

```python
import asyncio
from respo.ingestion.sources import SourceConfig
from respo.ingestion.sources.educational import (
    LearnXInYMinutesScraper,
    RosettaCodeScraper,
)
from respo.ingestion.sources import (
    StackOverflowScraper,
    PyPIScraper,
)

async def build_training_dataset():
    config = SourceConfig(max_files_per_repo=1000)
    
    # 1. Tutorials
    async with LearnXInYMinutesScraper(config) as scraper:
        async for file in scraper.scrape_repository("all"):
            save_to_dataset(file)
    
    # 2. Algorithmen
    async with RosettaCodeScraper(config) as scraper:
        async for file in scraper.scrape_repository("Category:Programming_Tasks"):
            save_to_dataset(file)
    
    # 3. Q&A
    async with StackOverflowScraper(config) as scraper:
        for tag in ["python", "javascript", "java"]:
            async for file in scraper.scrape_repository(tag):
                save_to_dataset(file)
    
    # 4. Packages
    async with PyPIScraper(config) as scraper:
        for pkg in ["requests", "flask", "django", "numpy"]:
            async for file in scraper.scrape_repository(pkg):
                save_to_dataset(file)

asyncio.run(build_training_dataset())
```

---

## ✅ Checkliste: Reicht das für Grundverständnis?

| Aspekt | Abgedeckt? | Quellen |
|--------|------------|---------|
| **Syntax & Keywords** | ✅ | Learn X, DevDocs, cheat.sh |
| **API-Referenzen** | ✅ | DevDocs, MDN, Docs |
| **Algorithmen** | ✅ | Rosetta Code, LeetCode |
| **Datenstrukturen** | ✅ | LeetCode, GitHub |
| **Design Patterns** | ✅ | DesignPatterns, GitHub |
| **Best Practices** | ✅ | PEPs, StackOverflow |
| **Error Handling** | ✅ | StackOverflow, GitHub |
| **Testing** | ⚠️ | GitHub (pytest, jest Repos) |
| **Debugging** | ⚠️ | StackOverflow |
| **Refactoring** | ⚠️ | GitHub PRs (TODO) |

### Noch zu ergänzen (optional):

1. **GitHub PRs mit Reviews** - Zeigt Vorher/Nachher und Feedback
2. **Compiler/Interpreter Fehlermeldungen** - Error → Fix Paare
3. **Jupyter Notebooks** - Interaktive Tutorials mit Erklärungen
4. **Video-Transkripte** - YouTube Coding Tutorials

---

## 🚀 Fazit

**Ja, diese Quellen reichen für ein solides Grundverständnis!**

Die Kombination aus:
- Strukturierten Tutorials (Learn X, Rosetta Code)
- API-Dokumentation (DevDocs, MDN)
- Echtem Code (GitHub, PyPI, npm)
- Q&A mit Erklärungen (Stack Overflow)
- Best Practices (PEPs, Design Patterns)

...deckt alle wesentlichen Aspekte ab, die ein LLM für Programmierverständnis braucht.

Für **CodeLlama-ähnliche Qualität** empfehle ich:
1. ~50GB deduplizierter Code
2. Fokus auf Top-Repositories (Stars > 100)
3. Balance zwischen Tutorials (10%) und echtem Code (90%)
4. Multi-Language Training für Transfer-Learning
