/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_query_builder.cpp                              ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-22 08:22:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     387                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "aql/aql_query_builder.h"
#include "aql/aql_query_validator.h"
#include "aql/llm_aql_handler.h"

#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <spdlog/spdlog.h>

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

// ============================================================================
// Pimpl implementation
// ============================================================================

class AQLQueryBuilder::Impl {
public:
    std::vector<ForClause>     for_clauses;
    std::vector<std::string>   let_clauses;   // raw "var = expr" strings
    std::vector<std::string>   filters;
    std::vector<SortClause>    sorts;
    std::vector<CollectClause> collects;
    int                        limit_count  = -1;
    int                        limit_offset = 0;
    std::string                return_expr;

    void reset() {
        for_clauses.clear();
        let_clauses.clear();
        filters.clear();
        sorts.clear();
        collects.clear();
        limit_count  = -1;
        limit_offset = 0;
        return_expr.clear();
    }

    // Renders the partial or complete query
    std::string render(bool require_complete) const {
        if (require_complete) {
            if (for_clauses.empty()) {
                throw std::logic_error("AQLQueryBuilder: query requires at least one FOR clause");
            }
            if (return_expr.empty()) {
                throw std::logic_error("AQLQueryBuilder: query requires a RETURN clause");
            }
        }

        std::ostringstream oss;
        bool first_clause = true;

        auto sep = [&]() -> std::ostringstream& {
            if (!first_clause) oss << "\n  ";
            first_clause = false;
            return oss;
        };

        for (const auto& fc : for_clauses) {
            sep() << "FOR " << fc.variable << " IN " << fc.collection;
        }
        for (const auto& lc : let_clauses) {
            sep() << "LET " << lc;
        }
        for (const auto& f : filters) {
            sep() << "FILTER " << f;
        }
        for (const auto& c : collects) {
            sep() << "COLLECT " << c.variable << " = " << c.expression;
        }
        for (const auto& s : sorts) {
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

        return oss.str();
    }
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

AQLQueryBuilder::AQLQueryBuilder()
    : impl_(std::make_unique<Impl>()) {}

AQLQueryBuilder::~AQLQueryBuilder() = default;

AQLQueryBuilder::AQLQueryBuilder(AQLQueryBuilder&&) noexcept = default;
AQLQueryBuilder& AQLQueryBuilder::operator=(AQLQueryBuilder&&) noexcept = default;

// ============================================================================
// Fluent builder methods
// ============================================================================

AQLQueryBuilder& AQLQueryBuilder::forIn(
    const std::string& variable,
    const std::string& collection
) {
    if (variable.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::forIn: variable must not be empty");
    }
    if (collection.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::forIn: collection must not be empty");
    }
    impl_->for_clauses.push_back({variable, collection});
    return *this;
}

AQLQueryBuilder& AQLQueryBuilder::filter(const std::string& condition) {
    if (condition.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::filter: condition must not be empty");
    }
    impl_->filters.push_back(condition);
    return *this;
}

AQLQueryBuilder& AQLQueryBuilder::sort(const std::string& field, bool ascending) {
    if (field.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::sort: field must not be empty");
    }
    impl_->sorts.push_back({field, ascending});
    return *this;
}

AQLQueryBuilder& AQLQueryBuilder::limit(int count, int offset) {
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

AQLQueryBuilder& AQLQueryBuilder::ret(const std::string& expression) {
    if (expression.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::ret: expression must not be empty");
    }
    impl_->return_expr = expression;
    return *this;
}

AQLQueryBuilder& AQLQueryBuilder::let(
    const std::string& variable,
    const std::string& expression
) {
    if (variable.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::let: variable must not be empty");
    }
    if (expression.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::let: expression must not be empty");
    }
    impl_->let_clauses.push_back(variable + " = " + expression);
    return *this;
}

AQLQueryBuilder& AQLQueryBuilder::collect(
    const std::string& variable,
    const std::string& expression
) {
    if (variable.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::collect: variable must not be empty");
    }
    if (expression.empty()) {
        throw std::invalid_argument("AQLQueryBuilder::collect: expression must not be empty");
    }
    impl_->collects.push_back({variable, expression});
    return *this;
}

AQLQueryBuilder& AQLQueryBuilder::reset() {
    impl_->reset();
    return *this;
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
    return !impl_->for_clauses.empty() && !impl_->return_expr.empty();
}

bool AQLQueryBuilder::isValid() const {
    // A valid (possibly incomplete) query must have no contradictions:
    // - At least one FOR clause if any other clause is present
    bool has_any_clause = !impl_->filters.empty()
        || !impl_->sorts.empty()
        || !impl_->collects.empty()
        || !impl_->let_clauses.empty()
        || impl_->limit_count >= 0
        || !impl_->return_expr.empty();

    if (has_any_clause && impl_->for_clauses.empty()) {
        return false;
    }
    return true;
}

ValidationResult AQLQueryBuilder::validate() const {
    AQLQueryValidator validator;
    return validator.validate(*this);
}

// ============================================================================
// Rule-based suggestions
// ============================================================================

std::vector<std::string> AQLQueryBuilder::getNextSteps() const {
    std::vector<std::string> steps;

    if (impl_->for_clauses.empty()) {
        // Query must start with FOR
        steps.push_back("FOR");
        return steps;
    }

    // After FOR, most clauses are optional and can be added in any order
    // (before RETURN)
    if (impl_->return_expr.empty()) {
        steps.push_back("LET");
        steps.push_back("FILTER");
        steps.push_back("COLLECT");
        steps.push_back("SORT");
        if (impl_->limit_count < 0) {
            steps.push_back("LIMIT");
        }
        steps.push_back("RETURN");
    }
    // If RETURN is already set, the query is complete
    // (though additional FOR clauses for nested loops are still possible)
    steps.push_back("FOR");  // nested FOR is always valid

    return steps;
}

// ============================================================================
// LLM-powered suggestions
// ============================================================================

std::vector<std::string> AQLQueryBuilder::getCompletionSuggestions(
    LLMAQLHandler& handler,
    const std::string& schema_context,
    int max_suggestions
) const {
    std::vector<std::string> suggestions;

    try {
        std::string partial = getPartialQuery();

        std::ostringstream prompt;
        prompt << "You are an AQL (ArangoDB Query Language) expert for ThemisDB.\n";
        if (!schema_context.empty()) {
            prompt << "Available schema:\n" << schema_context << "\n\n";
        }
        prompt << "Current partial AQL query:\n```\n" << partial << "\n```\n\n";
        prompt << "Valid next clauses (based on AQL grammar): ";
        auto steps = getNextSteps();
        for (size_t i = 0; i < steps.size(); ++i) {
            if (i > 0) prompt << ", ";
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
    } catch (const std::exception& e) {
        spdlog::warn("AQLQueryBuilder::getCompletionSuggestions failed: {}", e.what());
    }

    return suggestions;
}

std::string AQLQueryBuilder::getLLMSuggestion(
    LLMAQLHandler& handler,
    const std::string& intent,
    const std::string& schema_context
) const {
    try {
        std::string partial = getPartialQuery();

        std::ostringstream context;
        if (!schema_context.empty()) {
            context << "Schema:\n" << schema_context << "\n\n";
        }
        if (!partial.empty()) {
            context << "Partial query so far:\n```\n" << partial << "\n```\n\n";
        }

        return handler.translateNLToAQL(intent, context.str());
    } catch (const std::exception& e) {
        spdlog::warn("AQLQueryBuilder::getLLMSuggestion failed: {}", e.what());
        return "";
    }
}

} // namespace aql
} // namespace themis
