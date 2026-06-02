/*
 * ThemisDB | File: cicada.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 101
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=2, L=0
 * PR History (last 5): #3579 docs(performance): Issue #3... (2026-03-12) | #160 Implement Phase 2 and Phase... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
