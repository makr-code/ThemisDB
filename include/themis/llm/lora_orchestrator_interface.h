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
    /**
     * @brief TBD: Describe ~ILoRAOrchestrator.
     * @return Return value.
     */
    virtual ~ILoRAOrchestrator() = default;
    /**
     * @brief TBD: Describe createAdapter.
     * @param[in] adapter_id Input parameter.
     * @param[in] data Input parameter.
     * @param[in] params Input parameter.
     * @param[in] async Input parameter.
     * @return Return value.
     */
    virtual std::string createAdapter(const std::string& adapter_id,
                                      const TrainingData& data,
                                      const LoRAHyperparameters& params,
                                      bool async) = 0;
    /**
     * @brief TBD: Describe getJob.
     * @param[in] job_id Input parameter.
     * @return Return value.
     */
    virtual std::optional<LoRAJobInfo> getJob(const std::string& job_id) = 0;
    /**
     * @brief TBD: Describe isLoaded.
     * @param[in] adapter_id Input parameter.
     * @return True on success.
     */
    virtual bool isLoaded(const std::string& adapter_id) const = 0;
    /**
     * @brief TBD: Describe loadAdapter.
     * @param[in] adapter_id Input parameter.
     * @param[in] async Input parameter.
     */
    virtual void loadAdapter(const std::string& adapter_id, bool async) = 0;
    /**
     * @brief TBD: Describe verifyAuditChain.
     * @param[in] adapter_id Input parameter.
     * @return True on success.
     */
    virtual bool verifyAuditChain(const std::string& adapter_id) const = 0;

    /**
     * @brief Lightweight JSON-based accessors to avoid heavy type deps in callers
     * @param[in] adapter_id Input parameter.
     * @return Return value.
     */
    virtual std::optional<nlohmann::json> getAdapter(const std::string& adapter_id) const = 0;
    /**
     * @brief TBD: Describe searchAdapters.
     * @param[in] criteria Input parameter.
     * @return Return value.
     */
    virtual std::vector<nlohmann::json> searchAdapters(const nlohmann::json& criteria) const = 0;
    /**
     * @brief TBD: Describe getVersions.
     * @param[in] adapter_id Input parameter.
     * @return Return value.
     */
    virtual std::vector<std::string> getVersions(const std::string& adapter_id) const = 0;
    /**
     * @brief TBD: Describe getProvenanceRecord.
     * @param[in] adapter_id Input parameter.
     * @return Return value.
     */
    virtual std::optional<nlohmann::json> getProvenanceRecord(const std::string& adapter_id) const = 0;
    /**
     * @brief TBD: Describe getInferenceAuditLog.
     * @param[in] adapter_id Input parameter.
     * @return Return value.
     */
    virtual std::vector<nlohmann::json> getInferenceAuditLog(const std::string& adapter_id) const = 0;
    /**
     * @brief TBD: Describe listAdapterSnapshots.
     * @param[in] adapter_id Input parameter.
     * @return Return value.
     */
    virtual std::vector<nlohmann::json> listAdapterSnapshots(const std::string& adapter_id) const = 0;
};

} // namespace lora
} // namespace llm
} // namespace themis
