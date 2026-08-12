/**
 * @file aql_query_builder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=16, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_query_builder.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

#include "aql/aql_query_validator.h"
#include "aql/aql_schema_provider.h"
#include "aql/llm_aql_handler.h"

namespace themis {
namespace aql {

// ============================================================================
// Internal state
// ============================================================================

struct ForClause {
    std::string variable;
    std::string collection;
};

struct SortClause {
    std::string field;
    bool ascending;
};

struct CollectClause {
    std::string variable;
    std::string expression;
};

struct ForTraverseClause {
    std::string vertex_var;
    std::string edge_var;
    std::string path_var;
    std::string start;
    std::string graph;
    std::string direction;
    int min_depth;
    int max_depth;
};

enum class DMLType { INSERT, UPDATE, REMOVE, UPSERT, REPLACE };

struct DMLClause {
    DMLType type;
    std::string collection;
    std::string doc_expr;    // INSERT / UPDATE / REMOVE / REPLACE document expression
    std::string filter_expr; // UPSERT search expression
    std::string insert_expr; // UPSERT insert expression
    std::string update_expr; // UPSERT update expression
};

struct WindowClause {
    std::string partition_expr; // empty for row-based windows
    std::string window_spec;
};

// ============================================================================
// Pimpl implementation
// ============================================================================

class AQLQueryBuilder::Impl {
  public:
    std::vector<ForClause> for_clauses;
    std::vector<ForTraverseClause> for_traverse_clauses;
    std::vector<std::string> let_clauses; // raw "var = expr" strings
    std::vector<std::string> filters;
    std::vector<WindowClause> window_clauses;
    std::vector<SortClause> sorts;
    std::vector<CollectClause> collects;
    int limit_count  = -1;
    int limit_offset = 0;
    std::string return_expr;
    std::vector<DMLClause> dml_clauses;

    // Schema snapshot attached via setSchema()
    std::vector<CollectionMetadata> schema;

    // Opt-in ingestion enrichment flag for DML clauses
    bool ingestion_enrichment = false;

    void reset() {
        for_clauses.clear();
        for_traverse_clauses.clear();
        let_clauses.clear();
        filters.clear();
        window_clauses.clear();
        sorts.clear();
        collects.clear();
        limit_count  = -1;
        limit_offset = 0;
        return_expr.clear();
        dml_clauses.clear();
        ingestion_enrichment = false;
    }

    // Renders the partial or complete query
    std::string render(bool require_complete) const {
        bool has_for           = !for_clauses.empty() || !for_traverse_clauses.empty();
        bool has_return_or_dml = !return_expr.empty() || !dml_clauses.empty();

        if (require_complete) {
            // DML-only queries (e.g., INSERT without FOR) are complete by themselves
            if (!has_for && dml_clauses.empty()) {
                throw std::logic_error("AQLQueryBuilder: query requires at least one FOR clause");
            }
            if (!has_return_or_dml) {
                throw std::logic_error("AQLQueryBuilder: query requires a RETURN clause or a DML clause");
            }
        }

        std::ostringstream oss;
        bool first_clause = true;

        auto sep = [&]() -> std::ostringstream & {
            if (!first_clause) {
                oss << "\n  ";
            }
            first_clause = false;
            return oss;
        };

        for (const auto &fc : for_clauses) {
            sep() << "FOR " << fc.variable << " IN " << fc.collection;
        }
        for (const auto &ft : for_traverse_clauses) {
            sep() << "FOR " << ft.vertex_var << ", " << ft.edge_var << ", " << ft.path_var << " IN " << ft.min_depth
                  << ".." << ft.max_depth << " " << ft.direction << " " << ft.start << " GRAPH " << ft.graph;
        }
        for (const auto &lc : let_clauses) {
            sep() << "LET " << lc;
        }
        for (const auto &f : filters) {
            sep() << "FILTER " << f;
        }
        for (const auto &wc : window_clauses) {
            sep();
            if (!wc.partition_expr.empty()) {
                oss << "WINDOW " << wc.partition_expr << " WITH " << wc.window_spec;
            } else {
                oss << "WINDOW " << wc.window_spec;
            }
        }
        for (const auto &c : collects) {
            sep() << "COLLECT " << c.variable << " = " << c.expression;
        }
        for (const auto &s : sorts) {
            sep() << "SORT " << s.field << (s.ascending ? " ASC" : " DESC");
        }
        if (limit_count >= 0) {
            sep();
            if (limit_offset > 0) {
                oss << "LIMIT " << limit_offset << ", " << limit_count;
            } else {
                oss << "LIMIT " << limit_count;
            }
        }
        if (!return_expr.empty()) {
            sep() << "RETURN " << return_expr;
        }
        for (const auto &dml : dml_clauses) {
            switch (dml.type) {
                case DMLType::INSERT:
                    sep() << "INSERT " << dml.doc_expr << " INTO " << dml.collection;
                    break;
                case DMLType::UPDATE:
                    sep() << "UPDATE " << dml.doc_expr << " IN " << dml.collection;
                    break;
                case DMLType::REMOVE:
                    sep() << "REMOVE " << dml.doc_expr << " IN " << dml.collection;
                    break;
                case DMLType::REPLACE:
                    sep() << "REPLACE " << dml.doc_expr << " IN " << dml.collection;
                    break;
                case DMLType::UPSERT:
                    sep() << "UPSERT " << dml.filter_expr << " INSERT " << dml.insert_expr << " UPDATE "
                          << dml.update_expr << " IN " << dml.collection;
                    break;
            }
        }

        return oss.str();
    }
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

AQLQueryBuilder::AQLQueryBuilder() : impl_(std::make_unique<Impl>()) {}

AQLQueryBuilder::~AQLQueryBuilder() = default;

AQLQueryBuilder::AQLQueryBuilder(AQLQueryBuilder &&) noexcept            = default;
AQLQueryBuilder &AQLQueryBuilder::operator=(AQLQueryBuilder &&) noexcept = default;

// ============================================================================
// Fluent builder methods
// ============================================================================

AQLQueryBuilder &AQLQueryBuilder::forIn(const std::string &variable, const std::string &collection) {
    if (variable.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::forIn: variable must not be empty");
    }
    if (collection.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::forIn: collection must not be empty");
    }
    impl_->for_clauses.push_back({variable, collection});
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::filter(const std::string &condition) {
    if (condition.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::filter: condition must not be empty");
    }
    impl_->filters.push_back(condition);
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::sort(const std::string &field, bool ascending) {
    if (field.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::sort: field must not be empty");
    }
    impl_->sorts.push_back({field, ascending});
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::limit(int count, int offset) {
    if (count < 0) {
        throw std::invalid_argument("AQLQueryBuilder::limit: count must be non-negative");
    }
    if (offset < 0) {
        throw std::invalid_argument("AQLQueryBuilder::limit: offset must be non-negative");
    }
    impl_->limit_count  = count;
    impl_->limit_offset = offset;
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::ret(const std::string &expression) {
    if (expression.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::ret: expression must not be empty");
    }
    impl_->return_expr = expression;
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::let(const std::string &variable, const std::string &expression) {
    if (variable.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::let: variable must not be empty");
    }
    if (expression.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::let: expression must not be empty");
    }
    impl_->let_clauses.push_back(variable + " = " + expression);
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::collect(const std::string &variable, const std::string &expression) {
    if (variable.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::collect: variable must not be empty");
    }
    if (expression.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::collect: expression must not be empty");
    }
    impl_->collects.push_back({variable, expression});
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::reset() {
    impl_->reset();
    return *this;
}

// ============================================================================
// Graph traversal
// ============================================================================

AQLQueryBuilder &AQLQueryBuilder::forTraverse(const std::string &vertex_var, const std::string &edge_var,
                                              const std::string &path_var, const std::string &start,
                                              const std::string &graph, const std::string &direction, int min_depth,
                                              int max_depth) {
    if (vertex_var.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::forTraverse: vertex_var must not be empty");
    }
    if (edge_var.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::forTraverse: edge_var must not be empty");
    }
    if (path_var.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::forTraverse: path_var must not be empty");
    }
    if (start.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::forTraverse: start must not be empty");
    }
    if (graph.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::forTraverse: graph must not be empty");
    }
    if (min_depth < 0) {
        throw std::invalid_argument("AQLQueryBuilder::forTraverse: min_depth must be non-negative");
    }
    if (max_depth < 0) {
        throw std::invalid_argument("AQLQueryBuilder::forTraverse: max_depth must be non-negative");
    }
    if (min_depth > max_depth) {
        throw std::invalid_argument("AQLQueryBuilder::forTraverse: min_depth (" + std::to_string(min_depth)
                                    + ") must not exceed max_depth (" + std::to_string(max_depth) + ")");
    }
    impl_->for_traverse_clauses.push_back(
        {vertex_var, edge_var, path_var, start, graph, direction, min_depth, max_depth});
    return *this;
}

// ============================================================================
// DML methods
// ============================================================================

AQLQueryBuilder &AQLQueryBuilder::insertInto(const std::string &collection, const std::string &doc_expr) {
    if (collection.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::insertInto: collection must not be empty");
    }
    if (doc_expr.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::insertInto: doc_expr must not be empty");
    }
    impl_->dml_clauses.push_back({DMLType::INSERT, collection, doc_expr, {}, {}, {}});
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::updateIn(const std::string &collection, const std::string &doc_expr) {
    if (collection.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::updateIn: collection must not be empty");
    }
    if (doc_expr.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::updateIn: doc_expr must not be empty");
    }
    impl_->dml_clauses.push_back({DMLType::UPDATE, collection, doc_expr, {}, {}, {}});
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::removeIn(const std::string &collection, const std::string &doc_expr) {
    if (collection.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::removeIn: collection must not be empty");
    }
    if (doc_expr.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::removeIn: doc_expr must not be empty");
    }
    impl_->dml_clauses.push_back({DMLType::REMOVE, collection, doc_expr, {}, {}, {}});
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::upsertIn(const std::string &collection, const std::string &filter_expr,
                                           const std::string &insert_expr, const std::string &update_expr) {
    if (collection.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::upsertIn: collection must not be empty");
    }
    if (filter_expr.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::upsertIn: filter_expr must not be empty");
    }
    if (insert_expr.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::upsertIn: insert_expr must not be empty");
    }
    if (update_expr.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::upsertIn: update_expr must not be empty");
    }
    impl_->dml_clauses.push_back({DMLType::UPSERT, collection, {}, filter_expr, insert_expr, update_expr});
    return *this;
}

AQLQueryBuilder &AQLQueryBuilder::replaceIn(const std::string &collection, const std::string &doc_expr) {
    if (collection.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::replaceIn: collection must not be empty");
    }
    if (doc_expr.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::replaceIn: doc_expr must not be empty");
    }
    impl_->dml_clauses.push_back({DMLType::REPLACE, collection, doc_expr, {}, {}, {}});
    return *this;
}

// ============================================================================
// WINDOW analytics
// ============================================================================

AQLQueryBuilder &AQLQueryBuilder::window(const std::string &partition_expr, const std::string &window_spec) {
    if (window_spec.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::window: window_spec must not be empty");
    }
    impl_->window_clauses.push_back({partition_expr, window_spec});
    return *this;
}

// ============================================================================
// Subquery support
// ============================================================================

AQLQueryBuilder &AQLQueryBuilder::subquery(const std::string &variable, const AQLQueryBuilder &inner) {
    if (variable.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::subquery: variable must not be empty");
    }
    std::string inner_query = inner.getPartialQuery();
    if (inner_query.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::subquery: inner builder has no clauses");
    }
    // Render as: variable = ( <inner_query> )
    impl_->let_clauses.push_back(variable + " = ( " + inner_query + " )");
    return *this;
}

// ============================================================================
// Schema-aware query generation
// ============================================================================

AQLQueryBuilder &AQLQueryBuilder::setSchema(const std::vector<CollectionMetadata> &schema) {
    impl_->schema = schema;
    return *this;
}

std::string AQLQueryBuilder::getSchemaContext() const {
    return formatSchemaContext(impl_->schema);
}

std::vector<std::string> AQLQueryBuilder::getFieldsForCollection(const std::string &collection) const {
    for (const auto &col : impl_->schema) {
        if (col.name == collection) {
            std::vector<std::string> fields;
            fields.reserve(col.fields.size());
            for (const auto &f : col.fields) {
                fields.push_back(f.name);
            }
            return fields;
        }
    }
    return {};
}

// ============================================================================
// Query output
// ============================================================================

std::string AQLQueryBuilder::build() const {
    return impl_->render(/*require_complete=*/true);
}

