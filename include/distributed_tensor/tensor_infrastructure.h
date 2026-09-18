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


enum class NodeStatus {
  HEALTHY,

  DEGRADED,

  UNAVAILABLE,

  OFFLINE,

  UNKNOWN,
};

enum class HardwareCapability {
  CPU_ONLY,

  NVIDIA_CUDA,

  AMD_HIP,

  GOOGLE_TPU,

  INTEL_HABANA,

  CUSTOM,
};

struct ClusterNode {
  std::string node_id;

  std::string endpoint;

  uint16_t rpc_port = 50051;

  NodeStatus status = NodeStatus::UNKNOWN;

  HardwareCapability hardware = HardwareCapability::CPU_ONLY;

  uint64_t total_storage_bytes = 0;

  uint64_t used_storage_bytes = 0;

  uint64_t available_memory_bytes = 0;

  uint32_t network_bandwidth_mbps = 1000;

  float network_utilization = 0.0f;

  float cpu_utilization = 0.0f;

  std::string last_heartbeat_at;

  std::string region;

  std::string rack_id;

  std::unordered_map<std::string, std::string> custom_metadata;
};

struct StripeTransport {
  std::string protocol = "grpc";

  bool compression_enabled = true;

  std::string compression_algorithm = "zstd";

  bool encryption_enabled = true;

  std::string encryption_algorithm = "AES256";

  uint32_t parallel_streams = 4;

  uint32_t timeout_ms = 30000;

  uint32_t max_retries = 3;

  std::string backoff_strategy = "exponential";

  std::unordered_map<std::string, std::string> custom_metadata;
};

class TensorInfrastructureManager {
 public:
  TensorInfrastructureManager() = default;

  TensorInfrastructureManager(const TensorInfrastructureManager&) = delete;

  TensorInfrastructureManager(TensorInfrastructureManager&&) noexcept = default;

  TensorInfrastructureManager& operator=(const TensorInfrastructureManager&) =
      delete;

  TensorInfrastructureManager& operator=(
      TensorInfrastructureManager&&) noexcept = default;

  /**
   * @brief Tensor Infrastructure Manager.
   * @return Return value.
   */
  virtual ~TensorInfrastructureManager() = default;

  /**
   * @brief Register node.
   * @param[in] node Input parameter.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  virtual bool register_node(const ClusterNode& node) noexcept = 0;

  /**
   * @brief Unregister node.
   * @param[in] node_id Identifier of the node.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  virtual bool unregister_node(const std::string& node_id) noexcept = 0;

  /**
   * @brief Get node.
   * @param[in] node_id Identifier of the node.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  virtual std::optional<ClusterNode> get_node(
      const std::string& node_id) const noexcept = 0;

  /**
   * @brief List nodes.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  virtual std::vector<ClusterNode> list_nodes() const noexcept = 0;

  /**
   * @brief Get healthy nodes.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  virtual std::vector<ClusterNode> get_healthy_nodes() const noexcept = 0;

  /**
   * @brief Update node status.
   * @param[in] node_id Identifier of the node.
   * @param[in] status Input parameter.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  virtual bool update_node_status(const std::string& node_id,
                                   NodeStatus status) noexcept = 0;

  /**
   * @brief Get stripe transport.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  virtual const StripeTransport& get_stripe_transport() const noexcept = 0;

  /**
   * @brief Set stripe transport.
   * @param[in] transport Input parameter.
   * @note Exception safety: noexcept.
   */
  virtual void set_stripe_transport(StripeTransport transport) noexcept = 0;

  /**
   * @brief Is node available.
   * @param[in] node_id Identifier of the node.
   * @param[in] required_capacity Input parameter.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  virtual bool is_node_available(const std::string& node_id,
                                  uint64_t required_capacity) const
      noexcept = 0;
};

class DefaultTensorInfrastructureManager : public TensorInfrastructureManager {
 public:
  DefaultTensorInfrastructureManager();

  DefaultTensorInfrastructureManager(
      DefaultTensorInfrastructureManager&&) noexcept = default;

  DefaultTensorInfrastructureManager& operator=(
      DefaultTensorInfrastructureManager&&) noexcept = default;

  ~DefaultTensorInfrastructureManager() override = default;

  bool register_node(const ClusterNode& node) noexcept override;

  bool unregister_node(const std::string& node_id) noexcept override;

  std::optional<ClusterNode> get_node(
      const std::string& node_id) const noexcept override;

  std::vector<ClusterNode> list_nodes() const noexcept override;

  std::vector<ClusterNode> get_healthy_nodes() const noexcept override;

  bool update_node_status(const std::string& node_id,
                          NodeStatus status) noexcept override;

  const StripeTransport& get_stripe_transport() const noexcept override;

  void set_stripe_transport(StripeTransport transport) noexcept override;

  bool is_node_available(const std::string& node_id,
                         uint64_t required_capacity) const noexcept override;

 private:
  std::unordered_map<std::string, ClusterNode> nodes_;

  StripeTransport stripe_transport_;
};


}  // namespace distributed_tensor
}  // namespace themis
