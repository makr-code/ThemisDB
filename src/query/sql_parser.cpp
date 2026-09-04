/**
 * @file sql_parser.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.16
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SQL dialect compatibility layer – SELECT/INSERT/UPDATE/DELETE passthrough.
// Implements a standalone SQL tokenizer + recursive-descent parser and an
// AQL transpiler.  The generated AQL can be fed directly into executeAql().
//
// Thread-safety note: SQLParser instances are NOT thread-safe.
// Create one instance per thread or protect with a mutex.

#include "query/sql_parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "utils/error_registry.h"
#include "utils/logger.h"

namespace themis {
namespace query {

// ============================================================================
// Helpers – value serialisation shared by parser and transpiler
// ============================================================================

namespace {

std::string sqlValueToAQL(const SQLValue& val) {
    return std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss;
            oss << v;
            return oss.str();
        } else {
            // std::string – emit as a quoted AQL string literal
            std::string out;
            out.reserve(v.size() + 2);
            out += '"';
            for (char c : v) {
                if (c == '"') {
                  out += "\\\"";
                }
                else if (c == '\\') out += "\\\\";
                else if (c == '\n') out += "\\n";
                else if (c == '\r') out += "\\r";
                else if (c == '\t') out += "\\t";
                else out += c;
            }
            out += '"';
            return out;
        }
    }, val);
}

} // anonymous namespace

// ============================================================================
// SQLExpr toAQL implementations
// ============================================================================

std::string SQLLiteralExpr::toAQL(const std::string& /*var*/) const {
    return sqlValueToAQL(value);
}

std::string SQLColumnExpr::toAQL(const std::string& var) const {
    if (column == "*") {
      return var;
    }
    return var + "." + column;
}

std::string SQLBinaryOpExpr::toAQL(const std::string& var) const {
    if (op == "LIKE") {
        // AQL LIKE function: LIKE(doc.col, "pattern")
        return "LIKE(" + left->toAQL(var) + ", " + right->toAQL(var) + ")";
    }
    if (op == "IN") {
        return left->toAQL(var) + " IN " + right->toAQL(var);
    }
    return left->toAQL(var) + " " + op + " " + right->toAQL(var);
}

std::string SQLUnaryOpExpr::toAQL(const std::string& var) const {
    if (op == "NOT") {
        return "NOT " + operand->toAQL(var);
    }
    if (op == "IS_NULL") {
        return operand->toAQL(var) + " == null";
    }
    if (op == "IS_NOT_NULL") {
        return operand->toAQL(var) + " != null";
    }
    return operand->toAQL(var);
}

std::string SQLListExpr::toAQL(const std::string& var) const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) {
          oss << ", ";
        }
        oss << elements[i]->toAQL(var);
    }
    oss << "]";
    return oss.str();
}

// ============================================================================
// Lexer / Tokenizer
// ============================================================================

namespace {

enum class SQLTokenType {
    // Keywords
    SELECT, FROM, WHERE, ORDER, BY, ASC, DESC, LIMIT, OFFSET,
    INSERT, INTO, VALUES,
    UPDATE, SET,
    DELETE,
    AND, OR, NOT, IN, LIKE,
    IS, NUL, // NULL keyword (NUL to avoid conflict with NULL macro)
    TRUE_KW, FALSE_KW,
    AS, DISTINCT,
    // Operators
    EQ,     // =
    NEQ,    // != or <>
    LT,     // <
    LTE,    // <=
    GT,     // >
    GTE,    // >=
    // Punctuation
    COMMA, LPAREN, RPAREN, DOT, STAR, SEMICOLON,
    // Literals
    IDENT, STRING_LIT, INT_LIT, FLOAT_LIT,
    // Special
    END_OF_INPUT, INVALID
};

struct SQLToken {
    SQLTokenType type;
    std::string  value;
    size_t       pos = 0;
};

static const std::unordered_map<std::string, SQLTokenType> kKeywords = {
    {"SELECT",   SQLTokenType::SELECT},
    {"FROM",     SQLTokenType::FROM},
    {"WHERE",    SQLTokenType::WHERE},
    {"ORDER",    SQLTokenType::ORDER},
    {"BY",       SQLTokenType::BY},
    {"ASC",      SQLTokenType::ASC},
    {"DESC",     SQLTokenType::DESC},
    {"LIMIT",    SQLTokenType::LIMIT},
    {"OFFSET",   SQLTokenType::OFFSET},
    {"INSERT",   SQLTokenType::INSERT},
    {"INTO",     SQLTokenType::INTO},
    {"VALUES",   SQLTokenType::VALUES},
    {"UPDATE",   SQLTokenType::UPDATE},
    {"SET",      SQLTokenType::SET},
    {"DELETE",   SQLTokenType::DELETE},
    {"AND",      SQLTokenType::AND},
    {"OR",       SQLTokenType::OR},
    {"NOT",      SQLTokenType::NOT},
    {"IN",       SQLTokenType::IN},
    {"LIKE",     SQLTokenType::LIKE},
    {"IS",       SQLTokenType::IS},
    {"NULL",     SQLTokenType::NUL},
    {"TRUE",     SQLTokenType::TRUE_KW},
    {"FALSE",    SQLTokenType::FALSE_KW},
    {"AS",       SQLTokenType::AS},
    {"DISTINCT", SQLTokenType::DISTINCT},
};

class SQLLexer {
public:
    explicit SQLLexer(const std::string& input) : input_(input), pos_(0) {}