std::string AQLQueryBuilder::getPartialQuery() const {
    return impl_->render(/*require_complete=*/false);
}

// ============================================================================
// Validation and state
// ============================================================================

bool AQLQueryBuilder::isComplete() const {
    bool has_for           = !impl_->for_clauses.empty() || !impl_->for_traverse_clauses.empty();
    bool has_return_or_dml = !impl_->return_expr.empty() || !impl_->dml_clauses.empty();
    // DML-only queries (e.g., INSERT without a FOR loop) are also considered complete
    if (!impl_->dml_clauses.empty() && impl_->for_clauses.empty() && impl_->for_traverse_clauses.empty()) {
        return true;
    }
    return has_for && has_return_or_dml;
}

bool AQLQueryBuilder::isValid() const {
    // A valid (possibly incomplete) query must have no contradictions:
    // - At least one FOR clause if any other clause (except DML) is present
    bool has_for        = !impl_->for_clauses.empty() || !impl_->for_traverse_clauses.empty();
    bool has_any_clause = !impl_->filters.empty() || !impl_->sorts.empty() || !impl_->collects.empty()
                          || !impl_->let_clauses.empty() || !impl_->window_clauses.empty() || impl_->limit_count >= 0
                          || !impl_->return_expr.empty();

    if (has_any_clause && !has_for) {
        return false;
    }

    // Traversal depth must be logically consistent
    for (const auto &ft : impl_->for_traverse_clauses) {
        if (ft.min_depth > ft.max_depth) {
            return false;
        }
    }

    return true;
}

