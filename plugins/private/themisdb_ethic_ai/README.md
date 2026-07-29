# themisdb_ethic_ai

Private plugin repository for the **Ethics AI** module of ThemisDB.

> **Source of Truth:** All implementation is maintained here.  
> The `plugins/private/themisdb_ethic_ai` submodule in the main ThemisDB repository
> points to this repo.

## Repository Structure

```
themisdb_ethic_ai/
├── CMakeLists.txt
├── plugin.json.in              ← Plugin manifest template
├── README.md
├── ROADMAP.md
├── ARCHITECTURE.md
├── SECURITY.md
├── AUDIT.md
├── FUTURE_ENHANCEMENTS.md
├── include/                    ← Public C++ headers
│   ├── ethics_ai_plugin_interface.h
│   ├── ethics_ai_types.h
│   ├── ethics_evaluator.h
│   ├── ethics_profile_registry.h
│   ├── ethics_selection_router.h
│   ├── discourse_memory_store.h
│   ├── discourse_engine.h
│   ├── argument_store.h
│   ├── llm_cascade_router.h
│   ├── synthesis_matrix_builder.h
│   ├── tournament_mode_selector.h
│   ├── convergence_marker_engine.h
│   ├── cross_school_tension_resolver.h
│   ├── position_abstract_validator.h
│   ├── prior_round_compressor.h
│   ├── rag_context_engine.h
│   ├── philosophy_loader.h
│   ├── chain_visualizer.h
│   ├── ethics_aql_queries.h
│   └── ethics_base_entity_adapter.h
├── src/                        ← Implementation files
│   ├── ethics_ai_plugin.cpp
│   ├── ethics_ai_types.cpp
│   ├── ethics_evaluator.cpp
│   ├── ethics_profile_registry.cpp
│   ├── ethics_selection_router.cpp
│   ├── discourse_engine.cpp
│   ├── discourse_memory_store.cpp
│   ├── argument_store.cpp
│   ├── philosophy_loader.cpp
│   ├── rag_context_engine.cpp
│   ├── chain_visualizer.cpp
│   ├── convergence_marker_engine.cpp
│   ├── cross_school_tension_resolver.cpp
│   ├── tournament_mode_selector.cpp
│   ├── synthesis_matrix_builder.cpp
│   ├── position_abstract_validator.cpp
│   ├── prior_round_compressor.cpp
│   └── llm_cascade_router.cpp
├── philosophies/               ← YAML philosophy profiles (22 profiles)
│   ├── kant.yaml
│   ├── utilitarianism.yaml
│   ├── socratic.yaml
│   ├── rawls.yaml
│   └── ... (18 further profiles)
├── examples/
│   ├── CMakeLists.txt
│   └── README.md
├── tests/
│   ├── CMakeLists.txt
│   └── README.md
└── docs/
    ├── API.md
    └── README.md
```

## Migration from ThemisDB Monorepo

Files are migrated from the main ThemisDB repository:

| ThemisDB source | Destination in this repo |
|---|---|
| `src/ethics_ai/*.cpp` | `src/*.cpp` |
| `include/ethics_ai/*.h` | `include/*.h` |
| `src/ethics_ai/*.h` (private headers) | `include/*.h` |
| `plugins/ethics_ai/ethics_ai_plugin.json.in` | `plugin.json.in` |
| `plugins/ethics_ai/philosophies/` | `philosophies/` |
| `plugins/ethics_ai/examples/` | `examples/` |
| `tests/ethics_ai/` | `tests/` |
| `docs/ethics_ai/` | `docs/` |

## Build

```bash
cmake -B build -DTHEMISDB_SDK_DIR=/path/to/themisdb-sdk
cmake --build build
```

## Editions

Available in: **Enterprise**, **Hyperscaler**, **Military**

License feature: `ethics_ai_advanced`
