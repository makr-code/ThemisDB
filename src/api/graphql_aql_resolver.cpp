// IMPORTANT: This file is explicitly NOT part of Unity build to avoid namespace corruption.
// Compile with: cl /TP /EHsc /W4 /permissive- /Zc:inline ...
// Link into themis_query.lib as separate object file.
//
// When Unity build concatenates multiple .cpp files without proper namespace closure,
// nested namespace definitions in graphql_aql_resolver.cpp would see corrupted namespace context.
// This implementation avoids the issue by using fully-qualified names.

#include "api/graphql_aql_resolver.h"
#include "api/graphql.h"
#include "query/query_engine.h"
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
    if (!doc || !doc->operation) {
        return 0;
    }
    if (doc->operation->selectionSet) {
        return scoreSelectionSet(doc->operation->selectionSet, 0);
    }
    return 0;
}

::themis::query::QueryResourceLimits GraphQLComplexityEstimator::limitsFor(
    uint32_t complexity) {
    if (complexity > kGraphQLMaxComplexity) {
        throw std::runtime_error(
            makeComplexityErrorMessage(complexity, kGraphQLMaxComplexity));
    }
    
    ::themis::query::QueryResourceLimits limits;
    if (complexity < 100) {
        limits.max_rows = 1000000;
        limits.timeout_ms = 60000;
    } else if (complexity < 500) {
        limits.max_rows = 50000;
        limits.timeout_ms = 20000;
    } else {
        limits.max_rows = 10000;
        limits.timeout_ms = 10000;
    }
    return limits;
}

uint32_t GraphQLComplexityEstimator::scoreSelectionSet(
    const std::shared_ptr<SelectionSet>& set,
    uint32_t depth) {
    if (!set || set->selections.empty()) {
        return 0;
    }
    
    uint32_t score = 0;
    uint32_t depthMultiplier = 1 + depth;
    
    for (const auto& sel : set->selections) {
        if (sel && sel->field) {
            score += depthMultiplier * 10;
            if (sel->field->selectionSet) {
                score += scoreSelectionSet(sel->field->selectionSet, depth + 1);
            }
        }
    }
    
    return score;
}

std::shared_ptr<Value> jsonToGqlValue(const nlohmann::json& j) {
    if (j.is_null()) {
        return Value::null();
    } else if (j.is_boolean()) {
        return std::make_shared<Value>(j.get<bool>());
    } else if (j.is_number_integer()) {
        return std::make_shared<Value>(j.get<int64_t>());
    } else if (j.is_number_float()) {
        return std::make_shared<Value>(j.get<double>());
    } else if (j.is_string()) {
        return std::make_shared<Value>(j.get<std::string>());
    } else if (j.is_array()) {
        std::vector<std::shared_ptr<Value>> vec;
        for (const auto& elem : j) {
            vec.push_back(jsonToGqlValue(elem));
        }
        return std::make_shared<Value>(vec);
    } else if (j.is_object()) {
        std::map<std::string, std::shared_ptr<Value>> obj;
        for (const auto& [key, val] : j.items()) {
            obj[key] = jsonToGqlValue(val);
        }
        return std::make_shared<Value>(obj);
    }
    return Value::null();
}

nlohmann::json gqlValueToJson(const std::shared_ptr<Value>& v) {
    if (!v) {
        return nlohmann::json();
    }
    
    if (v->isNull()) {
        return nlohmann::json();
    } else if (v->isBoolean()) {
        return nlohmann::json(v->asBoolean());
    } else if (v->isInt()) {
        return nlohmann::json(v->asInt());
    } else if (v->isFloat()) {
        return nlohmann::json(v->asFloat());
    } else if (v->isString()) {
        return nlohmann::json(v->asString());
    } else if (v->isArray()) {
        auto arr = v->asArray();
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
    ::themis::query::QueryEngine* eng = engine_;
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
            auto result = std::make_shared<Value>("OK");
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
        return std::make_shared<Value>(std::string("1.8.0-rc1"));
    };
}

ExecutionContext::Resolver GraphQLAqlResolverFactory::makeSchemaVersionResolver() {
    return [](
        const Field& /*field*/,
        const std::shared_ptr<Value>& /*parent*/,
        const ExecutionContext& /*ctx*/) -> std::shared_ptr<Value>
    {
        return std::make_shared<Value>(std::string("2.0.0"));
    };
}

void GraphQLAqlResolverFactory::injectResolvers(
    ExecutionContext& ctx,
    const Document& doc,
    ::themis::query::QueryEngine* eng) {
    GraphQLAqlResolverFactory factory(eng);
}

std::string GraphQLAqlResolverFactory::extractStringArg(
    const Field& field,
    const std::string& argName) const {
    return "";
}

::tl::expected<nlohmann::json, ::themis::query::QueryError> 
GraphQLAqlResolverFactory::executeAqlWithLimits(
    const std::string& aql,
    ::themis::query::QueryEngine& eng,
    const ::themis::query::QueryResourceLimits& limits) const {
    return nlohmann::json::object();
}

} // namespace graphql
} // namespace themis
