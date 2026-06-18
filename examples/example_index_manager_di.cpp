/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_index_manager_di.cpp                       ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     195                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/// @file example_index_manager_di.cpp
/// @brief Example demonstrating IndexManager with Dependency Injection
///
/// This example shows how to use the new IndexManager with dependency
/// injection for expression evaluation and storage.

#include "index/index_manager.h"
#include "core/index_initialization.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/storage_interface.h"
#include <iostream>
#include <memory>

using namespace themis;

// Example 1: Simple expression evaluator implementation
class SimpleExpressionEvaluator : public IExpressionEvaluator {
public:
    bool evaluate(const std::string& expression, const void* context) const override {
        (void)context;
        std::cout << "Evaluating expression: " << expression << std::endl;
        return true;
    }
    
    std::string get_expression_type() const override {
        return "SimpleAQL";
    }
};

// Example 2: Mock storage engine
class MockStorageEngine : public IStorageEngine {
public:
    bool open(const std::string& db_path) override {
        std::cout << "Opening database at: " << db_path << std::endl;
        return true;
    }
    
    void close() override {
        std::cout << "Closing database" << std::endl;
    }
    
    bool put(const std::string& key, const std::string& value) override {
        std::cout << "Put: " << key << " = " << value << std::endl;
        return true;
    }
    
    std::optional<std::string> get(const std::string& key) override {
        std::cout << "Get: " << key << std::endl;
        return std::nullopt;
    }
    
    bool del(const std::string& key) override {
        std::cout << "Delete: " << key << std::endl;
        return true;
    }
};

void example1_basic_usage() {
    std::cout << "\n=== Example 1: Basic Usage ===" << std::endl;
    
    // Create dependencies
    auto evaluator = std::make_shared<SimpleExpressionEvaluator>();
    auto storage = std::make_shared<MockStorageEngine>();
    
    // Create index manager with dependencies
    auto index_mgr = std::make_shared<IndexManager>(evaluator, storage);
    
    // Verify dependencies are set
    std::cout << "Expression evaluator type: " 
              << index_mgr->getExpressionEvaluator()->get_expression_type() 
              << std::endl;
    
    // List indexes (should be empty initially)
    auto indices = index_mgr->listIndexes();
    std::cout << "Number of indexes: " << indices.size() << std::endl;
}

void example2_builder_pattern() {
    std::cout << "\n=== Example 2: Builder Pattern ===" << std::endl;
    
    // Create dependencies
    auto evaluator = std::make_shared<SimpleExpressionEvaluator>();
    auto storage = std::make_shared<MockStorageEngine>();
    
    // Use builder pattern
    auto index_mgr = IndexManagerBuilder::standard()
        .withEvaluator(evaluator)
        .withStorage(storage)
        .build();
    
    std::cout << "Index manager created with builder pattern" << std::endl;
    std::cout << "Expression evaluator type: " 
              << index_mgr->getExpressionEvaluator()->get_expression_type() 
              << std::endl;
}

void example3_late_binding() {
    std::cout << "\n=== Example 3: Late Binding ===" << std::endl;
    
    // Create index manager without dependencies
    auto index_mgr = IndexManager::createDefault();
    std::cout << "Created default index manager" << std::endl;
    
    // Set dependencies later
    auto evaluator = std::make_shared<SimpleExpressionEvaluator>();
    index_mgr->setExpressionEvaluator(evaluator);
    std::cout << "Set expression evaluator: " 
              << index_mgr->getExpressionEvaluator()->get_expression_type() 
              << std::endl;
    
    auto storage = std::make_shared<MockStorageEngine>();
    index_mgr->setStorage(storage);
    std::cout << "Set storage engine" << std::endl;
}

void example4_evaluator_propagation() {
    std::cout << "\n=== Example 4: Evaluator Propagation ===" << std::endl;
    
    // Create index manager
    auto index_mgr = IndexManager::createDefault();
    
    // Set evaluator
    auto evaluator = std::make_shared<SimpleExpressionEvaluator>();
    index_mgr->setExpressionEvaluator(evaluator);
    
    std::cout << "Set evaluator on IndexManager" << std::endl;
    std::cout << "Evaluator will be propagated to concrete managers when RocksDB is set" 
              << std::endl;
    
    // Note: In a real scenario with RocksDB:
    // - VectorIndexManager would receive the evaluator
    // - SecondaryIndexManager would receive the evaluator
    // - GraphIndexManager would receive the evaluator
    // 
    // auto vector_mgr = index_mgr->getVectorIndexManager();
    // assert(vector_mgr->getExpressionEvaluator() == evaluator);
}

void example5_expression_evaluation() {
    std::cout << "\n=== Example 5: Expression Evaluation ===" << std::endl;
    
    auto evaluator = std::make_shared<SimpleExpressionEvaluator>();
    auto index_mgr = std::make_shared<IndexManager>(evaluator, nullptr);
    
    // Get the evaluator and use it
    auto eval = index_mgr->getExpressionEvaluator();
    
    // Evaluate some expressions
    bool result1 = eval->evaluate("price > 100", nullptr);
    std::cout << "Result 1: " << (result1 ? "true" : "false") << std::endl;
    
    bool result2 = eval->evaluate("category = 'electronics'", nullptr);
    std::cout << "Result 2: " << (result2 ? "true" : "false") << std::endl;
}

int main() {
    std::cout << "IndexManager Dependency Injection Examples" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    try {
        example1_basic_usage();
        example2_builder_pattern();
        example3_late_binding();
        example4_evaluator_propagation();
        example5_expression_evaluation();
        
        std::cout << "\n=== All examples completed successfully ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
