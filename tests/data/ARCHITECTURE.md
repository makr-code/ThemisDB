# Test Data Architecture (`tests/data/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Kontext

`tests/data/` stellt wiederverwendbare Datenartefakte für deterministische Testläufe bereit.

## Architektur-Surfaces

| Surface | Ort |
|---|---|
| Zertifikats- und Signatur-Testdaten | `tests/data/certificates/` |
| Konsumenten | Security-/Integrations-Tests unter `tests/` |

## Laufzeitmodell

1. Tests lesen statische Daten aus `tests/data/`.
2. Daten repräsentieren bekannte Positiv-/Negativfälle (z. B. gültig/abgelaufen/schwach).
3. Reproduktion erfolgt über identische Dateipfade im Repository.

## Nicht-Ziele

- keine produktive Schlüssel- oder Zertifikatsverwaltung
- keine Runtime-Generierung außerhalb expliziter Testskripte

## Sourcecode Verification (Scope: tests/data)

- Verifiziert:
  - `tests/data/certificates/README.md`
  - Zertifikats-/Signaturartefakte unter `tests/data/certificates/`
