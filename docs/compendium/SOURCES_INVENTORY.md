# Repository-Informationsquellen für Kapitel-Generierung
## Vollständiges Verzeichnis lokaler & externer Ressourcen

**Datum:** 13. Januar 2026  
**Status:** Komplett durchsucht und kategorisiert

---

## ⚠️ KRITISCHER HINWEIS: Bestehende Kapitel erweitern!

### Aktuelle Situation:
- **40+ Kapitel** existieren bereits im `./docs/`-Verzeichnis
- **Alle Hauptthemen** sind bereits abgedeckt
- **Keine neuen Kapitel** nötig (Ausnahme: völlig neue Sachverhalte)

### Richtige Vorgehensweise:

```
UMFORMULIERUNGSWORKFLOW:
1. Bestehendes Kapitel öffnen: ./docs/chapter_N.md
2. SOURCES_INVENTORY.md nutzen: Alle Quellen für dieses Thema
3. LLM-Prompt erstellen mit:
   - Bestehendem Text (als Input)
   - Verbesserungsanforderungen
   - Zusätzlichen Quellen (RocksDB, Boost, Benchmarks, etc.)
4. Output: Tiefere, wissenschaftlichere Version des GLEICHEN Kapitels
5. Ergebnis: chapter_N.md wird aktualisiert (nicht gelöscht/neugeschrieben)
```

### Nur NEUE Kapitel wenn:
- ❌ Thema existiert in keinem der 47 bestehenden Kapitel
- ✅ Mit Team abstimmen bevor neue chapter_*.md angelegt wird
- ✅ In mkdocs-nav.yml registrieren

---

## 📂 LOKALE INFORMATIONSQUELLEN

### 🎯 Prim\u00e4re Quellen (Kapitel-Basis)

```
Pfad: .\docs\
Inhalt: 40+ Kapitel (chapter_00.md bis chapter_41.md)
Plus: 7 Anhang-Dateien (appendix_*.md)

Struktur:
- chapter_00_genesis.md (Urspr\u00fcnge, Geschichte)
- chapter_01_introduction.md (Einf\u00fchrung)
- chapter_02_architecture.md (Architektur)
- chapter_03_multimodel.md (Multi-Model-Konzept)
- chapter_04_installation.md (Installation)
- chapter_05_relational.md (Relationales Modell)
- chapter_06_graph.md (Graph-Datenbank)
- chapter_07_document.md (Dokumente)
- chapter_08_vector.md (Vektor-DB)
- chapter_08_storage_layer.md (Storage Layer)
- chapter_09_timeseries.md (Zeit-Reihen)
- chapter_10_enterprise.md (Enterprise-Features)
- ... (weitere Kapitel)
- appendix_d_feature_status.md
- appendix_e_incident_runbooks.md
- appendix_f_aql_cheatsheet.md
- appendix_g_configuration.md
- appendix_h_glossary.md
- appendix_i_troubleshooting.md
- appendix_literatur.md
```

### 📋 Meta-Dokumentation (Implementierungs-Reports)

| Datei | Inhalt | Nutzen f\u00fcr Kapitel-Generierung |
|-------|--------|----------------------------------|
| **MASTER_IMPLEMENTATION_SUMMARY.md** | Gesamt-Build-Strategie, Phasen 1-3 | Verst\u00e4ndnis der Gesamt-Architektur |
| **IMPLEMENTATION_COMPLETE.md** | PDF-Generierungs-Verbesserungen, Buchlayout-Standards | Layout-Anforderungen beachten |
| **STATUS_UPDATE.md** | Aktueller Stand, Validierungsergebnisse | Was wurde bereits umgesetzt |
| **PHASE1_IMPLEMENTATION_REPORT.md** | YAML-Integration Details | Struktur-Basis |
| **PHASE2_IMPLEMENTATION_REPORT.md** | PDF-Enhancement Details | PDF-Richtlinien |
| **PHASE3_IMPLEMENTATION_REPORT.md** | PDF Bookmarks & QA Details | Bookmark-Anforderungen |
| **PHASE3_ROADMAP.md** | Zuk\u00fcnftige Verbesserungen | Geplante Features |