ValidationResult AQLQueryBuilder::validate() const {
    AQLQueryValidator validator;
    ValidationResult result = validator.validate(*this);

    // Check traversal depth constraints
    for (const auto &ft : impl_->for_traverse_clauses) {
        if (ft.min_depth > ft.max_depth) {
            result.is_valid = false;
            result.issues.push_back({ValidationIssue::Severity::ERROR,
                                     "Graph traversal min_depth (" + std::to_string(ft.min_depth)
                                         + ") is greater than max_depth (" + std::to_string(ft.max_depth) + ")",
                                     "FOR"});
        }
    }

    // Schema-aware: warn when a collection named in a FOR clause is not found
    // in the attached metadata snapshot.
    if (!impl_->schema.empty()) {
        for (const auto &fc : impl_->for_clauses) {
            bool found = false;
            for (const auto &col : impl_->schema) {
                if (col.name == fc.collection) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                result.issues.push_back({ValidationIssue::Severity::WARNING,
                                         "Collection '" + fc.collection + "' not found in attached schema", "FOR"});
            }
        }
    }

    return result;
}

ValidationResult AQLQueryBuilder::validate(const std::vector<CollectionMetadata> &schema) const {
    // Delegate to AQLQueryValidator which has full schema-aware logic for both
    // structural checks and unknown-collection / unknown-field detection.
    AQLQueryValidator validator;
    return validator.validate(*this, schema);
}

