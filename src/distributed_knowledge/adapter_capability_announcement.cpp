/**
 * @file adapter_capability_announcement.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB — Licensed under MIT License

#include "distributed_knowledge/adapter_capability_announcement.h"
#include <mutex>
#include <optional>
#include <chrono>

namespace themis::distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// GossipAdapterPublisher
// ─────────────────────────────────────────────────────────────────────────────

GossipAdapterPublisher::GossipAdapterPublisher(
    std::string local_shard_id,
    std::function<void(nlohmann::json)> gossip_message_fn)
    : local_shard_id_(std::move(local_shard_id))
    , gossip_message_fn_(std::move(gossip_message_fn))
{}

GossipAdapterPublisher::~GossipAdapterPublisher() = default;

void GossipAdapterPublisher::announce(AdapterCapabilityAnnouncement announcement) {
    announcement.shard_id    = local_shard_id_;
    announcement.announced_at = std::chrono::system_clock::now();

    nlohmann::json payload = announcement.toJson();
    payload["message_type"] = "adapter_capability";

    {
        std::lock_guard<std::mutex> lk(mutex_);
        last_announcement_ = announcement;
    }

    if (gossip_message_fn_) {
        gossip_message_fn_(std::move(payload));
    }
}

void GossipAdapterPublisher::handleInboundMessage(const nlohmann::json& payload) {
    auto announcement = AdapterCapabilityAnnouncement::fromJson(payload);

    AnnouncementCallback cb;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        cb = on_announcement_;
    }

    if (cb) {
        cb(announcement);
    }
}

void GossipAdapterPublisher::setAnnouncementCallback(AnnouncementCallback cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    on_announcement_ = std::move(cb);
}

std::optional<AdapterCapabilityAnnouncement>
GossipAdapterPublisher::lastAnnouncement() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return last_announcement_;
}

// ─────────────────────────────────────────────────────────────────────────────
// DK-OR: GDPR erase
// ─────────────────────────────────────────────────────────────────────────────

themis::governance::StoreErasureResult GossipAdapterPublisher::erase(
    const std::string& /*subject_id*/,
    themis::governance::Regulation /*regulation*/)
{
    std::lock_guard<std::mutex> lk(mutex_);
    last_announcement_.reset();
    ++erase_count_;

    themis::governance::StoreErasureResult result;
    result.store_id       = "GossipAdapterPublisher";
    result.records_erased = 1;
    result.success        = true;
    return result;
}

size_t GossipAdapterPublisher::eraseCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return erase_count_;
}

} // namespace themis::distributed_knowledge
