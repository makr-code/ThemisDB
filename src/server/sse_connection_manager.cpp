/*
 * ThemisDB | File: sse_connection_manager.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=15, H=93, M=12, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "server/sse_connection_manager.h"
#include "cdc/changefeed.h"
#include "utils/logger.h"
#include <algorithm>

namespace themis {
namespace server {

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

// Delegating constructor with default config
SseConnectionManager::SseConnectionManager(
    std::shared_ptr<Changefeed> changefeed,
    boost::asio::io_context& ioc
)
    : SseConnectionManager(std::move(changefeed), ioc, ConnectionConfig{}) {}

SseConnectionManager::~SseConnectionManager() {
    shutdown();
}

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
        if (connections_.size() == 1 && !running_) {
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

void SseConnectionManager::unregisterConnection(uint64_t conn_id) {
    std::unique_lock<std::shared_mutex> lock(connections_mutex_);
    
    auto it = connections_.find(conn_id);
    if (it != connections_.end()) {
        it->second->active = false;
        connections_.erase(it);
        total_disconnects_++;
        
        THEMIS_INFO("SSE connection unregistered: id={}", conn_id);
        
        // Stop polling if no more connections
        if (connections_.empty()) {
            running_ = false;
            if (poll_timer_) {
                poll_timer_->cancel();
            }
        }
    }
}

std::vector<std::string> SseConnectionManager::pollEvents(
    uint64_t conn_id,
    size_t max_events
) {
    std::unique_lock<std::shared_mutex> lock(connections_mutex_);
    
    auto it = connections_.find(conn_id);
    if (it == connections_.end() || !it->second->active) {
        return {};
    }
    
    auto& conn = it->second;
    
    // Optional server-side rate limit per connection (events/second)
    if (config_.max_events_per_second > 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - conn->window_start).count();
        if (elapsed_ms >= 1000) {
            // Reset 1s window
            conn->window_start = now;
            conn->sent_in_window = 0;
        }
        // Compute remaining budget for this window
        uint32_t budget = 0;
        if (conn->sent_in_window < config_.max_events_per_second) {
            budget = config_.max_events_per_second - conn->sent_in_window;
        }
        if (budget == 0) {
            // No budget left -> defer sending
            return {};
        }
        // Apply both client poll cap and server budget
        size_t allowed = static_cast<size_t>(budget);
        size_t count = std::min({max_events, conn->buffered_events.size(), allowed});
        if (count == 0) {
            return {};
        }
        std::vector<std::string> events(
            conn->buffered_events.begin(),
            conn->buffered_events.begin() + count
        );
        // Remove consumed events from buffer
        conn->buffered_events.erase(
            conn->buffered_events.begin(),
            conn->buffered_events.begin() + count
        );
        if (!events.empty()) {
            conn->last_activity = std::chrono::steady_clock::now();
            total_events_sent_ += events.size();
            conn->sent_in_window += static_cast<uint32_t>(events.size());
        }
        
        return events;
    }
    
    // No server-side rate limit: take events from buffer (up to max_events)
    size_t count = std::min(max_events, conn->buffered_events.size());
    std::vector<std::string> events(
        conn->buffered_events.begin(),
        conn->buffered_events.begin() + count
    );
    
    // Remove consumed events from buffer
    conn->buffered_events.erase(
        conn->buffered_events.begin(),
        conn->buffered_events.begin() + count
    );
    
    if (!events.empty()) {
        conn->last_activity = std::chrono::steady_clock::now();
        total_events_sent_ += events.size();
    }
    
    return events;
}

bool SseConnectionManager::needsHeartbeat(uint64_t conn_id) const {
    std::shared_lock<std::shared_mutex> lock(connections_mutex_);
    
    auto it = connections_.find(conn_id);
    if (it == connections_.end()) {
        return false;
    }
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - it->second->last_heartbeat
    ).count();
    
    return elapsed >= config_.heartbeat_interval_ms;
}

void SseConnectionManager::recordHeartbeat(uint64_t conn_id) {
    std::unique_lock<std::shared_mutex> lock(connections_mutex_);
    
    auto it = connections_.find(conn_id);
    if (it != connections_.end()) {
        it->second->last_heartbeat = std::chrono::steady_clock::now();
        total_heartbeats_sent_++;
    }
}

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

void SseConnectionManager::shutdown() {
    THEMIS_INFO("SSE Connection Manager shutting down...");
    
    running_ = false;
    
    if (poll_timer_) {
        poll_timer_->cancel();
    }
    
    std::unique_lock<std::shared_mutex> lock(connections_mutex_);
    for (auto& [id, conn] : connections_) {
        conn->active = false;
    }
    connections_.clear();
    
    THEMIS_INFO("SSE Connection Manager shutdown complete");
}

void SseConnectionManager::backgroundPollTask() {
    if (!running_) {
        return;
    }
    
    try {
        // Snapshot the active connection list under a brief read lock so that
        // addConnection() / removeConnection() are not blocked for the full
        // changefeed poll duration (which may involve I/O and JSON serialisation).
        std::vector<std::pair<uint64_t, std::shared_ptr<Connection>>> active_conns;
        {
            std::shared_lock<std::shared_mutex> lock(connections_mutex_);
            active_conns.reserve(connections_.size());
            for (auto& [id, conn] : connections_) {
                if (conn->active) {
                    active_conns.emplace_back(id, conn);
                }
            }
        }

        for (auto& [id, conn] : active_conns) {
            // If buffer is full, apply backpressure policy
            if (conn->buffered_events.size() >= config_.max_buffered_events && !config_.drop_oldest_on_overflow) {
                THEMIS_WARN("SSE connection {} buffer full, skipping poll", id);
                continue;
            }
            
            // Query new events since last sequence — without holding connections_mutex_.
            Changefeed::ListOptions options;
            options.from_sequence = conn->current_sequence;
            options.limit = 100;
            
            if (!conn->key_prefix.empty()) {
                options.key_prefix = conn->key_prefix;
            }
            
            if (!conn->event_types.empty()) {
                options.event_types = conn->event_types;
            }
            
            auto events = changefeed_->listEvents(options);
            
            if (events.empty()) continue;

            // Re-acquire write lock briefly to append events to the connection buffer.
            std::unique_lock<std::shared_mutex> lock(connections_mutex_);
            // Re-check active flag: the connection may have been removed while we polled.
            auto it = connections_.find(id);
            if (it == connections_.end() || !it->second->active) continue;
            auto& c = *it->second;

            for (const auto& event : events) {
                // Ensure capacity: drop oldest if configured
                while (c.buffered_events.size() >= config_.max_buffered_events) {
                    if (config_.drop_oldest_on_overflow) {
                        if (!c.buffered_events.empty()) {
                            c.buffered_events.erase(c.buffered_events.begin());
                            c.dropped_events++;
                            total_dropped_events_++;
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                }

                std::string sse_line = "id: " + std::to_string(event.sequence) + "\n";
                sse_line += "data: " + event.toJson().dump() + "\n\n";
                c.buffered_events.push_back(std::move(sse_line));
                c.current_sequence = std::max(c.current_sequence, event.sequence);
            }
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("SSE background poll error: {}", e.what());
    }
    
    // Schedule next poll
    if (running_) {
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
