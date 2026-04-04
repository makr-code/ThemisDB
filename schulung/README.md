# ThemisDB Schulungsunterlagen

Willkommen beim ThemisDB-Schulungscurriculum. Dieses Verzeichnis enthält alle Materialien für eine strukturierte Einführung in ThemisDB — die hochperformante Multi-Model-Datenbank mit nativer KI-Integration.

## 📂 Verzeichnisstruktur

```
schulung/
├── README.md                        ← Diese Datei
├── praesentation/                   ← Marp-kompatible Foliensätze
│   ├── 01_einfuehrung_und_uebersicht.md
│   ├── 02_datenmodelle_und_architektur.md
│   ├── 03_aql_abfragesprache.md
│   ├── 04_installation_und_setup.md
│   └── 05_anwendungsbeispiele.md
├── dokumente/                       ← Schulungsdokumente & Arbeitsblätter
│   ├── 01_quickstart_guide.md
│   ├── 02_aql_referenz_kurzuebersicht.md
│   ├── 03_datenmodellierung_guide.md
│   ├── 04_uebungsaufgaben.md
│   └── 05_best_practices_guide.md
└── examples/                        ← Praxisnahe Code-Beispiele
    ├── README.md
    ├── 01_grundlegende_operationen/
    ├── 02_aql_queries/
    ├── 03_graph_daten/
    └── 04_multimodell_anwendung/
```

## 🎯 Lernziele

Nach Abschluss dieser Schulung können Teilnehmer:

- Die Architektur und Kernkonzepte von ThemisDB erläutern
- ThemisDB installieren und konfigurieren
- Datenmodelle für verschiedene Anwendungsfälle entwerfen
- AQL-Abfragen für relationale, Graph-, Vektor- und Dokumentenoperationen schreiben
- ThemisDB in eigene Anwendungen integrieren
- Best Practices für Performance, Sicherheit und Betrieb anwenden

## 📋 Curriculum-Übersicht

### Modul 1 — Einführung & Überblick (2 h)
- Was ist ThemisDB?
- Multi-Model-Konzept
- Vergleich mit anderen Datenbanken
- Anwendungsfälle und Einsatzgebiete

### Modul 2 — Datenmodelle & Architektur (3 h)
- Relationales Modell
- Dokumentenmodell
- Graphmodell
- Vektormodell
- ACID-Transaktionen und MVCC
- Architekturprinzipien

### Modul 3 — AQL Abfragesprache (4 h)
- Grundlegende Syntax
- CRUD-Operationen
- Joins und Aggregationen
- Graph-Traversierungen
- Vektor- und Volltextsuche
- LLM-Erweiterungen

### Modul 4 — Installation & Setup (2 h)
- Docker-Deployment
- Konfiguration
- Verbindung und Client-Setup
- Monitoring

### Modul 5 — Anwendungsbeispiele & Best Practices (3 h)
- Hands-on Beispiele
- Performance-Optimierung
- Sicherheit
- Produktionsbetrieb

## 🛠️ Voraussetzungen

- Grundkenntnisse in SQL oder einer anderen Abfragesprache
- Grundkenntnisse in Python (für Code-Beispiele)
- Docker installiert und lauffähig
- Internetzugang für den Download von Images

## 📖 Empfohlene Reihenfolge

1. Lesen Sie zunächst `dokumente/01_quickstart_guide.md`
2. Sehen Sie sich die Präsentationen in numerischer Reihenfolge an
3. Arbeiten Sie die `examples/` sequenziell durch
4. Nutzen Sie `dokumente/02_aql_referenz_kurzuebersicht.md` als Nachschlagewerk
5. Lösen Sie die Übungsaufgaben in `dokumente/04_uebungsaufgaben.md`

## 🔗 Weiterführende Ressourcen

- [Offizielle Dokumentation](../docs/)
- [AQL Grammatik](../aql/README.md)
- [Vollständige Beispiele](../examples/)
- [API Referenz](../src/aql/README.md)
- [CHANGELOG](../CHANGELOG.md)
