// Implementation of JSON (de)serialization for TaskExecutionResult
#include "scheduler/task_result_store.h"

namespace themis {
namespace scheduler {

nlohmann::json TaskExecutionResult::toJson() const {
    nlohmann::json j;
    j["task_id"] = task_id;
    j["task_name"] = task_name;
    j["timestamp_ms"] = timestamp_ms;
    j["duration_ms"] = duration_ms;
    j["success"] = success;
    j["output"] = output;
    j["error"] = error;
    return j;
}

TaskExecutionResult TaskExecutionResult::fromJson(const nlohmann::json& j) {
    TaskExecutionResult r;
    if (j.contains("task_id")) r.task_id = j.at("task_id").get<std::string>();
    if (j.contains("task_name")) r.task_name = j.at("task_name").get<std::string>();
    if (j.contains("timestamp_ms")) r.timestamp_ms = j.at("timestamp_ms").get<int64_t>();
    if (j.contains("duration_ms")) r.duration_ms = j.at("duration_ms").get<double>();
    if (j.contains("success")) r.success = j.at("success").get<bool>();
    if (j.contains("output")) r.output = j.at("output");
    if (j.contains("error")) r.error = j.at("error").get<std::string>();
    return r;
}

} // namespace scheduler
} // namespace themis
