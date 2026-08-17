> **Build:** `cmake --preset release && cmake --build build/release`

# Distributed Tensor Module — Public Headers

**Module Path:** `include/distributed_tensor/`
**Implementation:** `../../src/distributed_tensor/`

## Purpose

Public interfaces and declarations for ThemisDB's distributed tensor sharding, placement, and computation subsystem, providing abstractions for tensor distribution across nodes and integrity verification.

## Canonical Module Documentation

`include/distributed_tensor/` contains public header contracts. Canonical module behavior, architecture, and operations docs live in `src/distributed_tensor/`:

- [`../../src/distributed_tensor/README.md`](../../src/distributed_tensor/README.md)
- [`../../src/distributed_tensor/ARCHITECTURE.md`](../../src/distributed_tensor/ARCHITECTURE.md)
- [`../../src/distributed_tensor/ROADMAP.md`](../../src/distributed_tensor/ROADMAP.md)
- [`../../src/distributed_tensor/FUTURE_ENHANCEMENTS.md`](../../src/distributed_tensor/FUTURE_ENHANCEMENTS.md)

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `artifact_manifest.h` | `ArtifactManifest` — distributed tensor metadata and placement info |
| `distributed_planner.h` | `DistributedPlanner` — tensor distribution and placement strategy |
| `integrity_verification.h` | `IntegrityVerification` — checksum and verification for distributed tensors |
| `shard_coordinator.h` | `ShardCoordinator` — orchestration of tensor shards across nodes |
| `tensor_replication.h` | `TensorReplication` — replication policies and cross-node synchronization |

## Usage

```cpp
#include "distributed_tensor/distributed_planner.h"

auto planner = themis::distributed_tensor::createDistributedPlanner();

auto placement = planner->planPlacement(tensor, nodes, {
    .replication_factor = 3,
    .locality_aware = true
});

planner->execute(placement);
```

For full runtime usage examples (planning, verification, coordination), see [`../../src/distributed_tensor/README.md`](../../src/distributed_tensor/README.md).

## Key Configuration Surface

Important configuration entry points are declared in:

- `distributed_planner.h` (`DistributedPlanner::Config` for placement strategy)
- `tensor_replication.h` (`TensorReplication::Config` for replication policies)
- `shard_coordinator.h` (coordinator configuration and tuning)
- `integrity_verification.h` (checksum algorithms and verification options)

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-distributed_tensor
```

## See Also

- [`../../src/distributed_tensor/README.md`](../../src/distributed_tensor/README.md) — implementation details
- [`../../src/tensor/README.md`](../../src/tensor/README.md) — base tensor module
- [`../../src/sharding/README.md`](../../src/sharding/README.md) — sharding coordination

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
