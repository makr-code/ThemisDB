/**
 * @file index_initialization.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/index_manager.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/storage_interface.h"
#include <memory>
#include <stdexcept>

namespace themis {

/**
 * @brief Builder pattern for constructing IndexManager instances
 * 
 * Provides a fluent interface for configuring and building IndexManager
 * instances with custom or default dependencies.
 * 
 * Example usage:
 * @code
 * auto index_mgr = IndexManagerBuilder::standard()
 *     .withEvaluator(custom_evaluator)
 *     .withStorage(custom_storage)
 *     .build();
 * @endcode
 */
class IndexManagerBuilder {
public:
    /**
     * @brief Construct an empty builder.
     *
     * The builder starts without mandatory dependencies. Callers may supply
     * a custom evaluator, storage backend, or RocksDB wrapper before invoking
     * build().
     */
    IndexManagerBuilder() = default;
    
    /**
     * @brief Set the expression evaluator
     * 
     * @param evaluator Expression evaluator implementation. May be null if the
     *        downstream IndexManager constructor accepts late binding, but the
     *        resulting manager will only be usable once the dependency is set.
     * @return Reference to this builder for chaining
     */
    IndexManagerBuilder& withEvaluator(IExpressionEvaluatorPtr evaluator) {
        evaluator_ = evaluator;
        return *this;
    }
    
    /**
     * @brief Set the storage engine (optional)
     * 
     * @param storage Storage engine implementation. A null value leaves the
     *        storage dependency unset for deferred wiring.
     * @return Reference to this builder for chaining
     */
    IndexManagerBuilder& withStorage(IStorageEnginePtr storage) {
        storage_ = storage;
        return *this;
    }
    
    /**
     * @brief Set the RocksDB wrapper
     * 
     * @param db RocksDB wrapper instance. May be null to skip attaching a
     *        database handle during build().
     * @return Reference to this builder for chaining
     */
    IndexManagerBuilder& withRocksDB(std::shared_ptr<RocksDBWrapper> db) {
        db_ = db;
        return *this;
    }
    
    /**
     * @brief Build the IndexManager instance
     * 
        * @return Shared pointer to configured IndexManager.
        * @throws std::runtime_error if the underlying IndexManager constructor
        *         rejects the configured dependency set.
        * @note RocksDB is optional and, when provided, is attached before the
        *       returned manager is handed back to the caller.
     */
    std::shared_ptr<IndexManager> build() {
        // Create index manager with optional dependencies
        auto index_mgr = std::make_shared<IndexManager>(evaluator_, storage_);
        
        // Set RocksDB if provided
        if (db_) {
            index_mgr->setRocksDB(db_);
        }
        
        return index_mgr;
    }
    
    /**
     * @brief Create a builder with standard default implementations
     * 
     * Returns a builder pre-configured with minimal dependencies. The caller
     * can override individual components before calling build(); the defaults
     * intentionally stay sparse so tests can inject custom collaborators.
     * 
     * @return Builder with default implementations.
     */
    static IndexManagerBuilder standard() {
        IndexManagerBuilder builder;
        // Don't set defaults - let them be optional
        return builder;
    }
    
private:
    IExpressionEvaluatorPtr evaluator_;
    IStorageEnginePtr storage_;
    std::shared_ptr<RocksDBWrapper> db_;
};

} // namespace themis
