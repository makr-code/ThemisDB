# ThemisDB Kompendium

**Das vollständige Handbuch für ThemisDB v1.3.5**

---

## Überblick

Dies ist das **Kompendium** - eine narrative, buchähnliche Dokumentation für ThemisDB, die über 700 Einzeldokumente und 21 Example-Projekte in eine kohärente, didaktische Struktur integriert.

### Im Gegensatz zur Referenzdokumentation

- **Referenz (../de/):** 729 Einzeldokumente, durchsuchbar, API-fokussiert
- **Kompendium (hier):** 30 Kapitel, narrativ, mit vollständigen Examples

---

## Struktur

```
docs/compendium/
├── README.md                          # Diese Datei
├── STRATEGY_WITH_EXAMPLES.md          # Kompendium-Strategie
├── mkdocs-compendium.yml              # MkDocs-Konfiguration
├── index.md                           # Startseite
├── preface.md                         # Vorwort
├── chapter_01_introduction.md         # ✅ Kapitel 1 (fertig)
├── chapter_02_architecture.md         # 🚧 Kapitel 2 (geplant)
├── chapter_03_multimodel.md           # 🚧 Kapitel 3 (geplant)
├── ...                                # Weitere Kapitel
└── appendix_*.md                      # Anhänge
```

---

## Status

### ✅ Fertiggestellt (Teil I - Grundlagen KOMPLETT)

- **Strategie-Dokument:** Vollständiger Plan (881 Zeilen)
- **MkDocs-Konfiguration:** Build-System bereit
- **Index & Vorwort:** Einführung und Navigation
- **Kapitel 1:** Einführung (504 Zeilen, ~7.200 Wörter)
- **Kapitel 2:** Architektur (723 Zeilen, ~8.500 Wörter)
- **Kapitel 3:** Multi-Model (760 Zeilen, ~7.500 Wörter)
- **Kapitel 4:** Installation & Setup (753 Zeilen, ~6.500 Wörter)
- **Teil I (Grundlagen):** ✅ **KOMPLETT** (4/4 Kapitel, 2.740 Zeilen, ~29.700 Wörter)

### 🚧 In Arbeit

- Teil II: Kapitel 5-8 (Datenmodelle)
- Teile III-VIII: Kapitel 9-30
- Anhänge A-D

### 📅 Zeitplan

- **Pilot (Woche 1):** Kapitel 1, 6 fertigstellen
- **Bulk Content (Wochen 2-7):** Teile I-III
- **Polishing (Wochen 8-9):** Review, Diagramme
- **Release (Woche 10):** Finales PDF

---

## Build-Anleitung

### Voraussetzungen

```bash
pip install -r ../../requirements-docs.txt
```

### HTML Build

```bash
cd docs/compendium
mkdocs build -f mkdocs-compendium.yml
```

Output: `site/` Verzeichnis

### PDF Build

```bash
export ENABLE_PDF_EXPORT=1
mkdocs build -f mkdocs-compendium.yml
```

Output: `../ThemisDB-Kompendium-v1.3.5.pdf`

### Development Server

```bash
mkdocs serve -f mkdocs-compendium.yml
```

Öffne: http://localhost:8000

---

## Struktur-Übersicht

### Teil I: Grundlagen (100 Seiten)
- Kapitel 1: Einführung ✅
- Kapitel 2: Architektur 🚧
- Kapitel 3: Multi-Model 🚧
- Kapitel 4: Installation 🚧

### Teil II: Datenmodelle (140 Seiten)
- Kapitel 5: Relational 🚧
- Kapitel 6: Graph 🚧
- Kapitel 7: Dokumente 🚧
- Kapitel 8: Vektoren 🚧

### Teil III-VIII: ... (siehe STRATEGY_WITH_EXAMPLES.md)

---

## Examples-Integration

Alle 21 Examples sind auf Kapitel verteilt:

- **01_hello_world** → Kapitel 1 ✅
- **02_todo_app** → Kapitel 2
- **06_graph_social_network** → Kapitel 6
- **09_iot_sensor_network** → Kapitel 9
- ... (siehe Strategie für vollständiges Mapping)

---

## Mitarbeit

### Kapitel schreiben

1. Template verwenden (siehe chapter_01_introduction.md)
2. Struktur beibehalten:
   - Überblick
   - Theorie
   - Praxis (mit Example)
   - Patterns
   - Zusammenfassung
3. 20-40 Seiten pro Kapitel
4. Code-Beispiele testen

### Review-Prozess

1. Draft erstellen
2. Technische Richtigkeit prüfen
3. Beispiele testen
4. Schreibstil polieren
5. PR erstellen

---

## Stil-Guide

### Schreibstil

- **Aktiv statt Passiv:** "ThemisDB speichert..." statt "wird gespeichert"
- **Du-Form:** Direkte Ansprache des Lesers
- **Beispiele:** Konzepte mit realen Szenarien erklären
- **Präzise:** Technisch korrekt, aber verständlich

### Code-Beispiele

- **Vollständig:** Keine Pseudo-Code-Fragmente
- **Getestet:** Alle Beispiele müssen laufen
- **Kommentiert:** Wichtige Teile erklären
- **Realistisch:** Echte Use Cases

### Diagramme

- **Mermaid:** Für Architektur und Flows
- **ASCII:** Für einfache Strukturen
- **Screenshots:** Für UI (falls relevant)

---

## Ressourcen

- **Strategie:** [STRATEGY_WITH_EXAMPLES.md](STRATEGY_WITH_EXAMPLES.md)
- **Examples:** [../../examples/](../../examples/)
- **Referenz-Docs:** [../de/](../de/)
- **API Specs:** [../openapi.yaml](../openapi.yaml)

---

## Kontakt

- **Issues:** https://github.com/makr-code/ThemisDB/issues
- **Discussions:** https://github.com/makr-code/ThemisDB/discussions
- **Email:** (für Kompendium-spezifische Fragen)

---

**Version:** 1.3.5  
**Status:** Pilot Phase  
**Letzte Aktualisierung:** 28. Dezember 2025
