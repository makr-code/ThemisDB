/**
 * @file query_plan_node.h
 * @brief Query plan node hierarchy with explicit move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace themis {
namespace query {

/// Forward declarations
class QueryContext;

/**
 * @brief Base class for query plan nodes with explicit move semantics
 * 
 * Provides the foundation for query execution plans with support for
 * construction, moving, and tree traversal.
 * 
 * Thread-safety:
 * - NOT thread-safe for move operations
 * - Only move during initialization/teardown
 * 
 * Move Semantics:
 * - Explicit move constructor transfers children and metadata
 * - Explicit move assignment transfers ownership
 * - Copy semantics are deleted
 * - All operations marked noexcept
 * 
 * Invariants:
 * - After move: source is in valid empty state
 * - children_ vector may be empty but is valid
 * - node_type_ describes the node type
 * 
 * @code
 * std::unique_ptr<QueryPlanNode> node1 = createSelectNode();
 * std::unique_ptr<QueryPlanNode> node2 = std::move(node1);  // ✅ Move
 * @endcode
 */
class QueryPlanNode {
protected:
    std::vector<std::unique_ptr<QueryPlanNode>> children_;
    std::string node_type_;

public:
    /// Default constructor
    QueryPlanNode() = default;

    /// Constructor with node type
    explicit QueryPlanNode(const std::string& type) : node_type_(type) {}

    /**
     * @brief Move constructor - transfers children and metadata
     * 
     * @param[in,out] other Source node (will be empty after move)
     * 
     * @post this->children_ contains all child nodes from other
     * @post this->node_type_ = old other.node_type_
     * @post other.children_.empty()
     * @post other.node_type_.empty()
     * 
     * Exception safety: noexcept
     */
    QueryPlanNode(QueryPlanNode&& other) noexcept
        : children_(std::move(other.children_)),
          node_type_(std::move(other.node_type_)) {
        other.children_.clear();
    }

    /**
     * @brief Move assignment operator - transfers children and metadata
     * 
     * @param[in,out] other Source node (will be empty after move)
     * @return Reference to this
     * 
     * @post this->children_ contains all child nodes from other
     * @post other.children_.empty()
     * 
     * Exception safety: noexcept
     */
    QueryPlanNode& operator=(QueryPlanNode&& other) noexcept {
        if (this != &other) {
            children_ = std::move(other.children_);
            node_type_ = std::move(other.node_type_);
            other.children_.clear();
        }
        return *this;
    }

    /// Delete copy constructor
    QueryPlanNode(const QueryPlanNode&) = delete;
    
    /// Delete copy assignment operator
    QueryPlanNode& operator=(const QueryPlanNode&) = delete;

    /// Virtual destructor for polymorphic deletion
    virtual ~QueryPlanNode() = default;

    /**
     * @brief Get the node type
     * @return Node type identifier
     */
    const std::string& getNodeType() const noexcept { return node_type_; }

    /**
     * @brief Get number of child nodes
     * @return Count of child nodes
     */
    size_t getChildCount() const noexcept { return children_.size(); }

    /**
     * @brief Get child node at index
     * @param[in] index Child index
     * @return Pointer to child node, or nullptr if index out of range
     */
    QueryPlanNode* getChild(size_t index) {
        if (index >= children_.size()) return nullptr;
        return children_[index].get();
    }

    /**
     * @brief Get const child node at index
     * @param[in] index Child index
     * @return Const pointer to child node, or nullptr if index out of range
     */
    const QueryPlanNode* getChild(size_t index) const {
        if (index >= children_.size()) return nullptr;
        return children_[index].get();
    }

    /**
     * @brief Add a child node
     * @param[in] child Unique pointer to child node
     */
    void addChild(std::unique_ptr<QueryPlanNode> child) {
        if (child) {
            children_.push_back(std::move(child));
        }
    }

    /**
     * @brief Execute the query plan node
     * @param[in] context Query execution context
     * @return true if execution succeeded
     */
    virtual bool execute(QueryContext* context) = 0;

    /**
     * @brief Get string representation for debugging
     * @return Human-readable node description
     */
    virtual std::string toString() const;
};

/**
 * @brief SELECT query node
 */
class SelectNode : public QueryPlanNode {
private:
    std::vector<std::string> columns_;
    std::vector<std::string> predicates_;

public:
    SelectNode() : QueryPlanNode("SELECT") {}

    /**
     * @brief Move constructor - transfers columns and predicates
     */
    SelectNode(SelectNode&& other) noexcept
        : QueryPlanNode(std::move(other)),
          columns_(std::move(other.columns_)),
          predicates_(std::move(other.predicates_)) {}

