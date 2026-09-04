/**
 * @file cypher_parser.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Cypher MATCH/WHERE/RETURN parser and AQL transpiler.
//
// Architecture:
//   CypherParser::parse()
//     └─ Lexer::tokenize()       – produces Token stream
//     └─ Parser (recursive-descent)
//          ├─ parseQuery()       – top-level MATCH … WHERE … RETURN
//          ├─ parsePathPattern() – (node) (-[rel]-> (node))*
//          ├─ parseNodePattern() – (var:Label {props})
//          ├─ parseRelPattern()  – -[var:TYPE*m..n]->
//          ├─ parseExpr()        – WHERE predicate tree
//          └─ parseReturnItems() – RETURN *, n, n.prop AS alias
//
//   CypherToAQLTranspiler::transpile()
//     ├─ emits FOR … IN … loops for each pattern node/rel
//     ├─ emits FILTER clauses from node-property inline filters + WHERE
//     ├─ emits SORT / LIMIT
//     └─ emits RETURN

#include "query/cypher_parser.h"
#include "utils/error_registry.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include "utils/logger.h"

namespace themis {
namespace query {

// ============================================================================
// Token
// ============================================================================

enum class TokenType {
    // Keywords
    KW_MATCH, KW_WHERE, KW_RETURN, KW_ORDER, KW_BY, KW_ASC, KW_DESC,
    KW_LIMIT, KW_SKIP, KW_AS, KW_DISTINCT, KW_AND, KW_OR, KW_NOT,
    KW_NULL, KW_TRUE, KW_FALSE, KW_IN, KW_STARTS, KW_ENDS, KW_WITH,
    KW_CONTAINS, KW_IS,
    // Punctuation
    LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE,
    COLON, COMMA, DOT, PIPE, STAR,
    ARROW_R,   // ->
    ARROW_L,   // <-
    DASH,      // -
    LT, GT, EQ, NEQ, LTE, GTE,
    CARET, SEMI,
    // Literals
    INT_LIT, FLOAT_LIT, STRING_LIT,
    // Identifier
    IDENT,
    // Sentinel
    END_OF_FILE
};

struct CypherParser::Token {
    TokenType type;
    std::string value;
    size_t position = 0;  ///< byte offset in the original query string
};

// ============================================================================
// Lexer
// ============================================================================

struct CypherParser::Lexer {
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
        // Skip single-line comments: // …
        if (pos + 1 < src.size() && src[pos] == '/' && src[pos + 1] == '/') {
            while (pos < src.size() && src[pos] != '\n')
                ++pos;
            skipWhitespace();
        }
        // Skip block comments: /* … */
        if (pos + 1 < src.size() && src[pos] == '/' && src[pos + 1] == '*') {
            pos += 2;
            while (pos + 1 < src.size() && !(src[pos] == '*' && src[pos + 1] == '/'))
                ++pos;
            pos += 2;
            skipWhitespace();
        }
    }

    static TokenType classifyKeyword(const std::string& upper) {
        if (upper == "MATCH") {
          return TokenType::KW_MATCH;
        }
        if (upper == "WHERE") {
          return TokenType::KW_WHERE;
        }
        if (upper == "RETURN") {
          return TokenType::KW_RETURN;
        }
        if (upper == "ORDER") {
          return TokenType::KW_ORDER;
        }
        if (upper == "BY") {
          return TokenType::KW_BY;
        }
        if (upper == "ASC") {
          return TokenType::KW_ASC;
        }
        if (upper == "DESC") {
          return TokenType::KW_DESC;
        }
        if (upper == "LIMIT") {
          return TokenType::KW_LIMIT;
        }
        if (upper == "SKIP") {
          return TokenType::KW_SKIP;
        }
        if (upper == "AS") {
          return TokenType::KW_AS;
        }
        if (upper == "DISTINCT") {
          return TokenType::KW_DISTINCT;
        }
        if (upper == "AND") {
          return TokenType::KW_AND;
        }
        if (upper == "OR") {
          return TokenType::KW_OR;
        }
        if (upper == "NOT") {
          return TokenType::KW_NOT;
        }
        if (upper == "NULL") {
          return TokenType::KW_NULL;
        }
        if (upper == "TRUE") {
          return TokenType::KW_TRUE;
        }
        if (upper == "FALSE") {
          return TokenType::KW_FALSE;
        }
        if (upper == "IN") {
          return TokenType::KW_IN;
        }
        if (upper == "STARTS") {
          return TokenType::KW_STARTS;
        }
        if (upper == "ENDS") {
          return TokenType::KW_ENDS;
        }
        if (upper == "WITH") {
          return TokenType::KW_WITH;
        }
        if (upper == "CONTAINS") {
          return TokenType::KW_CONTAINS;
        }
        if (upper == "IS") {
          return TokenType::KW_IS;
        }
        return TokenType::IDENT;
    }

    std::vector<CypherParser::Token> tokenize() {
        std::vector<CypherParser::Token> tokens = {};

        tokens.reserve(src.size());

        while (true) {
            skipWhitespace();
            if (pos >= static_cast<int>(src.size())) {
              break;
            }

            CypherParser::Token tok;
            tok.position = pos;

            char ch = peek();

            // --- String literal ---
            if (ch == '\'' || ch == '"') {
                char delim = advance();
                std::string s = {};
                while (pos < src.size() && peek() != delim) {
                    char c = advance();
                    if (c == '\\' && pos < src.size()) {
                        char esc = advance();
                        switch (esc) {
                            case 'n':  s += '\n'; break;
                            case 't':  s += '\t'; break;
                            case 'r':  s += '\r'; break;
                            default:   s += esc;  break;
                        }
                    } else {
                        s += c;
                    }
                }
                if (static_cast<int>(src.size()) > pos) advance();  // closing delimiter
                tok.type  = TokenType::STRING_LIT;
                tok.value = std::move(s);
                tokens.push_back(std::move(tok));
                continue;
            }

            // --- Number literal ---
            if (std::isdigit(static_cast<unsigned char>(ch)) ||
                (ch == '-' && std::isdigit(static_cast<unsigned char>(peek(1))))) {
                std::string num = {};
                if (ch == '-') {
                  num += advance();
                }
                bool is_float = false;
                while (static_cast<size_t>(pos) < src.size()) {
                    if (std::isdigit(static_cast<unsigned char>(peek()))) {
                        num += advance();
                        continue;
                    }
                    if (peek() == '.' && !is_float &&
                        std::isdigit(static_cast<unsigned char>(peek(1)))) {
                        is_float = true;
                        num += advance();
                        continue;
                    }
                    break;
                }
                // Scientific notation
                if (pos < src.size() && (peek() == 'e' || peek() == 'E')) {
                    is_float = true;
                    num += advance();
                    if (pos < src.size() && (peek() == '+' || peek() == '-'))
                        num += advance();
                    while (pos < src.size() && std::isdigit(static_cast<unsigned char>(peek())))
                        num += advance();
                }
                tok.type  = is_float ? TokenType::FLOAT_LIT : TokenType::INT_LIT;
                tok.value = std::move(num);
                tokens.push_back(std::move(tok));
                continue;
            }

            // --- Identifier or keyword ---
            if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
                std::string id = {};
                while (pos < src.size() &&
                       (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
                    id += advance();
                std::string upper = id;
                std::transform(upper.begin(), upper.end(), upper.begin(),
                               [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
                tok.type  = classifyKeyword(upper);
                tok.value = std::move(id);
                tokens.push_back(std::move(tok));
                continue;
            }

            // --- Backtick-quoted identifier ---
            if (ch == '`') {
                advance();
                std::string id = {};
                while (pos < src.size() && peek() != '`')
                    id += advance();
                if (static_cast<int>(src.size()) > pos) {
                  advance();
                }
                tok.type  = TokenType::IDENT;
                tok.value = std::move(id);
                tokens.push_back(std::move(tok));
                continue;
            }

            // --- Multi-char operators and punctuation ---
            advance();  // consume `ch`
            switch (ch) {
                case '(':  tok.type = TokenType::LPAREN;    tok.value = "("; break;
                case ')':  tok.type = TokenType::RPAREN;    tok.value = ")"; break;
                case '[':  tok.type = TokenType::LBRACKET;  tok.value = "["; break;
                case ']':  tok.type = TokenType::RBRACKET;  tok.value = "]"; break;
                case '{':  tok.type = TokenType::LBRACE;    tok.value = "{"; break;
                case '}':  tok.type = TokenType::RBRACE;    tok.value = "}"; break;
                case ':':  tok.type = TokenType::COLON;     tok.value = ":"; break;
                case ',':  tok.type = TokenType::COMMA;     tok.value = ","; break;
                case '.':  tok.type = TokenType::DOT;       tok.value = "."; break;
                case '|':  tok.type = TokenType::PIPE;      tok.value = "|"; break;
                case '*':  tok.type = TokenType::STAR;      tok.value = "*"; break;
                case '^':  tok.type = TokenType::CARET;     tok.value = "^"; break;
                case ';':  tok.type = TokenType::SEMI;      tok.value = ";"; break;
                case '=':  tok.type = TokenType::EQ;        tok.value = "="; break;
                case '+':  tok.type = TokenType::IDENT;     tok.value = "+"; break;  // handled inline
                case '<':
                    if (peek() == '=') { advance(); tok.type = TokenType::LTE; tok.value = "<="; }
                    else if (peek() == '>') { advance(); tok.type = TokenType::NEQ; tok.value = "<>"; }
                    else if (peek() == '-') { advance(); tok.type = TokenType::ARROW_L; tok.value = "<-"; }
                    else { tok.type = TokenType::LT; tok.value = "<"; }
                    break;
                case '>':
                    if (peek() == '=') { advance(); tok.type = TokenType::GTE; tok.value = ">="; }
                    else { tok.type = TokenType::GT; tok.value = ">"; }
                    break;
                case '-':
                    if (peek() == '>') { advance(); tok.type = TokenType::ARROW_R; tok.value = "->"; }
                    else { tok.type = TokenType::DASH; tok.value = "-"; }
                    break;
                default:
                    // Skip unknown characters silently
                    continue;
            }
            tokens.push_back(std::move(tok));
        }

        CypherParser::Token eof;
        eof.type     = TokenType::END_OF_FILE;
        eof.value    = "";
        eof.position = pos;
        tokens.push_back(std::move(eof));
        return tokens;
    }
};

// ============================================================================
// Recursive-descent parser
// ============================================================================

struct CypherParser::Parser {
    std::vector<CypherParser::Token> tokens;
    size_t cursor = 0;

    explicit Parser(std::vector<CypherParser::Token> toks)
        : tokens(std::move(toks)) {}

    // Collapse token-boundary spaces around dots: "n . prop" → "n.prop"
    static std::string collapseDotSpaces(const std::string& s) {
        std::string out = {};
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ) {
            if (i + 2 < s.size() &&
                s[i] == ' ' && s[i + 1] == '.' && s[i + 2] == ' ') {
                out += '.';
                i += 3;
            } else {
                out += s[i++];
            }
        }
        return out;
    }

    // ---- Token helpers -------------------------------------------------------

    const CypherParser::Token& current() const {
        return tokens[cursor < tokens.size() ? cursor : tokens.size() - 1];
    }

    const CypherParser::Token& peek([[maybe_unused]] size_t offset = 1) const {
        size_t idx = cursor + offset;
        return tokens[idx < tokens.size() ? idx : tokens.size() - 1];
    }

    bool check(TokenType t) const { return current().type == t; }

    bool match(TokenType t) {
        if (check(t)) { ++cursor; return true; }
        return false;
    }

    // Advance and return the consumed token; throw on mismatch.
    const CypherParser::Token& expect(TokenType t, const std::string& msg) {
        if (!check(t)) {
            throw CypherParseError{
                msg + " (got '" + current().value + "')",
                current().position
            };
        }
        return tokens[cursor++];
    }

    std::string expectIdent(const std::string& ctx) {
        if (!check(TokenType::IDENT)) {
            throw CypherParseError{
                "Expected identifier " + ctx + " (got '" + current().value + "')",
                current().position
            };
        }
        return tokens[cursor++].value;
    }

    bool isAtEnd() const { return check(TokenType::END_OF_FILE); }

    // Upper-case helper
    static std::string toUpper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
        return s;
    }

    // ---- Grammar rules -------------------------------------------------------

    CypherASTNode parseQuery() {
        CypherASTNode ast;

        // Optional leading semicolons / whitespace
        while (match(TokenType::SEMI)) {}

        expect(TokenType::KW_MATCH, "Expected MATCH keyword");

        // One or more comma-separated path patterns
        ast.match_patterns.push_back(parsePathPattern());
        while (match(TokenType::COMMA))
            ast.match_patterns.push_back(parsePathPattern());

        // Optional WHERE
        if (match(TokenType::KW_WHERE))
            ast.where = parseExpr();

        // RETURN
        expect(TokenType::KW_RETURN, "Expected RETURN keyword");

        if (match(TokenType::KW_DISTINCT))
            ast.return_distinct = true;

        parseReturnItems(ast);

        // ORDER BY
        if (check(TokenType::KW_ORDER)) {
            ++cursor;
            expect(TokenType::KW_BY, "Expected BY after ORDER");
            parseSortSpecs(ast);
        }

        // SKIP
        if (match(TokenType::KW_SKIP)) {
            const auto& t = expect(TokenType::INT_LIT, "Expected integer after SKIP");
            try {
                ast.skip = std::stoll(t.value);
            } catch (const std::out_of_range&) {
                THEMIS_WARN("cypher_parser::parseQuery: SKIP value overflow '{}'", t.value);
                throw CypherParseError{"SKIP value '" + t.value + "' is out of integer range", t.position};
            } catch (const std::invalid_argument&) {
                THEMIS_WARN("cypher_parser::parseQuery: SKIP value '{}' is not a valid integer", t.value);
                throw CypherParseError{"SKIP value '" + t.value + "' is not a valid integer", t.position};
            }
        }
 
        // LIMIT
        if (match(TokenType::KW_LIMIT)) {
            const auto& t = expect(TokenType::INT_LIT, "Expected integer after LIMIT");
            try {
                ast.limit = std::stoll(t.value);
            } catch (const std::out_of_range&) {
                THEMIS_WARN("cypher_parser::parseQuery: LIMIT value overflow '{}'", t.value);
                throw CypherParseError{"LIMIT value '" + t.value + "' is out of integer range", t.position};
            } catch (const std::invalid_argument&) {
                THEMIS_WARN("cypher_parser::parseQuery: LIMIT value '{}' is not a valid integer", t.value);
                throw CypherParseError{"LIMIT value '" + t.value + "' is not a valid integer", t.position};
            }
        }

        // Allow trailing semicolon
        match(TokenType::SEMI);

        if (!isAtEnd()) {
            throw CypherParseError{
                "Unexpected token '" + current().value + "' after query",
                current().position
            };
        }

        return ast;
    }

    // path_pattern := node_pattern (rel_pattern node_pattern)*
    CypherPathPattern parsePathPattern() {
        CypherPathPattern path;
        path.start = parseNodePattern();

        while (check(TokenType::DASH) || check(TokenType::ARROW_L)) {
            CypherPathSegment seg;
            seg.rel  = parseRelPattern();
            seg.node = parseNodePattern();
            path.segments.push_back(std::move(seg));
        }

        return path;
    }

    // node_pattern := LPAREN [ident] [:label]* [{props}] RPAREN
    CypherNodePattern parseNodePattern() {
        expect(TokenType::LPAREN, "Expected '(' for node pattern");
        CypherNodePattern node;

        // Optional variable
        if (check(TokenType::IDENT))
            node.variable = tokens[cursor++].value;

        // Labels: (:Label)*
        while (match(TokenType::COLON)) {
            if (!check(TokenType::IDENT)) {
                throw CypherParseError{
                    "Expected label name after ':'",
                    current().position
                };
            }
            node.labels.push_back(tokens[cursor++].value);
        }

        // Inline property map: { key: value, … }
        if (match(TokenType::LBRACE))
            node.properties = parsePropMap();

        expect(TokenType::RPAREN, "Expected ')' to close node pattern");
        return node;
    }

    // prop_map := key COLON literal (COMMA key COLON literal)*
    std::vector<CypherPropertyFilter> parsePropMap() {
        std::vector<CypherPropertyFilter> props = {};

        if (check(TokenType::RBRACE)) {
            ++cursor;
            return props;
        }
        do {
            CypherPropertyFilter f;
            f.key = expectIdent("as property key");
            expect(TokenType::COLON, "Expected ':' between property key and value");
            f.value = parseLiteralValue();
            props.push_back(std::move(f));
        } while (match(TokenType::COMMA));
        expect(TokenType::RBRACE, "Expected closing brace to close property map");
        return props;
    }

    CypherLiteralValue parseLiteralValue() {
        if (match(TokenType::KW_NULL)) {
          return nullptr;
        }
        if (match(TokenType::KW_TRUE)) {
          return true;
        }
        if (match(TokenType::KW_FALSE)) {
          return false;
        }
 
        if (check(TokenType::INT_LIT)) {
            int64_t v;
            try {
                v = std::stoll(current().value);
            } catch (const std::out_of_range&) {
                THEMIS_WARN("cypher_parser::parseLiteralValue: integer overflow '{}'", current().value);
                throw CypherParseError{"Integer literal '" + current().value + "' is out of range",
                                       current().position};
            } catch (const std::invalid_argument&) {
                THEMIS_WARN("cypher_parser::parseLiteralValue: invalid integer '{}'", current().value);
                throw CypherParseError{"Integer literal '" + current().value + "' is not a valid integer",
                                       current().position};
            }
            ++cursor;
            return v;
        }
        if (check(TokenType::FLOAT_LIT)) {
            double v = 0;
            try {
                v = std::stod(current().value);
            } catch (const std::out_of_range&) {
                THEMIS_WARN("cypher_parser::parseLiteralValue: float overflow '{}'", current().value);
                throw CypherParseError{"Float literal '" + current().value + "' is out of range",
                                       current().position};
            } catch (const std::invalid_argument&) {
                THEMIS_WARN("cypher_parser::parseLiteralValue: invalid float '{}'", current().value);
                throw CypherParseError{"Float literal '" + current().value + "' is not a valid float",
                                       current().position};
            }
            ++cursor;
            return v;
        }
        if (check(TokenType::STRING_LIT)) {
            std::string v = current().value;
            ++cursor;
            return v;
        }
        throw CypherParseError{
            "Expected literal value (got '" + current().value + "')",
            current().position
        };
    }

    // rel_pattern:
    //   -[var:TYPE*m..n]->   (Out)
    //   <-[var:TYPE*m..n]-   (In)
    //   -[var:TYPE*m..n]-    (Both)
    CypherRelPattern parseRelPattern() {
        CypherRelPattern rel;

        bool left_arrow = match(TokenType::ARROW_L);  // <-
        if (!left_arrow)
            expect(TokenType::DASH, "Expected '-' to start relationship pattern");

        if (!check(TokenType::LBRACKET)) {
            if (left_arrow) {
                expect(TokenType::DASH, "Expected '-' after '<-' in anonymous relationship");
                rel.direction = CypherRelDirection::In;
            } else if (match(TokenType::ARROW_R)) {
                rel.direction = CypherRelDirection::Out;
            } else {
                expect(TokenType::DASH, "Expected '-' or '->' in anonymous relationship");
                rel.direction = CypherRelDirection::Both;
            }
            return rel;
        }

        expect(TokenType::LBRACKET, "Expected '[' for relationship pattern");

        // Optional variable
        if (check(TokenType::IDENT))
            rel.variable = tokens[cursor++].value;

        // Types: :TYPE (| TYPE)*
        if (match(TokenType::COLON)) {
            if (!check(TokenType::IDENT)) {
                throw CypherParseError{
                    "Expected relationship type after ':'",
                    current().position
                };
            }
            rel.types.push_back(tokens[cursor++].value);
            while (match(TokenType::PIPE)) {
                rel.types.push_back(expectIdent("as relationship type"));
            }
        }

        // Variable-length: *min..max
        if (match(TokenType::STAR)) {
            // Enforce an upper bound on hop counts to prevent BFS/DFS DoS.
            static constexpr int kMaxHops = 1000;
            // min
            if (check(TokenType::INT_LIT)) {
                int hops = 0;
                try {
                    hops = std::stoi(current().value);
                } catch (...) {
                    THEMIS_WARN("cypher_parser::parseRelPattern: unhandled exception caught");
                    throw CypherParseError{"Hop count out of integer range", current().position};
                }
                if (hops < 0 || hops > kMaxHops) {
                    throw CypherParseError{
                        "Cypher hop count " + current().value +
                        " is out of valid range [0, " + std::to_string(kMaxHops) + "]",
                        current().position};
                }
                rel.min_hops = hops;
                ++cursor;
            }
            // ..
            if (check(TokenType::DOT) && peek().type == TokenType::DOT) {
                cursor += 2;  // consume both dots
                if (check(TokenType::INT_LIT)) {
                    int hops = 0;
                    try {
                        hops = std::stoi(current().value);
                    } catch (...) {
                        THEMIS_WARN("cypher_parser: unhandled exception caught");
                        throw CypherParseError{"Hop count out of integer range", current().position};
                    }
                    if (hops < 0 || hops > kMaxHops) {
                        throw CypherParseError{
                            "Cypher hop count " + current().value +
                            " is out of valid range [0, " + std::to_string(kMaxHops) + "]",
                            current().position};
                    }
                    rel.max_hops = hops;
                    ++cursor;
                }
            }
        }

        expect(TokenType::RBRACKET, "Expected ']' to close relationship pattern");

        if (left_arrow) {
            // <-[…]-
            expect(TokenType::DASH, "Expected '-' after relationship bracket for '<-[…]-'");
            rel.direction = CypherRelDirection::In;
        } else if (match(TokenType::ARROW_R)) {
            // -[…]->
            rel.direction = CypherRelDirection::Out;
        } else {
            // -[…]-
            expect(TokenType::DASH, "Expected '-' or '->' after relationship bracket");
            rel.direction = CypherRelDirection::Both;
        }

        return rel;
    }

    // ---- Expression parsing --------------------------------------------------
    // expr      := or_expr
    // or_expr   := and_expr (OR and_expr)*
    // and_expr  := not_expr (AND not_expr)*
    // not_expr  := [NOT] is_expr
    // is_expr   := comparison [IS [NOT] NULL]
    // comparison:= additive [op additive]
    // additive  := primary
    // primary   := ident DOT ident
    //            | literal
    //            | ident [IN list_literal | STARTS WITH | ENDS WITH | CONTAINS]
    //            | LPAREN expr RPAREN

    std::shared_ptr<CypherExpr> parseExpr() {
        return parseOrExpr();
    }

    std::shared_ptr<CypherExpr> parseOrExpr() {
        auto left = parseAndExpr();
        while (match(TokenType::KW_OR)) {
            auto right = parseAndExpr();
            left = std::make_shared<CypherBinaryOpExpr>("OR", std::move(left), std::move(right));
        }
        return left;
    }

    std::shared_ptr<CypherExpr> parseAndExpr() {
        auto left = parseNotExpr();
        while (match(TokenType::KW_AND)) {
            auto right = parseNotExpr();
            left = std::make_shared<CypherBinaryOpExpr>("AND", std::move(left), std::move(right));
        }
        return left;
    }

    std::shared_ptr<CypherExpr> parseNotExpr() {
        if (match(TokenType::KW_NOT)) {
            auto operand = parseIsExpr();
            return std::make_shared<CypherUnaryOpExpr>("NOT", std::move(operand));
        }
        return parseIsExpr();
    }

    std::shared_ptr<CypherExpr> parseIsExpr() {
        auto expr = parseComparison();
        // IS NULL | IS NOT NULL
        if (match(TokenType::KW_IS)) {
            bool negated = match(TokenType::KW_NOT);
            if (!check(TokenType::KW_NULL)) {
                throw CypherParseError{
                    "Expected NULL after IS [NOT]",
                    current().position
                };
            }
            ++cursor;
            std::string op = negated ? "IS NOT NULL" : "IS NULL";
            return std::make_shared<CypherUnaryOpExpr>(std::move(op), std::move(expr));
        }
        return expr;
    }

    std::shared_ptr<CypherExpr> parseComparison() {
        auto left = parsePrimary();

        // Binary comparison operators
        static const std::pair<TokenType, std::string> cmp_ops[] = {
            { TokenType::EQ,  "=" },
            { TokenType::NEQ, "<>" },
            { TokenType::LT,  "<" },
            { TokenType::LTE, "<=" },
            { TokenType::GT,  ">" },
            { TokenType::GTE, ">=" },
        };
        for (const auto& [tt, op] : cmp_ops) {
            if (match(tt)) {
                auto right = parsePrimary();
                return std::make_shared<CypherBinaryOpExpr>(op, std::move(left), std::move(right));
            }
        }

        // IN  /  NOT IN
        if (match(TokenType::KW_IN)) {
            auto right = parsePrimary();
            return std::make_shared<CypherBinaryOpExpr>("IN", std::move(left), std::move(right));
        }
        if (check(TokenType::KW_NOT) && peek().type == TokenType::KW_IN) {
            cursor += 2;
            auto right = parsePrimary();
            return std::make_shared<CypherBinaryOpExpr>("NOT IN", std::move(left), std::move(right));
        }

        // STARTS WITH
        if (check(TokenType::KW_STARTS) && peek().type == TokenType::KW_WITH) {
            cursor += 2;
            auto right = parsePrimary();
            return std::make_shared<CypherBinaryOpExpr>("STARTS WITH", std::move(left), std::move(right));
        }

        // ENDS WITH
        if (check(TokenType::KW_ENDS) && peek().type == TokenType::KW_WITH) {
            cursor += 2;
            auto right = parsePrimary();
            return std::make_shared<CypherBinaryOpExpr>("ENDS WITH", std::move(left), std::move(right));
        }

        // CONTAINS
        if (match(TokenType::KW_CONTAINS)) {
            auto right = parsePrimary();
            return std::make_shared<CypherBinaryOpExpr>("CONTAINS", std::move(left), std::move(right));
        }

        return left;
    }

    std::shared_ptr<CypherExpr> parsePrimary() {
        // Parenthesised sub-expression
        if (match(TokenType::LPAREN)) {
            auto e = parseExpr();
            expect(TokenType::RPAREN, "Expected ')' to close expression");
            return e;
        }

        if (match(TokenType::LBRACKET)) {
            std::string list_literal = "[";
            bool first = true;
            while (!check(TokenType::RBRACKET)) {
                if (!first) {
                    expect(TokenType::COMMA, "Expected ',' between list literal elements");
                    list_literal += ", ";
                }

                if (check(TokenType::STRING_LIT)) {
                    list_literal += '"' + current().value + '"';
                    ++cursor;
                } else if (check(TokenType::INT_LIT) || check(TokenType::FLOAT_LIT) ||
                           check(TokenType::KW_TRUE) || check(TokenType::KW_FALSE) ||
                           check(TokenType::KW_NULL)) {
                    list_literal += current().value;
                    ++cursor;
                } else {
                    throw CypherParseError{
                        "Expected literal value inside list expression",
                        current().position
                    };
                }
                first = false;
            }
            expect(TokenType::RBRACKET, "Expected ']' to close list expression");
            return std::make_shared<CypherLiteralExpr>(std::move(list_literal));
        }

        // NULL / TRUE / FALSE
        if (match(TokenType::KW_NULL)) {
          return std::make_shared<CypherLiteralExpr>(nullptr);
        }
        if (match(TokenType::KW_TRUE)) {
          return std::make_shared<CypherLiteralExpr>(true);
        }
        if (match(TokenType::KW_FALSE)) {
          return std::make_shared<CypherLiteralExpr>(false);
        }

        // Numeric / string literals
        if (check(TokenType::INT_LIT)) {
            int64_t v;
            try {
                v = std::stoll(current().value);
            } catch (...) {
                THEMIS_WARN("cypher_parser::parsePrimary: unhandled exception caught");
                throw CypherParseError{"Integer literal '" + current().value + "' is out of range",
                                       current().position};
            }
            ++cursor;
            return std::make_shared<CypherLiteralExpr>(v);
        }
        if (check(TokenType::FLOAT_LIT)) {
            double v = 0;
            try {
                v = std::stod(current().value);
            } catch (...) {
                THEMIS_WARN("cypher_parser::parsePrimary: unhandled exception caught");
                throw CypherParseError{"Float literal '" + current().value + "' is out of range",
                                       current().position};
            }
            ++cursor;
            return std::make_shared<CypherLiteralExpr>(v);
        }
        if (check(TokenType::STRING_LIT)) {
            std::string v = current().value;
            ++cursor;
            return std::make_shared<CypherLiteralExpr>(std::move(v));
        }

        // Identifier — may be var.prop or just var
        if (check(TokenType::IDENT)) {
            std::string first = tokens[cursor++].value;
            if (match(TokenType::DOT)) {
                std::string prop = expectIdent("as property name");
                return std::make_shared<CypherPropertyExpr>(std::move(first), std::move(prop));
            }
            // Bare identifier treated as a string literal (e.g., label reference)
            return std::make_shared<CypherLiteralExpr>(std::move(first));
        }

        throw CypherParseError{
            "Unexpected token '" + current().value + "' in expression",
            current().position
        };
    }

    // ---- RETURN items --------------------------------------------------------

    // return_item := STAR | (expr [AS ident])
    // The raw expression text is captured by re-serialising the expr tree.
    void parseReturnItems(CypherASTNode& ast) {
        // RETURN *
        if (check(TokenType::STAR)) {
            ++cursor;
            CypherReturnItem item;
            item.star = true;
            ast.return_items.push_back(std::move(item));
            return;
        }

        do {
            CypherReturnItem item;
            item.star     = false;
            item.distinct = ast.return_distinct;

            // Capture the raw expression text for the return item.
            // We peek-ahead and collect token text until a structural stop.
            size_t start = cursor;
            auto expr = parseExpr();  // parse and discard the AST node (we only need the text)
            // Reconstruct expression text from token values, collapsing "n . prop" → "n.prop"
            std::string expr_text = {};
            for (size_t i = start; i < cursor; ++i) {
                if (i > start) {
                  expr_text += " ";
                }
                expr_text += tokens[i].value;
            }
            item.expression = collapseDotSpaces(expr_text);
            // expression AST built above; text is what we store

            if (match(TokenType::KW_AS))
                item.alias = expectIdent("as return alias");

            ast.return_items.push_back(std::move(item));
        } while (match(TokenType::COMMA));
    }

    // ---- ORDER BY ------------------------------------------------------------

    void parseSortSpecs(CypherASTNode& ast) {
        do {
            CypherSortSpec spec;
            size_t start = cursor;
            parseExpr();
            std::string expr_text = {};
            for (size_t i = start; i < cursor; ++i) {
                if (i > start) {
                  expr_text += " ";
                }
                expr_text += tokens[i].value;
            }
            spec.expression = collapseDotSpaces(expr_text);
            spec.ascending  = true;
            if (match(TokenType::KW_ASC)) {
              spec.ascending = true;
            }
            if (match(TokenType::KW_DESC)) {
              spec.ascending = false;
            }
            ast.order_by.push_back(std::move(spec));
        } while (match(TokenType::COMMA));
    }
};

