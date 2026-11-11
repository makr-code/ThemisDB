---
Author: ThemisDB Contributors
Created: 2026-04-17
Last Updated: 2026-11-11
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
compilation of `monitoring_api_handler.cpp`, `monitoring_api_routes.cpp`, `index_api_handler.cpp` and
`prompt_api_handler.cpp` receives
`/Od` plus explicit `/Ob0` to reduce compiler heap pressure (`C1060`) on
constrained CI workers. `/Ob0` overrides inherited `/Ob2` inlining, which
`/Od` alone does not explicitly disable.
`/bigobj`, unity exclusion and the global `/Zm200` setting are retained.

The local `vscode-windows-release` configure preset sets the option to `OFF`.
Release therefore uses its normal `/O2` optimization and inherited inlining
for these four files, without either per-file `/Od` or `/Ob0` override.
The workspace tasks named
`CMake: Configure (windows-release)` and `CMake: Build (windows-release)` use
this local preset. Build directory and edition remain
`build-msvc-windows-release` and `COMMUNITY`.

Reconfigure before building after changing this option. CMake caches the value:
switching to another preset using the same build directory does not reset it.
To restore the workaround locally, set the local override to `ON` and reconfigure.
There is no automatic retry with disabled optimization if `/O2` fails.

Historically, the Windows lane in `.github/workflows/release-build-matrix.yml`
serialized dependency/configure and build work (`VCPKG_MAX_CONCURRENCY=1`,
`CMAKE_BUILD_PARALLEL_LEVEL=1`, build `--parallel 1`).
[Windows run 42155076805, job 127687580213](https://github.com/makr-code/ThemisDB/actions/runs/42155076805/job/127687580213)
still encountered `C1060` despite serial compilation and `/Od`. The correction
at that time added `/Ob0` under the same non-Debug, low-memory guard for all three
handlers; other translation units were unchanged. That correction required a
fresh native Windows run; no native success is claimed for it.

On 2026-11-09, [run 42638257654, job 129343673144](https://github.com/makr-code/ThemisDB/actions/runs/42638257654/job/129343673144)
still failed with `C1060` at `monitoring_api_handler.cpp(2246)` inside
`MonitoringApiHandler::registerRoutes()` on VS18 MSVC 14.51 Hostx64/x64, despite
serial compilation and `/Od /Ob0 /GL- /bigobj /Zm200`. The scoped fix moves only
that method, unchanged with all 47 routes, into `monitoring_api_routes.cpp`,
including only the handler and route-registry headers. The original handler
retains the registry include for `handleOpenApi()`. Both modular and monolithic
source lists and MSVC property lists include the new translation unit alongside
the handler: modular low-memory default/non-Debug/opt-out semantics and
monolithic `/bigobj` and unity exclusions are unchanged. No flags are changed.
Current dependency/configure concurrency defaults to 4 independently of the
serial build; the historical serial-configure evidence above is not a statement
of the current default. Native Windows validation of this split remains pending.

On 2026-11-10, [run 42747840520, job 129662241930](https://github.com/makr-code/ThemisDB/actions/runs/42747840520/job/129662241930)
failed with `C1060` at `monitoring_api_routes.cpp(155)` with the same low-memory
flags and serial build. The route translation unit now uses eight file-local
registration helpers, each containing at most ten routes, to reduce per-function
nested JSON initializer pressure. The dispatcher preserves all 47 annotations
and their order; compiler flags and both source lists remain unchanged.
Focused tests check the batch bound, route coverage, nested license schema, and
repeat-registration stability. Native MSVC confirmation still requires a new
Windows run.

On 2026-11-11, [run 42972100442, job 130366604809](https://github.com/makr-code/ThemisDB/actions/runs/42972100442/job/130366604809)
confirmed that the eight-helper split was insufficient: serial VS18 MSVC
14.51.36231 Hostx64/x64 compilation with `/Od /Ob0 /GL- /bigobj /Zm200` failed
with `C1060` at `monitoring_api_routes.cpp(158)`, at the end of the single deeply
nested license response initializer. The license helper now builds each property,
the object schema, and the response in separate shallow JSON statements, without
raw JSON parsing. All nine properties, examples, date format, response content,
route metadata, and the 47-route registration order are preserved. Flags, source
lists, workflows, and unrelated routes are unchanged. Existing focused tests
guard per-expression complexity and compare the complete license response and
metadata. Native Windows verification of this change remains pending.
Local Linux validation on 2026-11-11: all 11 compiler regressions passed.
The Windows memory suite had three passes and two configure-test errors before
log creation (Windows-only `cygpath` unavailable and `THEMIS_CACHE_TOOLS_PREFIX`
unset); its three build/parallelism tests also passed when run explicitly.
The `community-release` configure with tests enabled stopped at missing RocksDB;
the existing focused C++ target could not build (`build.ninja` was not generated),
so its tests were not run.

Validation on 2026-10-29: both generated compile commands contain `/O2` and
`/bigobj`, without `/Od`; the local Release build completed without `C1060` or
`D9025`. Health, Metrics and JSON-body Index-Stats checks, plus A/B/C/F live
smokes, passed. Evidence is under
`benchmarks/benchmark_results/optimized_release_validation/20261029_204406/`.
This historical evidence verifies only the monitoring and index handler
overrides, not the newly included prompt handler, a whole-program optimization audit
or comparative performance. The previous Release measurement binary had a
different hash and its raw results are not relabeled.

_Automatisch erzeugt/aktualisiert am 2026-04-17._
