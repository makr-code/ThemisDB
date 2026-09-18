/**
 * @file index_initialization.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class IndexManagerBuilder {
public:
    IndexManagerBuilder() = default;
    
    /**
     * @brief With Evaluator.
     * @param[in] evaluator Input parameter.
     * @return Return value.
     * @details Implements withEvaluator without additional internal calls.
     */
    IndexManagerBuilder& withEvaluator(IExpressionEvaluatorPtr evaluator) {
        evaluator_ = evaluator;
        return *this;
    }
    
    /**
     * @brief With Storage.
     * @param[in] storage Input parameter.
     * @return Return value.
     * @details Implements withStorage without additional internal calls.
     */
    IndexManagerBuilder& withStorage(IStorageEnginePtr storage) {
        storage_ = storage;
        return *this;
    }
    
    /**
     * @brief With Rocks DB.
     * @param[in] db Input parameter.
     * @return Return value.
     * @details Implements withRocksDB without additional internal calls.
     */
    IndexManagerBuilder& withRocksDB(std::shared_ptr<RocksDBWrapper> db) {
        db_ = db;
        return *this;
    }
    
    /**
     * @brief Build.
     * @return Return value.
     * @details Calls: setRocksDB().
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
     * @brief Standard.
     * @return Return value.
     * @details Implements standard without additional internal calls.
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
