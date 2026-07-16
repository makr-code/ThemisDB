# ThemisDB Documentation Toolchain (MkDocs, Preview, Publish)

Stand: 2026-05-13

Diese Seite konsolidiert den aktuellen, verbindlichen Build-/Preview-/Publish-Flow fuer die Dokumentation.

## 1) Steuerdateien und Zustaendigkeit

| Datei | Aufgabe |
|---|---|
| `mkdocs.yml` | Hauptkonfiguration (Theme, Navigation, `with-pdf`, `print-site`) |
| `mkdocs-nopdf.yml` | Build ohne PDF-Plugin (INHERIT + Plugin-Override) |
| `requirements-docs.txt` | Reproduzierbar gepinnte Python-Doku-Abhaengigkeiten |
| `docs/print-banner.html` | Banner fuer die Druck-/Print-Seite |
| `docs/print-cover.html` | Cover fuer die Print/PDF-Ausgabe |
| `docs/_Sidebar.md`, `docs/_Footer.md` | Wiki-Navigation/-Footer |
| `docs/website/**` | Marketing-Website-Quellen (nicht Teil des MkDocs-Builds) |
| `docs/_generated/**` | Generierte Doku-Indizes/Artefakte (z. B. `primary_index.json`) |

## 2) Build- und Preview-Workflow (lokal)

### 2.1 Abhaengigkeiten installieren

```bash
python3 -m pip install -r requirements-docs.txt
```

### 2.2 Preview (ohne PDF, schnell fuer Autoren)

```bash
python3 -m mkdocs serve --config-file mkdocs-nopdf.yml
```

- URL: `http://127.0.0.1:8000/`
- Nutzt `print-site`, aber **kein** `with-pdf`.

### 2.3 Produktions-Build ohne PDF

```bash
python3 -m mkdocs build --config-file mkdocs-nopdf.yml --clean
```

- Output: `site/`

### 2.4 Produktions-Build mit PDF

```bash
ENABLE_PDF_EXPORT=1 python3 -m mkdocs build --clean
```

- Site-Output: `site/`
- PDF-Output: `artifacts/docs/ThemisDB-Documentation-v1.3.5.pdf`

## 3) Publish-Workflow (aktueller Stand)

## 3.1 CI/Workflow-Status

- Der aktive Workflow-Kern ist in `.github/WORKFLOW_REGISTRY.md` dokumentiert.
- Es gibt aktuell **keinen aktiven GitHub-Pages-Deploy-Workflow** fuer MkDocs in `.github/workflows/`.
- Die dokumentierten historischen Pipelines (`docs/ci-cd/workflows/docs/docs-pipeline.md`, `docs/ci-cd/GITHUB_PAGES_SETUP.md`) sind Legacy-Referenz und nicht der aktive Ist-Stand.

## 3.2 Verfuegbare Publikationspfade

1. **Static Site Artefakt**: `site/` nach lokalem/CI-Build bereitstellen.
2. **Wiki-Pfad**:
   - `python3 tools/publish_wiki.py`
   - alternativ `scripts/sync-wiki.ps1`
3. **Print/PDF-Pfad**:
   - `site/print_page/index.html` fuer Browser-Druck
   - optionales PDF in `artifacts/docs/` bei `ENABLE_PDF_EXPORT=1`

## 4) PDF-/Print-Pfade und Unterschiede

| Thema | `mkdocs-nopdf.yml` (Standard) | `mkdocs.yml` + `ENABLE_PDF_EXPORT=1` |
|---|---|---|
| Ziel | schnelle Site-Builds ohne PDF-Plugin | Site + PDF-Artefakt |
| Plugins | `search`, `exclude`, `print-site` | `search`, `with-pdf`, `print-site` |
| `print_page` | Ja (`site/print_page/index.html`) | Ja (`site/print_page/index.html`) |
| PDF-Datei | nein | `artifacts/docs/ThemisDB-Documentation-v1.3.5.pdf` |

## 5) Troubleshooting (haeufige Build-Fehler)

| Symptom | Ursache | Behebung |
|---|---|---|
| `No module named mkdocs` / Plugin fehlt | Abhaengigkeiten nicht installiert/inkonsistent | `python3 -m pip install -r requirements-docs.txt` |
| `Template file docs/print-*.html not found` | Pfad/Datei fuer Print-Templates fehlt | `docs/print-banner.html` und `docs/print-cover.html` pruefen |
| PDF wird nicht erzeugt | `ENABLE_PDF_EXPORT` nicht gesetzt | Build mit `ENABLE_PDF_EXPORT=1` starten |
| Viele Warnungen bei `--strict` | Altlasten in Navigation/Ankern | Warnungen gezielt im betroffenen Bereich bereinigen; Build ohne `--strict` nur fuer lokale Diagnose |
| Build bricht bei WeasyPrint-Systemlibs | Plattformabhaengigkeit von `mkdocs-with-pdf` | fuer Authoring `mkdocs-nopdf.yml` nutzen; PDF in passender Build-Umgebung erzeugen |

## 6) Review-/Audit-Nachweis fuer diese Konsolidierung

**Referenzen (verbindlich):**
- [DOCUMENTATION_REVIEW_GUIDELINES.md](DOCUMENTATION_REVIEW_GUIDELINES.md)
- [SYSTEMATISCHER_REVIEWPLAN.md](SYSTEMATISCHER_REVIEWPLAN.md)
- [PR_DOCUMENTATION_CHECKLIST.md](PR_DOCUMENTATION_CHECKLIST.md)
- [de/development/SOURCE_CODE_AUDIT.md](de/development/SOURCE_CODE_AUDIT.md)
- [audit-framework/AUDIT_RUNBOOK.md](audit-framework/AUDIT_RUNBOOK.md)

**Gepruefte Bereiche in diesem Change:**
- `mkdocs.yml`
- `mkdocs-nopdf.yml`
- `requirements-docs.txt`
- `docs/print-banner.html`
- `docs/print-cover.html`
- `docs/README-DOCUMENTATION.md`
- `docs/README.md`
- `docs/website/README.md`
- `docs/_generated/README.md`
- `docs/_Sidebar.md`
- `docs/_Footer.md`

## 7) AI-Leitfaden fuer Entwicklerdokumentation

Fuer AI-gestuetzte Pflege von Entwicklerdokumentation ist verbindlich:
- `docs/_standards/AI_DEVELOPER_DOCUMENTATION_GUIDE.md`

Der Leitfaden regelt insbesondere:
- Datei-Zustaendigkeiten (`ROADMAP.md` vs `FUTURE_ENHANCEMENTS.md` vs `CHANGELOG.md`)
- Pflege-Workflow inkl. Sourcecode-/Issue-Validierung
- Trennung zwischen Zukunftsplanung und erledigten Aenderungen
