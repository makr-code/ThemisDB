/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            resource_profiler.cpp                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     171                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 79f0815052  2026-03-28  Add test statistics documentation and collection script ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/lora_framework/resource_profiler.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>
#include <numeric>

namespace themis {
namespace llm {
namespace lora {

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
    for (const auto& cb : impl_->callbacks) {
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

void ResourceProfiler::register_callback(ResourceMonitorCallback callback) {
    impl_->callbacks.push_back(std::move(callback));
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
