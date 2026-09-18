/**
 * @file window_evaluator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>
#include "query/aql_parser.h"

namespace themis {
namespace query {


enum class WindowFunctionType {
    ROW_NUMBER,      // Sequential row number
    RANK,            // Rank with gaps
    DENSE_RANK,      // Rank without gaps
    LAG,             // Previous row value
    LEAD,            // Next row value
    FIRST_VALUE,     // First value in window
    LAST_VALUE,      // Last value in window
    NTH_VALUE        // N-th value in window (Phase 2)
};

enum class WindowFrameType {
    ROWS,            // Physical rows (count-based)
    RANGE            // Logical range (value-based)
};

struct WindowFrameBound {
    enum class BoundType {
        UNBOUNDED_PRECEDING,    // Start of partition
        UNBOUNDED_FOLLOWING,    // End of partition
        CURRENT_ROW,            // Current row
        PRECEDING,              // N rows/values before current
        FOLLOWING               // N rows/values after current
    };
    
    BoundType type;
    int64_t offset = 0;  // For PRECEDING/FOLLOWING
    
    /**
     * @brief Unbounded Preceding.
     * @return Return value.
     * @details Implements unboundedPreceding without additional internal calls.
     */
    static WindowFrameBound unboundedPreceding() {
        return {BoundType::UNBOUNDED_PRECEDING, 0};
    }
    
    /**
     * @brief Unbounded Following.
     * @return Return value.
     * @details Implements unboundedFollowing without additional internal calls.
     */
    static WindowFrameBound unboundedFollowing() {
        return {BoundType::UNBOUNDED_FOLLOWING, 0};
    }
    
    /**
     * @brief Current Row.
     * @return Return value.
     * @details Implements currentRow without additional internal calls.
     */
    static WindowFrameBound currentRow() {
        return {BoundType::CURRENT_ROW, 0};
    }
    
    /**
     * @brief Preceding.
     * @param[in] n Input parameter.
     * @return Return value.
     * @details Implements preceding without additional internal calls.
     */
    static WindowFrameBound preceding(int64_t n) {
        return {BoundType::PRECEDING, n};
    }
    
    /**
     * @brief Following.
     * @param[in] n Input parameter.
     * @return Return value.
     * @details Implements following without additional internal calls.
     */
    static WindowFrameBound following(int64_t n) {
        return {BoundType::FOLLOWING, n};
    }
};

struct WindowFrame {
    WindowFrameType type = WindowFrameType::RANGE;
    WindowFrameBound start = WindowFrameBound::unboundedPreceding();
    WindowFrameBound end = WindowFrameBound::currentRow();
    
    // Default: RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
    WindowFrame() = default;
    
    WindowFrame(WindowFrameType t, WindowFrameBound s, WindowFrameBound e)
        : type(t), start(s), end(e) {}
};

struct WindowEvalSpec {
    std::string name;  // Named window (e.g., "w" in WINDOW w AS (...))
    std::vector<std::shared_ptr<Expression>> partitionBy;  // PARTITION BY expressions
    std::vector<SortSpec> orderBy;                         // ORDER BY specifications
    WindowFrame frame;                                     // Frame definition
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

struct WindowFunctionCall {
    WindowFunctionType funcType;
    std::shared_ptr<Expression> argument;  // For LAG/LEAD/FIRST_VALUE/LAST_VALUE/NTH_VALUE
    int64_t offset = 1;                    // For LAG/LEAD (default: 1)
    std::shared_ptr<Expression> defaultValue;  // Default when out of bounds
    std::string windowName;                // Reference to named window (e.g., "w")
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

class WindowEvaluator {
public:
    WindowEvaluator() = default;
    