    std::vector<SQLToken> tokenize() {
        std::vector<SQLToken> tokens;
        while (pos_ < input_.size()) {
            skipWhitespace();
            if (pos_ >= input_.size()) {
              break;
            }

            size_t start = pos_;
            char c = input_[pos_];

            // Single-line comment (-- ...)
            if (c == '-' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '-') {
                while (pos_ < input_.size() && input_[pos_] != '\n') {
                  ++pos_;
                }
                continue;
            }

            // String literals
            if (c == '\'' || c == '"') {
                tokens.push_back(readString(c, start));
                continue;
            }

            // Numbers
            if (std::isdigit(c) || (c == '-' && pos_ + 1 < input_.size() && std::isdigit(input_[pos_ + 1]))) {
                tokens.push_back(readNumber(start));
                continue;
            }

            // Identifiers / keywords
            if (std::isalpha(c) || c == '_') {
                tokens.push_back(readIdent(start));
                continue;
            }

            // Operators
            switch (c) {
                case '=': tokens.push_back({SQLTokenType::EQ, "=", start}); ++pos_; break;
                case '<':
                    if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
                        tokens.push_back({SQLTokenType::LTE, "<=", start}); pos_ += 2;
                    } else if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '>') {
                        tokens.push_back({SQLTokenType::NEQ, "<>", start}); pos_ += 2;
                    } else {
                        tokens.push_back({SQLTokenType::LT, "<", start}); ++pos_;
                    }
                    break;
                case '>':
                    if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
                        tokens.push_back({SQLTokenType::GTE, ">=", start}); pos_ += 2;
                    } else {
                        tokens.push_back({SQLTokenType::GT, ">", start}); ++pos_;
                    }
                    break;
                case '!':
                    if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
                        tokens.push_back({SQLTokenType::NEQ, "!=", start}); pos_ += 2;
                    } else {
                        tokens.push_back({SQLTokenType::INVALID, "!", start}); ++pos_;
                    }
                    break;
                case ',': tokens.push_back({SQLTokenType::COMMA, ",", start}); ++pos_; break;
                case '(': tokens.push_back({SQLTokenType::LPAREN, "(", start}); ++pos_; break;
                case ')': tokens.push_back({SQLTokenType::RPAREN, ")", start}); ++pos_; break;
                case '.': tokens.push_back({SQLTokenType::DOT, ".", start}); ++pos_; break;
                case '*': tokens.push_back({SQLTokenType::STAR, "*", start}); ++pos_; break;
                case ';': tokens.push_back({SQLTokenType::SEMICOLON, ";", start}); ++pos_; break;
                default:  tokens.push_back({SQLTokenType::INVALID, std::string(1, c), start}); ++pos_; break;
            }
        }
        tokens.push_back({SQLTokenType::END_OF_INPUT, "", input_.size()});
        return tokens;
    }

private:
    const std::string& input_;
    size_t pos_;

    void skipWhitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
          ++pos_;
        }
    }

    SQLToken readString(char delim, size_t start) {
        ++pos_; // skip opening delimiter
        std::string val;
        while (pos_ < input_.size()) {
            char ch = input_[pos_];
            // SQL doubled-delimiter escape: '' or "" within a string
            if (ch == delim) {
                if (pos_ + 1 < input_.size() && input_[pos_ + 1] == delim) {
                    val += delim;
                    pos_ += 2;
                    continue;
                }
                break; // closing delimiter
            }
            if (ch == '\\' && pos_ + 1 < input_.size()) {
                ++pos_;
                switch (input_[pos_]) {
                    case 'n': val += '\n'; break;
                    case 'r': val += '\r'; break;
                    case 't': val += '\t'; break;
                    default:  val += input_[pos_]; break;
                }
            } else {
                val += ch;
            }
            ++pos_;
        }
        if (pos_ < input_.size()) ++pos_; // skip closing delimiter
        return {SQLTokenType::STRING_LIT, val, start};
    }

    SQLToken readNumber([[maybe_unused]] size_t start) {
        std::string val;
        if (input_[pos_] == '-') { val += '-'; ++pos_; }
        bool is_float = false;
        while (pos_ < input_.size() && (std::isdigit(input_[pos_]) || input_[pos_] == '.')) {
            if (input_[pos_] == '.') {
              is_float = true;
            }
            val += input_[pos_++];
        }
        return {is_float ? SQLTokenType::FLOAT_LIT : SQLTokenType::INT_LIT, val, start};
    }

    SQLToken readIdent([[maybe_unused]] size_t start) {
        std::string val;
        while (pos_ < input_.size() && (std::isalnum(input_[pos_]) || input_[pos_] == '_')) {
            val += input_[pos_++];
        }
        // Uppercase for keyword lookup
        std::string upper = val;
        std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c){ return std::toupper(c); });
        auto it = kKeywords.find(upper);
        if (it != kKeywords.end()) {
            return {it->second, upper, start};
        }
        return {SQLTokenType::IDENT, val, start};
    }
};

