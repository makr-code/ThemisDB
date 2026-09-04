/**
 * @file graphql_federation.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/graphql_federation.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string GraphQLFederationSupport::GraphQLSchemaGenerator::pgTypeToGraphQL(const std::string &pg_type) const {
    // Strip modifiers like VARCHAR(255) → VARCHAR
    std::string base = pg_type;
    auto paren       = base.find('(');
    if (paren != std::string::npos) {
        base = base.substr(0, paren);
    }
    // lower-case for comparison
    std::transform(base.begin(), base.end(), base.begin(), [](unsigned char c) { return std::tolower(c); });

    if (base == "integer" || base == "int" || base == "int4" || base == "smallint" || base == "int2") {
        return "Int";
    }
    if (base == "bigint" || base == "int8") {
        return "ID";
    }
    if (base == "real" || base == "float4" || base == "float8" || base == "double precision" || base == "numeric"
        || base == "decimal") {
        return "Float";
    }
    if (base == "boolean" || base == "bool") {
        return "Boolean";
    }
    if (base == "uuid") {
        return "ID";
    }
    if (base == "timestamp" || base == "timestamptz" || base == "date" || base == "time") {
        return "String";
    }
    if (base == "json" || base == "jsonb") {
        return "JSON";
    }
    return "String"; // default
}

std::string GraphQLFederationSupport::GraphQLSchemaGenerator::tableNameToTypeName(const std::string &table) const {
    // snake_case → PascalCase
    std::string result = {};
    bool upper_next = true;
    for (char c : table) {
        if (c == '_') {
            upper_next = true;
            continue;
        }
        result += upper_next ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
        upper_next = false;
    }
    return result;
}

std::string GraphQLFederationSupport::GraphQLSchemaGenerator::columnToField(const std::string &col,
                                                                            const std::string &pg_type,
                                                                            bool nullable) const {
    std::string gql_type = pgTypeToGraphQL(pg_type);
    return "  " + col + ": " + gql_type + (nullable ? "" : "!");
}

// ---------------------------------------------------------------------------
// generateFederatedSchema
// ---------------------------------------------------------------------------

std::string GraphQLFederationSupport::GraphQLSchemaGenerator::generateFederatedSchema(
    const std::vector<InferenceTableSchema> &schemas, const std::string &service_name,
    const std::vector<std::string> &external_entities) {
    std::set<std::string> externals(external_entities.begin(), external_entities.end());

    std::ostringstream out;

    // Federation v2 header
    out << "extend schema\n"
        << "  @link(url: \"https://specs.apollo.dev/federation/v2.3\",\n"
        << "        import: [\"@key\", \"@shareable\", \"@external\","
        << " \"@requires\", \"@provides\"])\n\n";

    out << "# Service: " << service_name << "\n\n";

    // Custom scalars
    out << "scalar JSON\n\n";

    for (const auto &schema : schemas) {
        std::string type_name = tableNameToTypeName(schema.name);
        bool is_external      = externals.count(schema.name) > 0;

        // @key directive from primary keys
        std::string key_fields = {};
        for (const auto &pk : schema.primary_keys) {
            if (!key_fields.empty()) {
                key_fields += " ";
            }
            key_fields += pk;
        }

        if (is_external) {
            out << "extend type " << type_name;
        } else {
            out << "type " << type_name;
        }

        if (!key_fields.empty()) {
            out << " @key(fields: \"" << key_fields << "\")";
        }
        out << " {\n";

        // Build a set of FK columns for annotation
        std::set<std::string> fk_cols;
        std::map<std::string, std::string> fk_targets = {};

        for (const auto &fk : schema.foreign_keys) {
            fk_cols.insert(fk.first);
            fk_targets[fk.first] = fk.second;
        }

        // Fields
        for (const auto &col : schema.columns) {
            std::string pg_type = "text";
            if (schema.column_types.count(col)) {
                pg_type = schema.column_types.at(col);
            }
            bool is_pk
                = std::find(schema.primary_keys.begin(), schema.primary_keys.end(), col) != schema.primary_keys.end();
            bool nullable = !is_pk;

            out << columnToField(col, pg_type, nullable);

            if (is_external && is_pk) {
                out << " @external";
            }

            if (fk_cols.count(col)) {
                out << "  # FK -> " << fk_targets.at(col);
            }
            out << "\n";
        }

        out << "}\n\n";
    }

    return out.str();
}

// ---------------------------------------------------------------------------
// generatePlainSchema
// ---------------------------------------------------------------------------

std::string GraphQLFederationSupport::GraphQLSchemaGenerator::generatePlainSchema(
    const std::vector<InferenceTableSchema> &schemas) {
    std::ostringstream out = {};
    out << "scalar JSON\n\n";

    for (const auto &schema : schemas) {
        out << "type " << tableNameToTypeName(schema.name) << " {\n";
        for (const auto &col : schema.columns) {
            std::string pg_type = "text";
            if (schema.column_types.count(col)) {
                pg_type = schema.column_types.at(col);
            }
            bool is_pk
                = std::find(schema.primary_keys.begin(), schema.primary_keys.end(), col) != schema.primary_keys.end();
            out << columnToField(col, pg_type, !is_pk) << "\n";
        }
        out << "}\n\n";
    }

    // Root query type
    out << "type Query {\n";
    for (const auto &schema : schemas) {
        std::string tn    = tableNameToTypeName(schema.name);
        std::string field = schema.name; // snake_case field name
        out << "  " << field << "(id: ID!): " << tn << "\n";
        out << "  " << field << "s: [" << tn << "!]!\n";
    }
    out << "}\n";

    return out.str();
}

} // namespace importers
} // namespace themis
