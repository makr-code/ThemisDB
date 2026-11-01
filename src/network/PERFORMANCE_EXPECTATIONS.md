---
Author: "ThemisDB Contributors"
Created: "unknown (pre-existing document; creation date not recorded)"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Network Performance Evidence

## Current benchmark sources

The network benchmark targets are in `benchmarks/network/`:

| Source | Cases | What they establish |
|---|---|---|
| `bench_network_release_gates.cpp` | NRG-01..NRG-06 | In-process frame dispatch, auth/rate-limit helpers, WebSocket dispatch, mock accept, serialization gates |
| `bench_network_routing_gates.cpp` | NRG-07..NRG-12 | In-memory topology, load-balancer, circuit-breaker, queue, and session-guard paths |
| `bench_network_protocol_path.cpp` | NRG-P1..NRG-P4 | Explicit stub/model cases for WebSocket parsing, zero-copy overhead, QUIC 0-RTT, and gRPC metadata |

The release and routing targets define regression thresholds; they do not
measure production listener capacity or prove live transport latency. The
protocol-path cases are stubs, and representative hardware baselines remain
pending. Do not report these thresholds as achieved measurements.

No current network benchmark directly compares `ProtobufParser`/
`ProtobufSerializer` with libprotobuf or measures live TCP/TLS/QUIC/gRPC
throughput.

## Build and run

The `linux-release` preset disables benchmarks by default. Configure and build
with:

```sh
cmake --preset linux-release -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build-gcc-linux-release --target \
  bench_network_bench_network_release_gates \
  bench_network_bench_network_routing_gates \
  bench_network_bench_network_protocol_path
```

Run the relevant executable for its Google Benchmark output. Record compiler,
build flags, hardware, exact case, and whether the path is modeled or stubbed
before comparing results. See [`RESEARCH.md`](RESEARCH.md) for measurement
controls and limits.