// ============================================================================
// Recursive-descent parser
// ============================================================================

class SQLParserImpl {
public:
    explicit SQLParserImpl(std::vector<SQLToken> tokens)
        : tokens_(std::move(tokens)), pos_(0) {}

    Result<SQLASTNode> parseStatement() {
        if (current().type == SQLTokenType::SELECT) {
            auto res = parseSelect();
            if (!res) {
              return Err<SQLASTNode>(res.error().code(), res.error().context());
            }
            SQLASTNode node;
            node.statement_type = SQLStatementType::Select;
            node.select = std::move(res.value());
            return Ok(std::move(node));
        }
        if (current().type == SQLTokenType::INSERT) {
            auto res = parseInsert();
            if (!res) {
              return Err<SQLASTNode>(res.error().code(), res.error().context());
            }
            SQLASTNode node;
            node.statement_type = SQLStatementType::Insert;
            node.insert = std::move(res.value());
            return Ok(std::move(node));
        }
        if (current().type == SQLTokenType::UPDATE) {
            auto res = parseUpdate();
            if (!res) {
              return Err<SQLASTNode>(res.error().code(), res.error().context());
            }
            SQLASTNode node;
            node.statement_type = SQLStatementType::Update;
            node.update = std::move(res.value());
            return Ok(std::move(node));
        }
        if (current().type == SQLTokenType::DELETE) {
            auto res = parseDelete();
            if (!res) {
              return Err<SQLASTNode>(res.error().code(), res.error().context());
            }
            SQLASTNode node;
            node.statement_type = SQLStatementType::Delete;
            node.del = std::move(res.value());
            return Ok(std::move(node));
        }

        return parseError("Expected SELECT, INSERT, UPDATE, or DELETE");
    }

private:
    std::vector<SQLToken> tokens_;
    size_t pos_;

    const SQLToken& current() const { return tokens_[pos_]; }

    void advance() {
        if (pos_ + 1 < tokens_.size()) {
          ++pos_;
        }
    }

    bool check(SQLTokenType t) const { return current().type == t; }

    bool match(SQLTokenType t) {
        if (check(t)) { advance(); return true; }
        return false;
    }

