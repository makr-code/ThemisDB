/**
 * @file gremlin_parser.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=25, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Gremlin MATCH traversal parser and AQL transpiler.
//
// Architecture:
//   GremlinParser::parse()
//     └─ Lexer::tokenize()   – produces Token stream from "g.V().step()…"
//     └─ Parser              – iterative step parser
//          ├─ parseStart()   – g.V() / g.V(id) / g.E()
//          └─ parseStep()    – hasLabel/has/hasNot/out/in/both/values/
//                              valueMap/id/label/count/dedup/limit/range/
//                              as/select/order/by
//
//   GremlinToAQLTranspiler::transpile()
//     – Emits AQL FOR/FILTER/SORT/LIMIT/RETURN from the step list

#include "query/gremlin_parser.h"
#include "utils/error_registry.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include "utils/logger.h"

namespace themis {
namespace query {

// ============================================================================
// Token
// ============================================================================

enum class GremlinTokenType {
    IDENT,         // g, V, E, hasLabel, out, …
    DOT,           // .
    LPAREN,        // (
    RPAREN,        // )
    COMMA,         // ,
    INT_LIT,       // 0, 42, -5
    FLOAT_LIT,     // 3.14
    STRING_LIT,    // 'text' or "text"
    BOOL_TRUE,     // true
    BOOL_FALSE,    // false
    END_OF_FILE
};

struct GremlinParser::Token {
    GremlinTokenType type;
    std::string value;
    size_t position = 0;
};

// ============================================================================
// Lexer
// ============================================================================

struct GremlinParser::Lexer {
    const std::string& src;
    size_t pos = 0;

    explicit Lexer(const std::string& s) : src(s) {}

    char peek([[maybe_unused]] size_t offset = 0) const {
        size_t p = pos + offset;
        return (p < src.size()) ? src[p] : '\0';
    }

    char advance() {
        return (pos < src.size()) ? src[pos++] : '\0';
    }

    void skipWhitespace() {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
            ++pos;
    }

    std::string readString(char delim) {
        ++pos;  // skip opening delimiter
        std::string buf = {};
        while (pos < src.size() && src[pos] != delim) {
            if (src[pos] == '\\' && pos + 1 < src.size()) {
                ++pos;
                switch (src[pos]) {
                    case 'n': buf += '\n'; break;
                    case 't': buf += '\t'; break;
                    case 'r': buf += '\r'; break;
                    default:  buf += src[pos]; break;
                }
            } else {
                buf += src[pos];
            }
            ++pos;
        }
        if (pos < src.size()) ++pos;  // skip closing delimiter
        return buf;
    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens = {};

        tokens.reserve(src.size());
        while (true) {
            skipWhitespace();
            if (pos >= src.size()) {
                tokens.push_back({GremlinTokenType::END_OF_FILE, "", pos});
                break;
            }
            size_t start = pos;
            char c = src[pos];

            if (c == '.') {
                ++pos;
                tokens.push_back({GremlinTokenType::DOT, ".", start});
            } else if (c == '(') {
                ++pos;
                tokens.push_back({GremlinTokenType::LPAREN, "(", start});
            } else if (c == ')') {
                ++pos;
                tokens.push_back({GremlinTokenType::RPAREN, ")", start});
            } else if (c == ',') {
                ++pos;
                tokens.push_back({GremlinTokenType::COMMA, ",", start});
            } else if (c == '"' || c == '\'') {
                std::string s = readString(c);
                tokens.push_back({GremlinTokenType::STRING_LIT, s, start});
            } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
                bool neg = (c == '-');
                if (neg) {
                  ++pos;
                }
                std::string num = {};
                if (neg) {
                  num += '-';
                }
                bool has_dot = false;
                while (pos < src.size() &&
                       (std::isdigit(static_cast<unsigned char>(src[pos])) || src[pos] == '.')) {
                    if (src[pos] == '.') {
                        if (has_dot) {
                          break;
                        }
                        has_dot = true;
                    }
                    num += src[pos++];
                }
                if (has_dot)
                    tokens.push_back({GremlinTokenType::FLOAT_LIT, num, start});
                else
                    tokens.push_back({GremlinTokenType::INT_LIT, num, start});
            } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                std::string ident = {};
                while (pos < src.size() &&
                       (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_'))
                    ident += src[pos++];
                if (ident == "true")
                    tokens.push_back({GremlinTokenType::BOOL_TRUE, ident, start});
                else if (ident == "false")
                    tokens.push_back({GremlinTokenType::BOOL_FALSE, ident, start});
                else
                    tokens.push_back({GremlinTokenType::IDENT, ident, start});
            } else {
                // Unknown character – skip it
                ++pos;
            }
        }
        return tokens;
    }
};

// ============================================================================
// Parser
// ============================================================================

struct GremlinParser::Parser {
    const std::vector<Token>& tokens;
    size_t pos = 0;

    explicit Parser(const std::vector<Token>& toks) : tokens(toks) {}

    const Token& peek([[maybe_unused]] size_t offset = 0) const {
        size_t p = pos + offset;
        if (p >= tokens.size()) {
          return tokens.back();
        }
        return tokens[p];
    }

    const Token& consume() {
        const Token& t = tokens[pos];
        if (pos < tokens.size() - 1) {
          ++pos;
        }
        return t;
    }

    bool check(GremlinTokenType t, size_t offset = 0) const {
        return peek(offset).type == t;
    }

    bool matchIdent(const std::string& s, size_t offset = 0) const {
        return check(GremlinTokenType::IDENT, offset) && peek(offset).value == s;
    }

    void expect(GremlinTokenType t, const std::string& ctx) {
        if (!check(t))
            throw std::runtime_error("Expected " + ctx + " at position "
                                     + std::to_string(peek().position));
        consume();
    }

    // Parse a GremlinValue literal from the token stream
    GremlinValue parseValue() {
        const Token& t = peek();
        if (t.type == GremlinTokenType::STRING_LIT) {
            std::string v = t.value;
            consume();
            return GremlinValue(v);
        }
        if (t.type == GremlinTokenType::BOOL_TRUE) {
            consume();
            return GremlinValue(true);
        }
        if (t.type == GremlinTokenType::BOOL_FALSE) {
            consume();
            return GremlinValue(false);
        }
        if (t.type == GremlinTokenType::INT_LIT) {
            int64_t n;
            try { n = std::stoll(t.value); }
            catch (...) {
                THEMIS_WARN("gremlin_parser::parseValue: unhandled exception caught");
                throw std::runtime_error("Integer literal '" + t.value + "' is out of range at position "
                                         + std::to_string(t.position));
            }
            consume();
            return GremlinValue(n);
        }
        if (t.type == GremlinTokenType::FLOAT_LIT) {
            double d = 0;
            try { d = std::stod(t.value); }
            catch (...) {
                THEMIS_WARN("gremlin_parser::parseValue: unhandled exception caught");
                throw std::runtime_error("Float literal '" + t.value + "' is out of range at position "
                                         + std::to_string(t.position));
            }
            consume();
            return GremlinValue(d);
        }
        if (matchIdent("null")) {
            consume();
            return GremlinValue(nullptr);
        }
        throw std::runtime_error("Expected a literal value at position "
                                 + std::to_string(t.position));
    }

    // Parse P.eq(v) / P.within(v1,v2) / bare predicate identifier
    GremlinPredicate parsePredicate() {
        // P.op(args) syntax
        if (matchIdent("P") && check(GremlinTokenType::DOT, 1)) {
            consume();  // P
            consume();  // .
            if (!check(GremlinTokenType::IDENT))
                throw std::runtime_error("Expected predicate name after P.");
            std::string opName = peek().value;
            consume();

            static const std::unordered_map<std::string, GremlinPredOp> opMap = {
                {"eq",      GremlinPredOp::Eq},
                {"neq",     GremlinPredOp::Neq},
                {"lt",      GremlinPredOp::Lt},
                {"lte",     GremlinPredOp::Lte},
                {"gt",      GremlinPredOp::Gt},
                {"gte",     GremlinPredOp::Gte},
                {"within",  GremlinPredOp::Within},
                {"without", GremlinPredOp::Without}
            };
            auto it = opMap.find(opName);
            if (it == opMap.end())
                throw std::runtime_error("Unknown predicate: " + opName);

            expect(GremlinTokenType::LPAREN, "'('");
            std::vector<GremlinValue> vals = {};

            if (!check(GremlinTokenType::RPAREN))
                vals.push_back(parseValue());
            while (check(GremlinTokenType::COMMA)) {
                consume();
                vals.push_back(parseValue());
            }
            expect(GremlinTokenType::RPAREN, "')'");
            return GremlinPredicate{it->second, std::move(vals)};
        }

        // Bare identifier predicates: eq(v), neq(v), gt(v), …
        if (check(GremlinTokenType::IDENT)) {
            std::string opName = peek().value;
            static const std::unordered_map<std::string, GremlinPredOp> opMap = {
                {"eq",  GremlinPredOp::Eq},
                {"neq", GremlinPredOp::Neq},
                {"lt",  GremlinPredOp::Lt},
                {"lte", GremlinPredOp::Lte},
                {"gt",  GremlinPredOp::Gt},
                {"gte", GremlinPredOp::Gte}
            };
            auto it = opMap.find(opName);
            if (it != opMap.end()) {
                consume();  // opName
                expect(GremlinTokenType::LPAREN, "'('");
                std::vector<GremlinValue> vals = {};

                if (!check(GremlinTokenType::RPAREN))
                    vals.push_back(parseValue());
                expect(GremlinTokenType::RPAREN, "')'");
                return GremlinPredicate{it->second, std::move(vals)};
            }
        }

        throw std::runtime_error("Expected a predicate at position "
                                 + std::to_string(peek().position));
    }

    // Try to parse a predicate; if not possible, parse a literal value and wrap in Eq
    bool tryParsePredicateOrValue(GremlinStep& step) {
        // Check for P.xxx pattern or bare eq/neq/etc.
        bool looks_like_pred =
            (matchIdent("P") && check(GremlinTokenType::DOT, 1)) ||
            (check(GremlinTokenType::IDENT) &&
             std::unordered_map<std::string, int>{{"eq",0},{"neq",0},{"lt",0},
                 {"lte",0},{"gt",0},{"gte",0}}.count(peek().value));

        if (looks_like_pred) {
            step.predicate = parsePredicate();
            return true;
        }
        // Otherwise parse as a literal value
        step.values.push_back(parseValue());
        return false;
    }

    // Parse one step after '.'
    GremlinStep parseStep(const std::string& name, size_t stepPos) {
        GremlinStep step;

        // Mapping from step name to kind
        static const std::unordered_map<std::string, GremlinStepKind> stepMap = {
            {"hasLabel", GremlinStepKind::HasLabel},
            {"has",      GremlinStepKind::Has},
            {"hasNot",   GremlinStepKind::HasNot},
            {"hasId",    GremlinStepKind::HasId},
            {"out",      GremlinStepKind::Out},
            {"in",       GremlinStepKind::In},
            {"both",     GremlinStepKind::Both},
            {"outE",     GremlinStepKind::OutE},
            {"inE",      GremlinStepKind::InE},
            {"bothE",    GremlinStepKind::BothE},
            {"outV",     GremlinStepKind::OutV},
            {"inV",      GremlinStepKind::InV},
            {"bothV",    GremlinStepKind::BothV},
            {"values",   GremlinStepKind::Values},
            {"valueMap", GremlinStepKind::ValueMap},
            {"id",       GremlinStepKind::Id},
            {"label",    GremlinStepKind::Label},
            {"count",    GremlinStepKind::Count},
            {"dedup",    GremlinStepKind::Dedup},
            {"limit",    GremlinStepKind::Limit},
            {"range",    GremlinStepKind::Range},
            {"as",       GremlinStepKind::As},
            {"select",   GremlinStepKind::Select},
            {"order",    GremlinStepKind::Order},
            {"by",       GremlinStepKind::By}
        };

        auto it = stepMap.find(name);
        if (it == stepMap.end())
            throw std::runtime_error("Unknown Gremlin step '" + name
                                     + "' at position " + std::to_string(stepPos));
        step.kind = it->second;

        expect(GremlinTokenType::LPAREN, "'('");

        switch (step.kind) {
            case GremlinStepKind::HasLabel:
                // hasLabel("Label") or hasLabel("A", "B", …)
                while (!check(GremlinTokenType::RPAREN)) {
                    if (check(GremlinTokenType::STRING_LIT))
                        step.strings.push_back(peek().value);
                    consume();
                    if (check(GremlinTokenType::COMMA)) {
                      consume();
                    }
                }
                break;

            case GremlinStepKind::Has:
                // has("key") or has("key", value) or has("key", P.pred)
                if (check(GremlinTokenType::STRING_LIT)) {
                    step.strings.push_back(peek().value);
                    consume();
                }
                if (check(GremlinTokenType::COMMA)) {
                    consume();
                    if (!check(GremlinTokenType::RPAREN))
                        tryParsePredicateOrValue(step);
                }
                break;

            case GremlinStepKind::HasNot:
            [[fallthrough]];\n            case GremlinStepKind::HasId:
                if (check(GremlinTokenType::STRING_LIT) ||
                    check(GremlinTokenType::INT_LIT)) {
                    step.strings.push_back(peek().value);
                    consume();
                }
                break;

            case GremlinStepKind::Out:
            [[fallthrough]];\n            case GremlinStepKind::In:
            [[fallthrough]];\n            case GremlinStepKind::Both:
            [[fallthrough]];\n            case GremlinStepKind::OutE:
            [[fallthrough]];\n            case GremlinStepKind::InE:
            [[fallthrough]];\n            case GremlinStepKind::BothE:
                // out() / out("EDGE_LABEL") / out("A", "B")
                while (!check(GremlinTokenType::RPAREN)) {
                    if (check(GremlinTokenType::STRING_LIT))
                        step.strings.push_back(peek().value);
                    consume();
                    if (check(GremlinTokenType::COMMA)) {
                      consume();
                    }
                }
                break;

            case GremlinStepKind::OutV:
            [[fallthrough]];\n            case GremlinStepKind::InV:
            [[fallthrough]];\n            case GremlinStepKind::BothV:
            [[fallthrough]];\n            case GremlinStepKind::Count:
            [[fallthrough]];\n            case GremlinStepKind::Dedup:
            [[fallthrough]];\n            case GremlinStepKind::Id:
            [[fallthrough]];\n            case GremlinStepKind::Label:
            [[fallthrough]];\n            case GremlinStepKind::Order:
                // No arguments
                break;

            case GremlinStepKind::Values:
            [[fallthrough]];\n            case GremlinStepKind::ValueMap:
            [[fallthrough]];\n            case GremlinStepKind::Select:
                // values("p1") / valueMap("p1","p2") / select("a","b")
                while (!check(GremlinTokenType::RPAREN)) {
                    if (check(GremlinTokenType::STRING_LIT))
                        step.strings.push_back(peek().value);
                    consume();
                    if (check(GremlinTokenType::COMMA)) {
                      consume();
                    }
                }
                break;

            case GremlinStepKind::As:
                // as("alias")
                if (check(GremlinTokenType::STRING_LIT)) {
                    step.strings.push_back(peek().value);
                    consume();
                }
                break;

            case GremlinStepKind::Limit:
                // limit(n)
                if (check(GremlinTokenType::INT_LIT)) {
                    try { step.count = std::stoll(peek().value); }
                    catch (...) {
                        THEMIS_WARN("gremlin_parser: unhandled exception caught");
                        throw std::runtime_error("Gremlin limit count '" + peek().value + "' is out of range");
                    }
                    consume();
                }
                break;

            case GremlinStepKind::Range:
                // range(lo, hi)
                if (check(GremlinTokenType::INT_LIT)) {
                    try { step.count = std::stoll(peek().value); }
                    catch (...) {
                        THEMIS_WARN("gremlin_parser: unhandled exception caught");
                        throw std::runtime_error("Gremlin range start '" + peek().value + "' is out of range");
                    }
                    consume();
                }
                if (check(GremlinTokenType::COMMA)) {
                  consume();
                }
                if (check(GremlinTokenType::INT_LIT)) {
                    try { step.count2 = std::stoll(peek().value); }
                    catch (...) {
                        THEMIS_WARN("gremlin_parser: unhandled exception caught");
                        throw std::runtime_error("Gremlin range end '" + peek().value + "' is out of range");
                    }
                    consume();
                }
                break;

            case GremlinStepKind::By:
                // by("prop") or by("prop", decr)
                if (check(GremlinTokenType::STRING_LIT)) {
                    step.strings.push_back(peek().value);
                    consume();
                }
                if (check(GremlinTokenType::COMMA)) {
                  consume();
                }
                if (matchIdent("Order")) {
                    consume();
                    expect(GremlinTokenType::DOT, "'.' after Order");
                }
                if (matchIdent("decr") || matchIdent("desc")) {
                    step.ascending = false;
                    consume();
                }
                break;

            default:
                break;
        }

        expect(GremlinTokenType::RPAREN, "')'");
        return step;
    }

    GremlinASTNode parse() {
        GremlinASTNode ast;

        // Must start with 'g'
        if (!matchIdent("g"))
            throw std::runtime_error("Gremlin query must start with 'g'");
        consume();

        // '.'
        if (!check(GremlinTokenType::DOT))
            throw std::runtime_error("Expected '.' after 'g'");
        consume();

        // V or E
        if (!check(GremlinTokenType::IDENT))
            throw std::runtime_error("Expected 'V' or 'E' after 'g.'");
        std::string startStep = peek().value;
        size_t startPos = peek().position;
        if (startStep != "V" && startStep != "E")
            throw std::runtime_error("Expected 'V' or 'E' at position "
                                     + std::to_string(startPos));
        consume();

        GremlinStep start;
        start.kind = (startStep == "V") ? GremlinStepKind::V : GremlinStepKind::E;
        expect(GremlinTokenType::LPAREN, "'('");
        // Optional seed ID: g.V("id") or g.V(123)
        if (!check(GremlinTokenType::RPAREN)) {
            if (check(GremlinTokenType::STRING_LIT)) {
                start.values.emplace_back(peek().value);
                consume();
            } else if (check(GremlinTokenType::INT_LIT)) {
                int64_t iv;
                try { iv = std::stoll(peek().value); }
                catch (...) {
                    THEMIS_WARN("gremlin_parser::parse: unhandled exception caught");
                    throw std::runtime_error("Integer value '" + peek().value + "' is out of range");
                }
                start.values.emplace_back(iv);
                consume();
            }
        }
        expect(GremlinTokenType::RPAREN, "')'");
        ast.steps.push_back(std::move(start));

        // Parse chained steps: .stepName(args)
        while (check(GremlinTokenType::DOT)) {
            consume();  // '.'
            if (!check(GremlinTokenType::IDENT))
                throw std::runtime_error("Expected step name at position "
                                         + std::to_string(peek().position));
            std::string stepName = peek().value;
            size_t stepPos = peek().position;
            consume();
            ast.steps.push_back(parseStep(stepName, stepPos));
        }

        if (!check(GremlinTokenType::END_OF_FILE))
            throw std::runtime_error("Unexpected token at position "
                                     + std::to_string(peek().position));

        if (ast.steps.empty())
            throw std::runtime_error("Empty Gremlin traversal");

        return ast;
    }
};

// ============================================================================
// GremlinParser::parse
// ============================================================================

Result<GremlinASTNode> GremlinParser::parse(const std::string& gremlin) {
    if (gremlin.empty())
        return Err<GremlinASTNode>(errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                                   "empty Gremlin query");

    try {
        Lexer lexer(gremlin);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        return Ok(parser.parse());
    } catch (const std::exception& e) {
        return Err<GremlinASTNode>(errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                                   std::string(e.what()));
    }
}

// ============================================================================
// GremlinToAQLTranspiler
// ============================================================================

std::string GremlinToAQLTranspiler::valueToAQL(const GremlinValue& val) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) return "null";
        if constexpr (std::is_same_v<T, bool>)           return v ? "true" : "false";
        if constexpr (std::is_same_v<T, int64_t>)        return std::to_string(v);
        if constexpr (std::is_same_v<T, double>) {
            std::ostringstream os = {};
            os << v;
            return os.str();
        }
        if constexpr (std::is_same_v<T, std::string>) {
            // Escape double quotes
            std::string out = "\"";
            for (char c : v) {
                if (c == '"') {
                  out += "\\\"";
                }
                else if (c == '\\') out += "\\\\";
                else out += c;
            }
            out += "\"";
            return out;
        }
        return "null";
    }, val);
}

std::string GremlinToAQLTranspiler::predicateToAQL(const GremlinPredicate& pred,
                                                    const std::string& lhs) {
    if (pred.values.empty() &&
        pred.op != GremlinPredOp::Within &&
        pred.op != GremlinPredOp::Without)
        return lhs;  // no-op predicate

    auto rhs = [&]() -> std::string {
        return pred.values.empty() ? "null" : valueToAQL(pred.values[0]);
    };

    switch (pred.op) {
        case GremlinPredOp::Eq:      return lhs + " == " + rhs();
        case GremlinPredOp::Neq:     return lhs + " != " + rhs();
        case GremlinPredOp::Lt:      return lhs + " < "  + rhs();
        case GremlinPredOp::Lte:     return lhs + " <= " + rhs();
        case GremlinPredOp::Gt:      return lhs + " > "  + rhs();
        case GremlinPredOp::Gte:     return lhs + " >= " + rhs();
        case GremlinPredOp::Within: {
            std::string list = "[";
            for (size_t i = 0; i < pred.values.size(); ++i) {
                if (i) {
                  list += ", ";
                }
                list += valueToAQL(pred.values[i]);
            }
            list += "]";
            return lhs + " IN " + list;
        }
        case GremlinPredOp::Without: {
            std::string list = "[";
            for (size_t i = 0; i < pred.values.size(); ++i) {
                if (i) {
                  list += ", ";
                }
                list += valueToAQL(pred.values[i]);
            }
            list += "]";
            return lhs + " NOT IN " + list;
        }
    }
    return lhs;
}

Result<std::string> GremlinToAQLTranspiler::transpile(const GremlinASTNode& ast) {
    try {
        if (ast.steps.empty())
            return Err<std::string>(errors::ErrorCode::ERR_QUERY_INVALID,
                                    "Empty Gremlin AST");

        std::ostringstream aql;

        // -------------------------------------------------------
        // Collect structural info from step list
        // -------------------------------------------------------
        // Current vertex/edge variable name
        std::string vVar = "_v";
        // Labels collected from hasLabel()
        std::vector<std::string> labels;
        // Edge labels from out()/in()/both()
        std::vector<std::string> edgeLabels;
        // Traversal direction: 0=none, 1=out, 2=in, 3=both
        int traversalDir = 0;
        // Whether we have a traversal step
        bool hasTraversal = false;
        // Seed ID from V(id)
        std::string seedId;
        // Filters accumulated
        std::vector<std::string> filters;
        // Select aliases
        std::vector<std::string> selectAliases;
        // As alias
        std::string asAlias;
        // Values properties
        std::vector<std::string> valueProps;
        // ValueMap properties
        std::vector<std::string> valueMapProps;
        bool useValueMap = false;
        bool useId = false;
        bool useLabel = false;
        bool useCount = false;
        bool useDedup = false;
        // Sort
        std::string sortProp = {};
        bool sortAsc = true;
        bool hasSort = false;
        // Pagination
        std::optional<int64_t> limitVal;
        std::optional<int64_t> rangeStart;
        std::optional<int64_t> rangeEnd;
        bool startedWithE = false;

        for (const auto& step : ast.steps) {
            switch (step.kind) {
                case GremlinStepKind::V:
                    if (!step.strings.empty()) {
                        seedId = step.strings[0];
                    } else if (!step.values.empty()) {
                        seedId = valueToAQL(step.values[0]);
                    }
                    break;
                case GremlinStepKind::E:
                    startedWithE = true;
                    if (!step.strings.empty()) {
                        seedId = step.strings[0];
                    } else if (!step.values.empty()) {
                        seedId = valueToAQL(step.values[0]);
                    }
                    break;
                case GremlinStepKind::HasLabel:
                    for (const auto& l : step.strings) {
                      labels.push_back(l);
                    }
                    break;
                case GremlinStepKind::Has:
                    if (!step.strings.empty()) {
                        std::string key = step.strings[0];
                        if (step.predicate.has_value()) {
                            filters.push_back(predicateToAQL(*step.predicate, vVar + "." + key));
                        } else if (!step.values.empty()) {
                            filters.push_back(vVar + "." + key + " == " + valueToAQL(step.values[0]));
                        }
                        // has("key") alone → existence check
                        if (!step.predicate.has_value() && step.values.empty()) {
                            filters.push_back(vVar + "." + key + " != null");
                        }
                    }
                    break;
                case GremlinStepKind::HasNot:
                    if (!step.strings.empty())
                        filters.push_back(vVar + "." + step.strings[0] + " == null");
                    break;
                case GremlinStepKind::HasId:
                    if (!step.strings.empty())
                        filters.push_back(vVar + "._key == " + "\"" + step.strings[0] + "\"");
                    break;
                case GremlinStepKind::Out:
                    hasTraversal = true;
                    traversalDir = 1;
                    for (const auto& l : step.strings) {
                      edgeLabels.push_back(l);
                    }
                    break;
                case GremlinStepKind::In:
                    hasTraversal = true;
                    traversalDir = 2;
                    for (const auto& l : step.strings) {
                      edgeLabels.push_back(l);
                    }
                    break;
                case GremlinStepKind::Both:
                    hasTraversal = true;
                    traversalDir = 3;
                    for (const auto& l : step.strings) {
                      edgeLabels.push_back(l);
                    }
                    break;
                case GremlinStepKind::Values:
                    for (const auto& p : step.strings) {
                      valueProps.push_back(p);
                    }
                    break;
                case GremlinStepKind::ValueMap:
                    useValueMap = true;
                    for (const auto& p : step.strings) {
                      valueMapProps.push_back(p);
                    }
                    break;
                case GremlinStepKind::Id:
                    useId = true;
                    break;
                case GremlinStepKind::Label:
                    useLabel = true;
                    break;
                case GremlinStepKind::Count:
                    useCount = true;
                    break;
                case GremlinStepKind::Dedup:
                    useDedup = true;
                    break;
                case GremlinStepKind::As:
                    if (!step.strings.empty()) {
                      asAlias = step.strings[0];
                    }
                    break;
                case GremlinStepKind::Select:
                    for (const auto& a : step.strings) {
                      selectAliases.push_back(a);
                    }
                    break;
                case GremlinStepKind::Order:
                    hasSort = true;
                    break;
                case GremlinStepKind::By:
                    if (!step.strings.empty()) {
                      sortProp = step.strings[0];
                    }
                    sortAsc = step.ascending;
                    break;
                case GremlinStepKind::Limit:
                    if (step.count.has_value()) {
                      limitVal = step.count;
                    }
                    break;
                case GremlinStepKind::Range:
                    if (step.count.has_value()) {
                      rangeStart = step.count;
                    }
                    if (step.count2.has_value()) {
                      rangeEnd   = step.count2;
                    }
                    break;
                default:
                    break;
            }
        }

        // -------------------------------------------------------
        // Determine the collection/source to iterate
        // -------------------------------------------------------
        std::string collection = {};
        if (!labels.empty())
            collection = labels[0];  // primary label → collection name
        else if (startedWithE)
            collection = "_edges";
        else
            collection = "_vertices";

        // -------------------------------------------------------
        // Build AQL
        // -------------------------------------------------------

        // FOR loop / source
        if (!seedId.empty()) {
            aql << "FOR " << vVar << " IN " << collection << "\n";
            if (seedId.size() >= 2 && seedId.front() == '"' && seedId.back() == '"') {
                aql << "FILTER " << vVar << "._key == " << seedId << "\n";
            } else {
                aql << "FILTER " << vVar << "._key == \"" << seedId << "\"\n";
            }
        } else {
            aql << "FOR " << vVar << " IN " << collection << "\n";
        }

        // FILTER from hasLabel (additional labels)
        for (size_t i = 1; i < labels.size(); ++i)
            aql << "FILTER " << vVar << "._label == \"" << labels[i] << "\"\n";

        // FILTER from has() / hasNot()
        for (const auto& f : filters)
            aql << "FILTER " << f << "\n";

        // TRAVERSAL step
        std::string nVar = "_n";
        if (hasTraversal) {
            std::string dir = {};
            if (traversalDir == 1) {
              dir = "OUTBOUND";
            }
            else if (traversalDir == 2) dir = "INBOUND";
            else dir = "ANY";

            std::string graphName =
                edgeLabels.empty() ? "_edges" : edgeLabels[0];

            aql << "FOR _e, " << nVar << " IN 1..1 " << dir << " "
                << vVar << " GRAPH \"" << graphName << "\"\n";

            // Additional edge label filters
            for (size_t i = 1; i < edgeLabels.size(); ++i)
                aql << "FILTER _e._label == \"" << edgeLabels[i] << "\"\n";
        }

        // SORT
        if (hasSort && !sortProp.empty()) {
            std::string sortVar = hasTraversal ? nVar : vVar;
            aql << "SORT " << sortVar << "." << sortProp
                << (sortAsc ? " ASC" : " DESC") << "\n";
        }

        // LIMIT / RANGE
        if (rangeStart.has_value() && rangeEnd.has_value()) {
            aql << "LIMIT " << *rangeStart << ", " << (*rangeEnd - *rangeStart) << "\n";
        } else if (limitVal.has_value()) {
            aql << "LIMIT " << *limitVal << "\n";
        }

        // RETURN
        std::string retVar = hasTraversal ? nVar : vVar;

        if (useCount) {
            // Wrap inner query in LENGTH()
            // We already have the FOR/FILTER lines; emit as subquery
            std::string inner = aql.str();
            aql.str("");
            aql.clear();
            aql << "RETURN LENGTH(" << inner << "RETURN " << retVar << ")";
            return Ok(aql.str());
        }

        const std::string returnPrefix = useDedup ? "RETURN DISTINCT " : "RETURN ";

        if (!selectAliases.empty()) {
            aql << returnPrefix << "{";
            for (size_t i = 0; i < selectAliases.size(); ++i) {
                if (i) {
                  aql << ", ";
                }
                aql << selectAliases[i] << ": " << retVar;
            }
            aql << "}";
        } else if (!valueProps.empty()) {
            if (valueProps.size() == 1) {
                aql << returnPrefix << retVar << "." << valueProps[0];
            } else {
                aql << returnPrefix << "{";
                for (size_t i = 0; i < valueProps.size(); ++i) {
                    if (i) {
                      aql << ", ";
                    }
                    aql << valueProps[i] << ": " << retVar << "." << valueProps[i];
                }
                aql << "}";
            }
        } else if (useValueMap) {
            if (valueMapProps.empty()) {
                aql << returnPrefix << retVar;
            } else {
                aql << returnPrefix << "{";
                for (size_t i = 0; i < valueMapProps.size(); ++i) {
                    if (i) {
                      aql << ", ";
                    }
                    aql << valueMapProps[i] << ": " << retVar << "." << valueMapProps[i];
                }
                aql << "}";
            }
        } else if (useId) {
            aql << returnPrefix << retVar << "._key";
        } else if (useLabel) {
            aql << returnPrefix << retVar << "._label";
        } else {
            aql << returnPrefix << retVar;
        }

        return Ok(aql.str());

    } catch (const std::exception& e) {
        return Err<std::string>(errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                                std::string("Gremlin transpilation error: ") + e.what());
    }
}

}  // namespace query
}  // namespace themis

