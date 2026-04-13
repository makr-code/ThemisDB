# Docker Image CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/04-release_docker-image.yml`

## Aufgabe

CI-Workflow zum Bauen eines Docker-Images für Release-Tags (`v*`) mit optionalem manuellem Start per `workflow_dispatch`.

## Auslöser (Triggers)

- **`push`** — Automatisch bei Tags `v*`
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `tag_name` | Optionaler Tag für manuellen Lauf | — | — |

## Jobs

### `build`
**Anzeigename:** Build

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout** — `actions/checkout@v4`
- **Create placeholder for optional proprietary directory** — `mkdir -p internal`
- **Build the Docker image** — `docker build ...`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/04-release_docker-image.yml)
- [Alle Workflows](../README.md)
