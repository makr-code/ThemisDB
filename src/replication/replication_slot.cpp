/**
 * @file replication_slot.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Replication Slot Management Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "replication/replication_slot.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace themisdb {
namespace replication {

// ============================================================================
// ReplicationSlot
// ============================================================================

ReplicationSlot::ReplicationSlot(
    const std::string&           name,
    const std::string&           plugin_name,
    const std::string&           downstream_node_id,
    std::shared_ptr<WALManager>  wal_manager,
    const std::string&           state_file_path)
    : wal_manager_(std::move(wal_manager))
    , state_file_path_(state_file_path)
{
    state_.name               = name;
    state_.plugin_name        = plugin_name;
    state_.downstream_node_id = downstream_node_id;
    state_.status             = SlotStatus::ACTIVE;
    state_.confirmed_lsn      = 0;
    state_.restart_lsn        = 0;
    state_.created_at         = std::chrono::system_clock::now();
    state_.last_activity      = state_.created_at;

    // Load persisted state if the file already exists (restart scenario)
    if (std::filesystem::exists(state_file_path_)) {
        loadState();
    } else {
        persistState();
    }
}

// ---------------------------------------------------------------------------
// Control API
// ---------------------------------------------------------------------------

bool ReplicationSlot::pause()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (state_.status != SlotStatus::ACTIVE) return false;
    state_.status        = SlotStatus::PAUSED;
    state_.last_activity = std::chrono::system_clock::now();
    persistState();
    return true;
}

bool ReplicationSlot::resume()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (state_.status != SlotStatus::PAUSED) return false;
    state_.status        = SlotStatus::ACTIVE;
    state_.last_activity = std::chrono::system_clock::now();
    persistState();
    return true;
}

bool ReplicationSlot::drop()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (state_.status == SlotStatus::DROPPED) return false;
    state_.status        = SlotStatus::DROPPED;
    state_.last_activity = std::chrono::system_clock::now();
    persistState();
    return true;
}

// ---------------------------------------------------------------------------
// Progress tracking
// ---------------------------------------------------------------------------

bool ReplicationSlot::advance(uint64_t confirmed_lsn)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (state_.status != SlotStatus::ACTIVE) return false;
    if (confirmed_lsn < state_.confirmed_lsn) return false; // do not go backwards
    state_.confirmed_lsn = confirmed_lsn;
    state_.restart_lsn   = confirmed_lsn; // restart from acked position
    state_.last_activity = std::chrono::system_clock::now();
    persistState();
    return true;
}

// ---------------------------------------------------------------------------
// Introspection
// ---------------------------------------------------------------------------

const std::string& ReplicationSlot::name() const
{
    // state_.name is written once at construction; no lock needed
    return state_.name;
}

ReplicationSlot::SlotStatus ReplicationSlot::status() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_.status;
}

ReplicationSlot::SlotState ReplicationSlot::state() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_;
}

uint64_t ReplicationSlot::lag() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    const uint64_t leader_seq = wal_manager_->getCurrentSequence();
    if (leader_seq <= state_.confirmed_lsn) return 0;
    return leader_seq - state_.confirmed_lsn;
}

// ---------------------------------------------------------------------------
// Persistence helpers
// ---------------------------------------------------------------------------

void ReplicationSlot::persistState() const
{
    // Write a minimal JSON state file
    // Called while holding state_mutex_
    std::filesystem::create_directories(
        std::filesystem::path(state_file_path_).parent_path());

    std::ofstream ofs(state_file_path_, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) return;

    auto tp_to_ms = [](const std::chrono::system_clock::time_point& tp) -> int64_t {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()).count();
    };

    int status_int = 0;
    switch (state_.status) {
        case SlotStatus::ACTIVE:  status_int = 0; break;
        case SlotStatus::PAUSED:  status_int = 1; break;
        case SlotStatus::DROPPED: status_int = 2; break;
    }

    ofs << "{\n"
        << "  \"name\": \"" << state_.name << "\",\n"
        << "  \"status\": " << status_int << ",\n"
        << "  \"confirmed_lsn\": " << state_.confirmed_lsn << ",\n"
        << "  \"restart_lsn\": " << state_.restart_lsn << ",\n"
        << "  \"plugin_name\": \"" << state_.plugin_name << "\",\n"
        << "  \"downstream_node_id\": \"" << state_.downstream_node_id << "\",\n"
        << "  \"created_at_ms\": " << tp_to_ms(state_.created_at) << ",\n"
        << "  \"last_activity_ms\": " << tp_to_ms(state_.last_activity) << "\n"
        << "}\n";
    ofs.flush();
}

void ReplicationSlot::loadState()
{
    // Load state from JSON file (called at construction, without lock)
    std::ifstream ifs(state_file_path_);
    if (!ifs.is_open()) return;

    std::string line;
    while (std::getline(ifs, line)) {
        auto extract = [&](const std::string& key) -> std::string {
            const std::string search = "\"" + key + "\": ";
            auto pos = line.find(search);
            if (pos == std::string::npos) return "";
            pos += search.size();
            // Remove quotes if present
            auto end = line.find(',', pos);
            if (end == std::string::npos) end = line.find('}', pos);
            std::string val = line.substr(pos, (end != std::string::npos) ? end - pos : std::string::npos);
            // Trim quotes
            if (!val.empty() && val.front() == '"') val = val.substr(1);
            if (!val.empty() && val.back()  == '"') val.pop_back();
            return val;
        };

        if (line.find("\"status\"") != std::string::npos) {
            const std::string v = extract("status");
            if (!v.empty()) {
                int si = std::stoi(v);
                switch (si) {
                    case 0: state_.status = SlotStatus::ACTIVE;  break;
                    case 1: state_.status = SlotStatus::PAUSED;  break;
                    case 2: state_.status = SlotStatus::DROPPED; break;
                }
            }
        } else if (line.find("\"confirmed_lsn\"") != std::string::npos) {
            const std::string v = extract("confirmed_lsn");
            if (!v.empty()) state_.confirmed_lsn = std::stoull(v);
        } else if (line.find("\"restart_lsn\"") != std::string::npos) {
            const std::string v = extract("restart_lsn");
            if (!v.empty()) state_.restart_lsn = std::stoull(v);
        }
    }
}

// ============================================================================
// ReplicationSlotManager
// ============================================================================

ReplicationSlotManager::ReplicationSlotManager(
    const ManagerConfig&         config,
    std::shared_ptr<WALManager>  wal_manager)
    : config_(config)
    , wal_manager_(std::move(wal_manager))
{}

std::shared_ptr<ReplicationSlot>
ReplicationSlotManager::createSlot(
    const std::string& name,
    const std::string& plugin_name,
    const std::string& downstream_node_id)
{
    std::lock_guard<std::mutex> lock(slots_mutex_);
    if (slots_.count(name)) return nullptr; // already exists

    const std::string state_path =
        config_.wal_directory + "/slots/" + name + ".json";

    auto slot = std::make_shared<ReplicationSlot>(
        name, plugin_name, downstream_node_id, wal_manager_, state_path);
    slots_[name] = slot;
    return slot;
}

std::shared_ptr<ReplicationSlot>
ReplicationSlotManager::getSlot(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(slots_mutex_);
    const auto it = slots_.find(name);
    return (it != slots_.end()) ? it->second : nullptr;
}

bool ReplicationSlotManager::dropSlot(const std::string& name)
{
    std::lock_guard<std::mutex> lock(slots_mutex_);
    const auto it = slots_.find(name);
    if (it == slots_.end()) return false;
    it->second->drop();
    slots_.erase(it);
    return true;
}

std::vector<ReplicationSlot::SlotState>
ReplicationSlotManager::listSlots() const
{
    std::lock_guard<std::mutex> lock(slots_mutex_);
    std::vector<ReplicationSlot::SlotState> states;
    states.reserve(slots_.size());
    for (const auto& kv : slots_) {
        states.push_back(kv.second->state());
    }
    return states;
}

size_t ReplicationSlotManager::slotCount() const
{
    std::lock_guard<std::mutex> lock(slots_mutex_);
    return slots_.size();
}

uint64_t ReplicationSlotManager::minConfirmedLsn() const
{
    std::lock_guard<std::mutex> lock(slots_mutex_);
    uint64_t min_lsn = std::numeric_limits<uint64_t>::max();
    bool found_active = false;
    for (const auto& kv : slots_) {
        const auto st = kv.second->state();
        if (st.status == ReplicationSlot::SlotStatus::ACTIVE ||
            st.status == ReplicationSlot::SlotStatus::PAUSED) {
            min_lsn = std::min(min_lsn, st.confirmed_lsn);
            found_active = true;
        }
    }
    return found_active ? min_lsn : 0;
}

void ReplicationSlotManager::loadPersistedSlots()
{
    const std::string slots_dir = config_.wal_directory + "/slots";
    if (!std::filesystem::exists(slots_dir)) return;

    for (const auto& entry : std::filesystem::directory_iterator(slots_dir)) {
        if (entry.path().extension() != ".json") continue;
        const std::string slot_name = entry.path().stem().string();

        std::lock_guard<std::mutex> lock(slots_mutex_);
        if (slots_.count(slot_name)) continue; // already loaded

        auto slot = std::make_shared<ReplicationSlot>(
            slot_name, "physical", "",
            wal_manager_, entry.path().string());
        // Only keep non-dropped slots
        if (slot->status() != ReplicationSlot::SlotStatus::DROPPED) {
            slots_[slot_name] = slot;
        }
    }
}

} // namespace replication
} // namespace themisdb
