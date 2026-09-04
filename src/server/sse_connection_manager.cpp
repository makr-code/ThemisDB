/**
 * @file sse_connection_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/sse_connection_manager.h"
#include "cdc/changefeed.h"
#include "utils/logger.h"
#include <algorithm>

namespace themis {
namespace server {

/**
 * @brief Construct SSE manager with explicit connection policy.
 * @param changefeed Changefeed source.
 * @param ioc io_context for timer scheduling.
 * @param config Runtime connection policy.
 */
SseConnectionManager::SseConnectionManager(
    std::shared_ptr<Changefeed> changefeed,
    boost::asio::io_context& ioc,
    const ConnectionConfig& config
)
    : changefeed_(std::move(changefeed))
    , ioc_(ioc)
    , config_(config)
    , poll_timer_(std::make_unique<boost::asio::steady_timer>(ioc_))
{
    THEMIS_INFO(
        "SSE Connection Manager initialized (heartbeat: {}ms, poll: {}ms, retry: {}ms, buffer: {}, drop_oldest: {}, max_eps: {})",
        config_.heartbeat_interval_ms,
        config_.event_poll_interval_ms,
        config_.retry_ms,
        config_.max_buffered_events,
        config_.drop_oldest_on_overflow,
        config_.max_events_per_second
    );
}

/**
 * @brief Construct SSE manager with default connection policy.
 * @param changefeed Changefeed source.
 * @param ioc io_context for timer scheduling.
 */
SseConnectionManager::SseConnectionManager(
    std::shared_ptr<Changefeed> changefeed,
    boost::asio::io_context& ioc
)
    : SseConnectionManager(std::move(changefeed), ioc, ConnectionConfig{}) {}

/** @brief Destructor; calls shutdown(). */
SseConnectionManager::~SseConnectionManager() {
    shutdown();
}

/**
 * @brief Register a client connection for SSE event streaming.
 * @param from_seq Initial sequence cursor for replay.
 * @param key_prefix Optional key prefix filter.
 * @param event_types Optional event-type filter set.
 * @return Unique connection id.
 */
uint64_t SseConnectionManager::registerConnection(
    uint64_t from_seq,
    const std::string& key_prefix,
    const std::set<Changefeed::ChangeEventType>& event_types
) {
    uint64_t conn_id = 0;
    bool start_background = false;

    {
        std::unique_lock<std::shared_mutex> lock(connections_mutex_);

        auto conn = std::make_shared<Connection>();
        conn->id = next_conn_id_++;
        conn->current_sequence = from_seq;
        conn->key_prefix = key_prefix;
        conn->event_types = event_types;
        conn->last_activity = std::chrono::steady_clock::now();
        conn->last_heartbeat = std::chrono::steady_clock::now();

        connections_[conn->id] = conn;
        conn_id = conn->id;

        THEMIS_INFO("SSE connection registered: id={}, from_seq={}, prefix='{}'",
            conn->id, from_seq, key_prefix);

        // Mark to start background polling if first connection
        if (static_cast<int>(connections_.size()) == 1 && !running_) {
            running_ = true;
            start_background = true;
        }
    }

    // Start background polling outside of the connections mutex to avoid
    // self-deadlock (backgroundPollTask acquires the same mutex).
    if (start_background) {
        backgroundPollTask();
    }

    return conn_id;
}

/**
 * @brief Unregister and deactivate a client connection.
 * @param conn_id Connection id.
 */
void SseConnectionManager::unregisterConnection([[maybe_unused]] uint64_t conn_id) {
    bool stop_polling = false;
    std::unique_lock<std::shared_mutex> lock(connections_mutex_);

    auto removed = connections_.extract(conn_id);
    if (!removed.empty()) {
        if (removed.mapped()) {
            removed.mapped()->active.store(false, std::memory_order_relaxed);
        }
        total_disconnects_++;

        THEMIS_INFO("SSE connection unregistered: id={}", conn_id);

        // Stop polling if no more connections
        if (connections_.empty()) {
            running_ = false;
            stop_polling = true;
        }
    }

    lock.unlock();
    if (stop_polling) {
        std::lock_guard<std::mutex> timer_lock(poll_timer_mutex_);
        if (poll_timer_) {
            poll_timer_->cancel();
        }
    }
}

