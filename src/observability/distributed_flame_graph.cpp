/*
 * ThemisDB | File: distributed_flame_graph.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 302
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=9, H=22, M=13, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "observability/distributed_flame_graph.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/** Parse pprof folded-stacks text into a {stack → count} map. */
static std::map<std::string, uint64_t> parseFolded(const std::string& text) {
    std::map<std::string, uint64_t> result;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        auto space = line.rfind(' ');
        if (space == std::string::npos) continue;
        std::string stack = line.substr(0, space);
        uint64_t count = 0;
        try {
            count = std::stoull(line.substr(space + 1));
        } catch (...) {
            continue;
        }
        result[stack] += count;
    }
    return result;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// MergedFlameGraph
// ---------------------------------------------------------------------------

std::string MergedFlameGraph::toFoldedText() const {
    std::string out;
    for (const auto& [stack, count] : stacks) {
        out += stack;
        out += ' ';
        out += std::to_string(count);
        out += '\n';
    }
    return out;
}

json MergedFlameGraph::toJSON() const {
    auto ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                     generated_at.time_since_epoch())
                     .count();

    // Build per-stack array for structured consumers
    json stacks_arr = json::array();
    for (const auto& [stack, count] : stacks) {
        stacks_arr.push_back({{"stack", stack}, {"count", count}});
    }

    return json{
        {"generated_at_ms", ts_ms},
        {"node_ids", node_ids},
        {"node_count", node_ids.size()},
        {"stack_count", stacks.size()},
        {"folded_text", toFoldedText()},
        {"stacks", stacks_arr}
    };
}

// ---------------------------------------------------------------------------
// DistributedFlameGraph::Impl
// ---------------------------------------------------------------------------

class DistributedFlameGraph::Impl {
public:
    explicit Impl(const DistributedFlameGraphConfig& config) : config_(config) {}

    void addNodeProfile(const NodeProfile& profile) {
        std::lock_guard<std::mutex> lk(mutex_);
        profiles_[profile.node_id] = profile;
        // Enforce max_nodes: evict the oldest entry (by insertion order)
        // when the limit is exceeded.
        while (config_.max_nodes > 0 && profiles_.size() > config_.max_nodes) {
            profiles_.erase(profiles_.begin());
        }
    }

    void clearProfiles() {
        std::lock_guard<std::mutex> lk(mutex_);
        profiles_.clear();
    }

    MergedFlameGraph merge() const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::vector<std::string> ids;
        ids.reserve(profiles_.size());
        for (const auto& [id, _] : profiles_) {
            ids.push_back(id);
        }
        return mergeProfiles(ids);
    }

    MergedFlameGraph mergeFiltered(const std::vector<std::string>& node_ids) const {
        std::lock_guard<std::mutex> lk(mutex_);
        return mergeProfiles(node_ids);
    }

    ProfileDiff diff(const MergedFlameGraph& baseline,
                     const MergedFlameGraph& current) const {
        ProfileDiff result;

        const std::string base_text    = baseline.toFoldedText();
        const std::string current_text = current.toFoldedText();

        auto baseMap = parseFolded(base_text);
        auto curMap  = parseFolded(current_text);

        uint64_t baseTotal = 0, curTotal = 0;
        for (const auto& [k, v] : baseMap) baseTotal += v;
        for (const auto& [k, v] : curMap)  curTotal  += v;

        if (baseTotal > 0 && curTotal > 0) {
            result.cpu_regression_percent =
                (static_cast<double>(curTotal) /
                 static_cast<double>(baseTotal) - 1.0) * 100.0;
        }

        constexpr double kChangeThreshold = 0.10; // 10 % normalised share change

        for (const auto& [stack, curCnt] : curMap) {
            auto baseIt = baseMap.find(stack);
            if (baseIt == baseMap.end()) {
                result.new_hotspots.push_back(stack);
            } else {
                double baseNorm = (baseTotal > 0)
                    ? static_cast<double>(baseIt->second) /
                      static_cast<double>(baseTotal)
                    : 0.0;
                double curNorm = (curTotal > 0)
                    ? static_cast<double>(curCnt) /
                      static_cast<double>(curTotal)
                    : 0.0;
                if (std::abs(curNorm - baseNorm) > kChangeThreshold) {
                    result.changed_hotspots.push_back(stack);
                }
            }
        }

        for (const auto& [stack, _] : baseMap) {
            if (curMap.find(stack) == curMap.end()) {
                result.removed_hotspots.push_back(stack);
            }
        }

        // Cap list sizes for usability
        auto trim = [](std::vector<std::string>& v) {
            if (v.size() > 20) v.resize(20);
        };
        trim(result.new_hotspots);
        trim(result.removed_hotspots);
        trim(result.changed_hotspots);

        return result;
    }

    std::vector<std::string> getNodeIds() const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::vector<std::string> ids;
        ids.reserve(profiles_.size());
        for (const auto& [id, _] : profiles_) {
            ids.push_back(id);
        }
        return ids;
    }

    size_t nodeCount() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return profiles_.size();
    }

    DistributedFlameGraphConfig getConfig() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return config_;
    }

