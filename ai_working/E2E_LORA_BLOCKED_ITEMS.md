---
Author: "ThemisDB Team"
Created: "2026-11-10"
Last Updated: "2026-11-10"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot (Auto mode; exact model backend not verifiable)"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# E2E LoRA pipeline — blocked items (human hand-off)

Context: [E2E_LORA_PIPELINE_PLAN.md](E2E_LORA_PIPELINE_PLAN.md). The items below could not be
completed because the session permission layer denied the required file writes (including
reverts). Each item lists the current tree state, the build impact and the exact remaining work,
so that a maintainer can finish it or authorise an agent to do so.

| ID | File(s) | Tree state | Build impact | Decision needed |
|----|---------|-----------|--------------|-----------------|
| B | `src/training/ada_lora_adapter.cpp` | Header changed, `.cpp` unchanged | **Link errors** for all new header methods; behaviour of existing methods unchanged | Authorise edit of the `.cpp` (recommended) or revert `include/training/ada_lora_adapter.h` |
| E | `src/llm/continuous_batch_scheduler.cpp` (+ `.h`) | Half-edited | **Link errors** (`stepTokens`, `eraseActive`); decode requests stall (`in_flight` never cleared) | Authorise completion (option A) or revert both files (option B) |
| T | `tests/llm/CMakeLists.txt` | Unchanged | New test only registered via `test_*.cpp` glob | Authorise explicit target block |
| P | Defaults of `AdapterSignaturePolicy` | All `true` (fail-closed) | Training/hot-load reject unsigned adapters until signer/verifier are wired | Intentional; keep unless governance decides otherwise |
| L | `src/llm/llama_lora_adapter.cpp` (shim) | Still exports old `llama_lora_adapter_*` names and test hooks | Dead legacy surface; serving path no longer uses it | Authorise rewrite/removal |
| G | `tests/llm/test_gpu_lora_integration.cpp` (~305) | `EXPECT_EQ(caps.supports_lora, themis_llama_lora_available())` | **Test fails**: `supports_lora` is now true whenever the manager exists | Authorise test change |
| S | `include/server/http_server.h` (+ `http_server.cpp`, `main_server.cpp`) | Unchanged | LoRA E2E path not reachable from the running server; inline handler still returns a fabricated "queued" job | Authorise edit of `http_server.h` |

---

## B — AdaLoRA implementation (`src/training/ada_lora_adapter.cpp`)

### State

`include/training/ada_lora_adapter.h` already declares the new API and the v2 checkpoint
format; `src/training/ada_lora_adapter.cpp` still contains the v1 implementation. Undefined
symbols: `updateImportanceFromGradients`, `setSensitivityBetas`, `sensitivityBetas`,
`getTripletScores`, `setBudgetSchedule`, `budgetAt`, `orthogonalityPenalty`,
`orthogonalityPenaltyGradients`; `AdaLoRALayerStats::alpha` is never populated.

### Required implementation (reference: Zhang et al., ICLR 2023, arXiv:2303.10512)

Layer layout is unchanged: `B` is `in_dim × max_rank` (row-major, index `d*max_rank+i`),
`A` is `max_rank × out_dim`. No separate Λ.

1. **Layer state:** add `triplet_scores[max_rank]`, `Ī/Ū` for B and A (allocated lazily on
   first gradient update), `score_source` (0 none, 1 magnitude, 2 sensitivity),
   `sensitivity_steps`. `importance` = mean of triplet scores over `max_rank`.
2. **Magnitude fallback** (`updateImportance`): `S_i = ||B[:,i]||² · ||A[i,:]||²`.
3. **Sensitivity** (`updateImportanceFromGradients(layer, grad_B, grad_A)`):
   validate sizes and finiteness (`std::invalid_argument`), then per weight
   `I = |w·g|`, `Ī = β1·Ī + (1−β1)·I`, `Ū = β2·Ū + (1−β2)·|I−Ī|`, and
   `S_i = mean_d Ī·Ū(B[d,i]) + mean_d Ī·Ū(A[i,d])`.