    Result<SQLASTNode> parseError(const std::string& msg) const {
        return Err<SQLASTNode>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            msg + " (at position " + std::to_string(current().pos) + ", token '" + current().value + "')"
        );
    }

    template<typename T>
    static Result<T> forwardError(const Result<SQLASTNode>& src) {
        return Err<T>(src.error().code(), src.error().context());
    }

    // ---------- SELECT ----------

    Result<SQLSelectStatement> parseSelect() {
        advance(); // consume SELECT
        SQLSelectStatement stmt;

        // Optional DISTINCT (accepted but not propagated to AQL)
        match(SQLTokenType::DISTINCT);

        // Column list or *
        if (check(SQLTokenType::STAR)) {
            stmt.star = true;
            advance();
        } else {
            while (true) {
                if (!check(SQLTokenType::IDENT)) {
                    auto err = parseError("Expected column name in SELECT");
                    return Err<SQLSelectStatement>(err.error().code(), err.error().context());
                }
                std::string col = current().value;
                advance();
                // Optional table prefix: col might have been "tbl" and next is DOT
                if (check(SQLTokenType::DOT)) {
                    advance();
                    if (!check(SQLTokenType::IDENT)) {
                        auto err = parseError("Expected column name after '.'");
                        return Err<SQLSelectStatement>(err.error().code(), err.error().context());
                    }
                    col = current().value; // use only the column part
                    advance();
                }
                // Optional alias (AS alias)
                if (check(SQLTokenType::AS)) {
                    advance(); // skip AS
                    if (check(SQLTokenType::IDENT)) advance(); // skip alias
                }
                stmt.columns.push_back(col);
                if (!match(SQLTokenType::COMMA)) {
                  break;
                }
            }
        }

        // FROM
        if (!match(SQLTokenType::FROM)) {
            auto err = parseError("Expected FROM");
            return Err<SQLSelectStatement>(err.error().code(), err.error().context());
        }
        if (!check(SQLTokenType::IDENT)) {
            auto err = parseError("Expected table name after FROM");
            return Err<SQLSelectStatement>(err.error().code(), err.error().context());
        }
        stmt.table = current().value;
        advance();
        // Optional table alias
        if (check(SQLTokenType::AS)) { advance(); if (check(SQLTokenType::IDENT)) advance(); }
        else if (check(SQLTokenType::IDENT) && !isKeyword(current().type)) { advance(); }

        // Optional WHERE
        if (match(SQLTokenType::WHERE)) {
            auto expr = parseExpr();
            if (!expr) {
              return Err<SQLSelectStatement>(expr.error().code(), expr.error().context());
            }
            stmt.where = std::move(expr.value());
        }

        // Optional ORDER BY
        if (check(SQLTokenType::ORDER)) {
            advance();
            if (!match(SQLTokenType::BY)) {
                auto err = parseError("Expected BY after ORDER");
                return Err<SQLSelectStatement>(err.error().code(), err.error().context());
            }
            while (check(SQLTokenType::IDENT)) {
                SQLSortSpec spec;
                spec.column = current().value;
                advance();
                if (match(SQLTokenType::DESC)) {
                    spec.ascending = false;
                } else {
                    match(SQLTokenType::ASC);
                    spec.ascending = true;
                }
                stmt.order_by.push_back(spec);
                if (!match(SQLTokenType::COMMA)) {
                  break;
                }
            }
        }

        // Optional LIMIT
        if (match(SQLTokenType::LIMIT)) {
            if (!check(SQLTokenType::INT_LIT)) {
                auto err = parseError("Expected integer after LIMIT");
                return Err<SQLSelectStatement>(err.error().code(), err.error().context());
            }
            stmt.limit = [&]() -> int64_t {
                try { return std::stoll(current().value); }
                catch (...) {
                    THEMIS_WARN("sql_parser: unhandled exception caught");
                    throw std::runtime_error("SQL LIMIT value '" + current().value + "' is out of integer range");
                }
            }();
            advance();
            // Optional OFFSET
            if (match(SQLTokenType::OFFSET)) {
                if (!check(SQLTokenType::INT_LIT)) {
                    auto err = parseError("Expected integer after OFFSET");
                    return Err<SQLSelectStatement>(err.error().code(), err.error().context());
                }
                stmt.offset = [&]() -> int64_t {
                    try { return std::stoll(current().value); }
                    catch (...) {
                        THEMIS_WARN("sql_parser: unhandled exception caught");
                        throw std::runtime_error("SQL OFFSET value '" + current().value + "' is out of integer range");
                    }
                }();
                advance();
            }
        }

        // Optional trailing semicolon
        match(SQLTokenType::SEMICOLON);

        return Ok(std::move(stmt));
    }

    // ---------- INSERT ----------

    Result<SQLInsertStatement> parseInsert() {
        advance(); // consume INSERT
        if (!match(SQLTokenType::INTO)) {
            auto err = parseError("Expected INTO after INSERT");
            return Err<SQLInsertStatement>(err.error().code(), err.error().context());
        }
        if (!check(SQLTokenType::IDENT)) {
            auto err = parseError("Expected table name after INSERT INTO");
            return Err<SQLInsertStatement>(err.error().code(), err.error().context());
        }
        SQLInsertStatement stmt;
        stmt.table = current().value;
        advance();

        // Column list
        if (!match(SQLTokenType::LPAREN)) {
            auto err = parseError("Expected '(' after table name in INSERT");
            return Err<SQLInsertStatement>(err.error().code(), err.error().context());
        }
        while (check(SQLTokenType::IDENT)) {
            stmt.columns.push_back(current().value);
            advance();
            if (!match(SQLTokenType::COMMA)) {
              break;
            }
        }
        if (!match(SQLTokenType::RPAREN)) {
            auto err = parseError("Expected ')' after column list in INSERT");
            return Err<SQLInsertStatement>(err.error().code(), err.error().context());
        }

        // VALUES
        if (!match(SQLTokenType::VALUES)) {
            auto err = parseError("Expected VALUES in INSERT statement");
            return Err<SQLInsertStatement>(err.error().code(), err.error().context());
        }
        if (!match(SQLTokenType::LPAREN)) {
            auto err = parseError("Expected '(' after VALUES");
            return Err<SQLInsertStatement>(err.error().code(), err.error().context());
        }
        while (!check(SQLTokenType::RPAREN) && !check(SQLTokenType::END_OF_INPUT)) {
            auto val = parseLiteralValue();
            if (!val) {
              return Err<SQLInsertStatement>(val.error().code(), val.error().context());
            }
            stmt.values.push_back(std::move(val.value()));
            if (!match(SQLTokenType::COMMA)) {
              break;
            }
        }
        if (!match(SQLTokenType::RPAREN)) {
            auto err = parseError("Expected ')' after VALUES list");
            return Err<SQLInsertStatement>(err.error().code(), err.error().context());
        }

        if (stmt.columns.size() != stmt.values.size()) {
            return Err<SQLInsertStatement>(
                errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                "Column count does not match value count in INSERT"
            );
        }

        match(SQLTokenType::SEMICOLON);
        return Ok(std::move(stmt));
    }

    // ---------- UPDATE ----------

    Result<SQLUpdateStatement> parseUpdate() {
        advance(); // consume UPDATE
        if (!check(SQLTokenType::IDENT)) {
            auto err = parseError("Expected table name after UPDATE");
            return Err<SQLUpdateStatement>(err.error().code(), err.error().context());
        }
        SQLUpdateStatement stmt;
        stmt.table = current().value;
        advance();

        if (!match(SQLTokenType::SET)) {
            auto err = parseError("Expected SET after table name in UPDATE");
            return Err<SQLUpdateStatement>(err.error().code(), err.error().context());
        }

        // assignment list: col = val [, col = val ...]
        while (check(SQLTokenType::IDENT)) {
            std::string col = current().value;
            advance();
            if (!match(SQLTokenType::EQ)) {
                auto err = parseError("Expected '=' in SET assignment");
                return Err<SQLUpdateStatement>(err.error().code(), err.error().context());
            }
            auto val = parseLiteralValue();
            if (!val) {
              return Err<SQLUpdateStatement>(val.error().code(), val.error().context());
            }
            stmt.assignments.push_back({col, std::move(val.value())});
            if (!match(SQLTokenType::COMMA)) {
              break;
            }
        }

        if (stmt.assignments.empty()) {
            return Err<SQLUpdateStatement>(
                errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                "UPDATE statement requires at least one SET assignment"
            );
        }

        // Optional WHERE
        if (match(SQLTokenType::WHERE)) {
            auto expr = parseExpr();
            if (!expr) {
              return Err<SQLUpdateStatement>(expr.error().code(), expr.error().context());
            }
            stmt.where = std::move(expr.value());
        }

        match(SQLTokenType::SEMICOLON);
        return Ok(std::move(stmt));
    }

    // ---------- DELETE ----------

    Result<SQLDeleteStatement> parseDelete() {
        advance(); // consume DELETE
        if (!match(SQLTokenType::FROM)) {
            auto err = parseError("Expected FROM after DELETE");
            return Err<SQLDeleteStatement>(err.error().code(), err.error().context());
        }
        if (!check(SQLTokenType::IDENT)) {
            auto err = parseError("Expected table name after DELETE FROM");
            return Err<SQLDeleteStatement>(err.error().code(), err.error().context());
        }
        SQLDeleteStatement stmt;
        stmt.table = current().value;
        advance();

        // Optional WHERE
        if (match(SQLTokenType::WHERE)) {
            auto expr = parseExpr();
            if (!expr) {
              return Err<SQLDeleteStatement>(expr.error().code(), expr.error().context());
            }
            stmt.where = std::move(expr.value());
        }

        match(SQLTokenType::SEMICOLON);
        return Ok(std::move(stmt));
    }

    // ---------- Expression parser (WHERE conditions) ----------
    // Precedence: OR < AND < NOT < comparison < IS/IN/LIKE < primary

    Result<std::shared_ptr<SQLExpr>> parseExpr() {
        return parseOr();
    }

    Result<std::shared_ptr<SQLExpr>> parseOr() {
        auto left = parseAnd();
        if (!left) {
          return left;
        }
        while (check(SQLTokenType::OR)) {
            advance();
            auto right = parseAnd();
            if (!right) {
              return right;
            }
            left = Ok(std::static_pointer_cast<SQLExpr>(std::make_shared<SQLBinaryOpExpr>("OR", std::move(left.value()), std::move(right.value()))));
        }
        return left;
    }

    Result<std::shared_ptr<SQLExpr>> parseAnd() {
        auto left = parseNot();
        if (!left) {
          return left;
        }
        while (check(SQLTokenType::AND)) {
            advance();
            auto right = parseNot();
            if (!right) {
              return right;
            }
            left = Ok(std::static_pointer_cast<SQLExpr>(std::make_shared<SQLBinaryOpExpr>("AND", std::move(left.value()), std::move(right.value()))));
        }
        return left;
    }

    Result<std::shared_ptr<SQLExpr>> parseNot() {
        if (match(SQLTokenType::NOT)) {
            auto operand = parseComparison();
            if (!operand) {
              return operand;
            }
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLUnaryOpExpr>("NOT", std::move(operand.value()))
            );
        }
        return parseComparison();
    }

    Result<std::shared_ptr<SQLExpr>> parseComparison() {
        auto left = parsePrimary();
        if (!left) {
          return left;
        }

        // IS [NOT] NULL
        if (check(SQLTokenType::IS)) {
            advance();
            bool is_not = match(SQLTokenType::NOT);
            if (!match(SQLTokenType::NUL)) {
                auto err = parseError("Expected NULL after IS [NOT]");
                return Err<std::shared_ptr<SQLExpr>>(err.error().code(), err.error().context());
            }
            std::string op = is_not ? "IS_NOT_NULL" : "IS_NULL";
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLUnaryOpExpr>(op, std::move(left.value()))
            );
        }

        // [NOT] IN (list)
        bool negated = false;
        if (check(SQLTokenType::NOT)) {
            advance();
            negated = true;
        }
        if (check(SQLTokenType::IN)) {
            advance();
            if (!match(SQLTokenType::LPAREN)) {
                auto err = parseError("Expected '(' after IN");
                return Err<std::shared_ptr<SQLExpr>>(err.error().code(), err.error().context());
            }
            std::vector<std::shared_ptr<SQLExpr>> elems;
            while (!check(SQLTokenType::RPAREN) && !check(SQLTokenType::END_OF_INPUT)) {
                auto elem = parsePrimary();
                if (!elem) {
                  return elem;
                }
                elems.push_back(std::move(elem.value()));
                if (!match(SQLTokenType::COMMA)) {
                  break;
                }
            }
            if (!match(SQLTokenType::RPAREN)) {
                auto err = parseError("Expected ')' after IN list");
                return Err<std::shared_ptr<SQLExpr>>(err.error().code(), err.error().context());
            }
            auto list = std::make_shared<SQLListExpr>(std::move(elems));
            std::shared_ptr<SQLExpr> in_expr = std::make_shared<SQLBinaryOpExpr>("IN", std::move(left.value()), std::move(list));
            if (negated) {
                in_expr = std::make_shared<SQLUnaryOpExpr>("NOT", std::move(in_expr));
            }
            return Ok(std::move(in_expr));
        }
        if (negated) {
            // "NOT" was consumed but no IN followed – putting it back is tricky;
            // treat as parse error for unsupported syntax
            auto err = parseError("NOT without IN is not supported in this context");
            return Err<std::shared_ptr<SQLExpr>>(err.error().code(), err.error().context());
        }

        // LIKE
        if (check(SQLTokenType::LIKE)) {
            advance();
            auto right = parsePrimary();
            if (!right) {
              return right;
            }
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLBinaryOpExpr>("LIKE", std::move(left.value()), std::move(right.value()))
            );
        }

        // Comparison operators: =, !=, <>, <, <=, >, >=
        std::string op;
        switch (current().type) {
            case SQLTokenType::EQ:  op = "=="; advance(); break;
            case SQLTokenType::NEQ: op = "!="; advance(); break;
            case SQLTokenType::LT:  op = "<";  advance(); break;
            case SQLTokenType::LTE: op = "<="; advance(); break;
            case SQLTokenType::GT:  op = ">";  advance(); break;
            case SQLTokenType::GTE: op = ">="; advance(); break;
            default: return left; // no operator – just return the primary
        }

        auto right = parsePrimary();
        if (!right) {
          return right;
        }
        return Ok<std::shared_ptr<SQLExpr>>(
            std::make_shared<SQLBinaryOpExpr>(op, std::move(left.value()), std::move(right.value()))
        );
    }

    Result<std::shared_ptr<SQLExpr>> parsePrimary() {
        // Parenthesised sub-expression
        if (match(SQLTokenType::LPAREN)) {
            auto inner = parseExpr();
            if (!inner) {
              return inner;
            }
            if (!match(SQLTokenType::RPAREN)) {
                auto err = parseError("Expected ')' after expression");
                return Err<std::shared_ptr<SQLExpr>>(err.error().code(), err.error().context());
            }
            return inner;
        }

        // NULL keyword
        if (match(SQLTokenType::NUL)) {
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLLiteralExpr>(SQLValue{std::nullptr_t{}})
            );
        }
        // TRUE / FALSE
        if (match(SQLTokenType::TRUE_KW)) {
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLLiteralExpr>(SQLValue{true})
            );
        }
        if (match(SQLTokenType::FALSE_KW)) {
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLLiteralExpr>(SQLValue{false})
            );
        }

        // String literal
        if (check(SQLTokenType::STRING_LIT)) {
            std::string val = current().value;
            advance();
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLLiteralExpr>(SQLValue{val})
            );
        }
        // Integer literal
        if (check(SQLTokenType::INT_LIT)) {
            int64_t val;
            try { val = std::stoll(current().value); }
            catch (...) {
                THEMIS_WARN("sql_parser::parsePrimary: unhandled exception caught");
                throw std::runtime_error("Integer literal '" + current().value + "' is out of range");
            }
            advance();
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLLiteralExpr>(SQLValue{val})
            );
        }
        // Float literal
        if (check(SQLTokenType::FLOAT_LIT)) {
            double val = 0;
            try { val = std::stod(current().value); }
            catch (...) {
                THEMIS_WARN("sql_parser::parsePrimary: unhandled exception caught");
                throw std::runtime_error("Float literal '" + current().value + "' is out of range");
            }
            advance();
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLLiteralExpr>(SQLValue{val})
            );
        }

        // Identifier (column reference, possibly table.col)
        if (check(SQLTokenType::IDENT)) {
            std::string name = current().value;
            advance();
            if (check(SQLTokenType::DOT)) {
                advance(); // consume '.'
                if (!check(SQLTokenType::IDENT) && !check(SQLTokenType::STAR)) {
                    auto err = parseError("Expected column name after '.'");
                    return Err<std::shared_ptr<SQLExpr>>(err.error().code(), err.error().context());
                }
                std::string col = current().value;
                advance();
                return Ok<std::shared_ptr<SQLExpr>>(
                    std::make_shared<SQLColumnExpr>(name, col)
                );
            }
            return Ok<std::shared_ptr<SQLExpr>>(
                std::make_shared<SQLColumnExpr>(name)
            );
        }

        auto err = parseError("Unexpected token in expression");
        return Err<std::shared_ptr<SQLExpr>>(err.error().code(), err.error().context());
    }

    // Parse a literal value (used in INSERT VALUES and UPDATE SET)
    Result<SQLValue> parseLiteralValue() {
        if (match(SQLTokenType::NUL)) return Ok(SQLValue{std::nullptr_t{}});
        if (match(SQLTokenType::TRUE_KW))  return Ok(SQLValue{true});
        if (match(SQLTokenType::FALSE_KW)) return Ok(SQLValue{false});
        if (check(SQLTokenType::STRING_LIT)) {
            std::string v = current().value; advance();
            return Ok(SQLValue{v});
        }
        if (check(SQLTokenType::INT_LIT)) {
            int64_t v;
            try { v = std::stoll(current().value); } catch (...) {
                THEMIS_WARN("sql_parser::parseLiteralValue: unhandled exception caught");
                throw std::runtime_error("Integer literal '" + current().value + "' is out of range");
            }
            advance();
            return Ok(SQLValue{v});
        }
        if (check(SQLTokenType::FLOAT_LIT)) {
            double v = 0;
            try { v = std::stod(current().value); } catch (...) {
                THEMIS_WARN("sql_parser::parseLiteralValue: unhandled exception caught");
                throw std::runtime_error("Float literal '" + current().value + "' is out of range");
            }
            advance();
            return Ok(SQLValue{v});
        }
        // Allow bare identifiers as string values in some contexts
        if (check(SQLTokenType::IDENT)) {
            std::string v = current().value; advance();
            return Ok(SQLValue{v});
        }
        auto err = parseError("Expected a literal value (string, number, NULL, TRUE, FALSE)");
        return Err<SQLValue>(err.error().code(), err.error().context());
    }

    // Return true for token types that are SQL keywords (not plain identifiers)
    static bool isKeyword(SQLTokenType t) {
        switch (t) {
            case SQLTokenType::SELECT: case SQLTokenType::FROM:   case SQLTokenType::WHERE:
            [[fallthrough]];\n            case SQLTokenType::ORDER:  case SQLTokenType::BY:     case SQLTokenType::ASC:
            [[fallthrough]];\n            case SQLTokenType::DESC:   case SQLTokenType::LIMIT:  case SQLTokenType::OFFSET:
            [[fallthrough]];\n            case SQLTokenType::INSERT: case SQLTokenType::INTO:   case SQLTokenType::VALUES:
            [[fallthrough]];\n            case SQLTokenType::UPDATE: case SQLTokenType::SET:    case SQLTokenType::DELETE:
            [[fallthrough]];\n            case SQLTokenType::AND:    case SQLTokenType::OR:     case SQLTokenType::NOT:
            [[fallthrough]];\n            case SQLTokenType::IN:     case SQLTokenType::LIKE:   case SQLTokenType::IS:
            [[fallthrough]];\n            case SQLTokenType::NUL:    case SQLTokenType::TRUE_KW: case SQLTokenType::FALSE_KW:
            [[fallthrough]];\n            case SQLTokenType::AS:     case SQLTokenType::DISTINCT:
                return true;
            default: return false;
        }
    }
};

} // anonymous namespace