### 🎨 Strategie & Best Practices

| Datei | Inhalt | Nutzen |
|-------|--------|--------|
| **STRATEGY_WITH_EXAMPLES.md** | Struktur-Vorbilder aus Tech-B\u00fcchern (Kleppmann, Rust-Book, Go-Book) | Kapitel-Aufbau orientieren |
| **BUILD_GAPS_ANALYSIS.md** | Identifizierte L\u00fccken & Verbesserungen | Was fehlt noch |
| **ANCHOR_SYSTEM_DOCUMENTATION.md** | Marker-System f\u00fcr Seitennummern | Technische Seitenreferenzierung |
| **BUILD_REPORT_v1.4.0_ANCHOR_SYSTEM.md** | Anchor-Umsetzung im Build | Wie Marker funktionieren |
| **THEMISDB_CUSTOM_THEME.md** | Theme-Anpassungen | Styling-Richtlinien |

### 📊 PDF & Layout-Dokumentation

| Datei | Fokus |
|-------|-------|
| **PDF_GENERATION_GUIDE_v1.4.0-alpha.md** | Detailliertes PDF-Build-System |
| **PDF_GENERATION_README.md** | PDF-Prozess \u00dcbersicht |
| **PDF_QUICKSTART.md** | Schnelle Referenz |
| **PDF_STRATEGY_ANALYSIS.md** | Vergleich: WeasyPrint vs. wkhtmltopdf |
| **PDF_LAYOUT_IMPROVEMENTS.md** | Layout-Optimierungen (Seitenumbr\u00fcche, Widow/Orphan) |
| **CSS_ANALYSIS.md** | CSS-Optimierungen f\u00fcr PDF-Rendering |
| **LAYOUT_COMPARISON.md** | HTML vs. PDF Layout-Unterschiede |

### \ud83d\udd� Debugging & Validierungs-Guides

| Datei | Zweck |
|-------|-------|
| **DEBUG_YAML_GUIDE.md** | YAML-Parser Debugging |
| **BUILD.md** | Build-Prozess Dokumentation |
| **EXPORT_README.md** | Export & Output-Formatierung |
| **debug_anchors.py** | Script zum Anchor-Debugging |
| **validate_debug_yaml.py** | YAML-Validierungs-Script |

### \u2744\ufe0f Alternative Guides

| Datei | Inhalt |
|-------|--------|
| **PANDOC_LATEX_GUIDE.md** | LaTeX/Pandoc Alternativen |
| **MKDOCS_THEMES.md** | MkDocs Theme-System |
| **ANCHOR_USAGE_GUIDE.md** | Praktische Anchor-Nutzung |

### \u⚙\ufe0f Konfiguration & Styling

```
mkdocs-compendium.yml    - MkDocs Hauptkonfiguration
mkdocs-nav.yml           - Navigations-Struktur (11 Teile, 53 Seiten)
styles_modern_book.scss  - Moderne Buch-Stylesheets
styles_modern_book_final.scss - Finalisierte Styles
styles_minimal.scss      - Minimalistische Styles
styles_modern_book.css   - Kompilierte CSS-Version
styles_pdf_optimization.scss - PDF-spezifische Optimierungen
```

### \ud83d\udcbe Build-System & Scripts

```
step1_generate_svgs.py   - Mermaid-Diagramme \u2192 SVG
step2_generate_html.py   - Markdown \u2192 HTML mit Marker-Injection
step3_generate_pdf.py    - HTML \u2192 PDF (WeasyPrint/wkhtmltopdf)
step4_add_bookmarks.py   - PDF Bookmarks + TOC Rebuild + Figure Index
step5_cleanup.py         - Cleanup Intermediate Files
generate_test_pdf.py     - Test-PDF Generierung
fix_markdown_figures.py  - Markdown-Figuren-Fix
```

### \ud83d\udccb Weitere Dokumentationen

- **README.md** - Projekt\u00fcbersicht, Quick Start
- **VERSION** - Versions-Info
- **requirements.txt** - Python Dependencies
- **package.json** - Node.js Dependencies
- **TODO_V1.4.0_ALPHA_UPDATES.md** - Ausstehende Aufgaben
- **V1.4.0_ALPHA_UPDATE_NOTES.md** - Update-Details

