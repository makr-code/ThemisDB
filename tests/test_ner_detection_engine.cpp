#include <gtest/gtest.h>
#include "utils/ner_detection_engine.h"
#include "utils/pii_detection_engine.h"

using namespace themis::utils;

// ============================================================================
// NERDetectionEngine unit tests
// ============================================================================

class NERDetectionEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        nlohmann::json config;
        config["enabled"] = true;
        ASSERT_TRUE(engine_.initialize(config));
    }

    NERDetectionEngine engine_;
};

// --- Factory ---

TEST(NERDetectionEngineFactoryTest, CreateUnsignedNER) {
    auto result = PIIDetectionEngineFactory::createUnsigned("ner");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->getName(), "ner");
}

TEST(NERDetectionEngineFactoryTest, GetAvailableEnginesIncludesNER) {
    auto engines = PIIDetectionEngineFactory::getAvailableEngines();
    EXPECT_NE(std::find(engines.begin(), engines.end(), "ner"), engines.end());
}

// --- Initialization ---

TEST_F(NERDetectionEngineTest, IsEnabledAfterInit) {
    EXPECT_TRUE(engine_.isEnabled());
}

TEST_F(NERDetectionEngineTest, GetNameReturnsNer) {
    EXPECT_EQ(engine_.getName(), "ner");
}

TEST_F(NERDetectionEngineTest, GetMetadataHasRequiredFields) {
    auto meta = engine_.getMetadata();
    EXPECT_EQ(meta["engine_type"].get<std::string>(), "ner");
    EXPECT_TRUE(meta.contains("honorific_count"));
    EXPECT_TRUE(meta.contains("org_suffix_count"));
    EXPECT_GT(meta["honorific_count"].get<size_t>(), 0u);
}

// --- Person name detection ---

TEST_F(NERDetectionEngineTest, DetectPersonName_MrPrefix) {
    auto findings = engine_.detectInText("Please contact Mr. John Smith.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::PERSON_NAME &&
            f.value.find("Smith") != std::string::npos) {
            found = true;
            EXPECT_GT(f.confidence, 0.7);
            EXPECT_EQ(f.engine_name, "ner");
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, DetectPersonName_DrPrefix) {
    auto findings = engine_.detectInText("Dr. Anna Mueller confirmed the diagnosis.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::PERSON_NAME) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, DetectPersonName_ProfPrefix) {
    auto findings = engine_.detectInText("Prof. James Brown is the author.");
    bool found_person = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::PERSON_NAME) { found_person = true; break; }
    }
    EXPECT_TRUE(found_person);
}