// ============================================================================
// Rule-based suggestions
// ============================================================================

std::vector<std::string> AQLQueryBuilder::getNextSteps() const {
    std::vector<std::string> steps;

    bool has_for = !impl_->for_clauses.empty() || !impl_->for_traverse_clauses.empty();

    if (!has_for && impl_->dml_clauses.empty()) {
        // Query must start with FOR or a standalone DML statement
        steps.push_back("FOR");
        steps.push_back("INSERT");
        steps.push_back("UPSERT");
        steps.push_back("UPDATE");
        steps.push_back("REMOVE");
        steps.push_back("REPLACE");
        return steps;
    }

    // After FOR, most clauses are optional and can be added in any order
    // (before RETURN or a DML terminator)
    if (has_for && impl_->return_expr.empty() && impl_->dml_clauses.empty()) {
        steps.push_back("LET");
        steps.push_back("FILTER");
        steps.push_back("WINDOW");
        steps.push_back("COLLECT");
        steps.push_back("SORT");
        if (impl_->limit_count < 0) {
            steps.push_back("LIMIT");
        }
        steps.push_back("RETURN");
        // DML as FOR-loop terminator
        steps.push_back("INSERT");
        steps.push_back("UPDATE");
        steps.push_back("REMOVE");
        steps.push_back("REPLACE");
        steps.push_back("UPSERT");
    }
    // Nested FOR is always valid
    steps.push_back("FOR");

    return steps;
}

