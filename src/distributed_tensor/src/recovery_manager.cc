/**
 * @file recovery_manager.cc
 * @brief Recovery manager implementation stub.
 *
 * Skeleton: in-memory job queue with synchronous no-op execution.
 * Replace with Reed-Solomon reconstruction in sub-issue #5433.
 */

#include "distributed_tensor/include/recovery_manager.h"

namespace themis::distributed_tensor {

namespace {

class RecoveryManagerImpl final : public IRecoveryManager {
public:
    explicit RecoveryManagerImpl(std::shared_ptr<IManifestStore> manifest)
        : manifest_(std::move(manifest)) {}

    std::string enqueue(const std::string& artifact_id,
                         RecoveryStrategy strategy) override {
        std::string job_id = "job-" + std::to_string(jobs_.size() + 1);
        RecoveryJob job{
            .job_id      = job_id,
            .artifact_id = artifact_id,
            .strategy    = strategy,
            .status      = RecoveryStatus::Queued,
            .created_at  = std::chrono::system_clock::now(),
        };
        jobs_[job_id] = std::move(job);
        queue_.push_back(job_id);
        return job_id;
    }

    std::optional<RecoveryJob> statusOf(
        const std::string& job_id) const override {
        auto it = jobs_.find(job_id);
        if (it == jobs_.end()) return std::nullopt;
        return it->second;
    }

    RecoveryJob executeNext() override {
        if (queue_.empty()) return RecoveryJob{.status = RecoveryStatus::Failed,
                                               .failure_reason = "queue empty"};
        std::string job_id = queue_.front();
        queue_.erase(queue_.begin());
        auto& job = jobs_[job_id];
        job.status = RecoveryStatus::InProgress;
        if (callback_) callback_(job);

        // TODO(#5433): Implement actual Reed-Solomon or replica fetch.
        job.status       = RecoveryStatus::Failed;
        job.failure_reason = "recovery not implemented yet";
        job.completed_at = std::chrono::system_clock::now();
        if (callback_) callback_(job);
        return job;
    }

    std::vector<std::string> listJobs(RecoveryStatus status) const override {
        std::vector<std::string> ids;
        for (const auto& [id, job] : jobs_)
            if (job.status == status) ids.push_back(id);
        return ids;
    }

    void onStatusChange(StatusCallback cb) override {
        callback_ = std::move(cb);
    }

private:
    std::shared_ptr<IManifestStore>           manifest_;
    std::unordered_map<std::string, RecoveryJob> jobs_;
    std::vector<std::string>                  queue_;
    StatusCallback                            callback_;
};

} // namespace

std::unique_ptr<IRecoveryManager> makeRecoveryManager(
    std::shared_ptr<IManifestStore> manifest) {
    return std::make_unique<RecoveryManagerImpl>(std::move(manifest));
}

} // namespace themis::distributed_tensor
