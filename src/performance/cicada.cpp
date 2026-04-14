/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cicada.cpp                                         ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:35:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     115                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e8990043f6  2026-03-09  fix(performance): implement Cicada install_writes() data ... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "performance/cicada.h"
#include <algorithm>

namespace themis {
namespace performance {

void CicadaTransaction::record_read(CicadaRecord* record, uint64_t version_read) {
    read_set_.push_back({record, version_read});
}

void CicadaTransaction::record_write(CicadaRecord* record, std::string data) {
    write_set_.push_back({record, std::move(data)});
}

bool CicadaTransaction::execute(const TransactionFunc& func) {
    return func();
}

bool CicadaTransaction::validate_reads() {
    // Phase 1: Validate that all read versions are still current
    for (const auto& entry : read_set_) {
        uint64_t current_version = entry.record->get_version();
        if (current_version != entry.version || entry.record->is_locked()) {
            return false; // Version changed or locked -> abort
        }
    }
    return true;
}

bool CicadaTransaction::acquire_write_locks() {
    // Phase 2: Try to lock all write set records
    for (const auto& entry : write_set_) {
        if (!entry.record->try_lock()) {
            // Failed to acquire lock, release already acquired locks
            release_locks();
            return false;
        }
    }
    return true;
}

void CicadaTransaction::install_writes() {
    // Phase 3: Install new versions — write pending data then release the lock
    for (const auto& entry : write_set_) {
        // Write the new data value into the record while the write lock is held
        entry.record->set_data(entry.data);
        entry.record->unlock_and_increment_version();
    }
}

void CicadaTransaction::release_locks() {
    for (const auto& entry : write_set_) {
        if (entry.record->is_locked()) {
            entry.record->unlock_and_increment_version();
        }
    }
}

bool CicadaTransaction::commit() {
    if (aborted_) {
        return false;
    }
    
    // Three-phase commit protocol (from Cicada paper)
    
    // Phase 1: Validate reads
    if (!validate_reads()) {
        abort();
        return false;
    }
    
    // Phase 2: Acquire write locks
    if (!acquire_write_locks()) {
        abort();
        return false;
    }
    
    // Phase 3: Install writes and release locks
    install_writes();
    
    return true;
}

void CicadaTransaction::abort() {
    aborted_ = true;
    release_locks();
    read_set_.clear();
    write_set_.clear();
}

} // namespace performance
} // namespace themis
