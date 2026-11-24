// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/local_projection_publisher.h"

#include "distributed_tensor/graph_summary_projection.h"
#include "sharding/shard_artifact_fragment_store.h"
#include "sharding/shard_rpc_client.h"
#include "storage/rocksdb_wrapper.h"

#include <nlohmann/json.hpp>
#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace themis::distributed_tensor {
namespace {

constexpr std::size_t kMaxIdentityBytes = 1024;
constexpr std::size_t kMaxManifestBytes = 16384;
constexpr std::string_view kManifestPrefix = "themis/tensor-projection-manifest/v1/";

void requireIdentity(std::string_view value) {
  if (value.empty() || value.size() > kMaxIdentityBytes) {
    throw std::invalid_argument("projection publication identity is empty or oversized");
  }
}

std::string hexEncode(std::string_view value) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(value.size() * 2);
  for (const unsigned char byte : value) {
    result.push_back(digits[byte >> 4]);
    result.push_back(digits[byte & 15]);
  }
  return result;
}

std::string manifestKey(std::string_view principal, std::string_view artifact) {
  return std::string(kManifestPrefix) + hexEncode(principal) + "/" + hexEncode(artifact);
}

std::string manifestArtifact(std::string_view artifact) {
  return "tspjm1-" + std::string(artifact);
}

std::string hash(std::string_view data) {
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  unsigned int size = 0;
  if (EVP_Digest(data.data(), data.size(), digest.data(), &size,
                 EVP_sha256(), nullptr) != 1 || size != 32) {
    throw std::runtime_error("projection publication SHA-256 unavailable");
  }
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(64);
  for (std::size_t i = 0; i < size; ++i) {
    result.push_back(digits[digest[i] >> 4]);
    result.push_back(digits[digest[i] & 15]);
  }
  return result;
}

std::string artifactId(const std::string& principal, const std::string& tenant,
                       const std::string& source, const std::string& shard,
                       const std::string& node, const std::string& scope,
                       const std::string& digest) {
  std::string input = "themis.tensor.local-projection/v1";
  for (const auto* field : {&principal, &tenant, &source, &shard, &node, &scope, &digest}) {
    input.append(std::to_string(field->size()));
    input.push_back(':');
    input.append(*field);
  }
  return "tspj1-" + hash(input);
}

ArtifactManifest makeManifest(const std::string& artifact, const std::string& source,
                              const std::string& shard, const std::string& node,
                              const std::string& digest, uint64_t size) {
  ArtifactManifest manifest(artifact, ArtifactClass::DERIVED);
  manifest.set_version("1");
  manifest.set_parent_artifact_id(source);
  ShardPlacement placement;
  placement.shard_id = shard;
  placement.node_id = node;
  placement.shard_size_bytes = size;
  placement.shard_content_hash = digest;
  manifest.add_shard_placement(std::move(placement));
  return manifest;
}

struct ManifestRecord {
  std::string principal;
  std::string tenant;
  std::string source;
  std::string shard;
  std::string node;
  std::string scope;
  std::string digest;
  uint64_t size;
};

