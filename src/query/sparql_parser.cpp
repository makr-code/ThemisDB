/**
 * @file sparql_parser.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=19, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPARQL compatibility layer – SELECT query parsing and AQL transpilation.
// Implements a standalone SPARQL tokenizer + recursive-descent parser and an
// AQL transpiler.  Generated AQL can be fed directly into executeAql().
//
// Thread-safety note: SPARQLParser instances are NOT thread-safe.
// Create one instance per thread or protect with a mutex.

#include "query/sparql_parser.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "utils/error_registry.h"
#include "utils/logger.h"

namespace themis {
namespace query {

// ============================================================================
// Helpers – literal value serialisation shared by parser and transpiler
// ============================================================================

namespace {

std::string sparqlLiteralToAQL(const SPARQLLiteralValue& val) {
    return std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss = {};
            oss << v;
            return oss.str();
        } else {
            // std::string – emit as a quoted AQL string literal
            std::string out = {};
            out.reserve(static_cast<int>(v.size()) + 2);
            out += '"';
            for (char c : v) {
                if (c == '"') {
                  out += "\\\"";
                }
                else if (c == '\\') out += "\\\\";
                else if (c == '\n') out += "\\n";
                else if (c == '\r') out += "\\r";
                else if (c == '\t') out += "\\t";
                else                out += c;
            }
            out += '"';
            return out;
        }
    }, val);
}

// ============================================================================
// Lexer
// ============================================================================

enum class SPARQLTokenType {
    // Keywords
    SELECT, WHERE, FILTER, FROM, ORDER, BY, ASC, DESC, LIMIT, OFFSET,
    TRUE_KW, FALSE_KW,
    // Punctuation
    LBRACE, RBRACE, LPAREN, RPAREN, DOT, COMMA, STAR, SEMICOLON,
    // Operators
    EQ, NEQ, LT, LTE, GT, GTE, AND_OP, OR_OP, NOT_OP,
    // Terms
    VAR,         ///< ?name or $name
    URI,         ///< <uri>
    PREFIXED,    ///< prefix:local or bare identifier
    STRING_LIT,  ///< "..." or '...'
    INT_LIT,
    FLOAT_LIT,
    // Special
    END_OF_INPUT, INVALID
};

struct SPARQLToken {
    SPARQLTokenType type;
    std::string     value;
    size_t          pos = 0;
};

static const std::unordered_map<std::string, SPARQLTokenType> kSPARQLKeywords = {
    {"SELECT",  SPARQLTokenType::SELECT},
    {"WHERE",   SPARQLTokenType::WHERE},
    {"FILTER",  SPARQLTokenType::FILTER},
    {"FROM",    SPARQLTokenType::FROM},
    {"ORDER",   SPARQLTokenType::ORDER},
    {"BY",      SPARQLTokenType::BY},
    {"ASC",     SPARQLTokenType::ASC},
    {"DESC",    SPARQLTokenType::DESC},
    {"LIMIT",   SPARQLTokenType::LIMIT},
    {"OFFSET",  SPARQLTokenType::OFFSET},
    {"TRUE",    SPARQLTokenType::TRUE_KW},
    {"FALSE",   SPARQLTokenType::FALSE_KW},
};

class SPARQLLexer {
public:
    explicit SPARQLLexer(const std::string& input) : input_(input), pos_(0) {}

    std::vector<SPARQLToken> tokenize() {
        std::vector<SPARQLToken> tokens = {};

        while (static_cast<size_t>(pos_) < input_.size()) {
            skipWhitespace();
            if (pos_ >= static_cast<int>(input_.size())) {
              break;
            }

            size_t start = pos_;
            char c = input_[pos_];

            // Comment (# to end of line)
            if (c == '#') {
                while (pos_ < input_.size() && input_[pos_] != '\n') {
                  ++pos_;
                }
                continue;
            }

            // Variable ?name or $name
            if (c == '?' || c == '$') {
                ++pos_;
                size_t name_start = pos_;
                while (pos_ < input_.size() &&
                       (std::isalnum(static_cast<unsigned char>(input_[pos_])) || input_[pos_] == '_')) {
                    ++pos_;
                }
                tokens.push_back({SPARQLTokenType::VAR,
                                   input_.substr(name_start, pos_ - name_start), start});
                continue;
            }

            // URI reference <uri> vs. comparison operator < / <=
            if (c == '<') {
                char next = (pos_ + 1 < input_.size()) ? input_[pos_ + 1] : '\0';
                // Treat as URI when the character after '<' starts a typical URI scheme
                // (letter, digit, underscore, slash, hash) and is not '=' or '>'
                if (next != '=' && next != '>' && next != ' ' && next != '\t' &&
                    next != '\n' && next != '\r' && next != '(' && next != ')' &&
                    next != ',' && next != '{' && next != '}' && next != '\0' &&
                    (std::isalpha(static_cast<unsigned char>(next)) ||
                     next == '_' || next == '/' || next == '#')) {
                    ++pos_;  // consume '<'
                    size_t uri_start = pos_;
                    while (pos_ < input_.size() && input_[pos_] != '>') {
                      ++pos_;
                    }
                    std::string uri = input_.substr(uri_start, pos_ - uri_start);
                    if (static_cast<int>(input_.size()) > pos_) ++pos_;  // consume '>'
                    tokens.push_back({SPARQLTokenType::URI, uri, start});
                    continue;
                } else if (next == '=') {
                    tokens.push_back({SPARQLTokenType::LTE, "<=", start});
                    pos_ += 2;
                    continue;
                } else {
                    tokens.push_back({SPARQLTokenType::LT, "<", start});
                    ++pos_;
                    continue;
                }
            }

            // String literals
            if (c == '"' || c == '\'') {
                tokens.push_back(readString(c, start));
                continue;
            }

            // Numbers (and negative numbers)
            if (std::isdigit(static_cast<unsigned char>(c)) ||
                (c == '-' && pos_ + 1 < input_.size() &&
                 std::isdigit(static_cast<unsigned char>(input_[pos_ + 1])))) {
                tokens.push_back(readNumber(start));
                continue;
            }

            // Identifiers / keywords / prefixed names
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                tokens.push_back(readIdent(start));
                continue;
            }

            // Single- and multi-character operators / punctuation
            switch (c) {
                case '{': tokens.push_back({SPARQLTokenType::LBRACE,  "{",  start}); ++pos_; break;
                case '}': tokens.push_back({SPARQLTokenType::RBRACE,  "}",  start}); ++pos_; break;
                case '(': tokens.push_back({SPARQLTokenType::LPAREN,  "(",  start}); ++pos_; break;
                case ')': tokens.push_back({SPARQLTokenType::RPAREN,  ")",  start}); ++pos_; break;
                case '.': tokens.push_back({SPARQLTokenType::DOT,     ".",  start}); ++pos_; break;
                case ',': tokens.push_back({SPARQLTokenType::COMMA,   ",",  start}); ++pos_; break;
                case '*': tokens.push_back({SPARQLTokenType::STAR,    "*",  start}); ++pos_; break;
                case ';': tokens.push_back({SPARQLTokenType::SEMICOLON, ";", start}); ++pos_; break;
                case '=': tokens.push_back({SPARQLTokenType::EQ,      "==", start}); ++pos_; break;
                case '>':
                    if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
                        tokens.push_back({SPARQLTokenType::GTE, ">=", start}); pos_ += 2;
                    } else {
                        tokens.push_back({SPARQLTokenType::GT,  ">",  start}); ++pos_;
                    }
                    break;
                case '!':
                    if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
                        tokens.push_back({SPARQLTokenType::NEQ,    "!=", start}); pos_ += 2;
                    } else {
                        tokens.push_back({SPARQLTokenType::NOT_OP, "!",  start}); ++pos_;
                    }
                    break;
                case '&':
                    if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '&') {
                        tokens.push_back({SPARQLTokenType::AND_OP, "&&", start}); pos_ += 2;
                    } else {
                        tokens.push_back({SPARQLTokenType::INVALID, "&", start}); ++pos_;
                    }
                    break;
                case '|':
                    if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '|') {
                        tokens.push_back({SPARQLTokenType::OR_OP, "||", start}); pos_ += 2;
                    } else {
                        tokens.push_back({SPARQLTokenType::INVALID, "|", start}); ++pos_;
                    }
                    break;
                default:
                    tokens.push_back({SPARQLTokenType::INVALID, std::string(1, c), start});
                    ++pos_;
                    break;
            }
        }
        tokens.push_back({SPARQLTokenType::END_OF_INPUT, "", pos_});
        return tokens;
    }

private:
    const std::string& input_;
    size_t pos_ = {};

    void skipWhitespace() {
        while (pos_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }

    SPARQLToken readString(char quote, size_t start) {
        ++pos_;  // skip opening quote
        std::string val = {};
        while (pos_ < input_.size() && input_[pos_] != quote) {
            if (input_[pos_] == '\\' && pos_ + 1 < input_.size()) {
                char esc = input_[pos_ + 1];
                switch (esc) {
                    case '"':  case '\'': case '\\': val += esc; break;
                    case 'n':  val += '\n'; break;
                    case 'r':  val += '\r'; break;
                    case 't':  val += '\t'; break;
                    default:   val += '\\'; val += esc; break;
                }
                pos_ += 2;
            } else {
                val += input_[pos_++];
            }
        }
        if (static_cast<int>(input_.size()) > pos_) ++pos_;  // skip closing quote
        return {SPARQLTokenType::STRING_LIT, val, start};
    }

    SPARQLToken readNumber([[maybe_unused]] size_t start) {
        size_t num_start = pos_;
        if (input_[pos_] == '-') {
          ++pos_;
        }
        while (pos_ < input_.size() &&
               std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
        bool is_float = false;
        if (pos_ < input_.size() && input_[pos_] == '.') {
            is_float = true;
            ++pos_;
            while (pos_ < input_.size() &&
                   std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }
        std::string num_str = input_.substr(num_start, pos_ - num_start);
        return {is_float ? SPARQLTokenType::FLOAT_LIT : SPARQLTokenType::INT_LIT,
                num_str, start};
    }

    SPARQLToken readIdent([[maybe_unused]] size_t start) {
        size_t ident_start = pos_;
        while (pos_ < input_.size() &&
               (std::isalnum(static_cast<unsigned char>(input_[pos_])) ||
                input_[pos_] == '_' || input_[pos_] == '-')) {
            ++pos_;
        }
        std::string ident = input_.substr(ident_start, pos_ - ident_start);

        // Prefixed name: ident followed immediately by ':' and local part
        if (pos_ < input_.size() && input_[pos_] == ':') {
            ++pos_;  // consume ':'
            size_t local_start = pos_;
            while (pos_ < input_.size() &&
                   (std::isalnum(static_cast<unsigned char>(input_[pos_])) ||
                    input_[pos_] == '_' || input_[pos_] == '-')) {
                ++pos_;
            }
            std::string local = input_.substr(local_start, pos_ - local_start);
            return {SPARQLTokenType::PREFIXED, ident + ":" + local, start};
        }

        // Keyword check (case-insensitive)
        std::string upper = ident;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        auto it = kSPARQLKeywords.find(upper);
        if (it != kSPARQLKeywords.end()) {
            return {it->second, upper, start};
        }

        return {SPARQLTokenType::PREFIXED, ident, start};
    }
};

// ============================================================================
// Recursive-descent parser
// ============================================================================

class SPARQLParserImpl {
public:
    explicit SPARQLParserImpl(std::vector<SPARQLToken> tokens)
        : tokens_(std::move(tokens)), pos_(0) {}

    Result<SPARQLASTNode> parse() {
        try {
            if (!check(SPARQLTokenType::SELECT)) {
                return parseError<SPARQLASTNode>("Expected SELECT keyword");
            }

            auto select_result = parseSelect();
            if (!select_result) {
                return Err<SPARQLASTNode>(select_result.error().code(),
                                          select_result.error().context());
            }

            // Optional trailing semicolon
            match(SPARQLTokenType::SEMICOLON);

            if (!check(SPARQLTokenType::END_OF_INPUT)) {
                return parseError<SPARQLASTNode>("Unexpected token after query");
            }

            SPARQLASTNode ast;
            ast.select = std::move(*select_result);
            return Ok(std::move(ast));

        } catch (const std::exception& e) {
            return Err<SPARQLASTNode>(errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                                      std::string(e.what()));
        }
    }

private:
    std::vector<SPARQLToken> tokens_;
    size_t pos_ = {};

    const SPARQLToken& current() const { return tokens_[pos_]; }

    void advance() {
        if (pos_ + 1 < tokens_.size()) {
          ++pos_;
        }
    }

    bool check(SPARQLTokenType t) const { return current().type == t; }

    bool match(SPARQLTokenType t) {
        if (check(t)) { advance(); return true; }
        return false;
    }

    template<typename T>
    Result<T> parseError(const std::string& msg) const {
        return Err<T>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            msg + " (at position " + std::to_string(current().pos) +
                ", token '" + current().value + "')");
    }

    // ---------- SELECT ----------

    Result<SPARQLSelectStatement> parseSelect() {
        advance();  // consume SELECT
        SPARQLSelectStatement stmt;

        // SELECT * or SELECT ?var1 ?var2 ...
        if (check(SPARQLTokenType::STAR)) {
            stmt.star = true;
            advance();
        } else {
            while (check(SPARQLTokenType::VAR)) {
                stmt.variables.push_back(current().value);
                advance();
            }
            if (stmt.variables.empty()) {
                return parseError<SPARQLSelectStatement>(
                    "Expected variable list or * after SELECT");
            }
        }

        // Optional FROM <graph_uri>
        if (check(SPARQLTokenType::FROM)) {
            advance();
            if (!check(SPARQLTokenType::URI)) {
                return parseError<SPARQLSelectStatement>("Expected URI after FROM");
            }
            stmt.from_graph = current().value;
            advance();
        }

        // WHERE { ... }
        if (!check(SPARQLTokenType::WHERE)) {
            return parseError<SPARQLSelectStatement>("Expected WHERE keyword");
        }
        advance();

        if (!check(SPARQLTokenType::LBRACE)) {
            return parseError<SPARQLSelectStatement>("Expected '{' after WHERE");
        }
        advance();

        // Graph patterns: triple patterns and FILTER clauses
        while (!check(SPARQLTokenType::RBRACE) &&
               !check(SPARQLTokenType::END_OF_INPUT)) {
            if (check(SPARQLTokenType::FILTER)) {
                advance();  // consume FILTER
                auto expr = parseFilterExpr();
                if (!expr) {
                    return Err<SPARQLSelectStatement>(expr.error().code(),
                                                      expr.error().context());
                }
                SPARQLWhereClause clause;
                clause.kind       = SPARQLClauseKind::Filter;
                clause.filter_expr = std::move(*expr);
                stmt.where_clauses.push_back(std::move(clause));
            } else {
                auto triple = parseTriplePattern();
                if (!triple) {
                    return Err<SPARQLSelectStatement>(triple.error().code(),
                                                      triple.error().context());
                }
                SPARQLWhereClause clause;
                clause.kind   = SPARQLClauseKind::TriplePattern;
                clause.triple = std::move(*triple);
                stmt.where_clauses.push_back(std::move(clause));
            }
        }

        if (!check(SPARQLTokenType::RBRACE)) {
            return parseError<SPARQLSelectStatement>("Expected '}' to close WHERE clause");
        }
        advance();

        // Optional ORDER BY
        if (check(SPARQLTokenType::ORDER)) {
            advance();
            if (!match(SPARQLTokenType::BY)) {
                return parseError<SPARQLSelectStatement>("Expected BY after ORDER");
            }

            // One or more order specs
            while (true) {
                bool has_dir   = false;
                bool ascending = true;

                if (check(SPARQLTokenType::ASC)) {
                    has_dir = true;
                    advance();
                    if (!match(SPARQLTokenType::LPAREN)) {
                        return parseError<SPARQLSelectStatement>("Expected '(' after ASC");
                    }
                } else if (check(SPARQLTokenType::DESC)) {
                    has_dir    = true;
                    ascending  = false;
                    advance();
                    if (!match(SPARQLTokenType::LPAREN)) {
                        return parseError<SPARQLSelectStatement>("Expected '(' after DESC");
                    }
                }

                if (!check(SPARQLTokenType::VAR)) {
                    if (has_dir) {
                        return parseError<SPARQLSelectStatement>(
                            "Expected variable in ORDER BY");
                    }
                    break;  // no more ORDER BY items
                }
                std::string var = current().value;
                advance();

                if (has_dir && !match(SPARQLTokenType::RPAREN)) {
                    return parseError<SPARQLSelectStatement>(
                        "Expected ')' after ORDER BY variable");
                }
                stmt.order_by.push_back({var, ascending});

                // Continue only if the next token is another direction keyword or variable
                if (!check(SPARQLTokenType::ASC) &&
                    !check(SPARQLTokenType::DESC) &&
                    !check(SPARQLTokenType::VAR)) {
                    break;
                }
            }
        }

        // Optional LIMIT
        if (check(SPARQLTokenType::LIMIT)) {
            advance();
            if (!check(SPARQLTokenType::INT_LIT)) {
                return parseError<SPARQLSelectStatement>("Expected integer after LIMIT");
            }
            try { stmt.limit = std::stoll(current().value); }
            catch (...) {
                THEMIS_DEBUG("sparql_parser: unhandled exception caught");
                throw std::runtime_error("SPARQL LIMIT value '" + current().value + "' is out of integer range");
            }
            advance();
        }

        // Optional OFFSET
        if (check(SPARQLTokenType::OFFSET)) {
            advance();
            if (!check(SPARQLTokenType::INT_LIT)) {
                return parseError<SPARQLSelectStatement>("Expected integer after OFFSET");
            }
            try { stmt.offset = std::stoll(current().value); }
            catch (...) {
                THEMIS_DEBUG("sparql_parser: unhandled exception caught");
                throw std::runtime_error("SPARQL OFFSET value '" + current().value + "' is out of integer range");
            }
            advance();
        }

        return Ok(std::move(stmt));
    }

    // ---------- Triple Pattern ----------

    Result<SPARQLTriplePattern> parseTriplePattern() {
        auto subj = parseTerm();
        if (!subj) return Err<SPARQLTriplePattern>(subj.error().code(),
                                                    subj.error().context());
        auto pred = parseTerm();
        if (!pred) return Err<SPARQLTriplePattern>(pred.error().code(),
                                                    pred.error().context());
        auto obj  = parseTerm();
        if (!obj)  return Err<SPARQLTriplePattern>(obj.error().code(),
                                                    obj.error().context());
        match(SPARQLTokenType::DOT);  // optional trailing dot
        return Ok(SPARQLTriplePattern{*subj, *pred, *obj});
    }

    Result<SPARQLTerm> parseTerm() {
        SPARQLTerm term = {};

        if (check(SPARQLTokenType::VAR)) {
            term.type  = SPARQLTermType::Variable;
            term.value = current().value;
            advance();
        } else if (check(SPARQLTokenType::URI)) {
            term.type  = SPARQLTermType::URIRef;
            term.value = current().value;
            advance();
        } else if (check(SPARQLTokenType::PREFIXED)) {
            term.type  = SPARQLTermType::PrefixedName;
            term.value = current().value;
            advance();
        } else if (check(SPARQLTokenType::STRING_LIT)) {
            term.type           = SPARQLTermType::Literal;
            term.value          = current().value;
            term.literal_value  = std::string(current().value);
            term.is_literal_value = true;
            advance();
        } else if (check(SPARQLTokenType::INT_LIT)) {
            term.type           = SPARQLTermType::Literal;
            term.value          = current().value;
            try { term.literal_value  = std::stoll(current().value); }
            catch (...) {
                THEMIS_WARN("sparql_parser::parseTerm: unhandled exception caught");
                throw std::runtime_error("Integer literal '" + current().value + "' is out of range");
            }
            term.is_literal_value = true;
            advance();
        } else if (check(SPARQLTokenType::FLOAT_LIT)) {
            term.type           = SPARQLTermType::Literal;
            term.value          = current().value;
            try { term.literal_value  = std::stod(current().value); }
            catch (...) {
                THEMIS_WARN("sparql_parser::parseTerm: unhandled exception caught");
                throw std::runtime_error("Float literal '" + current().value + "' is out of range");
            }
            term.is_literal_value = true;
            advance();
        } else if (check(SPARQLTokenType::TRUE_KW)) {
            term.type           = SPARQLTermType::Literal;
            term.value          = "true";
            term.literal_value  = true;
            term.is_literal_value = true;
            advance();
        } else if (check(SPARQLTokenType::FALSE_KW)) {
            term.type           = SPARQLTermType::Literal;
            term.value          = "false";
            term.literal_value  = false;
            term.is_literal_value = true;
            advance();
        } else {
            return parseError<SPARQLTerm>(
                "Expected variable, URI, prefixed name, or literal");
        }
        return Ok(std::move(term));
    }

    // ---------- Filter expressions ----------

    Result<std::shared_ptr<SPARQLExpr>> parseFilterExpr() {
        if (!check(SPARQLTokenType::LPAREN)) {
            return parseError<std::shared_ptr<SPARQLExpr>>("Expected '(' after FILTER");
        }
        advance();
        auto expr = parseExpr();
        if (!expr) {
          return expr;
        }
        if (!match(SPARQLTokenType::RPAREN)) {
            return parseError<std::shared_ptr<SPARQLExpr>>(
                "Expected ')' to close FILTER");
        }
        return expr;
    }

    Result<std::shared_ptr<SPARQLExpr>> parseExpr()         { return parseOrExpr(); }

    Result<std::shared_ptr<SPARQLExpr>> parseOrExpr() {
        auto left = parseAndExpr();
        if (!left) {
          return left;
        }
        while (check(SPARQLTokenType::OR_OP)) {
            advance();
            auto right = parseAndExpr();
            if (!right) {
              return right;
            }
            auto node  = std::make_shared<SPARQLBinaryOpExpr>();
            node->op   = "||";
            node->left = std::move(*left);
            node->right = std::move(*right);
            left = Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        return left;
    }

    Result<std::shared_ptr<SPARQLExpr>> parseAndExpr() {
        auto left = parseRelationalExpr();
        if (!left) {
          return left;
        }
        while (check(SPARQLTokenType::AND_OP)) {
            advance();
            auto right = parseRelationalExpr();
            if (!right) {
              return right;
            }
            auto node  = std::make_shared<SPARQLBinaryOpExpr>();
            node->op   = "&&";
            node->left = std::move(*left);
            node->right = std::move(*right);
            left = Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        return left;
    }

    Result<std::shared_ptr<SPARQLExpr>> parseRelationalExpr() {
        auto left = parseUnaryExpr();
        if (!left) {
          return left;
        }

        std::string op = {};
        if      (check(SPARQLTokenType::EQ)) {
          op = "==";
        }
        else if (check(SPARQLTokenType::NEQ)) op = "!=";
        else if (check(SPARQLTokenType::LT))  op = "<";
        else if (check(SPARQLTokenType::LTE)) op = "<=";
        else if (check(SPARQLTokenType::GT))  op = ">";
        else if (check(SPARQLTokenType::GTE)) op = ">=";

        if (!op.empty()) {
            advance();
            auto right = parseUnaryExpr();
            if (!right) {
              return right;
            }
            auto node  = std::make_shared<SPARQLBinaryOpExpr>();
            node->op   = op;
            node->left = std::move(*left);
            node->right = std::move(*right);
            return Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        return left;
    }

    Result<std::shared_ptr<SPARQLExpr>> parseUnaryExpr() {
        if (check(SPARQLTokenType::NOT_OP)) {
            advance();
            auto operand = parsePrimaryExpr();
            if (!operand) {
              return operand;
            }
            auto node    = std::make_shared<SPARQLUnaryOpExpr>();
            node->op      = "!";
            node->operand = std::move(*operand);
            return Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        return parsePrimaryExpr();
    }

    Result<std::shared_ptr<SPARQLExpr>> parsePrimaryExpr() {
        if (check(SPARQLTokenType::LPAREN)) {
            advance();
            auto expr = parseExpr();
            if (!expr) {
              return expr;
            }
            if (!match(SPARQLTokenType::RPAREN)) {
                return parseError<std::shared_ptr<SPARQLExpr>>("Expected ')'");
            }
            return expr;
        }
        if (check(SPARQLTokenType::VAR)) {
            auto node = std::make_shared<SPARQLVariableExpr>();
            node->name = current().value;
            advance();
            return Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        if (check(SPARQLTokenType::STRING_LIT)) {
            auto node  = std::make_shared<SPARQLLiteralExpr>();
            node->value = std::string(current().value);
            advance();
            return Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        if (check(SPARQLTokenType::INT_LIT)) {
            auto node  = std::make_shared<SPARQLLiteralExpr>();
            try { node->value = std::stoll(current().value); }
            catch (...) {
                THEMIS_WARN("sparql_parser::parsePrimaryExpr: unhandled exception caught");
                throw std::runtime_error("Integer literal '" + current().value + "' is out of range");
            }
            advance();
            return Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        if (check(SPARQLTokenType::FLOAT_LIT)) {
            auto node  = std::make_shared<SPARQLLiteralExpr>();
            try { node->value = std::stod(current().value); }
            catch (...) {
                THEMIS_WARN("sparql_parser::parsePrimaryExpr: unhandled exception caught");
                throw std::runtime_error("Float literal '" + current().value + "' is out of range");
            }
            advance();
            return Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        if (check(SPARQLTokenType::TRUE_KW)) {
            auto node  = std::make_shared<SPARQLLiteralExpr>();
            node->value = true;
            advance();
            return Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        if (check(SPARQLTokenType::FALSE_KW)) {
            auto node  = std::make_shared<SPARQLLiteralExpr>();
            node->value = false;
            advance();
            return Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        if (check(SPARQLTokenType::URI)) {
            auto node  = std::make_shared<SPARQLLiteralExpr>();
            node->value = std::string(current().value);
            advance();
            return Ok<std::shared_ptr<SPARQLExpr>>(std::move(node));
        }
        return parseError<std::shared_ptr<SPARQLExpr>>("Expected expression");
    }
};

// ============================================================================
// AQL generation helpers (used by transpiler)
// ============================================================================

/// Serialise a SPARQL term to an AQL string.
/// Variables are resolved through var_bindings; constants become AQL literals.
static std::string termToAQLStr(const SPARQLTerm& term,
                                const std::map<std::string, std::string>& var_bindings,
                                const std::string& current_triple,
                                const std::string& field) {
    switch (term.type) {
        case SPARQLTermType::Variable: {
            auto it = var_bindings.find(term.value);
            if (it != var_bindings.end()) {
              return it->second;
            }
            return current_triple + "." + field;  // first/unbound variable
        }
        case SPARQLTermType::URIRef:
        [[fallthrough]];\n        case SPARQLTermType::PrefixedName:
            return "\"" + term.value + "\"";
        case SPARQLTermType::Literal:
            if (term.is_literal_value) {
              return sparqlLiteralToAQL(term.literal_value);
            }
            return "\"" + term.value + "\"";
    }
    return "null";
}

/// Serialise a FILTER expression to AQL, resolving variables through bindings.
static std::string filterExprToAQL(const SPARQLExpr& expr,
                                   const std::map<std::string, std::string>& var_bindings) {
    switch (expr.type()) {
        case SPARQLExprType::Literal: {
            return sparqlLiteralToAQL(static_cast<const SPARQLLiteralExpr&>(expr).value);
        }
        case SPARQLExprType::Variable: {
            const auto& v = static_cast<const SPARQLVariableExpr&>(expr);
            auto it = var_bindings.find(v.name);
            return (it != var_bindings.end()) ? it->second : "null";
        }
        case SPARQLExprType::BinaryOp: {
            const auto& b = static_cast<const SPARQLBinaryOpExpr&>(expr);
            std::string left  = filterExprToAQL(*b.left,  var_bindings);
            std::string right = filterExprToAQL(*b.right, var_bindings);
            // Map SPARQL logical operators to AQL keywords
            std::string aql_op = b.op;
            if (b.op == "&&") {
              aql_op = "AND";
            }
            else if (b.op == "||") aql_op = "OR";
            return "(" + left + " " + aql_op + " " + right + ")";
        }
        case SPARQLExprType::UnaryOp: {
            const auto& u = static_cast<const SPARQLUnaryOpExpr&>(expr);
            std::string operand = filterExprToAQL(*u.operand, var_bindings);
            if (u.op == "!") {
              return "NOT " + operand;
            }
            return operand;
        }
    }
    return "null";
}

}  // anonymous namespace

// ============================================================================
// SPARQLParser – public API
// ============================================================================

Result<SPARQLASTNode> SPARQLParser::parse(const std::string& sparql_query) {
    try {
        SPARQLLexer lexer(sparql_query);
        auto tokens = lexer.tokenize();
        SPARQLParserImpl impl(std::move(tokens));
        return impl.parse();
    } catch (const std::exception& e) {
        return Err<SPARQLASTNode>(errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                                  std::string(e.what()));
    }
}

// ============================================================================
// SPARQLToAQLTranspiler
// ============================================================================

std::string SPARQLToAQLTranspiler::transpileSelect(const SPARQLSelectStatement& stmt) {
    std::ostringstream oss = {};

    // Maps variable name -> AQL path expression (e.g. "_t0.subject")
    std::map<std::string, std::string> var_bindings;
    int triple_idx = 0;

    // Emit FOR loops and FILTER clauses in declaration order
    for (const auto& clause : stmt.where_clauses) {
        if (clause.kind == SPARQLClauseKind::TriplePattern && clause.triple) {
            const auto& tp    = *clause.triple;
            std::string t_var = "_t" + std::to_string(triple_idx++);

            oss << "FOR " << t_var << " IN " << collection_ << "\n";

            std::vector<std::string> constraints;

            // Subject
            if (tp.subject.type == SPARQLTermType::Variable) {
                auto it = var_bindings.find(tp.subject.value);
                if (it != var_bindings.end()) {
                    constraints.push_back(t_var + ".subject == " + it->second);
                } else {
                    var_bindings[tp.subject.value] = t_var + ".subject";
                }
            } else {
                constraints.push_back(
                    t_var + ".subject == " +
                    termToAQLStr(tp.subject, var_bindings, t_var, "subject"));
            }

            // Predicate
            if (tp.predicate.type == SPARQLTermType::Variable) {
                auto it = var_bindings.find(tp.predicate.value);
                if (it != var_bindings.end()) {
                    constraints.push_back(t_var + ".predicate == " + it->second);
                } else {
                    var_bindings[tp.predicate.value] = t_var + ".predicate";
                }
            } else {
                constraints.push_back(
                    t_var + ".predicate == " +
                    termToAQLStr(tp.predicate, var_bindings, t_var, "predicate"));
            }

            // Object
            if (tp.object.type == SPARQLTermType::Variable) {
                auto it = var_bindings.find(tp.object.value);
                if (it != var_bindings.end()) {
                    constraints.push_back(t_var + ".object == " + it->second);
                } else {
                    var_bindings[tp.object.value] = t_var + ".object";
                }
            } else {
                constraints.push_back(
                    t_var + ".object == " +
                    termToAQLStr(tp.object, var_bindings, t_var, "object"));
            }

            if (!constraints.empty()) {
                oss << "FILTER ";
                for (size_t i = 0; i < constraints.size(); ++i) {
                    if (i > 0) {
                      oss << " AND ";
                    }
                    oss << constraints[i];
                }
                oss << "\n";
            }

        } else if (clause.kind == SPARQLClauseKind::Filter && clause.filter_expr) {
            oss << "FILTER " << filterExprToAQL(*clause.filter_expr, var_bindings) << "\n";
        }
    }

    // ORDER BY
    if (!stmt.order_by.empty()) {
        oss << "SORT ";
        for (size_t i = 0; i < stmt.order_by.size(); ++i) {
            if (i > 0) {
              oss << ", ";
            }
            const auto& spec = stmt.order_by[i];
            auto it = var_bindings.find(spec.variable);
            oss << (it != var_bindings.end() ? it->second : spec.variable);
            oss << (spec.ascending ? " ASC" : " DESC");
        }
        oss << "\n";
    }

    // LIMIT / OFFSET
    if (stmt.limit) {
        if (stmt.offset) {
            oss << "LIMIT " << *stmt.offset << ", " << *stmt.limit << "\n";
        } else {
            oss << "LIMIT " << *stmt.limit << "\n";
        }
    }

    // RETURN
    if (!stmt.star && stmt.variables.size() == 1) {
        // Single variable: return binding directly (or null if unbound)
        auto it = var_bindings.find(stmt.variables[0]);
        oss << "RETURN " << (it != var_bindings.end() ? it->second : "null");
    } else {
        // Multiple variables or SELECT *: return an object
        const std::vector<std::string>* keys = nullptr;
        std::vector<std::string> all_keys;

        if (stmt.star || stmt.variables.empty()) {
            for (const auto& [k, _] : var_bindings) {
              all_keys.push_back(k);
            }
            keys = &all_keys;
        } else {
            keys = &stmt.variables;
        }

        oss << "RETURN {";
        bool first = true;
        for (const auto& var : *keys) {
            if (!first) {
              oss << ", ";
            }
            auto it = var_bindings.find(var);
            oss << var << ": " << (it != var_bindings.end() ? it->second : "null");
            first = false;
        }
        oss << "}";
    }

    return oss.str();
}

Result<std::string> SPARQLToAQLTranspiler::transpile(const SPARQLASTNode& ast) {
    return Ok(transpileSelect(ast.select));
}

}  // namespace query
}  // namespace themis

