/**
 * @file isolation_level.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.45
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

namespace themis {

/// Isolation levels for transactions.
///
/// Listed in increasing strictness order.
/// - READ_UNCOMMITTED: Lowest isolation; no extra read locks acquired. At the
///                     RocksDB storage layer this maps to READ_COMMITTED (RocksDB
///                     never exposes uncommitted writes). Higher layers treat this
///                     level as a hint to skip optimistic conflict checks. Callers
///                     should expect: no dirty reads (guaranteed by storage), but
///                     non-repeatable reads and phantom reads are possible.
/// - READ_COMMITTED:   Only committed data is visible. Default for most workloads.
///                     Non-repeatable reads and phantom reads are possible.
/// - REPEATABLE_READ:  Snapshot isolation – the transaction sees a consistent
///                     snapshot of data as of its start time. Non-repeatable reads
///                     are prevented; phantom reads are prevented for the rows read
///                     at start.
/// - SERIALIZABLE:     Full serializability via Snapshot Isolation + write-conflict
///                     detection (SSI). Also prevents phantom reads and write skew.
///                     Slowest but safest. May abort more transactions due to
///                     conflicts.
///
/// Note: value 2 is intentionally reserved (gap between READ_COMMITTED=1 and
/// REPEATABLE_READ=3) to preserve backward compatibility with the legacy Snapshot=3
/// alias.  SerializableSnapshot is an alias for SERIALIZABLE (both equal 4).
enum class IsolationLevel {
    // Legacy aliases preserved for backward compatibility
    ReadCommitted       = 1, ///< Same as READ_COMMITTED
    Snapshot            = 3, ///< Same as REPEATABLE_READ (snapshot isolation)
    SerializableSnapshot = 4, ///< Same as SERIALIZABLE (SSI – predicate locking)

    // Standard SQL names
    READ_UNCOMMITTED = 0, ///< Lowest isolation; no extra read locks
    READ_COMMITTED   = 1, ///< Only committed values visible (default)
    REPEATABLE_READ  = 3, ///< Snapshot isolation – no non-repeatable reads
    SERIALIZABLE     = 4  ///< SSI – also prevents phantom reads and write skew
};

} // namespace themis
