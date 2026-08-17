/**
 * @file tensor_infrastructure.h
 * @brief Low-level infrastructure for distributed tensor communication.
 *
 * Abstractions for inter-node transport, serialisation, and collective
 * operations used by the distributed tensor subsystem.
 */

// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "distributed_tensor/artifact_manifest.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace themis {
namespace distributed_tensor {

/// @defgroup tensor_infrastructure Tensor Infrastructure
/// @brief Node registry, stripe transport, and cluster coordination.
/// @{

/// Node status enumeration.
///
/// Tracks the operational status of a cluster node.
enum class NodeStatus {
  /// Node is healthy and operational.
  HEALTHY,

  /// Node is degraded but operational (e.g., elevated latency).
  DEGRADED,

  /// Node is temporarily unavailable.
  UNAVAILABLE,

  /// Node is permanently offline or removed from cluster.
  OFFLINE,

  /// Node is unknown or not yet initialized.
  UNKNOWN,
};

/// Node hardware capability enumeration.
///
/// Specifies available hardware accelerators on a node.
enum class HardwareCapability {
  /// CPU-only compute.
  CPU_ONLY,

  /// NVIDIA CUDA GPU.
  NVIDIA_CUDA,

  /// AMD HIP/ROCm GPU.
  AMD_HIP,

  /// Google TPU.
  GOOGLE_TPU,

  /// Intel Habana Gaudi.
  INTEL_HABANA,

  /// Custom or mixed accelerators.
  CUSTOM,
};

/// Cluster node information.
///
/// Tracks metadata and capabilities of a node in the distributed cluster.
struct ClusterNode {
  /// Unique node identifier.
  std::string node_id;

  /// Node hostname or IP address.
  std::string endpoint;

  /// RPC port for communication.
  uint16_t rpc_port = 50051;

  /// Current operational status.
  NodeStatus status = NodeStatus::UNKNOWN;

  /// Hardware capability of this node.
  HardwareCapability hardware = HardwareCapability::CPU_ONLY;

  /// Available storage capacity in bytes.
  uint64_t total_storage_bytes = 0;

  /// Used storage capacity in bytes.
  uint64_t used_storage_bytes = 0;

  /// Available memory (RAM) in bytes.
  uint64_t available_memory_bytes = 0;

  /// Network bandwidth capacity in Mbps.
  uint32_t network_bandwidth_mbps = 1000;

  /// Current network utilization (0.0 to 1.0).
  float network_utilization = 0.0f;

  /// Current CPU utilization (0.0 to 1.0).
  float cpu_utilization = 0.0f;

  /// Timestamp of last heartbeat (ISO 8601).
  std::string last_heartbeat_at;

  /// Timezone or region identifier.
  std::string region;

  /// Rack or topology zone.
  std::string rack_id;

  /// Custom metadata.
  std::unordered_map<std::string, std::string> custom_metadata;
};

/// Stripe transport information.
///
/// Describes transport configuration for moving shards across nodes.
struct StripeTransport {
  /// Protocol used (e.g., "grpc", "http", "rdma").
  std::string protocol = "grpc";

  /// Compression enabled for network transfer.
  bool compression_enabled = true;

  /// Compression algorithm (e.g., "gzip", "zstd", "snappy").
  std::string compression_algorithm = "zstd";

  /// Encryption enabled for transport security.
  bool encryption_enabled = true;

  /// Encryption algorithm (e.g., "AES256", "ChaCha20").
  std::string encryption_algorithm = "AES256";

  /// Parallel streams per transport connection.
  uint32_t parallel_streams = 4;

  /// Transport timeout in milliseconds.
  uint32_t timeout_ms = 30000;

  /// Retry policy: maximum attempts on transient failure.
  uint32_t max_retries = 3;

  /// Backoff strategy for retries (e.g., "exponential", "linear").
  std::string backoff_strategy = "exponential";

  /// Custom transport parameters.
  std::unordered_map<std::string, std::string> custom_metadata;
};

/// Tensor infrastructure manager interface.
///
/// Manages cluster nodes, monitoring, and stripe transport coordination.
class TensorInfrastructureManager {
 public:
  /// Construct a tensor infrastructure manager.
  TensorInfrastructureManager() = default;

  /// Copy constructor deleted.
  TensorInfrastructureManager(const TensorInfrastructureManager&) = delete;

