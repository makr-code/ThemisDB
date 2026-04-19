# toolbox

Pfad: `include/toolbox`

## Zweck
Dieser Ordner enthält 0 Unterordner und 5 Dateien und bildet einen abgegrenzten Teil der Repository-Struktur.

## Dateien nach Kategorien
- **Sourcecode**: `content_toolbox_bridge.h`, `ingestion_toolbox.h`, `toolbox_builder.h`
- **Dokumentation**: `FUTURE_ENHANCEMENTS.md`, `ROADMAP.md`

## Hinweise
- Änderungen in diesem Ordner sollten mit den übergeordneten Architektur- und Sicherheitsrichtlinien des Projekts abgestimmt werden.
- Für tieferliegende Teilbereiche existieren ggf. zusätzliche README- und Moduldokumente.

_Automatisch erzeugt/aktualisiert am 2026-04-17._

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "toolbox/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
