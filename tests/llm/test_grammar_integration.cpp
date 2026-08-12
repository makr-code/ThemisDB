/**
 * @file test_grammar_integration.cpp
 * @brief Integration tests for Grammar class (Q1 robustness work).
 *
 * These tests exercise the Grammar constructors without a real llama_model so
 * they can run in any CI environment.  They validate:
 *
 *  - Empty EBNF / empty start-symbol → isValid() == false with descriptive
 *    error.
 *  - Null model in the model-aware constructor → hard error (not silent
 *    fallback).
 *  - Valid EBNF text is stored correctly even when compilation fails because
 *    the llama API is absent in the test environment.
 *  - Move semantics work correctly.
 */

#include <gtest/gtest.h>
#include "llm/grammar.h"

using namespace themis::llm;

// ---------------------------------------------------------------------------
// Empty / degenerate input tests
// ---------------------------------------------------------------------------

TEST(GrammarIntegrationTest, EmptyEBNF_IsInvalid) {
    Grammar g("", "root");
    EXPECT_FALSE(g.isValid());
    EXPECT_FALSE(g.getError().empty());
}

TEST(GrammarIntegrationTest, EmptyEBNF_ErrorMentionsEmpty) {
    Grammar g("", "root");
    EXPECT_NE(g.getError().find("empty"), std::string::npos);
}

TEST(GrammarIntegrationTest, EmptyStartSymbol_IsInvalid) {
    Grammar g("root ::= \"hello\"", "");
    EXPECT_FALSE(g.isValid());
    EXPECT_FALSE(g.getError().empty());
}

