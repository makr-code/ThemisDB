/**
 * @file shard_rpc_fragment_fetcher.h
 * @brief ShardRPC adapter for manifest-bound distributed tensor fragments.
 */

// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "distributed_tensor/distributed_planner.h"
#include "sharding/shard_rpc_client.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::distributed_tensor {

/**
 * @brief Fetches tensor fragments over the authenticated ShardRPC transport.
 *
 * The supplied clients are keyed by configured peer node ID and retained by
 * shared ownership. Each received payload is checked against the manifest
 * digest before it can become a successful FragmentLoadResult.
 */
class ShardRPCFragmentFetcher final : public IFragmentFetcher {
 public:
  struct Config {
    uint64_t max_response_bytes_per_fragment = 64ULL * 1024ULL * 1024ULL;
    uint64_t max_response_bytes_per_batch = 256ULL * 1024ULL * 1024ULL;
  };

  /**
   * @brief Create the adapter with an explicit peer-to-client map.
   * @param clients Map from manifest placement node IDs to configured mTLS
   *        clients. Entries must be non-empty and non-null.
   * @param config Per-fragment and aggregate response byte ceilings; each
   *        ceiling must be positive and no greater than 1 GiB.
   * @throws std::invalid_argument for empty, invalid, or unbounded configuration.
   * @note Client objects are retained by shared ownership; calls do not mutate
   *       adapter state and may be issued concurrently by callers.
   */
  explicit ShardRPCFragmentFetcher(
      std::unordered_map<std::string,
                         std::shared_ptr<sharding::ShardRPCClient>> clients);
  ShardRPCFragmentFetcher(
      std::unordered_map<std::string,
                         std::shared_ptr<sharding::ShardRPCClient>> clients,
      Config config);

  /**
   * @brief Fetch and verify one fragment.
   * @param request Requires target node, shard, artifact, expected SHA-256,
   *        positive timeout, and bounded response size.
   * @param correlation_id Optional identifier echoed by every transport frame.
   * @return Successful result only after complete transport and manifest-digest
   *         verification; all failures have success=false and an error reason.
   * @note The request is not retried. Timeouts bound transport work; failed or
   *       corrupt payloads are never returned in fragment_data.
   */
  [[nodiscard]] FragmentLoadResult fetchFragment(
      const FragmentLoadRequest& request,
      const std::string& correlation_id = {}) const noexcept override;

  /**
   * @brief Fetch a batch sequentially under the aggregate byte ceiling.
   * @param requests Ordered fragment requests; processing stops successfully
   *        only for entries that fit the remaining aggregate budget.
   * @param correlation_id Optional identifier shared by all requests.
   * @return One result per request, preserving input order.
   * @note Sequential execution bounds in-flight payload memory and preserves
   *       deterministic result ordering; transport failures are reported per
   *       fragment without a success-shaped fallback.
   */
  [[nodiscard]] std::vector<FragmentLoadResult> fetchFragments(
      const std::vector<FragmentLoadRequest>& requests,
      const std::string& correlation_id = {}) const noexcept override;

 private:
  std::unordered_map<std::string,
                     std::shared_ptr<sharding::ShardRPCClient>> clients_;
  Config config_;
};

}  // namespace themis::distributed_tensor
