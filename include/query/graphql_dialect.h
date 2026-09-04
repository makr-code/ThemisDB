/**
 * @file graphql_dialect.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
    virtual ~IGraphQLDialect() = default;
    virtual GraphQLQuery parse(const std::string& query_str) = 0;
    virtual std::string toAQL(const GraphQLQuery& query) = 0;
    virtual GraphQLResult execute(const std::string& query_str,
                                   const std::map<std::string, std::string>& variables = {}) = 0;
    virtual bool registerTypeResolver(const std::string& type_name,
                                       std::function<std::string(const GraphQLField&)> resolver) = 0;
    virtual std::string schemaSDL() const = 0;
};

}} // namespace themis::query
