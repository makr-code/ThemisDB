/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_template_compiler.h                         ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:08:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     335                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 240a2c1d8b  2026-04-12  feat(prompt_engineering): Typed Template DSL — PromptTemp... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_template_compiler.h
 * @brief Typed Template DSL for prompt engineering (PE-MISSING-001).
 *
 * Implements the typed template DSL described in
 * `src/prompt_engineering/FUTURE_ENHANCEMENTS.md §Structured Prompt Template
 * DSL`.  The compiler parses a source template string into a compact AST and
 * exposes a `CompiledPromptTemplate` that validates context types at
 * publish/render time rather than at string-interpolation time.
 *
 * ## DSL Syntax
 *
 * | Construct          | Syntax                                      |
 * |--------------------|---------------------------------------------|
 * | Variable slot      | `{var_name}` (also: `{{ var_name }}`)       |
 * | Conditional block  | `{% if var_name %}...{% endif %}`           |
 * | Else branch        | `{% if var %}...{% else %}...{% endif %}`   |
 * | Loop               | `{% for item in list_var %}...{% endfor %}` |
 * | Literal brace      | `{{` or `}}` emits a single `{` or `}`      |
 *
 * The single-brace slot syntax `{var_name}` is backward-compatible with the
 * existing `PromptManager::injectContext()` convention.
 *
 * ## Slot types
 *
 * Slots are declared through `SlotDefinition`s passed to
 * `PromptTemplateCompiler::compile()`.  Supported types:
 *
 * | SlotType         | Context value type                         |
 * |------------------|--------------------------------------------|
 * | STRING           | `std::string`                              |
 * | LIST             | `std::vector<std::string>`                 |
 * | DOCUMENT_CHUNK   | `std::vector<std::pair<std::string,std::string>>` (content, source) |
 *
 * When a loop variable is used (`{% for item in list_var %}`), `list_var` must
 * resolve to a LIST or DOCUMENT_CHUNK slot; the loop variable `item` is bound
 * to each element (string for LIST, content field for DOCUMENT_CHUNK).
 *
 * ## Usage
 * ```cpp
 * SlotDefinition name_slot{"name", SlotType::STRING, true, ""};
 * SlotDefinition items_slot{"items", SlotType::LIST, false, ""};
 *
 * PromptTemplateCompiler compiler;
 * auto tmpl = compiler.compile(
 *     "Hello {name}!\n{% for item in items %}- {item}\n{% endfor %}",
 *     {name_slot, items_slot});
 *
 * PromptContext ctx;
 * ctx["name"]  = PromptContextValue::fromString("Alice");
 * ctx["items"] = PromptContextValue::fromList({"foo", "bar"});
 *
 * std::string rendered = tmpl.render(ctx);
 * // "Hello Alice!\n- foo\n- bar\n"
 * ```
 *
 * ## Performance targets (from FUTURE_ENHANCEMENTS.md)
 * - Compile a 4 KB template: < 50 ms.
 * - Render with a 2 KB context: < 1 ms P99.
 *
 * @see src/prompt_engineering/FUTURE_ENHANCEMENTS.md §Structured Prompt Template DSL
 */

#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// SlotType
// ============================================================================

/**
 * @brief The runtime type of a named variable slot in a compiled template.
 */
enum class SlotType {
    STRING,         ///< A single string value.
    LIST,           ///< An ordered list of strings.
    DOCUMENT_CHUNK  ///< A list of (content, source) pairs from a RAG retriever.
};

// ============================================================================
// SlotDefinition
// ============================================================================

/**
 * @brief Describes one typed variable slot in a compiled template.
 */
struct SlotDefinition {
    std::string name;          ///< Slot name as it appears in `{name}` tags.
    SlotType    type = SlotType::STRING;
    bool        required = true;
    std::string default_value; ///< Used when slot is absent and not required.

    nlohmann::json toJson() const {
        static const char* const kTypeNames[] = {
            "STRING", "LIST", "DOCUMENT_CHUNK"
        };
        return {
            {"name",          name},
            {"type",          kTypeNames[static_cast<int>(type)]},
            {"required",      required},
            {"default_value", default_value}
        };
    }
};

// ============================================================================
// PromptContextValue
// ============================================================================

/**
 * @brief A typed context value bound to a slot at render time.
 */
struct PromptContextValue {
    SlotType kind = SlotType::STRING;

    std::string                                      str_val;
    std::vector<std::string>                         list_val;
    std::vector<std::pair<std::string, std::string>> chunks; ///< (content, source)

    static PromptContextValue fromString(std::string s) {
        PromptContextValue v;
        v.kind    = SlotType::STRING;
        v.str_val = std::move(s);
        return v;
    }

    static PromptContextValue fromList(std::vector<std::string> l) {
        PromptContextValue v;
        v.kind     = SlotType::LIST;
        v.list_val = std::move(l);
        return v;
    }

    static PromptContextValue fromChunks(
            std::vector<std::pair<std::string, std::string>> c) {
        PromptContextValue v;
        v.kind   = SlotType::DOCUMENT_CHUNK;
        v.chunks = std::move(c);
        return v;
    }

    /** @brief Serialise to a plain string (for TEXT substitution). */
    std::string toString() const;

