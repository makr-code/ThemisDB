# distributed_knowledge

Pfad: `include/distributed_knowledge`

## Zweck
Dieser Ordner enthält 0 Unterordner und 4 Dateien und bildet einen abgegrenzten Teil der Repository-Struktur.

## Dateien nach Kategorien
- **Sourcecode**: `adapter_capability_announcement.h`, `cross_shard_feedback_sync.h`, `federated_rag_merger.h`, `lora_federation_coordinator.h`

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
#include "distributed_knowledge/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
