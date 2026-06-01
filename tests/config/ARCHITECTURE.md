# Test Configuration Architecture (`tests/config/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Kontext

`tests/config/` kapselt testnahe Konfigurationsartefakte, die von Testläufen konsistent referenziert werden.

## Architektur-Surfaces

| Surface | Ort |
|---|---|
| Test-Policy-Konfiguration | `tests/config/policies.test.yaml` |
| Aufrufkontext | `tests/CMakeLists.txt` + `ctest`-Presets |

## Laufzeitmodell

1. Tests laden Konfigurationen aus `tests/config/`.
2. Policy-Parameter steuern Verhalten/Assertions in den betroffenen Suiten.
3. Änderungen werden über Standard-Testflow (`cmake` + `ctest`) verifiziert.

## Nicht-Ziele

- keine produktive Runtime-Konfiguration
- keine Duplikation globaler App-Konfiguration aus `config/`

## Sourcecode Verification (Scope: tests/config)

- Verifiziert:
  - `tests/config/policies.test.yaml`
  - Referenz auf `tests/` Testlaufkontext (`tests/README.md`)
