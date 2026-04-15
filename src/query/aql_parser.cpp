/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_parser.cpp                                     ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:43:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     1324                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • c613ea7a9e  2026-03-04  Refactor error masking and enhance archive processor vali... ║
    • f82bf2ae9f  2026-03-04  Refactor tenant manager tests and add new test cases ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "query/aql_parser.h"
#include "utils/error_registry.h"
#include <cctype>
#include <optional>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <fmt/format.h>

namespace themis {
namespace query {

// ============================================================================
// Tokenizer (Lexer)
// ============================================================================

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
    
    // Operators
    EQ, NEQ, LT, LTE, GT, GTE,
    PLUS, MINUS, STAR, SLASH, MODULO,
    ASSIGN,  // Single '=' for assignments (COLLECT var = expr, AGGREGATE var = func)
    
    // Literals
    IDENTIFIER, STRING, INTEGER, FLOAT,
    
    // Punctuation
    DOT, COMMA, COLON, LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    
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

class Tokenizer {
public:
    explicit Tokenizer(const std::string& input)
        : input_(input), pos_(0), line_(1), column_(1) {}
    
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        
        while (pos_ < input_.size()) {
            skipWhitespace();
            if (pos_ >= input_.size()) break;
            
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
    size_t pos_;
    size_t line_;
    size_t column_;
    
    char peek(size_t offset = 0) const {
        size_t p = pos_ + offset;
        return (p < input_.size()) ? input_[p] : '\0';
    }
    
    char advance() {
        if (pos_ >= input_.size()) return '\0';
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
        
        while (std::isalnum(peek()) || peek() == '_') {
            value += advance();
        }
        
        // Convert to lowercase for keyword matching
        std::string lower = value;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        // Check keywords
        if (lower == "for") return Token(TokenType::FOR, value, line, col);
        if (lower == "in") return Token(TokenType::IN, value, line, col);
        if (lower == "filter") return Token(TokenType::FILTER, value, line, col);
        if (lower == "sort") return Token(TokenType::SORT, value, line, col);
        if (lower == "limit") return Token(TokenType::LIMIT, value, line, col);
    if (lower == "return") return Token(TokenType::RETURN, value, line, col);
    if (lower == "let") return Token(TokenType::LET, value, line, col);
        if (lower == "asc") return Token(TokenType::ASC, value, line, col);
        if (lower == "desc") return Token(TokenType::DESC, value, line, col);
    if (lower == "and") return Token(TokenType::AND, value, line, col);
    if (lower == "or") return Token(TokenType::OR, value, line, col);
    if (lower == "xor") return Token(TokenType::XOR, value, line, col);
        if (lower == "not") return Token(TokenType::NOT, value, line, col);
        if (lower == "true") return Token(TokenType::TRUE, value, line, col);
        if (lower == "false") return Token(TokenType::FALSE, value, line, col);
        if (lower == "null") return Token(TokenType::NULL_LITERAL, value, line, col);
        if (lower == "graph") return Token(TokenType::GRAPH, value, line, col);
        if (lower == "outbound") return Token(TokenType::OUTBOUND, value, line, col);
        if (lower == "inbound") return Token(TokenType::INBOUND, value, line, col);
        if (lower == "any") return Token(TokenType::ANY, value, line, col);
    if (lower == "type") return Token(TokenType::TYPE, value, line, col);
        if (lower == "collect") return Token(TokenType::COLLECT, value, line, col);
        if (lower == "aggregate") return Token(TokenType::AGGREGATE, value, line, col);
        
            // Phase 2: Hybrid Query Keywords
            // Note: SIMILARITY and PROXIMITY are function names, not keywords - should remain as IDENTIFIER
            // if (lower == "similarity") return Token(TokenType::SIMILARITY, value, line, col);
            // if (lower == "proximity") return Token(TokenType::PROXIMITY, value, line, col);
            if (lower == "shortest_path") return Token(TokenType::SHORTEST_PATH, value, line, col);
            if (lower == "to") return Token(TokenType::TO, value, line, col);
        
        // Phase 3: Subqueries & CTEs
        if (lower == "with") return Token(TokenType::WITH, value, line, col);
        if (lower == "as") return Token(TokenType::AS, value, line, col);
        if (lower == "all") return Token(TokenType::ALL, value, line, col);
        if (lower == "satisfies") return Token(TokenType::SATISFIES, value, line, col);

        // Phase 4: Multi-statement transaction AQL
        if (lower == "begin") return Token(TokenType::BEGIN, value, line, col);
        if (lower == "commit") return Token(TokenType::COMMIT, value, line, col);
        if (lower == "rollback") return Token(TokenType::ROLLBACK, value, line, col);
        
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
    
    const Token& current() const {
        return (pos_ < tokens_.size()) ? tokens_[pos_] : tokens_.back();
    }
    
    const Token& peek(size_t offset = 1) const {
        size_t p = pos_ + offset;
        return (p < tokens_.size()) ? tokens_[p] : tokens_.back();
    }
    
    void advance() {
        if (pos_ < tokens_.size()) pos_++;
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
    
    std::shared_ptr<Query> parseQuery(bool isSubquery = false) {
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
            ForNode node;
            node.variable = varVertex;
            node.collection = collection;
            return node;
        }

        if (match(TokenType::INTEGER)) {
            // Parse min..max
            int minDepth = std::stoi(current().value);
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
            int maxDepth = std::stoi(current().value);
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
            if (varEdge) trav->varEdge = *varEdge;
            if (varPath) trav->varPath = *varPath;
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
        int64_t first = std::stoll(current().value);
        advance();
        
        if (match(TokenType::COMMA)) {
            advance();
            if (!match(TokenType::INTEGER)) {
                throw std::runtime_error("Expected integer after comma in LIMIT");
            }
            int64_t second = std::stoll(current().value);
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
                if (!first) advance();
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
            int64_t value = std::stoll(current().value);
            advance();
            return std::make_shared<LiteralExpr>(value);
        }
        if (match(TokenType::FLOAT)) {
            double value = std::stod(current().value);
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
                        if (!args.empty()) expect(TokenType::COMMA, "Expected comma");
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
    if (op_str == "==") return BinaryOperator::Eq;
    if (op_str == "!=") return BinaryOperator::Neq;
    if (op_str == "<") return BinaryOperator::Lt;
    if (op_str == "<=") return BinaryOperator::Lte;
    if (op_str == ">") return BinaryOperator::Gt;
    if (op_str == ">=") return BinaryOperator::Gte;
    if (op_str == "AND") return BinaryOperator::And;
    if (op_str == "OR") return BinaryOperator::Or;
    if (op_str == "XOR") return BinaryOperator::Xor;
    if (op_str == "+") return BinaryOperator::Add;
    if (op_str == "-") return BinaryOperator::Sub;
    if (op_str == "*") return BinaryOperator::Mul;
    if (op_str == "/") return BinaryOperator::Div;
    if (op_str == "%") return BinaryOperator::Mod;
    if (op_str == "IN") return BinaryOperator::In;
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
        // Each statement starts at FOR (or WITH) and ends just before the
        // next FOR/WITH/COMMIT/ROLLBACK/END_OF_FILE.
        size_t start = 1; // skip BEGIN token
        const size_t n = tokens.size();

        auto isStatementStart = [](TokenType t) {
            return t == TokenType::FOR || t == TokenType::WITH;
        };
        auto isTerminator = [](TokenType t) {
            return t == TokenType::COMMIT || t == TokenType::ROLLBACK || t == TokenType::END_OF_FILE;
        };

        while (start < n && !isTerminator(tokens[start].type)) {
            if (!isStatementStart(tokens[start].type)) {
                return Err<AqlTransactionBlock>(
                    errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                    fmt::format(
                        "Expected FOR or WITH at line {}, column {} inside transaction block",
                        tokens[start].line, tokens[start].column)
                );
            }

            const bool startsWithClause = (tokens[start].type == TokenType::WITH);
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
                    if (depth > 0) --depth;
                }
                // WITH-statements must include their first top-level FOR
                if (depth == 0 && startsWithClause && tok.type == TokenType::FOR && !consumedTopLevelForAfterWith) {
                    consumedTopLevelForAfterWith = true;
                    ++end;
                    continue;
                }

                // Only recognise statement/block boundaries at the top level
                if (depth == 0 && (isStatementStart(tok.type) || isTerminator(tok.type))) {
                    break;
                }
                ++end;
            }

            // Build a sub-token list for this statement (include an EOF sentinel)
            std::vector<Token> sub(tokens.begin() + start, tokens.begin() + end);
            sub.emplace_back(TokenType::END_OF_FILE, "", 0, 0);

            Parser subParser(std::move(sub));
            auto stmtResult = subParser.parse();
            if (!stmtResult) {
                return Err<AqlTransactionBlock>(
                    stmtResult.error().code(),
                    fmt::format("Error in statement {} of transaction block: {}",
                                block.statements.size() + 1,
                                stmtResult.error().message())
                );
            }
            block.statements.push_back(std::move(*stmtResult));
            start = end;
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

        return Ok(std::move(block));

    } catch (const std::exception& e) {
        return Err<AqlTransactionBlock>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            fmt::format("Failed to parse transaction block: {}", e.what())
        );
    }
}

}  // namespace query
}  // namespace themis
