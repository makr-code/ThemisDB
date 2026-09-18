/**
 * @file cicada.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/cicada.h"
#include "performance/phase2_feature_flags.h"
#include <algorithm>
#include <stdexcept>

namespace themis {
namespace performance {

/**
 * @brief Hardware validation for Cicada OCC
 * @return True when the operation succeeds.
 * @details Calls: Phase2FeatureFlags::instance(), cicada_hardware_supported().
 */
static bool is_cicada_hardware_supported() {
    return Phase2FeatureFlags::instance().cicada_hardware_supported();
}

/**
 * @brief Record read.
 * @param[in,out] record Input/output parameter.
 * @param[in] version_read Input parameter.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: push_back().
 */
void CicadaTransaction::record_read(CicadaRecord* record, uint64_t version_read) {
    if (!record) {
        throw std::runtime_error("Cicada: Cannot record read on null record");
    }
    read_set_.push_back({record, version_read});
}

/**
 * @brief Record write.
 * @param[in,out] record Input/output parameter.
 * @param[in] data Input parameter.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: push_back(), std::move().
 */
void CicadaTransaction::record_write(CicadaRecord* record, std::string data) {
    if (!record) {
        throw std::runtime_error("Cicada: Cannot record write on null record");
    }
    write_set_.push_back({record, std::move(data)});
}

/**
 * @brief Execute.
 * @param[in] func Input parameter.
 * @return True when the operation succeeds.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: func().
 */
bool CicadaTransaction::execute(const TransactionFunc& func) {
    if (!func) {
        throw std::runtime_error("Cicada: Transaction function is null");
    }
    return func();
}

/**
 * @brief Validate reads.
 * @return True when the operation succeeds.
 * @details Calls: get_version(), is_locked().
 */
bool CicadaTransaction::validate_reads() {
    // Phase 1: Validate that all read versions are still current
    for (const auto& entry : read_set_) {
        if (!entry.record) {
            return false;  // Null record pointer
        }
        
        uint64_t current_version = entry.record->get_version();
        if (current_version != entry.version || entry.record->is_locked()) {
            return false; // Version changed or locked -> abort
        }
    }
    return true;
}

/**
 * @brief Acquire write locks.
 * @return True when the operation succeeds.
 * @details Calls: release_locks(), try_lock().
 */
bool CicadaTransaction::acquire_write_locks() {
    // Phase 2: Try to lock all write set records
    for (const auto& entry : write_set_) {
        if (!entry.record) {
            release_locks();
            return false;  // Null record pointer
        }
        
        if (!entry.record->try_lock()) {
            // Failed to acquire lock, release already acquired locks
            release_locks();
            return false;
        }
    }
    return true;
}

/**
 * @brief Install writes.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: size(), set_data(), unlock_and_increment_version().
 */
void CicadaTransaction::install_writes() {
    // Phase 3: Install new versions — write pending data then release the lock
    for (const auto& entry : write_set_) {
        if (!entry.record) {
            throw std::runtime_error("Cicada: Null record during write installation");
        }
        
        // Validate data size is reasonable (prevent OOM)
        if (entry.data.size() > (1 << 30)) {  // 1GB limit per value
            throw std::runtime_error("Cicada: Value size exceeds maximum (1GB)");
        }
        
        // Write the new data value into the record while the write lock is held
        entry.record->set_data(entry.data);
        entry.record->unlock_and_increment_version();
    }
}

/**
 * @brief Release locks.
 * @details Calls: is_locked(), unlock_and_increment_version().
 */
void CicadaTransaction::release_locks() {
    for (const auto& entry : write_set_) {
        if (entry.record && entry.record->is_locked()) {
            entry.record->unlock_and_increment_version();
        }
    }
}

/**
 * @brief Commit.
 * @return True when the operation succeeds.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: is_cicada_hardware_supported(), validate_reads(), abort(), acquire_write_locks(), install_writes().
 */
bool CicadaTransaction::commit() {
    // Validate hardware support for Cicada (RDTSC + CMPXCHG16B)
    if (!is_cicada_hardware_supported()) {
        throw std::runtime_error(
            "Cicada: Hardware does not support RDTSC + CMPXCHG16B required for OCC. "
            "Use pessimistic locking instead."
        );
    }
    
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

/**
 * @brief Abort.
 * @details Calls: release_locks(), clear().
 */
void CicadaTransaction::abort() {
    aborted_ = true;
    release_locks();
    read_set_.clear();
    write_set_.clear();
}

} // namespace performance
} // namespace themis
