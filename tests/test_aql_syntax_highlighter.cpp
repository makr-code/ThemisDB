/**
 * @file test_aql_syntax_highlighter.cpp
 * @brief Unit tests for AQLSyntaxHighlighter – syntax highlighting and
 *        error annotation of AQL code in LLM responses.
 */

#include <gtest/gtest.h>
#include "aql/aql_syntax_highlighter.h"

#include <string>
#include <vector>
#include <algorithm>

using namespace themis::aql;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool containsToken(const std::vector<AQLToken>& tokens,
                           AQLTokenType type, const std::string& value) {
    return std::any_of(tokens.begin(), tokens.end(),
        [&](const AQLToken& t) {
            return t.type == type && t.value == value;
        });
}

static bool hasAnnotationContaining(const std::vector<AQLAnnotation>& anns,
                                    const std::string& substr) {
    return std::any_of(anns.begin(), anns.end(),
        [&](const AQLAnnotation& a) {
            return a.message.find(substr) != std::string::npos;
        });
}

// ---------------------------------------------------------------------------
// Tokenizer tests
// ---------------------------------------------------------------------------

class AQLTokenizerTest : public ::testing::Test {
protected:
    AQLSyntaxHighlighter h{false}; // plain text, no ANSI
};

TEST_F(AQLTokenizerTest, CoreKeywordsAreTagged) {
    auto tokens = h.tokenize("FOR doc IN documents FILTER doc.age > 18 RETURN doc");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "FOR"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "IN"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "FILTER"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "RETURN"));
}

TEST_F(AQLTokenizerTest, CaseInsensitiveKeywords) {
    auto tokens = h.tokenize("for doc in documents return doc");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "for"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "in"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "return"));
}

TEST_F(AQLTokenizerTest, LLMKeywordsAreTagged) {
    auto tokens = h.tokenize("LLM INFER \"hello\" USING MODEL \"llama\"");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::LLM_KEYWORD, "LLM"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::LLM_KEYWORD, "INFER"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::LLM_KEYWORD, "MODEL"));
}

TEST_F(AQLTokenizerTest, StringLiterals) {
    auto tokens = h.tokenize("FILTER doc.name == \"Alice\"");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::STRING, "\"Alice\""));
}

TEST_F(AQLTokenizerTest, SingleQuoteStringLiterals) {
    auto tokens = h.tokenize("FILTER doc.city == 'Seattle'");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::STRING, "'Seattle'"));
}

TEST_F(AQLTokenizerTest, NumberLiterals) {
    auto tokens = h.tokenize("FILTER doc.age > 18");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::NUMBER, "18"));
}

TEST_F(AQLTokenizerTest, FloatLiterals) {
    auto tokens = h.tokenize("FILTER doc.score > 0.85");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::NUMBER, "0.85"));
}

TEST_F(AQLTokenizerTest, OperatorsAreTagged) {
    auto tokens = h.tokenize("FILTER a == b && c != d");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::OPERATOR, "=="));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::OPERATOR, "&&"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::OPERATOR, "!="));
}

TEST_F(AQLTokenizerTest, SingleLineComment) {
    auto tokens = h.tokenize("FOR doc IN docs // this is a comment\nRETURN doc");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::COMMENT, "// this is a comment"));
}

TEST_F(AQLTokenizerTest, BlockComment) {
    auto tokens = h.tokenize("/* block */ FOR doc IN docs RETURN doc");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::COMMENT, "/* block */"));
}

TEST_F(AQLTokenizerTest, BuiltinFunctionCall) {
    auto tokens = h.tokenize("FILTER SIMILARITY(doc.vec, @q, 10)");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::FUNCTION, "SIMILARITY"));
}

TEST_F(AQLTokenizerTest, IdentifiersAreTagged) {
    auto tokens = h.tokenize("FOR doc IN my_collection RETURN doc");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::IDENTIFIER, "doc"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::IDENTIFIER, "my_collection"));
}

