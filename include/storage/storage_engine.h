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
     */
    ScanCounters scanCounters() const;

    /**
     * @brief Reset all scan performance counters to zero.
     */
    void resetScanCounters();

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
     * @param field_name Name of the field to encrypt
     * @param plaintext Plaintext data
     * @return Encrypted data
     */
    std::vector<uint8_t> encrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& plaintext);
    
    /**
     * @brief Decrypt a field after retrieving
     * 
     * Uses the injected field encryption provider.
     * 
     * @param field_name Name of the field to decrypt
     * @param ciphertext Encrypted data
     * @return Decrypted plaintext
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
};

} // namespace themis
