# LoRA Forward Optimization

## Acceptance checks

- Preserve the output of `input @ B @ A * scaling` while evaluating it as
  `(input @ B) @ A * scaling`, avoiding the full `B @ A` intermediate.
- Add a numeric regression test with non-square input/output dimensions and
  non-zero weights.
- Build and run the focused LoRA unit test and both LoRA benchmark CTests;
  retain existing benchmark gates and compare measured forward latency.
