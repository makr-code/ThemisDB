/**
 * @file tensor_infrastructure.h
 * @brief Tensor artifact fabric infrastructure — transport, registry, and health.
 *
 * Provides the low-level fabric that all distributed tensor operations run on:
 * a node registry, a stripe transport layer, and a health-monitor interface.
 *
 * Planned in: src/distributed_tensor/README.md (sub-issue 3.7 / #5435)
 */

#pragma once

#include "tensor_artifact_classes.h"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::distributed_tensor {

/// Health state of a node in the tensor fabric.
enum class NodeHealth {
    Healthy,
    Degraded,  ///< Slower than expected; usable but low-priority
    Unhealthy, ///< Not responding; excluded from placement
    Unknown,   ///< Health not yet determined
};

/// Live health record for one fabric node.
struct NodeHealthRecord {
    std::string  shard_key;
    NodeHealth   health = NodeHealth::Unknown;
    double       last_latency_ms = 0.0;
    std::chrono::system_clock::time_point last_checked;
    std::string  last_error;
};

/// Configuration for the stripe transport layer.
struct TransportConfig {
    std::uint32_t  connect_timeout_ms  = 500;
    std::uint32_t  request_timeout_ms  = 5000;
    std::uint32_t  max_retries         = 3;
    std::size_t    stripe_chunk_bytes  = 4 * 1024 * 1024; // 4 MiB
    bool           enable_tls          = true;
};

/**
 * @brief Node registry for the distributed tensor fabric.
 */
class INodeRegistry {
public:
    virtual ~INodeRegistry() = default;

    /// Register a node in the fabric.
    virtual void registerNode(const std::string& shard_key,
                               const std::string& endpoint) = 0;

    /// Remove a node.
    virtual void deregisterNode(const std::string& shard_key) = 0;

    /// Look up the endpoint for a shard key.
    virtual std::optional<std::string> endpoint(
        const std::string& shard_key) const = 0;

    /// Return the health record for a node.
    virtual NodeHealthRecord healthOf(const std::string& shard_key) const = 0;

    /// Return all shard keys with at least the given health level.
    virtual std::vector<std::string> healthyNodes(
        NodeHealth min_health = NodeHealth::Healthy) const = 0;
};

/**
 * @brief Stripe transport layer for reading and writing artifact stripes.
 */
class IStripeTransport {
public:
    virtual ~IStripeTransport() = default;

    /// Write a stripe payload to a remote shard.
    virtual bool writeStripe(const std::string& shard_key,
                              const std::string& artifact_id,
                              std::uint32_t stripe_index,
                              const std::vector<std::uint8_t>& data) = 0;

    /// Read a stripe payload from a remote shard.
    virtual std::vector<std::uint8_t> readStripe(
        const std::string& shard_key,
        const std::string& artifact_id,
        std::uint32_t stripe_index) = 0;

    /// Delete a stripe from a remote shard.
    virtual bool deleteStripe(const std::string& shard_key,
                               const std::string& artifact_id,
                               std::uint32_t stripe_index) = 0;

    /// Register a health-change callback.
    using HealthCallback = std::function<void(const NodeHealthRecord&)>;
    virtual void onHealthChange(HealthCallback cb) = 0;
};

/// Factory: create a node registry backed by the given initial topology.
std::unique_ptr<INodeRegistry> makeNodeRegistry(
    const std::vector<std::pair<std::string, std::string>>& shard_endpoints);

/// Factory: create a stripe transport using the given registry and configuration.
std::unique_ptr<IStripeTransport> makeStripeTransport(
    std::shared_ptr<INodeRegistry> registry, const TransportConfig& cfg);

} // namespace themis::distributed_tensor
