# ThemisDB Config Editor (wxWidgets)

Cross-platform desktop prototype for editing ThemisDB configuration files with a structured tabbed layout.

## Features

- Native wxWidgets UI (Windows/Linux/macOS)
- Tabbed editor:
  - General: server host and port
  - Storage: data directory
  - Network: bind address and TLS toggle
  - Raw YAML: direct full-file editing
- Open/Save/Save As for YAML/JSON files
- Bidirectional sync between form tabs and raw YAML tab

## Build

Enable the optional build flag during configure:

- `-DTHEMIS_BUILD_TOOLS=ON` (default)

The target name is:

- `themis_config_wx`

## Dependencies

- wxWidgets (core/base/adv)
- yaml-cpp

If dependencies are missing, CMake skips the target and prints a status warning.
