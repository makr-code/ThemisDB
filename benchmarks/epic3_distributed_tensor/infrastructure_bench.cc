// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/// @file infrastructure_bench.cc
/// @brief Phase 5 benchmark source for distributed tensor control-plane stability.
///
/// Benchmark IDs:
///   E3INF-01  Mixed control-plane churn latency under healthy/degraded node updates
///   E3INF-02  Availability ratio under mixed placement and recovery pressure
///
/// This benchmark does not assert measured gate outcomes by itself. It provides
/// the runtime source required by the Phase 5 profile
/// `infrastructure_control_plane_stability`, which is evaluated against
/// `release_gate_manifest_epic3.json` after result-bundle aggregation.

#include <benchmark/benchmark.h>

#include "distributed_tensor/tensor_infrastructure.h"

#include <cstdint>
#include <string>
#include <vector>

namespace {

using themis::distributed_tensor::ClusterNode;
using themis::distributed_tensor::DefaultTensorInfrastructureManager;
using themis::distributed_tensor::HardwareCapability;
using themis::distributed_tensor::NodeStatus;

constexpr uint64_t kRequiredCapacityBytes = 8ULL * 1024 * 1024;

[[nodiscard]] ClusterNode makeNode(int index) {
  ClusterNode node;
  node.node_id = "node-" + std::to_string(index);
  node.endpoint = "10.0.0." + std::to_string(index + 1);
  node.rpc_port = static_cast<uint16_t>(50051 + index);
  node.status = NodeStatus::HEALTHY;
  node.hardware = (index % 3 == 0) ? HardwareCapability::NVIDIA_CUDA
                                    : HardwareCapability::CPU_ONLY;
  node.total_storage_bytes = 512ULL * 1024 * 1024;
  node.used_storage_bytes =
      static_cast<uint64_t>(index % 7) * 16ULL * 1024 * 1024;
  node.available_memory_bytes = 64ULL * 1024 * 1024;
  node.network_bandwidth_mbps = 2500;
  node.network_utilization = 0.05f * static_cast<float>(index % 5);
  node.cpu_utilization = 0.10f * static_cast<float>(index % 4);
  node.region = "bench-region";
  node.rack_id = "rack-" + std::to_string(index % 3);
  return node;
}

[[nodiscard]] DefaultTensorInfrastructureManager makeInfrastructureManager(
    int node_count, int degraded_nodes) {
  DefaultTensorInfrastructureManager manager;
  for (int index = 0; index < node_count; ++index) {
    auto node = makeNode(index);
    if (index < degraded_nodes) {
      node.status = NodeStatus::DEGRADED;
    }
    benchmark::DoNotOptimize(manager.register_node(node));
  }
  return manager;
}

void BM_Epic3InfrastructureControlPlaneLatency(benchmark::State& state) {
  const auto node_count = static_cast<int>(state.range(0));
  const auto degraded_nodes = static_cast<int>(state.range(1));

  for (auto _ : state) {
    auto manager = makeInfrastructureManager(node_count, degraded_nodes);

    for (int index = 0; index < node_count; ++index) {
      const auto& node_id = std::string("node-") + std::to_string(index);
      const auto next_status = (index % 5 == 0) ? NodeStatus::DEGRADED
                                                : NodeStatus::HEALTHY;
      benchmark::DoNotOptimize(manager.update_node_status(node_id, next_status));
      benchmark::DoNotOptimize(
          manager.is_node_available(node_id, kRequiredCapacityBytes));
    }

    const auto healthy = manager.get_healthy_nodes();
    benchmark::DoNotOptimize(healthy);
  }

  state.SetItemsProcessed(state.iterations() * node_count);
  state.counters["nodes"] = static_cast<double>(node_count);
  state.counters["degraded_nodes"] = static_cast<double>(degraded_nodes);
}

BENCHMARK(BM_Epic3InfrastructureControlPlaneLatency)
    ->Args({6, 1})
    ->Args({12, 2})
    ->Args({24, 4})
    ->Unit(benchmark::kMicrosecond);

void BM_Epic3InfrastructureAvailabilityRatio(benchmark::State& state) {
  const auto node_count = static_cast<int>(state.range(0));
  const auto degraded_nodes = static_cast<int>(state.range(1));
  auto manager = makeInfrastructureManager(node_count, degraded_nodes);

  for (int index = 0; index < degraded_nodes; ++index) {
    benchmark::DoNotOptimize(
        manager.update_node_status("node-" + std::to_string(index),
                                   NodeStatus::DEGRADED));
  }
  if (node_count > degraded_nodes) {
    benchmark::DoNotOptimize(
        manager.update_node_status("node-" + std::to_string(node_count - 1),
                                   NodeStatus::OFFLINE));
  }

  double availability_ratio = 0.0;
  for (auto _ : state) {
    int available_nodes = 0;
    for (int index = 0; index < node_count; ++index) {
      if (manager.is_node_available("node-" + std::to_string(index),
                                    kRequiredCapacityBytes)) {
        ++available_nodes;
      }
    }
    availability_ratio =
        (node_count == 0)
            ? 0.0
            : static_cast<double>(available_nodes) /
                  static_cast<double>(node_count);
    benchmark::DoNotOptimize(availability_ratio);
  }

  state.SetItemsProcessed(state.iterations() * node_count);
  state.counters["availability_ratio"] = availability_ratio;
  state.counters["nodes"] = static_cast<double>(node_count);
  state.counters["degraded_nodes"] = static_cast<double>(degraded_nodes);
}

BENCHMARK(BM_Epic3InfrastructureAvailabilityRatio)
    ->Args({6, 1})
    ->Args({12, 2})
    ->Args({24, 4})
    ->Unit(benchmark::kMicrosecond);

}  // namespace
