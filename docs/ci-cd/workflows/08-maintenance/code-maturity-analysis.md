# Code Maturity Analysis & Auto-Versioning

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/08-maintenance_code-maturity-analysis.yml`

## Aufgabe

Automatisch und zeitgesteuert ausgeführter Workflow: **Code Maturity Analysis & Auto-Versioning**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei relevanten Änderungen unter `src/`, `include/`, `tests/`, `benchmarks/`, `projects/`, `plugins/` sowie bei Änderungen am Workflow oder Analyseskript
- **`schedule`** — Zeitgesteuert (Cron-Schedule) (`0 3 * * *`, täglich um 03:00 UTC)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Jobs

### `analyze-and-version`
**Anzeigename:** Code Maturity Analysis & Auto-Versioning

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install dependencies** — `pip install gitpython pyyaml`
- **Skip if triggered by bot commit** — `COMMIT_MSG=$(git log -1 --pretty=%s)`
- **Run code maturity analysis** — `python .github/scripts/analyze_code_maturity.py --root .`
- **Configure Git bot user** — `git config user.name 'ThemisDB Version Bot'`
- **Commit updated files and push safely** — `git add -A`
- **Write job summary** — `echo "## 📈 Code Maturity Analysis & Auto-Versioning" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/08-maintenance_code-maturity-analysis.yml)
- [Alle Workflows](../README.md)