// ============================================================================
// SQLParser – public API
// ============================================================================

Result<SQLASTNode> SQLParser::parse(const std::string& sql_query) {
    try {
        SQLLexer lexer(sql_query);
        auto tokens = lexer.tokenize();
        SQLParserImpl impl(std::move(tokens));
        return impl.parseStatement();
    } catch (const std::exception& e) {
        return Err<SQLASTNode>(errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                               std::string(e.what()));
    }
}

// ============================================================================
// SQLToAQLTranspiler – public API
// ============================================================================

Result<std::string> SQLToAQLTranspiler::transpile(const SQLASTNode& ast) {
    switch (ast.statement_type) {
        case SQLStatementType::Select:
            if (!ast.select) {
                return Err<std::string>(errors::ErrorCode::ERR_QUERY_INVALID, "Missing SELECT node");
            }
            return Ok(transpileSelect(*ast.select));

        case SQLStatementType::Insert:
            if (!ast.insert) {
                return Err<std::string>(errors::ErrorCode::ERR_QUERY_INVALID, "Missing INSERT node");
            }
            return Ok(transpileInsert(*ast.insert));

        case SQLStatementType::Update:
            if (!ast.update) {
                return Err<std::string>(errors::ErrorCode::ERR_QUERY_INVALID, "Missing UPDATE node");
            }
            return Ok(transpileUpdate(*ast.update));

        case SQLStatementType::Delete:
            if (!ast.del) {
                return Err<std::string>(errors::ErrorCode::ERR_QUERY_INVALID, "Missing DELETE node");
            }
            return Ok(transpileDelete(*ast.del));
    }
    return Err<std::string>(errors::ErrorCode::ERR_QUERY_INVALID, "Unknown SQL statement type");
}

