#pragma once

#include <memory>
#include <string>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

struct TrainingData;
struct LoRAHyperparameters;
struct LoRAJobInfo {
    std::string job_id;
    std::chrono::system_clock::time_point started_at;
};

class ILoRAOrchestrator {
public:
    virtual ~ILoRAOrchestrator() = default;
    virtual std::string createAdapter(const std::string& adapter_id,
                                      const TrainingData& data,
                                      const LoRAHyperparameters& params,
                                      bool async) = 0;
    virtual std::optional<LoRAJobInfo> getJob(const std::string& job_id) = 0;
    virtual bool isLoaded(const std::string& adapter_id) const = 0;
    virtual void loadAdapter(const std::string& adapter_id, bool async) = 0;
    virtual bool verifyAuditChain(const std::string& adapter_id) const = 0;

    // Lightweight JSON-based accessors to avoid heavy type deps in callers
    virtual std::optional<nlohmann::json> getAdapter(const std::string& adapter_id) const = 0;
    virtual std::vector<nlohmann::json> searchAdapters(const nlohmann::json& criteria) const = 0;
    virtual std::vector<std::string> getVersions(const std::string& adapter_id) const = 0;
    virtual std::optional<nlohmann::json> getProvenanceRecord(const std::string& adapter_id) const = 0;
    virtual std::vector<nlohmann::json> getInferenceAuditLog(const std::string& adapter_id) const = 0;
    virtual std::vector<nlohmann::json> listAdapterSnapshots(const std::string& adapter_id) const = 0;
};

} // namespace lora
} // namespace llm
} // namespace themis