  /// Move constructor.
  TensorInfrastructureManager(TensorInfrastructureManager&&) noexcept = default;

  /// Assignment operator deleted.
  TensorInfrastructureManager& operator=(const TensorInfrastructureManager&) =
      delete;

  /// Move assignment operator.
  TensorInfrastructureManager& operator=(
      TensorInfrastructureManager&&) noexcept = default;

  /// Virtual destructor.
  virtual ~TensorInfrastructureManager() = default;

  /// Register a node in the cluster.
  ///
  /// @param node Cluster node information.
  /// @return true if registration succeeded, false otherwise.
  virtual bool register_node(const ClusterNode& node) noexcept = 0;

  /// Unregister a node from the cluster.
  ///
  /// @param node_id Node identifier.
  /// @return true if unregistration succeeded, false if node not found.
  virtual bool unregister_node(const std::string& node_id) noexcept = 0;

  /// Get information about a specific node.
  ///
  /// @param node_id Node identifier.
  /// @return Cluster node information if found.
  virtual std::optional<ClusterNode> get_node(
      const std::string& node_id) const noexcept = 0;

  /// List all registered nodes.
  ///
  /// @return Vector of all cluster nodes.
  virtual std::vector<ClusterNode> list_nodes() const noexcept = 0;

  /// Get healthy (operational) nodes.
  ///
  /// @return Vector of healthy cluster nodes.
  virtual std::vector<ClusterNode> get_healthy_nodes() const noexcept = 0;

  /// Update node status.
  ///
  /// @param node_id Node identifier.
  /// @param status New node status.
  /// @return true if update succeeded, false if node not found.
  virtual bool update_node_status(const std::string& node_id,
                                   NodeStatus status) noexcept = 0;

  /// Get transport configuration.
  ///
  /// @return Current stripe transport configuration.
  virtual const StripeTransport& get_stripe_transport() const noexcept = 0;

  /// Update transport configuration.
  ///
  /// @param transport New stripe transport configuration.
  virtual void set_stripe_transport(StripeTransport transport) noexcept = 0;

  /// Check node availability for shard placement.
  ///
  /// @param node_id Node identifier.
  /// @param required_capacity Required storage capacity in bytes.
  /// @return true if node is available with sufficient capacity.
  virtual bool is_node_available(const std::string& node_id,
                                  uint64_t required_capacity) const
      noexcept = 0;
};

/// Default tensor infrastructure manager implementation.
///
/// Provides node registry, monitoring, and stripe transport coordination.
class DefaultTensorInfrastructureManager : public TensorInfrastructureManager {
 public:
  /// Construct the default infrastructure manager.
  DefaultTensorInfrastructureManager();

  /// Move constructor.
  DefaultTensorInfrastructureManager(
      DefaultTensorInfrastructureManager&&) noexcept = default;

  /// Move assignment operator.
  DefaultTensorInfrastructureManager& operator=(
      DefaultTensorInfrastructureManager&&) noexcept = default;

  /// Destructor.
  ~DefaultTensorInfrastructureManager() override = default;

  /// Register a node.
  bool register_node(const ClusterNode& node) noexcept override;

  /// Unregister a node.
  bool unregister_node(const std::string& node_id) noexcept override;

  /// Get node information.
  std::optional<ClusterNode> get_node(
      const std::string& node_id) const noexcept override;

  /// List all nodes.
  std::vector<ClusterNode> list_nodes() const noexcept override;

  /// Get healthy nodes.
  std::vector<ClusterNode> get_healthy_nodes() const noexcept override;

  /// Update node status.
  bool update_node_status(const std::string& node_id,
                          NodeStatus status) noexcept override;

  /// Get stripe transport configuration.
  const StripeTransport& get_stripe_transport() const noexcept override;

  /// Set stripe transport configuration.
  void set_stripe_transport(StripeTransport transport) noexcept override;

  /// Check node availability.
  bool is_node_available(const std::string& node_id,
                         uint64_t required_capacity) const noexcept override;

 private:
  /// In-memory node registry (in production, would use persistent storage).
  std::unordered_map<std::string, ClusterNode> nodes_;

  /// Stripe transport configuration.
  StripeTransport stripe_transport_;
};

/// @}

}  // namespace distributed_tensor
}  // namespace themis
