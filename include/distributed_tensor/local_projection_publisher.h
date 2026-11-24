// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "distributed_tensor/artifact_manifest.h"
#include "tensor/tensor_mid_layer.h"

#include <memory>
#include <optional>
#include <string>
#include <cstdint>

namespace themis {
class RocksDBWrapper;
namespace sharding {
class ShardArtifactFragmentStore;
class ShardRPCClient;
}
}

namespace themis::distributed_tensor {

/**
 * @brief Publishes one local advisory adapter-similarity projection and its manifest.
 *
 * The host must authenticate the caller, bind each principal to one tenant,
 * verify the summary's provenance, and supply authoritative local shard and
 * node identities. Do not expose publication directly to untrusted requests.
 */
class LocalProjectionPublisher final {
 public:
  /**
   * @brief Bind a publisher to one local fragment store and RocksDB database.
   * @param database Open database also used by fragments; retained by shared ownership.
   * @param shard_id Authoritative local shard identifier, non-empty and at most 1024 bytes.
   * @param node_id Authoritative local node identifier, non-empty and at most 1024 bytes.
   * @param max_fragment_bytes Maximum fragment size, in [1, 1 GiB]; configure
   *        the reader with the same or a larger limit.
   * @throws std::invalid_argument for null, closed, empty, or oversized inputs.
   * @note Callers must keep the database open until both objects are destroyed.
   */
  LocalProjectionPublisher(
      std::shared_ptr<RocksDBWrapper> database, std::string shard_id,
      std::string node_id, uint64_t max_fragment_bytes);

  /**
   * @brief Persist a projection before publishing its immutable local manifest.
   * @param principal_id Host-authorized storage principal, tenant-bound by policy.
   * @param tenant_id Authenticated tenant owning the trusted summary.
   * @param source_id Stable source artifact or snapshot identifier.
   * @param summary Trusted non-federated adapter summary; its scope must use
   *        `__adapters__:<tenant>:` and each candidate key must equal
   *        `__adapters__:<tenant>:<domain>:<base_model_id>`.
   * @return Complete public manifest for the local shard.
   * @throws std::invalid_argument for invalid identities, scope, candidates, or projection.
   * @throws std::runtime_error for hash, storage, integrity, or manifest conflicts.
   * @note The projection, local manifest and remote-readable manifest record
   *       are committed synchronously in that order. Failure between commits
   *       can leave incomplete local state; retry is idempotent.
   */
  [[nodiscard]] ArtifactManifest publish(
      const std::string& principal_id, const std::string& tenant_id,
      const std::string& source_id, const tensor::TensorLayerSummary& summary);

  /**
   * @brief Derive and publish an adapter-similarity projection from a graph.
   * @param graph Trusted graph containing the query and candidate adapters.
   * @param principal_id Host-authorized principal bound to the tenant.
   * @param tenant_id Authenticated tenant owning the query.
   * @param source_id Stable source snapshot identifier supplied by the host.
   * @param domain Query adapter domain.
   * @param base_model_id Query adapter base-model identifier.
   * @param top_k Maximum candidate count, from 1 through the codec limit.
   * @return Persisted single-shard public manifest.
   * @throws std::invalid_argument for invalid arguments or non-publishable results.
   * @throws std::runtime_error for a missing query, mismatched graph metadata,
   *         hashing or storage errors.
   * @note Callers must establish graph provenance, principal/tenant binding,
   *       and snapshot freshness, and exclude graph mutations during this call.
   *       Injected graph scoring callbacks are trusted.
   */
  [[nodiscard]] ArtifactManifest publishFromGraph(
      const tensor::TensorFingerprintGraph& graph, const std::string& principal_id,
      const std::string& tenant_id, const std::string& source_id,
      const std::string& domain, const std::string& base_model_id, std::size_t top_k);

  /**
   * @brief Load and verify a local published manifest and its fragment.
   * @param principal_id Host-authorized storage principal used at publication.
   * @param artifact_id Immutable artifact ID returned by publish().
   * @return The verified manifest, or empty if no local manifest exists.
   * @throws std::invalid_argument for invalid identifiers.
   * @throws std::runtime_error on storage errors, corruption, or missing fragment.
   * @note This reads local storage; use loadRemote() for mTLS transport.
   */
  [[nodiscard]] std::optional<ArtifactManifest> load(
      const std::string& principal_id, const std::string& artifact_id) const;

  /**
   * @brief Fetch a published projection manifest from its authenticated shard.
   * @param client Configured mTLS client with the expected peer SAN and node binding;
   *        both client and server response ceilings must permit 16 KiB.
   * @param principal_id Expected tenant-bound principal configured on the server.
   * @param tenant_id Expected authorized tenant.
   * @param shard_id Expected shard served by the endpoint.
   * @param node_id Expected node associated with the verified TLS peer.
   * @param artifact_id Content-bound artifact ID obtained from a trusted source.
   * @param max_projection_bytes Maximum accepted projection payload size.
   * @param timeout_ms Shared deadline budget for both RPCs, between 1 and 30000 ms.
   * @param correlation_id Optional RPC correlation ID, at most 256 bytes.
   * @return Single-shard public manifest after fetching and verifying its projection.
   * @throws std::invalid_argument for invalid identities or resource limits.
   * @throws std::runtime_error for RPC, authentication, integrity, or record errors.
   * @note The caller supplies trustworthy peer-to-node and principal-to-tenant
   *       bindings and a fresh artifact ID. TLS does not authenticate the
   *       original producer or provide freshness or manifest discovery. The
   *       planner may fetch the projection again; hashing and decoding are
   *       synchronous and are not preempted by the RPC deadline.
   */
  [[nodiscard]] static ArtifactManifest loadRemote(
      sharding::ShardRPCClient& client, const std::string& principal_id,
      const std::string& tenant_id, const std::string& shard_id,
      const std::string& node_id, const std::string& artifact_id,
      uint64_t max_projection_bytes, uint32_t timeout_ms,
      const std::string& correlation_id);

 private:
  std::shared_ptr<RocksDBWrapper> database_;
  std::shared_ptr<sharding::ShardArtifactFragmentStore> fragments_;
  std::string shard_id_;
  std::string node_id_;
  uint64_t max_fragment_bytes_;
};

}  // namespace themis::distributed_tensor