/**
 * @brief Drain formatted SSE events for a connection.
 * @param conn_id Connection id.
 * @param max_events Maximum events to return.
 * @return Vector of SSE formatted event lines.
 */
std::vector<std::string> SseConnectionManager::pollEvents(
    uint64_t conn_id,
    size_t   max_events
) {
    std::unique_lock<std::shared_mutex> lock(connections_mutex_);

    auto it = connections_.find(conn_id);
    if (it == connections_.end() || !it->second
        || !it->second->active.load(std::memory_order_relaxed)) {
        return {};
    }

    auto& conn = it->second;

    // Apply server-side rate limit when configured.
    size_t count = max_events;
    if (config_.max_events_per_second > 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - conn->window_start).count();
        if (elapsed_ms >= 1000) {
            conn->window_start = now;
            conn->sent_in_window = 0;
        }
        uint32_t budget = 0;
        if (conn->sent_in_window < config_.max_events_per_second) {
            budget = config_.max_events_per_second - conn->sent_in_window;
        }
        if (budget == 0) {
            return {};
        }
        count = std::min({max_events, conn->buffered_events.size(),
                          static_cast<size_t>(budget)});
    } else {
        count = std::min(max_events, conn->buffered_events.size());
    }

    if (count == 0) {
        return {};
    }

    // Drain formatted SSE lines from the front of buffered_events.
    std::vector<std::string> events(
        conn->buffered_events.begin(),
        conn->buffered_events.begin() + static_cast<ptrdiff_t>(count)
    );
    conn->buffered_events.erase(
        conn->buffered_events.begin(),
        conn->buffered_events.begin() + static_cast<ptrdiff_t>(count)
    );

    // Keep raw_buffered_events in sync so that pollRawEvents() remains consistent
    // with the number of formatted lines already consumed.
    const size_t raw_count = std::min(count, conn->raw_buffered_events.size());
    if (raw_count > 0) {
        conn->raw_buffered_events.erase(
            conn->raw_buffered_events.begin(),
            conn->raw_buffered_events.begin() + static_cast<std::ptrdiff_t>(raw_count));
    }

    conn->last_activity = std::chrono::steady_clock::now();
    total_events_sent_ += events.size();
    if (config_.max_events_per_second > 0) {
        conn->sent_in_window += static_cast<uint32_t>(events.size());
    }

    return events;
}

/**
 * @brief Drain raw changefeed events for at-least-once delivery tracking.
 * @param conn_id Connection id.
 * @param max_events Maximum events to return.
 * @return Raw change events in ascending sequence order.
 */
std::vector<Changefeed::ChangeEvent> SseConnectionManager::pollRawEvents(
    uint64_t conn_id,
    size_t max_events
) {
    std::unique_lock<std::shared_mutex> lock(connections_mutex_);

    auto it = connections_.find(conn_id);
    if (it == connections_.end() || !it->second
        || !it->second->active.load(std::memory_order_relaxed)) {
        return {};
    }

    auto& conn = it->second;

    // Apply the same optional server-side rate limit as pollEvents().
    size_t count = max_events;
    if (config_.max_events_per_second > 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - conn->window_start).count();
        if (elapsed_ms >= 1000) {
            conn->window_start = now;
            conn->sent_in_window = 0;
        }
        uint32_t budget = 0;
        if (conn->sent_in_window < config_.max_events_per_second) {
            budget = config_.max_events_per_second - conn->sent_in_window;
        }
        if (budget == 0) {
            return {};
        }
        count = std::min({max_events, conn->raw_buffered_events.size(),
                          static_cast<size_t>(budget)});
    } else {
        count = std::min(max_events, conn->raw_buffered_events.size());
    }

    if (count == 0) {
        return {};
    }

    std::vector<Changefeed::ChangeEvent> raw_events(
        conn->raw_buffered_events.begin(),
        conn->raw_buffered_events.begin() + static_cast<std::ptrdiff_t>(count)
    );
    conn->raw_buffered_events.erase(
        conn->raw_buffered_events.begin(),
        conn->raw_buffered_events.begin() + static_cast<std::ptrdiff_t>(count)
    );

    if (!raw_events.empty()) {
        conn->last_activity = std::chrono::steady_clock::now();
        total_events_sent_ += raw_events.size();
        if (config_.max_events_per_second > 0) {
            conn->sent_in_window += static_cast<uint32_t>(raw_events.size());
        }
    }

    return raw_events;
}

