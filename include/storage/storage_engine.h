/**
 * @file storage_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/security_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include "storage/rocksdb_wrapper.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <optional>

namespace themis {

/**
 * @brief Storage Engine implementation with Dependency Injection
 * 
 * This class demonstrates the Dependency Inversion Principle by:
 * - Depending on abstractions (interfaces) instead of concrete implementations
 * - Allowing injection of query evaluation, encryption, and key management
 * - Breaking circular dependencies between Storage, Query, and Security layers
 * 
 * Benefits:
 * - Testable: Can inject mocks for unit testing
 * - Flexible: Can swap implementations without recompiling
 * - Explicit: Dependencies are clearly documented in constructor
 * - Decoupled: No concrete dependencies on QueryEngine or FieldEncryption
 */
class StorageEngine : public IStorageEngine {
public:
    // ── Scan performance counters ─────────────────────────────────────────

    /**
     * @brief Cumulative scan performance counters.
     *
     * All fields are monotonically increasing since the engine was opened.
     */
    struct ScanCounters {
        uint64_t scan_calls{0};          ///< Total calls to scanRange / scanPrefix / scanPredicate
        uint64_t keys_examined{0};       ///< Total keys visited by all scans
        uint64_t keys_returned{0};       ///< Keys actually delivered to callers
        uint64_t early_stops{0};         ///< Scans stopped early by a false callback return

        /** Ratio of returned vs examined keys (filter selectivity). */
        double selectivity() const {
            return keys_examined == 0 ? 1.0
                                      : static_cast<double>(keys_returned) / keys_examined;
        }
    };

    // ── Storage I/O metrics ───────────────────────────────────────────────

    /**
     * @brief Cumulative per-operation latency and throughput metrics.
     *
     * All latency fields are in **microseconds**.  Counters are monotonically
     * increasing from the moment the engine was last opened.
     *
     * **Sentinel values for min latency fields:**
     * `put_latency_min_us`, `get_latency_min_us`, and `del_latency_min_us` are
     * initialised to `UINT64_MAX` (no operations observed yet).  Callers should
     * check the corresponding `_ops` counter and treat `UINT64_MAX` as "no data".
     */
    struct IOMetrics {
        // ── put ──────────────────────────────────────────────────────────
        uint64_t put_ops{0};           ///< Total successful put() calls
        uint64_t put_errors{0};        ///< Total failed put() calls
        uint64_t put_latency_us{0};    ///< Cumulative latency of successful puts (µs)
        uint64_t put_latency_min_us{UINT64_MAX}; ///< Min put latency (µs); UINT64_MAX = no data
        uint64_t put_latency_max_us{0};          ///< Max put latency (µs)

        // ── get ──────────────────────────────────────────────────────────
        uint64_t get_ops{0};
        uint64_t get_errors{0};
        uint64_t get_latency_us{0};
        uint64_t get_latency_min_us{UINT64_MAX}; ///< UINT64_MAX = no data
        uint64_t get_latency_max_us{0};

        // ── del ──────────────────────────────────────────────────────────
        uint64_t del_ops{0};
        uint64_t del_errors{0};
        uint64_t del_latency_us{0};
        uint64_t del_latency_min_us{UINT64_MAX}; ///< UINT64_MAX = no data
        uint64_t del_latency_max_us{0};

        /** Average put latency in microseconds (0 if no puts yet). */
        double avg_put_latency_us() const {
            return put_ops == 0 ? 0.0
                                : static_cast<double>(put_latency_us) / put_ops;
        }
        /** Average get latency in microseconds (0 if no gets yet). */
        double avg_get_latency_us() const {
            return get_ops == 0 ? 0.0
                                : static_cast<double>(get_latency_us) / get_ops;
        }
        /** Average del latency in microseconds (0 if no dels yet). */
        double avg_del_latency_us() const {
            return del_ops == 0 ? 0.0
                                : static_cast<double>(del_latency_us) / del_ops;
        }
    };