// ============================================================================
// CypherParser::parse  – public entry point
// ============================================================================

Result<CypherASTNode> CypherParser::parse(const std::string& cypher_query) {
    try {
        Lexer lex(cypher_query);
        auto tokens = lex.tokenize();

        Parser parser(std::move(tokens));
        auto ast = parser.parseQuery();
        return Ok(std::move(ast));
    } catch (const CypherParseError& e) {
        return Err<CypherASTNode>(errors::ErrorCode::ERR_QUERY_PARSE_FAILED, e.toString());
    } catch (const std::exception& e) {
        return Err<CypherASTNode>(errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                                  std::string("Internal parse error: ") + e.what());
    }
}

// ============================================================================
// CypherToAQLTranspiler – helpers
// ============================================================================

/*static*/
std::string CypherToAQLTranspiler::literalToAQL(const CypherLiteralValue& val) {
    return std::visit([](const auto& v) -> std::string {
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
            // std::string – escape inner double quotes
            if (v.size() >= 2 && v.front() == '[' && v.back() == ']') {
                return v;
            }
            std::string out = {};
            out.reserve(v.size() + 2);
            out += '"';
            for (char c : v) {
                if (c == '"' || c == '\\') {
                  out += '\\';
                }
                out += c;
            }
            out += '"';
            return out;
        }
    }, val);
}

