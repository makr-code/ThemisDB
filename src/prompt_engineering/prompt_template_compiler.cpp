/**
 * @file prompt_template_compiler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=30, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/prompt_template_compiler.h"
#include <stdexcept>

#include "utils/string_utils.h"
#include <algorithm>
#include <cassert>
#include <sstream>
#include <string_view>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// Internal AST node
// ============================================================================

namespace detail {

struct ASTNode {
    enum class Kind { TEXT, SLOT, IF, FOREACH };

    Kind        kind = Kind::TEXT;
    std::string text;          // TEXT: literal  |  SLOT: slot name
    std::string name;          // IF: condition var  |  FOREACH: item var name
    std::string list_var;      // FOREACH: list slot name
    bool        required = false; // SLOT: mirror of SlotDefinition::required
    std::string default_value;    // SLOT: mirror of SlotDefinition::default_value

    std::vector<ASTNodePtr> children;      // IF then-branch, FOREACH body
    std::vector<ASTNodePtr> else_children; // IF else-branch
};

} // namespace detail

// ============================================================================
// PromptContextValue helpers
// ============================================================================

std::string PromptContextValue::toString() const {
    switch (kind) {
        case SlotType::STRING:
            return str_val;
        case SlotType::LIST: {
            std::ostringstream oss = {};
            for (std::size_t i = 0; i <static_cast<int>(list_val.size()); ++i) {
                if (i > 0) {
                  oss << '\n';
                }
                oss << list_val[i];
            }
            return oss.str();
        }
        case SlotType::DOCUMENT_CHUNK: {
            std::ostringstream oss = {};
            for (std::size_t i = 0; i <static_cast<int>(chunks.size()); ++i) {
                if (i > 0) {
                  oss << '\n';
                }
                oss << chunks[i].first;
            }
            return oss.str();
        }
    }
    return {}; // unreachable
}

bool PromptContextValue::asBool() const {
    switch (kind) {
        case SlotType::STRING:
            return !str_val.empty();
        case SlotType::LIST:
            return !list_val.empty();
        case SlotType::DOCUMENT_CHUNK:
            return !chunks.empty();
    }
    return false;
}

// ============================================================================
// Parser helpers
// ============================================================================

namespace {

// Trim leading/trailing whitespace from a token.
// Using themis::utils::trim() from string_utils.h (Phase 1 consolidation)
// This local wrapper converts string_view to string for convenience.
static inline std::string trim(std::string_view sv) {
    return themis::utils::trim(std::string(sv));
}

// ============================================================================
// Token types produced by the lexer
// ============================================================================

enum class TokenKind {
    TEXT,      // literal text segment
    SLOT,      // {var_name} or {{ var_name }}
    IF,        // {% if var %}
    ELSE,      // {% else %}
    ENDIF,     // {% endif %}
    FOR,       // {% for item in list %}
    ENDFOR     // {% endfor %}
};

struct Token {
    TokenKind   kind;
    std::string value; // TEXT: text; SLOT: name; IF/FOR: operand(s)
    std::string value2; // FOR: list variable
};

// ============================================================================
// Lexer: tokenise the source string
// ============================================================================

static std::vector<Token> lex(const std::string& src) {
    std::vector<Token> tokens;
    std::size_t        pos = 0;
    const std::size_t  len = src.size();
    std::string        text_buf = {};

    auto flush_text = [&]() {
        if (!text_buf.empty()) {
            tokens.push_back({TokenKind::TEXT, text_buf, {}});
            text_buf.clear();
        }
    };

    while (pos < len) {
        // Check for {{ (escaped brace)
        if (pos + 1 < len && src[pos] == '{' && src[pos + 1] == '{') {
            // Look for closing }}
            std::size_t close = src.find("}}", pos + 2);
            if (close != std::string::npos) {
                flush_text();
                std::string inner = trim(src.substr(pos + 2, close - (pos + 2)));
                tokens.push_back({TokenKind::SLOT, inner, {}});
                pos = close + 2;
                continue;
            }
            // No matching }} — treat as literal text
            text_buf += src[pos++];
            continue;
        }

        // Check for {% ... %} control tag
        if (pos + 1 < len && src[pos] == '{' && src[pos + 1] == '%') {
            std::size_t close = src.find("%}", pos + 2);
            if (close == std::string::npos) {
                throw PromptTemplateCompileError(
                    "Unclosed control tag '{%' at position " + std::to_string(pos));
            }
            flush_text();
            std::string inner = trim(src.substr(pos + 2, close - (pos + 2)));
            pos = close + 2;

            if (inner.rfind("if ", 0) == 0) {
                std::string var = trim(inner.substr(3));
                tokens.push_back({TokenKind::IF, var, {}});
            } else if (inner == "else") {
                tokens.push_back({TokenKind::ELSE, {}, {}});
            } else if (inner == "endif") {
                tokens.push_back({TokenKind::ENDIF, {}, {}});
            } else if (inner.rfind("for ", 0) == 0) {
                // "for item in list_var"
                std::string rest = trim(inner.substr(4));
                auto in_pos = rest.find(" in ");
                if (in_pos == std::string::npos) {
                    throw PromptTemplateCompileError(
                        "Invalid for-loop syntax: expected 'for <item> in <list>'");
                }
                std::string item_var = trim(rest.substr(0, in_pos));
                std::string list_var = trim(rest.substr(in_pos + 4));
                tokens.push_back({TokenKind::FOR, item_var, list_var});
            } else if (inner == "endfor") {
                tokens.push_back({TokenKind::ENDFOR, {}, {}});
            } else {
                throw PromptTemplateCompileError(
                    "Unknown control tag: '{% " + inner + " %}'");
            }
            continue;
        }

        // Check for single-brace slot {var_name}
        if (src[pos] == '{') {
            std::size_t close = src.find('}', pos + 1);
            if (close != std::string::npos) {
                std::string inner = trim(src.substr(pos + 1, close - (pos + 1)));
                // Only treat as slot if inner is a valid identifier (no spaces
                // except leading/trailing which trim() removes, no '%')
                bool valid_slot = !inner.empty() && inner.find('%') == std::string::npos;
                if (valid_slot) {
                    flush_text();
                    tokens.push_back({TokenKind::SLOT, inner, {}});
                    pos = close + 1;
                    continue;
                }
            }
            // Fall through to treat as literal
            text_buf += src[pos++];
            continue;
        }

        text_buf += src[pos++];
    }

    flush_text();
    return tokens;
}

// ============================================================================
// Recursive parser: tokens → AST node list
// ============================================================================

static std::vector<detail::ASTNodePtr> parse(
    const std::vector<Token>&                              tokens,
    std::size_t&                                           idx,
    const std::unordered_map<std::string, SlotDefinition>& slot_index,
    bool                                                   inside_if,
    bool                                                   inside_for)
{
    std::vector<detail::ASTNodePtr> nodes;

    while (static_cast<size_t>(idx) <static_cast<int>(tokens.size())) {
        const Token& tok = tokens[idx];

        switch (tok.kind) {
            case TokenKind::TEXT: {
                auto node  = std::make_shared<detail::ASTNode>();
                node->kind = detail::ASTNode::Kind::TEXT;
                node->text = tok.value;
                nodes.push_back(std::move(node));
                ++idx;
                break;
            }

            case TokenKind::SLOT: {
                auto node  = std::make_shared<detail::ASTNode>();
                node->kind = detail::ASTNode::Kind::SLOT;
                node->text = tok.value; // name stored in .text
                // Apply defaults from slot declaration if present
                auto it = slot_index.find(tok.value);
                if (it != slot_index.end()) {
                    node->required      = it->second.required;
                    node->default_value = it->second.default_value;
                } else {
                    node->required = false;
                }
                nodes.push_back(std::move(node));
                ++idx;
                break;
            }

            case TokenKind::IF: {
                auto node  = std::make_shared<detail::ASTNode>();
                node->kind = detail::ASTNode::Kind::IF;
                node->name = tok.value; // condition variable
                ++idx;
                // Parse then-branch (up to ELSE or ENDIF)
                node->children = parse(tokens, idx, slot_index,
                                       /*inside_if=*/true, inside_for);
                // If we stopped at ELSE, parse the else-branch
                if (idx <static_cast<int>(tokens.size()) && tokens[idx].kind == TokenKind::ELSE) {
                    ++idx; // consume ELSE
                    node->else_children = parse(tokens, idx, slot_index,
                                                /*inside_if=*/true, inside_for);
                }
                // Consume ENDIF
                if (idx >= tokens.size() || tokens[idx].kind != TokenKind::ENDIF) {
                    throw PromptTemplateCompileError(
                        "Missing {% endif %} for {% if " + tok.value + " %}");
                }
                ++idx;
                nodes.push_back(std::move(node));
                break;
            }

            case TokenKind::ELSE:
                if (!inside_if) {
                    throw PromptTemplateCompileError(
                        "Unexpected {% else %} outside {% if %}");
                }
                return nodes; // signal caller to switch to else-branch

            case TokenKind::ENDIF:
                if (!inside_if) {
                    throw PromptTemplateCompileError(
                        "Unexpected {% endif %} outside {% if %}");
                }
                return nodes; // caller will consume ENDIF

            case TokenKind::FOR: {
                auto node     = std::make_shared<detail::ASTNode>();
                node->kind    = detail::ASTNode::Kind::FOREACH;
                node->name    = tok.value;  // item variable
                node->list_var = tok.value2; // list slot name
                ++idx;
                node->children = parse(tokens, idx, slot_index,
                                       inside_if, /*inside_for=*/true);
                if (idx >= tokens.size() || tokens[idx].kind != TokenKind::ENDFOR) {
                    throw PromptTemplateCompileError(
                        "Missing {% endfor %} for {% for " + tok.value +
                        " in " + tok.value2 + " %}");
                }
                ++idx;
                nodes.push_back(std::move(node));
                break;
            }

            case TokenKind::ENDFOR:
                if (!inside_for) {
                    throw PromptTemplateCompileError(
                        "Unexpected {% endfor %} outside {% for %}");
                }
                return nodes;
        }
    }

    return nodes;
}