ManifestRecord decodeManifest(const std::vector<uint8_t>& bytes,
                              const std::string& principal,
                              const std::string& shard,
                              const std::string& node,
                              const std::string& artifact,
                              uint64_t max_projection_bytes) {
  if (bytes.size() > kMaxManifestBytes) {
    throw std::runtime_error("projection manifest exceeds its size limit");
  }
  try {
    const auto json = nlohmann::json::parse(bytes.begin(), bytes.end());
    ManifestRecord record{
        json.at("principal").get<std::string>(),
        json.at("tenant").get<std::string>(),
        json.at("source").get<std::string>(),
        json.at("shard").get<std::string>(),
        json.at("node").get<std::string>(),
        json.at("scope").get<std::string>(),
        json.at("digest").get<std::string>(),
        json.at("size").get<uint64_t>()};
    const auto digest_is_sha256 = record.digest.size() == 64 &&
        std::all_of(record.digest.begin(), record.digest.end(), [](unsigned char ch) {
          return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
        });
    const auto tenant_prefix = "__adapters__:" + record.tenant + ":";
    if (json.at("format").get<int>() != 1 ||
        json.at("artifact").get<std::string>() != artifact ||
        record.principal != principal || record.shard != shard || record.node != node ||
        record.tenant.empty() || record.tenant.size() > kMaxIdentityBytes ||
        record.tenant.find(':') != std::string::npos ||
        record.source.empty() || record.source.size() > kMaxIdentityBytes ||
        record.scope.size() > GraphSummaryProjectionCodec::kMaxTextBytes ||
        record.scope.size() <= tenant_prefix.size() ||
        record.scope.rfind(tenant_prefix, 0) != 0 || !digest_is_sha256 ||
        record.size == 0 || record.size > max_projection_bytes ||
        record.size > GraphSummaryProjectionCodec::kMaxPayloadBytes ||
        artifactId(record.principal, record.tenant, record.source, record.shard,
                   record.node, record.scope, record.digest) != artifact) {
      throw std::runtime_error("projection manifest is invalid");
    }
    return record;
  } catch (const nlohmann::json::exception& error) {
    throw std::runtime_error(std::string("projection manifest is corrupt: ") + error.what());
  }
}

void validateProjection(const std::vector<uint8_t>& bytes, const ManifestRecord& record) {
  const auto decoded = GraphSummaryProjectionCodec::decode(bytes);
  if (!decoded.ok() || decoded.summary->scope_key != record.scope) {
    throw std::runtime_error("projection manifest payload is invalid or has a different scope");
  }
  const auto tenant_prefix = "__adapters__:" + record.tenant + ":";
  for (const auto& candidate : decoded.summary->similar_adapters) {
    if (candidate.domain.empty() || candidate.base_model_id.empty() ||
        candidate.adapter_key != tenant_prefix + candidate.domain + ":" +
                                     candidate.base_model_id) {
      throw std::runtime_error("projection manifest contains an invalid candidate");
    }
  }
}

}  // namespace

LocalProjectionPublisher::LocalProjectionPublisher(
    std::shared_ptr<RocksDBWrapper> database, std::string shard_id,
    std::string node_id, uint64_t max_fragment_bytes)
    : database_(std::move(database)),
      shard_id_(std::move(shard_id)),
      node_id_(std::move(node_id)),
      max_fragment_bytes_(max_fragment_bytes) {
  requireIdentity(shard_id_);
  requireIdentity(node_id_);
  fragments_ = std::make_shared<sharding::ShardArtifactFragmentStore>(
      database_, max_fragment_bytes);
}