4. **Betas:** `setSensitivityBetas` accepts `[0,1)` only; default `0.85/0.85`.
5. **`reallocateRanks(budget)`:**
   - throw `std::invalid_argument` for `budget == 0`, for `budget < layerCount()` and for
     `budget > Σ max_rank` (empty adapter: return an empty successful result);
   - every layer keeps its best triplet; remaining slots are ranked globally by score desc,
     then position-in-layer asc, then layer insertion order asc (equal scores ⇒ round robin,
     e.g. 3 layers, budget 5 ⇒ 2/2/1);
   - physically permute B columns, A rows, scores and EMA state so the active components are
     `[0, active_rank)` — `src/training/adalora_tt_bridge.cpp` relies on this prefix;
   - `ReallocResult.total_active_rank == budget`.
6. **Cubic schedule:** `setBudgetSchedule(b0, bT, ti, tf)` requires `bT > 0`, `b0 ≥ bT`;
   `budgetAt(t, T)` throws `std::logic_error` if not configured and `std::invalid_argument`
   if `T ≤ ti+tf`; returns `b0` for `t < ti`, `bT` for `t ≥ T−tf`, otherwise
   `floor(bT + (b0−bT)·(1 − (t−ti)/(T−ti−tf))³)`.
7. **Orthogonality regulariser:** `R = ||BᵀB−I||²_F + ||AAᵀ−I||²_F`; gradients
   `∂R/∂B = 4·B·(BᵀB−I)`, `∂R/∂A = 4·(AAᵀ−I)·A`.
8. **`getLayerStats`** fills `alpha`; **`forward`** keeps using `alpha / max_rank` and only the
   active prefix.
9. **Persistence v2:** exactly as specified in the header (`saveToFile` doc comment).
   `loadFromFile` parses into a fresh `Impl` and swaps only on success; validate shapes
   (`active_rank ≤ max_rank`, dims > 0, name length ≤ 4096, no duplicates), alpha > 0,
   betas in `[0,1)`, and that tensor sizes fit the remaining file size. Reject every version
   other than 2. `isCacheValid` must accept version 2 only.
10. **Optional (needs header declaration):** `toMetadata()` returning
    `themis.adapter.{method, rank, rank_pattern}` and
    `themis.training.adalora.{init_r, target_r, tinit, tfinal, deltaT, beta1, beta2,
    orth_reg_weight}` for the GGUF/safetensors metadata; `setReallocationInterval`,
    `setOrthogonalityRegWeight`. If these are added, extend the v2 header fields accordingly
    (float `orth_reg_weight`, uint64 `delta_t` after β2) and update the format doc.
11. **Initialisation:** keep deterministic B init seeded from the layer name hash (test
    `AddLayerDeterministicInitializationByName`).

### Tests to adjust / add

- `tests/test_ada_lora_adapter.cpp` and `tests/legacy/ada/test_ada_lora_adapter.cpp`:
  `ReallocRespectsMaxRank` uses `reallocateRanks(100)` with Σmax = 5 ⇒ change to
  `EXPECT_THROW(..., std::invalid_argument)` plus a valid `reallocateRanks(5)` case.
- `tests/llm/test_llm_inference_roles.cpp` (~line 154): `reallocateRanks(4)` on a layer with
  max 3 ⇒ use 3 and add a throw check for 4.
- `tests/training/test_adalora_edge_regressions.cpp`: add hand-computed cases
  - β1=β2=0.5, B={1,2}, A={3}, all gradients 1 ⇒ triplet score 1.4375 after step 1,
    2.15625 after step 2;
  - b0=24, bT=12, ti=10, tf=20, T=110 ⇒ `budgetAt(50) == 13`;
  - B=diag(2,1), A=0 (r=2, in=2, out=1) ⇒ penalty 11, `∂R/∂B = {24,0,0,0}`;
  - alpha/active_rank/scores round-trip through `saveToFile`/`loadFromFile`; v1 file rejected.
- Unchanged and still valid: budgets in `tests/llm/test_llm_adalora_doku_training.cpp`
  (12, 6, 20, 8), cross-module test (8 for 2×4).

### Docs

`src/training/ARCHITECTURE.md`: section "AdaLoRA rank allocation" (scores, schedule,
regulariser, v2 format, invariant "active components are a prefix").

---

## E — Continuous batching (`src/llm/continuous_batch_scheduler.*`)

### State

`include/llm/continuous_batch_scheduler.h` is consistent (adds `max_active_sequences`,
`ScheduledRequest::in_flight`, `StepReport`, `submitRequest(..., exact_prompt_tokens = 0)`,
a 3-argument `processBatchResults`, private `stepTokens`, `eraseActive`,
`cancelled_in_flight_`). `src/llm/continuous_batch_scheduler.cpp` is half-edited:
exact prompt tokens, revised `cancelRequest`, chunked-prefill continuation, `in_flight`
marking and `max_active_sequences` are applied; the KV-exhaustion guard is unchanged.

