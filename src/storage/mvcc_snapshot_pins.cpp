/**
 * @file mvcc_snapshot_pins.cpp
 * @brief Implementation of MVCCSnapshotPinRegistry (durable MVCC snapshot leases).
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/mvcc_snapshot_pins.h"

#include "storage/rocksdb_wrapper.h"

#include <algorithm>
#include <array>
#include <random>
#include <stdexcept>

namespace themis {

namespace {

std::string encodePinValue(const MVCCSnapshotPinRegistry::Pin& pin) {
    std::string v = pin.ts.encodeToString();
    v.append(pin.owner);
    return v;
}

bool isHexLeaseId(std::string_view id) {
    if (id.empty() || id.size() > 64) return false;
    return std::all_of(id.begin(), id.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

} // namespace

MVCCSnapshotPinRegistry::MVCCSnapshotPinRegistry(std::shared_ptr<RocksDBWrapper> db)
    : db_(std::move(db)) {
    if (!db_) return;
    std::string error;
    db_->scanPrefix(kKeyPrefix, [&](std::string_view key, std::string_view value) -> bool {
        std::string_view id = key.substr(kKeyPrefix.size());
        if (!isHexLeaseId(id) || value.size() < 8) {
            error = "MVCCSnapshotPinRegistry: malformed persisted pin record";
            return false;
        }
        Pin pin;
        pin.lease_id = std::string(id);
        pin.ts = HLCTimestamp::decodeFromBytes(reinterpret_cast<const uint8_t*>(value.data()));
        pin.owner = std::string(value.substr(8));
        pins_.emplace(pin.lease_id, std::move(pin));
        return true;
    });
    if (!error.empty()) {
        throw std::runtime_error(error);
    }
}

std::string MVCCSnapshotPinRegistry::generateLeaseId() {
    static constexpr char kHex[] = "0123456789abcdef";
    std::random_device rd;
    std::string id;
    id.reserve(32);
    for (int i = 0; i < 4; ++i) {
        uint32_t r = rd();
        for (int n = 0; n < 8; ++n) {
            id.push_back(kHex[(r >> (n * 4)) & 0xF]);
        }
    }
    return id;
}

bool MVCCSnapshotPinRegistry::persist(const Pin& pin) {
    if (!db_) return true;
    const std::string key = std::string(kKeyPrefix) + pin.lease_id;
    return db_->put(key, std::string_view(encodePinValue(pin)));
}

std::optional<MVCCSnapshotPinRegistry::Pin>
MVCCSnapshotPinRegistry::acquire(HLCTimestamp ts, std::string_view owner) {
    // Exclusive GC interlock: no GC pass is between victim selection and
    // deletion while the pin is published.
    std::unique_lock<std::shared_mutex> gc_lock(gc_mu_);
    Pin pin;
    pin.ts = ts;
    pin.owner = std::string(owner);
    {
        std::lock_guard<std::mutex> lk(mu_);
        do {
            pin.lease_id = generateLeaseId();
        } while (pins_.count(pin.lease_id) != 0);
    }
    if (!persist(pin)) {
        return std::nullopt;
    }
    std::lock_guard<std::mutex> lk(mu_);
    pins_.emplace(pin.lease_id, pin);
    return pin;
}

bool MVCCSnapshotPinRegistry::release(std::string_view lease_id) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = pins_.find(std::string(lease_id));
    if (it == pins_.end()) return false;
    if (db_ && !db_->del(std::string(kKeyPrefix) + it->first)) {
        return false;
    }
    pins_.erase(it);
    return true;
}

std::optional<MVCCSnapshotPinRegistry::Pin>
MVCCSnapshotPinRegistry::find(std::string_view lease_id) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = pins_.find(std::string(lease_id));
    if (it == pins_.end()) return std::nullopt;
    return it->second;
}

std::vector<MVCCSnapshotPinRegistry::Pin> MVCCSnapshotPinRegistry::list() const {
    std::lock_guard<std::mutex> lk(mu_);
    std::vector<Pin> out;
    out.reserve(pins_.size());
    for (const auto& [id, pin] : pins_) out.push_back(pin);
    return out;
}

bool MVCCSnapshotPinRegistry::isPinned(HLCTimestamp ts) const {
    std::lock_guard<std::mutex> lk(mu_);
    return std::any_of(pins_.begin(), pins_.end(),
                       [&](const auto& kv) { return kv.second.ts == ts; });
}

std::vector<HLCTimestamp> MVCCSnapshotPinRegistry::pinnedTimestamps() const {
    std::vector<HLCTimestamp> out;
    {
        std::lock_guard<std::mutex> lk(mu_);
        out.reserve(pins_.size());
        for (const auto& [id, pin] : pins_) out.push_back(pin.ts);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

std::shared_lock<std::shared_mutex> MVCCSnapshotPinRegistry::gcGuard() const {
    return std::shared_lock<std::shared_mutex>(gc_mu_);
}

} // namespace themis