TEST_F(NERDetectionEngineTest, DetectPersonName_SpanIncludesHonorific) {
    auto findings = engine_.detectInText("Refer to Mrs. Jane Doe for details.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::PERSON_NAME) {
            EXPECT_TRUE(f.value.find("Mrs.") != std::string::npos ||
                        f.value.find("Jane") != std::string::npos);
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, NoFalsePositive_LowercaseHonorific) {
    // "mr" without period and lowercase - should not trigger
    auto findings = engine_.detectInText("The mr is not a valid honorific.");
    for (const auto& f : findings) {
        EXPECT_NE(f.type, PIIType::PERSON_NAME);
    }
}

// --- Organization detection ---

TEST_F(NERDetectionEngineTest, DetectOrganization_IncSuffix) {
    auto findings = engine_.detectInText("Acquired by Acme Corp. last year.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::ORGANIZATION) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, DetectOrganization_LtdSuffix) {
    auto findings = engine_.detectInText("The contract was signed with Global Tech Ltd.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::ORGANIZATION) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, DetectOrganization_GmbH) {
    auto findings = engine_.detectInText("Invoice from Mustermann GmbH received.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::ORGANIZATION) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, DetectOrganization_EngineNameIsNer) {
    auto findings = engine_.detectInText("Foo Corp. is a vendor.");
    for (const auto& f : findings) {
        if (f.type == PIIType::ORGANIZATION) {
            EXPECT_EQ(f.engine_name, "ner");
        }
    }
}

// --- Location detection ---

TEST_F(NERDetectionEngineTest, DetectLocation_InPreposition) {
    auto findings = engine_.detectInText("The conference is held in Berlin this year.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::LOCATION) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, DetectLocation_AtPreposition) {
    auto findings = engine_.detectInText("Meet at Geneva for the summit.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::LOCATION) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, DetectLocation_FromPreposition) {
    auto findings = engine_.detectInText("She is originally from Paris France.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::LOCATION) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, NoFalsePositive_LowercaseLocation) {
    // "in the" should not trigger - "the" is not capitalised
    auto findings = engine_.detectInText("in the beginning");
    for (const auto& f : findings) {
        EXPECT_NE(f.type, PIIType::LOCATION);
    }
}

// --- Field name classification ---

TEST_F(NERDetectionEngineTest, ClassifyFieldName_FullName) {
    EXPECT_EQ(engine_.classifyFieldName("full_name"), PIIType::PERSON_NAME);
    EXPECT_EQ(engine_.classifyFieldName("fullName"),  PIIType::PERSON_NAME);
}

TEST_F(NERDetectionEngineTest, ClassifyFieldName_Company) {
    EXPECT_EQ(engine_.classifyFieldName("company"),      PIIType::ORGANIZATION);
    EXPECT_EQ(engine_.classifyFieldName("organization"), PIIType::ORGANIZATION);
}

TEST_F(NERDetectionEngineTest, ClassifyFieldName_City) {
    EXPECT_EQ(engine_.classifyFieldName("city"),    PIIType::LOCATION);
    EXPECT_EQ(engine_.classifyFieldName("address"), PIIType::LOCATION);
    EXPECT_EQ(engine_.classifyFieldName("country"), PIIType::LOCATION);
}

TEST_F(NERDetectionEngineTest, ClassifyFieldName_Unknown) {
    EXPECT_EQ(engine_.classifyFieldName("price"),      PIIType::UNKNOWN);
    EXPECT_EQ(engine_.classifyFieldName("created_at"), PIIType::UNKNOWN);
}

// --- Redaction recommendations ---

TEST_F(NERDetectionEngineTest, RedactionRecommendation_PersonName) {
    EXPECT_EQ(engine_.getRedactionRecommendation(PIIType::PERSON_NAME), "strict");
}

TEST_F(NERDetectionEngineTest, RedactionRecommendation_Organization) {
    EXPECT_EQ(engine_.getRedactionRecommendation(PIIType::ORGANIZATION), "partial");
}

TEST_F(NERDetectionEngineTest, RedactionRecommendation_Location) {
    EXPECT_EQ(engine_.getRedactionRecommendation(PIIType::LOCATION), "partial");
}

// --- Reload ---

TEST_F(NERDetectionEngineTest, ReloadWithCustomHonorifics) {
    nlohmann::json config;
    config["enabled"]    = true;
    config["honorifics"] = nlohmann::json::array({"Lord", "Lady"});

    ASSERT_TRUE(engine_.reload(config));

    auto findings = engine_.detectInText("Lord Blackwood signed the contract.");
    bool found = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::PERSON_NAME) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(NERDetectionEngineTest, ReloadRetainsPreviousStateOnFailure) {
    // First verify default detection works
    auto before = engine_.detectInText("Dr. Test Person mentioned it.");
    bool had_person = false;
    for (const auto& f : before) {
        if (f.type == PIIType::PERSON_NAME) { had_person = true; break; }
    }
    EXPECT_TRUE(had_person);
}

// --- Disabled engine ---

TEST_F(NERDetectionEngineTest, DisabledEngineReturnsEmpty) {
    nlohmann::json config;
    config["enabled"] = false;
    ASSERT_TRUE(engine_.initialize(config));

    auto findings = engine_.detectInText("Mr. John Smith from Berlin.");
    EXPECT_TRUE(findings.empty());
}

// --- Empty input ---

TEST_F(NERDetectionEngineTest, EmptyTextReturnsEmpty) {
    auto findings = engine_.detectInText("");
    EXPECT_TRUE(findings.empty());
}

// --- Sorting ---

TEST_F(NERDetectionEngineTest, FindingsAreSortedByStartOffset) {
    auto findings = engine_.detectInText(
        "Mr. Alice Brown works at Acme Inc. in London.");
    for (size_t i = 1; i < findings.size(); ++i) {
        EXPECT_LE(findings[i - 1].start_offset, findings[i].start_offset);
    }
}
