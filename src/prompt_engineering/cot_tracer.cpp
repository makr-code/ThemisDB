/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cot_tracer.cpp                                     ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 20:34:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     227                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 93aebd9731  2026-03-23  feat(prompt_engineering): CoT Step Tracer — IChainOfThoug... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "prompt_engineering/cot_tracer.h"

#include <algorithm>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// CoTSpanRecord::toJson
// ============================================================================

nlohmann::json CoTSpanRecord::toJson() const {
    const auto ts =
        std::chrono::system_clock::to_time_t(start_time);
    return {
        {"step_index",   step_index},
        {"label",        label},
        {"content",      content},
        {"token_count",  token_count},
        {"duration_us",  duration.count()},
        {"start_time",   static_cast<std::int64_t>(ts)}
    };
}

// ============================================================================
// RecordingCoTTracer
// ============================================================================

void RecordingCoTTracer::onStepBegin(
    StepId             step_index,
    const std::string& label) noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    if (pending_.size() <= step_index) {
        pending_.resize(step_index + 1);
    }
    pending_[step_index].label      = label;
    pending_[step_index].start_time = std::chrono::system_clock::now();
}

void RecordingCoTTracer::onStepEnd(
    StepId                    step_index,
    const std::string&        content,
    std::chrono::microseconds duration) noexcept {
    std::lock_guard<std::mutex> lk(mutex_);

    CoTSpanRecord rec;
    rec.step_index  = step_index;
    rec.content     = content;
    rec.duration    = duration;
    rec.token_count = content.size() / 4;  // BPE approximation (chars / 4)

    if (pending_.size() > step_index) {
        rec.label      = pending_[step_index].label;
        rec.start_time = pending_[step_index].start_time;
    } else {
        rec.start_time = std::chrono::system_clock::now() - duration;
    }

    spans_.push_back(std::move(rec));
}

std::vector<CoTSpanRecord> RecordingCoTTracer::spans() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return spans_;
}

std::size_t RecordingCoTTracer::spanCount() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return spans_.size();
}

bool RecordingCoTTracer::hasSpans() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return !spans_.empty();
}

void RecordingCoTTracer::reset() {
    std::lock_guard<std::mutex> lk(mutex_);
    spans_.clear();
    pending_.clear();
}

nlohmann::json RecordingCoTTracer::toJson() const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto arr = nlohmann::json::array();
    for (const auto& rec : spans_) {
        arr.push_back(rec.toJson());
    }
    return arr;
}

// ============================================================================
// CoTTraceCollector
// ============================================================================

void CoTTraceCollector::onStepBegin(
    StepId             step_index,
    const std::string& label) noexcept {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (pending_.size() <= step_index) {
            pending_.resize(step_index + 1);
        }
        pending_[step_index].label      = label;
        pending_[step_index].start_time = std::chrono::system_clock::now();
    }
    // Forward to children — must not hold mutex while calling into child to
    // avoid potential deadlock; snapshot the list first.
    std::vector<std::shared_ptr<IChainOfThoughtTracer>> children;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        children = children_;
    }
    for (auto& child : children) {
        child->onStepBegin(step_index, label);
    }
}

void CoTTraceCollector::onStepEnd(
    StepId                    step_index,
    const std::string&        content,
    std::chrono::microseconds duration) noexcept {
    {
        std::lock_guard<std::mutex> lk(mutex_);

        CoTSpanRecord rec;
        rec.step_index  = step_index;
        rec.content     = content;
        rec.duration    = duration;
        rec.token_count = content.size() / 4;

        if (pending_.size() > step_index) {
            rec.label      = pending_[step_index].label;
            rec.start_time = pending_[step_index].start_time;
        } else {
            rec.start_time = std::chrono::system_clock::now() - duration;
        }

        spans_.push_back(std::move(rec));
    }
    total_steps_traced_.fetch_add(1, std::memory_order_relaxed);

    std::vector<std::shared_ptr<IChainOfThoughtTracer>> children;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        children = children_;
    }
    for (auto& child : children) {
        child->onStepEnd(step_index, content, duration);
    }
}

void CoTTraceCollector::addTracer(
    std::shared_ptr<IChainOfThoughtTracer> tracer) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (tracer) {
        children_.push_back(std::move(tracer));
    }
}

void CoTTraceCollector::removeTracer(const IChainOfThoughtTracer* tracer) {
    std::lock_guard<std::mutex> lk(mutex_);
    children_.erase(
        std::remove_if(children_.begin(), children_.end(),
                       [tracer](const auto& p) { return p.get() == tracer; }),
        children_.end());
}

std::size_t CoTTraceCollector::tracerCount() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return children_.size();
}

std::vector<CoTSpanRecord> CoTTraceCollector::spans() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return spans_;
}

std::size_t CoTTraceCollector::spanCount() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return spans_.size();
}

void CoTTraceCollector::reset() {
    std::lock_guard<std::mutex> lk(mutex_);
    spans_.clear();
    pending_.clear();
    total_steps_traced_.store(0, std::memory_order_relaxed);
}

nlohmann::json CoTTraceCollector::toJson() const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto arr = nlohmann::json::array();
    for (const auto& rec : spans_) {
        arr.push_back(rec.toJson());
    }
    return arr;
}

std::size_t CoTTraceCollector::totalStepsTraced() const noexcept {
    return total_steps_traced_.load(std::memory_order_relaxed);
}

} // namespace prompt_engineering
} // namespace themis