/*static*/
std::string CypherToAQLTranspiler::exprToAQL(const CypherExpr& expr,
                                               const std::string& default_var) {
    switch (expr.exprType()) {
        case CypherExprType::Literal: {
            const auto& lit = static_cast<const CypherLiteralExpr&>(expr);
            return literalToAQL(lit.value);
        }
        case CypherExprType::Property: {
            const auto& prop = static_cast<const CypherPropertyExpr&>(expr);
            return prop.variable + "." + prop.property;
        }
        case CypherExprType::BinaryOp: {
            const auto& bin = static_cast<const CypherBinaryOpExpr&>(expr);
            std::string left  = exprToAQL(*bin.left,  default_var);
            std::string right = exprToAQL(*bin.right, default_var);

            // Map Cypher operators to AQL
            const std::string& op = bin.op;
            if (op == "AND") {
              return "(" + left + " AND " + right + ")";
            }
            if (op == "OR") {
              return "(" + left + " OR "  + right + ")";
            }
            if (op == "IN") {
              return left + " IN " + right;
            }
            if (op == "NOT IN") {
              return left + " NOT IN " + right;
            }
            if (op == "STARTS WITH") {
              return "STARTS_WITH(" + left + ", " + right + ")";
            }
            if (op == "ENDS WITH") {
              return "ENDS_WITH(" + left + ", " + right + ")";
            }
            if (op == "CONTAINS") {
              return "CONTAINS(" + left + ", " + right + ")";
            }
            // Comparison ops pass through: =, <>, <, <=, >, >=
            // AQL uses == for equality
            if (op == "=") {
              return left + " == " + right;
            }
            if (op == "<>") {
              return left + " != " + right;
            }
            return left + " " + op + " " + right;
        }
        case CypherExprType::UnaryOp: {
            const auto& un = static_cast<const CypherUnaryOpExpr&>(expr);
            std::string operand = exprToAQL(*un.operand, default_var);
            if (un.op == "NOT") {
              return "!(" + operand + ")";
            }
            if (un.op == "IS NULL") {
              return operand + " == null";
            }
            if (un.op == "IS NOT NULL") {
              return operand + " != null";
            }
            return un.op + "(" + operand + ")";
        }
    }
    return "";
}

