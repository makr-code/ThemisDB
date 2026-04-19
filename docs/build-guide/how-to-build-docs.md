# How to Build the ThemisDB Documentation Locally

This guide covers how to build the ThemisDB documentation site locally using MkDocs.

---

## Prerequisites

- **Python 3.9+** installed and on your `PATH`
- A clone of the repository

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
```

---

## 1 — Install Dependencies

All documentation dependencies are in `requirements-docs.txt`:

```bash
pip install -r requirements-docs.txt
```

> **Tip:** Use a virtual environment to keep your system Python clean:
> ```bash
> python -m venv .venv
> source .venv/bin/activate   # Windows: .venv\Scripts\activate
> pip install -r requirements-docs.txt
> ```

---

## 2 — Build the Static Site

Build the documentation site **without** PDF export (faster, recommended for local development):

```bash
mkdocs build --config-file mkdocs-nopdf.yml --clean
```

Or use the convenience script:

```bash
./scripts/build-docs.sh        # Linux / macOS
.\scripts\build-docs.ps1       # Windows (PowerShell)
```

The generated site is placed in `./site/`.

---

## 3 — Live Preview (Development Server)

Start the MkDocs development server to preview changes with live reload:

```bash
mkdocs serve --config-file mkdocs-nopdf.yml
```

Open <http://127.0.0.1:8000> in your browser. Every time you save a file, the
page refreshes automatically.

---

## 4 — Build with PDF Export (Optional)

PDF export requires the `mkdocs-with-pdf` plugin and is **opt-in** via an
environment variable so it does not slow down every build:

```bash
# Linux / macOS
ENABLE_PDF_EXPORT=1 ./scripts/build-docs.sh

# Windows (PowerShell)
$env:ENABLE_PDF_EXPORT = "1"
.\scripts\build-docs.ps1
```

The PDF is written to `artifacts/docs/ThemisDB-Documentation-v1.3.5.pdf`
(not inside `docs/` — see [Artifact Locations](#artifact-locations)).

---

## 5 — Validate the Documentation

Run the full documentation validation suite (linter + link checker + TOC
checker) before opening a pull request:

```bash
./scripts/validate-docs.sh
```

Or run individual checks:

```bash
# Markdown / frontmatter linting
python3 scripts/docs-lint.py

# Internal link validation
python3 scripts/link-check.py --internal-only

# TOC consistency
python3 scripts/toc-check.py
```

---

## Artifact Locations

| Artifact | Path |
|----------|------|
| Static site | `./site/` |
| PDF export | `./artifacts/docs/ThemisDB-Documentation-v1.3.5.pdf` |

Both paths are excluded from Git via `.gitignore`.

---

## CI Pipeline Overview

The GitHub Actions pipeline mirrors the local workflow:

| Stage | Trigger | Workflow job |
|-------|---------|--------------|
| **Validate** (lint + links + TOC) | Every PR and push | `validate` |
| **Build site** (mkdocs) | After validate passes | `build-site` |
| **Build PDF** (on-demand) | `workflow_dispatch` with `enable_pdf: true`, or release tags `v*` | `build-pdf` |
| **Deploy to GitHub Pages** | Push to `main` or manual dispatch | `deploy-pages` |

See `.github/workflows/docs_docs-pipeline.yml` for the full definition.