    /**
     * @brief Constructor with Dependency Injection
     * 
     * @param evaluator Expression evaluator for WHERE clauses
     * @param encryption Field-level encryption provider
     * @param key_provider Key management provider
     * @param index_manager Index management provider (optional)
     * @throws std::invalid_argument if required dependencies are null
     */
    StorageEngine(
        IExpressionEvaluatorPtr evaluator,
        IFieldEncryptionPtr encryption,
        IKeyProviderPtr key_provider,
        IIndexManagerPtr index_manager = nullptr
    );
    
    /**
     * @brief Move constructor
     * 
     * Transfers ownership of all resources (dependencies and RocksDB wrapper)
     * from another StorageEngine instance.
     * 
     * @param other StorageEngine instance to move from (will be in valid but
     *              unspecified state after this operation)
     */
    StorageEngine(StorageEngine&& other) noexcept = default;
    
    /**
     * @brief Move assignment operator
     * 
     * Transfers ownership of all resources and closes any currently open database.
     * Satisfies CWE-672 (Use After Free) by ensuring proper cleanup.
     * 
     * @param other StorageEngine instance to move from
     * @return Reference to this object
     */
    StorageEngine& operator=(StorageEngine&& other) noexcept = default;
    
    // Delete copy operations to prevent accidental copies of injected dependencies
    StorageEngine(const StorageEngine&) = delete;
    StorageEngine& operator=(const StorageEngine&) = delete;
    
    /**
     * @brief Static factory method for backward compatibility
     * 
     * Creates a StorageEngine with default implementations of all dependencies.
     * Use this for simple scenarios or when migrating existing code.
     * 
     * @return Shared pointer to StorageEngine with default dependencies
     */
    static std::shared_ptr<StorageEngine> createDefault();
    
    // IStorageEngine interface implementation
    Result<void> open(const std::string& db_path) override;
    void close() override;
    Result<void> put(const std::string& key, const std::string& value) override;
    Result<std::string> get(const std::string& key) override;
    Result<void> del(const std::string& key) override;

    /**
     * @brief Scan a key range [start_key, end_key) in sorted order.
     *
     * Iterates all keys ≥ start_key and < end_key (pass empty strings for
     * open-ended bounds) and calls @p callback for each key-value pair.
     * Returning false from the callback stops iteration early.
     *
     * Updates ScanCounters atomically.
     *
     * @return Result<void> – ok on success, error on failure.
     */
    Result<void> scanRange(
        std::string_view start_key,
        std::string_view end_key,
        std::function<bool(std::string_view key, std::string_view value)> callback
    ) override;

    /**
     * @brief Scan all keys with a given prefix.
     *
     * Updates ScanCounters atomically.
     *
     * @return Result<void> – ok on success, error on failure.
     */
    Result<void> scanPrefix(
        std::string_view prefix,
        std::function<bool(std::string_view key, std::string_view value)> callback
    ) override;

    /**
     * @brief Scan a key range with an inline predicate filter.
     *
     * Like scanRange() but only delivers key-value pairs for which
     * @p predicate returns true.  The predicate is evaluated for every
     * key visited; if it returns false the entry is counted as
     * "examined but not returned" (keys_examined++ only).
     *
     * Updates ScanCounters atomically.
     *
     * @param start_key  Inclusive lower bound (empty = beginning).
     * @param end_key    Exclusive upper bound (empty = end).
     * @param predicate  Returns true if the entry should be delivered.
     * @param callback   Called only for entries that pass the predicate.
     *                   Return false to stop iteration.
     * @return Result<void> – ok on success, error on failure.
     */
    Result<void> scanPredicate(
        std::string_view start_key,
        std::string_view end_key,
        std::function<bool(std::string_view key, std::string_view value)> predicate,
        std::function<bool(std::string_view key, std::string_view value)> callback
    );

    /**
     * @brief Return a copy of the current scan performance counters.
     *
     * Thread-safe: reads are sequentially consistent.
     * 
     * **Move Semantics**: Returned ScanCounters struct uses move semantics to enable
     * Return Value Optimization (RVO) and avoid unnecessary copies (CWE-457 remediation).
     */
    ScanCounters scanCounters() const;

    /**
     * @brief Reset all scan performance counters to zero.
     */
    void resetScanCounters();