ArtifactManifest LocalProjectionPublisher::publish(
    const std::string& principal_id, const std::string& tenant_id,
    const std::string& source_id, const tensor::TensorLayerSummary& summary) {
  requireIdentity(principal_id);
  requireIdentity(tenant_id);
  requireIdentity(source_id);
  const auto tenant_prefix = "__adapters__:" + tenant_id + ":";
  if (tenant_id.find(':') != std::string::npos ||
      summary.scope_key.size() <= tenant_prefix.size() ||
      summary.scope_key.rfind(tenant_prefix, 0) != 0) {
    throw std::invalid_argument("projection scope does not match the authorized tenant");
  }
  for (const auto& candidate : summary.similar_adapters) {
    if (candidate.domain.empty() || candidate.base_model_id.empty() ||
        candidate.adapter_key != tenant_prefix + candidate.domain + ":" +
                                     candidate.base_model_id) {
      throw std::invalid_argument("projection candidate does not match the authorized tenant");
    }
  }

  const auto payload = GraphSummaryProjectionCodec::encode(summary);
  const auto digest = hash(std::string_view(
      reinterpret_cast<const char*>(payload.data()), payload.size()));
  const auto artifact = artifactId(principal_id, tenant_id, source_id, shard_id_,
                                   node_id_, summary.scope_key, digest);
  nlohmann::json record = {
      {"format", 1}, {"principal", principal_id}, {"tenant", tenant_id},
      {"source", source_id}, {"shard", shard_id_}, {"node", node_id_},
      {"scope", summary.scope_key}, {"digest", digest}, {"size", payload.size()},
      {"artifact", artifact}};
  std::string serialized;
  try {
    serialized = record.dump();
  } catch (const nlohmann::json::exception& error) {
    throw std::invalid_argument(std::string("invalid projection identity: ") + error.what());
  }
  if (serialized.size() > kMaxManifestBytes || serialized.size() > max_fragment_bytes_) {
    throw std::invalid_argument("projection manifest exceeds its size limit");
  }
  const std::vector<uint8_t> bytes(serialized.begin(), serialized.end());
  const auto key = manifestKey(principal_id, artifact);
  const auto outcome = fragments_->put(principal_id, shard_id_, artifact, payload);
  if (outcome != sharding::ShardArtifactFragmentStore::PutResult::Stored &&
      outcome != sharding::ShardArtifactFragmentStore::PutResult::Unchanged) {
    throw std::runtime_error("projection fragment could not be persisted");
  }
  auto transaction = database_->beginDurableTransaction(
      RocksDBWrapper::TransactionIsolationLevel::Snapshot);
  if (!transaction || !transaction->isActive() || !transaction->getForUpdate(key)) {
    throw std::runtime_error("projection manifest transaction unavailable");
  }
  const auto existing = transaction->getChecked(key);
  if (!existing) {
    throw std::runtime_error("projection manifest read failed: " + existing.error().message());
  }
  if (*existing && **existing != bytes) {
    throw std::runtime_error("projection manifest conflicts with existing record");
  }
  if (!transaction->put(key, bytes) || !transaction->commit()) {
    throw std::runtime_error("projection manifest commit failed");
  }
  const auto manifest_outcome =
      fragments_->put(principal_id, shard_id_, manifestArtifact(artifact), bytes);
  if (manifest_outcome != sharding::ShardArtifactFragmentStore::PutResult::Stored &&
      manifest_outcome != sharding::ShardArtifactFragmentStore::PutResult::Unchanged) {
    throw std::runtime_error("projection manifest publication failed");
  }
  return makeManifest(artifact, source_id, shard_id_, node_id_, digest, payload.size());
}

ArtifactManifest LocalProjectionPublisher::publishFromGraph(
    const tensor::TensorFingerprintGraph& graph, const std::string& principal_id,
    const std::string& tenant_id, const std::string& source_id,
    const std::string& domain, const std::string& base_model_id, std::size_t top_k) {
  requireIdentity(principal_id);
  requireIdentity(tenant_id);
  requireIdentity(source_id);
  if (tenant_id.find(':') != std::string::npos || domain.empty() ||
      base_model_id.empty() || domain.size() > GraphSummaryProjectionCodec::kMaxTextBytes ||
      base_model_id.size() > GraphSummaryProjectionCodec::kMaxTextBytes ||
      top_k == 0 || top_k > GraphSummaryProjectionCodec::kMaxCandidateCount) {
    throw std::invalid_argument("invalid projection graph query");
  }
  tensor::TensorLayerSummary summary;
  summary.scope_key = "__adapters__:" + tenant_id + ":" + domain + ":" + base_model_id;
  summary.layer_kind = tensor::TensorLayerKind::FingerprintSummary;
  auto snapshot = graph.snapshotSimilarForTenant(summary.scope_key, tenant_id, top_k);
  if (!snapshot) {
    throw std::runtime_error("projection graph query adapter not found");
  }
  if (snapshot->query_domain != domain || snapshot->query_base_model_id != base_model_id) {
    throw std::runtime_error("projection graph query metadata mismatch");
  }
  summary.similar_adapters = std::move(snapshot->candidates);
  summary.candidate_count = summary.similar_adapters.size();
  return publish(principal_id, tenant_id, source_id, summary);
}

