# Secret Scanning CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/05-quality_security_secret-scanning-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Secret Scanning**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `gitleaks`
**Anzeigename:** Gitleaks secret scan

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository (full history)** — `actions/checkout@v4`
- **Run Gitleaks** — `gitleaks/gitleaks-action@v2`
- **Write job summary** — `echo "## 🔐 Secret Scanning – Gitleaks" >> "$GITHUB_STEP_SUMMARY"`

### `detect-secrets`
**Anzeigename:** detect-secrets baseline audit

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install detect-secrets** — `pip install detect-secrets==1.4.0`
- **Audit against baseline** — `detect-secrets audit --report --only-allowlisted .secrets.baseline || true`
- **Write job summary** — `echo "## 🔐 Secret Scanning – detect-secrets" >> "$GITHUB_STEP_SUMMARY"`

### `entropy-scan`
**Anzeigename:** Entropy and pattern scan

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Run entropy scanner on all tracked files** — `python3 scripts/secret_scan.py --all \`
- **Upload scan report on failure** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔐 Secret Scanning – Entropy + Pattern Scanner" >> "$GITHUB_STEP_SUMMARY`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality_security_secret-scanning-ci.yml)
- [Alle Workflows](../README.md)


