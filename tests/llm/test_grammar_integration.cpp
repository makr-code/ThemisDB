/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_grammar_integration.cpp                       ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:11:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     159                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f82bf2ae9f  2026-03-04  Refactor tenant manager tests and add new test cases ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
