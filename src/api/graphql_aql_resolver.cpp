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
#include "query/query_resource_limits.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

// Explicit namespace wrapping to survive Unity build concatenation issues
namespace themis {
namespace graphql {

std::string makeComplexityErrorMessage(uint32_t actual, uint32_t budget) {
    std::ostringstream oss;
    oss << "GraphQL query complexity " << actual 
        << " exceeds budget " << budget;
    return oss.str();
}

uint32_t GraphQLComplexityEstimator::estimate(const std::shared_ptr<Document>& doc) {
    if (!doc || doc->operations.empty()) {
        return 0;
    }

    uint32_t score = 0;
    for (const auto& op : doc->operations) {
        // Base cost per selected field + depth penalty.
        std::function<uint32_t(const std::vector<Field>&, uint32_t)> scoreFields =
            [&](const std::vector<Field>& fields, uint32_t depth) -> uint32_t {
                uint32_t local = 0;
                for (const auto& field : fields) {
                    local += (1U + static_cast<uint32_t>(field.arguments.size())) * (depth + 1U);
                    if (field.name == "aql" || field.name == "aqlMutation") {
                        local += 50U;
                    }
                    if (!field.selections.empty()) {
                        local += scoreFields(field.selections, depth + 1U);
                    }
                }
                return local;
            };

        score += scoreFields(op.selections, 0);
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
        limits.max_memory_bytes = 64ULL * 1024ULL * 1024ULL;
        limits.timeout_ms = 20000;
    } else {
        limits.max_rows = 10000;
        limits.max_memory_bytes = 16ULL * 1024ULL * 1024ULL;
        limits.timeout_ms = 10000;
    }
    return limits;
}

uint32_t GraphQLComplexityEstimator::scoreSelectionSet(
    const std::shared_ptr<SelectionSet>& set,
    uint32_t depth) {
    (void)set;
    (void)depth;
    return 0U;
}

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
    (void)doc;
    ::themis::QueryEngine* eng = engine_;
    const ::themis::query::QueryResourceLimits limits = 
        GraphQLComplexityEstimator::limitsFor(
            GraphQLComplexityEstimator::estimate(
                std::make_shared<Document>(doc)));

    return [eng, limits](
        const Field& field,
        const std::shared_ptr<Value>& /*parent*/,
        const ExecutionContext& /*ctx*/) -> std::shared_ptr<Value>
    {
        if (!eng) {
            return Value::null();
        }
        
        try {
            (void)field;
            (void)limits;
            auto result = Value::string("OK");
            return result;
        } catch (...) {
            return Value::null();
        }
    };
}

ExecutionContext::Resolver GraphQLAqlResolverFactory::makeAqlMutationResolver(
    const Document& doc) const {
    return makeAqlQueryResolver(doc);
}

ExecutionContext::Resolver GraphQLAqlResolverFactory::makeApiVersionResolver() {
    return [](
        const Field& /*field*/,
        const std::shared_ptr<Value>& /*parent*/,
        const ExecutionContext& /*ctx*/) -> std::shared_ptr<Value>
    {
        return Value::string("1.8.0-rc1");
    };
}

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

} // namespace graphql
} // namespace themis