### Remaining work (option A)

1. Define:
   ```cpp
   size_t ContinuousBatchScheduler::stepTokens(const ScheduledRequest* r) const;
   // DECODE ⇒ 1; PREFILL ⇒ remaining prompt tokens, capped by
   // effective_prefill_chunk_size_ when enable_chunked_prefill.
   void ContinuousBatchScheduler::eraseActive(const ScheduledRequest* r);
   // remove r from active_requests_ by pointer identity.
   ```
2. Clear `in_flight` in `processBatchResults` (both overloads); skip finished requests;
   release `cancelled_in_flight_` entries after their step completes.
3. Implement the `StepReport` overload (tokens produced, finished flag, error) used by the
   executor; the legacy 2-argument overload keeps its documented semantics.
4. `canAddToBatch()` must count `stepTokens()` instead of the full prompt.
5. Keep the KV guard; size executor KV accounting with `n_seq_max + 1` spare blocks above
   `n_ctx / 16` so that free blocks never reach 0 while sequences are running.
6. Then the rest of package E: executor thread (adapter-grouped `llama_batch` with multiple
   `seq_id`, `llama_set_adapter_lora`/`llama_clear_adapter_lora` per group, per-sequence
   sampler chains, `llama_memory_seq_rm` on finish/cancel, decode-error isolation),
   production caller from `LlamaWrapper::generate`, gtest for pure scheduling logic.
7. **Open finding:** the response cache key is still `prompt|model_id`; it must include the
   adapter id(s) and scale(s), otherwise responses of different adapters are mixed.

### Revert (option B)

```bash
git checkout -- src/llm/continuous_batch_scheduler.cpp include/llm/continuous_batch_scheduler.h
```
Neither file had sibling changes, so nothing else is lost.

---

## T — Test registration (`tests/llm/CMakeLists.txt`)

`tests/llm/test_lora_training_snapshot_provenance.cpp` is only picked up by the `test_*.cpp`
glob. Add an explicit focused target that links `themis_core` (or the modular target that
contains `src/storage/mvcc_snapshot_pins.cpp`, `src/llm/lora_framework/training_snapshot.cpp`,
`src/llm/lora_framework/adapter_provenance.cpp`, `lora_safetensors_package.cpp`,
`lora_dataset_loader.cpp`) and OpenSSL, following the `MultiLoRAManager` pattern
(explicit `themis_llm` link). None of the new C++ tests have been compiled or run in this
session (no CMake/gtest available).

---

## P — Signature policy defaults

`AdapterSignaturePolicy{require_signed_imports, require_signed_hot_load,
require_signed_training}` default to `true`. This is the governance-correct setting for
tamper-evident training-data snapshot provenance; a request to lower the defaults was denied
and is not recommended. Consequence: until `src/main_server.cpp` wires a configured signing key
and trusted verifier keys, training jobs fail with an explicit reason and unsigned adapters are
rejected on import and hot-load (wiring in progress, package D follow-up).

---

## L / G — Legacy LoRA shim and GPU integration test

- `src/llm/llama_lora_adapter.cpp` (99 lines) still exports the pre-b7974 symbols
  (`llama_lora_adapter_*`) and test hooks. The serving path (`MultiLoRAManager`,
  `LlamaWrapper`) uses only `llama_adapter_lora_init`, `llama_set_adapter_lora`,
  `llama_rm_adapter_lora`. Remaining work: delete the shim (and its CMake entry) or reduce it to
  the GGUF metadata contract declared in `include/llm/llama_lora_adapter.h`; adjust
  `tests/llm/test_gpu_lora_integration.cpp` lines ~31 and ~386 that test the shim symbols.
- `tests/llm/test_gpu_lora_integration.cpp` ~line 305: replace
  `EXPECT_EQ(caps.supports_lora, themis_llama_lora_available())` with
  `EXPECT_TRUE(caps.supports_lora)` (LoRA support now means "manager exists"; adapters are
  hot-loaded via the b7974 API).
- Generated/historical docs still mention old names (`src/llm/DOXYGEN.md`, `src/AUDIT.md`,
  `src/STUB_INVENTORY.md`); regenerate after the shim decision.
