/**
 * @file test_legal_modality_analyzer.cpp
 * @brief Tests for Legal Modality Analyzer (German Administrative Law)
 * 
 * Tests the extraction of modal verbs with legal semantics from German
 * administrative law texts (Verwaltungsrecht).
 */

#include <gtest/gtest.h>
#include "analytics/nlp_text_analyzer.h"
#include <string>
#include <algorithm>

using namespace themis;
using namespace themis::analytics;

class LegalModalityAnalyzerTest : public ::testing::Test {
protected:
    NlpTextAnalyzer analyzer;
    
    void SetUp() override {
        // Analyzer is ready to use
    }
    
    // Helper to find modality by verb in results
    LegalModality* findModalityByVerb(std::vector<LegalModality>& modalities, const std::string& verb) {
        auto it = std::find_if(modalities.begin(), modalities.end(),
                              [&verb](const LegalModality& m) {
                                  return m.verb == verb;
                              });
        return (it != modalities.end()) ? &(*it) : nullptr;
    }
};

/**
 * @test Basic extraction of "muss" (must) - binding obligation
 */
TEST_F(LegalModalityAnalyzerTest, ExtractMussModalVerb) {
    std::string text = "Der Antragsteller muss die Unterlagen einreichen.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_GE(modalities.size(), 1);
    
    auto* muss_modality = findModalityByVerb(modalities, "muss");
    ASSERT_NE(muss_modality, nullptr);
    
    EXPECT_EQ(muss_modality->category, "obligation");
    EXPECT_FLOAT_EQ(muss_modality->strength, 1.0f);
    EXPECT_EQ(muss_modality->deontic_logic, "O(φ)");
    EXPECT_FALSE(muss_modality->interpretation.empty());
}

/**
 * @test Basic extraction of "soll" (shall) - default rule
 */
TEST_F(LegalModalityAnalyzerTest, ExtractSollModalVerb) {
    std::string text = "Die Behörde soll binnen zwei Wochen entscheiden.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_GE(modalities.size(), 1);
    
    auto* soll_modality = findModalityByVerb(modalities, "soll");
    ASSERT_NE(soll_modality, nullptr);
    
    EXPECT_EQ(soll_modality->category, "default_obligation");
    EXPECT_FLOAT_EQ(soll_modality->strength, 0.8f);
    EXPECT_EQ(soll_modality->deontic_logic, "O_default(φ)");
    EXPECT_GT(soll_modality->context_requirements.size(), 0);
}

/**
 * @test Basic extraction of "kann" (may) - discretionary permission
 */
TEST_F(LegalModalityAnalyzerTest, ExtractKannModalVerb) {
    std::string text = "Die Behörde kann eine Fristverlängerung gewähren.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_GE(modalities.size(), 1);
    
    auto* kann_modality = findModalityByVerb(modalities, "kann");
    ASSERT_NE(kann_modality, nullptr);
    
    EXPECT_EQ(kann_modality->category, "permission");
    EXPECT_FLOAT_EQ(kann_modality->strength, 0.3f);
    EXPECT_EQ(kann_modality->deontic_logic, "P(φ)");
    EXPECT_GT(kann_modality->context_requirements.size(), 0);
}

/**
 * @test Multiple modal verbs in a single text
 */
TEST_F(LegalModalityAnalyzerTest, ExtractMultipleModalVerbs) {
    std::string text = "Der Antragsteller muss die Unterlagen einreichen. "
                       "Die Behörde soll binnen zwei Wochen entscheiden. "
                       "Die Genehmigung kann mit Auflagen versehen werden.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_EQ(modalities.size(), 3);
    
    // Verify all three modal verbs are found
    auto* muss = findModalityByVerb(modalities, "muss");
    auto* soll = findModalityByVerb(modalities, "soll");
    auto* kann = findModalityByVerb(modalities, "kann");
    
    EXPECT_NE(muss, nullptr);
    EXPECT_NE(soll, nullptr);
    EXPECT_NE(kann, nullptr);
    
    // Verify they are sorted by position
    EXPECT_LT(muss->position, soll->position);
    EXPECT_LT(soll->position, kann->position);
}

/**
 * @test Modal verbs are case-insensitive
 */
TEST_F(LegalModalityAnalyzerTest, CaseInsensitiveMatching) {
    std::string text = "MUSS die Behörde entscheiden?";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_GE(modalities.size(), 1);
    EXPECT_EQ(modalities[0].verb, "MUSS");
    EXPECT_EQ(modalities[0].category, "obligation");
}

/**
 * @test Word boundary detection - should not match substrings
 */
TEST_F(LegalModalityAnalyzerTest, WordBoundaryDetection) {
    // "vermuss" and "gekannt" should not match
    std::string text = "Es ist vermuss, aber nicht gekannt.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    // Should find no matches (if word boundaries work correctly)
    EXPECT_EQ(modalities.size(), 0);
}

/**
 * @test Real administrative law example
 */
