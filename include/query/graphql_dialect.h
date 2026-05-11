/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphql_dialect.h                                  ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */
#pragma once
// GraphQL query dialect support for ThemisDB
#include <string>
#include <vector>
#include &lt;map&gt;
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
    std::map<std::string, std::string> extensions;
    bool has_errors() const { return !errors.empty(); }
};

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