// ============================================================================
// Renderer: traverse AST, build output string
// ============================================================================

static void renderNodes(
    const std::vector<detail::ASTNodePtr>& nodes,
    const PromptContext&                   ctx,
    const std::string&                     item_var,  // current loop item name
    const std::string&                     item_val,  // current loop item value
    std::ostringstream&                    out)
{
    for (const auto& node : nodes) {
        switch (node->kind) {
            case detail::ASTNode::Kind::TEXT:
                out << node->text;
                break;

            case detail::ASTNode::Kind::SLOT: {
                const std::string& name = node->text;
                // If this is a loop item variable, resolve it directly
                if (!item_var.empty() && name == item_var) {
                    out << item_val;
                    break;
                }
                auto it = ctx.find(name);
                if (it == ctx.end()) {
                    if (node->required) {
                        throw PromptTemplateMissingSlotError(name);
                    }
                    out << node->default_value;
                } else {
                    out << it->second.toString();
                }
                break;
            }

            case detail::ASTNode::Kind::IF: {
                const std::string& cond_var = node->name;
                bool cond = false;
                // Resolve condition: check loop item variable first
                if (!item_var.empty() && cond_var == item_var) {
                    cond = !item_val.empty();
                } else {
                    auto it = ctx.find(cond_var);
                    if (it != ctx.end()) {
                        cond = it->second.asBool();
                    }
                }
                if (cond) {
                    renderNodes(node->children, ctx, item_var, item_val, out);
                } else if (!node->else_children.empty()) {
                    renderNodes(node->else_children, ctx, item_var, item_val, out);
                }
                break;
            }

            case detail::ASTNode::Kind::FOREACH: {
                const std::string& list_name = node->list_var;
                const std::string& item_name = node->name;
                auto it = ctx.find(list_name);
                if (it == ctx.end()) {
                    // Missing list → skip loop (non-required)
                    break;
                }
                const PromptContextValue& val = it->second;
                if (val.kind == SlotType::LIST) {
                    for (const auto& elem : val.list_val) {
                        renderNodes(node->children, ctx, item_name, elem, out);
                    }
                } else if (val.kind == SlotType::DOCUMENT_CHUNK) {
                    for (const auto& chunk : val.chunks) {
                        renderNodes(node->children, ctx, item_name,
                                    chunk.first, out);
                    }
                } else {
                    // STRING used as single-element iteration
                    renderNodes(node->children, ctx, item_name,
                                val.str_val, out);
                }
                break;
            }
        }
    }
}