    /** @brief Truthy test used by `{% if %}` blocks. */
    bool asBool() const;
};

// ============================================================================
// PromptContext
// ============================================================================

/** @brief Map from slot name to its runtime value. */
using PromptContext = std::unordered_map<std::string, PromptContextValue>;

// ============================================================================
// CompileError / RenderError / MissingSlotError
// ============================================================================

/** @brief Thrown by `PromptTemplateCompiler::compile()` on parse errors. */
class PromptTemplateCompileError : public std::runtime_error {
public:
    explicit PromptTemplateCompileError(const std::string& what)
        : std::runtime_error(what) {}
};

/** @brief Thrown by `CompiledPromptTemplate::render()` when a required slot is missing. */
class PromptTemplateMissingSlotError : public std::runtime_error {
public:
    explicit PromptTemplateMissingSlotError(const std::string& slot_name)
        : std::runtime_error("Missing required slot: " + slot_name)
        , slot_name_(slot_name) {}

    const std::string& slotName() const noexcept { return slot_name_; }
private:
    std::string slot_name_;
};

/** @brief Thrown by `CompiledPromptTemplate::render()` on slot type mismatch. */
class PromptTemplateTypeMismatchError : public std::runtime_error {
public:
    explicit PromptTemplateTypeMismatchError(const std::string& msg)
        : std::runtime_error(msg) {}
};

// ============================================================================
// IPromptTemplate (interface — PE-MISSING-001)
// ============================================================================

/**
 * @brief Abstract interface for typed prompt templates.
 *
 * Both `CompiledPromptTemplate` (DSL) and any future template backend must
 * satisfy this interface so that the rest of the system can work with templates
 * polymorphically.
 */
class IPromptTemplate {
public:
    virtual ~IPromptTemplate() = default;

    /** @brief Return the raw source string that was compiled. */
    virtual const std::string& source() const noexcept = 0;

    /** @brief Return the list of declared slot definitions. */
    virtual const std::vector<SlotDefinition>& slots() const noexcept = 0;

    /**
     * @brief Render the template using the supplied context.
     *
     * @throws PromptTemplateMissingSlotError  if a required slot is absent.
     * @throws PromptTemplateTypeMismatchError if a slot value has the wrong type.
     */
    virtual std::string render(const PromptContext& ctx) const = 0;

    /**
     * @brief Validate the context without rendering.
     *
     * Returns an empty vector on success; returns error messages when required
     * slots are missing or types are wrong.  Never throws.
     */
    virtual std::vector<std::string> validate(const PromptContext& ctx) const noexcept = 0;
};

// ============================================================================
// Internal AST node types (forward)
// ============================================================================

namespace detail {
struct ASTNode;
using ASTNodePtr = std::shared_ptr<ASTNode>;
} // namespace detail

// ============================================================================
// CompiledPromptTemplate
// ============================================================================

/**
 * @brief A compiled, type-validated prompt template.
 *
 * Produced by `PromptTemplateCompiler::compile()`.  The internal AST is
 * immutable; the object is cheap to copy (the AST is shared through
 * `shared_ptr`).
 */
class CompiledPromptTemplate final : public IPromptTemplate {
public:
    CompiledPromptTemplate() = default;
    ~CompiledPromptTemplate() override = default;

    CompiledPromptTemplate(const CompiledPromptTemplate&)            = default;
    CompiledPromptTemplate& operator=(const CompiledPromptTemplate&) = default;
    CompiledPromptTemplate(CompiledPromptTemplate&&)                 = default;
    CompiledPromptTemplate& operator=(CompiledPromptTemplate&&)      = default;

    // IPromptTemplate ----------------------------------------------------------

    const std::string&              source() const noexcept override;
    const std::vector<SlotDefinition>& slots() const noexcept override;
    std::string                     render(const PromptContext& ctx) const override;
    std::vector<std::string>        validate(const PromptContext& ctx) const noexcept override;

    // Serialisation ------------------------------------------------------------

    nlohmann::json toJson() const;

private:
    friend class PromptTemplateCompiler; // only the compiler constructs this

    std::string                                      source_;
    std::vector<SlotDefinition>                      slots_;
    std::vector<detail::ASTNodePtr>                  ast_;
    std::unordered_map<std::string, SlotDefinition>  slot_index_;
};

// ============================================================================
// PromptTemplateCompiler
// ============================================================================

/**
 * @brief Compiles a DSL source string into a `CompiledPromptTemplate`.
 *
 * The compiler is stateless; a single instance can compile multiple templates
 * concurrently from different threads.
 *
 * @throws PromptTemplateCompileError  on any parse error.
 */
class PromptTemplateCompiler {
public:
    /**
     * @brief Compile @p source using the supplied slot declarations.
     *
     * Slots that appear in the source but are not declared receive an implicit
     * `SlotType::STRING` declaration.  Declared slots that do not appear in
     * the source are allowed (they may be used by conditional expressions that
     * are never true).
     */
    CompiledPromptTemplate compile(
        const std::string&            source,
        const std::vector<SlotDefinition>& slots = {}) const;
};

} // namespace prompt_engineering
} // namespace themis
