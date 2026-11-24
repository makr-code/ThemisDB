// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/shard_rpc_fragment_fetcher.h"

#include "distributed_tensor/integrity_verification.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <stdexcept>
#include <utility>

namespace themis::distributed_tensor {
namespace {

constexpr uint64_t kMaxTensorFragmentBytes = 1ULL << 30;

FragmentLoadResult failedResult(const FragmentLoadRequest& request,
                                std::string reason) {
  FragmentLoadResult result;
  result.shard_id = request.shard_id;
  result.artifact_id = request.artifact_id;
  result.error_reason = std::move(reason);
  return result;
}

}  // namespace

ShardRPCFragmentFetcher::ShardRPCFragmentFetcher(
    std::unordered_map<std::string,
                       std::shared_ptr<sharding::ShardRPCClient>> clients)
    : ShardRPCFragmentFetcher(std::move(clients), Config{}) {}

ShardRPCFragmentFetcher::ShardRPCFragmentFetcher(
    std::unordered_map<std::string,
                       std::shared_ptr<sharding::ShardRPCClient>> clients,
    Config config)
    : clients_(std::move(clients)), config_(config) {
  if (clients_.empty() || config_.max_response_bytes_per_fragment == 0 ||
      config_.max_response_bytes_per_fragment > kMaxTensorFragmentBytes ||
      config_.max_response_bytes_per_batch == 0 ||
      config_.max_response_bytes_per_batch > kMaxTensorFragmentBytes) {
    throw std::invalid_argument(
        "ShardRPCFragmentFetcher requires peers and positive byte limits up to 1 GiB");
  }
  for (const auto& [node_id, client] : clients_) {
    if (node_id.empty() || !client) {
      throw std::invalid_argument(
          "ShardRPCFragmentFetcher peer IDs and clients must be non-empty");
    }
  }
}

FragmentLoadResult ShardRPCFragmentFetcher::fetchFragment(
    const FragmentLoadRequest& request,
    const std::string& correlation_id) const noexcept {
  const auto started = std::chrono::steady_clock::now();
  try {
    if (request.target_node_id.empty() || request.shard_id.empty() ||
        request.artifact_id.empty() || request.expected_content_hash.empty() ||
        request.timeout_ms == 0 || request.max_response_bytes == 0 ||
        request.max_response_bytes > config_.max_response_bytes_per_fragment) {
      return failedResult(request,
                          "fragment request is missing identity or exceeds its byte limit");
    }
    const auto peer = clients_.find(request.target_node_id);
    if (peer == clients_.end()) {
      return failedResult(request, "no ShardRPC client configured for target node");
    }

    auto payload = peer->second->fetchTensorFragment(
        request.target_node_id, request.shard_id, request.artifact_id,
        request.max_response_bytes, request.timeout_ms, correlation_id);
    if (payload.sha256 != request.expected_content_hash) {
      return failedResult(request,
                          "fragment digest does not match the manifest");
    }

    const std::string bytes(payload.bytes.begin(), payload.bytes.end());
    const DefaultIntegrityVerificationEngine verifier;
    if (!verifier.verify_shard(bytes, request.expected_content_hash)) {
      return failedResult(request,
                          "fragment bytes failed manifest SHA-256 verification");
    }

    FragmentLoadResult result;
    result.shard_id = request.shard_id;
    result.artifact_id = request.artifact_id;
    result.success = true;
    result.fragment_data = std::move(payload.bytes);
    result.content_hash = request.expected_content_hash;
    result.load_latency_ms = static_cast<float>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started)
            .count()) /
        1000.0f;
    result.loaded_at = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    return result;
  } catch (const std::exception& error) {
    return failedResult(request, error.what());
  } catch (...) {
    return failedResult(request, "fragment fetch failed with a non-standard exception");
  }
}

std::vector<FragmentLoadResult> ShardRPCFragmentFetcher::fetchFragments(
    const std::vector<FragmentLoadRequest>& requests,
    const std::string& correlation_id) const noexcept {
  std::vector<FragmentLoadResult> results;
  results.reserve(requests.size());
  uint64_t total_bytes = 0;
  std::set<std::pair<std::string, std::string>> seen_fragments;
  for (const auto& request : requests) {
    if (!seen_fragments.emplace(request.shard_id, request.artifact_id).second) {
      results.push_back(failedResult(request, "duplicate fragment request"));
      continue;
    }
    if (request.max_response_bytes == 0 ||
        request.max_response_bytes >
            config_.max_response_bytes_per_batch -
                std::min(total_bytes, config_.max_response_bytes_per_batch)) {
      results.push_back(failedResult(
          request, "fragment batch exceeds its aggregate response byte limit"));
      continue;
    }
    auto result = fetchFragment(request, correlation_id);
    if (result.success) {
      total_bytes += result.fragment_data.size();
    }
    results.push_back(std::move(result));
  }
  return results;
}

}  // namespace themis::distributed_tensor