---

## \ud83c\udfa8 EXTERNE LIBRARY-DOKUMENTATION

### Storage & Performance

**RocksDB** (Key-Value Storage)
- Wiki: https://github.com/facebook/rocksdb/wiki
- Tuning Guide: https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide
- API: https://rocksdb.org/docs/
- Relevanz: Storage Layer Implementierung

**LevelDB** (Alternativer KV-Store)
- Dokumentation: https://github.com/google/leveldb
- Relevanz: Vergleich mit RocksDB

### C++ Grundlagen

**Boost C++ Libraries**
- Dokumentation: https://www.boost.org/doc/libs/
- Boost.Asio: Network & Event Handling
- Boost.Spirit: Parser & Generators
- Boost.Graph: Graph Algorithms
- Boost.Thread: Threading

**MVCC (Multi-Version Concurrency Control)**
- PostgreSQL MVCC: https://www.postgresql.org/docs/current/mvcc.html
- MySQL InnoDB: https://dev.mysql.com/doc/refman/8.0/en/innodb-architecture.html
- Academic Papers: "Concurrency Control and Recovery in Database Systems"

### JSON & Parsing

**nlohmann/json**
- Dokumentation: https://json.nlohmann.me/
- GitHub: https://github.com/nlohmann/json
- API: https://json.nlohmann.me/api/basic_json/

### Spezial-Technologien

**V8 JavaScript Engine**
- Dokumentation: https://v8.dev/docs
- Relevanz: JavaScript-Integration

**libuv (Event Loop)**
- Dokumentation: https://docs.libuv.org/
- Relevanz: Async I/O

**OpenSSL / Cryptography**
- OpenSSL: https://www.openssl.org/docs/
- Relevanz: Security & Encryption

### Basis-Datenbank

**ArangoDB (Upstream)**
- Dokumentation: https://docs.arangodb.com/stable/
- GitHub: https://github.com/arangodb/arangodb
- Relevanz: ThemisDB basiert darauf

---

## \ud83c\udcd6 AKADEMISCHE QUELLEN

### Datenbank-Theorie
- Papers on arxiv.org: "multi-model database" OR "graph database"
- ACM Digital Library: SIGMOD, VLDB Proceedings
- IEEE Xplore: Database Systems

### Graph-Algorithmen
- "Introduction to Algorithms" (CLRS)
- Papers: BFS, DFS, Shortest Path, Community Detection
- Graph Theory: https://en.wikipedia.org/wiki/Graph_theory

### Verteilte Systeme
- "Designing Data-Intensive Applications" (Kleppmann)
- "The Art of Computer Systems Performance Analysis"
- CAP Theorem, Consensus Algorithms, etc.

### Performance & Optimierung
- Query Optimization Papers
- Index Structures (B-Trees, LSM Trees, Hash Tables)
- Cardinality Estimation

---

## \ud83d\udcc1 STRUKTUR-VORBILDER AUS TECH-B\u00dcCHERN

### Pattern 1: Kleppmann - "Designing Data-Intensive Applications"
\`\`\`
Kapitel-Struktur:
1. Motivierende Szenarien
2. Konzeptionelle Grundlagen (mit Diagrammen)
3. Vergleich mehrerer Ans\u00e4tze
4. Implementierungs-Details (z.B. Algorithmen)
5. Real-world Fallstudien
6. Trade-offs Diskussion
\`\`\`

### Pattern 2: "Programming Rust"
\`\`\`
Kapitel-Struktur:
1. Konzept-Erkl\u00e4rung
2. "Let's code..." - Inkrementelles Beispiel
3. Schrittweise Erweiterung
4. Best Practices
5. Vollst\u00e4ndiges Programm am Ende
\`\`\`

### Pattern 3: "The Go Programming Language"
\`\`\`
Kapitel-Struktur:
1. Konzept + Motivation
2. Vollst\u00e4ndiges, lauff\u00e4higes Programm
3. Detaillierte Erkl\u00e4rung jeder Zeile
4. Variationen & Optimierungen
5. Eigenst\u00e4ndige, selbsterkl\u00e4rende Kapitel
\`\`\`

### ThemisDB Buchlayout Standards (aus IMPLEMENTATION_COMPLETE.md)
```
Seitennummerierung:
- R\u00f6misch (i, ii, iii...) f\u00fcr Verzeichnisse
- Arabisch (1, 2, 3...) f\u00fcr Hauptinhalt