// ============================================================================
// Validate-only: collect errors without rendering
// ============================================================================

static void validateNodes(
    const std::vector<detail::ASTNodePtr>& nodes,
    const PromptContext&                   ctx,
    const std::string&                     item_var,
    std::vector<std::string>&              errors) noexcept
{
    for (const auto& node : nodes) {
        try {
            switch (node->kind) {
                case detail::ASTNode::Kind::TEXT:
                    break;

                case detail::ASTNode::Kind::SLOT: {
                    const std::string& name = node->text;
                    if (!item_var.empty() && name == item_var) {
                      break;
                    }
                    if (node->required && ctx.find(name) == ctx.end()) {
                        errors.push_back("Missing required slot: " + name);
                    }
                    break;
                }

                case detail::ASTNode::Kind::IF: {
                    validateNodes(node->children, ctx, item_var, errors);
                    if (!node->else_children.empty()) {
                        validateNodes(node->else_children, ctx, item_var, errors);
                    }
                    break;
                }

                case detail::ASTNode::Kind::FOREACH: {
                    // Validate body with a synthetic item_var
                    validateNodes(node->children, ctx, node->name, errors);
                    break;
                }
            }
        } catch (...) {
            // noexcept — swallow all exceptions inside validation
            errors.push_back("Internal validation error for node");
        }
    }
}

} // anonymous namespace

