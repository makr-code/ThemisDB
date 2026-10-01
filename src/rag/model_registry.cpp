/**
 * @file model_registry.cpp
 * @brief Model Registry implementation
 *
 * Provides model version tracking, lifecycle management, and persistence.
 */

#include "rag/model_registry.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>

#include <nlohmann/json.hpp>

namespace themis::rag::lifecycle {

namespace {

nlohmann::json ModelMetadataToJson(const ModelMetadata& metadata) {
  nlohmann::json payload;
  payload["version"] = metadata.version;
  payload["model_id"] = metadata.model_id;
  payload["built_at_us"] = metadata.built_at_us;
  payload["status_changed_at_us"] = metadata.status_changed_at_us;
  payload["status"] = StatusToString(metadata.status);
  payload["parent_version"] = metadata.parent_version;
  payload["training_dataset_id"] = metadata.training_dataset_id;
  payload["metrics_json"] = metadata.metrics_json;
  payload["cost_stats_json"] = metadata.cost_stats_json;
  payload["notes"] = metadata.notes;
  payload["model_checksum"] = metadata.model_checksum;
  payload["model_location"] = metadata.model_location;
  return payload;
}

std::optional<ModelMetadata> JsonToModelMetadata(const nlohmann::json& json_value) {
  if (!json_value.is_object()) {
    return std::nullopt;
  }

  ModelMetadata metadata;
  metadata.version = json_value.value("version", 0u);
  if (metadata.version == 0) {
    return std::nullopt;
  }

  metadata.model_id = json_value.value("model_id", "");
  metadata.built_at_us = json_value.value("built_at_us", 0ull);
  metadata.status_changed_at_us = json_value.value("status_changed_at_us", 0ull);

  const auto status_str = json_value.value("status", "draft");
  const auto parsed_status = StringToStatus(status_str);
  if (!parsed_status.has_value()) {
    return std::nullopt;
  }
  metadata.status = *parsed_status;

  metadata.parent_version = json_value.value("parent_version", 0u);
  metadata.training_dataset_id = json_value.value("training_dataset_id", "");
  metadata.metrics_json = json_value.value("metrics_json", "");
  metadata.cost_stats_json = json_value.value("cost_stats_json", "");
  metadata.notes = json_value.value("notes", "");
  metadata.model_checksum = json_value.value("model_checksum", "");
  metadata.model_location = json_value.value("model_location", "");
  return metadata;
}

}  // namespace

std::string StatusToString(ModelStatus status) {
  switch (status) {
    case ModelStatus::kDraft:
      return "draft";
    case ModelStatus::kValidated:
      return "validated";
    case ModelStatus::kCandidate:
      return "candidate";
    case ModelStatus::kDeployed:
      return "deployed";
    case ModelStatus::kRetired:
      return "retired";
    case ModelStatus::kFailed:
      return "failed";
  }
  return "unknown";
}

std::optional<ModelStatus> StringToStatus(const std::string& status_str) {
  if (status_str == "draft") return ModelStatus::kDraft;
  if (status_str == "validated") return ModelStatus::kValidated;
  if (status_str == "candidate") return ModelStatus::kCandidate;
  if (status_str == "deployed") return ModelStatus::kDeployed;
  if (status_str == "retired") return ModelStatus::kRetired;
  if (status_str == "failed") return ModelStatus::kFailed;
  return std::nullopt;
}

struct ModelRegistry::Impl {
  std::string persistence_path;
  std::map<uint32_t, ModelMetadata> models;  // version -> metadata
  uint32_t next_version = 1;
  mutable std::mutex mu;

  uint64_t GetCurrentTimestampUs() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<microseconds>(duration).count();
  }
};

ModelRegistry::ModelRegistry(const std::string& persistence_path)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->persistence_path = persistence_path;
  Load();
}

ModelRegistry::~ModelRegistry() = default;

uint32_t ModelRegistry::RegisterModel(const std::string& model_id,
                                      const std::string& training_dataset_id,
                                      const std::string& metrics_json,
                                      const std::string& cost_stats_json,
                                      const std::string& model_location,
                                      uint32_t parent_version) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  ModelMetadata meta;
  meta.version = pimpl_->next_version++;
  meta.model_id = model_id;
  meta.built_at_us = pimpl_->GetCurrentTimestampUs();
  meta.status_changed_at_us = meta.built_at_us;
  meta.status = ModelStatus::kDraft;
  meta.parent_version = parent_version;
  meta.training_dataset_id = training_dataset_id;
  meta.metrics_json = metrics_json;
  meta.cost_stats_json = cost_stats_json;
  meta.model_location = model_location;
  meta.notes = "Model created";

  // Placeholder: compute checksum from model_location
  meta.model_checksum = "checksum_" + std::to_string(meta.version);

  auto version = meta.version;
  pimpl_->models[version] = meta;

  // Persist immediately
  Persist();

  return version;
}