/**
 * @brief Determine whether a heartbeat should be sent now.
 * @param conn_id Connection id.
 * @return true when heartbeat interval elapsed.
 */
bool SseConnectionManager::needsHeartbeat([[maybe_unused]] uint64_t conn_id) const {
    std::shared_lock<std::shared_mutex> lock(connections_mutex_);

    auto it = connections_.find(conn_id);
    if (it == connections_.end() || !it->second) {
        return false;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - it->second->last_heartbeat
    ).count();
    
    return elapsed >= config_.heartbeat_interval_ms;
}

/**
 * @brief Record heartbeat emission timestamp for a connection.
 * @param conn_id Connection id.
 */
void SseConnectionManager::recordHeartbeat([[maybe_unused]] uint64_t conn_id) {
    std::unique_lock<std::shared_mutex> lock(connections_mutex_);
    
    auto it = connections_.find(conn_id);
    if (it != connections_.end() && it->second) {
        it->second->last_heartbeat = std::chrono::steady_clock::now();
        total_heartbeats_sent_++;
    }
}

/**
 * @brief Get manager-level cumulative statistics.
 * @return Statistics snapshot.
 */
SseConnectionManager::ConnectionStats SseConnectionManager::getStats() const {
    std::shared_lock<std::shared_mutex> lock(connections_mutex_);
    
    ConnectionStats s{};
    s.active_connections = connections_.size();
    s.total_events_sent = total_events_sent_.load();
    s.total_heartbeats_sent = total_heartbeats_sent_.load();
    s.total_disconnects = total_disconnects_.load();
    s.total_dropped_events = total_dropped_events_.load();
    return s;
}

/**
 * @brief Stop polling and tear down all active connections.
 */
void SseConnectionManager::shutdown() {
    THEMIS_INFO("SSE Connection Manager shutting down...");
    
    running_ = false;

    std::unique_lock<std::shared_mutex> lock(connections_mutex_);
    for (auto& [id, conn] : connections_) {
        if (conn) {
            conn->active.store(false, std::memory_order_relaxed);
        }
    }
    connections_.clear();
    lock.unlock();

    std::lock_guard<std::mutex> timer_lock(poll_timer_mutex_);
    if (poll_timer_) {
        poll_timer_->cancel();
        // Reset so that a second shutdown() call (e.g. from the destructor after
        // an explicit HttpServer::stop()) cannot touch a timer whose io_context
        // may already have been destroyed.
        poll_timer_.reset();
    }
    
    THEMIS_INFO("SSE Connection Manager shutdown complete");
}

/**
 * @brief Background poll loop scheduled via timer to fill connection buffers.
 */
