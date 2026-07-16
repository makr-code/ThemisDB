# Root Docs Hygiene

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/08-maintenance_root-docs-hygiene.yml`

## Aufgabe

CI-Workflow zur Klassifizierung von Markdown- und Textdateien im Repository-Root in drei Gruppen: kanonische Root-Dokumente, Kandidaten fuer `docs/` und Kandidaten fuer `docs/archive/`.

## Ausloeser (Triggers)

- **`push`** — Automatisch bei Aenderungen an Top-Level-`.md`/`.txt`, `docs/**`, dem Pruefskript oder dem Workflow selbst
- **`pull_request`** — Automatisch bei Pull Requests mit denselben Pfaden
- **`schedule`** — Zeitgesteuert (Cron-Schedule) (`30 4 * * *`, taeglich um 04:30 UTC)
- **`workflow_dispatch`** — Manuell ueber die GitHub Actions UI ausfuehrbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `fail_on_findings` | Workflow fehlschlagen lassen, wenn Move-/Archive-Kandidaten vorhanden sind | — | `false` |

## Jobs

### `check`
**Anzeigename:** repo root markdown hygiene

**Laeuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Run root docs hygiene check** — `python3 scripts/root-docs-hygiene.py --format text --output /tmp/root-docs-hygiene.txt`
- **Generate JSON hygiene report** — `python3 scripts/root-docs-hygiene.py --format json --output /tmp/root-docs-hygiene.json`
- **Upload hygiene reports** — `actions/upload-artifact@v4`
- **Write job summary** — schreibt Kennzahlen und den Textreport nach `$GITHUB_STEP_SUMMARY`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../../../.github/workflows/08-maintenance_root-docs-hygiene.yml)
- [Alle Workflows](../README.md)