std::optional<ArtifactManifest> LocalProjectionPublisher::load(
    const std::string& principal_id, const std::string& artifact_id) const {
  requireIdentity(principal_id);
  requireIdentity(artifact_id);
  auto transaction = database_->beginTransaction(
      RocksDBWrapper::TransactionIsolationLevel::Snapshot);
  if (!transaction || !transaction->isActive()) {
    throw std::runtime_error("projection manifest read transaction unavailable");
  }
  const auto stored = transaction->getChecked(manifestKey(principal_id, artifact_id));
  transaction->rollback();
  if (!stored) {
    throw std::runtime_error("projection manifest read failed: " + stored.error().message());
  }
  if (!*stored) {
    return std::nullopt;
  }
  const auto record =
      decodeManifest(**stored, principal_id, shard_id_, node_id_, artifact_id, max_fragment_bytes_);
  const auto fragment = fragments_->read(record.principal, record.shard, artifact_id, record.size);
  if (!fragment || fragment->bytes.size() != record.size || fragment->sha256 != record.digest) {
    throw std::runtime_error("projection manifest fragment is unavailable or inconsistent");
  }
  validateProjection(fragment->bytes, record);
  return makeManifest(artifact_id, record.source, record.shard, record.node,
                      record.digest, record.size);
}

ArtifactManifest LocalProjectionPublisher::loadRemote(
    sharding::ShardRPCClient& client, const std::string& principal_id,
    const std::string& tenant_id, const std::string& shard_id,
    const std::string& node_id, const std::string& artifact_id,
    uint64_t max_projection_bytes, uint32_t timeout_ms,
    const std::string& correlation_id) {
  requireIdentity(principal_id);
  requireIdentity(tenant_id);
  requireIdentity(shard_id);
  requireIdentity(node_id);
  requireIdentity(artifact_id);
  if (tenant_id.find(':') != std::string::npos || max_projection_bytes == 0 ||
      max_projection_bytes > (1ULL << 30) || timeout_ms == 0 || timeout_ms > 30000 ||
      correlation_id.size() > 256) {
    throw std::invalid_argument("invalid projection manifest request");
  }
  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(timeout_ms);
  const auto payload = client.fetchTensorFragment(
      node_id, shard_id, manifestArtifact(artifact_id), kMaxManifestBytes,
      timeout_ms, correlation_id);
  if (payload.bytes.empty()) {
    throw std::runtime_error("projection manifest response is empty");
  }
  const auto digest = hash(std::string_view(
      reinterpret_cast<const char*>(payload.bytes.data()), payload.bytes.size()));
  if (digest != payload.sha256) {
    throw std::runtime_error("projection manifest response digest mismatch");
  }
  const auto record =
      decodeManifest(payload.bytes, principal_id, shard_id, node_id, artifact_id,
                     max_projection_bytes);
  if (record.tenant != tenant_id) {
    throw std::runtime_error("projection manifest tenant mismatch");
  }
  const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline - std::chrono::steady_clock::now()).count();
  if (remaining <= 0) {
    throw std::runtime_error("projection manifest retrieval deadline exceeded");
  }
  const auto fragment = client.fetchTensorFragment(
      node_id, shard_id, artifact_id, record.size,
      static_cast<uint32_t>(remaining), correlation_id);
  if (fragment.bytes.size() != record.size || fragment.sha256 != record.digest ||
      hash(std::string_view(reinterpret_cast<const char*>(fragment.bytes.data()),
                            fragment.bytes.size())) != record.digest) {
    throw std::runtime_error("projection manifest fragment integrity mismatch");
  }
  validateProjection(fragment.bytes, record);
  return makeManifest(artifact_id, record.source, record.shard, record.node,
                      record.digest, record.size);
}

}  // namespace themis::distributed_tensor
