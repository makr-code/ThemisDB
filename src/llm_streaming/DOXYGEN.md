---
Author: "ThemisDB Team"
Created: "2026-09-20"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# LLM Streaming Scoped Inventory and Doxygen Status

## Scoped module inventory (not generated Doxygen output)

The current `src/llm_streaming/` scope contains 17 Markdown files and
`.gitkeep`; it contains no C++ source or header. This page is a manually
maintained scope/status note, not a Doxygen-generated API inventory.

The previous version referred to a machine-local Windows XML path and listed
stub classes from stress/benchmark sources. Those counts and stub details are
stale and are not the API of this directory.

## External helper reference (not part of this directory's inventory)

The separate public helper declaration is `themis::llm::StreamingHandler` in
`include/llm/streaming_handler.h`, implemented by
`src/llm/streaming_handler.cpp`. Its public methods are:

- `formatSseEvent(token, request_id, index, done)`
- `formatDoneEvent(request_id)`
- `formatChunkedData(data)`
- `makeStreamCallback(sink, request_id)`

This external helper is documented in `API.md`; it is not Doxygen output for
`src/llm_streaming/`. The `Stub*` types in
`tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`,
`tests/integration/test_llm_streaming_soak.cpp`, and
`benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp` are local
validation scaffolding, not public module classes.
`tests/llm/test_streaming_handler.cpp` is the direct unit test for the helper
API; it is not an end-to-end transport inventory.

## Generation status

The repository generator is `scripts/generate_module_doxygen_xml.py` and
supports `--modules`, `--write-module-markdown`, and required output locations
for `--output-root`, `--manifest`, and `--summary-md`. No generated XML artifact
or completed generator run for this updated inventory was reviewed here.
Generate API output from the canonical `llm` header/implementation if needed;
do not characterize that output as an inventory of `src/llm_streaming/`.

**TODO:** record the invocation's actual output paths after a current Doxygen
run. The old absolute `C:\Projects\...` paths are intentionally not retained.
