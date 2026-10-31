---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "build-test"
Canonical_References: "tests/exporters/CMakeLists.txt; CMakePresets.json; include/exporters/*.h"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Exporter tests

Exact model backend not verifiable from final document text alone.

This directory has 16 focused C++ test source files. They cover AQL predicate
filtering, Arrow IPC, data augmentation, encryption, format registration,
contract/phase hardening, high-cardinality stress, templates, Hugging Face
dataset and Hub behavior, incremental watermarks, joins, JSONL, Parquet, and
cursor streaming. The implementation and test inventory are in
[`CMakeLists.txt`](./CMakeLists.txt); public contracts are in
[`../../include/exporters/README.md`](../../include/exporters/README.md).

## Configure, build, and run

Use the repository's `linux-debug` preset with tests enabled:

```sh
cmake --preset linux-debug -DTHEMIS_BUILD_TESTS=ON
cmake --build --preset linux-debug --target module_exporters_test_jsonl_llm_exporter_focused
ctest --test-dir build-gcc-linux-debug \
  -R '^test_jsonl_llm_exporter_ExportersFocusedTests$' --output-on-failure
```

Replace the example stem/target with another `test_*.cpp` basename. CMake
creates `module_exporters_<stem>_focused` and registers
`<stem>_ExportersFocusedTests`; all entries carry the `exporters` label. Run the
whole module's registered tests with:

```sh
ctest --test-dir build-gcc-linux-debug -L exporters --output-on-failure
```

The selected configure preset and local dependency availability affect whether
these commands complete; record actual results instead of inferring success
from target registration.
