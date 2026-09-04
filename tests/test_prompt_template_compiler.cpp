/**
 * @file test_prompt_template_compiler.cpp
 * @brief Unit tests for the Typed Template DSL (PE-MISSING-001).
 *
 * Acceptance criteria:
 *  AC-1   PromptTemplateCompiler produces a CompiledPromptTemplate (no throw) for a plain text source.
 *  AC-2   Plain text renders unchanged.
 *  AC-3   Single-brace slot {name} is substituted from a STRING context value.
 *  AC-4   Double-brace slot {{ name }} is substituted from a STRING context value.
 *  AC-5   Missing optional slot renders the default_value.
 *  AC-6   Missing required slot throws PromptTemplateMissingSlotError on render().
 *  AC-7   validate() returns empty vector when all required slots are present.
 *  AC-8   validate() returns one error per missing required slot.
 *  AC-9   validate() never throws (noexcept contract).
 *  AC-10  source() returns the original source string.
 *  AC-11  slots() contains exactly the declared slots plus implicitly found ones.
 *  AC-12  {% if var %}...{% endif %} renders then-block when var is non-empty string.
 *  AC-13  {% if var %}...{% endif %} renders nothing when var is absent.
 *  AC-14  {% if var %}...{% else %}...{% endif %} renders else-block when var is absent.
 *  AC-15  {% for item in list %}...{% endfor %} iterates over LIST values.
 *  AC-16  for-loop with an empty LIST produces empty output.
 *  AC-17  for-loop uses the item variable inside the body.
 *  AC-18  DOCUMENT_CHUNK slot renders as multi-line content via toString().
 *  AC-19  for-loop over DOCUMENT_CHUNK iterates content strings.
 *  AC-20  LIST slot render via toString() joins with newline.
 *  AC-21  Unclosed {% if %} throws PromptTemplateCompileError.
 *  AC-22  Unclosed {% for %} throws PromptTemplateCompileError.
 *  AC-23  Unknown control tag throws PromptTemplateCompileError.
 *  AC-24  Stray {% endif %} outside {% if %} throws PromptTemplateCompileError.
 *  AC-25  Stray {% endfor %} outside {% for %} throws PromptTemplateCompileError.
 *  AC-26  PromptTemplateValidator accepts a valid PromptTemplate JSON.
 *  AC-27  PromptTemplateValidator rejects a JSON with empty 'name' field.
 *  AC-28  PromptTemplateValidator rejects a JSON with missing 'content' field.
 *  AC-29  PromptTemplateValidator::validate(string) rejects invalid JSON.
 *  AC-30  CompiledPromptTemplate::toJson() contains 'source' and 'slots' keys.
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_template_compiler.h"
#include "prompt_engineering/prompt_template_validator.h"

#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::prompt_engineering;

// ============================================================================
// Fixture
// ============================================================================

class PromptTemplateCompilerTests : public ::testing::Test {
protected:
    PromptTemplateCompiler compiler;
};

// ============================================================================
// AC-1: compile plain text
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC01_CompilePlainText) {
    EXPECT_NO_THROW({
        auto tmpl = compiler.compile("Hello world");
    });
}

// ============================================================================
// AC-2: plain text renders unchanged
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC02_PlainTextRendersUnchanged) {
    auto tmpl = compiler.compile("Hello world");
    EXPECT_EQ(tmpl.render({}), "Hello world");
}

// ============================================================================
// AC-3: single-brace slot substitution
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC03_SingleBraceSlotSubstituted) {
    auto tmpl = compiler.compile("Hello {name}!");
    PromptContext ctx;
    ctx["name"] = PromptContextValue::fromString("Alice");
    EXPECT_EQ(tmpl.render(ctx), "Hello Alice!");
}

// ============================================================================
// AC-4: double-brace slot substitution
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC04_DoubleBraceSlotSubstituted) {
    auto tmpl = compiler.compile("Hello {{ name }}!");
    PromptContext ctx;
    ctx["name"] = PromptContextValue::fromString("Bob");
    EXPECT_EQ(tmpl.render(ctx), "Hello Bob!");
}

// ============================================================================
// AC-5: missing optional slot renders default_value
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC05_MissingOptionalSlotUsesDefault) {
    SlotDefinition sd;
    sd.name          = "greeting";
    sd.type          = SlotType::STRING;
    sd.required      = false;
    sd.default_value = "Hello";

    auto tmpl = compiler.compile("{greeting}, World!", {sd});
    PromptContext ctx; // empty — greeting absent
    EXPECT_EQ(tmpl.render(ctx), "Hello, World!");
}

// ============================================================================
// AC-6: missing required slot throws
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC06_MissingRequiredSlotThrows) {
    SlotDefinition sd;
    sd.name     = "user";
    sd.type     = SlotType::STRING;
    sd.required = true;

    auto tmpl = compiler.compile("Hello {user}!", {sd});
    EXPECT_THROW(tmpl.render({}), PromptTemplateMissingSlotError);
}

// ============================================================================
// AC-7: validate() returns empty when all required slots present
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC07_ValidateOkWhenAllSlotsPresent) {
    SlotDefinition sd;
    sd.name     = "city";
    sd.type     = SlotType::STRING;
    sd.required = true;

    auto tmpl = compiler.compile("Welcome to {city}!", {sd});
    PromptContext ctx;
    ctx["city"] = PromptContextValue::fromString("Berlin");
    auto errors = tmpl.validate(ctx);
    EXPECT_TRUE(errors.empty());
}

// ============================================================================
// AC-8: validate() reports missing required slots
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC08_ValidateReportsMissingRequiredSlot) {
    SlotDefinition sd;
    sd.name     = "topic";
    sd.type     = SlotType::STRING;
    sd.required = true;

    auto tmpl   = compiler.compile("Tell me about {topic}.", {sd});
    auto errors = tmpl.validate({}); // empty context
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("topic"), std::string::npos);
}

// ============================================================================
// AC-9: validate() never throws
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC09_ValidateIsNoexcept) {
    auto tmpl = compiler.compile("{x} and {y}");
    EXPECT_NO_THROW({
        tmpl.validate({}); // intentionally empty context
    });
}

// ============================================================================
// AC-10: source() returns original source
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC10_SourceReturnedUnchanged) {
    const std::string src = "The query is: {query}";
    auto tmpl = compiler.compile(src);
    EXPECT_EQ(tmpl.source(), src);
}

// ============================================================================
// AC-11: slots() reflects declared + implicitly found slots
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC11_SlotsContainsDeclaredAndImplicit) {
    SlotDefinition declared;
    declared.name     = "declared_slot";
    declared.type     = SlotType::STRING;
    declared.required = true;

    auto tmpl = compiler.compile("{declared_slot} {implicit_slot}", {declared});
    const auto& slots = tmpl.slots();

    bool found_declared = false, found_implicit = false;
    for (const auto& s : slots) {
        if (s.name == "declared_slot") {
          found_declared = true;
        }
        if (s.name == "implicit_slot") {
          found_implicit = true;
        }
    }
    EXPECT_TRUE(found_declared);
    EXPECT_TRUE(found_implicit);
}

// ============================================================================
// AC-12: {% if var %} renders then-block when var is non-empty string
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC12_IfBlockRendersWhenVarNonEmpty) {
    auto tmpl = compiler.compile("Start{% if show %} visible{% endif %} End");
    PromptContext ctx;
    ctx["show"] = PromptContextValue::fromString("yes");
    EXPECT_EQ(tmpl.render(ctx), "Start visible End");
}

// ============================================================================
// AC-13: {% if var %} renders nothing when var is absent
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC13_IfBlockRendersNothingWhenVarAbsent) {
    auto tmpl = compiler.compile("Start{% if show %} visible{% endif %} End");
    PromptContext ctx; // "show" absent
    EXPECT_EQ(tmpl.render(ctx), "Start End");
}

// ============================================================================
// AC-14: {% else %} renders when condition is absent
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC14_ElseBlockRendersWhenConditionFalse) {
    auto tmpl = compiler.compile(
        "{% if flag %}YES{% else %}NO{% endif %}");
    PromptContext ctx; // "flag" absent
    EXPECT_EQ(tmpl.render(ctx), "NO");
}

// ============================================================================
// AC-15: {% for item in list %} iterates over LIST values
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC15_ForLoopIteratesListValues) {
    auto tmpl = compiler.compile(
        "{% for item in items %}{item}\n{% endfor %}");
    PromptContext ctx;
    ctx["items"] = PromptContextValue::fromList({"apple", "banana", "cherry"});
    EXPECT_EQ(tmpl.render(ctx), "apple\nbanana\ncherry\n");
}

// ============================================================================
// AC-16: for-loop with empty LIST produces empty output
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC16_ForLoopEmptyListProducesEmpty) {
    auto tmpl = compiler.compile("{% for item in items %}{item}{% endfor %}");
    PromptContext ctx;
    ctx["items"] = PromptContextValue::fromList({});
    EXPECT_EQ(tmpl.render(ctx), "");
}

// ============================================================================
// AC-17: for-loop item variable usable in body
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC17_ForLoopItemVariableResolvedInBody) {
    auto tmpl = compiler.compile(
        "{% for x in xs %}[{x}]{% endfor %}");
    PromptContext ctx;
    ctx["xs"] = PromptContextValue::fromList({"1", "2", "3"});
    EXPECT_EQ(tmpl.render(ctx), "[1][2][3]");
}

// ============================================================================
// AC-18: DOCUMENT_CHUNK renders via toString()
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC18_DocumentChunkToStringMultiLine) {
    PromptContextValue chunks = PromptContextValue::fromChunks({
        {"First chunk content", "doc_a.pdf"},
        {"Second chunk content", "doc_b.pdf"}
    });
    std::string rendered = chunks.toString();
    EXPECT_NE(rendered.find("First chunk content"),  std::string::npos);
    EXPECT_NE(rendered.find("Second chunk content"), std::string::npos);
}

// ============================================================================
// AC-19: for-loop over DOCUMENT_CHUNK iterates content strings
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC19_ForLoopIteratesDocumentChunks) {
    auto tmpl = compiler.compile(
        "{% for chunk in docs %}- {chunk}\n{% endfor %}");
    PromptContext ctx;
    ctx["docs"] = PromptContextValue::fromChunks({
        {"Alpha", "src1"},
        {"Beta",  "src2"}
    });
    const std::string result = tmpl.render(ctx);
    EXPECT_NE(result.find("Alpha"), std::string::npos);
    EXPECT_NE(result.find("Beta"),  std::string::npos);
}

// ============================================================================
// AC-20: LIST toString() joins values with newline
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC20_ListToStringJoinsWithNewline) {
    PromptContextValue list_val =
        PromptContextValue::fromList({"line1", "line2", "line3"});
    const std::string s = list_val.toString();
    EXPECT_EQ(s, "line1\nline2\nline3");
}

// ============================================================================
// AC-21: unclosed {% if %} throws PromptTemplateCompileError
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC21_UnclosedIfThrows) {
    EXPECT_THROW(
        compiler.compile("{% if cond %}some text"),
        PromptTemplateCompileError);
}

// ============================================================================
// AC-22: unclosed {% for %} throws PromptTemplateCompileError
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC22_UnclosedForThrows) {
    EXPECT_THROW(
        compiler.compile("{% for x in xs %}{x}"),
        PromptTemplateCompileError);
}

// ============================================================================
// AC-23: unknown control tag throws PromptTemplateCompileError
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC23_UnknownControlTagThrows) {
    EXPECT_THROW(
        compiler.compile("{% unknown_tag %}"),
        PromptTemplateCompileError);
}

// ============================================================================
// AC-24: stray {% endif %} outside {% if %} throws
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC24_StrayEndifThrows) {
    EXPECT_THROW(
        compiler.compile("some text{% endif %}"),
        PromptTemplateCompileError);
}

// ============================================================================
// AC-25: stray {% endfor %} outside {% for %} throws
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC25_StrayEndforThrows) {
    EXPECT_THROW(
        compiler.compile("some text{% endfor %}"),
        PromptTemplateCompileError);
}

// ============================================================================
// AC-26: PromptTemplateValidator accepts valid JSON
// ============================================================================

TEST(PromptTemplateValidatorTests, AC26_ValidJsonAccepted) {
    PromptTemplateValidator v;
    nlohmann::json j = {
        {"id",          "tpl-001"},
        {"name",        "My Template"},
        {"version",     "v1"},
        {"content",     "Hello {name}"},
        {"description", "A greeting template"},
        {"active",      true},
        {"metadata",    nlohmann::json::object()},
        {"images",      nlohmann::json::array()}
    };
    auto result = v.validate(j);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

// ============================================================================
// AC-27: PromptTemplateValidator rejects empty 'name'
// ============================================================================

TEST(PromptTemplateValidatorTests, AC27_EmptyNameRejected) {
    PromptTemplateValidator v;
    nlohmann::json j = {
        {"id",          "tpl-002"},
        {"name",        ""},            // empty name
        {"version",     "v1"},
        {"content",     "text"},
        {"description", "desc"},
        {"active",      true},
        {"metadata",    nlohmann::json::object()},
        {"images",      nlohmann::json::array()}
    };
    auto result = v.validate(j);
    EXPECT_FALSE(result.valid);
    bool found = false;
    for (const auto& e : result.errors) {
        if (e.find("name") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// AC-28: PromptTemplateValidator rejects missing 'content'
// ============================================================================

TEST(PromptTemplateValidatorTests, AC28_MissingContentRejected) {
    PromptTemplateValidator v;
    nlohmann::json j = {
        {"id",          "tpl-003"},
        {"name",        "Test"},
        {"version",     "v1"},
        // "content" intentionally missing
        {"description", "desc"},
        {"active",      true},
        {"metadata",    nlohmann::json::object()},
        {"images",      nlohmann::json::array()}
    };
    auto result = v.validate(j);
    EXPECT_FALSE(result.valid);
    bool found = false;
    for (const auto& e : result.errors) {
        if (e.find("content") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// AC-29: PromptTemplateValidator rejects invalid JSON string
// ============================================================================

TEST(PromptTemplateValidatorTests, AC29_InvalidJsonStringRejected) {
    PromptTemplateValidator v;
    auto result = v.validate(std::string("this is not json {{{{"));
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

// ============================================================================
// AC-30: CompiledPromptTemplate::toJson() contains source and slots
// ============================================================================

TEST_F(PromptTemplateCompilerTests, AC30_ToJsonContainsSourceAndSlots) {
    SlotDefinition sd;
    sd.name     = "subject";
    sd.type     = SlotType::STRING;
    sd.required = true;

    auto tmpl = compiler.compile("About {subject}.", {sd});
    auto j    = tmpl.toJson();

    ASSERT_TRUE(j.contains("source"));
    ASSERT_TRUE(j.contains("slots"));
    EXPECT_EQ(j["source"].get<std::string>(), "About {subject}.");
    EXPECT_TRUE(j["slots"].is_array());
    EXPECT_FALSE(j["slots"].empty());
}
