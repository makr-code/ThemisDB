# ThemisDB Examples

Diese Sammlung enthält umfassende Beispiele für ThemisDB mit Python und Tkinter-Visualisierungen, die die Fähigkeiten der Datenbank an realen Aufgabenstellungen demonstrieren.

## 📚 Übersicht

Wir bieten **10 vollständig dokumentierte Beispiele** in drei Schwierigkeitsstufen:

### 🟢 Einfach (Simple) - Erste Schritte

| # | Beispiel | Beschreibung | Dauer | Status |
|---|----------|--------------|-------|--------|
| 01 | **Hello World** | Erste Schritte mit ThemisDB - CRUD-Operationen | 5-10 min | 📝 Planned |
| 02 | **Todo-App** | Aufgabenverwaltung mit Status und Filterung | 15-20 min | 📝 Planned |
| 03 | **Kontaktmanager** | Adressbuch mit Volltext-Suche | 15-20 min | 📝 Planned |

### 🟡 Mittel (Medium) - Reale Anwendungsfälle

| # | Beispiel | Beschreibung | Dauer | Status |
|---|----------|--------------|-------|--------|
| 04 | **Inventarsystem** | Lagerverwaltung mit Graph-Beziehungen | 30-40 min | 📝 Planned |
| 05 | **Zeitreihen-Monitor** | Echtzeitdaten-Visualisierung mit Charts | 30-40 min | 📝 Planned |
| 06 | **Soziales Netzwerk** | Graph-Visualisierung und Community-Erkennung | 40-50 min | 📝 Planned |
| 07 | **Dokumenten-Suche** | Vector Search & RAG mit Embeddings | 40-50 min | 📝 Planned |

### 🔴 Komplex (Complex) - Enterprise & AI

| # | Beispiel | Beschreibung | Dauer | Status |
|---|----------|--------------|-------|--------|
| 08 | **DMS/ERP-System** | Dokumentenmanagement mit Workflows | 60-90 min | 📝 Planned |
| 09 | **IoT-Sensornetzwerk** | Echtzeit-Datenverarbeitung mit CEP | 60-90 min | 📝 Planned |
| 10 | **Drohnenbild-Analyse** | KI-gestützte Bildanalyse mit LLM | 90-120 min | 📝 Planned |

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

## 🎯 Lernpfade

### Für Einsteiger
1. Start mit **01 - Hello World** für Grundlagen
2. Dann **02 - Todo-App** für praktische CRUD-Operationen
3. Weiter mit **03 - Kontaktmanager** für Suche und Queries

### Für Fortgeschrittene
4. **04 - Inventarsystem** für Multi-Model (Relational + Graph)
5. **05 - Zeitreihen-Monitor** für Time-Series Features
6. **06 - Soziales Netzwerk** für Graph-Algorithmen
7. **07 - Dokumenten-Suche** für Vector Search und RAG

### Für Experten
8. **08 - DMS/ERP-System** für komplexe Workflows
9. **09 - IoT-Sensornetzwerk** für Echtzeit-Verarbeitung
10. **10 - Drohnenbild-Analyse** für KI-Integration

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
