# Documentation Validation

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

♻️ **Reusable Workflow**

> **Workflow-Datei (historisch):** .github/workflows/05-quality_validation_documentation-validation.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Wiederverwendbarer Hilfs-Workflow: **Documentation Validation**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`, `release/**`) (11 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (11 überwachte Pfade)
- **`workflow_call`** — Aufrufbar als wiederverwendbarer Workflow (reusable workflow)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `docs-lint`
**Anzeigename:** Documentation Linting

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install dependencies** — `pip install pyyaml`
- **Run documentation linter** — `python3 scripts/docs-lint.py`
- **Generate lint report (JSON)** — `python3 scripts/docs-lint.py --format json --output /tmp/lint-report.json`
- **Upload lint report** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📝 Documentation Linting" >> "$GITHUB_STEP_SUMMARY"`

### `link-check`
**Anzeigename:** Link Validation

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install dependencies** — `pip install pyyaml`
- **Run link checker (internal links only)** — `python3 scripts/link-check.py --internal-only`
- **Generate link report (JSON)** — `python3 scripts/link-check.py --internal-only --format json --output /tmp/link-r`
- **Upload link report** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔗 Link Validation" >> "$GITHUB_STEP_SUMMARY"`

### `external-link-check`
**Anzeigename:** External Link Check

**Läuft auf:** `ubuntu-latest`
**Bedingung:** `github.ref == 'refs/heads/main' || github.ref == 'refs/heads/develop'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install dependencies** — `pip install pyyaml requests`
- **Run link checker (all links)** — `python3 scripts/link-check.py`
- **Generate external link report (JSON)** — `python3 scripts/link-check.py --format json --output /tmp/external-link-report.j`
- **Upload external link report** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🌐 External Link Check" >> "$GITHUB_STEP_SUMMARY"`

### `toc-validation`
**Anzeigename:** TOC Validation

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install dependencies** — `pip install pyyaml`
- **Run TOC validator** — `python3 scripts/toc-check.py`
- **Generate TOC report (JSON)** — `python3 scripts/toc-check.py --format json --output /tmp/toc-report.json`
- **Upload TOC report** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📚 TOC Validation" >> "$GITHUB_STEP_SUMMARY"`

### `metadata-check`
**Anzeigename:** Doc Metadata Check

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install dependencies** — `pip install pyyaml`
- **Check doc metadata (Secondary Docs)** — `python3 scripts/docs-lint.py \`
- **Generate metadata report (JSON)** — `python3 scripts/docs-lint.py \`
- **Upload metadata report** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🏷️ Doc Metadata Check" >> "$GITHUB_STEP_SUMMARY"`

### `doc-header-check`
**Anzeigename:** Doc Header Check (changed-only)

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install dependencies** — `pip install pyyaml`
- **Run doc header check (changed-only)** — `python3 scripts/doc-header-check.py --mode changed-only --base-ref origin/develo`
- **Generate header check report (JSON)** — `python3 scripts/doc-header-check.py \`
- **Upload header report** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📋 Doc Header Check" >> "$GITHUB_STEP_SUMMARY"`

### `drift-detection`
**Anzeigename:** Drift Detection (Primary → Secondary)

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Run drift detector** — `python3 scripts/drift-detector.py \`
- **Generate drift report (JSON)** — `python3 scripts/drift-detector.py \`
- **Create issues for drifting / stale docs** — `python3 tools/ci/module_docs_issue_reporter.py \`
- **Upload drift report** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔄 Drift Detection" >> "$GITHUB_STEP_SUMMARY"`

### `validation-summary`
**Anzeigename:** Validation Summary

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `docs-lint`, `link-check`, `toc-validation`, `metadata-check`
**Bedingung:** `always()`

**Schritte:**

- **Check job results** — `LINT="${{ needs.docs-lint.result }}"`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality_validation_documentation-validation.yml)
- [Alle Workflows](../README.md)