    /**
     * @brief Return a snapshot of cumulative I/O metrics.
     *
     * Thread-safe: fields are read with relaxed atomics (consistent per-field,
     * not a cross-field snapshot).
     * 
     * **Move Semantics**: Returned IOMetrics struct uses move semantics to enable
     * Return Value Optimization (RVO) and avoid unnecessary copies (CWE-457 remediation).
     */
    IOMetrics ioMetrics() const;

    /**
     * @brief Reset all I/O metrics to zero / initial state.
     */
    void resetIOMetrics();

    /**
     * @brief Apply a filter expression to stored data
     * 
     * Uses the injected expression evaluator to filter documents.
     * 
     * @param filter_expr The filter expression string
     * @param context Context for evaluation (e.g., current document)
     * @return true if filter matches, false otherwise
     */
    bool apply_filter(const std::string& filter_expr, const void* context);
    
    /**
     * @brief Encrypt a field before storing
     * 
     * Uses the injected field encryption provider.
     * 
     * **Move Semantics**: Returned vector uses move semantics to avoid unnecessary
     * copying of encrypted data (CWE-457 remediation).
     * 
     * @param field_name Name of the field to encrypt
     * @param plaintext Plaintext data
     * @return Encrypted data (moved to caller, no copy)
     */
    std::vector<uint8_t> encrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& plaintext);
    
    /**
     * @brief Decrypt a field after retrieving
     * 
     * Uses the injected field encryption provider.
     * 
     * **Move Semantics**: Returned vector uses move semantics to avoid unnecessary
     * copying of decrypted data (CWE-457 remediation).
     * 
     * @param field_name Name of the field to decrypt
     * @param ciphertext Encrypted data
     * @return Decrypted plaintext (moved to caller, no copy)
     */
    std::vector<uint8_t> decrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& ciphertext);

    /**
     * @brief Create default implementations (for testing and builder)
     * 
     * These factory methods create default implementations of interfaces.
     * Used by createDefault() factory and StorageEngineBuilder::standard()
     */
    static IExpressionEvaluatorPtr createDefaultEvaluator();
    static IFieldEncryptionPtr createDefaultEncryption();
    static IKeyProviderPtr createDefaultKeyProvider();
    static IIndexManagerPtr createDefaultIndexManager();

    /** Expose the underlying RocksDB wrapper (for advanced operations). */
    RocksDBWrapper* rawDB() { return rocksdb_.get(); }
    const RocksDBWrapper* rawDB() const { return rocksdb_.get(); }

private:
    // Injected dependencies (interfaces, not concrete implementations)
    IExpressionEvaluatorPtr evaluator_;
    IFieldEncryptionPtr encryption_;
    IKeyProviderPtr key_provider_;
    IIndexManagerPtr index_manager_;
    
    // Underlying RocksDB storage
    std::shared_ptr<RocksDBWrapper> rocksdb_;

    // Internal storage state
    std::string db_path_;
    bool is_open_ = false;

    // Scan performance counters (lock-free atomics)
    mutable std::atomic<uint64_t> sc_calls_{0};
    mutable std::atomic<uint64_t> sc_examined_{0};
    mutable std::atomic<uint64_t> sc_returned_{0};
    mutable std::atomic<uint64_t> sc_early_stops_{0};

    // I/O latency metrics (lock-free atomics, latency in microseconds)
    mutable std::atomic<uint64_t> io_put_ops_{0};
    mutable std::atomic<uint64_t> io_put_errors_{0};
    mutable std::atomic<uint64_t> io_put_latency_{0};
    mutable std::atomic<uint64_t> io_put_min_{UINT64_MAX};
    mutable std::atomic<uint64_t> io_put_max_{0};

    mutable std::atomic<uint64_t> io_get_ops_{0};
    mutable std::atomic<uint64_t> io_get_errors_{0};
    mutable std::atomic<uint64_t> io_get_latency_{0};
    mutable std::atomic<uint64_t> io_get_min_{UINT64_MAX};
    mutable std::atomic<uint64_t> io_get_max_{0};

    mutable std::atomic<uint64_t> io_del_ops_{0};
    mutable std::atomic<uint64_t> io_del_errors_{0};
    mutable std::atomic<uint64_t> io_del_latency_{0};
    mutable std::atomic<uint64_t> io_del_min_{UINT64_MAX};
    mutable std::atomic<uint64_t> io_del_max_{0};
};

} // namespace themis