TEST_F(AQLTokenizerTest, BindParameter) {
    auto tokens = h.tokenize("FILTER doc.id == @userId");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::IDENTIFIER, "@userId"));
}

TEST_F(AQLTokenizerTest, EmptyInput) {
    auto tokens = h.tokenize("");
    EXPECT_TRUE(tokens.empty());
}

TEST_F(AQLTokenizerTest, MultilineQuery) {
    const std::string code =
        "FOR user IN users\n"
        "  FILTER user.age > 18\n"
        "  SORT user.name ASC\n"
        "  LIMIT 10\n"
        "  RETURN user";
    auto tokens = h.tokenize(code);
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "FOR"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "SORT"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::KEYWORD, "LIMIT"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::NUMBER, "10"));
}

// ---------------------------------------------------------------------------
// highlightBlock tests
// ---------------------------------------------------------------------------

class AQLHighlightBlockTest : public ::testing::Test {
protected:
    AQLSyntaxHighlighter h_ansi{true};
    AQLSyntaxHighlighter h_plain{false};
};

TEST_F(AQLHighlightBlockTest, PlainModePreservesContent) {
    const std::string code = "FOR doc IN users RETURN doc";
    EXPECT_EQ(h_plain.highlightBlock(code), code);
}

TEST_F(AQLHighlightBlockTest, AnsiModeContainsEscapes) {
    const std::string code = "FOR doc IN users RETURN doc";
    auto result = h_ansi.highlightBlock(code);
    // Should contain ESC [ (ANSI escape sequence)
    EXPECT_NE(result.find('\x1b'), std::string::npos);
}

TEST_F(AQLHighlightBlockTest, AnsiModeReconstructsText) {
    const std::string code = "FOR doc IN users RETURN doc";
    auto result = h_plain.highlightBlock(code);
    // Plain mode: no extra characters, just the original text
    EXPECT_EQ(result, code);
}

TEST_F(AQLHighlightBlockTest, EmptyCodeProducesEmptyHighlight) {
    EXPECT_EQ(h_ansi.highlightBlock(""), "");
    EXPECT_EQ(h_plain.highlightBlock(""), "");
}

// ---------------------------------------------------------------------------
// annotateErrors tests
// ---------------------------------------------------------------------------

class AQLAnnotateErrorsTest : public ::testing::Test {
protected:
    AQLSyntaxHighlighter h{false};
};

TEST_F(AQLAnnotateErrorsTest, ValidQueryProducesNoErrors) {
    const std::string code = "FOR doc IN users FILTER doc.age > 18 RETURN doc";
    auto errors = h.annotateErrors(code);
    EXPECT_TRUE(errors.empty());
}

TEST_F(AQLAnnotateErrorsTest, UnclosedBrace) {
    const std::string code = "FOR doc IN users RETURN { name: doc.name";
    auto errors = h.annotateErrors(code);
    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(hasAnnotationContaining(errors, "Unclosed"));
}

TEST_F(AQLAnnotateErrorsTest, UnmatchedClosingParenthesis) {
    const std::string code = "FOR doc IN users RETURN doc.age )";
    auto errors = h.annotateErrors(code);
    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(hasAnnotationContaining(errors, "Unmatched closing"));
}

TEST_F(AQLAnnotateErrorsTest, MismatchedBrackets) {
    const std::string code = "RETURN ( 1 + 2 ]";
    auto errors = h.annotateErrors(code);
    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(hasAnnotationContaining(errors, "Mismatched"));
}

TEST_F(AQLAnnotateErrorsTest, ForWithoutIn) {
    // FOR missing IN keyword
    const std::string code = "FOR doc RETURN doc";
    auto errors = h.annotateErrors(code);
    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(hasAnnotationContaining(errors, "IN"));
}

TEST_F(AQLAnnotateErrorsTest, ValidForWithIn) {
    const std::string code = "FOR doc IN collection RETURN doc";
    auto errors = h.annotateErrors(code);
    // Should produce no errors from the FOR-without-IN check
    bool has_for_error = hasAnnotationContaining(errors, "IN");
    EXPECT_FALSE(has_for_error);
}

