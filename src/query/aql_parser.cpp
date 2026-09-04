/**
 * @file aql_parser.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/aql_parser.h"
#include "utils/error_registry.h"
#include <cctype>
#include <optional>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <fmt/format.h>
#include "utils/logger.h"

namespace themis {
namespace query {

// ============================================================================
// ParserScopeContext Implementation (Phase 2 Agent 1)
// ============================================================================

void ParserScopeContext::registerCollection(const std::string& collection_name) {
    if (collection_name.empty()) {
        return;  // Silently ignore empty collection names
    }

    // Extract scope namespace prefix (the part before the first dot, if any).
    // Collections like "scope1.table1" carry an explicit scope prefix.
    const auto dot_pos = collection_name.find('.');
    if (dot_pos != std::string::npos) {
        std::string prefix = collection_name.substr(0, dot_pos);
        if (!current_scope_prefix_.empty() && current_scope_prefix_ != prefix) {
            throw std::runtime_error(
                fmt::format("Parser error: scope mismatch — mix of scope '{}' and scope '{}' "
                            "is not allowed within a single query",
                            current_scope_prefix_, prefix));
        }
        current_scope_prefix_ = std::move(prefix);
    }

    registered_collections_.insert(collection_name);
}

bool ParserScopeContext::isCollectionInScope(const std::string& collection_name) const {
    if (collection_name.empty()) {
        return false;  // Empty collection names are never in scope
    }
    // Special case: "graph" is a synthetic collection name for traversal queries
    if (collection_name == "graph") {
        return true;
    }
    return registered_collections_.count(collection_name) > 0;
}

Result<bool> ParserScopeContext::validateCollectionAccess(
    const std::string& collection_name,
    const std::string& context_description) const {
    if (!isCollectionInScope(collection_name)) {
        // Build the registered-collections list efficiently using fmt::format
        // instead of string += in loop to avoid repeated allocations.
        std::string registered_list;
        if (registered_collections_.empty()) {
            registered_list = "(none)";
        } else {
            // Efficiently join collection names with commas.
            // Use std::ostringstream or manual building with proper capacity.
            std::ostringstream ss;
            bool first = true;
            for (const auto& c : registered_collections_) {
                if (!first) {
                  ss << ", ";
                }
                ss << c;
                first = false;
            }
            registered_list = ss.str();
        }
        return Err<bool>(
            errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
            fmt::format("Collection '{}' not in scope for {} statement. "
                       "Registered collections: {}",
                       collection_name, context_description, registered_list)
        );
    }
    return Ok(true);
}

void ParserScopeContext::pushScope() {
    scope_stack_.push_back(registered_collections_);
    scope_prefix_stack_.push_back(current_scope_prefix_);
}

void ParserScopeContext::popScope() {
    if (!scope_stack_.empty()) {
        registered_collections_ = std::move(scope_stack_.back());
        scope_stack_.pop_back();
    }
    if (!scope_prefix_stack_.empty()) {
        current_scope_prefix_ = std::move(scope_prefix_stack_.back());
        scope_prefix_stack_.pop_back();
    }
}

const std::set<std::string>& ParserScopeContext::getRegisteredCollections() const {
    return registered_collections_;
}

void ParserScopeContext::clear() {
    registered_collections_.clear();
    scope_stack_.clear();
    current_scope_prefix_.clear();
    scope_prefix_stack_.clear();
}

// ============================================================================
// Tokenizer (Lexer)
// ============================================================================

// Windows headers may define IN as a parameter annotation macro, which
// collides with the AQL token enum member name.
#ifdef IN
#undef IN
#endif
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
// [WAVE1-FIX: scope_mismatch:178] Some system or third-party headers define
// PHRASE, NEAR, and SEARCH as preprocessor macros.  Undef them here before
// the TokenType enum class so that the enum values are not silently replaced
// by macro expansions.  Because TokenType is an enum class, the values are
// already namespace-scoped; the undef guards provide an additional layer of
// protection against macro collision in translation units that include
// platform headers prior to this file.
#ifdef PHRASE
#undef PHRASE
#endif
#ifdef NEAR
#undef NEAR
#endif
#ifdef SEARCH
#undef SEARCH
#endif
#ifdef ANALYZER
#undef ANALYZER
#endif

enum class TokenType {
    // Keywords
    FOR, IN, FILTER, SORT, LIMIT, RETURN, LET,
    ASC, DESC, AND, OR, XOR, NOT,
    GRAPH, OUTBOUND, INBOUND, ANY,
    TYPE,
    COLLECT, AGGREGATE,
    TRUE, FALSE, NULL_LITERAL,
    
    // Phase 2: Hybrid Query Keywords
    SIMILARITY,      // SIMILARITY(vectorField, queryVector) for Vector+Geo
    PROXIMITY,       // PROXIMITY(geoField, point) for Content+Geo
    SHORTEST_PATH,   // SHORTEST_PATH TO target for Graph+Geo
    TO,              // TO keyword for shortest path target
    
    // Phase 3: Subqueries & CTEs
    WITH,            // WITH cteName = subquery for CTEs
    AS,              // AS alias for CTE naming
    ALL,             // ALL quantifier for array subqueries
    SATISFIES,       // SATISFIES for array predicates

    // Phase 4: Multi-statement transaction AQL
    BEGIN,           // BEGIN – start of a transaction block
    COMMIT,          // COMMIT – successfully end a transaction block
    ROLLBACK,        // ROLLBACK – abort a transaction block

    // Phase 5: DML mutation keywords (EPIC-004)
    INSERT,          // INSERT doc INTO collection
    UPDATE,          // UPDATE … SET … / UPDATE … WITH … IN …
    DELETE,          // DELETE FROM collection WHERE …  (SQL-style alias for REMOVE)
    REMOVE,          // REMOVE doc IN collection  (AQL-native)
    REPLACE,         // REPLACE search WITH replacement IN collection
    UPSERT,          // UPSERT search INSERT doc UPDATE upd IN collection
    INTO,            // INSERT INTO  /  INSERT doc INTO
    SET,             // UPDATE … SET k=v
    VALUES,          // INSERT INTO … VALUES {doc}
    FROM,            // DELETE FROM collection
    WHERE,           // … WHERE condition
    
    // Phase 6: FTS / SEARCH clause keywords (Target: Q3–Q4 2026)
    SEARCH,          // SEARCH — start of a full-text search clause
    PHRASE,          // PHRASE(field, "text") FTS predicate function
    NEAR,            // NEAR[n] proximity operator
    STARTS_WITH,     // STARTS_WITH(field, "prefix") FTS predicate function
    BOOST,           // BOOST n.n — per-clause relevance multiplier
    ANALYZER,        // ANALYZER "name" — text analyzer selection

    // Operators
    EQ, NEQ, LT, LTE, GT, GTE,
    PLUS, MINUS, STAR, SLASH, MODULO,
    ASSIGN,  // Single '=' for assignments (COLLECT var = expr, AGGREGATE var = func)
    
    // Literals
    IDENTIFIER, STRING, INTEGER, FLOAT,
    
    // Punctuation
    DOT, COMMA, SEMICOLON, COLON, LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    
    // Special
    END_OF_FILE, INVALID
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    
    Token(TokenType t, std::string v, size_t l, size_t c)
        : type(t), value(std::move(v)), line(l), column(c) {}
};

/** @brief Query function that converts a value to kenizer. */
class Tokenizer {
public:
    explicit Tokenizer(const std::string& input)
        : input_(input), pos_(0), line_(1), column_(1) {}
    
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        
        while (pos_ < input_.size()) {
            skipWhitespace();
            if (pos_ >= input_.size()) {
              break;
            }
            
            Token token = nextToken();
            if (token.type != TokenType::INVALID) {
                tokens.push_back(token);
            }
        }
        
        tokens.emplace_back(TokenType::END_OF_FILE, "", line_, column_);
        return tokens;
    }
    
