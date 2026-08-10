# LLM Wiki Module

**Status:** HARDENING  
**Phase:** 3-4 (Error handling & comprehensive tests)  
**Last Updated:** 2026-08-10  
**Owner:** LLM Platform Team

---

## Overview

The LLM Wiki module provides enterprise-grade AI safety, workspace management, and guardrail enforcement for LLM-based question-answering systems. It integrates Wikipedia data ingestion with prompt injection detection, workspace state isolation, and edition-based access control for secure multi-tenant deployment.

**Key Capabilities:**
- LLM-based Q&A with safe knowledge retrieval
- Prompt injection detection (5-category guardrail patterns)
- Workspace state management with checksum validation
- Edition gating (Community/Enterprise/Hyperscaler/Military)
- Wikipedia ingestion pipeline with partial-failure semantics
- Atomic state persistence with recovery from corruption

---

## Roadmap Status

**Current Phase:** Phase 3-4 (in progress)  
**Latest Delivery:** 2026-08-10 — Phase 3 error handling + partial Phase 4 test suite complete

**For detailed roadmap:** See `ROADMAP.md` in this directory

### Phase Progress
- [x] Phase 1 — Public SDK interface (ILLMWikiPlugin) + plugin manifest
- [x] Phase 2 — Core implementation (LLMWikiPluginImpl) + Python MVP CLI
- [~] Phase 3 — Error handling & edge cases (Guardrails, workspace state, edition gating) — COMPLETE
- [~] Phase 4 — Comprehensive test suite (LWP-01..LWP-20 focused tests) — IN PROGRESS
- [ ] Phase 5 — Performance hardening (p95 < 200ms target)
- [ ] Phase 6 — Documentation finalization & GA acceptance

---

## Architecture & Key Components

The module uses a **plugin-based architecture** with public SDK boundary and private enterprise implementation:

**Public Interface (include/llm_wiki/):**
- `llm_wiki_plugin_interface.h` — ILLMWikiPlugin contract; editions + capabilities
- `plugin.json` — Manifest with visibility, allowed editions (enterprise/hyperscaler/military)

**Core Implementation (src/llm_wiki/):**
- `guardrail_patterns.h` — 60+ injected-command patterns; shell/code/encoding/privilege/control-flow categories
- `workspace_state_manager.h` — Workspace isolation; checksum-based state validation
- `edition_gate.h/.cpp` — Edition-gated access control enforcement
- `workspace_state_manager.cpp` — Atomic write-replace + log-based recovery

**Private Plugin (plugins/private/themisdb_llm_wiki/):**
- `wikipedia/llm_wiki_plugin_impl.cpp` — LLMWikiPluginImpl; Wikipedia ingestion bridge
- `LLMWikiPluginImpl` — Query + ingest operations with guardrail checks

**Python CLI (scripts/llm_wiki_mvp.py):**
- MVP interface for index/query/workspace commands

---

## Gate Evidence & Testing

**Focused Tests:**
- `tests/llm_wiki/test_llm_wiki_plugin_phase3_phase4_focused.cpp` — LWP-01..LWP-08 (interface contracts)
- `tests/llm_wiki/test_llm_wiki_phase3_edge_cases_focused.cpp` — LWP-09..LWP-20 (workspace lifecycle + guardrails)
- TIMEOUT 120s per test (standard Phase 4+ gate)

**Test Categories (LWP-XX gates):**
- LWP-01..04 — SDK interface (create, query, ingest, status)
- LWP-05 — Guardrail injection detection
- LWP-06 — Workspace state + checksum validation
- LWP-07 — Edition gating (allowed/denied paths)
- LWP-08 — Error handling (invalid input, state corruption)
- LWP-09..16 — Workspace lifecycle (create, delete, orphan detection)
- LWP-17..20 — Guardrail pattern comprehensive coverage

**Performance Gates (Phase 5 target):**
- Query p95 < 200ms (at 5k chunks)
- Ingest throughput > 1k pages/sec

---

## Documentation & APIs

**API Reference:**
- `include/llm_wiki/llm_wiki_plugin_interface.h` — Public C++ contract; factory export (themisdb_llm_wiki_create)
- Doxygen comments cover all public methods

**Design Docs:**
- `ROADMAP.md` — Detailed phase breakdown and delivery evidence
- Phase 3 error handling: `src/llm_wiki/guardrail_patterns.h` (pattern taxonomy)
- Phase 3 workspace: `src/llm_wiki/workspace_state_manager.h` (state lifecycle)
- Phase 3 gating: `src/llm_wiki/edition_gate.h` (edition contract enforcement)

---

## Known Issues & Limitations

**Reference:** See `FUTURE_ENHANCEMENTS.md` and `ROADMAP.md` Phase 5-6 sections

**Phase 4 Pending:**
- Full ingest+query roundtrip tests (awaiting private plugin finalization)
- Edition-gate negative tests
- Performance benchmarks (Phase 5)

**Out of Scope (Phase 2+):**
- Custom NLP model training or fine-tuning
- Non-Wikipedia knowledge sources (may be added as plugins)
- Real-time collaboration or multi-user sessions

---

## Dependency Graph

**Depends On:**
- `llm` module — LLM inference backend
- `retrieval` module — Vector search (optional, for semantic retrieval)
- `utils` module — String utilities, JSON serialization
- RocksDB (optional; for Phase B persistent cache)

**Depended By:**
- API layer (`api/grpc/`) — Exposes LLM Wiki as gRPC service
- Enterprise plugins — Build on public ILLMWikiPlugin interface

---

## Build & Integration

**CMake Targets:**
- `themis_llm_wiki_plugin` — Public SDK library
- `module_llm_wiki_*_focused` — Focused test targets (Phase 4+)
- `bench_llm_wiki_plugin` — Performance benchmarks (Phase 5+)

**Edition Gating:**
- Community: No LLM Wiki (feature not included)
- Enterprise+: Full LLM Wiki with guardrails + workspace isolation

**Feature Flags:**
- `THEMISDB_WIKI_PHASE_B` — Enables persistent cache via RocksDB (Phase B; gated on RocksDB availability)

---

**Last Updated:** 2026-08-10  
**Next Review:** 2026-08-31 (Phase 4 completion validation)
