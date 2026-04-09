/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_flush_controller.cpp                      ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "timeseries/adaptive_flush_controller.h"
#include "utils/logger.h"

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

AdaptiveFlushController::AdaptiveFlushController(TSStore* store,
                                                 AdaptiveFlushControllerConfig config)
    : config_(std::move(config))
    , high_watermark_(static_cast<size_t>(config_.capacity * config_.watermark_ratio))
{
    if (!store) {
        throw std::invalid_argument("AdaptiveFlushController: store cannot be null");
    }

    TSAutoBufferConfig buf_cfg;
    buf_cfg.max_total_points            = config_.capacity;
    buf_cfg.flush_interval              = config_.flush_interval;
    buf_cfg.async_flush                 = config_.async_flush;
    buf_cfg.enable_adaptive_flush       = config_.enable_adaptive_flush;
    buf_cfg.backpressure_slo_ms         = config_.backpressure_slo_ms;
    buf_cfg.ewma_alpha                  = config_.ewma_alpha;
    buf_cfg.adaptive_batch_min          = config_.adaptive_batch_min;
    buf_cfg.adaptive_batch_max          = config_.adaptive_batch_max;
    buf_cfg.flush_batch_size            = config_.flush_batch_size;
    buf_cfg.overdue_flush_multiplier    = config_.overdue_flush_multiplier;
    buf_cfg.backpressure_high_watermark = high_watermark_;
    buf_cfg.backpressure_low_watermark  =
        static_cast<size_t>(high_watermark_ * 0.25); // 25% of high-watermark
    buf_cfg.metrics                     = config_.metrics;

    buffer_ = std::make_unique<TSAutoBuffer>(store, std::move(buf_cfg));

    THEMIS_INFO("AdaptiveFlushController created: capacity={}, watermark={}, "
                "flush_interval={}ms, adaptive={}",
                config_.capacity, high_watermark_,
                config_.flush_interval.count(),
                config_.enable_adaptive_flush);
}

AdaptiveFlushController::~AdaptiveFlushController() {
    if (buffer_ && buffer_->isRunning()) {
        buffer_->stop();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void AdaptiveFlushController::start() {
    buffer_->start();
}

void AdaptiveFlushController::stop() {
    buffer_->stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Write API
// ─────────────────────────────────────────────────────────────────────────────

Result<void> AdaptiveFlushController::add(const TSStore::DataPoint& point) {
    return buffer_->add(point);
}

Result<void> AdaptiveFlushController::addBatch(
    const std::vector<TSStore::DataPoint>& points)
{
    for (const auto& point : points) {
        auto result = buffer_->add(point);
        if (!result.has_value()) {
            return result;
        }
    }
    return OkVoid();
}

size_t AdaptiveFlushController::flush() {
    return buffer_->flush();
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats / Query
// ─────────────────────────────────────────────────────────────────────────────

AdaptiveFlushStats AdaptiveFlushController::getStats() const {
    TSAutoBufferStats raw = buffer_->getStats();

    AdaptiveFlushStats stats;
    stats.points_buffered            = raw.points_buffered.load();
    stats.points_flushed             = raw.points_flushed.load();
    stats.flush_count                = raw.flush_count.load();
    stats.backpressure_events        = raw.backpressure_events.load();
    stats.current_buffer_size        = raw.current_buffer_size;
    stats.current_ewma_latency_ms    = raw.current_ewma_latency_ms;
    stats.current_adaptive_batch_size = raw.current_adaptive_batch_size;
    return stats;
}

bool AdaptiveFlushController::isBackpressured() const {
    TSAutoBufferStats raw = buffer_->getStats();
    return raw.current_buffer_size >= high_watermark_;
}

bool AdaptiveFlushController::isRunning() const {
    return buffer_->isRunning();
}

} // namespace themis
