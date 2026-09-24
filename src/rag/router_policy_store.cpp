// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "router_policy_store.h"

#include <cstring>
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>

namespace themis::rag {

using json = nlohmann::json;

RouterPolicyStore::RouterPolicyStore(const std::string& db_path)
    : db_path_(db_path) {
  rocksdb::Options options;
  options.create_if_missing = true;
  
  rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_);
  if (!status.ok()) {
    throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
  }
}

RouterPolicyStore::~RouterPolicyStore() {
  if (db_) {
    delete db_;
    db_ = nullptr;
  }
}

std::optional<RouterPolicyStore::PolicySpec>
RouterPolicyStore::GetCurrentPolicy(QueryIntentClassifier::Intent intent) {
  // Get active version number
  std::string version_key = "active_policy_version_" + 
      std::to_string(static_cast<uint8_t>(intent));
  std::string version_str;
  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), version_key, 
                                     &version_str);
  
  if (!status.ok() || version_str.empty()) {
    return std::nullopt;
  }

  uint32_t version = std::stoul(version_str);
  return GetPolicyByVersion(intent, version);
}

std::optional<RouterPolicyStore::PolicySpec>
RouterPolicyStore::GetPolicyByVersion(QueryIntentClassifier::Intent intent,
                                      uint32_t version) {
  std::string policy_key = MakePolicyKey(intent, version);
  std::string policy_json;
  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), policy_key,
                                     &policy_json);
  
  if (!status.ok() || policy_json.empty()) {
    return std::nullopt;
  }

  try {
    auto j = json::parse(policy_json);
    PolicySpec spec;
    spec.version = j["version"];
    spec.intent = static_cast<QueryIntentClassifier::Intent>(j["intent"]);
    spec.lexical_weight = j["lexical_weight"];
    spec.dense_weight = j["dense_weight"];
    spec.graph_weight = j["graph_weight"];
    spec.created_at = j["created_at"];
    spec.baseline_ndcg = j["baseline_ndcg"];
    spec.confidence = j["confidence"];
    spec.description = j.value("description", "");
    return spec;
  } catch (const json::exception& e) {
    return std::nullopt;
  }
}

bool RouterPolicyStore::UpdatePolicy(const PolicySpec& new_policy) {
  // Create JSON representation
  json j;
  j["version"] = new_policy.version;
  j["intent"] = static_cast<uint8_t>(new_policy.intent);
  j["lexical_weight"] = new_policy.lexical_weight;
  j["dense_weight"] = new_policy.dense_weight;
  j["graph_weight"] = new_policy.graph_weight;
  j["created_at"] = new_policy.created_at;
  j["baseline_ndcg"] = new_policy.baseline_ndcg;
  j["confidence"] = new_policy.confidence;
  j["description"] = new_policy.description;

  std::string policy_key = MakePolicyKey(new_policy.intent, new_policy.version);
  rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), policy_key,
                                     j.dump());
  
  if (!status.ok()) {
    return false;
  }

  // Update active version
  std::string version_key = "active_policy_version_" + 
      std::to_string(static_cast<uint8_t>(new_policy.intent));
  status = db_->Put(rocksdb::WriteOptions(), version_key,
                   std::to_string(new_policy.version));
  
  return status.ok();
}

bool RouterPolicyStore::RollbackPolicy(QueryIntentClassifier::Intent intent,
                                       uint32_t target_version) {
  // Verify target version exists
  auto target_policy = GetPolicyByVersion(intent, target_version);
  if (!target_policy) {
    return false;
  }

  // Set as active version
  std::string version_key = "active_policy_version_" + 
      std::to_string(static_cast<uint8_t>(intent));
  rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), version_key,
                                     std::to_string(target_version));
  
  if (!status.ok()) {
    return false;
  }

  // Add history entry
  HistoryEntry entry;
  entry.version = target_version;
  entry.intent = intent;
  entry.timestamp_us = 
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
  entry.decision = "rollback";
  entry.reason = "User initiated rollback";

  json j;
  j["version"] = entry.version;
  j["intent"] = static_cast<uint8_t>(entry.intent);
  j["timestamp_us"] = entry.timestamp_us;
  j["decision"] = entry.decision;
  j["reason"] = entry.reason;

  std::string history_key = MakeHistoryKey(entry.timestamp_us);
  status = db_->Put(rocksdb::WriteOptions(), history_key, j.dump());

  return status.ok();
}

