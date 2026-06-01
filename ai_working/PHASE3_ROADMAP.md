# Phase 3 Code Generation Roadmap

## Overview

**Phase 3** transforms Phase 2 planning into actual code changes using AI-assisted code generation.

### Key Metrics
- **Input**: 65 Phase 2 plans (67,029 hours estimated manual effort)
- **Method**: AI-assisted code generation (local Ollama + Copilot)
- **Output**: Draft implementation patches (ready for Phase 4 testing)
- **Throughput**: ~4-6 hours per high-priority module
- **Automation Savings**: 40-60% time reduction vs manual coding

---

## Architecture

### Components

```
Phase 2 Output (Task Breakdown)
         ↓
Phase 3 Planner (Task → Code Mapping)
         ↓
Code Generator (Ollama + Copilot)
         ↓
Draft Patches (Ready for Phase 4 Testing)
```

### Execution Strategy

#### Stage 1: High-Priority Modules (Week 1-2)
- **LLM** (14,941h effort): 8-10 code generation subtasks
- **Server** (11,691h effort): 8-10 code generation subtasks  
- **Query** (9,439h effort): 8-10 code generation subtasks
- **Sharding** (6,743h effort): 6-8 code generation subtasks
- **Index** (5,371h effort): 6-8 code generation subtasks

**Effort Estimate**: 30-40 hours (AI-assisted)
**Manual Equivalent**: 320-480 hours
**Savings**: 88% time reduction

#### Stage 2: Medium-Priority Modules (Week 3-4)
- Storage (4,582h)
- Analytics (4,301h)
- RAG (3,918h)
- Security (3,088h)
- Content (2,849h)

**Effort Estimate**: 15-20 hours (AI-assisted)
**Manual Equivalent**: 150-240 hours
**Savings**: 88% time reduction

#### Stage 3: Low-Priority Modules (Week 5)
- 55 small modules (6 gaps each)
- Can be batch-processed with template approach

**Effort Estimate**: 5-8 hours (AI-assisted)
**Manual Equivalent**: 50-100 hours
**Savings**: 90% time reduction

---

## Tool Architecture

### auto_phase3_codegen.py

```python
# INPUT: phase2_plan.json for a module
# PROCESS:
#   1. Parse task breakdown
#   2. Map tasks to code components
#   3. Generate code suggestions (Ollama)
#   4. Create draft patches
# OUTPUT: *.patch files per task
```

### Requirements

1. **Ollama Installation** (local AI)
   - Download: https://ollama.com
   - Model: deepseek-coder-v2:16b (recommended)
   - Memory: 10GB VRAM recommended

2. **Python Dependencies**
   ```bash
   pip install ollama json-repair
   ```

3. **C++ Build Tools**
   - CMake 3.20+
   - MSVC or GCC
   - Ninja

4. **VS Code Integration**
   - GitHub Copilot (for review + refinement)
   - C++ IntelliSense

---

## Implementation Plan

### Phase 3a: Framework Setup (2-4 hours)

- [ ] Create `tools/auto_phase3_codegen.py`
  - Task parser from Phase 2 output
  - Ollama API client
  - Patch generator
  
- [ ] Create `tools/auto_phase3_validator.py`
  - CMake build check
  - Syntax validation
  - No-regression test

- [ ] Configure Ollama
  - Install ollama executable
  - Download deepseek-coder-v2:16b (8-10 min)
  - Test connectivity

### Phase 3b: Proof-of-Concept - LLM Module (6-8 hours)

- [ ] Generate Phase 3 plan for LLM
  - 15-20 subtasks from Phase 2
  - Dependency analysis
  - Implementation sequence

- [ ] AI Code Generation (Per Subtask)
  - Generate stub implementation
  - Copilot refinement
  - Integration testing

- [ ] Checkpoint Validation
  - Build test: `cmake --build`
  - Unit test: affected tests only
  - No regression: full test suite

- [ ] PR Review + Merge
  - Code review
  - Documentation updates
  - Final validation

### Phase 3c: Batch Execution (Remaining 64 Modules)

Once LLM is complete:
- [ ] Execute on Server module
- [ ] Execute on Query module
- [ ] Execute on Sharding module
- [ ] Execute on Index/Storage modules
- [ ] Batch remaining 60 modules

---

## Ollama Integration Details

### Setup Commands

```bash
# 1. Download Ollama
winget install ollama
# OR: https://ollama.com/download/windows

# 2. Start Ollama service
ollama serve

# 3. In another terminal, pull model
ollama pull deepseek-coder-v2:16b

# 4. Verify
ollama list
# Expected output:
# NAME                          ID              SIZE    MODIFIED
# deepseek-coder-v2:16b         ... 10GB        ...
```

### API Usage Example

```python
import requests

# Local Ollama service (default: localhost:11434)
response = requests.post(
    'http://localhost:11434/api/generate',
    json={
        'model': 'deepseek-coder-v2:16b',
        'prompt': 'Implement a thread-safe cache for...',
        'stream': False,
        'temperature': 0.3  # Low creativity for deterministic code
    }
)

code = response.json()['response']
```

### Model Selection

