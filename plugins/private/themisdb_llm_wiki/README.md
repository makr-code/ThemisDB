# themisdb_llm_wiki

Private plugin repository for the **LLM Wiki** module of ThemisDB.

> **Source of Truth:** All implementation is maintained here.  
> The `plugins/private/themisdb_llm_wiki` submodule in the main ThemisDB repository
> points to this repo.

## Purpose

Provides:
- **LLM Wiki MVP CLI** (`src/llm_wiki_mvp.py`) — `index` and `query` subcommands for building and querying LLM-backed wikis backed by ThemisDB
- **Wikipedia ingestion** — C++ pipeline for ingesting Wikipedia XML dumps into ThemisDB
- **Blog/Wiki example** — Python reference implementation for blog/wiki use case

## Repository Structure

```
themisdb_llm_wiki/
├── CMakeLists.txt
├── README.md
├── ROADMAP.md
├── src/
│   ├── llm_wiki_mvp.py         ← LLM Wiki MVP CLI (Python)
│   └── wikipedia/              ← Wikipedia ingestion (C++)
│       ├── wikipedia_xml_parser.cpp
│       ├── wikipedia_pipeline.cpp
│       ├── wikipedia_dump_reader.cpp
│       ├── wikipedia_transform.cpp
│       ├── wikipedia_validator.cpp
│       ├── wikipedia_checkpoint.cpp
│       ├── wikipedia_project_vector.cpp
│       ├── wikipedia_project_graph.cpp
│       ├── wikipedia_project_timeseries.cpp
│       └── wikipedia_project_process.cpp
├── include/
│   └── wikipedia/              ← C++ headers for wikipedia pipeline
│       ├── wikipedia_types.hpp
│       ├── wikipedia_config.hpp
│       ├── wikipedia_transform.hpp
│       ├── wikipedia_pipeline.hpp
│       ├── wikipedia_plugin.hpp
│       ├── wikipedia_checkpoint.hpp
│       └── wikipedia_types.hpp
├── examples/
│   ├── main.py
│   ├── models.py
│   ├── themis_client.py
│   ├── requirements.txt
│   ├── README.md
│   ├── HOW_TO.md
│   ├── ARCHITECTURE.md
│   ├── ROADMAP.md
│   ├── FUTURE_ENHANCEMENTS.md
│   ├── SECURITY.md
│   └── AUDIT.md
├── tools/
│   ├── wikipedia-ingestion/README.md
│   └── publish_wiki.py
├── benchmarks/
│   ├── wikipedia_stress_test.py
│   └── wikipedia_stress_runner.py
├── scripts/
│   └── sync-wiki.ps1
├── tests/
│   ├── test_llm_wiki_mvp.py    ← Python tests for MVP CLI
│   └── legacy/
│       └── test_wikipedia_ingestion_plugin.cpp
└── docs/
    └── RAG_LLM_USE_CASE.md
```

## Migration from ThemisDB Monorepo

| ThemisDB source | Destination |
|---|---|
| `scripts/llm_wiki_mvp.py` | `src/llm_wiki_mvp.py` |
| `tests/test_llm_wiki_mvp.py` | `tests/test_llm_wiki_mvp.py` |
| `examples/11_blog_wiki/` | `examples/` |
| `tools/wikipedia-ingestion/README.md` | `tools/wikipedia-ingestion/README.md` |
| `tools/publish_wiki.py` | `tools/publish_wiki.py` |
| `benchmarks/wikipedia_stress_test.py` | `benchmarks/wikipedia_stress_test.py` |
| `benchmarks/wikipedia_stress_runner.py` | `benchmarks/wikipedia_stress_runner.py` |
| `scripts/sync-wiki.ps1` | `scripts/sync-wiki.ps1` |
| `src/importers/wikipedia_*.cpp` | `src/wikipedia/` |
| `include/importers/wikipedia_*.hpp` | `include/wikipedia/` |
| `tests/legacy/importer/test_wikipedia_ingestion_plugin.cpp` | `tests/legacy/` |
| `docs/use-cases/RAG_LLM_USE_CASE.md` | `docs/RAG_LLM_USE_CASE.md` |

## Build

### Python (LLM Wiki MVP CLI)

```bash
pip install -r examples/requirements.txt
python src/llm_wiki_mvp.py index  --dir <wiki-dir>
python src/llm_wiki_mvp.py query  --query "What is ThemisDB?"
```

### C++ (Wikipedia ingestion)

```bash
cmake -B build -DTHEMISDB_SDK_DIR=/path/to/sdk
cmake --build build
```