private:
    std::string input_;
    // [WAVE1-FIX: scope_mismatch:234] pos_, line_, and column_ here are
    // private members of the Tokenizer class.  The identical names in the
    // Parser class (below) are also private members of a *separate* class;
    // they do not shadow each other in C++ (different class scopes).  The
    // scope_mismatch flag was a static-analysis heuristic false positive.
    // Both sets of members are intentionally named identically for
    // consistency between the two lexical-analysis states.
    size_t pos_;
    size_t line_;
    size_t column_;
    
    char peek([[maybe_unused]] size_t offset = 0) const {
        size_t p = pos_ + offset;
        return (p < input_.size()) ? input_[p] : '\0';
    }
    
    char advance() {
        if (pos_ >= input_.size()) {
          return '\0';
        }
        char ch = input_[pos_++];
        if (ch == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        return ch;
    }
    
    void skipWhitespace() {
        while (pos_ < input_.size() && std::isspace(peek())) {
            advance();
        }
    }
    
    Token nextToken() {
        size_t start_line = line_;
        size_t start_column = column_;
        char ch = peek();
        
        // String literal (unterstützt ' und ")
        if (ch == '"' || ch == '\'') {
            return readString(start_line, start_column);
        }
        
        // Number
        if (std::isdigit(ch) || (ch == '-' && std::isdigit(peek(1)))) {
            return readNumber(start_line, start_column);
        }
        
        // Identifier or keyword
        if (std::isalpha(ch) || ch == '_') {
            return readIdentifierOrKeyword(start_line, start_column);
        }
        
        // Operators and punctuation
        return readOperatorOrPunctuation(start_line, start_column);
    }
    
    Token readString(size_t line, size_t col) {
        char quote = peek();
        // Support both double and single quotes
        if (quote != '"' && quote != '\'') {
            // Fallback (shouldn't happen): treat as invalid
            return Token(TokenType::INVALID, std::string(1, advance()), line, col);
        }
        advance(); // Skip opening quote
        std::string value;
        value.reserve(256);  // Pre-allocate to avoid O(n²) growth
         
        while (peek() != quote && peek() != '\0') {
            if (peek() == '\\') {
                advance();
                char next = advance();
                switch (next) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case 'r': value += '\r'; break;
                    case '"': value += '"'; break;
                    case '\'': value += '\''; break;
                    case '\\': value += '\\'; break;
                    default: value += next; break;
                }
            } else {
                value += advance();
            }
        }
        
        if (peek() == quote) {
            advance(); // Skip closing quote
        }
        
        return Token(TokenType::STRING, value, line, col);
    }
    
    Token readNumber(size_t line, size_t col) {
        std::string value;
        value.reserve(32);  // Pre-allocate for typical number sizes
        bool is_float = false;
         
        if (peek() == '-') {
            value += advance();
        }
         
        while (std::isdigit(peek())) {
            value += advance();
        }
         
        // Only treat as float if dot is followed by a digit (e.g., 1.23)
        if (peek() == '.' && std::isdigit(peek(1))) {
            is_float = true;
            value += advance();
            while (std::isdigit(peek())) {
                value += advance();
            }
        }
         
        return Token(is_float ? TokenType::FLOAT : TokenType::INTEGER, value, line, col);
    }
    
    Token readIdentifierOrKeyword(size_t line, size_t col) {
        std::string value;
        value.reserve(64);  // Pre-allocate for typical identifier sizes
         
        while (std::isalnum(peek()) || peek() == '_') {
            value += advance();
        }
        
        // Convert to lowercase for keyword matching
        std::string lower = value;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        // Check keywords
        if (lower == "for") {
          return Token(TokenType::FOR, value, line, col);
        }
        if (lower == "in") {
          return Token(TokenType::IN, value, line, col);
        }
        if (lower == "filter") {
          return Token(TokenType::FILTER, value, line, col);
        }
        if (lower == "sort") {
          return Token(TokenType::SORT, value, line, col);
        }
        if (lower == "limit") {
          return Token(TokenType::LIMIT, value, line, col);
        }
    if (lower == "return") {
      return Token(TokenType::RETURN, value, line, col);
    }
    if (lower == "let") {
      return Token(TokenType::LET, value, line, col);
    }
        if (lower == "asc") {
          return Token(TokenType::ASC, value, line, col);
        }
        if (lower == "desc") {
          return Token(TokenType::DESC, value, line, col);
        }
    if (lower == "and") {
      return Token(TokenType::AND, value, line, col);
    }
    if (lower == "or") {
      return Token(TokenType::OR, value, line, col);
    }
    if (lower == "xor") {
      return Token(TokenType::XOR, value, line, col);
    }
        if (lower == "not") {
          return Token(TokenType::NOT, value, line, col);
        }
        if (lower == "true") {
          return Token(TokenType::TRUE, value, line, col);
        }
        if (lower == "false") {
          return Token(TokenType::FALSE, value, line, col);
        }
        if (lower == "null") {
          return Token(TokenType::NULL_LITERAL, value, line, col);
        }
        if (lower == "graph") {
          return Token(TokenType::GRAPH, value, line, col);
        }
        if (lower == "outbound") {
          return Token(TokenType::OUTBOUND, value, line, col);
        }
        if (lower == "inbound") {
          return Token(TokenType::INBOUND, value, line, col);
        }
        if (lower == "any") {
          return Token(TokenType::ANY, value, line, col);
        }
    if (lower == "type") {
      return Token(TokenType::TYPE, value, line, col);
    }
        if (lower == "collect") {
          return Token(TokenType::COLLECT, value, line, col);
        }
        if (lower == "aggregate") {
          return Token(TokenType::AGGREGATE, value, line, col);
        }
        
            // Phase 2: Hybrid Query Keywords
            // Note: SIMILARITY and PROXIMITY are function names, not keywords - should remain as IDENTIFIER
            // if (lower == "similarity") return Token(TokenType::SIMILARITY, value, line, col);
            // if (lower == "proximity") return Token(TokenType::PROXIMITY, value, line, col);
            if (lower == "shortest_path") {
              return Token(TokenType::SHORTEST_PATH, value, line, col);
            }
            if (lower == "to") {
              return Token(TokenType::TO, value, line, col);
            }
        
        // Phase 3: Subqueries & CTEs
        if (lower == "with") {
          return Token(TokenType::WITH, value, line, col);
        }
        if (lower == "as") {
          return Token(TokenType::AS, value, line, col);
        }
        if (lower == "all") {
          return Token(TokenType::ALL, value, line, col);
        }
        if (lower == "satisfies") {
          return Token(TokenType::SATISFIES, value, line, col);
        }

        // Phase 4: Multi-statement transaction AQL
        if (lower == "begin") {
          return Token(TokenType::BEGIN, value, line, col);
        }
        if (lower == "commit") {
          return Token(TokenType::COMMIT, value, line, col);
        }
        if (lower == "rollback") {
          return Token(TokenType::ROLLBACK, value, line, col);
        }

        // Phase 5: DML mutation keywords (EPIC-004)
        if (lower == "insert") {
          return Token(TokenType::INSERT,  value, line, col);
        }
        if (lower == "update") {
          return Token(TokenType::UPDATE,  value, line, col);
        }
        if (lower == "delete") {
          return Token(TokenType::DELETE,  value, line, col);
        }
        if (lower == "remove") {
          return Token(TokenType::REMOVE,  value, line, col);
        }
        if (lower == "replace") {
          return Token(TokenType::REPLACE, value, line, col);
        }
        if (lower == "upsert") {
          return Token(TokenType::UPSERT,  value, line, col);
        }
        if (lower == "into") {
          return Token(TokenType::INTO,    value, line, col);
        }
        if (lower == "set") {
          return Token(TokenType::SET,     value, line, col);
        }
        if (lower == "values") {
          return Token(TokenType::VALUES,  value, line, col);
        }
        if (lower == "from") {
          return Token(TokenType::FROM,    value, line, col);
        }
        if (lower == "where") {
          return Token(TokenType::WHERE,   value, line, col);
        }

        // Phase 6: FTS / SEARCH clause keywords
        if (lower == "search") {
          return Token(TokenType::SEARCH,      value, line, col);
        }
        if (lower == "phrase") {
          return Token(TokenType::PHRASE,       value, line, col);
        }
        if (lower == "near") {
          return Token(TokenType::NEAR,         value, line, col);
        }
        if (lower == "starts_with") {
          return Token(TokenType::STARTS_WITH,  value, line, col);
        }
        if (lower == "boost") {
          return Token(TokenType::BOOST,        value, line, col);
        }
        if (lower == "analyzer") {
          return Token(TokenType::ANALYZER,     value, line, col);
        }

        return Token(TokenType::IDENTIFIER, value, line, col);
    }
    
    Token readOperatorOrPunctuation(size_t line, size_t col) {
        char ch = peek();
        
        // Two-character operators
        if (ch == '=' && peek(1) == '=') {
            advance(); advance();
            // Check for invalid === operator
            if (peek() == '=') {
                return Token(TokenType::INVALID, "===", line, col);
            }
            return Token(TokenType::EQ, "==", line, col);
        }
        if (ch == '!' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::NEQ, "!=", line, col);
        }
        if (ch == '<' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::LTE, "<=", line, col);
        }
        if (ch == '>' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::GTE, ">=", line, col);
        }
        
        // Single-character operators/punctuation
        advance();
        switch (ch) {
            case ':': return Token(TokenType::COLON, ":", line, col);
            case '=': return Token(TokenType::ASSIGN, "=", line, col);  // Single = for assignments
            case '<': return Token(TokenType::LT, "<", line, col);
            case '>': return Token(TokenType::GT, ">", line, col);
            case '+': return Token(TokenType::PLUS, "+", line, col);
            case '-': return Token(TokenType::MINUS, "-", line, col);
            case '*': return Token(TokenType::STAR, "*", line, col);
            case '/': return Token(TokenType::SLASH, "/", line, col);
            case '%': return Token(TokenType::MODULO, "%", line, col);
            case '.': return Token(TokenType::DOT, ".", line, col);
            case ',': return Token(TokenType::COMMA, ",", line, col);
            case ';': return Token(TokenType::SEMICOLON, ";", line, col);
            case '(': return Token(TokenType::LPAREN, "(", line, col);
            case ')': return Token(TokenType::RPAREN, ")", line, col);
            case '{': return Token(TokenType::LBRACE, "{", line, col);
            case '}': return Token(TokenType::RBRACE, "}", line, col);
            case '[': return Token(TokenType::LBRACKET, "[", line, col);
            case ']': return Token(TokenType::RBRACKET, "]", line, col);
            default: return Token(TokenType::INVALID, std::string(1, ch), line, col);
        }
    }
};

// ============================================================================
// Parser
// ============================================================================

/** @brief Parser. */
class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens_(std::move(tokens)), pos_(0) {}
    
    Result<std::shared_ptr<Query>> parse() {
        try {
            // Check for invalid tokens first
            for (const auto& token : tokens_) {
                if (token.type == TokenType::INVALID) {
                    return Err<std::shared_ptr<Query>>(
                        errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                        fmt::format("Invalid token '{}' at line {}, column {}", 
                                    token.value, token.line, token.column)
                    );
                }
            }
            
            auto query = parseQuery(false); // false = not a subquery
            return Ok(query);
        } catch (const std::runtime_error& e) {
            const auto& tok = current();
            return Err<std::shared_ptr<Query>>(
                errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                fmt::format("Parse error at line {}, column {}: {}", 
                            tok.line, tok.column, e.what())
            );
        }
    }

    Result<std::shared_ptr<Expression>> parseStandaloneExpression() {
        try {
            for (const auto& token : tokens_) {
                if (token.type == TokenType::INVALID) {
                    return Err<std::shared_ptr<Expression>>(
                        errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                        fmt::format("Invalid token '{}' at line {}, column {}",
                                    token.value, token.line, token.column)
                    );
                }
            }

            auto expr = parseExpression();
            if (!match(TokenType::END_OF_FILE)) {
                const auto& tok = current();
                return Err<std::shared_ptr<Expression>>(
                    errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                    fmt::format("Unexpected token '{}' at line {}, column {}",
                                tok.value, tok.line, tok.column)
                );
            }

            return Ok(expr);
        } catch (const std::runtime_error& e) {
            const auto& tok = current();
            return Err<std::shared_ptr<Expression>>(
                errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                fmt::format("Parse error at line {}, column {}: {}",
                            tok.line, tok.column, e.what())
            );
        }
    }
    