void SseConnectionManager::backgroundPollTask() {
    if (!running_) {
        return;
    }

    if (!changefeed_) {
        THEMIS_WARN("SSE background poll skipped: changefeed unavailable");
        running_ = false;
        return;
    }
    
    try {
        struct PollTarget {
            uint64_t id = 0;
            std::shared_ptr<Connection> conn;
            uint64_t from_sequence;
            std::string key_prefix;
            std::set<Changefeed::ChangeEventType> event_types;
        };

        // Snapshot the active connection list under a brief read lock so that
        // addConnection() / removeConnection() are not blocked for the full
        // changefeed poll duration (which may involve I/O and JSON serialisation).
        // The buffer-full early-exit check is performed here under the lock to
        // avoid an unsynchronised read of conn->buffered_events outside the lock.
        std::vector<PollTarget> active_conns;
        {
            std::shared_lock<std::shared_mutex> lock(connections_mutex_);
            active_conns.reserve(connections_.size());
            for (auto& [id, conn] : connections_) {
                if (!conn || !conn->active.load(std::memory_order_relaxed)) {
                  continue;
                }
                // Backpressure: skip non-drop-oldest connections whose buffer is
                // already full.  This read is safe here because we hold the
                // connections_mutex_ shared lock; the write side (pollEventsWithSequences,
                // backgroundPollTask write path) holds the exclusive lock.
                if (conn->buffered_events.size() >= config_.max_buffered_events
                    && !config_.drop_oldest_on_overflow) {
                    THEMIS_WARN("SSE connection {} buffer full, skipping poll", id);
                    continue;
                }
                active_conns.push_back(PollTarget{
                    id,
                    conn,
                    conn->current_sequence.load(std::memory_order_relaxed),
                    conn->key_prefix,
                    conn->event_types
                });
            }
        }

        for (auto& target : active_conns) {
            // Query new events since last sequence — without holding connections_mutex_.
            Changefeed::ListOptions options;
            options.from_sequence = target.from_sequence;
            options.limit = 100;
            
            if (!target.key_prefix.empty()) {
                options.key_prefix = target.key_prefix;
            }
            
            if (!target.event_types.empty()) {
                options.event_types = target.event_types;
            }
            
            auto events = changefeed_->listEvents(options);
            
            if (events.empty()) {
              continue;
            }

            // Re-acquire write lock briefly to append events to the connection buffer.
            std::unique_lock<std::shared_mutex> lock(connections_mutex_);
            // Re-check active flag: the connection may have been unregistered while we polled.
            if (!target.conn || !target.conn->active.load(std::memory_order_relaxed)) {
              continue;
            }
            auto& c = *target.conn;

            for (const auto& event : events) {
                // Enforce capacity limit: drop oldest when configured, otherwise skip
                // new events to preserve the hard max_buffered_events bound.
                if (config_.drop_oldest_on_overflow
                    && static_cast<int>(c.buffered_events.size()) >= config_.max_buffered_events
                    && config_.max_buffered_events > 0) {
                    const size_t overflow_count =
                        c.buffered_events.size() - static_cast<size_t>(config_.max_buffered_events) + 1;
                    c.buffered_events.erase(
                        c.buffered_events.begin(),
                        c.buffered_events.begin() + static_cast<std::ptrdiff_t>(overflow_count));

                    const size_t raw_overflow_count =
                        std::min(overflow_count,static_cast<int>(c.raw_buffered_events.size()));
                    if (raw_overflow_count > 0) {
                        c.raw_buffered_events.erase(
                            c.raw_buffered_events.begin(),
                            c.raw_buffered_events.begin() + static_cast<std::ptrdiff_t>(raw_overflow_count));
                    }

                    c.dropped_events += overflow_count;
                    total_dropped_events_ += overflow_count;
                }

                // Skip event if buffer is still at capacity (drop_oldest_on_overflow==false).
                if (static_cast<int>(c.buffered_events.size()) >= config_.max_buffered_events) {
                    c.dropped_events++;
                    total_dropped_events_++;
                    continue;
                }

                std::string sse_line = "id: " + std::to_string(event.sequence) + "\n";
                sse_line += "data: " + event.toJson().dump() + "\n\n";
                c.buffered_events.push_back(std::move(sse_line));
                // Also buffer the raw event for pollRawEvents() / at-least-once tracking.
                c.raw_buffered_events.push_back(event);
                c.current_sequence.store(
                    std::max(c.current_sequence.load(std::memory_order_relaxed), event.sequence),
                    std::memory_order_relaxed);
            }
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("SSE background poll error: {}", e.what());
    }
    
    // Schedule next poll.
    std::lock_guard<std::mutex> timer_lock(poll_timer_mutex_);
    if (running_ && poll_timer_) {
        poll_timer_->expires_after(
            std::chrono::milliseconds(config_.event_poll_interval_ms)
        );
        poll_timer_->async_wait([this](const boost::system::error_code& ec) {
            if (!ec && running_) {
                backgroundPollTask();
            }
        });
    }
}

} // namespace server
} // namespace themis