TEST_F(AQLAnnotateErrorsTest, MultipleErrors) {
    const std::string code = "FOR doc RETURN (doc.name ]";
    auto errors = h.annotateErrors(code);
    EXPECT_GE(errors.size(), 2u);
}

TEST_F(AQLAnnotateErrorsTest, EmptyCodeProducesNoErrors) {
    auto errors = h.annotateErrors("");
    EXPECT_TRUE(errors.empty());
}

TEST_F(AQLAnnotateErrorsTest, AnnotationsHavePositionInfo) {
    const std::string code = "FOR doc RETURN doc";
    auto errors = h.annotateErrors(code);
    for (const auto& e : errors) {
        EXPECT_GE(e.line, 1u);
        EXPECT_GE(e.column, 1u);
        EXPECT_FALSE(e.message.empty());
    }
}

// ---------------------------------------------------------------------------
// formatLLMResponse tests
// ---------------------------------------------------------------------------

class AQLFormatLLMResponseTest : public ::testing::Test {
protected:
    AQLSyntaxHighlighter h_plain{false};
    AQLSyntaxHighlighter h_ansi{true};
};

TEST_F(AQLFormatLLMResponseTest, PlainTextPassedThrough) {
    const std::string response = "Here is the answer to your question.";
    auto result = h_plain.formatLLMResponse(response);
    EXPECT_EQ(result.text, response);
    EXPECT_TRUE(result.annotations.empty());
}

TEST_F(AQLFormatLLMResponseTest, AQLCodeBlockHighlighted) {
    const std::string response =
        "Here is the AQL:\n"
        "```aql\n"
        "FOR doc IN users RETURN doc\n"
        "```\n"
        "End of response.";

    auto result = h_plain.formatLLMResponse(response);
    // Prose parts should be preserved
    EXPECT_NE(result.text.find("Here is the AQL:"), std::string::npos);
    EXPECT_NE(result.text.find("End of response."), std::string::npos);
    // The code block should still contain the query text
    EXPECT_NE(result.text.find("FOR"), std::string::npos);
    EXPECT_NE(result.text.find("users"), std::string::npos);
}

TEST_F(AQLFormatLLMResponseTest, AnsiColoursAppliedToAQLBlock) {
    const std::string response =
        "```aql\n"
        "FOR doc IN users RETURN doc\n"
        "```";

    auto result = h_ansi.formatLLMResponse(response);
    // Should contain ANSI escape sequences
    EXPECT_NE(result.text.find('\x1b'), std::string::npos);
}

TEST_F(AQLFormatLLMResponseTest, NonAQLCodeBlockPassedThrough) {
    const std::string response =
        "```json\n"
        "{ \"key\": \"value\" }\n"
        "```";

    auto result = h_ansi.formatLLMResponse(response);
    // JSON block should appear essentially unchanged (no AQL highlights)
    EXPECT_NE(result.text.find("\"key\""), std::string::npos);
}

TEST_F(AQLFormatLLMResponseTest, ErrorsAnnotatedFromCodeBlock) {
    const std::string response =
        "Try this:\n"
        "```aql\n"
        "FOR doc RETURN doc\n"  // missing IN
        "```";

    auto result = h_plain.formatLLMResponse(response);
    EXPECT_FALSE(result.annotations.empty());
    EXPECT_TRUE(hasAnnotationContaining(result.annotations, "IN"));
}

TEST_F(AQLFormatLLMResponseTest, ValidAQLBlockProducesNoAnnotations) {
    const std::string response =
        "```aql\n"
        "FOR doc IN users FILTER doc.active == true RETURN doc\n"
        "```";

    auto result = h_plain.formatLLMResponse(response);
    EXPECT_TRUE(result.annotations.empty());
}