bool ModelRegistry::UpdateModelStatus(uint32_t version, ModelStatus new_status,
                                      const std::string& notes) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  auto it = pimpl_->models.find(version);
  if (it == pimpl_->models.end()) {
    return false;
  }

  // Validate state transition
  ModelStatus old_status = it->second.status;

  // Define valid transitions:
  // draft → validated, failed
  // validated → candidate, failed
  // candidate → deployed, failed
  // deployed → retired, failed
  // failed → any (recovery)
  // retired → (terminal, no transitions)

  bool valid_transition = false;
  if (old_status == ModelStatus::kRetired) {
    valid_transition = false;
  } else if (new_status == ModelStatus::kFailed) {
    valid_transition = true;
  } else if (old_status == ModelStatus::kDraft &&
             (new_status == ModelStatus::kValidated)) {
    valid_transition = true;
  } else if (old_status == ModelStatus::kValidated &&
             (new_status == ModelStatus::kCandidate)) {
    valid_transition = true;
  } else if (old_status == ModelStatus::kCandidate &&
             (new_status == ModelStatus::kDeployed)) {
    valid_transition = true;
  } else if (old_status == ModelStatus::kDeployed &&
             (new_status == ModelStatus::kRetired)) {
    valid_transition = true;
  }

  if (!valid_transition) {
    return false;
  }

  it->second.status = new_status;
  it->second.status_changed_at_us = pimpl_->GetCurrentTimestampUs();
  it->second.notes = notes.empty() ? StatusToString(new_status) : notes;

  // Persist immediately
  Persist();

  return true;
}

std::optional<ModelMetadata> ModelRegistry::GetDeployedModel() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  for (auto it = pimpl_->models.rbegin(); it != pimpl_->models.rend(); ++it) {
    if (it->second.status == ModelStatus::kDeployed) {
      return it->second;
    }
  }
  return std::nullopt;
}

std::optional<ModelMetadata> ModelRegistry::GetByVersion(uint32_t version) const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  auto it = pimpl_->models.find(version);
  if (it != pimpl_->models.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<ModelMetadata> ModelRegistry::GetLatest() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  if (pimpl_->models.empty()) {
    return std::nullopt;
  }
  return pimpl_->models.rbegin()->second;
}

std::vector<ModelMetadata> ModelRegistry::GetByStatus(ModelStatus status) const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  std::vector<ModelMetadata> result;
  for (const auto& [version, metadata] : pimpl_->models) {
    if (metadata.status == status) {
      result.push_back(metadata);
    }
  }
  return result;
}

std::vector<ModelMetadata> ModelRegistry::GetLineage(uint32_t version) const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  std::vector<ModelMetadata> result;
  uint32_t current = version;

  while (current > 0) {
    auto it = pimpl_->models.find(current);
    if (it == pimpl_->models.end()) {
      break;
    }
    result.push_back(it->second);
    current = it->second.parent_version;
  }

  return result;
}

bool ModelRegistry::Exists(uint32_t version) const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return pimpl_->models.count(version) > 0;
}

uint32_t ModelRegistry::Count() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return pimpl_->models.size();
}

bool ModelRegistry::Persist() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  if (pimpl_->persistence_path.empty()) {
    return false;
  }

  try {
    std::filesystem::path path(pimpl_->persistence_path);
    if (path.has_parent_path()) {
      std::filesystem::create_directories(path.parent_path());
    }

    nlohmann::json payload = nlohmann::json::object();
    payload["version"] = 1;
    payload["next_version"] = pimpl_->next_version;
    payload["models"] = nlohmann::json::array();

    for (const auto& [version, metadata] : pimpl_->models) {
      payload["models"].push_back(ModelMetadataToJson(metadata));
    }

    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output.is_open()) {
      return false;
    }
    output << payload.dump(2);
    output.flush();
    return output.good();
  } catch (const std::exception&) {
    return false;
  }
}

bool ModelRegistry::Load() {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  if (pimpl_->persistence_path.empty()) {
    return true;
  }

  std::ifstream input(pimpl_->persistence_path);
  if (!input.is_open()) {
    pimpl_->models.clear();
    pimpl_->next_version = 1;
    return true;
  }

  try {
    nlohmann::json payload;
    input >> payload;
    if (!payload.is_object()) {
      pimpl_->models.clear();
      pimpl_->next_version = 1;
      return false;
    }

    const int version = payload.value("version", 1);
    if (version != 1) {
      return false;
    }

    pimpl_->models.clear();
    pimpl_->next_version = payload.value("next_version", 1u);

    const auto models = payload.value("models", nlohmann::json::array());
    if (!models.is_array()) {
      return false;
    }

    for (const auto& model_json : models) {
      auto metadata = JsonToModelMetadata(model_json);
      if (!metadata.has_value()) {
        return false;
      }
      pimpl_->models[metadata->version] = *metadata;
    }

    if (pimpl_->models.empty()) {
      pimpl_->next_version = 1;
    } else {
      const auto max_version = std::max_element(
          pimpl_->models.begin(), pimpl_->models.end(),
          [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
          });
      pimpl_->next_version = max_version->first + 1;
    }
    return true;
  } catch (const std::exception&) {
    pimpl_->models.clear();
    pimpl_->next_version = 1;
    return false;
  }
}

std::string ModelRegistry::ToJson() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  nlohmann::json payload = nlohmann::json::object();
  payload["metadata"] = {
      {"total_models", pimpl_->models.size()},
      {"next_version", pimpl_->next_version}};
  payload["models"] = nlohmann::json::array();

  for (const auto& [version, metadata] : pimpl_->models) {
    payload["models"].push_back(ModelMetadataToJson(metadata));
  }

  return payload.dump(2);
}

}  // namespace themis::rag::lifecycle