bool RouterPolicyStore::RecordMetrics(
    const std::string& decision_id,
    QueryIntentClassifier::Intent intent,
    float ndcg_at_10,
    float latency_ms,
    float cost_usd) {
  // Store metrics under intent + current time bucket (hourly)
  int64_t now_us = 
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
  int64_t hour_bucket = now_us / (3600 * 1000000);  // Round to hour

  std::string metrics_key = "metrics:" + std::to_string(static_cast<uint8_t>(intent)) +
      ":" + std::to_string(hour_bucket);

  // Read existing metrics for this hour
  std::string metrics_json;
  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), metrics_key,
                                     &metrics_json);

  json j;
  if (status.ok() && !metrics_json.empty()) {
    try {
      j = json::parse(metrics_json);
    } catch (const json::exception&) {
      j = json::object();
    }
  }

  // Accumulate metrics
  if (!j.contains("samples")) {
    j["samples"] = 0;
    j["sum_ndcg"] = 0.0;
    j["sum_latency"] = 0.0;
    j["sum_cost"] = 0.0;
  }

  j["samples"] = j["samples"].get<uint64_t>() + 1;
  j["sum_ndcg"] = j["sum_ndcg"].get<float>() + ndcg_at_10;
  j["sum_latency"] = j["sum_latency"].get<float>() + latency_ms;
  j["sum_cost"] = j["sum_cost"].get<float>() + cost_usd;

  status = db_->Put(rocksdb::WriteOptions(), metrics_key, j.dump());
  return status.ok();
}

std::optional<RouterPolicyStore::MetricsSnapshot>
RouterPolicyStore::GetMetrics(
    QueryIntentClassifier::Intent intent,
    const std::optional<std::chrono::system_clock::time_point>& since_time) {
  // Aggregate metrics since specified time (or last hour if not specified)
  // This is a simplified implementation
  MetricsSnapshot snapshot;
  snapshot.sample_count = 0;
  snapshot.mean_ndcg_at_10 = 0.0f;
  snapshot.mean_latency_ms = 0.0f;
  snapshot.mean_cost_usd = 0.0f;

  // TODO: Scan RocksDB for metrics in time range
  // Sum up all samples and compute means

  if (snapshot.sample_count == 0) {
    return std::nullopt;
  }

  snapshot.mean_ndcg_at_10 /= snapshot.sample_count;
  snapshot.mean_latency_ms /= snapshot.sample_count;
  snapshot.mean_cost_usd /= snapshot.sample_count;

  return snapshot;
}

std::vector<RouterPolicyStore::HistoryEntry>
RouterPolicyStore::GetHistory(QueryIntentClassifier::Intent intent) {
  std::vector<HistoryEntry> history;
  // TODO: Scan policy_history CF for entries matching intent
  // Sort by timestamp (oldest first)
  return history;
}

std::map<QueryIntentClassifier::Intent, RouterPolicyStore::PolicySpec>
RouterPolicyStore::GetAllPolicies() {
  std::map<QueryIntentClassifier::Intent, PolicySpec> policies;
  
  // Get policies for all 4 intent categories
  for (int i = 0; i < 4; ++i) {
    auto policy = GetCurrentPolicy(static_cast<QueryIntentClassifier::Intent>(i));
    if (policy) {
      policies[policy->intent] = *policy;
    }
  }

  return policies;
}

bool RouterPolicyStore::IsHealthy() const {
  // Quick health check: can we read from DB?
  std::string dummy;
  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), "health_check",
                                     &dummy);
  return status.ok() || status.IsNotFound();
}

std::string RouterPolicyStore::MakePolicyKey(QueryIntentClassifier::Intent intent,
                                             uint32_t version) {
  return "policy:" + std::to_string(static_cast<uint8_t>(intent)) + ":" +
         std::to_string(version);
}

std::string RouterPolicyStore::MakeMetricsKey(
    QueryIntentClassifier::Intent intent) {
  return "metrics:" + std::to_string(static_cast<uint8_t>(intent));
}

std::string RouterPolicyStore::MakeHistoryKey(int64_t timestamp_us) {
  // Big-endian encoding for sorted iteration
  unsigned char buf[8];
  buf[0] = (timestamp_us >> 56) & 0xFF;
  buf[1] = (timestamp_us >> 48) & 0xFF;
  buf[2] = (timestamp_us >> 40) & 0xFF;
  buf[3] = (timestamp_us >> 32) & 0xFF;
  buf[4] = (timestamp_us >> 24) & 0xFF;
  buf[5] = (timestamp_us >> 16) & 0xFF;
  buf[6] = (timestamp_us >> 8) & 0xFF;
  buf[7] = timestamp_us & 0xFF;
  
  return std::string("history:") + std::string(reinterpret_cast<char*>(buf), 8);
}

}  // namespace themis::rag
