# Wave A Q3 Module Owner Quick-Start Guides
## Per-Module Rapid Onboarding (Sept 3-4, 2026)

---

## Module: SERVER
### Owner Onboarding (30 min)

**Your Role**: Implement + validate 8 new ACs (AC-S1/2/4/7/8) + maintain AC-S3/5/6 baseline

**What You Own**:
- HTTP timeout determinism (8 tests)
- Graceful shutdown drain (6 tests)
- Idempotency cache snapshot (5 tests)
- Voice API auth + fail-closed semantics (11 tests)
- **Total**: 30 new tests + 32 existing baseline tests = 62 tests

**Critical Dependencies**:
- ⏳ None (soft dependency on STORAGE AC-ST5 remote backup; can mock until Sept 15)
- ✅ Your tests can start immediately (Sept 3-4)

**Action Plan (Sept 3-4)**:
1. [ ] Read: `ai_working/WAVE_A_Q3_MODULES_DETAILED_STAGING_2026_09_02.md` § SERVER (5 min)
2. [ ] Assign tests to team members: 
   - AC-S1 (8 tests): Junior engineer (timing-heavy)
   - AC-S2 (6 tests): Shutdown expert
   - AC-S4 (5 tests): Concurrency expert
   - AC-S7/8 (11 tests): Security engineer
3. [ ] Create test stubs: `tests/server/test_server_http_timeout_determinism.cpp` (copy template from ROADMAP)
4. [ ] Team sync: Mondays 09:00 UTC (Sept 8, 15, 22, 29)
5. [ ] Submit PR: Sept 20 (partial results + evidence assembly starts Sept 22)

**Build Verification (Sept 4-5)**:
```bash
# After TRANSACTION verification completes, run:
cmake --preset community-release -DTHEMIS_BUILD_TESTS=ON -B /tmp/themis-build
cmake --build /tmp/themis-build --target test_server_http_timeout_determinism --parallel 4
ctest --build-dir /tmp/themis-build -R "TimeoutDeterminism|GracefulShutdown" -V
```

**Evidence Bundle Template**:
```markdown
# SERVER Module — Wave A Closure Evidence Bundle
**Target**: Sept 20, 2026

## AC-S1: HTTP Timeout Enforcement
- [x] All 8 tests passing
- [x] Clock drift ±100ms validated
- [x] SLA accuracy ±50ms confirmed
- [ ] Chaos evidence (network partition, clock skew)
- [ ] Risk assessment + sign-off

[... repeat for AC-S2/4/7/8 ...]
```

**Blockers? Escalate to @makr-code**:
- Missing libcurl headers → Install libcurl4-openssl-dev
- RocksDB configure failures → Use community-release-allow-missing-rocksdb preset
- Test framework issues → Reach out Sept 3

---

## Module: STORAGE
### Owner Onboarding (40 min)

**Your Role**: Implement 4 new ACs (AC-ST3/5/7/8) + maintain AC-ST1/2/4/6 baseline

**What You Own**:
- GGML context binding (6 tests)
- Remote S3/GCS/Azure manifest (8 tests)
- WAL replay determinism 50x (7 tests)
- MVCC snapshot isolation (8 tests)
- **Total**: 29 new tests + 16 existing baseline tests = 45 tests

**Critical Dependencies**:
- ⏳ **SOFT**: INDEX AC-I2 FTS design approval (Sept 10) blocks AC-ST5 remote backup manifest format
- ✅ Can start AC-ST3/7/8 immediately; AC-ST5 starts Sept 15 after design approval

**Action Plan (Sept 3-4)**:
1. [ ] Read: `WAVE_A_Q3_MODULES_DETAILED_STAGING_2026_09_02.md` § STORAGE
2. [ ] Assign tests:
   - AC-ST3 (6 tests): GGML/llvm expert (ggml_context* bindings)
   - AC-ST7 (7 tests): WAL/crash recovery expert (50x replay)
   - AC-ST8 (8 tests): MVCC/concurrency expert
   - AC-ST5 (8 tests): Hold until INDEX AC-I2 approved (Sept 10)