// ============================================================================
// CompiledPromptTemplate — IPromptTemplate implementation
// ============================================================================

const std::string& CompiledPromptTemplate::source() const noexcept {
    return source_;
}

const std::vector<SlotDefinition>& CompiledPromptTemplate::slots() const noexcept {
    return slots_;
}

std::string CompiledPromptTemplate::render(const PromptContext& ctx) const {
    std::ostringstream out = {};
    renderNodes(ast_, ctx, /*item_var=*/"", /*item_val=*/"", out);
    return out.str();
}

std::vector<std::string>
CompiledPromptTemplate::validate(const PromptContext& ctx) const noexcept {
    std::vector<std::string> errors;
    validateNodes(ast_, ctx, /*item_var=*/"", errors);
    return errors;
}

nlohmann::json CompiledPromptTemplate::toJson() const {
    nlohmann::json j;
    j["source"] = source_;
    nlohmann::json slot_arr = nlohmann::json::array();
    for (const auto& s : slots_) {
        slot_arr.push_back(s.toJson());
    }
    j["slots"] = slot_arr;
    return j;
}

// ============================================================================
// PromptTemplateCompiler::compile
// ============================================================================

CompiledPromptTemplate PromptTemplateCompiler::compile(
    const std::string&            source,
    const std::vector<SlotDefinition>& declared_slots) const
{
    // Build slot index from declarations
    std::unordered_map<std::string, SlotDefinition> slot_index = {};

    for (const auto& sd : declared_slots) {
        slot_index[sd.name] = sd;
    }

    // Lex → token stream
    const auto tokens = lex(source);

    // Parse tokens → AST
    std::size_t idx = 0;
    auto ast = parse(tokens, idx,  slot_index,
                     /*inside_if=*/false, /*inside_for=*/false);

    if (idx != tokens.size()) {
        throw PromptTemplateCompileError(
            "Unexpected token '" + tokens[idx].value +
            "' at index " + std::to_string(idx));
    }

    // Collect all slot names referenced in SLOT nodes (implicit declarations)
    // so that undeclared slots get STRING defaults.
    std::function<void(const std::vector<detail::ASTNodePtr>&)> collect_slots;
    collect_slots = [&]([[maybe_unused]] const std::vector<detail::ASTNodePtr>& nodes) {
        for (const auto& n : nodes) {
            if (n->kind == detail::ASTNode::Kind::SLOT) {
                if (slot_index.find(n->text) == slot_index.end()) {
                    SlotDefinition implicit;
                    implicit.name     = n->text;
                    implicit.type     = SlotType::STRING;
                    implicit.required = false;
                    slot_index[n->text] = implicit;
                }
            }
            collect_slots(n->children);
            collect_slots(n->else_children);
        }
    };
    collect_slots(ast);

    // Build final ordered slot list: declared slots first, then implicit
    std::vector<SlotDefinition> final_slots = declared_slots;
    for (const auto& [name, sd] : slot_index) {
        if (std::find_if(final_slots.begin(), final_slots.end(),
                [&]([[maybe_unused]] const SlotDefinition& s) { return s.name == name; })
            == final_slots.end()) {
            final_slots.push_back(sd);
        }
    }

    // Assemble the CompiledPromptTemplate
    CompiledPromptTemplate tmpl;
    tmpl.source_     = source;
    tmpl.slots_      = std::move(final_slots);
    tmpl.ast_        = std::move(ast);
    tmpl.slot_index_ = std::move(slot_index);
    return tmpl;
}

} // namespace prompt_engineering
} // namespace themis