private:
    std::vector<Token> tokens_;
    size_t pos_;
    std::shared_ptr<Query::TraversalNode> lastTraversal_;
    // PA-1 fix: tracks current expression parse depth to prevent unbounded recursion
    // (stack overflow via crafted queries with thousands of nested NOT / subexpressions).
    int depth_{0};
    static constexpr int kMaxExprDepth = 500;
    // Phase 2 Agent 1: Scope validation context
    ParserScopeContext scope_context_;
    
    const Token& current() const {
        return (pos_ < tokens_.size()) ? tokens_[pos_] : tokens_.back();
    }
    
    const Token& peek([[maybe_unused]] size_t offset = 1) const {
        size_t p = pos_ + offset;
        return (p < tokens_.size()) ? tokens_[p] : tokens_.back();
    }
    
    void advance() {
        if (pos_ < tokens_.size()) {
          pos_++;
        }
    }
    
    bool match(TokenType type) const {
        return current().type == type;
    }
    
    void expect(TokenType type, const std::string& msg) {
        if (!match(type)) {
            throw std::runtime_error(msg);
        }
        advance();
    }
    
    std::shared_ptr<Query> parseQuery([[maybe_unused]] bool isSubquery = false) {
        auto query = std::make_shared<Query>();
        
        // Phase 3: Optional WITH clause
        if (match(TokenType::WITH)) {
            query->with_clause = parseWithClause();
        }
        
        // One or more FOR clauses (first is also stored in for_node for backward compat)
        if (!match(TokenType::FOR)) {
            throw std::runtime_error("Expected FOR");
        }
        // Parse first FOR
        query->for_node = parseForClause();
        query->for_nodes.push_back(query->for_node);
        if (lastTraversal_) {
            query->traversal = lastTraversal_;
            lastTraversal_.reset();
        }
        // Additional FOR clauses (for joins); traversal queries won't have additional FORs
        while (match(TokenType::FOR)) {
            auto f = parseForClause();
            query->for_nodes.push_back(std::move(f));
        }
        
        // LET clauses (optional, multiple)
        while (match(TokenType::LET)) {
            auto let = parseLetClause();
            query->let_nodes.push_back(std::move(let));
        }

        // FILTER clauses (optional, multiple)
        while (match(TokenType::FILTER)) {
            query->filters.push_back(parseFilterClause());
        }
        
        // SORT clause (optional)
        if (match(TokenType::SORT)) {
            query->sort = parseSortClause();
        }
        
        // LIMIT clause (optional)
        if (match(TokenType::LIMIT)) {
            query->limit = parseLimitClause();
        }

        // COLLECT/GROUP BY clause (optional, MVP)
        if (match(TokenType::COLLECT)) {
            query->collect = parseCollectClause();
        }

        // SEARCH clause (optional, Phase 6 FTS)
        if (match(TokenType::SEARCH)) {
            query->search_clause = parseSearchClause();
        }

        auto parseShortestPathClause = [&]() {
            if (query->traversal && match(TokenType::SHORTEST_PATH)) {
                advance();
                expect(TokenType::TO, "Expected TO after SHORTEST_PATH");
                if (!match(TokenType::STRING) && !match(TokenType::IDENTIFIER)) {
                    throw std::runtime_error("Expected target vertex after SHORTEST_PATH TO");
                }
                std::string target = current().value;
                advance();
                query->traversal->shortestPath = true;
                query->traversal->shortestPathTarget = target;
            }
        };

        // Optional shortest path clause before RETURN (canonical position)
        parseShortestPathClause();
        
        // RETURN clause (optional)
        if (match(TokenType::RETURN)) {
            query->return_node = parseReturnClause();
        }

        // Backward compatibility: also accept SHORTEST_PATH after RETURN
        parseShortestPathClause();
        
        // End of query - only check for EOF if not in subquery context
        if (!isSubquery) {
            expect(TokenType::END_OF_FILE, "Expected end of query");
        }
        
        return query;
    }

    /**
     * @brief Parse a SEARCH clause: SEARCH [predicates] [IN field] [ANALYZER name] [BOOST n]
     *
     * Grammar (simplified):
     * @code
     * search_clause ::= SEARCH predicate_list [IN IDENTIFIER] [ANALYZER STRING] [BOOST NUMBER]
     * predicate_list ::= predicate { (AND | ',') predicate }
     * predicate      ::= PHRASE '(' field ',' STRING [',' STRING] ')'
     *                  | STARTS_WITH '(' field ',' STRING [',' STRING] ')'
     *                  | NEAR '[' NUMBER ']' '(' field ',' STRING ')'
     *                  | field '==' STRING        (implicit TERM predicate)
     *                  | STRING                   (bare term, field from IN clause)
     * @endcode
     *
     * @return Shared pointer to the parsed `SearchClauseNode`.
     * @throws std::runtime_error on syntax errors.
     *
     * @since Phase 6 FTS (Target: Q3 2026)
     */
    std::shared_ptr<SearchClauseNode> parseSearchClause() {
        expect(TokenType::SEARCH, "Expected SEARCH keyword");

        auto node = std::make_shared<SearchClauseNode>();

        // Helper: parse a single FTS predicate
        auto parsePredicate = [&]() -> FtsPredicateNode {
            FtsPredicateNode pred;

            if (match(TokenType::PHRASE)) {
                // PHRASE(field, "term" [, "analyzer"])
                pred.pred_type = FtsPredType::PHRASE;
                advance(); // consume PHRASE
                expect(TokenType::LPAREN, "Expected '(' after PHRASE");
                if (!match(TokenType::IDENTIFIER)) {
                    throw std::runtime_error("PHRASE: expected field expression");
                }
                pred.field = current().value;
                advance();
                expect(TokenType::COMMA, "PHRASE: expected ',' after field");
                if (!match(TokenType::STRING)) {
                    throw std::runtime_error("PHRASE: expected string term");
                }
                pred.term = current().value;
                advance();
                if (match(TokenType::COMMA)) {
                    advance();
                    if (match(TokenType::STRING)) {
                        pred.analyzer = current().value;
                        advance();
                    }
                }
                expect(TokenType::RPAREN, "PHRASE: expected closing ')'");

            } else if (match(TokenType::STARTS_WITH)) {
                // STARTS_WITH(field, "prefix" [, "analyzer"])
                pred.pred_type = FtsPredType::PREFIX;
                advance(); // consume STARTS_WITH
                expect(TokenType::LPAREN, "Expected '(' after STARTS_WITH");
                if (!match(TokenType::IDENTIFIER)) {
                    throw std::runtime_error("STARTS_WITH: expected field expression");
                }
                pred.field = current().value;
                advance();
                expect(TokenType::COMMA, "STARTS_WITH: expected ',' after field");
                if (!match(TokenType::STRING)) {
                    throw std::runtime_error("STARTS_WITH: expected string prefix");
                }
                pred.term = current().value;
                advance();
                if (match(TokenType::COMMA)) {
                    advance();
                    if (match(TokenType::STRING)) {
                        pred.analyzer = current().value;
                        advance();
                    }
                }
                expect(TokenType::RPAREN, "STARTS_WITH: expected closing ')'");

            } else if (match(TokenType::NEAR)) {
                // NEAR[n](field, "term")
                pred.pred_type = FtsPredType::PROXIMITY;
                advance(); // consume NEAR
                // Optional [n] distance specifier
                if (match(TokenType::LBRACKET)) {
                    advance();
                    if (match(TokenType::INTEGER) || match(TokenType::FLOAT)) {
                        try {
                            pred.proximity_distance = static_cast<uint32_t>(
                                std::stoul(current().value));
                        } catch (const std::out_of_range&) {
                            THEMIS_WARN("aql_parser: NEAR predicate distance value overflow '{}', using default 0", current().value);
                            pred.proximity_distance = 0;
                        } catch (const std::invalid_argument&) {
                            THEMIS_WARN("aql_parser: NEAR predicate distance '{}' is not a valid number, using default 0", current().value);
                            pred.proximity_distance = 0;
                        }
                        advance();
                    }
                    expect(TokenType::RBRACKET, "NEAR: expected ']' after distance");
                }
                expect(TokenType::LPAREN, "Expected '(' after NEAR[n]");
                if (!match(TokenType::IDENTIFIER)) {
                    throw std::runtime_error("NEAR: expected field expression");
                }
                pred.field = current().value;
                advance();
                expect(TokenType::COMMA, "NEAR: expected ',' after field");
                if (!match(TokenType::STRING)) {
                    throw std::runtime_error("NEAR: expected string term");
                }
                pred.term = current().value;
                advance();
                expect(TokenType::RPAREN, "NEAR: expected closing ')'");

            } else if (match(TokenType::STRING)) {
                // Bare string term — uses IN field or caller provides field later
                pred.pred_type = FtsPredType::TERM;
                pred.term = current().value;
                advance();

            } else if (match(TokenType::IDENTIFIER)) {
                // field == "term" implicit TERM predicate
                pred.pred_type = FtsPredType::TERM;
                pred.field = current().value;
                advance();
                if (match(TokenType::EQ)) {
                    advance();
                    if (!match(TokenType::STRING)) {
                        throw std::runtime_error("SEARCH: expected string literal after '=='");
                    }
                    pred.term = current().value;
                    advance();
                }
            } else {
                throw std::runtime_error("SEARCH: unexpected token in predicate list");
            }

            // Optional per-predicate BOOST modifier
            if (match(TokenType::BOOST)) {
                advance();
                if (!match(TokenType::INTEGER) && !match(TokenType::FLOAT)) {
                    throw std::runtime_error("SEARCH BOOST: expected numeric value");
                }
                try {
                    pred.boost = std::stod(current().value);
                } catch (const std::out_of_range&) {
                    THEMIS_WARN("aql_parser: SEARCH BOOST value overflow '{}', using default 1.0", current().value);
                    pred.boost = 1.0;
                } catch (const std::invalid_argument&) {
                    THEMIS_WARN("aql_parser: SEARCH BOOST value '{}' is not a valid number, using default 1.0", current().value);
                    pred.boost = 1.0;
                }
                advance();
            }

            // Optional per-predicate ANALYZER modifier
            if (match(TokenType::ANALYZER)) {
                advance();
                if (!match(TokenType::STRING)) {
                    throw std::runtime_error("SEARCH ANALYZER: expected string name");
                }
                pred.analyzer = current().value;
                advance();
            }

            return pred;
        };

        // Parse predicate list (comma- or AND-separated)
        node->predicates.push_back(parsePredicate());

        while (match(TokenType::COMMA) ||
               (match(TokenType::IDENTIFIER) && current().value == "AND")) {
            advance(); // consume ',' or AND
            node->predicates.push_back(parsePredicate());
        }

        // Optional: IN <field> clause
        if (match(TokenType::IN)) {
            advance();
            if (!match(TokenType::IDENTIFIER)) {
                throw std::runtime_error("SEARCH IN: expected field name");
            }
            node->in_field = current().value;
            advance();
        }

        // Optional: top-level ANALYZER "name"
        if (match(TokenType::ANALYZER)) {
            advance();
            if (!match(TokenType::STRING)) {
                throw std::runtime_error("SEARCH ANALYZER: expected string name");
            }
            node->default_analyzer = current().value;
            advance();
        }

        // Optional: top-level BOOST n
        if (match(TokenType::BOOST)) {
            advance();
            if (!match(TokenType::INTEGER) && !match(TokenType::FLOAT)) {
                throw std::runtime_error("SEARCH BOOST: expected numeric value");
            }
            try {
                node->top_boost = std::stod(current().value);
            } catch (const std::out_of_range&) {
                THEMIS_WARN("aql_parser: SEARCH top-level BOOST value overflow '{}', using default 1.0", current().value);
                node->top_boost = 1.0;
            } catch (const std::invalid_argument&) {
                THEMIS_WARN("aql_parser: SEARCH top-level BOOST value '{}' is not a valid number, using default 1.0", current().value);
                node->top_boost = 1.0;
            }
            advance();
        }

        return node;
    }

    ForNode parseForClause() {
        expect(TokenType::FOR, "Expected FOR");
        
        if (!match(TokenType::IDENTIFIER)) {
            throw std::runtime_error("Expected variable name after FOR");
        }
        std::string varVertex = current().value;
        advance();

        // Optional: ", e" und ", p" vor IN
        std::optional<std::string> varEdge;
        std::optional<std::string> varPath;
        if (match(TokenType::COMMA)) {
            advance();
            if (!match(TokenType::IDENTIFIER)) {
                throw std::runtime_error("Expected edge variable name after ','");
            }
            varEdge = current().value;
            advance();
            if (match(TokenType::COMMA)) {
                advance();
                if (!match(TokenType::IDENTIFIER)) {
                    throw std::runtime_error("Expected path variable name after second ','");
                }
                varPath = current().value;
                advance();
            }
        }

        expect(TokenType::IN, "Expected IN");

        // Two alternatives after IN:
        // 1) Collection name (IDENTIFIER) => relational
        // 2) Graph traversal: INTEGER .. INTEGER <DIRECTION> STRING GRAPH STRING
        if (match(TokenType::IDENTIFIER)) {
            std::string collection = current().value;
            advance();
            
            // Phase 2 Agent 1: Register and validate collection scope
            scope_context_.registerCollection(collection);
            
            ForNode node;
            node.variable = varVertex;
            node.collection = collection;
            return node;
        }

        if (match(TokenType::INTEGER)) {
            // Parse min..max
            // PA-2 fix: enforce an upper bound on graph traversal depth to prevent
            // BFS/DFS from being triggered with values like INT_MAX.
            static constexpr int kMaxTraversalDepth = 1000;

            int minDepth;
            try {
                minDepth = std::stoi(current().value);
            } catch (const std::out_of_range&) {
                throw std::runtime_error(
                    "Graph traversal min depth '" + current().value + "' is out of integer range");
            } catch (const std::invalid_argument&) {
                throw std::runtime_error(
                    "Graph traversal min depth '" + current().value + "' is not a valid integer");
            }
            if (minDepth < 0) {
                throw std::runtime_error("Graph traversal min depth must be >= 0");
            }
            advance();
            // Expect '..' as two DOT tokens
            if (!match(TokenType::DOT) || peek().type != TokenType::DOT) {
                throw std::runtime_error("Expected '..' in traversal depth range");
            }
            advance(); // first '.'
            advance(); // second '.'
            if (!match(TokenType::INTEGER)) {
                throw std::runtime_error("Expected max depth integer after '..'");
            }
            int maxDepth;
            try {
                maxDepth = std::stoi(current().value);
            } catch (const std::out_of_range&) {
                throw std::runtime_error(
                    "Graph traversal max depth '" + current().value + "' is out of integer range");
            } catch (const std::invalid_argument&) {
                throw std::runtime_error(
                    "Graph traversal max depth '" + current().value + "' is not a valid integer");
            }
            if (maxDepth > kMaxTraversalDepth) {
                throw std::runtime_error(
                    "Graph traversal max depth " + std::to_string(maxDepth) +
                    " exceeds limit " + std::to_string(kMaxTraversalDepth));
            }
            advance();

            // Direction
            Query::TraversalNode::Direction dir;
            if (match(TokenType::OUTBOUND)) { dir = Query::TraversalNode::Direction::Outbound; advance(); }
            else if (match(TokenType::INBOUND)) { dir = Query::TraversalNode::Direction::Inbound; advance(); }
            else if (match(TokenType::ANY)) { dir = Query::TraversalNode::Direction::Any; advance(); }
            else { throw std::runtime_error("Expected OUTBOUND, INBOUND or ANY in traversal"); }

            // Start vertex (STRING)
            if (!match(TokenType::STRING)) {
                throw std::runtime_error("Expected start vertex string literal in traversal");
            }
            std::string startVertex = current().value;
            advance();

            // GRAPH keyword and graph name
            // Optional TYPE "edgeType" vor GRAPH
            std::string edgeType;
            if (match(TokenType::TYPE)) {
                advance();
                if (!match(TokenType::STRING)) {
                    throw std::runtime_error("Expected edge type string literal after TYPE");
                }
                edgeType = current().value;
                advance();
            }

            expect(TokenType::GRAPH, "Expected GRAPH keyword in traversal");
            if (!match(TokenType::STRING)) {
                throw std::runtime_error("Expected graph name string literal after GRAPH");
            }
            std::string graphName = current().value;
            advance();

            // Build traversal node and stash it for parseQuery
            auto trav = std::make_shared<Query::TraversalNode>();
            trav->varVertex = varVertex;
            if (varEdge) {
              trav->varEdge = *varEdge;
            }
            if (varPath) {
              trav->varPath = *varPath;
            }
            trav->minDepth = minDepth;
            trav->maxDepth = maxDepth;
            trav->direction = dir;
            trav->startVertex = startVertex;
            trav->graphName = graphName;
            trav->edgeType = edgeType;
            lastTraversal_ = trav;

            // Still return a ForNode for compatibility (collection = "graph")
            ForNode node;
            node.variable = varVertex;
            node.collection = "graph";
            return node;
        }

        throw std::runtime_error("Expected collection name or traversal after IN");
    }

    LetNode parseLetClause() {
        expect(TokenType::LET, "Expected LET");
        if (!match(TokenType::IDENTIFIER)) {
            throw std::runtime_error("Expected variable name after LET");
        }
        std::string var = current().value;
        advance();
        expect(TokenType::ASSIGN, "Expected '=' after variable name in LET");
        auto expr = parseExpression();
        LetNode node{var, expr};
        return node;
    }
    
    std::shared_ptr<FilterNode> parseFilterClause() {
        expect(TokenType::FILTER, "Expected FILTER");
        
        auto condition = parseExpression();
        return std::make_shared<FilterNode>(condition);
    }
    
    std::shared_ptr<SortNode> parseSortClause() {
        expect(TokenType::SORT, "Expected SORT");
        
        std::vector<SortSpec> specs;
        
        do {
            if (!specs.empty()) {
                expect(TokenType::COMMA, "Expected comma");
            }
            
            SortSpec spec;
            spec.expression = parseExpression();
            spec.ascending = true; // Default ASC
            
            if (match(TokenType::ASC)) {
                advance();
                spec.ascending = true;
            } else if (match(TokenType::DESC)) {
                advance();
                spec.ascending = false;
            }
            
            specs.push_back(std::move(spec));
            
        } while (match(TokenType::COMMA));
        
        return std::make_shared<SortNode>(std::move(specs));
    }
    
    std::shared_ptr<LimitNode> parseLimitClause() {
        expect(TokenType::LIMIT, "Expected LIMIT");
        
        if (!match(TokenType::INTEGER)) {
            throw std::runtime_error("Expected integer after LIMIT");
        }
        int64_t first;
        try { first = std::stoll(current().value); }
        catch (...) {
            THEMIS_WARN("aql_parser::parseLimitClause: unhandled exception caught");
            throw std::runtime_error("LIMIT value '" + current().value + "' is out of integer range");
        }
        advance();
        
        if (match(TokenType::COMMA)) {
            advance();
            if (!match(TokenType::INTEGER)) {
                throw std::runtime_error("Expected integer after comma in LIMIT");
            }
            int64_t second;
            try { second = std::stoll(current().value); }
            catch (...) {
                THEMIS_WARN("aql_parser::parseLimitClause: unhandled exception caught");
                throw std::runtime_error("LIMIT value '" + current().value + "' is out of integer range");
            }
            advance();
            return std::make_shared<LimitNode>(first, second); // offset, count
        }
        
        return std::make_shared<LimitNode>(0, first); // count only
    }
    
    std::shared_ptr<ReturnNode> parseReturnClause() {
        expect(TokenType::RETURN, "Expected RETURN");
        
        auto expr = parseExpression();
        return std::make_shared<ReturnNode>(expr);
    }

    // Phase 3: Parse WITH clause
    std::shared_ptr<WithNode> parseWithClause() {
        expect(TokenType::WITH, "Expected WITH");
        
        auto withNode = std::make_shared<WithNode>();
        
        // Parse one or more CTEs: cteName AS (subquery) [, cteName AS (subquery)]*
        do {
            if (!withNode->ctes.empty()) {
                expect(TokenType::COMMA, "Expected comma between CTEs");
            }
            
            CTEDefinition cte;
            
            // CTE name
            if (!match(TokenType::IDENTIFIER)) {
                throw std::runtime_error("Expected CTE name after WITH");
            }
            cte.name = current().value;
            advance();
            
            // AS keyword
            expect(TokenType::AS, "Expected AS after CTE name");
            
            // Subquery in parentheses
            expect(TokenType::LPAREN, "Expected '(' before CTE subquery");
            
            // Parse subquery (recursive call to parseQuery with isSubquery=true)
            cte.subquery = parseQuery(true);
            
            expect(TokenType::RPAREN, "Expected ')' after CTE subquery");
            
            withNode->ctes.push_back(std::move(cte));
            
        } while (match(TokenType::COMMA));
        
        return withNode;
    }

    std::shared_ptr<CollectNode> parseCollectClause() {
        expect(TokenType::COLLECT, "Expected COLLECT");
        auto node = std::make_shared<CollectNode>();

        // Optional group variable(s): var = expr
        if (!match(TokenType::AGGREGATE)) {
            if (!match(TokenType::IDENTIFIER)) {
                throw std::runtime_error("Expected variable name after COLLECT");
            }
            std::string var = current().value;
            advance();
            expect(TokenType::ASSIGN, "Expected '=' after group variable in COLLECT");
            auto expr = parseExpression();
            node->groups.emplace_back(var, expr);
            // MVP: allow only one group; additional groups via comma could be added later
        }

        // Optional AGGREGATE section
        if (match(TokenType::AGGREGATE)) {
            advance();
            // Parse list: var = FUNC(expr?) [, var = FUNC(expr?)]*
            bool first = true;
            while (first || match(TokenType::COMMA)) {
                if (!first) {
                  advance();
                }
                first = false;
                if (!match(TokenType::IDENTIFIER)) {
                    throw std::runtime_error("Expected aggregation variable name after AGGREGATE");
                }
                std::string outVar = current().value;
                advance();
                expect(TokenType::ASSIGN, "Expected '=' in aggregation assignment");
                // Expect function call
                if (!match(TokenType::IDENTIFIER)) {
                    throw std::runtime_error("Expected aggregation function name (COUNT, SUM, AVG, MIN, MAX)");
                }
                std::string funcName = current().value;
                advance();
                expect(TokenType::LPAREN, "Expected '(' after aggregation function");
                std::shared_ptr<Expression> arg;
                if (!match(TokenType::RPAREN)) {
                    arg = parseExpression();
                }
                expect(TokenType::RPAREN, "Expected ')' to close aggregation function");
                CollectNode::Aggregation ag{outVar, funcName, arg};
                node->aggregations.push_back(std::move(ag));
            }
        }

        return node;
    }
    
    std::shared_ptr<Expression> parseExpression() {
        // PA-1 fix: guard against unbounded recursion from crafted deeply-nested queries.
        if (depth_ >= kMaxExprDepth) {
            throw std::runtime_error(
                fmt::format("Query expression exceeds maximum nesting depth of {}; "
                            "simplify the query.", kMaxExprDepth)
            );
        }
        ++depth_;
        struct DepthGuard { int& d; ~DepthGuard() { --d; } } guard{depth_};
        return parseLogicalOr();
    }
    
    std::shared_ptr<Expression> parseLogicalOr() {
        auto left = parseLogicalAnd();
        
        while (match(TokenType::OR) || match(TokenType::XOR)) {
            bool isXor = match(TokenType::XOR);
            advance();
            auto right = parseLogicalAnd();
            left = std::make_shared<BinaryOpExpr>(isXor ? BinaryOperator::Xor : BinaryOperator::Or, left, right);
        }
        
        return left;
    }
    
    std::shared_ptr<Expression> parseLogicalAnd() {
        auto left = parseComparison();
        
        while (match(TokenType::AND)) {
            advance();
            auto right = parseComparison();
            left = std::make_shared<BinaryOpExpr>(BinaryOperator::And, left, right);
        }
        
        return left;
    }
    
    std::shared_ptr<Expression> parseComparison() {
        auto left = parseAdditive();
        
        // Membership: left IN right (array or variable)
        // Debug: uncomment to trace tokens
        // std::cerr << "parseComparison current token: " << (int)current().type << " value='" << current().value << "'\n";
        if (match(TokenType::IN) || (match(TokenType::IDENTIFIER) && current().value == "IN")) {
            advance();
            auto right = parseAdditive();
            return std::make_shared<BinaryOpExpr>(BinaryOperator::In, left, right);
        }
        
        if (match(TokenType::EQ)) {
            advance();
            auto right = parseAdditive();
            return std::make_shared<BinaryOpExpr>(BinaryOperator::Eq, left, right);
        }
        if (match(TokenType::NEQ)) {
            advance();
            auto right = parseAdditive();
            return std::make_shared<BinaryOpExpr>(BinaryOperator::Neq, left, right);
        }
        if (match(TokenType::LT)) {
            advance();
            auto right = parseAdditive();
            return std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, left, right);
        }
        if (match(TokenType::LTE)) {
            advance();
            auto right = parseAdditive();
            return std::make_shared<BinaryOpExpr>(BinaryOperator::Lte, left, right);
        }
        if (match(TokenType::GT)) {
            advance();
            auto right = parseAdditive();
            return std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, left, right);
        }
        if (match(TokenType::GTE)) {
            advance();
            auto right = parseAdditive();
            return std::make_shared<BinaryOpExpr>(BinaryOperator::Gte, left, right);
        }
        
        return left;
    }
    
    std::shared_ptr<Expression> parseAdditive() {
        auto left = parseMultiplicative();
        
        while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
            BinaryOperator op = match(TokenType::PLUS) ? BinaryOperator::Add : BinaryOperator::Sub;
            advance();
            auto right = parseMultiplicative();
            left = std::make_shared<BinaryOpExpr>(op, left, right);
        }
        
        return left;
    }
    
    std::shared_ptr<Expression> parseMultiplicative() {
        auto left = parseUnary();
        
        while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::MODULO)) {
            BinaryOperator op = BinaryOperator::Mul;
            if (match(TokenType::STAR)) {
                op = BinaryOperator::Mul;
            } else if (match(TokenType::SLASH)) {
                op = BinaryOperator::Div;
            } else if (match(TokenType::MODULO)) {
                op = BinaryOperator::Mod;
            }
            advance();
            auto right = parseUnary();
            left = std::make_shared<BinaryOpExpr>(op, left, right);
        }
        
        return left;
    }
    
    std::shared_ptr<Expression> parseUnary() {
        if (match(TokenType::NOT)) {
            advance();
            auto operand = parseUnary();
            return std::make_shared<UnaryOpExpr>(UnaryOperator::Not, operand);
        }
        if (match(TokenType::MINUS)) {
            advance();
            auto operand = parseUnary();
            return std::make_shared<UnaryOpExpr>(UnaryOperator::Minus, operand);
        }
        
        return parsePostfix();
    }
    
    std::shared_ptr<Expression> parsePostfix() {
        auto expr = parsePrimary();
        
        // Handle field access: doc.field or doc.field1.field2
        while (match(TokenType::DOT)) {
            advance();
            if (!match(TokenType::IDENTIFIER)) {
                throw std::runtime_error("Expected field name after '.'");
            }
            std::string field = current().value;
            advance();
            expr = std::make_shared<FieldAccessExpr>(expr, field);
        }
        
        return expr;
    }
    
    std::shared_ptr<Expression> parsePrimary() {
        // Phase 3.2: Subquery in expression context
        // Pattern: (FOR ... RETURN expr)
        if (match(TokenType::LPAREN)) {
            size_t savedPos = pos_;
            advance();
            
            // Check if this is a subquery (starts with FOR)
            if (match(TokenType::FOR)) {
                // Parse as subquery (isSubquery=true)
                auto subquery = parseQuery(true);
                expect(TokenType::RPAREN, "Expected ')' after subquery");
                return std::make_shared<SubqueryExpr>(subquery);
            }
            
            // Not a subquery, restore position and parse as parenthesized expression
            pos_ = savedPos;
            advance();
            auto expr = parseExpression();
            expect(TokenType::RPAREN, "Expected ')'");
            return expr;
        }
        
        // Object literal: { key: expr, ... }
        if (match(TokenType::LBRACE)) {
            advance();
            std::vector<std::pair<std::string, std::shared_ptr<Expression>>> fields;
            if (!match(TokenType::RBRACE)) {
                while (true) {
                    // key can be IDENTIFIER or STRING
                    std::string key;
                    if (match(TokenType::IDENTIFIER) || match(TokenType::STRING)) {
                        key = current().value; advance();
                    } else {
                        throw std::runtime_error("Expected object key (identifier or string)");
                    }
                    expect(TokenType::COLON, "Expected ':' after object key");
                    auto val = parseExpression();
                    fields.emplace_back(key, val);
                    if (match(TokenType::COMMA)) { advance(); continue; }
                    break;
                }
                expect(TokenType::RBRACE, "Expected '}' at end of object");
            } else {
                advance(); // empty object {}
            }
            return std::make_shared<ObjectConstructExpr>(std::move(fields));
        }
        // Array literal: [ expr, ... ]
        if (match(TokenType::LBRACKET)) {
            advance();
            std::vector<std::shared_ptr<Expression>> elems;
            if (!match(TokenType::RBRACKET)) {
                while (true) {
                    elems.push_back(parseExpression());
                    if (match(TokenType::COMMA)) { advance(); continue; }
                    break;
                }
                expect(TokenType::RBRACKET, "Expected ']' at end of array");
            } else {
                advance(); // []
            }
            return std::make_shared<ArrayLiteralExpr>(std::move(elems));
        }
        
        // Literals
        if (match(TokenType::STRING)) {
            std::string value = current().value;
            advance();
            return std::make_shared<LiteralExpr>(value);
        }
        if (match(TokenType::INTEGER)) {
            int64_t value;
            try { value = std::stoll(current().value); }
            catch (...) {
                THEMIS_WARN("aql_parser: unhandled exception caught");
                throw std::runtime_error("Integer literal '" + current().value + "' is out of range");
            }
            advance();
            return std::make_shared<LiteralExpr>(value);
        }
        if (match(TokenType::FLOAT)) {
            double value;
            try { value = std::stod(current().value); }
            catch (...) {
                THEMIS_WARN("aql_parser: unhandled exception caught");
                throw std::runtime_error("Float literal '" + current().value + "' is out of range");
            }
            advance();
            return std::make_shared<LiteralExpr>(value);
        }
        if (match(TokenType::TRUE)) {
            advance();
            return std::make_shared<LiteralExpr>(true);
        }
        if (match(TokenType::FALSE)) {
            advance();
            return std::make_shared<LiteralExpr>(false);
        }
        if (match(TokenType::NULL_LITERAL)) {
            advance();
            return std::make_shared<LiteralExpr>(nullptr);
        }
        
        // Phase 3.3: ANY quantifier
        // Pattern: ANY var IN arrayExpr SATISFIES condition
        if (match(TokenType::ANY)) {
            advance();
            
            if (!match(TokenType::IDENTIFIER)) {
                throw std::runtime_error("Expected variable name after ANY");
            }
            std::string varName = current().value;
            advance();
            
            expect(TokenType::IN, "Expected IN after ANY variable");
            
            auto arrayExpr = parseExpression();
            
            expect(TokenType::SATISFIES, "Expected SATISFIES after array expression");
            
            auto condition = parseExpression();
            
            return std::make_shared<AnyExpr>(varName, arrayExpr, condition);
        }
        
        // Phase 3.3: ALL quantifier
        // Pattern: ALL var IN arrayExpr SATISFIES condition
        if (match(TokenType::ALL)) {
            advance();
            
            if (!match(TokenType::IDENTIFIER)) {
                throw std::runtime_error("Expected variable name after ALL");
            }
            std::string varName = current().value;
            advance();
            
            expect(TokenType::IN, "Expected IN after ALL variable");
            
            auto arrayExpr = parseExpression();
            
            expect(TokenType::SATISFIES, "Expected SATISFIES after array expression");
            
            auto condition = parseExpression();
            
            return std::make_shared<AllExpr>(varName, arrayExpr, condition);
        }
        
        // Identifier or function call
        if (match(TokenType::IDENTIFIER)) {
            std::string name = current().value;
            advance();

            // Support dotted function syntax like PATH.ALL(...) or MODULE.FUNC(...)
            if (match(TokenType::DOT) && peek().type == TokenType::IDENTIFIER && peek(1).type == TokenType::LPAREN) {
                // consume '.'
                advance();
                // second identifier
                std::string sec = current().value;
                advance();
                // combine name
                std::string fullName = name + "." + sec;
                // expect '(' and parse args
                expect(TokenType::LPAREN, "Expected '(' after function name");
                std::vector<std::shared_ptr<Expression>> args;
                if (!match(TokenType::RPAREN)) {
                    do {
                        if (!args.empty()) {
                          expect(TokenType::COMMA, "Expected comma");
                        }
                        args.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }
                expect(TokenType::RPAREN, "Expected ')'");
                return std::make_shared<FunctionCallExpr>(fullName, std::move(args));
            }

            // Function call (simple identifier)
            if (match(TokenType::LPAREN)) {
                advance();
                std::vector<std::shared_ptr<Expression>> args;

                if (!match(TokenType::RPAREN)) {
                    do {
                        if (!args.empty()) {
                            expect(TokenType::COMMA, "Expected comma");
                        }
                        args.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }

                expect(TokenType::RPAREN, "Expected ')'");
                std::string lower = name; std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower == "similarity") {
                    return std::make_shared<SimilarityCallExpr>(std::move(args));
                }
                if (lower == "proximity") {
                    return std::make_shared<ProximityCallExpr>(std::move(args));
                }
                return std::make_shared<FunctionCallExpr>(name, std::move(args));
            }

            // Simple variable
            return std::make_shared<VariableExpr>(name);
        }
        
        throw std::runtime_error("Unexpected token: " + current().value);
    }

    // ========================================================================
    // Mutation Parsing (EPIC-004 Phase 1) — public entry point
    // ========================================================================
public:
    /// @brief Entry point for DML statement parsing.
    ///
    /// Dispatches to the appropriate parseXxxStatement() method based on the
    /// leading keyword (INSERT | UPDATE | DELETE | REMOVE | REPLACE | UPSERT).
    ///
    /// @return Parsed MutationNode or a parse error.
    Result<std::shared_ptr<MutationNode>> parseMutation() {
        try {
            // Reject invalid tokens early.
            for (const auto& tok : tokens_) {
                if (tok.type == TokenType::INVALID) {
                    return Err<std::shared_ptr<MutationNode>>(
                        errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                        fmt::format("Invalid token '{}' at line {}, column {}",
                                    tok.value, tok.line, tok.column));
                }
            }

            std::shared_ptr<MutationNode> node;
            if (match(TokenType::INSERT)) {
                node = parseInsertStatement();
            } else if (match(TokenType::UPDATE)) {
                node = parseUpdateStatement();
            } else if (match(TokenType::DELETE)) {
                node = parseDeleteStatement();
            } else if (match(TokenType::REMOVE)) {
                node = parseRemoveStatement();
            } else if (match(TokenType::REPLACE)) {
                node = parseReplaceStatement();
            } else if (match(TokenType::UPSERT)) {
                node = parseUpsertStatement();
            } else {
                return Err<std::shared_ptr<MutationNode>>(
                    errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                    fmt::format("Expected DML keyword (INSERT/UPDATE/DELETE/REMOVE/REPLACE/UPSERT), got '{}'",
                                current().value));
            }

            // Consume optional trailing semicolons.
            while (match(TokenType::SEMICOLON)) {
              advance();
            }

            if (!match(TokenType::END_OF_FILE)) {
                return Err<std::shared_ptr<MutationNode>>(
                    errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                    fmt::format("Unexpected token '{}' after mutation statement",
                                current().value));
            }
            return Ok(node);
        } catch (const std::runtime_error& e) {
            const auto& tok = current();
            return Err<std::shared_ptr<MutationNode>>(
                errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                fmt::format("Parse error at line {}, column {}: {}",
                            tok.line, tok.column, e.what()));
        }
    }

private:

    // -----------------------------------------------------------------------
    // Helpers for mutation parsing
    // -----------------------------------------------------------------------

    /// @brief Consume the current token if it is an IDENTIFIER, INSERT, UPDATE,
    ///        DELETE, REMOVE, REPLACE, UPSERT, SET, FROM, WHERE, INTO, VALUES,
    ///        or any keyword that may also be used as a bare collection/field
    ///        name in a mutation context.  Returns the token value.
    std::string expectCollectionName(const std::string& context) {
        const TokenType t = current().type;
        // Allow any token type that could serve as a bare identifier in practice.
        static constexpr TokenType kNameableTypes[] = {
            TokenType::IDENTIFIER,
            // Contextual keywords — legal as collection names in AQL
            TokenType::INSERT, TokenType::UPDATE, TokenType::DELETE,
            TokenType::REMOVE, TokenType::REPLACE, TokenType::UPSERT,
            TokenType::SET,    TokenType::FROM,    TokenType::WHERE,
            TokenType::INTO,   TokenType::VALUES,
        };
        for (auto k : kNameableTypes) {
            if (t == k) {
                std::string name = current().value;
                advance();
                // Phase 2 Agent 1: Register collection in scope for mutation statements
                scope_context_.registerCollection(name);
                return name;
            }
        }
        throw std::runtime_error(
            fmt::format("Expected collection name in {} statement, got '{}'",
                        context, current().value));
    }

    /// @brief Parse optional `RETURN NEW` or `RETURN OLD` clause.
    ///        Sets *return_new / *return_old to true when detected.
    void parseReturnClause(bool& return_new, bool& return_old) {
        if (!match(TokenType::RETURN)) {
          return;
        }
        advance(); // consume RETURN
        const std::string val = [&](){
            std::string s = current().value;
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        }();
        if (val == "new") {
            return_new = true;
            advance();
        } else if (val == "old") {
            return_old = true;
            advance();
        }
        // Otherwise treat as no RETURN clause (the expression after RETURN
        // belongs to a wrapping query that the caller might handle).
    }

    // -----------------------------------------------------------------------
    // parseInsertStatement
    // -----------------------------------------------------------------------

    /// @brief Parse INSERT statement in two surface forms.
    ///
    ///   AQL-native: `INSERT doc_expr INTO collection [RETURN NEW]`
    ///   SQL-style:  `INSERT INTO collection VALUES {doc1}[, {doc2}...] [RETURN NEW]`
    std::shared_ptr<MutationNode> parseInsertStatement() {
        expect(TokenType::INSERT, "Expected INSERT");
        auto node = std::make_shared<InsertNode>();

        if (match(TokenType::INTO)) {
            // SQL-style: INSERT INTO collection VALUES {...}
            advance(); // consume INTO
            node->collection = expectCollectionName("INSERT");
            if (!match(TokenType::VALUES)) {
                throw std::runtime_error("Expected VALUES after collection name in INSERT INTO");
            }
            advance(); // consume VALUES
            // Parse one or more comma-separated document expressions.
            do {
                if (!node->documents.empty()) {
                    if (!match(TokenType::COMMA)) {
                      break;
                    }
                    advance();
                }
                node->documents.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        } else {
            // AQL-native: INSERT doc_expr INTO collection
            node->documents.push_back(parseExpression());
            expect(TokenType::INTO, "Expected INTO after document expression in INSERT");
            node->collection = expectCollectionName("INSERT");
        }

        bool dummy_old = false;
        parseReturnClause(node->return_new, dummy_old); // INSERT only supports RETURN NEW
        return node;
    }

    // -----------------------------------------------------------------------
    // parseUpdateStatement
    // -----------------------------------------------------------------------

    /// @brief Parse UPDATE statement in two surface forms.
    ///
    ///   SQL-style:  `UPDATE collection SET k=v [, ...] [WHERE cond] [LIMIT n] [RETURN NEW|OLD]`
    ///   AQL-native: `UPDATE search_expr WITH update_expr IN collection [RETURN NEW|OLD]`
    std::shared_ptr<MutationNode> parseUpdateStatement() {
        expect(TokenType::UPDATE, "Expected UPDATE");
        auto node = std::make_shared<UpdateNode>();

        // Peek ahead: if the next token is an IDENTIFIER followed by SET → SQL style.
        // If the next token starts an expression followed by WITH → AQL-native.
        // Heuristic: if token after the identifier is SET, it is SQL-style.
        const bool sql_style = [&]() -> bool {
            // SQL style: UPDATE <ident> SET ...
            if (current().type == TokenType::IDENTIFIER) {
                return peek().type == TokenType::SET;
            }
            return false;
        }();

        if (sql_style) {
            // SQL-style
            node->collection = expectCollectionName("UPDATE");
            expect(TokenType::SET, "Expected SET after collection name in UPDATE");

            // Parse k=v pairs.
            do {
                if (!node->set_clauses.empty()) {
                    if (!match(TokenType::COMMA)) {
                      break;
                    }
                    advance();
                }
                SetClause sc;
                if (!match(TokenType::IDENTIFIER)) {
                    throw std::runtime_error("Expected field name in SET clause");
                }
                sc.field = current().value;
                advance();
                // Accept both '=' (ASSIGN) and '==' (EQ) for robustness.
                if (!match(TokenType::ASSIGN) && !match(TokenType::EQ)) {
                    throw std::runtime_error("Expected '=' after field name in SET clause");
                }
                advance();
                sc.value = parseExpression();
                node->set_clauses.push_back(std::move(sc));
            } while (match(TokenType::COMMA));

            // Optional WHERE
            if (match(TokenType::WHERE) || match(TokenType::FILTER)) {
                advance();
                node->filter = parseExpression();
            }
        } else {
            // AQL-native: UPDATE search_expr WITH update_expr IN collection
            node->search_expr = parseExpression();
            expect(TokenType::WITH, "Expected WITH after search expression in UPDATE");
            node->update_expr = parseExpression();
            expect(TokenType::IN, "Expected IN after update expression");
            node->collection = expectCollectionName("UPDATE");
        }

        // Optional LIMIT
        if (match(TokenType::LIMIT)) {
            advance();
            if (!match(TokenType::INTEGER)) {
                throw std::runtime_error("Expected integer after LIMIT in UPDATE");
            }
            node->limit = std::stoll(current().value);
            advance();
        }

        parseReturnClause(node->return_new, node->return_old);
        return node;
    }

    // -----------------------------------------------------------------------
    // parseDeleteStatement
    // -----------------------------------------------------------------------

    /// @brief Parse DELETE (SQL-style alias for REMOVE).
    ///
    ///   `DELETE FROM collection [WHERE cond] [LIMIT n] [RETURN OLD]`
    std::shared_ptr<MutationNode> parseDeleteStatement() {
        expect(TokenType::DELETE, "Expected DELETE");
        auto node = std::make_shared<RemoveNode>();

        expect(TokenType::FROM, "Expected FROM after DELETE");
        node->collection = expectCollectionName("DELETE");

        // Optional WHERE
        if (match(TokenType::WHERE) || match(TokenType::FILTER)) {
            advance();
            node->filter = parseExpression();
        }

        // Optional LIMIT
        if (match(TokenType::LIMIT)) {
            advance();
            if (!match(TokenType::INTEGER)) {
                throw std::runtime_error("Expected integer after LIMIT in DELETE");
            }
            node->limit = std::stoll(current().value);
            advance();
        }

        bool dummy_new = false;
        parseReturnClause(dummy_new, node->return_removed);
        return node;
    }

    // -----------------------------------------------------------------------
    // parseRemoveStatement
    // -----------------------------------------------------------------------

    /// @brief Parse AQL-native REMOVE statement.
    ///
    ///   `REMOVE doc_expr IN collection [RETURN OLD]`
    std::shared_ptr<MutationNode> parseRemoveStatement() {
        expect(TokenType::REMOVE, "Expected REMOVE");
        auto node = std::make_shared<RemoveNode>();

        node->doc_expr = parseExpression();
        expect(TokenType::IN, "Expected IN after document expression in REMOVE");
        node->collection = expectCollectionName("REMOVE");

        bool dummy_new = false;
        parseReturnClause(dummy_new, node->return_removed);
        return node;
    }

    // -----------------------------------------------------------------------
    // parseReplaceStatement
    // -----------------------------------------------------------------------

    /// @brief Parse REPLACE statement.
    ///
    ///   `REPLACE search_expr WITH replacement IN collection [RETURN NEW|OLD]`
    std::shared_ptr<MutationNode> parseReplaceStatement() {
        expect(TokenType::REPLACE, "Expected REPLACE");
        auto node = std::make_shared<ReplaceNode>();

        node->search_expr = parseExpression();
        expect(TokenType::WITH, "Expected WITH after search expression in REPLACE");
        node->replacement = parseExpression();
        expect(TokenType::IN, "Expected IN after replacement expression in REPLACE");
        node->collection = expectCollectionName("REPLACE");

        parseReturnClause(node->return_new, node->return_old);
        return node;
    }

    // -----------------------------------------------------------------------
    // parseUpsertStatement
    // -----------------------------------------------------------------------

    /// @brief Parse UPSERT statement.
    ///
    ///   `UPSERT search_expr INSERT insert_doc UPDATE update_doc IN collection [RETURN NEW|OLD]`
    std::shared_ptr<MutationNode> parseUpsertStatement() {
        expect(TokenType::UPSERT, "Expected UPSERT");
        auto node = std::make_shared<UpsertNode>();

        node->search_expr = parseExpression();
        expect(TokenType::INSERT, "Expected INSERT after search expression in UPSERT");
        node->insert_doc = parseExpression();
        expect(TokenType::UPDATE, "Expected UPDATE after insert document in UPSERT");
        node->update_doc = parseExpression();
        expect(TokenType::IN, "Expected IN after update expression in UPSERT");
        node->collection = expectCollectionName("UPSERT");

        parseReturnClause(node->return_new, node->return_old);
        return node;
    }
};

// ============================================================================
// Parser Implementation
// ============================================================================

Result<std::shared_ptr<Query>> AQLParser::parse(const std::string& query_string) {
    try {
        // Tokenize
        Tokenizer tokenizer(query_string);
        auto tokens = tokenizer.tokenize();
        
        // Parse
        Parser parser(std::move(tokens));
        return parser.parse();
        
    } catch (const std::exception& e) {
        return Err<std::shared_ptr<Query>>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            fmt::format("Failed to parse query: {}", e.what())
        );
    }
}