// ============================================================================
// SQLToAQLTranspiler – private helpers
// ============================================================================

// Internal loop variable used for all generated FOR loops
static constexpr const char* kDocVar = "_doc";

std::string SQLToAQLTranspiler::valueToAQL(const SQLValue& val) {
    return sqlValueToAQL(val);
}

std::string SQLToAQLTranspiler::transpileSelect(const SQLSelectStatement& stmt) {
    std::ostringstream aql;
    const std::string var = kDocVar;

    aql << "FOR " << var << " IN " << stmt.table;

    // FILTER clause from WHERE
    if (stmt.where.has_value() && *stmt.where) {
        aql << " FILTER " << (*stmt.where)->toAQL(var);
    }

    // SORT clause
    if (!stmt.order_by.empty()) {
        aql << " SORT ";
        for (size_t i = 0; i < stmt.order_by.size(); ++i) {
            if (i > 0) {
              aql << ", ";
            }
            aql << var << "." << stmt.order_by[i].column;
            aql << (stmt.order_by[i].ascending ? " ASC" : " DESC");
        }
    }

    // LIMIT clause (AQL: LIMIT offset, count  or  LIMIT count)
    if (stmt.limit.has_value()) {
        aql << " LIMIT ";
        if (stmt.offset.has_value() && *stmt.offset > 0) {
            aql << *stmt.offset << ", ";
        }
        aql << *stmt.limit;
    }

    // RETURN clause
    aql << " RETURN ";
    if (stmt.star || stmt.columns.empty()) {
        aql << var;
    } else if (stmt.columns.size() == 1) {
        aql << var << "." << stmt.columns[0];
    } else {
        aql << "{";
        for (size_t i = 0; i < stmt.columns.size(); ++i) {
            if (i > 0) {
              aql << ", ";
            }
            aql << stmt.columns[i] << ": " << var << "." << stmt.columns[i];
        }
        aql << "}";
    }

    return aql.str();
}

