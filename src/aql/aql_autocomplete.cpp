/**
 * @file aql_autocomplete.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=19, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_autocomplete.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <unordered_set>

namespace themis {
namespace aql {

// ============================================================================
// Static keyword / function tables
// ============================================================================

namespace {

struct KeywordEntry {
    const char *keyword;
    const char *detail;
    const char *insert_text; // nullptr means use keyword as-is
    int sort_order;
};

// Core AQL clause keywords (primary completions at statement level)
const KeywordEntry kClauseKeywords[] = {
    {"FOR", "Iterate over a collection or expression", "FOR ${1:var} IN ${2:collection}", 0},
    {"FILTER", "Filter results by a boolean expression", "FILTER ${1:condition}", 0},
    {"SORT", "Sort results by one or more fields", "SORT ${1:expr} ${2:ASC}", 0},
    {"LIMIT", "Restrict the number of returned results", "LIMIT ${1:count}", 0},
    {"RETURN", "Return a value or expression", "RETURN ${1:expr}", 0},
    {"LET", "Bind a variable to an expression", "LET ${1:var} = ${2:expr}", 0},
    {"COLLECT", "Group results and perform aggregations", "COLLECT ${1:var} = ${2:expr}", 1},
    {"WITH", "Load additional vertex/edge collections for graph traversal", "WITH ${1:collection}", 2},
    {"INSERT", "Insert a document into a collection", "INSERT ${1:doc} INTO ${2:collection}", 3},
    {"UPDATE", "Update an existing document", "UPDATE ${1:doc} IN ${2:collection}", 3},
    {"REPLACE", "Replace an existing document", "REPLACE ${1:doc} IN ${2:collection}", 3},
    {"REMOVE", "Remove a document from a collection", "REMOVE ${1:doc} IN ${2:collection}", 3},
    {"UPSERT", "Insert or update a document",
     "UPSERT ${1:filter} INSERT ${2:insert} UPDATE ${3:update} IN ${4:collection}", 3},
};

// Modifier / expression keywords (valid inside clauses)
const KeywordEntry kModifierKeywords[] = {
    {"IN", "Collection or range operand", nullptr, 0},
    {"NOT", "Logical negation", nullptr, 0},
    {"AND", "Logical conjunction", nullptr, 0},
    {"OR", "Logical disjunction", nullptr, 0},
    {"ASC", "Ascending sort direction", nullptr, 0},
    {"DESC", "Descending sort direction", nullptr, 0},
    {"DISTINCT", "Return distinct values", nullptr, 0},
    {"NULL", "Null literal", nullptr, 0},
    {"TRUE", "Boolean true literal", nullptr, 0},
    {"FALSE", "Boolean false literal", nullptr, 0},
    {"OUTBOUND", "Graph traversal: outgoing edges", nullptr, 1},
    {"INBOUND", "Graph traversal: incoming edges", nullptr, 1},
    {"ANY", "Graph traversal: any direction", nullptr, 1},
    {"SHORTEST_PATH", "Find shortest path in a graph", "SHORTEST_PATH ${1:start} TO ${2:end} OUTBOUND ${3:collection}",
     2},
    {"AGGREGATE", "Aggregate expression in COLLECT clause", "AGGREGATE ${1:var} = ${2:aggr_fn}(${3:expr})", 2},
    {"GRAPH", "Named graph reference", nullptr, 2},
    {"OPTIONS", "Query execution options object", nullptr, 3},
    {"ALL", "Array quantifier: all elements satisfy", nullptr, 3},
    {"NONE", "Array quantifier: no element satisfies", nullptr, 3},
    {"SATISFIES", "Array quantifier body", nullptr, 3},
};

// LLM-extension keywords
const KeywordEntry kLlmKeywords[] = {
    {"LLM", "Start an LLM-assisted statement", nullptr, 0},
    {"INFER", "Run LLM inference on a text field", "LLM INFER ${1:prompt}", 0},
    {"RAG", "Retrieval-augmented generation", "LLM RAG ${1:query} FROM ${2:collection}", 0},
    {"EMBED", "Compute a vector embedding of text", "LLM EMBED ${1:text}", 0},
    {"MODEL", "Specify which LLM model to use", "MODEL \"${1:model_name}\"", 1},
    {"LORA", "Select a LoRA adapter for inference", "LORA \"${1:adapter_name}\"", 1},
};

struct FunctionEntry {
    const char *name;
    const char *signature; // short parameter summary
    const char *detail;
    int sort_order;
};

// Built-in AQL functions
const FunctionEntry kFunctions[] = {
    // Aggregate
    {"COUNT", "COUNT(expr)", "Count elements", 0},
    {"SUM", "SUM(expr)", "Sum of numeric values", 0},
    {"MIN", "MIN(expr)", "Minimum value", 0},
    {"MAX", "MAX(expr)", "Maximum value", 0},
    {"AVG", "AVG(expr)", "Average value", 0},
    // String
    {"CONCAT", "CONCAT(str, ...)", "Concatenate strings", 1},
    {"UPPER", "UPPER(str)", "Convert to uppercase", 1},
    {"LOWER", "LOWER(str)", "Convert to lowercase", 1},
    {"TRIM", "TRIM(str)", "Remove leading/trailing whitespace", 1},
    {"SUBSTRING", "SUBSTRING(str, offset, length)", "Extract substring", 1},
    {"CONTAINS", "CONTAINS(str, search)", "True if str contains search", 1},
    {"STARTS_WITH", "STARTS_WITH(str, prefix)", "True if str starts with prefix", 1},
    {"ENDS_WITH", "ENDS_WITH(str, suffix)", "True if str ends with suffix", 1},
    {"LIKE", "LIKE(str, pattern)", "True if str matches LIKE pattern", 1},
    {"LENGTH", "LENGTH(str_or_array)", "Length of string or array", 1},
    // Math
    {"FLOOR", "FLOOR(n)", "Round down to integer", 2},
    {"CEIL", "CEIL(n)", "Round up to integer", 2},
    {"ROUND", "ROUND(n)", "Round to nearest integer", 2},
    {"ABS", "ABS(n)", "Absolute value", 2},
    {"SQRT", "SQRT(n)", "Square root", 2},
    {"POW", "POW(base, exp)", "Raise base to power exp", 2},
    // Array
    {"PUSH", "PUSH(array, value)", "Append value to array", 3},
    {"POP", "POP(array)", "Remove last element from array", 3},
    {"APPEND", "APPEND(array, values)", "Append multiple values to array", 3},
    {"SLICE", "SLICE(array, start, length)", "Extract subarray", 3},
    {"FLATTEN", "FLATTEN(array, depth)", "Flatten nested arrays", 3},
    {"UNIQUE", "UNIQUE(array)", "Remove duplicate array values", 3},
    {"INTERSECTION", "INTERSECTION(a, b)", "Array intersection", 3},
    {"UNION", "UNION(a, b)", "Array union", 3},
    {"MINUS", "MINUS(a, b)", "Array difference", 3},
    // Type
    {"TO_STRING", "TO_STRING(value)", "Convert to string", 4},
    {"TO_NUMBER", "TO_NUMBER(value)", "Convert to number", 4},
    {"TO_BOOL", "TO_BOOL(value)", "Convert to boolean", 4},
    {"IS_STRING", "IS_STRING(value)", "True if value is a string", 4},
    {"IS_NUMBER", "IS_NUMBER(value)", "True if value is a number", 4},
    {"IS_BOOL", "IS_BOOL(value)", "True if value is a boolean", 4},
    {"IS_NULL", "IS_NULL(value)", "True if value is null", 4},
    {"IS_ARRAY", "IS_ARRAY(value)", "True if value is an array", 4},
    {"IS_OBJECT", "IS_OBJECT(value)", "True if value is an object", 4},
    // JSON
    {"JSON_STRINGIFY", "JSON_STRINGIFY(value)", "Serialize value to JSON string", 5},
    {"JSON_PARSE", "JSON_PARSE(str)", "Parse JSON string to value", 5},
    // Date/Time
    {"DATE_NOW", "DATE_NOW()", "Current timestamp in milliseconds", 6},
    {"DATE_FORMAT", "DATE_FORMAT(date, format)", "Format a date value", 6},
    {"DATE_ADD", "DATE_ADD(date, amount, unit)", "Add duration to a date", 6},
    // Vector / similarity
    {"SIMILARITY", "SIMILARITY(vec1, vec2)", "Cosine similarity between vectors", 7},
    {"THEMIS_EMBED", "THEMIS_EMBED(text)", "Compute text embedding vector", 7},
    // Geospatial
    {"ST_DISTANCE", "ST_DISTANCE(geo1, geo2)", "Distance between two geometries", 8},
    {"ST_WITHIN", "ST_WITHIN(geo, radius)", "True if geometry is within radius", 8},
    {"ST_INTERSECTS", "ST_INTERSECTS(geo1, geo2)", "True if geometries intersect", 8},
    // Fulltext
    {"FULLTEXT", "FULLTEXT(collection, attr, query)", "Fulltext search", 9},
};

} // anonymous namespace

// ============================================================================
// Helpers
// ============================================================================

static std::string aqlAutoCompleteToLower(const std::string &s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

static bool ciStartsWith(const std::string &s, const std::string &prefix) {
    if (prefix.empty()) {
        return true;
    }
    if (s.size() < prefix.size()) {
        return false;
    }
    return aqlAutoCompleteToLower(s).substr(0, prefix.size()) == aqlAutoCompleteToLower(prefix);
}

// Returns true when c is a valid AQL identifier character
static bool isIdentChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// ============================================================================
// AQLAutoComplete::extractPrefix
// ============================================================================

std::string AQLAutoComplete::extractPrefix(const std::string &text, std::size_t cursor) const {
    if (text.empty() || cursor == 0) {
        return "";
    }
    std::size_t end   = std::min(cursor, text.size());
    std::size_t start = end;
    while (start > 0 && isIdentChar(text[start - 1])) {
        --start;
    }
    return text.substr(start, end - start);
}

// ============================================================================
// AQLAutoComplete::prefixStart
// ============================================================================

std::size_t AQLAutoComplete::prefixStart(const std::string &text, std::size_t cursor) const {
    if (text.empty() || cursor == 0) {
        return 0;
    }
    std::size_t end   = std::min(cursor, text.size());
    std::size_t start = end;
    while (start > 0 && isIdentChar(text[start - 1])) {
        --start;
    }
    return start;
}

// ============================================================================
// AQLAutoComplete::isAfterDot
// ============================================================================

bool AQLAutoComplete::isAfterDot(const std::string &text, std::size_t cursor) const {
    if (text.empty() || cursor == 0) {
        return false;
    }
    std::size_t effective = std::min(cursor, text.size());
    // Skip backwards over current identifier chars
    std::size_t pos = effective;
    while (pos > 0 && isIdentChar(text[pos - 1])) {
        --pos;
    }
    return (pos > 0 && text[pos - 1] == '.');
}

// ============================================================================
// AQLAutoComplete::variableBeforeDot
// ============================================================================

std::string AQLAutoComplete::variableBeforeDot(const std::string &text, std::size_t cursor) const {
    if (text.empty() || cursor == 0) {
        return "";
    }
    std::size_t effective = std::min(cursor, text.size());
    // Skip current identifier (attribute prefix)
    std::size_t pos = effective;
    while (pos > 0 && isIdentChar(text[pos - 1])) {
        --pos;
    }
    // Expect a dot
    if (pos == 0 || text[pos - 1] != '.') {
        return "";
    }
    --pos; // skip dot
    // Skip variable name
    std::size_t var_end = pos;
    while (pos > 0 && isIdentChar(text[pos - 1])) {
        --pos;
    }
    return text.substr(pos, var_end - pos);
}

// ============================================================================
// AQLAutoComplete::declaredVariables
// ============================================================================

std::vector<std::string> AQLAutoComplete::declaredVariables(const std::string &text, std::size_t cursor) const {
    std::vector<std::string> vars = {};

    std::string prefix_text = text.substr(0, std::min(cursor, text.size()));

    // Static patterns compiled once for performance
    // FOR <var> IN ...
    static const std::regex for_re(R"(FOR\s+([A-Za-z_][A-Za-z0-9_]*)\s+IN)", std::regex::icase);
    // LET <var> = ...
    static const std::regex let_re(R"(LET\s+([A-Za-z_][A-Za-z0-9_]*)\s*=)", std::regex::icase);
    // COLLECT <var> = <expr> — captures the grouping variable after COLLECT
    static const std::regex collect_re(R"(COLLECT\s+([A-Za-z_][A-Za-z0-9_]*)\s*=)", std::regex::icase);
    // COLLECT ... INTO <group_var> — [\s\S] matches newlines too
    static const std::regex collect_into_re(R"(COLLECT\b[\s\S]*?\bINTO\s+([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase);
    // COLLECT WITH COUNT INTO <count_var>
    static const std::regex collect_count_re(R"(COLLECT\s+WITH\s+COUNT\s+INTO\s+([A-Za-z_][A-Za-z0-9_]*))",
                                             std::regex::icase);

    auto collect_matches = [&]([[maybe_unused]] const std::regex &re) {
        std::sregex_iterator it(prefix_text.begin(), prefix_text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            vars.push_back((*it)[1].str());
        }
    };

    collect_matches(for_re);
    collect_matches(let_re);
    collect_matches(collect_re);
    collect_matches(collect_into_re);
    collect_matches(collect_count_re);

    // De-duplicate while preserving order
    std::unordered_set<std::string> seen;
    std::vector<std::string> unique_vars = {};

    for (auto &v : vars) {
        if (seen.insert(v).second) {
            unique_vars.push_back(v);
        }
    }
    return unique_vars;
}

// ============================================================================
// AQLAutoComplete::parseSchema
// ============================================================================

std::vector<AQLAutoComplete::SchemaInfo> AQLAutoComplete::parseSchema(const std::string &schema_context) const {
    std::vector<SchemaInfo> result = {};

    if (schema_context.empty()) {
        return result;
    }

    // Attempt to parse: "collection: col1(f1, f2), col2(f3, f4)"
    // or simpler: "col1(f1, f2) col2(f3)"
    // or just: "col1, col2"

    // First try the "collection:" prefix format
    std::string text = schema_context;
    {
        // Remove leading "collection:" if present
        std::string lower_text = aqlAutoCompleteToLower(text);
        auto pos               = lower_text.find("collection:");
        if (pos != std::string::npos) {
            text = text.substr(pos + 11); // skip "collection:"
        }
    }

    // Parse "colname(field1, field2, ...)" entries
    std::regex col_re(R"(([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\))", std::regex::icase);
    std::sregex_iterator it(text.begin(), text.end(), col_re);
    std::sregex_iterator end;
    std::unordered_set<std::string> seen = {};

    for (; it != end; ++it) {
        SchemaInfo info;
        info.collection_name = (*it)[1].str();
        if (!seen.insert(aqlAutoCompleteToLower(info.collection_name)).second) {
            continue;
        }

        std::string fields_str = (*it)[2].str();
        std::regex field_re(R"([A-Za-z_][A-Za-z0-9_]*)");
        std::sregex_iterator fit(fields_str.begin(), fields_str.end(), field_re);
        std::sregex_iterator fend;
        for (; fit != fend; ++fit) {
            info.fields.push_back((*fit)[0].str());
        }
        result.push_back(std::move(info));
    }

    // If no "()" patterns found, fall back to comma-separated collection names
    if (result.empty()) {
        std::regex plain_re(R"([A-Za-z_][A-Za-z0-9_]*)");
        std::sregex_iterator pit(text.begin(), text.end(), plain_re);
        std::sregex_iterator pend;
        std::unordered_set<std::string> plain_seen = {};

        for (; pit != pend; ++pit) {
            std::string name = (*pit)[0].str();
            if (plain_seen.insert(aqlAutoCompleteToLower(name)).second) {
                SchemaInfo info;
                info.collection_name = name;
                result.push_back(std::move(info));
            }
        }
    }
    return result;
}

// ============================================================================
// AQLAutoComplete::keywordCandidates
// ============================================================================

std::vector<CompletionItem> AQLAutoComplete::keywordCandidates([[maybe_unused]] const std::string &text,
                                                               [[maybe_unused]] std::size_t cursor) const {
    std::vector<CompletionItem> items;

    // Always offer clause-level keywords
    for (const auto &e : kClauseKeywords) {
        CompletionItem item;
        item.label       = e.keyword;
        item.kind        = CompletionItemKind::Keyword;
        item.detail      = e.detail;
        item.insert_text = (e.insert_text ? e.insert_text : e.keyword);
        item.sort_order  = e.sort_order;
        items.push_back(std::move(item));
    }

    // Always offer modifier keywords
    for (const auto &e : kModifierKeywords) {
        CompletionItem item;
        item.label       = e.keyword;
        item.kind        = CompletionItemKind::Keyword;
        item.detail      = e.detail;
        item.insert_text = (e.insert_text ? e.insert_text : e.keyword);
        item.sort_order  = e.sort_order + 10; // lower priority than clauses
        items.push_back(std::move(item));
    }

    // LLM keywords — slightly lower priority
    for (const auto &e : kLlmKeywords) {
        CompletionItem item;
        item.label         = e.keyword;
        item.kind          = CompletionItemKind::Keyword;
        item.detail        = e.detail;
        item.documentation = "LLM-extension keyword";
        item.insert_text   = (e.insert_text ? e.insert_text : e.keyword);
        item.sort_order    = e.sort_order + 20;
        items.push_back(std::move(item));
    }

    // reserved: will be used for context-sensitive filtering in future
    // reserved: will be used for context-sensitive filtering in future
    return items;
}

// ============================================================================
// AQLAutoComplete::functionCandidates
// ============================================================================

std::vector<CompletionItem> AQLAutoComplete::functionCandidates() const {
    std::vector<CompletionItem> items = {};

    for (const auto &e : kFunctions) {
        CompletionItem item;
        item.label         = e.name;
        item.kind          = CompletionItemKind::Function;
        item.detail        = e.signature;
        item.documentation = e.detail;
        item.insert_text   = std::string(e.name) + "(${1})";
        item.sort_order    = e.sort_order + 30; // below keywords by default
        items.push_back(std::move(item));
    }
    return items;
}

// ============================================================================
// AQLAutoComplete::variableCandidates
// ============================================================================

std::vector<CompletionItem> AQLAutoComplete::variableCandidates(const std::string &text, std::size_t cursor) const {
    std::vector<CompletionItem> items;
    auto vars = declaredVariables(text, cursor);
    int order = 0;
    for (const auto &v : vars) {
        CompletionItem item;
        item.label       = v;
        item.kind        = CompletionItemKind::Variable;
        item.detail      = "Local variable";
        item.insert_text = v;
        item.sort_order  = order++;
        items.push_back(std::move(item));
    }
    return items;
}

// ============================================================================
// AQLAutoComplete::attributeCandidates
// ============================================================================

std::vector<CompletionItem> AQLAutoComplete::attributeCandidates(const std::string &variable,
                                                                 const std::vector<SchemaInfo> &schema,
                                                                 const std::string &text, std::size_t cursor) const {
    std::vector<CompletionItem> items;

    // Try to find which collection was bound to 'variable' via FOR <variable> IN <collection>
    std::string collection_name;
    if (!variable.empty() && !schema.empty()) {
        std::string prefix_text = text.substr(0, std::min(cursor, text.size()));
        try {
            std::regex for_re("FOR\\s+" + variable + "\\s+IN\\s+([A-Za-z_][A-Za-z0-9_]*)", std::regex::icase);
            std::smatch m;
            if (std::regex_search(prefix_text, m, for_re)) {
                collection_name = aqlAutoCompleteToLower(m[1].str());
            }
        } catch (const std::regex_error &) {
            // If regex construction fails (malformed variable), fall through
            // to the union-of-all-fields fallback below.
        }
    }

    // If we identified the collection, offer its fields
    if (!collection_name.empty()) {
        for (const auto &info : schema) {
            if (aqlAutoCompleteToLower(info.collection_name) == collection_name) {
                int order = 0;
                for (const auto &field : info.fields) {
                    CompletionItem item;
                    item.label       = field;
                    item.kind        = CompletionItemKind::Field;
                    item.detail      = info.collection_name + " field";
                    item.insert_text = field;
                    item.sort_order  = order++;
                    items.push_back(std::move(item));
                }
                break;
            }
        }
    } else if (!schema.empty()) {
        // Variable-collection binding not found — return all known fields
        // from all collections (union, de-duplicated)
        std::unordered_set<std::string> seen = {};

        for (const auto &info : schema) {
            for (const auto &field : info.fields) {
                if (seen.insert(aqlAutoCompleteToLower(field)).second) {
                    CompletionItem item;
                    item.label       = field;
                    item.kind        = CompletionItemKind::Field;
                    item.detail      = "Field (collection: " + info.collection_name + ")";
                    item.insert_text = field;
                    item.sort_order  = 0;
                    items.push_back(std::move(item));
                }
            }
        }
    }

    return items;
}

// ============================================================================
// AQLAutoComplete::filterAndSort
// ============================================================================

std::vector<CompletionItem> AQLAutoComplete::filterAndSort(std::vector<CompletionItem> candidates,
                                                           const std::string &prefix,
                                                           std::size_t prefix_start_col) const {
    // Filter: keep items whose label starts with prefix (case-insensitive)
    std::vector<CompletionItem> filtered = {};

    filtered.reserve(candidates.size());
    for (auto &item : candidates) {
        if (ciStartsWith(item.label, prefix)) {
            item.prefix_start = prefix_start_col;
            filtered.push_back(std::move(item));
        }
    }

    // Sort: primary by sort_order, secondary alphabetically on label
    std::stable_sort(filtered.begin(), filtered.end(), [](const CompletionItem &a, const CompletionItem &b) {
        if (a.sort_order != b.sort_order) {
            return a.sort_order < b.sort_order;
        }
        return aqlAutoCompleteToLower(a.label) < aqlAutoCompleteToLower(b.label);
    });

    return filtered;
}

// ============================================================================
// AQLAutoComplete::complete  (main entry point)
// ============================================================================

std::vector<CompletionItem> AQLAutoComplete::complete(const CompletionContext &ctx) const {
    const std::string &text = ctx.query_text;
    std::size_t cursor
        = (ctx.cursor_offset == std::string::npos) ? text.size() : std::min(ctx.cursor_offset, text.size());

    std::string prefix     = extractPrefix(text, cursor);
    std::size_t ps         = prefixStart(text, cursor);
    bool after_dot         = isAfterDot(text, cursor);
    std::string var_before = after_dot ? variableBeforeDot(text, cursor) : "";
    auto schema            = parseSchema(ctx.schema_context);

    std::vector<CompletionItem> candidates;

    if (after_dot) {
        // Attribute completion after variable.
        auto attrs = attributeCandidates(var_before, schema, text, cursor);
        candidates.insert(candidates.end(), attrs.begin(), attrs.end());
        // Also offer variable names (chaining like v.sub.attr)
        auto vars = variableCandidates(text, cursor);
        candidates.insert(candidates.end(), vars.begin(), vars.end());
    } else {
        // General completion: keywords + functions + variables
        auto kws  = keywordCandidates(text, cursor);
        auto fns  = functionCandidates();
        auto vars = variableCandidates(text, cursor);

        // Variables first (lower sort_order), then keywords, then functions
        candidates.insert(candidates.end(), vars.begin(), vars.end());
        candidates.insert(candidates.end(), kws.begin(), kws.end());
        candidates.insert(candidates.end(), fns.begin(), fns.end());
    }

    return filterAndSort(std::move(candidates), prefix, ps);
}

// ============================================================================
// AQLAutoComplete::allKeywords / allFunctions
// ============================================================================

std::vector<std::string> AQLAutoComplete::allKeywords() const {
    std::vector<std::string> result = {};

    for (const auto &e : kClauseKeywords) {
        result.push_back(e.keyword);
    }
    for (const auto &e : kModifierKeywords) {
        result.push_back(e.keyword);
    }
    for (const auto &e : kLlmKeywords) {
        result.push_back(e.keyword);
    }
    return result;
}

std::vector<std::string> AQLAutoComplete::allFunctions() const {
    std::vector<std::string> result = {};

    for (const auto &e : kFunctions) {
        result.push_back(e.name);
    }
    return result;
}

} // namespace aql
} // namespace themis