std::shared_ptr<Expression> AQLParser::parseExpression(const std::string& expr_str) {
    Tokenizer tokenizer(expr_str);
    auto tokens = tokenizer.tokenize();

    Parser parser(std::move(tokens));
    auto result = parser.parseStandaloneExpression();
    if (!result) {
        throw std::runtime_error(result.error().message());
    }

    return *result;
}

std::shared_ptr<Expression> AQLParser::parsePrimaryExpression(const std::string& expr_str) {
    return parseExpression(expr_str);
}

BinaryOperator AQLParser::stringToOperator(const std::string& op_str) {
    if (op_str == "==") {
      return BinaryOperator::Eq;
    }
    if (op_str == "!=") {
      return BinaryOperator::Neq;
    }
    if (op_str == "<") {
      return BinaryOperator::Lt;
    }
    if (op_str == "<=") {
      return BinaryOperator::Lte;
    }
    if (op_str == ">") {
      return BinaryOperator::Gt;
    }
    if (op_str == ">=") {
      return BinaryOperator::Gte;
    }
    if (op_str == "AND") {
      return BinaryOperator::And;
    }
    if (op_str == "OR") {
      return BinaryOperator::Or;
    }
    if (op_str == "XOR") {
      return BinaryOperator::Xor;
    }
    if (op_str == "+") {
      return BinaryOperator::Add;
    }
    if (op_str == "-") {
      return BinaryOperator::Sub;
    }
    if (op_str == "*") {
      return BinaryOperator::Mul;
    }
    if (op_str == "/") {
      return BinaryOperator::Div;
    }
    if (op_str == "%") {
      return BinaryOperator::Mod;
    }
    if (op_str == "IN") {
      return BinaryOperator::In;
    }
    throw std::runtime_error("Unknown operator: " + op_str);
}

