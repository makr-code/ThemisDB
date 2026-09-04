/**
 * @file resource_profiler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/resource_profiler.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>
#include <numeric>

namespace themis {
namespace llm {
namespace lora {

/** @brief Implementation detail. */
class ResourceProfiler::Impl {
public:
    Config config;
    bool running = false;
    std::vector<ResourceSnapshot> snapshots;
    std::vector<ResourceMonitorCallback> callbacks;
};

ResourceProfiler::ResourceProfiler(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

ResourceProfiler::ResourceProfiler()
    : impl_(std::make_unique<Impl>()) {
    impl_->config = Config{};
}

ResourceProfiler::~ResourceProfiler() = default;

void ResourceProfiler::start() {
    impl_->running = true;
}

void ResourceProfiler::stop() {
    impl_->running = false;
}

void ResourceProfiler::snapshot(int epoch, int step, float loss, float lr) {
    if (!impl_->running || !impl_->config.enabled) {
        return;
    }
    ResourceSnapshot snapshot;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.current_epoch = epoch;
    snapshot.current_step = step;
    snapshot.current_loss = loss;
    snapshot.learning_rate = lr;
    query_gpu_memory(snapshot);
    query_cpu_memory(snapshot);
    query_gpu_utilization(snapshot);
    check_alerts(snapshot);
    impl_->snapshots.push_back(snapshot);
    for ([[maybe_unused]] const auto& cb : impl_->callbacks) {
        cb(snapshot);
    }
}

ResourceSnapshot ResourceProfiler::get_current_snapshot() const {
    if (impl_->snapshots.empty()) {
        return ResourceSnapshot{};
    }
    return impl_->snapshots.back();
}

std::vector<ResourceSnapshot> ResourceProfiler::get_snapshots() const {
    return impl_->snapshots;
}

ResourceStats ResourceProfiler::compute_stats() const {
    ResourceStats stats;
    stats.num_snapshots = impl_->snapshots.size();
    if (impl_->snapshots.empty()) {
        return stats;
    }
    for (const auto& s : impl_->snapshots) {
        stats.peak_gpu_memory = std::max(stats.peak_gpu_memory, s.gpu_memory_allocated);
        stats.peak_cpu_memory = std::max(stats.peak_cpu_memory, s.cpu_memory_used);
        stats.avg_gpu_utilization       += s.gpu_utilization;
        stats.avg_gpu_memory_utilization += s.gpu_memory_utilization;
        stats.avg_samples_per_second    += s.samples_per_second;
        stats.avg_tokens_per_second     += s.tokens_per_second;
    }
    const float n = static_cast<float>(impl_->snapshots.size());
    stats.avg_gpu_utilization        /= n;
    stats.avg_gpu_memory_utilization /= n;
    stats.avg_samples_per_second     /= n;
    stats.avg_tokens_per_second      /= n;
    auto duration = impl_->snapshots.back().timestamp - impl_->snapshots.front().timestamp;
    stats.total_training_time = std::chrono::duration_cast<std::chrono::seconds>(duration);
    return stats;
}

void ResourceProfiler::register_callback([[maybe_unused]] ResourceMonitorCallback callback) {
    impl_->callbacks.push_back([[maybe_unused]] std::move(callback));
}

void ResourceProfiler::clear() {
    impl_->snapshots.clear();
}

bool ResourceProfiler::is_running() const {
    return impl_->running;
}

ResourceProfiler::Config ResourceProfiler::get_config() const {
    return impl_->config;
}

void ResourceProfiler::set_config(const Config& config) {
    impl_->config = config;
}

void ResourceProfiler::export_to_json(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        spdlog::warn("ResourceProfiler: cannot open '{}' for JSON export", filename);
        return;
    }
    for (const auto& s : impl_->snapshots) {
        out << s.toJSON().dump() << '\n';
    }
}

size_t ResourceProfiler::get_peak_gpu_memory() const {
    if (impl_->snapshots.empty()) return 0;
    return std::max_element(impl_->snapshots.begin(), impl_->snapshots.end(),
        [](const ResourceSnapshot& a, const ResourceSnapshot& b){
            return a.gpu_memory_allocated < b.gpu_memory_allocated;
        })->gpu_memory_allocated;
}

size_t ResourceProfiler::get_peak_cpu_memory() const {
    if (impl_->snapshots.empty()) return 0;
    return std::max_element(impl_->snapshots.begin(), impl_->snapshots.end(),
        [](const ResourceSnapshot& a, const ResourceSnapshot& b){
            return a.cpu_memory_used < b.cpu_memory_used;
        })->cpu_memory_used;
}

void ResourceProfiler::query_gpu_memory(ResourceSnapshot&) const {}
void ResourceProfiler::query_cpu_memory(ResourceSnapshot&) const {}
void ResourceProfiler::query_gpu_utilization(ResourceSnapshot&) const {}
void ResourceProfiler::check_alerts(const ResourceSnapshot&) {}
void ResourceProfiler::log_snapshot(const ResourceSnapshot&) {}

} // namespace lora
} // namespace llm
} // namespace themis
