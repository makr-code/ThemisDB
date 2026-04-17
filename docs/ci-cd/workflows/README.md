# Workflow-Dokumentation

Dieses Verzeichnis enthält eine Markdown-Dokumentation für jeden GitHub Actions Workflow im Repository.
Für jeden Workflow wird Aufgabe, Auslöser, Eingaben und Funktionsweise beschrieben.

## Wichtiger Hinweis (Stand 2026-04)

Das produktiv aktive CI/CD-Setup wurde auf einen schlanken 8-Workflow-Kern konsolidiert.
Die verbindliche, aktuelle Liste steht in `.github/WORKFLOW_REGISTRY.md`.
Diese Seite dient weiterhin als historischer Katalog fuer fruehere Workflow-Generationen.
Einträge hier koennen daher Workflows referenzieren, die bewusst entfernt wurden.

## Legende

| Symbol | Bedeutung |
|--------|-----------|
| 🔄 CI/CD | Automatisch bei Push/PR ausgelöst |
| 🖱️ Manuell | Nur über die GitHub Actions UI ausführbar |
| ♻️ Reusable | Wiederverwendbarer Workflow, der von anderen aufgerufen wird |
| ⏰ Geplant | Zeitgesteuert (Cron) |

**Historischer Katalog: 217 Workflows**

## Aktiver Workflow-Kern (8)

| Workflow-Datei | Typ | Zweck |
|---|---|---|
| `.github/workflows/01-core_ci.yml` | 🔄 CI/CD | Kern-Build und Basis-Tests |
| `.github/workflows/03-editions_ci.yml` | 🔄 CI/CD | Editions-Matrix (MINIMAL/COMMUNITY/ENTERPRISE/MILITARY/HYPERSCALER) |
| `.github/workflows/04-release_bootstrap-release-branches.yml` | 🖱️ Manuell | Release-Branch-Bootstrap |
| `.github/workflows/04-release_build-binaries.yml` | 🔄 CI/CD | Release-Binaries bauen und Bundle-Artefakte mit docs.db und Mini-LLM publizieren |
| `.github/workflows/04-release_publish-community.yml` | 🔄 CI/CD | Community-Container-Release |
| `.github/workflows/04-release_publish-private.yml` | 🔄 CI/CD | Private Editions (Enterprise/Hyperscaler) publizieren |
| `.github/workflows/09-pr-gates_quick-checks.yml` | 🔄 CI/CD | Schnelle PR-Gates (Lint/Configure/Audit) |
| `.github/workflows/09-pr-gates_path-policy.yml` | 🔄 CI/CD | Lane- und Pfad-Policy fuer PRs |

## Release-Bundle-Inhalt

Der aktive Binary-Release-Workflow `.github/workflows/04-release_build-binaries.yml` publiziert nicht mehr nur das nackte Server-Binary, sondern ein Bundle mit:

- `bin/themis_server`
- `data/docs.db`
- `data/docs_database.json`
- `models/default.gguf` sowie das kleine GGUF-Release-Modell und `mini-llm.manifest.json`

Damit stehen die vorcompilierte Dokumentationsdatenbank und ein kleines llama.cpp-kompatibles Modell bereits im Pre-Release-Artefakt zur Verfuegung.

## Container Registry (Community)

- Verbindliches Docker Hub Repository: `themisdb/themisdb`
- Aktiver Publish-Workflow: `.github/workflows/04-release_publish-community.yml`
- Historische Detailseite: `docs/ci-cd/workflows/04-release/dockerhub-publish-on-release.md`
- Hinweis: `makr-code/themisdb` ist kein aktives Ziel im produktiven CI/CD-Setup.

## Historische Dokumentation

Detaillierte Alt-Dokumente zu frueheren Workflow-Generationen bleiben in diesem
Ordner als Referenz erhalten, gelten jedoch nicht als aktive CI/CD-Definition.
Verbindlich fuer den aktuellen Betrieb sind nur die oben aufgefuehrten 8 Dateien
sowie `.github/WORKFLOW_REGISTRY.md`.
