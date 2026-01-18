#pragma once

#include <memory>
#include <string>

namespace themis {

/**
 * @brief Interface for expression evaluation
 * 
 * Provides abstract interface for evaluating filter expressions
 * and WHERE clauses without depending on concrete query engine implementations.
 */
class IExpressionEvaluator {
public:
    virtual ~IExpressionEvaluator() = default;
    
    /**
     * @brief Evaluate an expression against provided context
     * 
     * @param expression The filter expression to evaluate
     * @param context Opaque context pointer (e.g., document, row data)
     * @return true if expression evaluates to true, false otherwise
     */
    virtual bool evaluate(const std::string& expression, const void* context) = 0;
    
    /**
     * @brief Get the type of expression language supported
     * 
     * @return String identifying the expression language (e.g., "AQL", "SQL")
     */
    virtual std::string get_expression_type() const = 0;
};

/// Shared pointer type for IExpressionEvaluator
using IExpressionEvaluatorPtr = std::shared_ptr<IExpressionEvaluator>;

} // namespace themis