3. [ ] Create test stubs: `tests/storage/test_storage_ggml_context_binding.cpp`
4. [ ] **WAIT for AC-I2 approval** before starting AC-ST5
5. [ ] Team sync: Mondays 09:00 UTC starting Sept 8

**Build Verification (Sept 4-5)**:
```bash
# After TRANSACTION verification:
cmake --build /tmp/themis-build --target test_storage_ggml_context_binding --parallel 4
cmake --build /tmp/themis-build --target test_storage_wal_replay_determinism --parallel 4
ctest --build-dir /tmp/themis-build -R "GgmlContext|WalReplay|MvccSnapshot" -V
```

**Evidence Bundle Structure**:
```markdown
# STORAGE Module — Wave A Closure Evidence Bundle
**Target**: Sept 20, 2026

## AC-ST3: GGML Context Binding
- [ ] 6 tests passing
- [ ] ggml_context* forwarding verified
- [ ] Type registration once-only confirmed
- [ ] Determinism: 10x replay identical state

## AC-ST5: Remote Backup Manifest (BLOCKED until AC-I2 Sept 10)
- [ ] 8 tests passing
- [ ] Manifest + blob upload sequence
- [ ] Integrity validation + checksum
- [ ] Retry semantics

[... AC-ST7/8 ...]
```

**Blockers? Escalate**:
- Missing ggml headers → Needs GGML optional dependency
- RocksDB issues → Use diagnostic preset
- AC-I2 design delays → Escalate to steering committee

---

## Module: INDEX
### Owner Onboarding (45 min)

**Your Role**: Implement + review 6 new ACs (AC-I3-I8) + FTS design gate (AC-I2)

**What You Own**:
- Query determinism 50x (10 tests)
- B-Tree concurrency (8 tests)
- Index rebuild consistency (6 tests)
- Range/Bloom/Composite (18 tests)
- **FTS design review** (AC-I2 gate, Sept 3-10)
- **Total**: 42 new tests + 12 FTS parser baseline tests = 54 tests

**CRITICAL DEPENDENCY**:
- 🚨 **AC-I2 FTS Design Approval** (Sept 3-10) — If delayed past Sept 6, escalate!
- All other ACs blocked until AC-I2 → must approve by Sept 10

**Action Plan (Sept 3-4)**:
1. [ ] Read: `ai_working/QUERY_FTS_DESIGN_REVIEW_2026_09.md` (15 min)
2. [ ] **Organize FTS design review committee** (Sept 3-5):
   - Invite: Query optimizer, Parser lead, Executor lead
   - Schedule: Sept 3-4 kickoff, Sept 5-6 feedback, Sept 10 approval vote
3. [ ] Assign tests (Sept 4+):
   - AC-I3 (10 tests): Query determinism expert (50x replay, sort stability)
   - AC-I4 (8 tests): B-Tree concurrency expert
   - AC-I5/6/7/8 (18 tests): Query validation expert
4. [ ] **GATE**: Design review approval required before test implementation
5. [ ] Team sync: Mondays 09:00 UTC (starts Sept 8, after approval)

**FTS Design Review Process (Sept 3-10)**:
```
Sept 3 (Mon): Committee kickoff — review 6-section design spec
Sept 4 (Tue): Feedback collection — comments on interface contracts
Sept 5 (Wed): Revision window — incorporate feedback, final design
Sept 6 (Thu): Final review — steering committee pre-approval
Sept 10 (Mon): APPROVAL GATE — vote + merge to develop
```

**If Design Approval Delayed**:
- Sept 6: Escalate to @makr-code (steering committee)
- Sept 8: Fallback: INDEX starts with traditional query only (AC-I3-I8)
- Sept 15: FTS executor scheduled for Q4 (post-Wave-A)

**Build Verification (Sept 15+, after AC-I2 approved)**:
```bash
cmake --build /tmp/themis-build --target test_index_query_determinism --parallel 4
cmake --build /tmp/themis-build --target test_index_btree_concurrency --parallel 4
ctest --build-dir /tmp/themis-build -R "QueryDeterminism|BTree|IndexRebuild" -V
```

