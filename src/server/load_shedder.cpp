/*
 * ThemisDB | File: load_shedder.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=1, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "server/load_shedder.h"
#include <algorithm>

namespace themis {
namespace server {

LoadShedder::LoadShedder(const Config& config)
    : config_(config)
{
}

bool LoadShedder::shouldReject(Priority prio) const {
    if (!config_.enable_shedding) {
        return false;
    }
    
    double load = getCurrentLoad();
    
    // Always allow HIGH priority requests
    if (prio == Priority::HIGH) {
        return false;
    }
    
    // Reject LOW priority at 80% load
    if (prio == Priority::LOW && load > 0.80) {
        return true;
    }
    
    // Reject NORMAL priority at 95% load
    if (prio == Priority::NORMAL && load > 0.95) {
        return true;
    }
    
    return false;
}

void LoadShedder::updateLoad(double cpu_usage, double memory_usage, size_t queue_depth) {
    cpu_usage_.store(std::clamp(cpu_usage, 0.0, 1.0));
    memory_usage_.store(std::clamp(memory_usage, 0.0, 1.0));
    queue_depth_.store(queue_depth);
}

double LoadShedder::getCurrentLoad() const {
    double cpu = cpu_usage_.load();
    double mem = memory_usage_.load();
    size_t queue = queue_depth_.load();
    
    // Normalize queue depth to 0.0-1.0
    double queue_factor = std::min(
        static_cast<double>(queue) / config_.queue_depth_threshold,
        1.0
    );
    
    // Weighted average: CPU 50%, Memory 30%, Queue 20%
    return (cpu * 0.5) + (mem * 0.3) + (queue_factor * 0.2);
}

} // namespace server
} // namespace themis