TEST_F(LegalModalityAnalyzerTest, RealAdministrativeLawText) {
    std::string text = 
        "§ 42 VwVfG - Anhörung Beteiligter\n"
        "(1) Bevor ein Verwaltungsakt erlassen wird, der in Rechte eines Beteiligten eingreift, "
        "muss diesem Gelegenheit gegeben werden, sich zu den für die Entscheidung erheblichen "
        "Tatsachen zu äußern.\n"
        "(2) Von der Anhörung kann abgesehen werden, wenn sie nach den Umständen des Einzelfalls "
        "nicht geboten ist.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    // Should find "muss" (binding obligation) and "kann" (from "kann abgesehen werden")
    ASSERT_GT(modalities.size(), 0);
    
    bool found_muss = false;
    for (const auto& mod : modalities) {
        if (mod.verb == "muss") {
            found_muss = true;
            EXPECT_EQ(mod.category, "obligation");
            EXPECT_FLOAT_EQ(mod.strength, 1.0f);
        }
    }
    
    EXPECT_TRUE(found_muss);
}

/**
 * @test Empty text returns no modalities
 */
TEST_F(LegalModalityAnalyzerTest, EmptyTextReturnsNoModalities) {
    std::string text = "";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    EXPECT_EQ(modalities.size(), 0);
}

/**
 * @test Text without modal verbs returns no modalities
 */
TEST_F(LegalModalityAnalyzerTest, TextWithoutModalVerbs) {
    std::string text = "Die Behörde entscheidet über den Antrag.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    EXPECT_EQ(modalities.size(), 0);
}

/**
 * @test Deontic logic notation is correct
 */
TEST_F(LegalModalityAnalyzerTest, DeonticLogicNotation) {
    std::string text = "Der Antrag muss gestellt werden.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_GE(modalities.size(), 1);
    
    // "muss" should have O(φ) deontic notation
    auto* muss = findModalityByVerb(modalities, "muss");
    ASSERT_NE(muss, nullptr);
    EXPECT_EQ(muss->deontic_logic, "O(φ)");
}

/**
 * @test Context requirements are populated for "soll"
 */
TEST_F(LegalModalityAnalyzerTest, ContextRequirementsForSoll) {
    std::string text = "Die Genehmigung soll erteilt werden.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_GE(modalities.size(), 1);
    
    auto* soll = findModalityByVerb(modalities, "soll");
    ASSERT_NE(soll, nullptr);
    
    // Should have context requirements
    EXPECT_FALSE(soll->context_requirements.empty());
    
    // Check for expected requirements
    bool has_justification_req = false;
    for (const auto& req : soll->context_requirements) {
        if (req.find("Begründung") != std::string::npos) {
            has_justification_req = true;
        }
    }
    EXPECT_TRUE(has_justification_req);
}

/**
 * @test Context requirements are populated for "kann"
 */
TEST_F(LegalModalityAnalyzerTest, ContextRequirementsForKann) {
    std::string text = "Die Behörde kann Auflagen erteilen.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_GE(modalities.size(), 1);
    
    auto* kann = findModalityByVerb(modalities, "kann");
    ASSERT_NE(kann, nullptr);
    
    // Should have context requirements
    EXPECT_FALSE(kann->context_requirements.empty());
    
    // Check for proportionality requirement
    bool has_proportionality = false;
    for (const auto& req : kann->context_requirements) {
        if (req.find("Verhältnismäßigkeit") != std::string::npos) {
            has_proportionality = true;
        }
    }
    EXPECT_TRUE(has_proportionality);
}

/**
 * @test Position tracking is accurate
 */
TEST_F(LegalModalityAnalyzerTest, PositionTracking) {
    std::string text = "Der Antrag muss gestellt werden.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_GE(modalities.size(), 1);
    
    auto* muss = findModalityByVerb(modalities, "muss");
    ASSERT_NE(muss, nullptr);
    
    // "muss" should be at position 11 (after "Der Antrag ")
    EXPECT_EQ(muss->position, text.find("muss"));
}

/**
 * @test Strength values are correctly assigned
 */
TEST_F(LegalModalityAnalyzerTest, StrengthValues) {
    std::string text = "Es muss sein. Es soll sein. Es kann sein.";
    
    auto modalities = analyzer.extractLegalModalities(text, "de");
    
    ASSERT_EQ(modalities.size(), 3);
    
    auto* muss = findModalityByVerb(modalities, "muss");
    auto* soll = findModalityByVerb(modalities, "soll");
    auto* kann = findModalityByVerb(modalities, "kann");
    
    // Verify strength hierarchy: muss (1.0) > soll (0.8) > kann (0.3)
    ASSERT_NE(muss, nullptr);
    ASSERT_NE(soll, nullptr);
    ASSERT_NE(kann, nullptr);
    
    EXPECT_FLOAT_EQ(muss->strength, 1.0f);
    EXPECT_FLOAT_EQ(soll->strength, 0.8f);
    EXPECT_FLOAT_EQ(kann->strength, 0.3f);
    
    EXPECT_GT(muss->strength, soll->strength);
    EXPECT_GT(soll->strength, kann->strength);
}