/*static*/
std::string CypherToAQLTranspiler::nodePatternToFilter(const CypherNodePattern& node,
                                                        const std::string& var) {
    std::string filter = {};
    for (const auto& prop : node.properties) {
        if (!filter.empty()) {
          filter += " AND ";
        }
        filter += var + "." + prop.key + " == " + literalToAQL(prop.value);
    }
    return filter;
}

// ============================================================================
// CypherToAQLTranspiler::transpile
// ============================================================================

Result<std::string> CypherToAQLTranspiler::transpile(const CypherASTNode& ast) {
    try {
        if (ast.match_patterns.empty()) {
            return Err<std::string>(errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                                    "Query has no MATCH patterns");
        }

        std::ostringstream aql = {};

        // ----------------------------------------------------------------
        // Collect all bound variables (for RETURN * expansion)
        // ----------------------------------------------------------------
        std::vector<std::string> all_vars;
        auto addVar = [&]([[maybe_unused]] const std::string& v) {
            if (!v.empty() &&
                std::find(all_vars.begin(), all_vars.end(), v) == all_vars.end())
                all_vars.push_back(v);
        };

        for (const auto& path : ast.match_patterns) {
            addVar(path.start.variable);
            for (const auto& seg : path.segments) {
                addVar(seg.rel.variable);
                addVar(seg.node.variable);
            }
        }

        // ----------------------------------------------------------------
        // FOR loops – one per node; graph traversal for relationships
        // ----------------------------------------------------------------
        std::vector<std::string> filter_clauses;

        for (const auto& path : ast.match_patterns) {
            const auto& start = path.start;

            // Determine the AQL collection name from the first label, falling
            // back to the variable name, then to "Documents".
            std::string start_collection =
                start.labels.empty() ? (start.variable.empty() ? "Documents" : start.variable)
                                     : start.labels[0];
            std::string start_var =
                start.variable.empty() ? "_anon_start" : start.variable;

            aql << "FOR " << start_var << " IN " << start_collection << "\n";

            // Inline property filters on the start node
            std::string node_filter = nodePatternToFilter(start, start_var);
            if (!node_filter.empty())
                filter_clauses.push_back(std::move(node_filter));

            // Path segments (relationships)
            for (const auto& seg : path.segments) {
                const auto& rel  = seg.rel;
                const auto& dest = seg.node;

                // Direction keyword
                std::string dir_kw = {};
                switch (rel.direction) {
                    case CypherRelDirection::Out:  dir_kw = "OUTBOUND"; break;
                    case CypherRelDirection::In:   dir_kw = "INBOUND";  break;
                    case CypherRelDirection::Both: dir_kw = "ANY";      break;
                }

                // Hop count
                int min_h = rel.min_hops.value_or(1);
                int max_h = rel.max_hops.value_or(rel.min_hops.value_or(1));

                // Edge variable
                std::string edge_var = rel.variable.empty() ? "_e" : rel.variable;

                // Destination variable
                std::string dest_var =
                    dest.variable.empty() ? "_anon_dest" : dest.variable;

                // Graph name – use first type if given, else generic "graph"
                std::string graph_name =
                    rel.types.empty() ? "graph" : rel.types[0];

                aql << "FOR " << edge_var << ", " << dest_var
                    << " IN " << min_h << ".." << max_h
                    << " " << dir_kw << " " << start_var
                    << " GRAPH \"" << graph_name << "\"\n";

                // Multi-type filter: e._type IN ["T1","T2",…]
                if (rel.types.size() > 1) {
                    std::string type_list = {};
                    for (size_t i = 0; i < rel.types.size(); ++i) {
                        if (i) {
                          type_list += ", ";
                        }
                        type_list += "\"" + rel.types[i] + "\"";
                    }
                    filter_clauses.push_back(
                        edge_var + "._type IN [" + type_list + "]");
                }

                // Inline property filters on the destination node
                std::string dest_filter = nodePatternToFilter(dest, dest_var);
                if (!dest_filter.empty())
                    filter_clauses.push_back(std::move(dest_filter));
            }
        }

        // ----------------------------------------------------------------
        // FILTER from WHERE expression
        // ----------------------------------------------------------------
        if (ast.where) {
            std::string first_var =
                (!all_vars.empty() ? all_vars[0] : "");
            filter_clauses.push_back(exprToAQL(*ast.where, first_var));
        }

        for (const auto& f : filter_clauses)
            aql << "FILTER " << f << "\n";

        // ----------------------------------------------------------------
        // SORT
        // ----------------------------------------------------------------
        if (!ast.order_by.empty()) {
            aql << "SORT ";
            for (size_t i = 0; i < ast.order_by.size(); ++i) {
                if (i) {
                  aql << ", ";
                }
                aql << ast.order_by[i].expression
                    << (ast.order_by[i].ascending ? " ASC" : " DESC");
            }
            aql << "\n";
        }

        // ----------------------------------------------------------------
        // LIMIT / SKIP
        // ----------------------------------------------------------------
        if (ast.skip.has_value() || ast.limit.has_value()) {
            int64_t sk = ast.skip.value_or(0);
            if (ast.limit.has_value()) {
                aql << "LIMIT " << sk << ", " << ast.limit.value() << "\n";
            } else {
                // SKIP-only: use a very large sentinel limit
                aql << "LIMIT " << sk << ", 2147483647\n";
            }
        }

        // ----------------------------------------------------------------
        // RETURN
        // ----------------------------------------------------------------
        aql << "RETURN ";

        if (ast.return_distinct)
            aql << "DISTINCT ";

        // RETURN *
        if (!ast.return_items.empty() && ast.return_items[0].star) {
            if (all_vars.empty()) {
                aql << "{}\n";
            } else if (all_vars.size() == 1) {
                aql << all_vars[0] << "\n";
            } else {
                aql << "{";
                for (size_t i = 0; i < all_vars.size(); ++i) {
                    if (i) {
                      aql << ", ";
                    }
                    aql << all_vars[i] << ": " << all_vars[i];
                }
                aql << "}\n";
            }
        } else if (!ast.return_items.empty()) {
            // Multiple items → wrap in an object; single item → return directly.
            if (ast.return_items.size() == 1) {
                const auto& item = ast.return_items[0];
                aql << (!item.alias.empty() ? item.alias : item.expression) << "\n";
            } else {
                aql << "{";
                for (size_t i = 0; i < ast.return_items.size(); ++i) {
                    if (i) {
                      aql << ", ";
                    }
                    const auto& item = ast.return_items[i];
                    // Key: prefer alias, else last component of "n.prop"
                    std::string key = item.alias;
                    if (key.empty()) {
                        const std::string& expr = item.expression;
                        size_t dot = expr.rfind('.');
                        key = (dot != std::string::npos) ? expr.substr(dot + 1) : expr;
                    }
                    aql << key << ": " << item.expression;
                }
                aql << "}\n";
            }
        } else {
            // Fallback: return first variable
            aql << (all_vars.empty() ? "null" : all_vars[0]) << "\n";
        }

        return Ok(aql.str());

    } catch (const std::exception& e) {
        return Err<std::string>(errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                                std::string("Transpilation error: ") + e.what());
    }
}

}  // namespace query
}  // namespace themis