// ============================================================================
// LLM-powered suggestions
// ============================================================================

std::vector<std::string> AQLQueryBuilder::getCompletionSuggestions(LLMAQLHandler &handler,
                                                                   const std::string &schema_context,
                                                                   int max_suggestions) const {
    std::vector<std::string> suggestions;

    try {
        std::string partial = getPartialQuery();

        // Use the caller-supplied context, or fall back to the attached schema.
        const std::string &effective_schema = !schema_context.empty() ? schema_context : getSchemaContext();

        std::ostringstream prompt;
        prompt << "You are an AQL (ArangoDB Query Language) expert for ThemisDB.\n";
        if (!effective_schema.empty()) {
            prompt << "Available schema:\n" << effective_schema << "\n\n";
        }
        prompt << "Current partial AQL query:\n```\n" << partial << "\n```\n\n";
        prompt << "Valid next clauses (based on AQL grammar): ";
        auto steps = getNextSteps();
        for (size_t i = 0; i < steps.size(); ++i) {
            if (i > 0) {
                prompt << ", ";
            }
            prompt << steps[i];
        }
        prompt << "\n\n";
        prompt << "Generate exactly " << max_suggestions
               << " short AQL clause snippets that would naturally extend this query. "
               << "Return each suggestion on a separate line. "
               << "Return ONLY the AQL snippets, no explanations.";

        auto response = handler.executeInfer(prompt.str());

        // Split response by newlines into individual suggestions
        std::istringstream ss(response);
        std::string line;
        while (std::getline(ss, line) && (int)suggestions.size() < max_suggestions) {
            // Trim leading/trailing whitespace
            auto start = line.find_first_not_of(" \t\r\n");
            auto end   = line.find_last_not_of(" \t\r\n");
            if (start != std::string::npos) {
                line = line.substr(start, end - start + 1);
            }
            // Skip empty lines and markdown fences
            if (!line.empty() && line.find("```") == std::string::npos) {
                suggestions.push_back(line);
            }
        }
    } catch (const std::exception &e) {
        spdlog::warn("AQLQueryBuilder::getCompletionSuggestions failed: {}", e.what());
    }

    return suggestions;
}

std::string AQLQueryBuilder::getLLMSuggestion(LLMAQLHandler &handler, const std::string &intent,
                                              const std::string &schema_context) const {
    try {
        std::string partial = getPartialQuery();

        // Use the caller-supplied context, or fall back to the attached schema.
        const std::string &effective_schema = !schema_context.empty() ? schema_context : getSchemaContext();

        std::ostringstream context;
        if (!effective_schema.empty()) {
            context << "Schema:\n" << effective_schema << "\n\n";
        }
        if (!partial.empty()) {
            context << "Partial query so far:\n```\n" << partial << "\n```\n\n";
        }

        return handler.translateNLToAQL(intent, context.str());
    } catch (const std::exception &e) {
        spdlog::warn("AQLQueryBuilder::getLLMSuggestion failed: {}", e.what());
        return "";
    }
}

// ============================================================================
// Ingestion enrichment flag
// ============================================================================

AQLQueryBuilder &AQLQueryBuilder::withIngestionEnrichment(bool enabled) {
    impl_->ingestion_enrichment = enabled;
    return *this;
}

bool AQLQueryBuilder::hasIngestionEnrichment() const {
    return impl_->ingestion_enrichment;
}

} // namespace aql
} // namespace themis
