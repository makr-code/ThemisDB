/**
 * @file aql_query_template_library.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_query_template_library.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace aql {

// ============================================================================
// Construction
// ============================================================================

AQLQueryTemplateLibrary::AQLQueryTemplateLibrary() {
    registerBuiltins_();
}

// ============================================================================
// Registration
// ============================================================================

void AQLQueryTemplateLibrary::registerTemplate(const AQLQueryTemplate& tmpl) {
    if (tmpl.id.empty()) {
        throw std::invalid_argument("AQLQueryTemplateLibrary: template id must not be empty");
    }
    if (index_by_id_.count(tmpl.id)) {
        throw std::invalid_argument(
            "AQLQueryTemplateLibrary: template id already registered: " + tmpl.id
        );
    }
    index_by_id_[tmpl.id] = templates_.size();
    templates_.push_back(tmpl);
}

// ============================================================================
// Lookup
// ============================================================================

const std::vector<AQLQueryTemplate>& AQLQueryTemplateLibrary::all() const {
    return templates_;
}

static std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

std::vector<AQLQueryTemplate> AQLQueryTemplateLibrary::findByTag(
    const std::string& tag
) const {
    std::string lower_tag = toLower(tag);
    std::vector<AQLQueryTemplate> result = {};

    for (const auto& tmpl : templates_) {
        for (const auto& t : tmpl.tags) {
            if (toLower(t) == lower_tag) {
                result.push_back(tmpl);
                break;
            }
        }
    }
    return result;
}

std::vector<AQLQueryTemplate> AQLQueryTemplateLibrary::search(
    const std::string& keyword
) const {
    std::string lower_kw = toLower(keyword);
    std::vector<AQLQueryTemplate> result = {};

    for (const auto& tmpl : templates_) {
        if (toLower(tmpl.name).find(lower_kw) != std::string::npos ||
            toLower(tmpl.description).find(lower_kw) != std::string::npos) {
            result.push_back(tmpl);
            continue;
        }
        // Also search tags
        for (const auto& t : tmpl.tags) {
            if (toLower(t).find(lower_kw) != std::string::npos) {
                result.push_back(tmpl);
                break;
            }
        }
    }
    return result;
}

const AQLQueryTemplate* AQLQueryTemplateLibrary::findById(
    const std::string& id
) const {
    auto it = index_by_id_.find(id);
    if (it == index_by_id_.end()) {
        return nullptr;
    }
    return &templates_[it->second];
}

// ============================================================================
// Instantiation
// ============================================================================

std::string AQLQueryTemplateLibrary::instantiate(
    const std::string& id,
    const std::unordered_map<std::string, std::string>& parameters
) const {
    const AQLQueryTemplate* tmpl = findById(id);
    if (!tmpl) {
        throw std::invalid_argument(
            "AQLQueryTemplateLibrary: unknown template id: " + id
        );
    }
    return instantiate(*tmpl, parameters);
}

/*static*/
std::string AQLQueryTemplateLibrary::instantiate(
    const AQLQueryTemplate& tmpl,
    const std::unordered_map<std::string, std::string>& parameters
) {
    // Verify all required parameters are provided
    for (const auto& param : tmpl.parameters) {
        if (!parameters.count(param)) {
            throw std::invalid_argument(
                "AQLQueryTemplateLibrary: missing required parameter '" +
                param + "' for template '" + tmpl.id + "'"
            );
        }
    }

    // Replace {{param}} tokens
    std::string result = tmpl.template_body;
    for (const auto& [key, value] : parameters) {
        const std::string placeholder = "{{" + key + "}}";
        std::string::size_type pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.size(), value);
            pos += value.size();
        }
    }
    return result;
}

// ============================================================================
// Built-in templates
// ============================================================================

