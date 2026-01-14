# ThemisDB Examples

> **📚 New to ThemisDB?** Check out our comprehensive guides:
> - **[Examples Quickstart Guide](../docs/EXAMPLES_QUICKSTART.md)** - Get started in 10 minutes
> - **[Examples Index](../docs/EXAMPLES_INDEX.md)** - Complete catalog of all examples
> - **[Learning Paths](../docs/EXAMPLES_INDEX.md#-learning-paths)** - Guided learning for your role

Diese Sammlung enthält umfassende Beispiele für ThemisDB mit Python und Tkinter-Visualisierungen, die die Fähigkeiten der Datenbank an realen Aufgabenstellungen demonstrieren.

## 📚 Übersicht

Wir bieten **21 vollständig dokumentierte Beispiele** in drei Schwierigkeitsstufen:

### 🟢 Einfach (Simple) - Erste Schritte

| # | Beispiel | Beschreibung | Dauer | Status |
|---|----------|--------------|-------|--------|
| 01 | **Hello World** | Erste Schritte mit ThemisDB - CRUD-Operationen | 5-10 min | ✅ Implemented |
| 02 | **Todo-App** | Aufgabenverwaltung mit Status und Filterung | 15-20 min | 📝 Documented |
| 03 | **Kontaktmanager** | Adressbuch mit Volltext-Suche | 15-20 min | 📝 Documented |
| 11 | **Blog/Wiki-System** | Content-Management mit Markdown | 30-40 min | ✅ Ready |
| 12 | **Expense Tracker** | Haushaltsbuch mit Budget-Management | 30-40 min | ✅ Ready |
| 13 | **Recipe Manager** | Rezeptverwaltung mit Einkaufsliste | 30-40 min | ✅ Ready |

### 🟡 Mittel (Medium) - Reale Anwendungsfälle

| # | Beispiel | Beschreibung | Dauer | Status |
|---|----------|--------------|-------|--------|
| 04 | **Inventarsystem** | Lagerverwaltung mit Graph-Beziehungen | 30-40 min | 📝 Documented |
| 05 | **Zeitreihen-Monitor** | Echtzeitdaten-Visualisierung mit Charts | 30-40 min | 📝 Documented |
| 06 | **Soziales Netzwerk** | Graph-Visualisierung und Community-Erkennung | 40-50 min | 📝 Documented |
| 07 | **Dokumenten-Suche** | Vector Search & RAG mit Embeddings | 40-50 min | 📝 Documented |
| 14 | **E-Commerce Katalog** | Multi-Model Produktkatalog mit Empfehlungen | 60 min | ✅ Ready |
| 15 | **Event Management** | Veranstaltungsmanagement mit Ticketing | 60 min | ✅ Ready |
| 16 | **Kanban Board** | Agile Projektmanagement mit Sprints | 60 min | ✅ Ready |
| 17 | **CRM** | Customer Relationship Management | 60-90 min | ✅ Ready |

### 🔴 Komplex (Complex) - Enterprise & AI

| # | Beispiel | Beschreibung | Dauer | Status |
|---|----------|--------------|-------|--------|
| 08 | **DMS/ERP-System** | Dokumentenmanagement mit Workflows | 60-90 min | 📝 Documented |
| 09 | **IoT-Sensornetzwerk** | Echtzeit-Datenverarbeitung mit CEP | 60-90 min | 📝 Documented |
| 10 | **Drohnenbild-Analyse** | KI-gestützte Bildanalyse mit LLM | 90-120 min | 📝 Documented |
| 18 | **Real-Time Chat** | Echtzeit-Kommunikation mit Pub/Sub | 90-120 min | ✅ Ready |
| 19 | **Recommendation Engine** | ML-basierte Empfehlungen | 90-120 min | ✅ Ready |
| 20 | **Smart Home Dashboard** | IoT Automation mit CEP | 90-120 min | ✅ Ready |
| 21 | **Coding Platform** | ThemisDB als Coding-Plattform mit VSCode Integration & Web Scraping | 90-120 min | ✅ Ready |
| 23 | **Traveling Salesman Problem** | Routenoptimierung mit Graph-Algorithmen | 40-50 min | ✅ Ready |

## 🚀 Schnellstart

### Voraussetzungen

```bash
# Python 3.8 oder höher
python --version

# ThemisDB Server (Docker empfohlen)
docker run -d -p 8080:8080 -p 18765:18765 themisdb/themisdb:latest

# Python-Client installieren
pip install themisdb-client
```

### Beispiel starten

```bash
# Navigiere zu einem Beispiel
cd examples/01_hello_world

# Installiere Abhängigkeiten
pip install -r requirements.txt

# Starte die Anwendung
python main.py
```

## 📖 Dokumentation

Jedes Beispiel enthält vollständige Dokumentation:

- **README.md** - Übersicht, Features, Installation, Schnellstart
- **HOW_TO.md** - Schritt-für-Schritt-Anleitung zur Bedienung
- **Zusätzliche Guides** - Je nach Komplexität (ARCHITECTURE.md, DATA_MODEL.md, etc.)

**Für LLM-Integration:**
- [🧠 LLM Complete Setup Guide](../docs/de/guides/LLM_COMPLETE_SETUP_GUIDE.md) - Vollständiger Guide für LLM-Setup und Inferencing mit ThemisDB

## 🎯 Lernpfade

### Für Einsteiger
1. Start mit **01 - Hello World** für Grundlagen
2. Dann **02 - Todo-App** für praktische CRUD-Operationen
3. Weiter mit **03 - Kontaktmanager** für Suche und Queries
4. **11 - Blog/Wiki-System** für Content-Management
5. **12 - Expense Tracker** für finanzielle Datenmodellierung
6. **13 - Recipe Manager** für strukturierte Daten

### Für Fortgeschrittene
7. **04 - Inventarsystem** für Multi-Model (Relational + Graph)
8. **05 - Zeitreihen-Monitor** für Time-Series Features
9. **06 - Soziales Netzwerk** für Graph-Algorithmen
10. **07 - Dokumenten-Suche** für Vector Search und RAG
11. **14 - E-Commerce Katalog** für Multi-Model Showcase
12. **15 - Event Management** für Veranstaltungslogistik
13. **16 - Kanban Board** für Agile Workflows
14. **17 - CRM** für Business-Anwendungen

### Für Experten
15. **08 - DMS/ERP-System** für komplexe Workflows
16. **09 - IoT-Sensornetzwerk** für Echtzeit-Verarbeitung
17. **10 - Drohnenbild-Analyse** für KI-Integration
18. **18 - Real-Time Chat** für Echtzeit-Kommunikation
19. **19 - Recommendation Engine** für ML-Integration
20. **20 - Smart Home Dashboard** für IoT-Automation
21. **21 - Coding Platform** für VSCode-Integration und Web Scraping

## 🛠️ Technologien

Alle Beispiele verwenden:
- **Python 3.8+** - Programmiersprache
- **Tkinter** - GUI-Framework (Standard-Bibliothek)
- **ThemisDB Python Client** - Datenbank-Connector

Erweiterte Beispiele nutzen zusätzlich:
- **matplotlib** - Datenvisualisierung
- **NetworkX** - Graph-Visualisierung
- **OpenCV** - Computer Vision
- **sentence-transformers** - Text-Embeddings
- **scikit-learn** - Machine Learning

## 📋 Detaillierter Roadmap

Siehe [TODO.md](TODO.md) für den vollständigen Implementierungsplan mit:
- Detaillierte Feature-Listen für jedes Beispiel
- Technologie-Stack
- Dokumentations-Anforderungen
- Zeitplan und Fortschritt

## 🏗️ Projekt-Struktur

Einfache Beispiele:
```
01_hello_world/
├── README.md              # Übersicht
├── HOW_TO.md             # Anleitung
├── requirements.txt       # Dependencies
├── main.py               # Hauptanwendung
├── themis_client.py      # DB-Logik
└── screenshots/          # UI-Screenshots
```

Komplexe Beispiele:
```
10_drone_image_analysis/
├── README.md
├── HOW_TO.md
├── ARCHITECTURE.md
├── LLM_INTEGRATION.md
├── requirements.txt
├── src/
│   ├── main.py
│   ├── ui/              # UI-Komponenten
│   ├── models/          # Datenmodelle
│   ├── services/        # Business-Logik
│   └── utils/           # Hilfsfunktionen
├── config/
└── sample_data/
```

## 🤝 Beitragen

Möchten Sie ein Beispiel hinzufügen oder verbessern?

1. Folgen Sie der Struktur in [TODO.md](TODO.md)
2. Stellen Sie sicher, dass README.md und HOW_TO.md vorhanden sind
3. Kommentieren Sie Ihren Code ausführlich
4. Fügen Sie Screenshots hinzu
5. Testen Sie das Beispiel vollständig

## 📚 Weitere Ressourcen

- [ThemisDB Hauptdokumentation](../docs/)
- [Python Client SDK](../clients/python/)
- [AQL Query Language](../docs/aql/)
- [API Reference](../docs/api/)

## 🆘 Support

Bei Fragen oder Problemen:
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- [Dokumentation](https://makr-code.github.io/ThemisDB/)

---

**Status**: In Entwicklung | **Letzte Aktualisierung**: 2025-12-22