private:
    /**
     * Merge a specific list of node IDs.
     * Caller MUST hold mutex_.
     */
    MergedFlameGraph mergeProfiles(const std::vector<std::string>& ids) const {
        MergedFlameGraph result;
        result.generated_at = std::chrono::system_clock::now();

        for (const auto& id : ids) {
            auto it = profiles_.find(id);
            if (it == profiles_.end()) continue;
            const NodeProfile& np = it->second;
            if (np.snapshot.type != ProfileType::CPU) continue;

            result.node_ids.push_back(id);
            const std::string text = np.snapshot.dataAsString();
            auto stacks = parseFolded(text);

            if (config_.normalize_per_node) {
                // Compute total samples for this node
                uint64_t node_total = 0;
                for (const auto& [s, c] : stacks) node_total += c;
                if (node_total == 0) continue;

                // Scale each stack's count to its proportional share of this
                // node's total samples, expressed as parts-per-million (0 –
                // 1 000 000).  Every node therefore contributes an equal total
                // weight of 1 000 000 units regardless of its absolute sample
                // volume, so high-throughput nodes do not visually dominate
                // low-throughput ones in the merged flame graph.
                for (const auto& [stack, count] : stacks) {
                    uint64_t scaled = static_cast<uint64_t>(
                        static_cast<double>(count) /
                        static_cast<double>(node_total) * 1'000'000.0);
                    result.stacks[stack] += scaled;
                }
            } else {
                // Raw sum: add counts directly
                for (const auto& [stack, count] : stacks) {
                    result.stacks[stack] += count;
                }
            }
        }

        return result;
    }

    mutable std::mutex mutex_;
    DistributedFlameGraphConfig config_;
    // Ordered map so eviction of oldest entry on overflow is deterministic.
    std::map<std::string, NodeProfile> profiles_;
};

// ---------------------------------------------------------------------------
// DistributedFlameGraph – public API
// ---------------------------------------------------------------------------

DistributedFlameGraph::DistributedFlameGraph(const DistributedFlameGraphConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

DistributedFlameGraph::~DistributedFlameGraph() = default;

void DistributedFlameGraph::addNodeProfile(const NodeProfile& profile) {
    impl_->addNodeProfile(profile);
}

void DistributedFlameGraph::clearProfiles() {
    impl_->clearProfiles();
}

MergedFlameGraph DistributedFlameGraph::merge() const {
    return impl_->merge();
}

MergedFlameGraph DistributedFlameGraph::mergeFiltered(
        const std::vector<std::string>& node_ids) const {
    return impl_->mergeFiltered(node_ids);
}

ProfileDiff DistributedFlameGraph::diff(const MergedFlameGraph& baseline,
                                         const MergedFlameGraph& current) const {
    return impl_->diff(baseline, current);
}

std::vector<std::string> DistributedFlameGraph::getNodeIds() const {
    return impl_->getNodeIds();
}

size_t DistributedFlameGraph::nodeCount() const {
    return impl_->nodeCount();
}

DistributedFlameGraphConfig DistributedFlameGraph::getConfig() const {
    return impl_->getConfig();
}

} // namespace observability
} // namespace themis
