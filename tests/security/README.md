> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# security

Pfad: `tests/security`

## Zweck
Dieser Ordner enthält 1 Unterordner und 9 Dateien und bildet einen abgegrenzten Teil der Repository-Struktur.

## Unterordner
- `attack-vectors/`

## Dateien nach Kategorien
- **Sourcecode**: `test_access_control_manager.cpp`, `test_arrow_user_registration_plugin.cpp`, `test_fips_crypto_mode.cpp`, `test_input_validation_security.cpp`, `test_process_parser_hardening.cpp`, `test_row_level_security.cpp`, `test_security_evidence_collector.cpp`, `test_security_negative_integration.cpp`
- **Sonstiges**: `test_jwt_security.cpp.skip`

## Hinweise
- Änderungen in diesem Ordner sollten mit den übergeordneten Architektur- und Sicherheitsrichtlinien des Projekts abgestimmt werden.
- Für tieferliegende Teilbereiche existieren ggf. zusätzliche README- und Moduldokumente.

_Automatisch erzeugt/aktualisiert am 2026-04-17._
