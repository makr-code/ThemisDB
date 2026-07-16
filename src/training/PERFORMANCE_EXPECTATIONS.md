# PERFORMANCE_EXPECTATIONS - src/training

## Scope

- Module: src/training
- This file defines measurable training module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_gpu_training_cycle.cpp
  - benchmarks/bench_lora_training.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| TRNP-1 | training cycle CPU/GPU capability paths remain bounded | BM_GPUTrainingCycle_GPUDisabled, BM_TrainingCycle_CPU_Baseline, BM_TrainingCycle_CUDA, BM_TrainingCycle_HIP, BM_TrainingCycle_Vulkan, BM_CompleteTrainingStep_CUDA |
| TRNP-2 | LoRA layer construction and forward/backward paths remain bounded | BM_LoRALayer_Construction, BM_LoRALayer_Forward, BM_LoRALayer_Backward, BM_LoRALayer_RankImpact |
| TRNP-3 | attention and sequential LoRA composition paths remain bounded | BM_AttentionLoRA_Construction, BM_AttentionLoRA_Forward, BM_AttentionLoRA_Backward, BM_Sequential_Construction, BM_Sequential_Forward, BM_Sequential_Backward, BM_CompositePattern_Overhead |
| TRNP-4 | parameter and memory efficiency paths remain bounded | BM_LoRALayer_ParameterCount, BM_LoRALayer_ParameterAccess, BM_Sequential_ParameterCollection, BM_LoRALayer_MemoryUsage, BM_Compare_LoRAvsFullFinetuning |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| TRNG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| TRNG-2 | training hot-path p99 <= release threshold | p99 from mapped training benchmark cases |
| TRNG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional training benchmark scenarios are introduced.

## Sourcecode Verification (Module: training/performance)

- Verified benchmark sources:
  - benchmarks/bench_gpu_training_cycle.cpp
  - benchmarks/bench_lora_training.cpp
- Verified mapping surfaces:
  - training cycle, LoRA layer, adapter composition, and parameter-efficiency behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