    /**
     * @brief Evaluate.
     * @param[in] rows Input parameter.
     * @param[in] windowSpec Input parameter.
     * @param[in] windowFunc Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> evaluate(
        const std::vector<nlohmann::json>& rows,
        const WindowEvalSpec& windowSpec,
        const WindowFunctionCall& windowFunc,
        const std::string& forVariable
    );
    
private:
    /**
     * @brief Partition Rows.
     * @param[in] rows Input parameter.
     * @param[in] partitionBy Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<size_t>> partitionRows(
        const std::vector<nlohmann::json>& rows,
        const std::vector<std::shared_ptr<Expression>>& partitionBy,
        const std::string& forVariable
    );
    
    /**
     * @brief Sort Partition.
     * @param[in] rows Input parameter.
     * @param[in] partition Input parameter.
     * @param[in] orderBy Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::vector<size_t> sortPartition(
        const std::vector<nlohmann::json>& rows,
        const std::vector<size_t>& partition,
        const std::vector<SortSpec>& orderBy,
        const std::string& forVariable
    );
    
    /**
     * @brief Evaluate Row Number.
     * @param[in] partitionSize Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> evaluateRowNumber(size_t partitionSize);
    
    /**
     * @brief Evaluate Rank.
     * @param[in] rows Input parameter.
     * @param[in] sortedIndices Input parameter.
     * @param[in] orderBy Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> evaluateRank(
        const std::vector<nlohmann::json>& rows,
        const std::vector<size_t>& sortedIndices,
        const std::vector<SortSpec>& orderBy,
        const std::string& forVariable
    );
    
    /**
     * @brief Evaluate Dense Rank.
     * @param[in] rows Input parameter.
     * @param[in] sortedIndices Input parameter.
     * @param[in] orderBy Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> evaluateDenseRank(
        const std::vector<nlohmann::json>& rows,
        const std::vector<size_t>& sortedIndices,
        const std::vector<SortSpec>& orderBy,
        const std::string& forVariable
    );
    
    /**
     * @brief Evaluate Lag.
     * @param[in] rows Input parameter.
     * @param[in] sortedIndices Input parameter.
     * @param[in] argument Input parameter.
     * @param[in] offset Input parameter.
     * @param[in] defaultValue Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> evaluateLag(
        const std::vector<nlohmann::json>& rows,
        const std::vector<size_t>& sortedIndices,
        const std::shared_ptr<Expression>& argument,
        int64_t offset,
        const std::shared_ptr<Expression>& defaultValue,
        const std::string& forVariable
    );
    
    /**
     * @brief Evaluate Lead.
     * @param[in] rows Input parameter.
     * @param[in] sortedIndices Input parameter.
     * @param[in] argument Input parameter.
     * @param[in] offset Input parameter.
     * @param[in] defaultValue Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> evaluateLead(
        const std::vector<nlohmann::json>& rows,
        const std::vector<size_t>& sortedIndices,
        const std::shared_ptr<Expression>& argument,
        int64_t offset,
        const std::shared_ptr<Expression>& defaultValue,
        const std::string& forVariable
    );
    
    /**
     * @brief Evaluate First Value.
     * @param[in] rows Input parameter.
     * @param[in] sortedIndices Input parameter.
     * @param[in] argument Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> evaluateFirstValue(
        const std::vector<nlohmann::json>& rows,
        const std::vector<size_t>& sortedIndices,
        const std::shared_ptr<Expression>& argument,
        const std::string& forVariable
    );
    
    /**
     * @brief Evaluate Last Value.
     * @param[in] rows Input parameter.
     * @param[in] sortedIndices Input parameter.
     * @param[in] argument Input parameter.
     * @param[in] frame Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> evaluateLastValue(
        const std::vector<nlohmann::json>& rows,
        const std::vector<size_t>& sortedIndices,
        const std::shared_ptr<Expression>& argument,
        const WindowFrame& frame,
        const std::string& forVariable
    );
    
    /**
     * @brief Compare Rows.
     * @param[in] row1 Input parameter.
     * @param[in] row2 Input parameter.
     * @param[in] orderBy Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    int compareRows(
        const nlohmann::json& row1,
        const nlohmann::json& row2,
        const std::vector<SortSpec>& orderBy,
        const std::string& forVariable
    );
    
    /**
     * @brief Evaluate Expression.
     * @param[in] expr Input parameter.
     * @param[in] row Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    nlohmann::json evaluateExpression(
        const std::shared_ptr<Expression>& expr,
        const nlohmann::json& row,
        const std::string& forVariable
    );
    
    /**
     * @brief Make Partition Key.
     * @param[in] row Input parameter.
     * @param[in] partitionBy Input parameter.
     * @param[in] forVariable Input parameter.
     * @return Return value.
     */
    std::string makePartitionKey(
        const nlohmann::json& row,
        const std::vector<std::shared_ptr<Expression>>& partitionBy,
        const std::string& forVariable
    );
};

} // namespace query
} // namespace themis