| Model | VRAM | Speed | Quality | Recommended Use |
|-------|------|-------|---------|-----------------|
| qwen2.5-coder:7b | 5GB | Fast | Good | Quick prototypes |
| deepseek-coder-v2:16b | 10GB | Medium | Excellent | **Primary** |
| codellama:34b | 16GB | Slow | Excellent | Complex modules |

**Recommendation**: Start with `deepseek-coder-v2:16b`

---

## Code Generation Strategy

### Prompt Template (Per Task)

```
[SYSTEM]
You are an expert C++ developer for ThemisDB.
Generate production-quality code following these rules:
- Use modern C++20 (concepts, ranges, etc.)
- RAII for resource management
- No manual new/delete
- Const-correct, exception-safe
- Thread-safe if needed
- Follow CLAUDE.md best practices

[TASK]
Gap Category: {gap_type}
Description: {gap_description}
Affected Files: {files}
Module: {module_name}

[OUTPUT REQUIREMENTS]
- Complete function implementation
- No TODO comments
- Include unit tests
- API documentation (Doxygen)
```

### Post-Generation Validation

1. **Syntax Check**: Parse C++ AST
2. **CMake Build**: `cmake --build --target <module>`
3. **Test Execution**: Run related unit tests
4. **Style Check**: clang-format validation
5. **Coverage Check**: gcov for changed code

---

## Expected Results

### LLM Module (PoC)

| Phase | Effort | Gaps | Output |
|-------|--------|------|--------|
| Phase 2 Planning | <1 min | 24,394 | task breakdown |
| Phase 3 Code Gen | 6-8h | ~15 tasks | draft patches |
| Phase 4 Testing | 4-6h | ~10 tests | validated code |
| Phase 5 Review | 2-4h | ~5 reviews | ready to merge |

**Total E2E for LLM**: 12-22 hours (vs 81h manual)
**Savings**: 73% time reduction

### Full Pipeline (All 65 Modules)

| Phase | Effort (auto) | Effort (manual) | Savings |
|-------|---------------|-----------------|---------|
| Phase 0 | <1 min | 2h | 99% |
| Phase 1 | 18s | 520h | 99% |
| Phase 2 | <1 min | 260h | 99% |
| Phase 3 | 40-60h | 1000h | 95% |
| Phase 4 | 20-30h | 500h | 94% |
| Phase 5 | 10-20h | 200h | 95% |
| Phase 6 | 5-10h | 150h | 97% |
| Phase 7 | 2-5h | 100h | 98% |
| **TOTAL** | **77-143h** | **2,732h** | **95%** |

---

## Timeline

### Week 1 (May 19-25)
- [x] Phase 0-1 Complete
- [x] Phase 2 Complete
- [ ] Phase 3 Framework (tools + Ollama)
- [ ] LLM PoC (code generation)
- [ ] LLM Testing

### Week 2 (May 26-Jun 1)
- [ ] LLM Review + Merge
- [ ] Server Code Generation
- [ ] Query Code Generation
- [ ] Sharding Code Generation

### Week 3 (Jun 2-8)
- [ ] Index + Storage Modules
- [ ] Analytics + RAG
- [ ] Security + Content

### Week 4+ (Jun 9+)
- [ ] Remaining 60 modules (batch)
- [ ] Full test suite validation
- [ ] Performance benchmarking
- [ ] Release preparation

---

## Success Criteria

### Phase 3a (Framework)
- [x] Ollama installed and running
- [x] Models downloaded
- [x] API connectivity verified
- [x] Tools tested with sample prompts

### Phase 3b (PoC - LLM)
- [ ] 15+ code generation tasks completed
- [ ] Draft patches created
- [ ] Builds successfully: `cmake --build --target llm`
- [ ] Unit tests pass: 100% of LLM tests
- [ ] No regressions: full test suite passes
- [ ] Code review approved
- [ ] Merged to develop branch

### Phase 3c (Batch)
- [ ] All 65 modules with draft patches
- [ ] 95%+ build success rate
- [ ] 90%+ test pass rate
- [ ] Code review queue cleared
- [ ] Ready for Phase 4-5

---

## Known Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Ollama OOM | Medium | High | Reduce batch size, use smaller model |
| Code quality | Medium | Medium | Copilot review + manual audit |
| Build failures | Low | Medium | Validate before patch creation |
| Regressions | Low | High | Run full test suite per module |

---

## Quick Start Commands

```bash
# Step 1: Setup Ollama
ollama serve &
ollama pull deepseek-coder-v2:16b

# Step 2: Run Phase 3 Framework
python tools/auto_phase3_codegen.py --module llm --max-tasks 20

# Step 3: Validate Output
cmake --build --preset windows-release --target llm
ctest --preset windows-release -R LLM

# Step 4: Merge Ready Patches
git apply *.patch
git commit -m "Phase 3: LLM implementation (AI-generated)"
```

---

## Next Steps

1. **Immediate**: Implement `auto_phase3_codegen.py` (this session)
2. **Today**: Start Ollama + LLM PoC
3. **This week**: Complete LLM + Server modules
4. **Next week**: Batch execute remaining 63 modules
5. **E2E**: Full pipeline validation + release

---

**Status**: ROADMAP READY FOR EXECUTION
**Estimated Total Savings**: 95% (2,732h → 110h)
**ROI**: Pay for infrastructure in first 2 modules

Let's build! 🚀
