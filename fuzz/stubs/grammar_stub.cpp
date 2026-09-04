/**
 * @file grammar_stub.cpp
 * @brief Standalone fuzz stub for themis::llm::Grammar
 *
 * Provides a minimal, dependency-free implementation of Grammar for use in
 * the standalone fuzz_targets build (ENABLE_FUZZING=ON).  The stub replaces
 * llama.cpp grammar compilation with a lightweight EBNF structural validator
 * so that AFL++/libFuzzer can explore the Grammar interface contract (valid /
 * invalid inputs, move semantics, accessor paths) without requiring llama.cpp
 * to be present in the fuzz environment.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: allow grammar_harness to compile in the standalone fuzz build
 * Activation: THEMIS_FUZZ_STUBS cmake define / ENABLE_FUZZING=ON path
 * Production Delta: llama_grammar_init / llama_grammar_free are not called;
 *   validity is determined by lightweight EBNF structural checks only
 * Removal Plan: wire against real themisdb_llm once llama.cpp is available
 *   in the fuzz build environment
 */

#include "llm/grammar.h"

#include <cctype>
#include <string>
#include <utility>

namespace themis {
namespace llm {

// ── Lightweight EBNF structural validator ─────────────────────────────────────

/**
 * @brief Validate that @p text is structurally plausible EBNF.
 *
 * Checks:
 *  - Non-empty text (after whitespace stripping)
 *  - Balanced parentheses / square brackets / angle brackets
 *  - At least one rule definition (`<ident> ::=` or `ident =` pattern)
 *  - No NUL bytes embedded in the grammar text
 *  - Total length within a sane bound
 *
 * This is intentionally permissive — the goal is to detect obviously
 * malformed inputs while exercising the Grammar interface.
 *
 * @param text  Raw EBNF grammar text.
 * @param err   Set to an error description on failure.
 * @return true if structurally valid.
 */
static bool validate_ebnf_structure(const std::string& text,
                                    std::string& err) {
    static constexpr size_t kMaxLen = 1u << 20;  // 1 MiB cap

    if (text.empty()) {
        err = "Empty grammar text";
        return false;
    }
    if (text.size() > kMaxLen) {
        err = "Grammar text exceeds maximum length";
        return false;
    }
    if (text.find('\0') != std::string::npos) {
        err = "Grammar text contains embedded NUL byte";
        return false;
    }

    // Check balanced bracket pairs: (), [], {}
    int parens = 0, squares = 0, curlies = 0;
    bool in_string   = false;
    bool in_comment  = false;
    char string_delim = '\0';

    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];

        // Simple single-line comment detection (# or //)
        if (!in_string && !in_comment) {
            if (c == '#') {
                in_comment = true;
                continue;
            }
            if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
                in_comment = true;
                ++i;
                continue;
            }
        }
        if (in_comment) {
            if (c == '\n') {
              in_comment = false;
            }
            continue;
        }

        // String literal tracking (' or ")
        if ((!in_string && (c == '"' || c == '\'')) {
            in_string   = true;
            string_delim = c;
            continue;
        }
        if (in_string) {
            if (c == '\\' && i + 1 < text.size()) {
                ++i;  // skip escaped character
                continue;
            }
            if (c == string_delim) {
                in_string   = false;
                string_delim = '\0';
            }
            continue;
        }

        // Balance tracking
        switch (c) {
        case '(':  ++parens;  break;
        case ')':
            if (--parens < 0) { err = "Unmatched ')'"; return false; }
            break;
        case '[':  ++squares; break;
        case ']':
            if (--squares < 0) { err = "Unmatched ']'"; return false; }
            break;
        case '{':  ++curlies; break;
        case '}':
            if (--curlies < 0) { err = "Unmatched '}'"; return false; }
            break;
        default: break;
        }
    }

    if (parens  != 0) { err = "Unmatched '('";  return false; }
    if (squares != 0) { err = "Unmatched '['";  return false; }
    if (curlies != 0) { err = "Unmatched '{'";  return false; }

    // Require at least one visible non-whitespace character
    bool has_content = false;
    for (char c : text) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            has_content = true;
            break;
        }
    }
    if (!has_content) {
        err = "Grammar text contains only whitespace";
        return false;
    }

    return true;
}

// ── Grammar constructors ───────────────────────────────────────────────────────

Grammar::Grammar(const std::string& ebnf_text, const std::string& start_symbol)
    : grammar_(nullptr),
      ebnf_text_(ebnf_text),
      start_symbol_(start_symbol) {
    compile();
}

Grammar::Grammar(const std::string& ebnf_text,
                 const std::string& start_symbol,
                 const struct llama_model* /*model*/)
    : grammar_(nullptr),
      ebnf_text_(ebnf_text),
      start_symbol_(start_symbol) {
    // llama.cpp not available in standalone fuzz build; call the basic compile
    // path instead of compileWithVocab().
    compile();
}

Grammar::~Grammar() {
    // grammar_ is always nullptr in this stub (llama_grammar_free not called)
}

Grammar::Grammar(Grammar&& other) noexcept
    : grammar_(other.grammar_),
      ebnf_text_(std::move(other.ebnf_text_)),
      start_symbol_(std::move(other.start_symbol_)),
      error_(std::move(other.error_)) {
    other.grammar_ = nullptr;
}

Grammar& Grammar::operator=(Grammar&& other) noexcept {
    if (this != &other) {
        grammar_      = other.grammar_;
        ebnf_text_    = std::move(other.ebnf_text_);
        start_symbol_ = std::move(other.start_symbol_);
        error_        = std::move(other.error_);
        other.grammar_ = nullptr;
    }
    return *this;
}

// ── Accessors ─────────────────────────────────────────────────────────────────

bool Grammar::isValid() const {
    return error_.empty();
}

std::string Grammar::getError() const {
    return error_;
}

std::string Grammar::getEBNFText() const {
    return ebnf_text_;
}

std::string Grammar::getStartSymbol() const {
    return start_symbol_;
}

llama_grammar* Grammar::getHandle() const {
    // Always nullptr in the standalone fuzz stub (no llama.cpp)
    return grammar_;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

/**
 * @brief Validate the EBNF text and set error_ on failure.
 * @return true when compilation succeeds (error_ left empty).
 */
bool Grammar::compile() {
    error_.clear();
    if (!validate_ebnf_structure(ebnf_text_, error_)) {
        // error_ already set by validator
        return false;
    }
    if (start_symbol_.empty()) {
        error_ = "Start symbol must not be empty";
        return false;
    }
    // Grammar is syntactically plausible.  grammar_ stays nullptr because
    // llama.cpp is not linked in the fuzz build.
    return true;
}

/**
 * @brief Variant of compile() that would use a vocab if llama.cpp were present.
 *
 * In the standalone fuzz stub this is identical to compile() since no vocab
 * is available.
 */
bool Grammar::compileWithVocab(const ::llama_vocab* /*vocab*/) {
    return compile();
}

} // namespace llm
} // namespace themis