std::shared_ptr<Expression> AQLParser::parseMembership(std::shared_ptr<Expression> left) {
    auto nullExpr = std::make_shared<LiteralExpr>(nullptr);
    return std::make_shared<BinaryOpExpr>(BinaryOperator::In, std::move(left), std::move(nullExpr));
}

// JSON Serialization moved to src/query/aql_parser_json.cpp to reduce
// compile-time pressure on this translation unit.

// ============================================================================
// Multi-Statement Transaction Block Parsing
// ============================================================================

Result<AqlTransactionBlock> AQLParser::parseTransactionBlock(const std::string& input) {
    try {
        // Tokenize the full input
        Tokenizer tokenizer(input);
        auto tokens = tokenizer.tokenize();

        // Validate: must start with BEGIN
        if (tokens.empty() || tokens[0].type != TokenType::BEGIN) {
            return Err<AqlTransactionBlock>(
                errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                "Multi-statement transaction block must start with BEGIN"
            );
        }

        AqlTransactionBlock block;

        // Walk through the token stream slicing out individual statements.
        // Each statement starts at FOR, WITH, or a DML keyword (INSERT/UPDATE/
        // DELETE/REMOVE/REPLACE/UPSERT) and ends just before the next
        // top-level separator (';') or statement-start/COMMIT/ROLLBACK/EOF.
        size_t start = 1; // skip BEGIN token
        const size_t n = tokens.size();

        auto isMutationStart = [](TokenType t) {
            return t == TokenType::INSERT  || t == TokenType::UPDATE ||
                   t == TokenType::DELETE  || t == TokenType::REMOVE ||
                   t == TokenType::REPLACE || t == TokenType::UPSERT;
        };
        auto isStatementStart = [&isMutationStart](TokenType t) {
            return t == TokenType::FOR || t == TokenType::WITH || isMutationStart(t);
        };
        auto isTerminator = [](TokenType t) {
            return t == TokenType::COMMIT || t == TokenType::ROLLBACK || t == TokenType::END_OF_FILE;
        };
        auto isSeparator = [](TokenType t) {
            return t == TokenType::SEMICOLON;
        };

        while (start < n) {
            // Allow PostgreSQL-like optional semicolons between BEGIN/statement/terminator.
            while (start < n && isSeparator(tokens[start].type)) {
                ++start;
            }
            if (start >= n || isTerminator(tokens[start].type)) {
                break;
            }

            if (!isStatementStart(tokens[start].type)) {
                return Err<AqlTransactionBlock>(
                    errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                    fmt::format(
                        "Expected FOR, WITH, or DML keyword at line {}, column {} inside transaction block",
                        tokens[start].line, tokens[start].column)
                );
            }

            const bool isMutationStmt = isMutationStart(tokens[start].type);
            const bool startsWithClause = (!isMutationStmt && tokens[start].type == TokenType::WITH);
            bool consumedTopLevelForAfterWith = false;

            // Find the end of this statement, tracking nesting depth so that
            // FOR/WITH tokens inside parentheses (e.g. CTE subqueries in WITH
            // clauses) are not mistakenly treated as new statement boundaries.
            size_t end = start + 1;
            int depth = 0; // tracks LPAREN/RPAREN nesting
            while (end < n) {
                const auto& tok = tokens[end];
                if (tok.type == TokenType::LPAREN) {
                    ++depth;
                } else if (tok.type == TokenType::RPAREN) {
                    // Clamp to 0: an extra ')' at top level means the
                    // statement is malformed, which the sub-parser will
                    // report when we hand it the slice below.
                    if (depth > 0) {
                      --depth;
                    }
                }
                // WITH-statements must include their first top-level FOR
                if (depth == 0 && startsWithClause && tok.type == TokenType::FOR && !consumedTopLevelForAfterWith) {
                    consumedTopLevelForAfterWith = true;
                    ++end;
                    continue;
                }

                // Only recognise statement/block boundaries at the top level
                if (depth == 0 && (isSeparator(tok.type) || isStatementStart(tok.type) || isTerminator(tok.type))) {
                    break;
                }
                ++end;
            }

            // Build a sub-token list for this statement (include an EOF sentinel)
            std::vector<Token> sub(tokens.begin() + start, tokens.begin() + end);
            sub.emplace_back(TokenType::END_OF_FILE, "", 0, 0);

            Parser subParser(std::move(sub));

            if (isMutationStmt) {
                // Phase 4: DML statement — parse as a MutationNode.
                auto mutResult = subParser.parseMutation();
                if (!mutResult) {
                    return Err<AqlTransactionBlock>(
                        mutResult.error().code(),
                        fmt::format("Error in mutation statement {} of transaction block: {}",
                                    block.ordered_statements.size() + 1,
                                    mutResult.error().message())
                    );
                }
                AqlStatement s;
                s.kind     = AqlStatement::Kind::Mutation;
                s.mutation = std::move(*mutResult);
                block.ordered_statements.push_back(std::move(s));
            } else {
                auto stmtResult = subParser.parse();
                if (!stmtResult) {
                    return Err<AqlTransactionBlock>(
                        stmtResult.error().code(),
                        fmt::format("Error in statement {} of transaction block: {}",
                                    block.ordered_statements.size() + block.statements.size() + 1,
                                    stmtResult.error().message())
                    );
                }
                AqlStatement s;
                s.kind  = AqlStatement::Kind::Query;
                s.query = *stmtResult;
                block.ordered_statements.push_back(std::move(s));
                block.statements.push_back(std::move(*stmtResult));
            }
            start = end;

            // Consume one separator here; additional separators are consumed
            // at the top of the next iteration.
            if (start < n && isSeparator(tokens[start].type)) {
                ++start;
            }
        }

        if (start >= n || !isTerminator(tokens[start].type)) {
            return Err<AqlTransactionBlock>(
                errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                "Transaction block is missing COMMIT or ROLLBACK"
            );
        }

        // Determine terminator
        if (tokens[start].type == TokenType::COMMIT) {
            block.action = AqlTransactionAction::Commit;
        } else if (tokens[start].type == TokenType::ROLLBACK) {
            block.action = AqlTransactionAction::Rollback;
        } else {
            return Err<AqlTransactionBlock>(
                errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                "Transaction block is missing COMMIT or ROLLBACK"
            );
        }

        // Accept optional trailing semicolons after COMMIT/ROLLBACK, but no
        // additional tokens.
        ++start;
        while (start < n && isSeparator(tokens[start].type)) {
            ++start;
        }
        if (start < n && tokens[start].type != TokenType::END_OF_FILE) {
            return Err<AqlTransactionBlock>(
                errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                fmt::format(
                    "Unexpected token '{}' after transaction terminator at line {}, column {}",
                    tokens[start].value,
                    tokens[start].line,
                    tokens[start].column)
            );
        }

        return Ok(std::move(block));

    } catch (const std::exception& e) {
        return Err<AqlTransactionBlock>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            fmt::format("Failed to parse transaction block: {}", e.what())
        );
    }
}

