[docs](./index.md) > [CONTENT_MODEL](CONTENT_MODEL.md)
**Datum:** 2026-03-11
**Status:** stable
**Primary (Quelle der Wahrheit):**
- `src/README.md`
- `include/README.md`
- `examples/README.md`
- `docs/_standards/DOC_TEMPLATE.md`
- `docs/_standards/doc_header.schema.yml`

**Bezug / Reference:**
- Issue: [META] Dokumentationssystem: Primary → Secondary → Compendium
- Kontext: Dieses Dokument beschreibt das dreistufige Dokumentationsmodell von ThemisDB.

---

## TL;DR

ThemisDB verwendet ein **dreistufiges Dokumentationsmodell**: Primary Docs (Quellcode-nah in `src/`, `include/`, `examples/`) → Secondary Docs (mehrsprachig, nutzerorientiert in `docs/de/`, `docs/en/`) → Compendium (kurierte Gesamtschau in `compendium/`). Primäre Docs sind die einzige „Source of Truth". Drift zwischen den Stufen ist gewollt sichtbar.

---

## Kontext

- **Problem:** Dokumentation war über viele Verzeichnisse verstreut ohne klares Modell, wer welchen Inhalt „besitzt" und wie Änderungen fließen.
- **Ziel:** Ein reproduzierbares, wartbares Dokumentationssystem mit klaren Verantwortlichkeiten und automatisierter Drift-Erkennung.
- **Nicht-Ziele:** Vollständige Neuschreibung aller Inhalte; große API/Feature-Änderungen.

---

## Das Dreistufige Inhaltsmodell

```
┌─────────────────────────────────────────────────────────────────┐
│  PRIMÄR (Source of Truth)                                       │
│  src/**  ·  include/**  ·  examples/**                          │
│  README.md · ARCHITECTURE.md · ROADMAP.md · CHANGELOG.md …     │
│  → Implementierungsnah, darf "driften" um Stale-Docs zu zeigen  │
└────────────────────────┬────────────────────────────────────────┘
                         │  ableiten / überführen
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│  SEKUNDÄR (Nutzerorientierte Erklärung)                         │
│  docs/de/**  ·  docs/en/**  ·  docs/fr/**  …                   │
│  Jede Datei hat: Breadcrumb · Datum · Status · Primary-Quelle   │
│  Status: draft → review → stable | drifting | stale | archived  │
└────────────────────────┬────────────────────────────────────────┘
                         │  kuratieren / zusammenstellen
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│  COMPENDIUM (Whitebook / PDF)                                   │
│  compendium/docs/**                                              │
│  Kapitel · Anhänge · Glossar                                    │
│  → Abgeleitet aus Primär + Sekundär + examples/**               │
└─────────────────────────────────────────────────────────────────┘
```

### Stufe 1 — Primary Docs (Source of Truth)

**Speicherort:** `src/<modul>/`, `include/<modul>/`, `examples/<modul>/`

**Dateitypen:**
| Dateiname | Zweck |
|-----------|-------|
| `README.md` | Modul-Übersicht, API-Einstieg |
| `ARCHITECTURE.md` | Interne Designentscheidungen |
| `ROADMAP.md` | Geplante Features, Phasen |
| `CHANGELOG.md` | Versionshistorie |
| `FUTURE_ENHANCEMENTS.md` | Langfristige Ideen |

**Konventionen:**
- Jede Datei repräsentiert implementierungsnahe Wahrheit.
- Bewusstes „Driften" ist erlaubt — veraltete Primary Docs zeigen an, was überarbeitet werden muss.
- Ein automatisiertes Inventar wird generiert: `docs/_generated/primary_index.json` (via `tools/primary_docs_indexer.py`).

**Inventar aktualisieren:**
```bash
python3 tools/primary_docs_indexer.py --repo-root . --output docs/_generated/primary_index.json
```

### Stufe 2 — Secondary Docs (docs/*)

**Speicherort:** `docs/de/<modul>/`, `docs/en/<modul>/`, …

**Sprachen:**
| Verzeichnis | Status |
|-------------|--------|
| `docs/de/` | 🇩🇪 **Primärsprache (autoritativ)** |
| `docs/en/` | 🇬🇧 Vollständige Übersetzung |
| `docs/fr/` | 🇫🇷 In Entwicklung |
| `docs/es/` | 🇪🇸 In Entwicklung |

**Pflicht-Header** (jede Datei in `docs/de/` und `docs/en/`):

