> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# ThemisDB Public Headers (`include/`)

Dieses Verzeichnis enthält die öffentlichen C++-Schnittstellen von ThemisDB.

## Struktur

- Header spiegeln die Kernmodule aus `src/`.
- Top-Level enthält modulweise Unterordner (z. B. `ai`, `api`, `query`, `storage`, `llm`, `temporal`).
- Für Navigation pro Modul dient jeweils das Modul-`README.md` im jeweiligen Unterordner.

## Nutzung

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Installation

Header werden durch den regulären ThemisDB-Build bereitgestellt.

## Usage

Einbindung erfolgt über die Modul-Header unter `include/<modul>/...`.

## Bezug

- Implementierungen: [`../src/README.md`](../src/README.md)
- Gesamt-Buildfluss: [`../README.md`](../README.md)
- Architektur: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Erweiterungen: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- AI-Modul-Header: [`ai/README.md`](./ai/README.md)
