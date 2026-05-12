# Generic Installer Framework (GIUF)

Generisches, plattformuebergreifendes Framework fuer Installation, Update und Deinstallation von Desktop-Produkten.

## Vision

- Ein Core fuer Windows, Linux und macOS
- Produktlogik ueber externe Konfiguration statt hardcodierter Regeln
- Sichere Release-Auslieferung mit Signatur- und SHA-256-Pruefung
- Atomische Installation mit Rollback bei Fehlern

## Kernprinzipien

- Product-agnostic Core
- Declarative Configuration
- Secure by default
- Atomic Operations
- Rollback by design

## Schnellstart

1. Configure

```powershell
cmake -S . -B build
```

2. Build

```powershell
cmake --build build --config Release
```

3. CLI Stub testen

```powershell
.\build\tools\giuf-cli\giuf-cli.exe check
```

## Repository-Struktur

- `include/` oeffentliche API
- `src/` Core-Implementierung
- `tools/giuf-cli/` Referenz-CLI
- `schemas/` JSON-Schemas fuer Manifest/Config/Recipes
- `examples/` Beispielkonfiguration und Beispielmanifest
- `docs/` Integrations- und Architekturdokumentation

## Dokumente

- `ARCHITECTURE.md`
- `ROADMAP.md`
- `FUTURE_ENHANCEMENTS.md`
- `CONTRIBUTING.md`
- `SECURITY.md`
- `CHANGELOG.md`

## Security Baseline

- Manifest-Signaturpruefung ist Pflicht
- SHA-256-Verifikation fuer Artefakte ist Pflicht
- TLS fuer Downloadquellen ist Pflicht
- Downgrade nur per expliziter Policy-Freigabe

## Status

Bootstrap fuer eigenstaendiges Repository ist vorhanden. Naechster Schritt ist die produktive Core-Implementierung der Workflows `check`, `install`, `update`, `uninstall`.
