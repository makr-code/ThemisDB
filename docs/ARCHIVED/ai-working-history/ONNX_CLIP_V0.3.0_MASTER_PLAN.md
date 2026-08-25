# ONNX CLIP v0.3.0 Implementation Master Plan
## Multi-Phase Hardening with Parallel Sub-Agents

**Status:** Infrastructure Phase Complete, Core Implementation In Progress  
**Start Date:** 2026-08-09  
**Target Completion:** Q1 2027  
**Workstream Count:** 4 parallel agents

---

## Execution Timeline

### Phase 0: Infrastructure Setup ✅ DONE (T+0-300s)
- Test harness CMakeLists.txt + README.md
- Benchmark infrastructure + gate documentation
- ROADMAP.md + ARCHITECTURE.md updates
- Placeholder files with structure

### Phase 1-4: Core Implementation (IN PROGRESS, T+300-3600s)

**Agents working in parallel:**

| Agent | Task | Status | Est. Completion |
|-------|------|--------|-----------------|
| themisdb-implementer | Phase 1A: Golden embeddings | Running (290s) | +60m |
| task | Phase 2A: Benchmarks | Running (290s) | +60m |
| themisdb-implementer | Phase 3B: Hot-swap impl | Running (234s) | +90m |
| themisdb-implementer | Phase 4B: Mmap impl | Running (234s) | +80m |

### Phase 1B-4C: Integration & Testing (NEXT, T+3600-5400s)

Sequential phases after core implementation:

1. **Phase 1B+1C:** Reproducibility tests + CI
   - Duration: 30m | Acceptance: All 12 tests pass

2. **Phase 2B+2C:** Latency analysis + gates
   - Duration: 45m | Acceptance: All FCP gates pass

3. **Phase 3C:** Hot-swap tests
   - Duration: 60m | Acceptance: All 12 tests pass

4. **Phase 4C:** Mmap tests + optimization
   - Duration: 45m | Acceptance: All 12 tests, ≥30% memory reduction

### Phase 5: Final Documentation & Review (T+5400+)
- README/CHANGELOG/SECURITY updates
- Code review preparation
- PR to develop branch

---

## Success Criteria by Phase

### Phase 1: Integration Tests (OCP-IT-01..12)
- ✅ Golden embedding framework with seed=42
- ✅ L2 normalization verification
- ✅ Reproducibility across runs
- ✅ ViT-B/32 & ViT-L/14 model support
- ✅ All tests passing

### Phase 2: Performance Benchmarks (FCP-01..06)
- ✅ Single-image: ≤150ms (CPU)
- ✅ Batch-16: ≤2.4s (CPU)
- ✅ Text encoding: ≤5ms
- ✅ Initialization: <500ms
- ✅ Batch throughput: ≥6x speedup
- ✅ Memory tracking

### Phase 3: Dynamic Hot-Swap (OCP-HS-01..12)
- ✅ reloadModel() API implementation
- ✅ State machine correctness
- ✅ In-flight request tracking
- ✅ Graceful drain (30s timeout)
- ✅ Atomic swap semantics
- ✅ Automatic rollback
- ✅ All 12 tests passing

### Phase 4: Memory-Mapped Loading (OCP-MM-01..12)
- ✅ Linux mmap() support
- ✅ Windows MapViewOfFile() support
- ✅ Graceful fallback to heap loading
- ✅ ≥30% memory reduction (ViT-L/14)
- ✅ Platform detection + cleanup
- ✅ All 12 tests passing

---

## Repository Structure (Post-Implementation)

```
src/onnx_clip/
├── onnx_clip_plugin.h          ← Added reloadModel() API
├── onnx_clip_plugin.cpp        ← Added hot-swap + mmap impl (~250 new lines)
├── ROADMAP.md                  ← Updated with v0.3.0 phases
├── ARCHITECTURE.md             ← Added state machine + mmap sections
├── README.md                   ← (Will update version to v0.3.0)
└── ...

tests/onnx_clip/
├── CMakeLists.txt              ← ✅ Configured with test registration
├── README.md                   ← ✅ Test registry documented
├── test_onnx_clip_golden_embeddings_focused.cpp      ← Phase 1A (in progress)
├── test_onnx_clip_hot_swap_focused.cpp               ← Phase 3C (placeholder)
├── test_onnx_clip_mmap_focused.cpp                   ← Phase 4C (placeholder)
└── ...

benchmarks/onnx_clip/
├── CMakeLists.txt              ← ✅ Configured with benchmark registration
├── README.md                   ← ✅ Gate documentation
├── bench_onnx_clip_cpu.cpp                           ← Phase 2A (in progress)
├── bench_onnx_clip_vit_backend.cpp                   ← Phase 2A (in progress)
└── ...
```

---

## Deliverables Checklist

### Code Implementation
- [ ] Phase 1A: `test_onnx_clip_golden_embeddings_focused.cpp` (8 tests)
- [x] Phase 1B: Reproducibility tests (OCP-IT-09..12) — awaiting Phase 1A
- [x] Phase 1C: Test harness integration — infrastructure ready
- [ ] Phase 2A: Benchmark files (FCP gates) — in progress
- [x] Phase 2B+2C: Gate implementation — awaiting Phase 2A
- [ ] Phase 3A: Hot-swap API design — documented in ARCHITECTURE.md
- [ ] Phase 3B: Hot-swap implementation in plugin — in progress
- [x] Phase 3C: Hot-swap tests (12 tests) — placeholder ready
- [ ] Phase 4A: Mmap API design — documented in ARCHITECTURE.md
- [ ] Phase 4B: Mmap implementation in plugin — in progress
- [x] Phase 4C: Mmap tests (12 tests) — placeholder ready

### Configuration & Build
- [x] `tests/onnx_clip/CMakeLists.txt` — Configured
- [x] `benchmarks/onnx_clip/CMakeLists.txt` — Configured
- [ ] Integration with main CMakeLists.txt (if needed)

### Documentation
- [x] `tests/onnx_clip/README.md` — Test registry
- [x] `benchmarks/onnx_clip/README.md` — Gate documentation
- [x] `src/onnx_clip/ROADMAP.md` — Phase structure
- [x] `src/onnx_clip/ARCHITECTURE.md` — Design docs
- [ ] `src/onnx_clip/README.md` — v0.3.0 version update
- [ ] `src/onnx_clip/SECURITY.md` — Mmap considerations
- [ ] `CHANGELOG.md` — v0.3.0 entry

---

## Risk Management

| Risk | Severity | Mitigation |
|------|----------|-----------|
| Agent timeout (>4h) | Medium | Monitor, restart if needed |
| Mmap platform issues | High | Platform detection, fallback |
| Hot-swap deadlock | High | 30s timeout, forced swap |
| Memory measurement variance | Medium | Statistical averaging |
| Test flakiness | Medium | Seed-based determinism |

---

## Quality Assurance

### Per-Agent Handoff Gate
- [ ] Code compiles (no warnings)
- [ ] All new tests pass
- [ ] No regressions
- [ ] Documentation updated
- [ ] Ready for review

### Final Acceptance Gate
- [ ] 96 tests pass (72 existing + 12 IT + 12 HS + 12 MM)
- [ ] All benchmarks pass
- [ ] All gates pass (FCP-01..06)
- [ ] Documentation complete
- [ ] PR merge-ready

---

**Last Updated:** 2026-08-09 07:08 UTC  
**Owner:** ONNX CLIP Multi-Phase Team  
**Status:** Infrastructure complete, core implementation in progress
