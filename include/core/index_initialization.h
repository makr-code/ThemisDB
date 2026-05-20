/*
 * ThemisDB | File: index_initialization.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 108
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #628 Phase 4: IndexManager Refactoring with Dependency Injection (2026-03-11T18:11:59Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    IndexManagerBuilder() = default;
    
    /**
     * @brief Set the expression evaluator
     * 
     * @param evaluator Expression evaluator implementation
     * @return Reference to this builder for chaining
     */
    IndexManagerBuilder& withEvaluator(IExpressionEvaluatorPtr evaluator) {
        evaluator_ = evaluator;
        return *this;
    }
    
    /**
     * @brief Set the storage engine (optional)
     * 
     * @param storage Storage engine implementation
     * @return Reference to this builder for chaining
     */
    IndexManagerBuilder& withStorage(IStorageEnginePtr storage) {
        storage_ = storage;
        return *this;
    }
    
    /**
     * @brief Set the RocksDB wrapper
     * 
     * @param db RocksDB wrapper instance
     * @return Reference to this builder for chaining
     */
    IndexManagerBuilder& withRocksDB(std::shared_ptr<RocksDBWrapper> db) {
        db_ = db;
        return *this;
    }
    
    /**
     * @brief Build the IndexManager instance
     * 
     * @return Shared pointer to configured IndexManager
     * @note RocksDB is optional and can be set later via setRocksDB()
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
     * Returns a builder pre-configured with minimal dependencies.
     * You can override individual components before calling build().
     * 
     * @return Builder with default implementations
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
