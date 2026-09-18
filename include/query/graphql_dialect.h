/**
 * @file graphql_dialect.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
// GraphQL query dialect support for ThemisDB
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace themis { namespace query {

struct GraphQLField {
    std::string name;
    std::string alias;
    std::vector<GraphQLField> sub_fields;
    std::map<std::string, std::string> arguments;
    std::vector<std::string> directives;
};

struct GraphQLQuery {
    std::string operation_type;
    std::string operation_name;
    std::vector<GraphQLField> selection_set;
    std::map<std::string, std::string> variables;
    std::string raw_query;
};

struct GraphQLResult {
    std::string data_json;
    std::vector<std::map<std::string, std::string>> errors;
    std::map<std::string, std::string> extensions = {};

    bool has_errors() const { return !errors.empty(); }
};

/** @brief I graph ql dialect. */
class IGraphQLDialect {
public:
    /**
     * @brief TBD: Describe ~IGraphQLDialect.
     * @return Return value.
     */
    virtual ~IGraphQLDialect() = default;
    /**
     * @brief TBD: Describe parse.
     * @param[in] query_str Input parameter.
     * @return Return value.
     */
    virtual GraphQLQuery parse(const std::string& query_str) = 0;
    /**
     * @brief TBD: Describe toAQL.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    virtual std::string toAQL(const GraphQLQuery& query) = 0;
    virtual GraphQLResult execute(const std::string& query_str,
                                   const std::map<std::string, std::string>& variables = {}) = 0;
    virtual bool registerTypeResolver(const std::string& type_name,
                                       std::function<std::string(const GraphQLField&)> resolver) = 0;
    /**
     * @brief TBD: Describe schemaSDL.
     * @return Return value.
     */
    virtual std::string schemaSDL() const = 0;
};

}} // namespace themis::query