TEST_F(AQLFormatLLMResponseTest, MultipleCodeBlocksProcessed) {
    const std::string response =
        "First:\n"
        "```aql\n"
        "FOR a IN collA RETURN a\n"
        "```\n"
        "Second:\n"
        "```aql\n"
        "FOR b IN collB RETURN b\n"
        "```";

    auto result = h_plain.formatLLMResponse(response);
    EXPECT_NE(result.text.find("collA"), std::string::npos);
    EXPECT_NE(result.text.find("collB"), std::string::npos);
}

TEST_F(AQLFormatLLMResponseTest, NoCodeBlocksNoAnnotations) {
    const std::string response = "No code here, just prose.";
    auto result = h_plain.formatLLMResponse(response);
    EXPECT_EQ(result.text, response);
    EXPECT_TRUE(result.annotations.empty());
}

TEST_F(AQLFormatLLMResponseTest, UnlabelledCodeBlockTreatedAsAQL) {
    const std::string response =
        "```\n"
        "FOR doc IN users RETURN doc\n"
        "```";

    auto result = h_plain.formatLLMResponse(response);
    // Unlabelled block with valid AQL should produce no errors
    EXPECT_TRUE(result.annotations.empty());
}

TEST_F(AQLFormatLLMResponseTest, LLMExtensionKeywordsHighlighted) {
    const std::string response =
        "```aql\n"
        "LLM INFER \"Explain ThemisDB\" USING MODEL \"llama\"\n"
        "```";

    // Ensure LLM keyword tokens appear in tokenization
    AQLSyntaxHighlighter plain{false};
    auto tokens = plain.tokenize("LLM INFER \"Explain ThemisDB\" USING MODEL \"llama\"");
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::LLM_KEYWORD, "LLM"));
    EXPECT_TRUE(containsToken(tokens, AQLTokenType::LLM_KEYWORD, "INFER"));
}

// ---------------------------------------------------------------------------
// "Validate before use" pattern tests  (mirrors translateNLToAQL behaviour)
// ---------------------------------------------------------------------------

class AQLValidateBeforeUseTest : public ::testing::Test {
protected:
    AQLSyntaxHighlighter h{false};
};

TEST_F(AQLValidateBeforeUseTest, ValidGeneratedQueryPassesClean) {
    // Simulates a well-formed AQL string that an LLM returned
    const std::string generated =
        "FOR user IN users\n"
        "  FILTER user.city == \"Seattle\"\n"
        "  SORT user.name ASC\n"
        "  LIMIT 10\n"
        "  RETURN user";

    auto errors = h.annotateErrors(generated);
    EXPECT_TRUE(errors.empty())
        << "Valid generated AQL should have no structural errors";
}

TEST_F(AQLValidateBeforeUseTest, BrokenGeneratedQueryIsAnnotated) {
    // Simulates a partially hallucinated AQL block from an LLM
    const std::string generated =
        "FOR doc IN orders\n"
        "  FILTER doc.total > 100 {\n"  // spurious brace
        "  RETURN doc";

    auto errors = h.annotateErrors(generated);
    EXPECT_FALSE(errors.empty())
        << "Broken generated AQL must produce at least one annotation";
}

TEST_F(AQLValidateBeforeUseTest, AnnotationsHaveUsefulMessages) {
    const std::string query_missing_in = "FOR doc RETURN doc.name";
    auto errors = h.annotateErrors(query_missing_in);
    ASSERT_FALSE(errors.empty());
    for (const auto& e : errors) {
        EXPECT_GE(e.line, 1u);
        EXPECT_GE(e.column, 1u);
        EXPECT_FALSE(e.message.empty());
    }
}

TEST_F(AQLValidateBeforeUseTest, MultipleErrorsAllReported) {
    // Two independent errors: missing IN + unclosed parenthesis
    const std::string query_missing_in_and_unclosed_paren = "FOR doc RETURN (doc.name";
    auto errors = h.annotateErrors(query_missing_in_and_unclosed_paren);
    EXPECT_GE(errors.size(), 2u)
        << "Both the missing IN and the unclosed ( should be reported";
}