**Evidence Bundle Structure**:
```markdown
# INDEX Module — Wave A Closure Evidence Bundle
**Target**: Sept 20, 2026

## AC-I2: FTS Design Approval ✅ GATE
- [x] Design review committee approved (Sept 10)
- [x] Interface contracts frozen
- [x] Parser ↔ Executor binding finalized
- [x] Architecture document signed off

## AC-I3: Query Determinism
- [ ] 10 tests passing
- [ ] 50x replay identical results
- [ ] Sort stability validated (no floating-point variance >1e-7)

[... AC-I4-I8 ...]
```

**Blockers? Escalate IMMEDIATELY**:
- Design review feedback loop slow → Escalate Sept 4
- Committee disagreement → Escalate Sept 5
- Design approval delayed past Sept 6 → Escalate steering committee Sept 8

---

## Module: LLM
### Owner Onboarding (35 min)

**Your Role**: Implement + validate 6 new ACs (AC-L3-L8) + maintain AC-L1/L2 baseline

**What You Own**:
- CUDA wrapper adoption (12 tests)
- GPU determinism A100/H100 (8 tests)
- Model quantization accuracy (7 tests)
- Token generation + streaming (11 tests)
- Model loading timeout (4 tests)
- **Total**: 42 new tests + 11 existing baseline tests = 53 tests

**Critical Dependencies**:
- 🟡 **Parallel GPU CUDA Audit** (Sept 7-15): Identify wrapper targets
- ✅ Can start AC-L5/6/7/8 immediately (model ops)
- ⏳ AC-L3/4 depend on GPU audit baseline (Sept 15)

**Action Plan (Sept 3-4)**:
1. [ ] Read: `ai_working/WAVE_A_Q3_MODULES_DETAILED_STAGING_2026_09_02.md` § LLM
2. [ ] **Parallel Track: GPU CUDA Audit** (Sept 7-15):
   - Review: `ai_working/GPU_CUDA_AUDIT_BASELINE_2026_09_02.json` (290 unchecked calls)
   - Identify: High-impact wrapper targets (KernelExecutor, TensorAllocator, MemoryPool)
   - Plan: Wrapper adoption roadmap (290 → ≤170 calls)
   - Output: `ai_working/GPU_CUDA_WRAPPER_ADOPTION_PLAN_2026_09_15.md` (due Sept 15)
3. [ ] Assign tests:
   - AC-L5/6/7/8 (29 tests): Model inference + streaming lead (can start immediately)
   - AC-L3 (12 tests): GPU engineer (awaiting wrapper targets from audit)
   - AC-L4 (8 tests): GPU determinism expert (A100/H100 hardware required)
4. [ ] Team sync: Mondays 09:00 UTC starting Sept 8

**Parallel GPU Audit Work (Sept 7-15)**:
```
Sept 7-8: Audit baseline review (GPU_CUDA_AUDIT_BASELINE_2026_09_02.json)
Sept 9-12: High-impact call site analysis + documentation
Sept 13-14: Wrapper adoption planning + sequencing
Sept 15: Wrapper adoption plan due → AC-L3 implementation starts Sept 15
```

**Model Ops Tests (Can Start Immediately)**:
```bash
# AC-L5 Quantization (7 tests)
python3 impl_test_llm_quantization_accuracy.py

# AC-L6 Token Generation (6 tests)
python3 impl_test_llm_token_generation_determinism.py

# AC-L7 Streaming (5 tests)
python3 impl_test_llm_streaming_consistency.py

# AC-L8 Model Loading (4 tests)
python3 impl_test_llm_model_loading_timeout.py
```

**GPU Determinism Caveats (AC-L4)**:
- ⚠️ Requires A100 or H100 GPU hardware
- Fallback: CPU mock baseline (pass/fail only, no determinism proof)
- If A100/H100 unavailable: Document as "hardware constraint" + use CPU baseline

**Build Verification (Sept 15+)**:
```bash
# After GPU audit + wrapper targets identified:
cmake --build /tmp/themis-build --target test_llm_cuda_wrapper_adoption --parallel 4
cmake --build /tmp/themis-build --target test_llm_quantization_accuracy --parallel 4
ctest --build-dir /tmp/themis-build -R "LLM.*" -V
```