void AQLQueryTemplateLibrary::registerBuiltins_() {

    // ------------------------------------------------------------------
    // 1. Simple full-collection scan
    // ------------------------------------------------------------------
    registerTemplate({
        "simple_scan",
        "Simple Collection Scan",
        "Iterate over every document in a collection and return it.",
        "FOR {{var}} IN {{collection}}\n  RETURN {{var}}",
        {"basic", "scan", "read"},
        {"var", "collection"}
    });

    // ------------------------------------------------------------------
    // 2. Filtered scan
    // ------------------------------------------------------------------
    registerTemplate({
        "filtered_scan",
        "Filtered Scan",
        "Iterate over a collection and return documents matching a condition.",
        "FOR {{var}} IN {{collection}}\n"
        "  FILTER {{condition}}\n"
        "  RETURN {{var}}",
        {"basic", "filter", "read"},
        {"var", "collection", "condition"}
    });

    // ------------------------------------------------------------------
    // 3. Paginated query
    // ------------------------------------------------------------------
    registerTemplate({
        "paginated_query",
        "Paginated Query",
        "Return a page of results with a configurable offset and page size.",
        "FOR {{var}} IN {{collection}}\n"
        "  SORT {{sort_field}} {{sort_dir}}\n"
        "  LIMIT {{offset}}, {{count}}\n"
        "  RETURN {{var}}",
        {"pagination", "sort", "basic"},
        {"var", "collection", "sort_field", "sort_dir", "offset", "count"}
    });

    // ------------------------------------------------------------------
    // 4. Projection
    // ------------------------------------------------------------------
    registerTemplate({
        "projection",
        "Field Projection",
        "Return only specific fields from each document.",
        "FOR {{var}} IN {{collection}}\n"
        "  RETURN { {{projection}} }",
        {"basic", "projection", "read"},
        {"var", "collection", "projection"}
    });

    // ------------------------------------------------------------------
    // 5. Aggregation — count per group
    // ------------------------------------------------------------------
    registerTemplate({
        "group_count",
        "Group and Count",
        "Group documents by a field and count how many fall into each group.",
        "FOR {{var}} IN {{collection}}\n"
        "  COLLECT {{group_var}} = {{var}}.{{group_field}}\n"
        "  WITH COUNT INTO count\n"
        "  SORT count DESC\n"
        "  RETURN { {{group_var}}: {{group_var}}, count: count }",
        {"aggregation", "group", "count"},
        {"var", "collection", "group_var", "group_field"}
    });

    // ------------------------------------------------------------------
    // 6. Aggregation — sum
    // ------------------------------------------------------------------
    registerTemplate({
        "group_sum",
        "Group and Sum",
        "Group documents by a field and compute the sum of a numeric field.",
        "FOR {{var}} IN {{collection}}\n"
        "  COLLECT {{group_var}} = {{var}}.{{group_field}}\n"
        "  AGGREGATE total = SUM({{var}}.{{sum_field}})\n"
        "  SORT total DESC\n"
        "  RETURN { {{group_var}}: {{group_var}}, total: total }",
        {"aggregation", "group", "sum"},
        {"var", "collection", "group_var", "group_field", "sum_field"}
    });

    // ------------------------------------------------------------------
    // 7. Inner join (two FOR loops)
    // ------------------------------------------------------------------
    registerTemplate({
        "inner_join",
        "Inner Join (Two Collections)",
        "Join two collections on a common key and return merged documents.",
        "FOR {{var1}} IN {{collection1}}\n"
        "  FOR {{var2}} IN {{collection2}}\n"
        "    FILTER {{var2}}.{{join_key}} == {{var1}}._key\n"
        "    RETURN MERGE({{var1}}, {{var2}})",
        {"join", "relational"},
        {"var1", "collection1", "var2", "collection2", "join_key"}
    });

    // ------------------------------------------------------------------
    // 8. Graph traversal
    // ------------------------------------------------------------------
    registerTemplate({
        "graph_traversal",
        "Graph Traversal",
        "Traverse a graph starting from a given vertex, following edges.",
        "FOR {{var}}, e, p IN {{depth_min}}..{{depth_max}}\n"
        "  {{direction}} '{{start_node}}' {{edge_coll}}\n"
        "  RETURN {{var}}",
        {"graph", "traversal"},
        {"var", "depth_min", "depth_max", "direction", "start_node", "edge_coll"}
    });

    // ------------------------------------------------------------------
    // 9. Shortest path
    // ------------------------------------------------------------------
    registerTemplate({
        "shortest_path",
        "Shortest Path Between Two Vertices",
        "Find the shortest path between two vertices in a graph.",
        "FOR v IN OUTBOUND\n"
        "  SHORTEST_PATH '{{start_node}}' TO '{{end_node}}'\n"
        "  {{edge_coll}}\n"
        "  RETURN v",
        {"graph", "shortest-path"},
        {"start_node", "end_node", "edge_coll"}
    });

    // ------------------------------------------------------------------
    // 10. Vector similarity search
    // ------------------------------------------------------------------
    registerTemplate({
        "vector_similarity",
        "Vector Similarity Search",
        "Find the top-k most similar documents to a query vector.",
        "FOR {{var}} IN {{collection}}\n"
        "  FILTER SIMILARITY({{var}}.{{vector_field}}, @query_vector, {{top_k}})\n"
        "  RETURN {{var}}",
        {"vector", "similarity", "search"},
        {"var", "collection", "vector_field", "top_k"}
    });

    // ------------------------------------------------------------------
    // 11. Full-text search
    // ------------------------------------------------------------------
    registerTemplate({
        "fulltext_search",
        "Full-Text Search",
        "Return documents whose text field contains all given keywords.",
        "FOR {{var}} IN {{collection}}\n"
        "  FILTER FULLTEXT({{var}}.{{text_field}}, '{{search_terms}}')\n"
        "  RETURN {{var}}",
        {"fulltext", "search", "text"},
        {"var", "collection", "text_field", "search_terms"}
    });

    // ------------------------------------------------------------------
    // 12. Upsert
    // ------------------------------------------------------------------
    registerTemplate({
        "upsert",
        "Upsert (Insert or Update)",
        "Insert a document if it does not exist, or update it if it does.",
        "UPSERT { {{match_key}}: {{match_value}} }\n"
        "  INSERT {{insert_doc}}\n"
        "  UPDATE {{update_doc}}\n"
        "  IN {{collection}}",
        {"write", "upsert", "dml"},
        {"match_key", "match_value", "insert_doc", "update_doc", "collection"}
    });

    // ------------------------------------------------------------------
    // 13. Bulk insert
    // ------------------------------------------------------------------
    registerTemplate({
        "bulk_insert",
        "Bulk Insert from Array",
        "Insert all elements of a bind-parameter array into a collection.",
        "FOR doc IN @documents\n"
        "  INSERT doc INTO {{collection}}",
        {"write", "insert", "bulk", "dml"},
        {"collection"}
    });

    // ------------------------------------------------------------------
    // 14. Delete by filter
    // ------------------------------------------------------------------
    registerTemplate({
        "delete_filtered",
        "Delete Documents by Filter",
        "Remove all documents in a collection that match a condition.",
        "FOR {{var}} IN {{collection}}\n"
        "  FILTER {{condition}}\n"
        "  REMOVE {{var}} IN {{collection}}",
        {"write", "delete", "dml"},
        {"var", "collection", "condition"}
    });

    // ------------------------------------------------------------------
    // 15. Time-range query (timeseries)
    // ------------------------------------------------------------------
    registerTemplate({
        "timeseries_range",
        "Time-Range Query",
        "Return documents whose timestamp falls within a given range.",
        "FOR {{var}} IN {{collection}}\n"
        "  FILTER {{var}}.{{ts_field}} >= {{start_ts}}\n"
        "    AND {{var}}.{{ts_field}} < {{end_ts}}\n"
        "  SORT {{var}}.{{ts_field}} ASC\n"
        "  RETURN {{var}}",
        {"timeseries", "range", "filter"},
        {"var", "collection", "ts_field", "start_ts", "end_ts"}
    });
}

} // namespace aql
} // namespace themis