Widow/Orphan Control:
- Minimum 3 Zeilen zusammen auf Seite
- Verhindert isolierte Zeilen oben/unten

Intelligente Seitenumbr\u00fcche:
- \u00dcberschriften: page-break-after: avoid
- Code-Bl\u00f6cke: page-break-inside: avoid
- Tabellen: zusammen auf einer Seite
- Abbildungen: mit Beschriftungen nicht trennen

Verso/Recto Layout:
- Unterschiedliche R\u00e4nder f\u00fcr Links/Rechts
- Orientiert an klassischen Buchdrucken

Typographie:
- Georgia (Serif) f\u00fcr Flie\u00dftext
- Helvetica Neue (Sans) f\u00fcr Headlines
- Consolas/Courier (Monospace) f\u00fcr Code
- Silbentrennung & Blocksatz
```

---

## \ud83d\ude80 VERWENDUNGS-BEISPIEL

### Kapitel "Graph-Datenmodell" verfassen:

**Schritt 1: Quellen sammeln**
```
Lokal:
- .\\docs\\chapter_06_graph.md (Bestehendes Kapitel)
- .\\docs\\chapter_02_architecture.md (Kontext)
- STRATEGY_WITH_EXAMPLES.md (Best Practices)
- IMPLEMENTATION_COMPLETE.md (Layout-Standards)

Extern:
- RocksDB Wiki (Speicher-Implementierung)
- Boost.Graph Docs (Algorithmen)
- Academic Papers (Graph Theory)
```

**Schritt 2: Prompt erstellen**
```
QUELLEN: [Liste aller oben gefundenen Dateien]
STRUKTUR: Kleppmann-Pattern (Konzept → Vergleich → Implementierung)
LAYOUT: Beachte Widow/Orphan, Seitenmary, Marker-System
BEISPIELE: 3+ Code-Beispiele, progressiv komplexer
```

**Schritt 3: LLM-Generierung**
```
Claude mit Quellmaterial + Prompt \u2192 Rohtext
```

**Schritt 4: Validierung**
```
- Syntax-Check: Code-Beispiele testen
- Referenzen: Links & Cross-refs validieren
- Layout: Marker-System ber\u00fccksichtigt?
- Konsistenz: Style Guide befolgt?
```

---

## \ud83d\udcdd CHECKLISTE: ALLE QUELLEN NUTZEN

```
Prim\u00e4rquellen (\\.md Dateien):
\u2610 chapter_*.md (Bestehende Kapitel als Kontext)
\u2610 STRATEGY_WITH_EXAMPLES.md (Struktur-Vorbilder)
\u2610 IMPLEMENTATION_COMPLETE.md (Buchlayout-Standards)
\u2610 MASTER_IMPLEMENTATION_SUMMARY.md (Gesamt-\u00dcbersicht)
\u2610 STATUS_UPDATE.md (Aktueller Stand)

Meta-Dokumentation:
\u2610 PHASE*_IMPLEMENTATION_REPORT.md (Details zu Features)
\u2610 BUILD_GAPS_ANALYSIS.md (L\u00fccken identifizieren)
\u2610 ANCHOR_SYSTEM_DOCUMENTATION.md (Seitennummern)

Externe Libraries:
\u2610 RocksDB Wiki (Falls Storage relevant)
\u2610 Boost Docs (Falls Algorithmen/C++ relevant)
\u2610 nlohmann/json (Falls JSON-Processing)
\u2610 Academic Papers (Theorien & Algorithmen)

Validierung:
\u2610 Quellen-URLs verifiziert
\u2610 Code-Beispiele testen
\u2610 Layout-Standards befolgt
\u2610 Marker-System beachtet
```

---

**Version:** 1.0  
**Status:** Vollst\u00e4ndig durchsucht & dokumentiert  
**Letzte Aktualisierung:** 13. Januar 2026