**Evidence Bundle Structure**:
```markdown
# LLM Module — Wave A Closure Evidence Bundle
**Target**: Sept 20, 2026

## AC-L3: CUDA Wrapper Adoption
- [x] GPU CUDA audit baseline (Sept 15)
- [x] Wrapper targets identified (KernelExecutor, TensorAllocator, MemoryPool)
- [x] 12 adoption tests passing
- [ ] Determinism 50x replay validated

## AC-L4: GPU Determinism
- [ ] A100/H100 hardware: AVAILABLE | UNAVAILABLE (CPU fallback)
- [ ] 8 tests passing
- [ ] 50x replay: Identical outputs or CPU baseline
- [ ] FP tolerance: ≤1e-5

## AC-L5/6/7/8: Model Operations
- [ ] 29 tests passing (quantization, token generation, streaming, loading)
- [ ] Accuracy loss <2% (quantization)
- [ ] Streaming/batch parity validated
- [ ] Model loading ≤30s SLA

```

**Blockers? Escalate**:
- CUDA toolkit missing → Install libcuda1 + libcudnn8
- GPU hardware unavailable → Use CPU fallback + document
- GPU audit delays past Sept 15 → Escalate to GPU team lead

---

## Cross-Module Synchronization

### Weekly Team Sync (Mondays 09:00 UTC)
**Participants**: MODULE_OWNER + @makr-code (observer)

**Agenda** (30 min):
1. Status update: AC coverage %, tests passing, blockers
2. Blocker triage: Dependency issues, build failures, design feedback
3. Evidence collection: Progress on evidence bundles
4. Next week planning: Task assignments, test execution

**Running Meeting**: Sept 8, 15, 22, 29

---

## Escalation Path

**Issue**: Dependency blocker / Build failure / Design approval delay

**Escalation Steps**:
1. **Day 1**: Report in Slack #themis-wave-a channel
2. **Day 2**: If unresolved, ping @makr-code in issue comment
3. **Day 3**: Escalate to steering committee (if critical path impact)

**Critical Path Issues** (escalate immediately):
- ❌ AC-I2 design approval delay past Sept 6
- ❌ TRANSACTION AC-6 proof miss Sept 10
- ❌ CMake configure failures (build blockers)
- ❌ Test framework regressions

---

## Resources

**Documentation**:
- Wave A program overview: `ROADMAP.md` § Wave A
- Q3 detailed staging: `ai_working/WAVE_A_Q3_MODULES_DETAILED_STAGING_2026_09_02.md`
- TRANSACTION execution: `ai_working/TRANSACTION_AC9_10_5_EXECUTION_REPORT_2026_09_02.md`
- GPU audit baseline: `ai_working/GPU_CUDA_AUDIT_BASELINE_2026_09_02.json`
- QUERY design review: `ai_working/QUERY_FTS_DESIGN_REVIEW_2026_09.md`

**Tools**:
- Build verification: `python3 tools/ci/wave_a_transaction_build_verify.py --all`
- Test runner: `ctest --build-dir /tmp/themis-build --label-regex wave_a`
- Dependency tracker: `python3 tools/ci/wave_a_dependency_tracker.py`

**Contacts**:
- Program Lead: @makr-code
- Build/CI Support: @themis-ci
- Security Review: @security-team

---

## Quick Links

**You are**:
- [SERVER owner?](#module-server)
- [STORAGE owner?](#module-storage)
- [INDEX owner?](#module-index)
- [LLM owner?](#module-llm)

**Your next step**: Jump to your module section above ☝️

**Questions?** Post in #themis-wave-a or comment on WAVE_A_EXECUTION_STATUS PR.

**Timeline summary**:
```
Sept 3-4 : Owner onboarding + task assignment
Sept 4-5 : TRANSACTION build verification (parallel)
Sept 8   : Team sync #1
Sept 10  : TRANSACTION AC-6 proof + INDEX AC-I2 design approval GATE
Sept 15  : Q3 modules implementation starts (after TRANSACTION proof)
Sept 20  : Evidence bundle assembly begins
Sept 30  : All evidence bundles complete + sign-off
Oct 1    : Wave A Q3 closure = COMPLETE ✅
```

