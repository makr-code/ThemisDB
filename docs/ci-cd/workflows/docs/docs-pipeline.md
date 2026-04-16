# Documentation Pipeline

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/docs_docs-pipeline.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Documentation Pipeline**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`, `release/**`) (13 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (13 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `enable_pdf` | Enable PDF export (ENABLE_PDF_EXPORT) | — | `False` |
| `deploy_pages` | Deploy site to GitHub Pages | — | `False` |

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `validate`
**Anzeigename:** Validate (lint + links + TOC + header + drift)

**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/05-quality_validation_documentation-validation.yml`

Umfasst: docs-lint, link-check (intern), external-link-check, toc-validation,
metadata-check (`DOC_METADATA.md`, `README.md`, `developers.md`),
doc-header-check (changed-only), drift-detection, validation-summary.

### `build-site`
**Anzeigename:** Build MkDocs Site

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `validate`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install MkDocs dependencies** — `pip install -r requirements-docs.txt`
- **Build site (without PDF)** — `mkdocs build --config-file mkdocs-nopdf.yml --clean --strict`
- **Upload site artifact** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🏗️ MkDocs Build" >> "$GITHUB_STEP_SUMMARY"`

### `build-pdf`
**Anzeigename:** Build PDF (on-demand)

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `validate`
**Bedingung:** `(github.event_name == 'workflow_dispatch' && inputs.enable_pdf == true) || startsWith(github.ref, 'r`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install MkDocs + PDF plugin** — `pip install -r requirements-docs.txt`
- **Create PDF output directory** — `mkdir -p artifacts/docs`
- **Build site with PDF export** — `mkdocs build --clean`
- **Upload PDF artifact** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📄 PDF Export" >> "$GITHUB_STEP_SUMMARY"`

### `deploy-pages`
**Anzeigename:** Deploy to GitHub Pages

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `build-site`
**Bedingung:** `(github.ref == 'refs/heads/main' && github.event_name == 'push') || (github.event_name == 'workflow_`

**Schritte:**

- **Download site artifact** — `actions/download-artifact@v4`
- **Configure GitHub Pages** — `actions/configure-pages@v4`
- **Upload to Pages** — `actions/upload-pages-artifact@v3`
- **Deploy to GitHub Pages** — `actions/deploy-pages@v4`
- **Write job summary** — `echo "## 🚀 GitHub Pages Deployment" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `read`
- `issues`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/docs_docs-pipeline.yml)
- [Alle Workflows](../README.md)
