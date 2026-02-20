#pragma once

#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/security_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include "storage/rocksdb_wrapper.h"
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
     * @return Result<void> – ok on success, error on failure.
     */
    Result<void> scanPrefix(
        std::string_view prefix,
        std::function<bool(std::string_view key, std::string_view value)> callback
    ) override;

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
};

} // namespace themis
