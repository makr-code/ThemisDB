# Docs Orphan Check

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/08-maintenance_docs-orphan-check.yml`

## Aufgabe

CI-Workflow zur Erkennung verwaister Modul-Dokumentationsverzeichnisse in `docs/de/` und `docs/en/` sowie kaputter `PRIMARY_SOURCES.md`-Referenzen auf unterstützte Quell-Doku-Pfade wie `src/`, `include/`, `examples/`, `tools/`, `benchmarks/`, `tests/` oder `external/chimera/`.

## Auslöser (Triggers)

- **`push`** — Automatisch bei Änderungen an unterstützten Quell-Doku-Pfaden, `docs/de/`, `docs/en/`, dem Prüfskript oder dem Workflow selbst
- **`pull_request`** — Automatisch bei Pull Requests mit denselben Pfaden
- **`schedule`** — Zeitgesteuert (Cron-Schedule) (`0 4 * * *`, täglich um 04:00 UTC)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `fail_on_findings` | Workflow fehlschlagen lassen, wenn Orphans oder kaputte Referenzen gefunden werden | — | `false` |

## Jobs

### `check`
**Anzeigename:** docs/de + docs/en Orphan Check

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Run docs orphan check** — `python3 scripts/docs-orphan-check.py --format text --output /tmp/docs-orphan-check.txt`
- **Generate JSON orphan report** — `python3 scripts/docs-orphan-check.py --format json --output /tmp/docs-orphan-check.json --quiet`
- **Upload orphan-check reports** — `actions/upload-artifact@v4`
- **Write job summary** — schreibt Kennzahlen und den Textreport nach `$GITHUB_STEP_SUMMARY`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../../../.github/workflows/08-maintenance_docs-orphan-check.yml)
- [Alle Workflows](../README.md)
