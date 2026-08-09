# benchmarks/llama_cpp

Mirrored benchmark folder for `src/llama_cpp`.

## Release Gate Benchmarks (LLCPG-1..4)

These four benchmarks establish and validate the hard release gates for the
llama_cpp plugin (v2.3.0+). They run in stub mode in CI and with a real GGUF
model in pre-release validation.

| Gate ID | Benchmark | Stub Gate | Production Gate |
|---------|-----------|-----------|-----------------|
| LLCPG-1 | `LLCPG1_TTFT_Stub_Baseline` | ≤ 5 ms/call | ≤ 1100 ms P95 (A10G) |
| LLCPG-2 | `LLCPG2_BatchEmbedding_Stub` | Baseline throughput | ≥ 8500 tok/s |
| LLCPG-3 | `LLCPG3_LoRALoad_P99` | ≤ 1 ms P99 | ≤ 75 ms P99 |
| LLCPG-4 | `LLCPG4_RegressionBaseline` | Baseline snapshot | ≤ 8 % regression |

### Running only gate benchmarks

```bash
./bench_llama_cpp_inference \
  --benchmark_filter="LLCPG" \
  --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true \
  --benchmark_out=llcpg_results.json \
  --benchmark_out_format=json
```

### Regression gating

Save a baseline run with `--benchmark_out=baseline.json`, then compare
a subsequent run with the Google Benchmark compare tool:

```bash
python3 tools/compare.py benchmarks baseline.json current.json
```

Regression is considered a gate failure if any LLCPG-4 throughput metric
drops more than **8 %** from the recorded baseline.