```markdown
[docs](../../index.md) > [de](../index.md) > [<modul>](./index.md) > [<doc_kind>](./<doc_kind>.md)
**Datum:** YYYY-MM-DD
**Status:** draft | review | stable | deprecated | archived
**Primary (Quelle der Wahrheit):**
- `src/<modul>/README.md`

**Bezug / Reference:**
- Issue/PR: #<id>
- Kontext: <1 Satz warum diese Doku existiert>
```

**`doc_kind` Werte:**
- `architecture` — Designentscheidungen, interne Architektur
- `feature` — Feature-Beschreibungen für Endnutzer
- `howto` — Schritt-für-Schritt-Anleitungen
- `reference` — API-Referenz, Parameter-Listen
- `roadmap` — Geplante Entwicklung
- `troubleshooting` — Fehlerdiagnose
- `faq` — Häufige Fragen

**Status-Bedeutungen:**

| Status | Bedeutung |
|--------|-----------|
| `draft` | Entwurf, noch nicht überprüft |
| `review` | In Überprüfung |
| `stable` | Geprüft, aktuell |
| `deprecated` | Veraltet, wird bald entfernt |
| `archived` | Archiviert, nur noch historisch relevant |

Zusätzlich wird vom Drift-Detektor automatisch erkannt:

| Status | Drift-Erkennung |
|--------|-----------------|
| `drifting` | Secondary-Doc ist älter als 90 Tage relativ zur Primary-Quelle |
| `stale` | Secondary-Doc ist älter als 180 Tage relativ zur Primary-Quelle |

### Stufe 3 — Compendium (Whitebook)

**Speicherort:** `compendium/docs/`

**Struktur:**
```
compendium/docs/
├── cover.md                    # Titelseite
├── preface.md                  # Vorwort
├── chapter_00_genesis.md       # Ursprung und Vision
├── chapter_01_introduction.md  # Einführung
│   ...
├── chapter_42_*.md             # Spezialisierte Themen
├── appendix_d_feature_status.md
├── appendix_e_incident_runbooks.md
├── appendix_f_aql_cheatsheet.md
├── appendix_g_configuration.md
├── appendix_h_glossary.md
└── appendix_i_troubleshooting.md
```

**Build:**
```bash
# MkDocs-basierter Compendium-Build
mkdocs build --config-file compendium/mkdocs-compendium.yml

# Oder mit dem Skript (inkl. PDF-Optimierung):
bash scripts/build_compendium_pdf.sh
```

---

## Modul-Pipeline: Primary → Secondary → Compendium

### Schritt 1 — Primary Doc schreiben/aktualisieren

Datei: `src/<modul>/README.md` (oder `ARCHITECTURE.md`, `ROADMAP.md`, …)

```markdown
# Modulname

Kurze Beschreibung des Moduls.

## Features
- Feature A
- Feature B

## API
...
```

### Schritt 2 — Inventar aktualisieren

```bash
python3 tools/primary_docs_indexer.py
```

Ergebnis: `docs/_generated/primary_index.json` enthält den neuen Eintrag.

### Schritt 3 — Secondary Doc erstellen

Datei: `docs/de/<modul>/architecture.md`

```markdown
[docs](../../index.md) > [de](../index.md) > [<modul>](./index.md) > [architecture](./architecture.md)
**Datum:** 2026-03-11
**Status:** draft
**Primary (Quelle der Wahrheit):**
- `src/<modul>/ARCHITECTURE.md`

**Bezug / Reference:**
- Issue: #<id>
- Kontext: Architektur-Dokumentation für <modul>

---

## TL;DR
...
```

### Schritt 4 — Header-Validierung lokal prüfen

```bash
python3 scripts/doc-header-check.py --mode all
```

### Schritt 5 — Drift prüfen

```bash
python3 scripts/drift-detector.py
```

### Schritt 6 — Compendium-Kapitel aktualisieren

Falls relevant: `compendium/docs/chapter_<n>_<modul>.md` anpassen oder neu erstellen.

### Schritt 7 — CI

Der Pull Request durchläuft automatisch:
1. `docs-lint` — Markdown-Linting
2. `link-check` — Interne Links
3. `toc-validation` — Navigationsstruktur
4. `metadata-check` — YAML-Metadaten
5. `doc-header-check` — Header-Schema-Validierung
6. `drift-detection` — Vergleich Primary vs. Secondary

---

## CI-Validierungs-Pipeline