std::string SQLToAQLTranspiler::transpileInsert(const SQLInsertStatement& stmt) {
    std::ostringstream aql;
    aql << "INSERT {";
    for (size_t i = 0; i < stmt.columns.size(); ++i) {
        if (i > 0) {
          aql << ", ";
        }
        aql << stmt.columns[i] << ": " << valueToAQL(stmt.values[i]);
    }
    aql << "} INTO " << stmt.table;
    return aql.str();
}

std::string SQLToAQLTranspiler::transpileUpdate(const SQLUpdateStatement& stmt) {
    std::ostringstream aql;
    const std::string var = kDocVar;

    aql << "FOR " << var << " IN " << stmt.table;

    if (stmt.where.has_value() && *stmt.where) {
        aql << " FILTER " << (*stmt.where)->toAQL(var);
    }

    aql << " UPDATE " << var << " WITH {";
    for (size_t i = 0; i < stmt.assignments.size(); ++i) {
        if (i > 0) {
          aql << ", ";
        }
        aql << stmt.assignments[i].column << ": " << valueToAQL(stmt.assignments[i].value);
    }
    aql << "} IN " << stmt.table;

    return aql.str();
}

std::string SQLToAQLTranspiler::transpileDelete(const SQLDeleteStatement& stmt) {
    std::ostringstream aql;
    const std::string var = kDocVar;

    aql << "FOR " << var << " IN " << stmt.table;

    if (stmt.where.has_value() && *stmt.where) {
        aql << " FILTER " << (*stmt.where)->toAQL(var);
    }

    aql << " REMOVE " << var << " IN " << stmt.table;

    return aql.str();
}

}  // namespace query
}  // namespace themis

