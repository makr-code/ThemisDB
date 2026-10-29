---
Author: ThemisDB Contributors
Created: 2026-04-17
Last Updated: 2026-10-29
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: unknown
---

# cmake

Exact model backend not verifiable from final document text alone.

Pfad: `cmake`

## Zweck
Dieser Ordner enthält 4 Unterordner und 42 Dateien und bildet einen abgegrenzten Teil der Repository-Struktur.

## Unterordner
- `editions/`
- `features/`
- `platforms/`
- `validation/`

## Dateien nach Kategorien
- **Konfiguration**: `AccelerationBackends.cmake`, `BlobStorage.cmake`, `BufferManagement.cmake`, `BuildInfo.cmake`, `ChimeraAdapters.cmake`, `CMakePresets.json`, `CompilerCache.cmake`, `CompilerOptions.cmake`, `ContentProcessors.cmake`, `CopyRuntimeDlls.cmake`, `CUDA-Windows-MSVC.cmake`, `Dependencies.cmake`, `DistributedTraining.cmake`, `DockerBuild.cmake`, `DockerBuildSystem.cmake`, `DockerContext.cmake`, `EditionFeatures.cmake`, `EditionMatrix.cmake`, `ErrorHealthServices.cmake`, `FindKerberos.cmake` … (+19)
- **Dokumentation**: `CMakeLists.txt`, `CMakeLists_debug.txt`, `MODULAR_SOURCES.md`

## Hinweise
- Änderungen in diesem Ordner sollten mit den übergeordneten Architektur- und Sicherheitsrichtlinien des Projekts abgestimmt werden.
- Für tieferliegende Teilbereiche existieren ggf. zusätzliche README- und Moduldokumente.

## MSVC Handler Optimization

`THEMIS_MSVC_HEAVY_HANDLER_LOW_MEMORY` controls an existing compiler-memory
workaround in the modular MSVC network build. Its default is `ON`: non-Debug
compilation of `monitoring_api_handler.cpp` and `index_api_handler.cpp` receives
`/Od` to avoid compiler heap exhaustion (`C1060`) on constrained CI workers.
`/bigobj`, unity exclusion and the global `/Zm200` setting are retained.

The local `vscode-windows-release` configure preset sets the option to `OFF`.
Release therefore uses its normal `/O2` optimization for these two files,
without the per-file `/Od` override. The workspace tasks named
`CMake: Configure (windows-release)` and `CMake: Build (windows-release)` use
this local preset. Build directory and edition remain
`build-msvc-windows-release` and `COMMUNITY`.

Reconfigure before building after changing this option. CMake caches the value:
switching to another preset using the same build directory does not reset it.
To restore the workaround locally, set the local override to `ON` and reconfigure.
There is no automatic retry with disabled optimization if `/O2` fails.

Validation on 2026-10-29: both generated compile commands contain `/O2` and
`/bigobj`, without `/Od`; the local Release build completed without `C1060` or
`D9025`. Health, Metrics and JSON-body Index-Stats checks, plus A/B/C/F live
smokes, passed. Evidence is under
`benchmarks/benchmark_results/optimized_release_validation/20261029_204406/`.
This verifies the two handler overrides, not a whole-program optimization audit
or comparative performance. The previous Release measurement binary had a
different hash and its raw results are not relabeled.

_Automatisch erzeugt/aktualisiert am 2026-04-17._