```
PR/Push → docs-pipeline.yml                        ← einziger Einstiegspunkt
          ├── validate (ruft documentation-validation.yml als reusable workflow)
          │     ├── docs-lint
          │     ├── link-check (intern)
          │     ├── external-link-check (main/develop only)
          │     ├── toc-validation
          │     ├── metadata-check
          │     ├── doc-header-check (changed-only)
          │     ├── drift-detection
          │     │     └── [develop only] Issues für driftende/stale Sekundärdoku
          │     └── validation-summary
          ├── build-site (MkDocs ohne PDF)
          ├── build-pdf (on-demand / Release-Tags, ENABLE_PDF_EXPORT=1)
          └── deploy-pages (main branch)

Push to main/develop/release/* → primary-docs-index.yml
          └── Update docs/_generated/primary_index.json

Push to develop (src/**/*.md | include/**/*.md) → module-docs-sync.yml
          ├── module_docs_builder  → docs/de/<modul>/PRIMARY_SOURCES.md
          │                          docs/en/<modul>/PRIMARY_SOURCES.md
          │                          /tmp/module-findings.json
          ├── changelog_updater   → CHANGELOG.md [Unreleased] fortschreiben
          ├── git commit + push   → "docs: sync module docs [skip ci]"
          └── module_docs_issue_reporter → Issues für neue/underdokumentierte Module
```

**Skripte:**

| Skript | Zweck |
|--------|-------|
| `scripts/docs-lint.py` | Markdown-Syntax, Heading-Hierarchie, Metadaten |
| `scripts/link-check.py` | Interne/externe Links |
| `scripts/toc-check.py` | Navigationsstruktur vs. mkdocs.yml |
| `scripts/doc-header-check.py` | Header-Schema-Validierung |
| `scripts/drift-detector.py` | Drift Primary → Secondary |
| `tools/primary_docs_indexer.py` | Primary-Doc-Inventar generieren |
| `tools/module_docs_builder.py` | Modulweise PRIMARY_SOURCES.md generieren (DE + EN) |
| `tools/ci/changelog_updater.py` | CHANGELOG.md [Unreleased] automatisch fortschreiben |
| `tools/ci/module_docs_issue_reporter.py` | GitHub Issues für neue/underdokumentierte Module und Drift |

---

## Drift-Erkennung

Drift ist **gewollt** — er zeigt an, wenn Secondary Docs hinter ihren Primary-Quellen zurückbleiben.

**Workflow:**
1. `tools/primary_docs_indexer.py` speichert `last_modified` jeder Primary-Datei.
2. `scripts/drift-detector.py` vergleicht die Timestamps von Primary- und Secondary-Docs.
3. Docs mit Alter > 90 Tage relativ zur Primary-Quelle werden als `drifting` markiert.
4. Docs mit Alter > 180 Tage relativ zur Primary-Quelle werden als `stale` markiert.

**Drift-Report lokal ausführen:**
```bash
python3 scripts/drift-detector.py --format text
```

**Secondary Doc manuell als drifting kennzeichnen:**
```markdown
**Status:** drifting
```

---

## Lokaler Build (Reproduzierbar)

### Prerequisites

```bash
pip install -r requirements-docs.txt
```

### MkDocs Site bauen

```bash
bash scripts/build-docs.sh
# → Output: ./site/
```

### Mit PDF

```bash
ENABLE_PDF_EXPORT=1 bash scripts/build-docs.sh
# → Output: ./artifacts/docs/ThemisDB-Documentation-*.pdf
```

### Compendium bauen

```bash
bash scripts/build_compendium_pdf.sh
```

### Alle Validierungen lokal

```bash
bash scripts/validate-docs.sh
```

---

## Konventionen und Qualitätssicherung

### Dateinamen-Konventionen

- Lowercase, Bindestriche statt Leerzeichen: `feature-overview.md`
- Keine Sonderzeichen außer `-` und `_`
- Englische Dateinamen auch in deutschen Docs (Pfade sind sprachunabhängig)

### Markdown-Stil

Konfiguriert in `.markdownlint.json`:
- ATX-Style Headings (`#`, `##`, …)
- 2-Space Einrückung für Listen
- Kein Überspringen von Heading-Ebenen

### Schema-Validierung

Das Header-Schema ist definiert in `docs/_standards/doc_header.schema.yml`. Es wird von `scripts/doc-header-check.py` validiert.

Eine Vorlage ist in `docs/_standards/DOC_TEMPLATE.md` zu finden.

---

## Links

- [Sprachstruktur](../ARCHIVED/implementation-summaries/LANGUAGE_STRUCTURE.md)
- [Wiki- und Archiv-Strategie (Phase 5)](archive/2026-04/WIKI_ARCHIVE_STRATEGY.md)
- [DOC_TEMPLATE](`./_standards/DOC_TEMPLATE.md`)
- [doc_header.schema.yml](./_standards/doc_header.schema.yml)
- [Primary Index](./_generated/primary_index.json)
- [MkDocs Konfiguration](../mkdocs.yml)
- [Compendium Build](../scripts/build_compendium_pdf.sh)
