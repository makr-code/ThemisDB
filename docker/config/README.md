# config

Pfad: `docker/config`

## Zweck
Dieser Ordner enthält 0 Unterordner und 2 Dateien und bildet einen abgegrenzten Teil der Repository-Struktur.

## Dateien nach Kategorien
- **Container-Default**: `config.yaml` — wird im Image nach `/etc/themis/config.yaml` kopiert (`THEMIS_CONFIG`). Enthält nur Schlüssel, die `themis_server` liest (`server.host|port|worker_threads`, `storage.rocksdb_path`, `features.*`).
- **LoRA-Entwicklungsprofil**: `themisdb.yaml` — optional per `-v ./config/themisdb.yaml:/etc/themis/config.yaml:ro` einbinden.

## Hinweise
- Vorrang: CLI > Umgebungsvariable > Config-Datei > Default (siehe `docker/README.md`, Abschnitt "Runtime contract").
- Änderungen in diesem Ordner sollten mit den übergeordneten Architektur- und Sicherheitsrichtlinien des Projekts abgestimmt werden.
- Für tieferliegende Teilbereiche existieren ggf. zusätzliche README- und Moduldokumente.

_Automatisch erzeugt/aktualisiert am 2026-04-17._