// ============================================================================
// CQL DDL Parser (Phase 8.1)
// ============================================================================

/**
 * @brief Tokenise @p input into whitespace-separated uppercase words plus
 *        special single-character tokens: '(', ')', ','.
 *
 * This is deliberately simpler than the full AQL Tokenizer: DDL statements
 * have a fixed keyword structure and only require the RETURN body to be
 * preserved verbatim.  We therefore split until we hit "RETURN", then
 * capture everything that follows as the AQL body.
 */
static std::vector<std::string> tokeniseDdl(const std::string& input) {
    std::vector<std::string> tokens;
    size_t i = 0;
    const size_t n = input.size();

    // Convert the part before RETURN to uppercase tokens; capture RETURN body.
    while (i < n) {
        // Skip whitespace
        while (i < n && std::isspace(static_cast<unsigned char>(input[i]))) {
          ++i;
        }
        if (i >= n) {
          break;
        }

        char ch = input[i];

        if (ch == '(' || ch == ')' || ch == ',') {
            tokens.push_back(std::string(1, ch));
            ++i;
            continue;
        }

        // Collect a word token
        size_t start = i;
        while (i < n && !std::isspace(static_cast<unsigned char>(input[i]))
               && input[i] != '(' && input[i] != ')' && input[i] != ',') {
            ++i;
        }

        std::string word = input.substr(start, i - start);
        // Uppercase for keyword comparison
        std::string upper = word;
        std::transform(upper.begin(), upper.end(), upper.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        tokens.push_back(std::move(upper));
    }
    return tokens;
}

Result<ContinuousQueryDDL> AQLParser::parseDDL(const std::string& input) {
    // ── helpers ──────────────────────────────────────────────────────────────
    auto make_err = [](const std::string& msg) -> Result<ContinuousQueryDDL> {
        return Err<ContinuousQueryDDL>(
            errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX, msg);
    };

    // Strip leading/trailing whitespace
    std::string trimmed = input;
    {
        size_t s = trimmed.find_first_not_of(" \t\n\r");
        if (s == std::string::npos) {
            return make_err("Empty DDL statement");
        }
        size_t e = trimmed.find_last_not_of(" \t\n\r");
        trimmed = trimmed.substr(s, e - s + 1);
    }

    // Convert to uppercase for keyword matching
    std::string upper = trimmed;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    // ── SHOW CONTINUOUS QUERIES ───────────────────────────────────────────────
    if (upper == "SHOW CONTINUOUS QUERIES" ||
        upper.rfind("SHOW CONTINUOUS QUERIES", 0) == 0) {
        ContinuousQueryDDL ddl;
        ddl.ddl_type = ContinuousQueryDDLType::SHOW;
        return Ok(std::move(ddl));
    }

    auto tokens = tokeniseDdl(trimmed);
    if (tokens.empty()) {
        return make_err("Empty DDL statement");
    }

    // Peek helper — returns empty string when out of range
    auto tok = [&]([[maybe_unused]] size_t idx) -> const std::string& {
        static const std::string empty;
        return idx < tokens.size() ? tokens[idx] : empty;
    };

    const std::string& kw0 = tok(0);

    // ── DROP CONTINUOUS QUERY <name> ─────────────────────────────────────────
    if (kw0 == "DROP") {
        if (tok(1) != "CONTINUOUS" || tok(2) != "QUERY") {
            return make_err("Expected: DROP CONTINUOUS QUERY <name>");
        }
        if (tok(3).empty()) {
            return make_err("DROP CONTINUOUS QUERY requires a query name");
        }
        ContinuousQueryDDL ddl;
        ddl.ddl_type   = ContinuousQueryDDLType::DROP;
        ddl.query_name = tok(3);
        return Ok(std::move(ddl));
    }

    // ── DESCRIBE CONTINUOUS QUERY <name> ─────────────────────────────────────
    if (kw0 == "DESCRIBE") {
        if (tok(1) != "CONTINUOUS" || tok(2) != "QUERY") {
            return make_err("Expected: DESCRIBE CONTINUOUS QUERY <name>");
        }
        if (tok(3).empty()) {
            return make_err("DESCRIBE CONTINUOUS QUERY requires a query name");
        }
        ContinuousQueryDDL ddl;
        ddl.ddl_type   = ContinuousQueryDDLType::DESCRIBE;
        ddl.query_name = tok(3);
        return Ok(std::move(ddl));
    }

    // ── SHOW CONTINUOUS QUERIES (tokenised path) ──────────────────────────────
    if (kw0 == "SHOW") {
        if (tok(1) != "CONTINUOUS" || tok(2) != "QUERIES") {
            return make_err("Expected: SHOW CONTINUOUS QUERIES");
        }
        ContinuousQueryDDL ddl;
        ddl.ddl_type = ContinuousQueryDDLType::SHOW;
        return Ok(std::move(ddl));
    }

    // ── CREATE CONTINUOUS QUERY <name> ON <collection>
    //         WINDOW TIME(<r>,<s>) | COUNT(<rows>,<slide>) | TUMBLING(<i>)
    //         RETURN <aql_body> ────────────────────────────────────────────────
    if (kw0 != "CREATE") {
        return make_err(
            fmt::format("Unknown CQL DDL keyword '{}'; expected CREATE, DROP, SHOW, or DESCRIBE",
                        kw0));
    }

    if (tok(1) != "CONTINUOUS" || tok(2) != "QUERY") {
        return make_err("Expected: CREATE CONTINUOUS QUERY <name> ...");
    }

    const std::string& query_name = tok(3);
    if (query_name.empty() || query_name == "ON") {
        return make_err("CREATE CONTINUOUS QUERY requires a non-empty query name");
    }

    if (tok(4) != "ON") {
        return make_err("Expected ON <collection> after query name");
    }

    const std::string& collection = tok(5);
    if (collection.empty() || collection == "WINDOW") {
        return make_err("CREATE CONTINUOUS QUERY requires a non-empty source collection");
    }

    if (tok(6) != "WINDOW") {
        return make_err("Expected WINDOW keyword after collection name");
    }

    // tok(7) = window function name: TIME | COUNT | TUMBLING
    const std::string& win_func = tok(7);

    // tok(8) = '('
    if (tok(8) != "(") {
        return make_err("Expected '(' after WINDOW function name");
    }

    ContinuousQueryDDL ddl;
    ddl.ddl_type   = ContinuousQueryDDLType::CREATE;
    ddl.query_name = query_name;
    ddl.spec.source_collection = collection;
    ddl.spec.window_type       = win_func;

    // Parse WINDOW arguments; find matching ')'
    // We expect a flat comma-separated list of integer literals.
    size_t arg_start = 9;  // first token after '('
    std::vector<int64_t> args;
    size_t ti = arg_start;
    while (ti < tokens.size() && tok(ti) != ")") {
        const std::string& t = tok(ti);
        if (t == ",") { ++ti; continue; }
        // Must be an integer literal
        bool is_num = !t.empty() &&
                      std::all_of(t.begin(), t.end(),
                                  [](char c){ return std::isdigit(static_cast<unsigned char>(c)); });
        if (!is_num) {
            return make_err(fmt::format("Non-numeric window argument '{}' in WINDOW clause", t));
        }
        try {
            args.push_back(std::stoll(t));
        } catch (...) {
            THEMIS_WARN("aql_parser: unhandled exception caught");
            return make_err(fmt::format("Invalid window argument '{}' in WINDOW clause", t));
        }
        ++ti;
    }

    if (tok(ti) != ")") {
        return make_err("Unterminated WINDOW argument list — missing ')'");
    }
    size_t after_paren = ti + 1;  // token index after the closing ')'

    if (win_func == "TIME") {
        if (args.size() < 2) {
            return make_err("WINDOW TIME requires two arguments: TIME(<range_ms>, <slide_ms>)");
        }
        ddl.spec.range_ms = args[0];
        ddl.spec.slide_ms = args[1];
    } else if (win_func == "COUNT") {
        if (args.size() < 2) {
            return make_err("WINDOW COUNT requires two arguments: COUNT(<rows>, <slide_rows>)");
        }
        ddl.spec.rows       = args[0];
        ddl.spec.slide_rows = args[1];
    } else if (win_func == "TUMBLING") {
        if (args.empty()) {
            return make_err("WINDOW TUMBLING requires one argument: TUMBLING(<interval_ms>)");
        }
        ddl.spec.range_ms = args[0];
    } else {
        return make_err(
            fmt::format("Unknown window type '{}'; expected TIME, COUNT, or TUMBLING", win_func));
    }

    // Expect RETURN keyword next
    if (tok(after_paren) != "RETURN") {
        return make_err(
            fmt::format("Expected RETURN after WINDOW clause, got '{}'", tok(after_paren)));
    }

    // The AQL body is everything after the RETURN keyword in the *original* input,
    // preserving case and whitespace for correct AQL evaluation later.
    {
        // Find the position of "RETURN" in the original (case-insensitive search)
        std::string::size_type return_pos = std::string::npos;
        // Walk upper string to find standalone RETURN keyword
        std::string needle = "RETURN";
        size_t search_from = 0;
        while (true) {
            size_t found = upper.find(needle, search_from);
            if (found == std::string::npos) {
              break;
            }
            // Check word boundary
            bool left_ok  = (found == 0) || !std::isalnum(static_cast<unsigned char>(upper[found - 1]));
            bool right_ok = (found + needle.size() >= upper.size()) ||
                            !std::isalnum(static_cast<unsigned char>(upper[found + needle.size()]));
            if (left_ok && right_ok) {
                return_pos = found;
                break;
            }
            search_from = found + 1;
        }

        if (return_pos == std::string::npos) {
            return make_err("CREATE CONTINUOUS QUERY is missing a RETURN clause");
        }

        std::string body = trimmed.substr(return_pos + needle.size());
        // Trim leading whitespace from body
        size_t bs = body.find_first_not_of(" \t\n\r");
        ddl.spec.aql_body = (bs == std::string::npos) ? "" : body.substr(bs);
        if (ddl.spec.aql_body.empty()) {
            return make_err("RETURN clause must not be empty");
        }
    }

    return Ok(std::move(ddl));
}

// ============================================================================
// AQLParser::parseMutation
// ============================================================================

/**
 * @brief Parse a DML mutation statement.
 *
 * Tokenises @p input, constructs an inner Parser, and delegates to
 * Parser::parseMutation() which dispatches on the leading DML keyword.
 *
 * @param input  Raw AQL mutation string (case-insensitive keywords).
 * @return       Ok(MutationNode) on success, Err on parse failure.
 */
Result<std::shared_ptr<MutationNode>> AQLParser::parseMutation(const std::string& input) {
    Tokenizer tokenizer(input);
    auto tokens = tokenizer.tokenize();
    Parser p(std::move(tokens));
    return p.parseMutation();
}

// ============================================================================
// AQLParser::parseSchemaDDL
// ============================================================================

/// @brief Lightweight token produced by the schema-DDL tokeniser.
///
/// Each token carries both the uppercased form (for keyword comparison) and the
/// original-case form (for identifier names that must preserve user casing).
/// A character offset into the source string is stored so that the view-body
/// extractor can slice the original input without reconstructing it from tokens.
struct SchemaDdlToken {
    std::string upper;    ///< Uppercased value — used for keyword comparison.
    std::string original; ///< Original-case value — used for identifier names.
    size_t      start{0}; ///< Character offset in the source string.
};

/// @brief Tokenise a Schema DDL string into SchemaDdlToken entries.
///
/// Splits on whitespace and treats `(`, `)`, `,` as single-character tokens.
/// Curly braces `{` and `}` are intentionally NOT split so that OPTIONS blocks
/// are left intact for JSON extraction via substring search.
static std::vector<SchemaDdlToken> tokeniseSchemaDdl(const std::string& input) {
    std::vector<SchemaDdlToken> tokens;
    size_t i = 0;
    const size_t n = input.size();

    while (i < n) {
        // Skip whitespace
        while (i < n && std::isspace(static_cast<unsigned char>(input[i]))) {
          ++i;
        }
        if (i >= n) {
          break;
        }

        char ch = input[i];

        // Single-character structural tokens
        if (ch == '(' || ch == ')' || ch == ',') {
            SchemaDdlToken t;
            t.original = std::string(1, ch);
            t.upper    = t.original;
            t.start    = i;
            tokens.push_back(std::move(t));
            ++i;
            continue;
        }

        // Word / identifier token — collect until whitespace or structural char
        size_t word_start = i;
        while (i < n
               && !std::isspace(static_cast<unsigned char>(input[i]))
               && input[i] != '(' && input[i] != ')' && input[i] != ',') {
            ++i;
        }

        SchemaDdlToken t;
        t.original = input.substr(word_start, i - word_start);
        t.upper    = t.original;
        std::transform(t.upper.begin(), t.upper.end(), t.upper.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        t.start = word_start;
        tokens.push_back(std::move(t));
    }
    return tokens;
}

/// @brief Extract a JSON object block `{ … }` from @p source starting at or after @p from_pos.
///
/// Balances curly braces to locate the end of the JSON block.  Returns an empty
/// string if no complete `{…}` block is found.
static std::string extractJsonBlock(const std::string& source, size_t from_pos) {
    size_t brace_open = source.find('{', from_pos);
    if (brace_open == std::string::npos) return {};

    int depth = 0;
    for (size_t i = brace_open; i < source.size(); ++i) {
        if (source[i] == '{')      ++depth;
        else if (source[i] == '}') {
            --depth;
            if (depth == 0) {
              return source.substr(brace_open, i - brace_open + 1);
            }
        }
    }
    return {}; // unbalanced
}

/**
 * @brief Parse a Schema DDL statement into a SchemaDDL AST node.
 *
 * Implements the seven DDL forms listed in the AQL DDL Phase 2 specification.
 * Keyword matching is case-insensitive; identifier names preserve the casing
 * supplied by the caller.
 */
Result<SchemaDDL> AQLParser::parseSchemaDDL(const std::string& input) {
    // ── error helper ──────────────────────────────────────────────────────────
    auto make_err = [](const std::string& msg) -> Result<SchemaDDL> {
        return Err<SchemaDDL>(errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX, msg);
    };

    // ── trim ──────────────────────────────────────────────────────────────────
    std::string trimmed = input;
    {
        size_t s = trimmed.find_first_not_of(" \t\n\r");
        if (s == std::string::npos) {
          return make_err("Empty Schema DDL statement");
        }
        size_t e = trimmed.find_last_not_of(" \t\n\r");
        trimmed = trimmed.substr(s, e - s + 1);
    }

    // ── uppercase for keyword comparisons (same character positions as trimmed)
    std::string upper = trimmed;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    // ── tokenise ──────────────────────────────────────────────────────────────
    const auto tokens = tokeniseSchemaDdl(trimmed);
    if (tokens.empty()) {
      return make_err("Empty Schema DDL statement");
    }

    // Bounds-safe token accessors
    auto tok_up = [&]([[maybe_unused]] size_t idx) -> const std::string& {
        static const std::string empty;
        return idx < tokens.size() ? tokens[idx].upper : empty;
    };
    auto tok_orig = [&]([[maybe_unused]] size_t idx) -> const std::string& {
        static const std::string empty;
        return idx < tokens.size() ? tokens[idx].original : empty;
    };
    auto tok_start = [&]([[maybe_unused]] size_t idx) -> size_t {
        return idx < tokens.size() ? tokens[idx].start : trimmed.size();
    };

    const std::string& kw0 = tok_up(0);

    // ── CREATE ────────────────────────────────────────────────────────────────
    if (kw0 == "CREATE") {
        const std::string& kw1 = tok_up(1);

        // ── CREATE COLLECTION ─────────────────────────────────────────────────
        if (kw1 == "COLLECTION") {
            const std::string& name = tok_orig(2);
            if (name.empty()) {
                return make_err("CREATE COLLECTION requires a collection name");
            }

            SchemaDDL ddl;
            ddl.ddl_type = SchemaDDLType::CREATE_COLLECTION;
            ddl.name     = name;

            // Optional IF NOT EXISTS
            if (tok_up(3) == "IF" && tok_up(4) == "NOT" && tok_up(5) == "EXISTS") {
                ddl.if_exists = true;
            }

            // Optional OPTIONS {…}
            size_t opts_kw = upper.find("OPTIONS", tok_start(2));
            if (opts_kw != std::string::npos) {
                // Verify word boundary on left
                bool lb = (opts_kw == 0) ||
                          !std::isalnum(static_cast<unsigned char>(upper[opts_kw - 1]));
                if (lb) {
                    std::string json_str = extractJsonBlock(trimmed, opts_kw + 7);
                    if (!json_str.empty()) {
                        try {
                            ddl.options = nlohmann::json::parse(json_str);
                        } catch (const nlohmann::json::exception& je) {
                            return make_err(
                                fmt::format("Invalid OPTIONS JSON in CREATE COLLECTION: {}", je.what()));
                        }
                    }
                }
            }
            return Ok(std::move(ddl));
        }

        // ── CREATE [UNIQUE] INDEX ─────────────────────────────────────────────
        if (kw1 == "UNIQUE" || kw1 == "INDEX") {
            const bool is_unique = (kw1 == "UNIQUE");
            const size_t idx_kw  = is_unique ? 2u : 1u; // token index of INDEX keyword

            if (tok_up(idx_kw) != "INDEX") {
                return make_err(
                    fmt::format("Expected INDEX keyword, got '{}'", tok_orig(idx_kw)));
            }

            const std::string& idx_name = tok_orig(idx_kw + 1);
            if (idx_name.empty()) {
                return make_err("CREATE INDEX requires an index name");
            }

            if (tok_up(idx_kw + 2) != "ON") {
                return make_err("Expected ON after index name in CREATE INDEX");
            }

            const std::string& coll = tok_orig(idx_kw + 3);
            if (coll.empty()) {
                return make_err("CREATE INDEX requires a collection name after ON");
            }

            // Expect opening parenthesis
            const size_t paren_open_idx = idx_kw + 4;
            if (tok_up(paren_open_idx) != "(") {
                return make_err(
                    fmt::format("Expected '(' after collection name in CREATE INDEX, got '{}'",
                                tok_orig(paren_open_idx)));
            }

            // Parse comma-separated field names
            std::vector<FieldDef> fields;
            size_t fi = paren_open_idx + 1;
            while (fi < tokens.size() && tok_up(fi) != ")") {
                if (tok_up(fi) == ",") { ++fi; continue; }
                FieldDef f;
                f.name = tok_orig(fi);
                if (f.name.empty()) {
                  break;
                }
                fields.push_back(std::move(f));
                ++fi;
            }
            if (tok_up(fi) != ")") {
                return make_err("Unterminated field list in CREATE INDEX — missing ')'");
            }
            if (fields.empty()) {
                return make_err("CREATE INDEX requires at least one field");
            }

            // Optional modifiers after ')'
            std::string index_type = "hash"; // default
            bool is_sparse   = false;
            bool if_not_exists = false;
            for (size_t mi = fi + 1; mi < tokens.size(); ++mi) {
                if (tok_up(mi) == "TYPE" && mi + 1 < tokens.size()) {
                    index_type = tok_orig(mi + 1);
                    ++mi;
                } else if (tok_up(mi) == "SPARSE") {
                    is_sparse = true;
                } else if (tok_up(mi) == "IF"
                           && tok_up(mi + 1) == "NOT"
                           && tok_up(mi + 2) == "EXISTS") {
                    if_not_exists = true;
                    mi += 2;
                }
            }

            SchemaDDL ddl;
            ddl.ddl_type   = SchemaDDLType::CREATE_INDEX;
            ddl.name       = idx_name;
            ddl.collection = coll;
            ddl.if_exists  = if_not_exists;
            ddl.fields     = fields;

            ddl.index_def.name       = idx_name;
            ddl.index_def.collection = coll;
            ddl.index_def.fields     = fields;
            ddl.index_def.unique     = is_unique;
            ddl.index_def.sparse     = is_sparse;
            ddl.index_def.index_type = index_type;

            return Ok(std::move(ddl));
        }

        // ── CREATE VIEW ───────────────────────────────────────────────────────
        if (kw1 == "VIEW") {
            const std::string& view_name = tok_orig(2);
            if (view_name.empty()) {
                return make_err("CREATE VIEW requires a view name");
            }

            // Determine if IF NOT EXISTS appears before AS
            bool if_not_exists = false;
            size_t as_token_idx = 3; // expected index of "AS"

            if (tok_up(3) == "IF" && tok_up(4) == "NOT" && tok_up(5) == "EXISTS") {
                if_not_exists  = true;
                as_token_idx   = 6;
            }

            if (tok_up(as_token_idx) != "AS") {
                return make_err(
                    fmt::format("Expected AS after view name in CREATE VIEW, got '{}'",
                                tok_orig(as_token_idx)));
            }

            // Body starts immediately after "AS" — use character offset
            size_t body_start = tok_start(as_token_idx) + 2; // skip the 2-char "AS"
            while (body_start < trimmed.size()
                   && std::isspace(static_cast<unsigned char>(trimmed[body_start]))) {
                ++body_start;
            }
            std::string body = trimmed.substr(body_start);
            if (body.empty()) {
                return make_err("CREATE VIEW requires an AQL body after AS");
            }

            SchemaDDL ddl;
            ddl.ddl_type  = SchemaDDLType::CREATE_VIEW;
            ddl.name      = view_name;
            ddl.view_body = std::move(body);
            ddl.if_exists = if_not_exists;
            return Ok(std::move(ddl));
        }

        return make_err(
            fmt::format("Unknown Schema DDL after CREATE: '{}'; expected COLLECTION, INDEX, UNIQUE, or VIEW",
                        tok_up(1)));
    }

    // ── DROP ──────────────────────────────────────────────────────────────────
    if (kw0 == "DROP") {
        const std::string& kw1 = tok_up(1);

        // ── DROP COLLECTION ───────────────────────────────────────────────────
        if (kw1 == "COLLECTION") {
            const std::string& name = tok_orig(2);
            if (name.empty()) {
                return make_err("DROP COLLECTION requires a collection name");
            }
            SchemaDDL ddl;
            ddl.ddl_type = SchemaDDLType::DROP_COLLECTION;
            ddl.name     = name;
            if (tok_up(3) == "IF" && tok_up(4) == "EXISTS") {
                ddl.if_exists = true;
            }
            return Ok(std::move(ddl));
        }

        // ── DROP INDEX ────────────────────────────────────────────────────────
        if (kw1 == "INDEX") {
            const std::string& idx_name = tok_orig(2);
            if (idx_name.empty()) {
                return make_err("DROP INDEX requires an index name");
            }
            if (tok_up(3) != "ON") {
                return make_err("Expected ON after index name in DROP INDEX");
            }
            const std::string& coll = tok_orig(4);
            if (coll.empty()) {
                return make_err("DROP INDEX requires a collection name after ON");
            }
            SchemaDDL ddl;
            ddl.ddl_type   = SchemaDDLType::DROP_INDEX;
            ddl.name       = idx_name;
            ddl.collection = coll;
            if (tok_up(5) == "IF" && tok_up(6) == "EXISTS") {
                ddl.if_exists = true;
            }
            return Ok(std::move(ddl));
        }

        // ── DROP VIEW ─────────────────────────────────────────────────────────
        if (kw1 == "VIEW") {
            const std::string& view_name = tok_orig(2);
            if (view_name.empty()) {
                return make_err("DROP VIEW requires a view name");
            }
            SchemaDDL ddl;
            ddl.ddl_type = SchemaDDLType::DROP_VIEW;
            ddl.name     = view_name;
            if (tok_up(3) == "IF" && tok_up(4) == "EXISTS") {
                ddl.if_exists = true;
            }
            return Ok(std::move(ddl));
        }

        return make_err(
            fmt::format("Unknown Schema DDL after DROP: '{}'; expected COLLECTION, INDEX, or VIEW",
                        tok_up(1)));
    }

    // ── ALTER ─────────────────────────────────────────────────────────────────
    if (kw0 == "ALTER") {
        if (tok_up(1) != "COLLECTION") {
            return make_err(
                fmt::format("Expected COLLECTION after ALTER, got '{}'", tok_orig(1)));
        }
        const std::string& name = tok_orig(2);
        if (name.empty()) {
            return make_err("ALTER COLLECTION requires a collection name");
        }
        if (tok_up(3) != "SET") {
            return make_err("Expected SET after collection name in ALTER COLLECTION");
        }
        if (tok_up(4) != "OPTIONS") {
            return make_err("Expected OPTIONS after SET in ALTER COLLECTION");
        }

        std::string json_str = extractJsonBlock(trimmed, tok_start(4) + 7 /*"OPTIONS"*/);
        if (json_str.empty()) {
            return make_err("ALTER COLLECTION requires a JSON OPTIONS block after SET OPTIONS");
        }
        nlohmann::json opts;
        try {
            opts = nlohmann::json::parse(json_str);
        } catch (const nlohmann::json::exception& je) {
            return make_err(
                fmt::format("Invalid OPTIONS JSON in ALTER COLLECTION: {}", je.what()));
        }

        SchemaDDL ddl;
        ddl.ddl_type = SchemaDDLType::ALTER_COLLECTION;
        ddl.name     = name;
        ddl.options  = std::move(opts);
        return Ok(std::move(ddl));
    }

    return make_err(
        fmt::format("Unknown Schema DDL keyword '{}'; expected CREATE, DROP, or ALTER", kw0));
}

}  // namespace query
}  // namespace themis


