// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/router_policy_store.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/write_batch.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "utils/rocksdb_open_compat.h"

namespace themis::rag {

using json = nlohmann::json;

namespace {

constexpr const char* kPolicyNamespace = "policy";
constexpr const char* kMetricsNamespace = "metrics";
constexpr const char* kHistoryNamespace = "history";
constexpr const char* kCurrentVersionKey = "active_policy_version";
constexpr const char* kLoadVersionKey = "load_state";

std::string intentToString(QueryIntentClassifier::Intent intent) {
  switch (intent) {
    case QueryIntentClassifier::Intent::Factual:
     return "factual";
    case QueryIntentClassifier::Intent::Temporal:
     return "temporal";
    case QueryIntentClassifier::Intent::MultiHop:
     return "multi_hop";
    case QueryIntentClassifier::Intent::Comparison:
     return "comparison";
  }
  return "unknown";
}

std::string currentTimestampIso() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_MSC_VER)
  localtime_s(&tm, &time);
#else
  localtime_r(&time, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

std::string policyKey(QueryIntentClassifier::Intent intent, uint32_t version) {
  return std::string(kPolicyNamespace) + ":" + intentToString(intent) + ":" +
        std::to_string(version);
}

std::string metricsKey(QueryIntentClassifier::Intent intent, uint32_t version) {
  return std::string(kMetricsNamespace) + ":" + intentToString(intent) + ":" +
        std::to_string(version);
}

std::string historyKey(QueryIntentClassifier::Intent intent, int64_t timestamp_us) {
  return std::string(kHistoryNamespace) + ":" + intentToString(intent) + ":" +
        std::to_string(timestamp_us);
}

std::string loadKey(QueryIntentClassifier::Intent intent) {
  return std::string("current:") + intentToString(intent);
}

std::optional<RouterPolicyStore::PolicySpec> deserializePolicy(const std::string& payload) {
  try {
    const auto j = json::parse(payload);
    RouterPolicyStore::PolicySpec policy{};
    policy.version = j.value("version", 0u);
    const std::string intent_name = j.value("intent", "factual");
    if (intent_name == "factual") {
     policy.intent = QueryIntentClassifier::Intent::Factual;
    } else if (intent_name == "temporal") {
     policy.intent = QueryIntentClassifier::Intent::Temporal;
    } else if (intent_name == "multi_hop") {
     policy.intent = QueryIntentClassifier::Intent::MultiHop;
    } else if (intent_name == "comparison") {
     policy.intent = QueryIntentClassifier::Intent::Comparison;
    }
    policy.lexical_weight = j.value("lexical_weight", 0.0f);
    policy.dense_weight = j.value("dense_weight", 0.0f);
    policy.graph_weight = j.value("graph_weight", 0.0f);
    policy.created_at = j.value("created_at", "");
    policy.baseline_ndcg = j.value("baseline_ndcg", 0.0f);
    policy.confidence = j.value("confidence", 1.0f);
    policy.description = j.value("description", "");
    return policy;
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

std::string serializePolicy(const RouterPolicyStore::PolicySpec& policy) {
  json j;
  j["version"] = policy.version;
  j["intent"] = intentToString(policy.intent);
  j["lexical_weight"] = policy.lexical_weight;
  j["dense_weight"] = policy.dense_weight;
  j["graph_weight"] = policy.graph_weight;
  j["created_at"] = policy.created_at;
  j["baseline_ndcg"] = policy.baseline_ndcg;
  j["confidence"] = policy.confidence;
  j["description"] = policy.description;
  return j.dump();
}

std::optional<RouterPolicyStore::HistoryEntry> deserializeHistoryEntry(const std::string& payload) {
  try {
    const auto j = json::parse(payload);
    RouterPolicyStore::HistoryEntry entry{};
    entry.version = j.value("version", 0u);
    const std::string intent_name = j.value("intent", "factual");
    if (intent_name == "factual") {
     entry.intent = QueryIntentClassifier::Intent::Factual;
    } else if (intent_name == "temporal") {
     entry.intent = QueryIntentClassifier::Intent::Temporal;
    } else if (intent_name == "multi_hop") {
     entry.intent = QueryIntentClassifier::Intent::MultiHop;
    } else if (intent_name == "comparison") {
     entry.intent = QueryIntentClassifier::Intent::Comparison;
    }
    entry.timestamp_us = j.value("timestamp_us", int64_t{0});
    entry.decision = j.value("decision", "created");
    entry.metrics_delta = j.value("metrics_delta", 0.0f);
    entry.reason = j.value("reason", "policy update");
    return entry;
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

std::string serializeHistoryEntry(const RouterPolicyStore::HistoryEntry& entry) {
  json j;
  j["version"] = entry.version;
  j["intent"] = intentToString(entry.intent);
  j["timestamp_us"] = entry.timestamp_us;
  j["decision"] = entry.decision;
  j["metrics_delta"] = entry.metrics_delta;
  j["reason"] = entry.reason;
  return j.dump();
}

std::optional<RouterPolicyStore::MetricsSnapshot> deserializeMetrics(const std::string& payload) {
  try {
    const auto j = json::parse(payload);
    RouterPolicyStore::MetricsSnapshot snapshot{};
    snapshot.mean_ndcg_at_10 = j.value("mean_ndcg_at_10", 0.0f);
    snapshot.mean_latency_ms = j.value("mean_latency_ms", 0.0f);
    snapshot.mean_cost_usd = j.value("mean_cost_usd", 0.0f);
    snapshot.sample_count = j.value("sample_count", uint64_t{0});
    snapshot.window_start = j.value("window_start", "");
    snapshot.window_end = j.value("window_end", "");
    return snapshot;
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

std::string serializeMetrics(const RouterPolicyStore::MetricsSnapshot& snapshot) {
  json j;
  j["mean_ndcg_at_10"] = snapshot.mean_ndcg_at_10;
  j["mean_latency_ms"] = snapshot.mean_latency_ms;
  j["mean_cost_usd"] = snapshot.mean_cost_usd;
  j["sample_count"] = snapshot.sample_count;
  j["window_start"] = snapshot.window_start;
  j["window_end"] = snapshot.window_end;
  return j.dump();
}

void persistState(rocksdb::DB* db,
                 const std::map<QueryIntentClassifier::Intent, PolicySpec>& current_policies,
                 const std::map<QueryIntentClassifier::Intent, std::vector<PolicySpec>>& policy_history) {
  if (db == nullptr) {
    return;
  }
  rocksdb::WriteBatch batch;
  for (const auto& [intent, policy] : current_policies) {
    batch.Put(loadKey(intent), serializePolicy(policy));
  }
  for (const auto& [intent, history] : policy_history) {
    for (const auto& policy : history) {
     batch.Put(policyKey(intent, policy.version), serializePolicy(policy));
    }
  }
  db->Write(rocksdb::WriteOptions(), &batch);
}

std::map<QueryIntentClassifier::Intent, std::vector<RouterPolicyStore::PolicySpec>>
loadHistory(rocksdb::DB* db) {
  std::map<QueryIntentClassifier::Intent, std::vector<RouterPolicyStore::PolicySpec>> result;
  if (db == nullptr) {
    return result;
  }
  std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
  for (it->Seek("policy:"); it->Valid(); it->Next()) {
    std::string key = it->key().ToString();
    if (key.rfind("policy:", 0) != 0) {
     continue;
    }
    auto policy = deserializePolicy(it->value().ToString());
    if (!policy.has_value()) {
     continue;
    }
    result[policy->intent].push_back(*policy);
  }
  return result;
}

std::map<QueryIntentClassifier::Intent, RouterPolicyStore::PolicySpec>
loadCurrentPolicies(rocksdb::DB* db) {
  std::map<QueryIntentClassifier::Intent, RouterPolicyStore::PolicySpec> result;
  if (db == nullptr) {
    return result;
  }
  std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
  for (it->Seek("current:"); it->Valid(); it->Next()) {
    std::string key = it->key().ToString();
    if (key.rfind("current:", 0) != 0) {
     continue;
    }
    auto policy = deserializePolicy(it->value().ToString());
    if (!policy.has_value()) {
     continue;
    }
    result[policy->intent] = *policy;
  }
  return result;
}

}  // namespace

RouterPolicyStore::RouterPolicyStore(const std::string& db_path)
    : db_(nullptr), db_path_(db_path) {
  std::filesystem::create_directories(db_path_);
  rocksdb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = false;
  rocksdb::DB* db_raw = nullptr;
  const auto status = themis::storage::detail::openDbCompat(options, db_path_, &db_raw);
  if (!status.ok()) {
    throw std::runtime_error("Failed to open RouterPolicyStore RocksDB: " + status.ToString());
  }
  db_ = db_raw;
  Load();
}

RouterPolicyStore::~RouterPolicyStore() {
  if (db_ == nullptr) {
    return;
  }
  auto* db = static_cast<rocksdb::DB*>(db_);
  db->FlushWAL(true);
  delete db;
  db_ = nullptr;
}

bool RouterPolicyStore::Load() {
  if (db_ == nullptr) {
    return false;
  }
  auto* db = static_cast<rocksdb::DB*>(db_);
  auto current = loadCurrentPolicies(db);
  auto history = loadHistory(db);
  if (!current.empty() || !history.empty()) {
    current_policies_ = std::move(current);
    policy_history_ = std::move(history);
  }
  return true;
}

std::optional<RouterPolicyStore::PolicySpec>
RouterPolicyStore::GetCurrentPolicy(QueryIntentClassifier::Intent intent) {
  auto it = current_policies_.find(intent);
  if (it != current_policies_.end()) {
    return it->second;
  }

  PolicySpec default_policy{};
  default_policy.version = 0;
  default_policy.intent = intent;
  default_policy.lexical_weight = 0.3f;
  default_policy.dense_weight = 0.5f;
  default_policy.graph_weight = 0.2f;
  default_policy.created_at = currentTimestampIso();
  default_policy.baseline_ndcg = 0.0f;
  default_policy.confidence = 1.0f;
  default_policy.description = "default policy";

  return default_policy;
}

std::optional<RouterPolicyStore::PolicySpec>
RouterPolicyStore::GetPolicyByVersion(
    QueryIntentClassifier::Intent intent,
    uint32_t version) {
  auto history_it = policy_history_.find(intent);
  if (history_it != policy_history_.end()) {
    for (const auto& policy : history_it->second) {
     if (policy.version == version) {
       return policy;
     }
    }
  }
  if (db_ != nullptr) {
    std::string key = policyKey(intent, version);
    std::string value;
    auto* db = static_cast<rocksdb::DB*>(db_);
    const auto status = db->Get(rocksdb::ReadOptions(), key, &value);
    if (status.ok()) {
     auto policy = deserializePolicy(value);
     if (policy.has_value()) {
       return *policy;
     }
    }
  }
  return std::nullopt;
}

bool RouterPolicyStore::UpdatePolicy(const PolicySpec& new_policy) {
  if (db_ == nullptr) {
    return false;
  }
  auto* db = static_cast<rocksdb::DB*>(db_);
  PolicySpec stored = new_policy;
  if (stored.created_at.empty()) {
    stored.created_at = currentTimestampIso();
  }
  if (stored.version == 0) {
    const auto existing = GetCurrentPolicy(stored.intent);
    stored.version = existing.has_value() ? existing->version + 1u : 1u;
  }
  if (policy_history_.find(stored.intent) == policy_history_.end()) {
    policy_history_[stored.intent] = {};
  }
  policy_history_[stored.intent].push_back(stored);
  current_policies_[stored.intent] = stored;

  rocksdb::WriteBatch batch;
  batch.Put(loadKey(stored.intent), serializePolicy(stored));
  batch.Put(policyKey(stored.intent, stored.version), serializePolicy(stored));
  HistoryEntry entry{};
  entry.version = stored.version;
  entry.intent = stored.intent;
  entry.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  entry.decision = "promoted";
  entry.metrics_delta = stored.baseline_ndcg;
  entry.reason = stored.description.empty() ? "policy update" : stored.description;
  batch.Put(historyKey(entry.intent, entry.timestamp_us), serializeHistoryEntry(entry));
  const auto status = db->Write(rocksdb::WriteOptions(), &batch);
  return status.ok();
}

bool RouterPolicyStore::RollbackPolicy(QueryIntentClassifier::Intent intent,
                                     uint32_t target_version) {
  auto history_it = policy_history_.find(intent);
  if (history_it == policy_history_.end()) {
    return false;
  }

  for (const auto& policy : history_it->second) {
    if (policy.version == target_version) {
     current_policies_[intent] = policy;
     if (db_ != nullptr) {
       auto* db = static_cast<rocksdb::DB*>(db_);
       rocksdb::WriteBatch batch;
       batch.Put(loadKey(intent), serializePolicy(policy));
       HistoryEntry entry{};
       entry.version = policy.version;
       entry.intent = intent;
       entry.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();
       entry.decision = "rolled_back";
       entry.metrics_delta = 0.0f;
       entry.reason = "rollback to previous policy";
       batch.Put(historyKey(intent, entry.timestamp_us), serializeHistoryEntry(entry));
       db->Write(rocksdb::WriteOptions(), &batch);
     }
     return true;
    }
  }

  return false;
}

bool RouterPolicyStore::RecordMetrics(
    const std::string& decision_id,
    QueryIntentClassifier::Intent intent,
    float ndcg_at_10,
    float latency_ms,
    float cost_usd) {
  if (db_ == nullptr || decision_id.empty()) {
    return false;
  }
  auto* db = static_cast<rocksdb::DB*>(db_);
  const auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  json j;
  j["decision_id"] = decision_id;
  j["intent"] = intentToString(intent);
  j["ndcg_at_10"] = ndcg_at_10;
  j["latency_ms"] = latency_ms;
  j["cost_usd"] = cost_usd;
  j["timestamp_us"] = now_us;
  const std::string key = std::string(kMetricsNamespace) + ":" + intentToString(intent) + ":" + decision_id;
  const auto status = db->Put(rocksdb::WriteOptions(), key, j.dump());
  if (!status.ok()) {
    return false;
  }
  const auto active = GetCurrentPolicy(intent);
  if (active.has_value()) {
    const auto version_key = metricsKey(intent, active->version);
    std::string existing;
    auto read_status = db->Get(rocksdb::ReadOptions(), version_key, &existing);
    MetricsSnapshot aggregate{};
    if (read_status.ok()) {
     auto parsed = deserializeMetrics(existing);
     if (parsed.has_value()) {
       aggregate = *parsed;
     }
    }
    const auto sample_count = aggregate.sample_count + 1u;
    aggregate.mean_ndcg_at_10 = ((aggregate.mean_ndcg_at_10 * static_cast<float>(aggregate.sample_count)) + ndcg_at_10) /
                               static_cast<float>(sample_count);
    aggregate.mean_latency_ms = ((aggregate.mean_latency_ms * static_cast<float>(aggregate.sample_count)) + latency_ms) /
                               static_cast<float>(sample_count);
    aggregate.mean_cost_usd = ((aggregate.mean_cost_usd * static_cast<float>(aggregate.sample_count)) + cost_usd) /
                             static_cast<float>(sample_count);
    aggregate.sample_count = sample_count;
    aggregate.window_start = std::to_string(now_us);
    aggregate.window_end = std::to_string(now_us);
    const auto write_status = db->Put(rocksdb::WriteOptions(), version_key, serializeMetrics(aggregate));
    return write_status.ok();
  }
  return true;
}

std::optional<RouterPolicyStore::MetricsSnapshot>
RouterPolicyStore::GetMetrics(
    QueryIntentClassifier::Intent intent,
    const std::optional<std::chrono::system_clock::time_point>& since_time) {
  if (db_ == nullptr) {
    return std::nullopt;
  }
  auto* db = static_cast<rocksdb::DB*>(db_);
  const auto current = GetCurrentPolicy(intent);
  if (!current.has_value()) {
    return std::nullopt;
  }
  const std::string key = metricsKey(intent, current->version);
  std::string value;
  const auto status = db->Get(rocksdb::ReadOptions(), key, &value);
  if (!status.ok()) {
    return std::nullopt;
  }
  auto snapshot = deserializeMetrics(value);
  if (!snapshot.has_value()) {
    return std::nullopt;
  }
  if (since_time.has_value()) {
    const auto cutoff = std::chrono::duration_cast<std::chrono::microseconds>(
                           since_time->time_since_epoch())
                           .count();
    if (cutoff > 0 && std::stoll(snapshot->window_start) < cutoff) {
     return std::nullopt;
    }
  }
  return snapshot;
}

std::vector<RouterPolicyStore::HistoryEntry>
RouterPolicyStore::GetHistory(QueryIntentClassifier::Intent intent) {
  std::vector<HistoryEntry> result;
  auto history_it = policy_history_.find(intent);
  if (history_it != policy_history_.end()) {
    result.reserve(history_it->second.size());
    for (const auto& policy : history_it->second) {
     HistoryEntry entry{};
     entry.version = policy.version;
     entry.intent = intent;
     entry.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count();
     entry.decision = "created";
     entry.metrics_delta = 0.0f;
     entry.reason = policy.description.empty() ? "policy update" : policy.description;
     result.push_back(entry);
    }
  }
  if (db_ == nullptr) {
    return result;
  }
  auto* db = static_cast<rocksdb::DB*>(db_);
  std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
  for (it->Seek("history:"); it->Valid(); it->Next()) {
    const std::string key = it->key().ToString();
    if (key.rfind("history:", 0) != 0) {
     continue;
    }
    if (key.find(intentToString(intent)) == std::string::npos) {
     continue;
    }
    auto decoded = deserializeHistoryEntry(it->value().ToString());
    if (decoded.has_value() && decoded->intent == intent) {
     result.push_back(*decoded);
    }
  }
  std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.timestamp_us < rhs.timestamp_us;
  });
  return result;
}

std::map<QueryIntentClassifier::Intent, RouterPolicyStore::PolicySpec>
RouterPolicyStore::GetAllPolicies() {
  return current_policies_;
}

bool RouterPolicyStore::IsHealthy() const {
  return db_ != nullptr;
}

std::string RouterPolicyStore::MakePolicyKey(
    QueryIntentClassifier::Intent intent,
    uint32_t version) {
  return policyKey(intent, version);
}

std::string RouterPolicyStore::MakeMetricsKey(
    QueryIntentClassifier::Intent intent) {
  return std::string(kMetricsNamespace) + ":" + intentToString(intent);
}

std::string RouterPolicyStore::MakeHistoryKey(int64_t timestamp_us) {
  return std::string(kHistoryNamespace) + ":" + std::to_string(timestamp_us);
}

}  // namespace themis::rag