TEST(GrammarIntegrationTest, EmptyStartSymbol_ErrorMentionsEmpty) {
    Grammar g("root ::= \"hello\"", "");
    EXPECT_NE(g.getError().find("empty"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Model-aware constructor with null model (hard-error path)
// ---------------------------------------------------------------------------

TEST(GrammarIntegrationTest, NullModel_IsInvalid) {
    // A null model must produce a hard error — not a silent valid state.
    Grammar g("root ::= \"hello\"", "root", nullptr);
    EXPECT_FALSE(g.isValid());
}

TEST(GrammarIntegrationTest, NullModel_ErrorMentionsNull) {
    Grammar g("root ::= \"hello\"", "root", nullptr);
    const std::string& err = g.getError();
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(err.find("null") != std::string::npos ||
                err.find("unavailable") != std::string::npos)
        << "Expected null-model or unavailable-API error, got: " << err;
}

TEST(GrammarIntegrationTest, NullModel_HandleIsNull) {
    Grammar g("root ::= \"hello\"", "root", nullptr);
    EXPECT_EQ(g.getHandle(), nullptr);
}

// ---------------------------------------------------------------------------
// Accessor correctness
// ---------------------------------------------------------------------------

TEST(GrammarIntegrationTest, EBNFTextPreserved) {
    const std::string ebnf = "root ::= [a-z]+";
    Grammar g(ebnf, "root");
    EXPECT_EQ(g.getEBNFText(), ebnf);
}

TEST(GrammarIntegrationTest, StartSymbolPreserved) {
    Grammar g("root ::= \"x\"", "my_start");
    EXPECT_EQ(g.getStartSymbol(), "my_start");
}

TEST(GrammarIntegrationTest, NullModel_EBNFTextPreserved) {
    const std::string ebnf = "root ::= [0-9]+";
    Grammar g(ebnf, "root", nullptr);
    // Even when invalid, the EBNF text should be accessible for diagnostics.
    EXPECT_EQ(g.getEBNFText(), ebnf);
}

// ---------------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------------

TEST(GrammarIntegrationTest, MovedFromGrammar_ErrorPreserved) {
    Grammar g("root ::= \"hello\"", "root", nullptr);
    EXPECT_FALSE(g.isValid());

    Grammar g2(std::move(g));
    EXPECT_FALSE(g2.isValid());
    EXPECT_FALSE(g2.getError().empty());
}

TEST(GrammarIntegrationTest, MoveAssignment_InvalidGrammar) {
    Grammar g1("root ::= \"a\"", "root", nullptr);
    Grammar g2("root ::= \"b\"", "root", nullptr);

    g2 = std::move(g1);
    EXPECT_FALSE(g2.isValid());
}

// ---------------------------------------------------------------------------
// Basic-constructor path: valid EBNF without model (structural-only grammars)
// ---------------------------------------------------------------------------

TEST(GrammarIntegrationTest, BasicConstructor_ValidEBNF_StoresText) {
    // Without a real llama context, the grammar will be invalid (API absent),
    // but the EBNF text and start symbol must be stored for diagnostics.
    const std::string ebnf = "root ::= [A-Z][a-z]+";
    Grammar g(ebnf, "root");
    EXPECT_EQ(g.getEBNFText(), ebnf);
    EXPECT_EQ(g.getStartSymbol(), "root");
}

TEST(GrammarIntegrationTest, BasicConstructor_HandleMayBeNull_NoUndefinedBehavior) {
    // Calling getHandle() when the grammar is not compiled must return nullptr
    // rather than a dangling pointer.
    Grammar g("root ::= \"test\"", "root");
    // Either a valid handle (real llama linked) or nullptr — both are correct
    // in a test environment.  The important thing is no crash or UB.
    (void)g.getHandle();
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// Grammar API injection tests (GRAM-INJ-01..03)
// Tests for themis_grammar_inject_api_functions()
// ─────────────────────────────────────────────────────────────────────────────

// Forward declarations for the C injection/availability API (no llama.h needed).
extern "C" {
    bool themis_llama_grammar_available();
    void themis_grammar_inject_api_functions(void*, void*, void*, void*);
}

namespace {
    // Minimal mock grammar API functions for injection tests.
    static void* mock_ginit(void* /*vocab*/, const char* /*g*/, const char* /*s*/) {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(0xBEEF));
    }
    static void  mock_gfree(void* /*grammar*/) {}
    static void  mock_gsample(void* /*grammar*/, void* /*ctx*/, void* /*candidates*/) {}
    static void  mock_gaccept(void* /*grammar*/, void* /*ctx*/, int /*token*/) {}
} // namespace

// GRAM-INJ-01: After injecting all 4 mock functions, grammar API reports available.
TEST(GrammarApiInjectionTest, InjectedApiReportsAvailable) {
    themis_grammar_inject_api_functions(
        reinterpret_cast<void*>(mock_ginit),
        reinterpret_cast<void*>(mock_gfree),
        reinterpret_cast<void*>(mock_gsample),
        reinterpret_cast<void*>(mock_gaccept));
    EXPECT_TRUE(themis_llama_grammar_available());
    // Clean up.
    themis_grammar_inject_api_functions(nullptr, nullptr, nullptr, nullptr);
}

// GRAM-INJ-02: Partial injection (only init+free, sample/accept null) → NOT available.
// themis_grammar_inject_api_functions requires ALL four functions to be non-null.
TEST(GrammarApiInjectionTest, PartialInjectionIsNotAvailable) {
    themis_grammar_inject_api_functions(
        reinterpret_cast<void*>(mock_ginit),
        reinterpret_cast<void*>(mock_gfree),
        nullptr,   // sample missing
        nullptr);  // accept missing
    EXPECT_FALSE(themis_llama_grammar_available());
    themis_grammar_inject_api_functions(nullptr, nullptr, nullptr, nullptr);
}

// GRAM-INJ-03: Clearing injection reverts to dlsym-detected path without crashing.
TEST(GrammarApiInjectionTest, ClearInjectionRevertsGracefully) {
    // First inject so the override is active.
    themis_grammar_inject_api_functions(
        reinterpret_cast<void*>(mock_ginit),
        reinterpret_cast<void*>(mock_gfree),
        reinterpret_cast<void*>(mock_gsample),
        reinterpret_cast<void*>(mock_gaccept));
    ASSERT_TRUE(themis_llama_grammar_available());
    // Clear override — availability now reflects actual dlsym detection.
    themis_grammar_inject_api_functions(nullptr, nullptr, nullptr, nullptr);
    // Must not crash on subsequent availability check.
    EXPECT_NO_THROW(themis_llama_grammar_available());
}
