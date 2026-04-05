# CI Scope Classifier

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

♻️ **Reusable Workflow**

> **Workflow-Datei (historisch):** .github/workflows/01-core_ci-scope-classifier.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Wiederverwendbarer Hilfs-Workflow: **CI Scope Classifier**.

## Auslöser (Triggers)

- **`workflow_call`** — Aufrufbar als wiederverwendbarer Workflow (reusable workflow)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Jobs

### `classify`
**Anzeigename:** Classify CI Scope

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Determine changed files** — `CHANGED_FILES_PATH="/tmp/changed_files.txt"`
- **Set up Python** — `actions/setup-python@v5`
- **Install PyYAML** — `pip install --quiet pyyaml`
- **Run CI scope classifier** — `OUTPUTS_FILE="/tmp/ci_scope_outputs.env"`
- **Write job summary** — `echo "## 🔍 CI Scope Classifier" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- Workflow-Datei: historisch, siehe `.github/WORKFLOW_REGISTRY.md`
- [Alle Workflows](../README.md)


