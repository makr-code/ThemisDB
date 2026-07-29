# LLM Wiki Tests

## Migration

| ThemisDB source | Destination |
|---|---|
| `tests/test_llm_wiki_mvp.py` | `tests/test_llm_wiki_mvp.py` |
| `tests/legacy/importer/test_wikipedia_ingestion_plugin.cpp` | `tests/legacy/test_wikipedia_ingestion_plugin.cpp` |

## Running

```bash
# Python tests (requires ThemisDB SDK running)
python -m pytest tests/test_llm_wiki_mvp.py -v

# C++ legacy tests (via CMake/CTest)
ctest -L llm_wiki --output-on-failure
```
