/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            crdt_importer.cpp                                  ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:16:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     125                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9efa3acd76  2026-03-11  feat(importers): add PostgreSQL Importer v2.1+ with 12 ne... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/crdt_importer.h"
#include <stdexcept>
#include <chrono>
#include <map>

namespace themis {
namespace importers {
namespace crdt {

// ---------------------------------------------------------------------------
// CRDTRecord
// ---------------------------------------------------------------------------

CRDTTableState::CRDTRecord
CRDTTableState::CRDTRecord::merge(const CRDTRecord& left, const CRDTRecord& right) {
    // LWW ordering: wall_clock_ns DESC → lamport_clock DESC → replica_id ASC (tiebreak)
    if (left.wall_clock_ns > right.wall_clock_ns)  return left;
    if (left.wall_clock_ns < right.wall_clock_ns)  return right;
    if (left.lamport_clock > right.lamport_clock)  return left;
    if (left.lamport_clock < right.lamport_clock)  return right;
    // Lexicographic tiebreak: higher replica_id wins (deterministic)
    return (left.replica_id >= right.replica_id) ? left : right;
}

json CRDTTableState::CRDTRecord::toJson() const {
    return json{
        {"id",           id},
        {"value",        value},
        {"lamport_clock", lamport_clock},
        {"replica_id",   replica_id},
        {"wall_clock_ns", wall_clock_ns}
    };
}

CRDTTableState::CRDTRecord
CRDTTableState::CRDTRecord::fromJson(const json& j) {
    CRDTRecord r;
    r.id            = j.at("id").get<std::string>();
    r.value         = j.value("value", json{});
    r.lamport_clock = j.value("lamport_clock", uint64_t{0});
    r.replica_id    = j.value("replica_id", std::string{});
    r.wall_clock_ns = j.value("wall_clock_ns", uint64_t{0});
    return r;
}

// ---------------------------------------------------------------------------
// CRDTTableState
// ---------------------------------------------------------------------------

uint64_t CRDTTableState::tickClock() {
    return ++lamport_clock_;
}

size_t CRDTTableState::importWithCRDT(
    const std::string& table_name,
    const std::vector<json>& records,
    const std::string& replica_id)
{
    size_t written = 0;
    uint64_t now_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    for (const auto& rec : records) {
        if (!rec.contains("id")) {
            // Record must have an "id" field; skip silently
            continue;
        }

        CRDTRecord incoming;
        incoming.id            = rec.at("id").get<std::string>();
        incoming.value         = rec;
        incoming.lamport_clock = tickClock();
        incoming.replica_id    = replica_id;
        incoming.wall_clock_ns = now_ns;

        auto& table_state = state_[table_name];
        auto it = table_state.find(incoming.id);
        if (it == table_state.end()) {
            table_state.emplace(incoming.id, std::move(incoming));
        } else {
            it->second = CRDTRecord::merge(it->second, incoming);
        }
        ++written;
    }

    return written;
}

const CRDTTableState::CRDTRecord*
CRDTTableState::lookup(const std::string& table_name,
                        const std::string& record_id) const
{
    auto tit = state_.find(table_name);
    if (tit == state_.end()) return nullptr;
    auto rit = tit->second.find(record_id);
    if (rit == tit->second.end()) return nullptr;
    return &rit->second;
}

} // namespace crdt
} // namespace importers
} // namespace themis