    /**
     * @brief Move assignment operator
     */
    SelectNode& operator=(SelectNode&& other) noexcept {
        if (this != &other) {
            QueryPlanNode::operator=(std::move(other));
            columns_ = std::move(other.columns_);
            predicates_ = std::move(other.predicates_);
        }
        return *this;
    }

    SelectNode(const SelectNode&) = delete;
    SelectNode& operator=(const SelectNode&) = delete;

    void addColumn(const std::string& col) { columns_.push_back(col); }
    void addPredicate(const std::string& pred) { predicates_.push_back(pred); }

    const std::vector<std::string>& getColumns() const { return columns_; }
    const std::vector<std::string>& getPredicates() const { return predicates_; }

    bool execute(QueryContext* context) override;
};

/**
 * @brief JOIN query node
 */
class JoinNode : public QueryPlanNode {
private:
    std::string join_type_;
    std::vector<std::string> join_conditions_;

public:
    JoinNode() : QueryPlanNode("JOIN") {}

    /**
     * @brief Move constructor - transfers join conditions
     */
    JoinNode(JoinNode&& other) noexcept
        : QueryPlanNode(std::move(other)),
          join_type_(std::move(other.join_type_)),
          join_conditions_(std::move(other.join_conditions_)) {}

    /**
     * @brief Move assignment operator
     */
    JoinNode& operator=(JoinNode&& other) noexcept {
        if (this != &other) {
            QueryPlanNode::operator=(std::move(other));
            join_type_ = std::move(other.join_type_);
            join_conditions_ = std::move(other.join_conditions_);
        }
        return *this;
    }

    JoinNode(const JoinNode&) = delete;
    JoinNode& operator=(const JoinNode&) = delete;

    void setJoinType(const std::string& type) { join_type_ = type; }
    void addJoinCondition(const std::string& cond) { join_conditions_.push_back(cond); }

    const std::string& getJoinType() const { return join_type_; }
    const std::vector<std::string>& getJoinConditions() const { return join_conditions_; }

    bool execute(QueryContext* context) override;
};

/**
 * @brief AGGREGATE query node
 */
class AggregateNode : public QueryPlanNode {
private:
    std::vector<std::string> group_expressions_;
    std::vector<std::string> aggregate_functions_;

public:
    AggregateNode() : QueryPlanNode("AGGREGATE") {}

    /**
     * @brief Move constructor - transfers expressions and functions
     */
    AggregateNode(AggregateNode&& other) noexcept
        : QueryPlanNode(std::move(other)),
          group_expressions_(std::move(other.group_expressions_)),
          aggregate_functions_(std::move(other.aggregate_functions_)) {}

    /**
     * @brief Move assignment operator
     */
    AggregateNode& operator=(AggregateNode&& other) noexcept {
        if (this != &other) {
            QueryPlanNode::operator=(std::move(other));
            group_expressions_ = std::move(other.group_expressions_);
            aggregate_functions_ = std::move(other.aggregate_functions_);
        }
        return *this;
    }

    AggregateNode(const AggregateNode&) = delete;
    AggregateNode& operator=(const AggregateNode&) = delete;

    void addGroupExpression(const std::string& expr) { group_expressions_.push_back(expr); }
    void addAggregateFunction(const std::string& func) { aggregate_functions_.push_back(func); }

    const std::vector<std::string>& getGroupExpressions() const { return group_expressions_; }
    const std::vector<std::string>& getAggregateFunctions() const { return aggregate_functions_; }

    bool execute(QueryContext* context) override;
};

/**
 * @brief FILTER query node
 */
class FilterNode : public QueryPlanNode {
private:
    std::vector<std::string> filter_expressions_;

public:
    FilterNode() : QueryPlanNode("FILTER") {}

    /**
     * @brief Move constructor - transfers filter expressions
     */
    FilterNode(FilterNode&& other) noexcept
        : QueryPlanNode(std::move(other)),
          filter_expressions_(std::move(other.filter_expressions_)) {}

    /**
     * @brief Move assignment operator
     */
    FilterNode& operator=(FilterNode&& other) noexcept {
        if (this != &other) {
            QueryPlanNode::operator=(std::move(other));
            filter_expressions_ = std::move(other.filter_expressions_);
        }
        return *this;
    }

    FilterNode(const FilterNode&) = delete;
    FilterNode& operator=(const FilterNode&) = delete;

    void addFilterExpression(const std::string& expr) { filter_expressions_.push_back(expr); }

    const std::vector<std::string>& getFilterExpressions() const { return filter_expressions_; }

    bool execute(QueryContext* context) override;
};

}  // namespace query
}  // namespace themis
