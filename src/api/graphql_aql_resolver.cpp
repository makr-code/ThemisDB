/**
 * @file graphql_aql_resolver.cpp
 * @brief GraphQL-to-AQL query resolver implementation.
 *
 * Translates incoming GraphQL queries into ThemisDB AQL statements,
 * executes them, and maps results back to the GraphQL response schema.
 */

// IMPORTANT: This file is explicitly NOT part of Unity build to avoid namespace corruption.
// Compile with: cl /TP /EHsc /W4 /permissive- /Zc:inline ...
// Link into themis_query.lib as separate object file.
//
// When Unity build concatenates multiple .cpp files without proper namespace closure,
// nested namespace definitions in graphql_aql_resolver.cpp would see corrupted namespace context.
// This implementation avoids the issue by using fully-qualified names.

#include "api/graphql_aql_resolver.h"
#include "api/graphql.h"
#include "query/aql_runner.h"
#include "query/query_resource_limits.h"
#include <limits>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>

// Explicit namespace wrapping to survive Unity build concatenation issues
namespace themis {
namespace graphql {

namespace {
constexpr uint32_t kMaxComplexityScoringDepth = 64;

/**
 * @brief Checked Add.
 * @param[in] lhs Input parameter.
 * @param[in] rhs Input parameter.
 * @return Return value.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: max().
 */
uint32_t checkedAdd(uint32_t lhs, uint32_t rhs) {
    if (rhs > std::numeric_limits<uint32_t>::max() - lhs) {
        throw std::runtime_error("GraphQL complexity overflow");
    }
    return lhs + rhs;
}

/**
 * @brief Checked Mul.
 * @param[in] lhs Input parameter.
 * @param[in] rhs Input parameter.
 * @return Return value.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: max().
 */
uint32_t checkedMul(uint32_t lhs, uint32_t rhs) {
    if (lhs != 0
        && rhs > std::numeric_limits<uint32_t>::max() / lhs) {
        throw std::runtime_error("GraphQL complexity overflow");
    }
    return lhs * rhs;
}

/**
 * @brief Score Fields Bounded.
 * @param[in] fields Input parameter.
 * @param[in] depth Input parameter.
 * @return Return value.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: size(), checkedMul(), checkedAdd(), empty().
 */
uint32_t scoreFieldsBounded(const std::vector<Field>& fields, uint32_t depth) {
    if (depth > kMaxComplexityScoringDepth) {
        throw std::runtime_error("GraphQL complexity depth overflow");
    }

    uint32_t local = 0;
    for (const auto& field : fields) {
        const uint32_t argument_cost = static_cast<uint32_t>(field.arguments.size());
        const uint32_t field_cost =
            checkedMul(1 + argument_cost, depth + 1);
        local = checkedAdd(local, field_cost);

        if (field.name == "aql" || field.name == "aqlMutation") {
            local = checkedAdd(local, 50);
        }

        if (!field.selections.empty()) {
            local = checkedAdd(local, scoreFieldsBounded(field.selections, depth + 1));
        }
    }
    return local;
}

std::string escapeSingleQuotedAqlString(std::string_view input) {
    std::string escaped = {};
    escaped.reserve(input.size());
    for (const char c : input) {
        if (c == '\'') {
            escaped += "''";
        } else {
            escaped.push_back(c);
        }
    }
    return escaped;
}

std::string escapeBacktickIdentifier(std::string_view input) {
    std::string escaped = {};
    escaped.reserve(input.size());
    for (const char c : input) {
        if (c == '`') {
            escaped += "``";
        } else {
            escaped.push_back(c);
        }
    }
    return escaped;
}

std::string jsonToAqlLiteral(const nlohmann::json& value) {
    if (value.is_null() || value.is_boolean() || value.is_number() ||
        value.is_array() || value.is_object()) {
        return value.dump();
    }
    if (value.is_string()) {
        return "'" + escapeSingleQuotedAqlString(value.get<std::string>()) + "'";
    }
    return "null";
}

std::string applyBindVariablesToAql(const std::string& query,
                                    const nlohmann::json& bind_vars) {
    std::string resolved_query = query;
    std::vector<std::pair<std::string, nlohmann::json>> ordered_bindings = {};
    ordered_bindings.reserve(bind_vars.size());
    for (auto it = bind_vars.begin(); it != bind_vars.end(); ++it) {
        ordered_bindings.emplace_back(it.key(), it.value());
    }
    std::sort(ordered_bindings.begin(), ordered_bindings.end(),
              [](const auto& lhs, const auto& rhs) {
                  return lhs.first.size() > rhs.first.size();
              });

    const auto is_placeholder_char = [](char c) {
        const auto uc = static_cast<unsigned char>(c);
        return std::isalnum(uc) || c == '_' || c == '@';
    };

    for (const auto& [key, raw_value] : ordered_bindings) {
        const std::string placeholder = "@" + key;
        std::string replacement = {};

        if (!key.empty() && key.front() == '@') {
            if (!raw_value.is_string()) {
                throw std::runtime_error(
                    "AQL collection bind variable '" + key + "' must be a string");
            }
            replacement = "`" + escapeBacktickIdentifier(raw_value.get<std::string>()) + "`";
        } else {
            replacement = jsonToAqlLiteral(raw_value);
        }

        size_t pos = 0;
        while ((pos = resolved_query.find(placeholder, pos)) != std::string::npos) {
            const bool at_start = (pos == 0);
            const bool at_end = (pos + placeholder.size() >= resolved_query.size());
            const bool has_prefix_boundary =
                at_start || !is_placeholder_char(resolved_query[pos - 1]);
            const bool has_suffix_boundary =
                at_end || !is_placeholder_char(resolved_query[pos + placeholder.size()]);
            if (!has_prefix_boundary || !has_suffix_boundary) {
                ++pos;
                continue;
            }
            resolved_query.replace(pos, placeholder.size(), replacement);
            pos += replacement.size();
        }
    }
    return resolved_query;
}
} // namespace

/**
 * @brief Make Complexity Error Message.
 * @param[in] actual Input parameter.
 * @param[in] budget Input parameter.
 * @return Return value.
 * @details Calls: str().
 */
std::string makeComplexityErrorMessage(uint32_t actual, uint32_t budget) {
    std::ostringstream oss = {};
    oss << "GraphQL query complexity " << actual 
        << " exceeds budget " << budget;
    return oss.str();
}

/**
 * @brief Estimate.
 * @param[in] doc Input parameter.
 * @return Return value.
 * @details Calls: empty(), checkedAdd(), scoreFieldsBounded().
 */
uint32_t GraphQLComplexityEstimator::estimate(const std::shared_ptr<Document>& doc) {
    if (!doc || doc->operations.empty()) {
        return 0;
    }

    uint32_t score = 0;
    for (const auto& op : doc->operations) {
        score = checkedAdd(score, scoreFieldsBounded(op.selections, 0));
    }
    return score;
}

::themis::query::QueryResourceLimits GraphQLComplexityEstimator::limitsFor(
    uint32_t complexity) {
    if (complexity > kGraphQLMaxComplexity) {
        throw std::runtime_error(
            makeComplexityErrorMessage(complexity, kGraphQLMaxComplexity));
    }
    
    ::themis::query::QueryResourceLimits limits;
    if (complexity <= 100) {
        // Low complexity keeps row/memory limits unlimited and only applies a
        // bounded execution timeout.
        limits.max_rows = 0;
        limits.max_memory_bytes = 0;
        limits.timeout_ms = 30000;
    } else if (complexity <= 500) {
        limits.max_rows = 50000;
        limits.max_memory_bytes = 64 * 1024 * 1024;
        limits.timeout_ms = 20000;
    } else {
        limits.max_rows = 10000;
        limits.max_memory_bytes = 16 * 1024 * 1024;
        limits.timeout_ms = 10000;
    }
    return limits;
}

/**
 * @brief Score Selection Set.
 * @param[in] set Input parameter.
 * @param[in] depth Input parameter.
 * @return Return value.
 * @details Implements scoreSelectionSet without additional internal calls.
 */
uint32_t GraphQLComplexityEstimator::scoreSelectionSet(
    const std::shared_ptr<SelectionSet>& set,
    uint32_t depth) {
    (void)set;
    (void)depth;
    return 0;
}

/**
 * @brief Json To Gql Value.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: is_null(), Value::null(), is_boolean(), Value::boolean(), is_number_integer(), Value::integer(), is_number_float(), Value::floating().
 */
std::shared_ptr<Value> jsonToGqlValue(const nlohmann::json& j) {
    if (j.is_null()) {
        return Value::null();
    } else if (j.is_boolean()) {
        return Value::boolean(j.get<bool>());
    } else if (j.is_number_integer()) {
        return Value::integer(j.get<int64_t>());
    } else if (j.is_number_float()) {
        return Value::floating(j.get<double>());
    } else if (j.is_string()) {
        return Value::string(j.get<std::string>());
    } else if (j.is_array()) {
        ValueList vec;
        vec.reserve(j.size());
        for (const auto& elem : j) {
            vec.push_back(jsonToGqlValue(elem));
        }
        return Value::list(std::move(vec));
    } else if (j.is_object()) {
        ValueMap obj;
        for (const auto& [key, val] : j.items()) {
            obj[key] = jsonToGqlValue(val);
        }
        return Value::object(std::move(obj));
    }
    return Value::null();
}

/**
 * @brief Gql Value To Json.
 * @param[in] v Input parameter.
 * @return Return value.
 * @details Calls: nlohmann::json(), isNull(), isBool(), asBool(), isInt(), asInt(), isFloat(), asFloat().
 */
nlohmann::json gqlValueToJson(const std::shared_ptr<Value>& v) {
    if (!v) {
        return nlohmann::json();
    }
    
    if (v->isNull()) {
        return nlohmann::json();
    } else if (v->isBool()) {
        return nlohmann::json(v->asBool());
    } else if (v->isInt()) {
        return nlohmann::json(v->asInt());
    } else if (v->isFloat()) {
        return nlohmann::json(v->asFloat());
    } else if (v->isString() || v->isEnum() || v->isVariableRef()) {
        return nlohmann::json(v->asString());
    } else if (v->isList()) {
        const auto& arr = v->asList();
        nlohmann::json jsonArray = nlohmann::json::array();
        for (const auto& elem : arr) {
            jsonArray.push_back(gqlValueToJson(elem));
        }
        return jsonArray;
    } else if (v->isObject()) {
        auto obj = v->asObject();
        nlohmann::json jsonObj = nlohmann::json::object();
        for (const auto& [key, val] : obj) {
            jsonObj[key] = gqlValueToJson(val);
        }
        return jsonObj;
    }
    
    return nlohmann::json();
}

ExecutionContext::Resolver GraphQLAqlResolverFactory::makeAqlQueryResolver(
    const Document& doc) const {
    GraphQLAqlResolverFactory self(engine_);
    ::themis::QueryEngine* eng = engine_;
    const ::themis::query::QueryResourceLimits limits = 
        GraphQLComplexityEstimator::limitsFor(
            GraphQLComplexityEstimator::estimate(
                std::make_shared<Document>(doc)));

    return [eng, limits, self](
        const Field& field,
        const std::shared_ptr<Value>& /*parent*/,
        const ExecutionContext& /*ctx*/) -> std::shared_ptr<Value>
    {
        const std::string query = self.extractStringArg(field, "query");
        if (query.empty()) {
            throw std::runtime_error(
                "GraphQL field '" + field.name + "' requires non-empty string argument 'query'");
        }

        std::string resolved_query = query;
        auto variables_it = field.arguments.find("variables");
        if (variables_it != field.arguments.end() && variables_it->second && !variables_it->second->isNull()) {
            if (!variables_it->second->isObject()) {
                throw std::runtime_error(
                    "GraphQL field '" + field.name + "' argument 'variables' must be a JSON object");
            }

            const nlohmann::json bind_vars = gqlValueToJson(variables_it->second);
            if (!bind_vars.is_object()) {
                throw std::runtime_error(
                    "GraphQL field '" + field.name + "' argument 'variables' must map to a JSON object");
            }
            resolved_query = applyBindVariablesToAql(query, bind_vars);
        }

        if (!eng) {
            return Value::null();
        }

        auto aql_result = self.executeAqlWithLimits(resolved_query, *eng, limits);
        if (!aql_result) {
            throw std::runtime_error(
                "AQL execution failed for GraphQL field '" + field.name + "': " +
                aql_result.error().message());
        }

        return jsonToGqlValue(*aql_result);
    };
}

ExecutionContext::Resolver GraphQLAqlResolverFactory::makeAqlMutationResolver(
    const Document& doc) const {
    return makeAqlQueryResolver(doc);
}

/**
 * @brief Make Api Version Resolver.
 * @return Return value.
 * @details Calls: Value::string().
 */
ExecutionContext::Resolver GraphQLAqlResolverFactory::makeApiVersionResolver() {
    return [](
        const Field& /*field*/,
        const std::shared_ptr<Value>& /*parent*/,
        const ExecutionContext& /*ctx*/) -> std::shared_ptr<Value>
    {
        return Value::string("1.8.0-rc1");
    };
}

/**
 * @brief Make Schema Version Resolver.
 * @return Return value.
 * @details Calls: Value::string().
 */
ExecutionContext::Resolver GraphQLAqlResolverFactory::makeSchemaVersionResolver() {
    return [](
        const Field& /*field*/,
        const std::shared_ptr<Value>& /*parent*/,
        const ExecutionContext& /*ctx*/) -> std::shared_ptr<Value>
    {
        return Value::string("2.0.0");
    };
}

void GraphQLAqlResolverFactory::injectResolvers(
    ExecutionContext& ctx,
    const Document& doc,
    ::themis::QueryEngine* eng) {
    /**
     * @brief Factory.
     * @param[in] eng Input parameter.
     * @return Return value.
     */
    GraphQLAqlResolverFactory factory(eng);
    ctx.resolvers["aql"] = factory.makeAqlQueryResolver(doc);
    ctx.resolvers["aqlMutation"] = factory.makeAqlMutationResolver(doc);
    ctx.resolvers["apiVersion"] = makeApiVersionResolver();
    ctx.resolvers["schemaVersion"] = makeSchemaVersionResolver();
}

std::string GraphQLAqlResolverFactory::extractStringArg(
    const Field& field,
    const std::string& argName) const {
    auto it = field.arguments.find(argName);
    if (it == field.arguments.end() || !it->second || !it->second->isString()) {
        return {};
    }
    return it->second->asString();
}

Result<nlohmann::json> GraphQLAqlResolverFactory::executeAqlWithLimits(
    const std::string& aql,
    ::themis::QueryEngine& eng,
    const ::themis::query::QueryResourceLimits& limits) const {
    return ::themis::executeAqlWithLimits(aql, eng, limits);
}

} // namespace graphql
} // namespace themis
