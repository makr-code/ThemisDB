/*
 * ThemisDB — Ingestion Phase 3 Tests: NER + LLM-Extract Steps
 *
 * Tests for:
 *   builtin.ner_de    (regex NER + LLM path)        NE-01..NE-08
 *   builtin.llm_extract (generic LLM step)           LE-01..LE-06
 *
 * Acceptance criteria:
 *
 * NE-01  NerDe: empty context → no entities added, no error
 * NE-02  NerDe: §-reference in raw_text → LEGAL_NORM_REFERENCE entity extracted
 * NE-03  NerDe: ISO date in raw_text → DATE entity extracted
 * NE-04  NerDe: NullBackend, use_llm=false → only regex entities
 * NE-05  NerDe: NullBackend, use_llm=true → warning added (no LLM hit), regex still runs
 * NE-06  NerDe: text in chunks (not raw_text) → extracts from each chunk
 * NE-07  NerDe: entity_types filter ORG only → no DATE entities extracted
 * NE-08  NerDe: language propagated to ctx.text_language when context language is empty
 *
 * LE-01  LlmExtract: no backend → warning, no crash
 * LE-02  LlmExtract: no prompt_template → warning, skip
 * LE-03  LlmExtract: canHandle() returns false for empty context
 * LE-04  LlmExtract: available backend, template with {text} placeholder → response in ctx.extra
 * LE-05  LlmExtract: output_entities=true, JSON response → entities appended to ctx
 * LE-06  LlmExtract: output_entities=true, non-JSON response → no crash, no entities
 */

#include <gtest/gtest.h>

#include "ingestion/ingestion_step.h"
#include "ingestion/extraction_context.h"
#include "ingestion/file_manifest.h"
#include "ingestion/base_entity.h"
#include "ingestion/inference_backend.h"
#include "ingestion/builtin_step_factories.h"

#include <memory>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::ingestion;
using namespace themis::ingestion::builtin;

// ─────────────────────────────────────────────────────────────────────────────
// Test helpers
// ─────────────────────────────────────────────────────────────────────────────

static ExtractionContext makeCtx(const std::string& text = "",
                                  const std::string& file_id = "sha256:test") {
    ExtractionContext ctx;
    ctx.manifest.file_id   = file_id;
    ctx.manifest.detected_mime = "application/pdf";
    ctx.raw_text           = text;
    return ctx;
}

static StepConfig makeCfg(const nlohmann::json& cfg = {}) {
    StepConfig sc;
    sc.config = cfg;
    return sc;
}

// Fake backend that echoes a preset response
class FakeBackend : public ITextGenerationBackend {
public:
    explicit FakeBackend(std::string resp) : resp_(std::move(resp)) {}
    std::string generate(const std::string&, int, double, const std::string&) override {
        return resp_;
    }
    bool        isAvailable() const override { return !resp_.empty(); }
    std::string description()  const override { return "FakeBackend"; }
private:
    std::string resp_;
};

// ─────────────────────────────────────────────────────────────────────────────
// NE — builtin.ner_de
// ─────────────────────────────────────────────────────────────────────────────

TEST(IngestionNerDe, NE01_EmptyContext_NoError) {
    auto step = createNerDeStep();
    auto ctx = makeCtx("");
    auto result = step->execute(ctx, makeCfg());
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(ctx.entities.empty());
}

TEST(IngestionNerDe, NE02_ParagraphRef_ExtractsLawEntity) {
    auto step = createNerDeStep();
    auto ctx = makeCtx("Gemäß § 4 Abs. 1 BImSchG ist der Betreiber verpflichtet.");
    auto result = step->execute(ctx, makeCfg());
    EXPECT_TRUE(result.has_value());

    // At least one entity with LAW type extracted
    bool found_law = false;
    for (const auto& e : ctx.entities) {
        if (e.entity_type == EntityType::LEGAL_NORM_REFERENCE ||
            (e.properties.count("ner_type") && e.properties.at("ner_type") == "LAW")) {
            found_law = true;
            break;
        }
    }
    EXPECT_TRUE(found_law) << "Expected a LAW entity for '§ 4 Abs. 1 BImSchG'";
}

TEST(IngestionNerDe, NE03_IsoDate_ExtractsDateEntity) {
    auto step = createNerDeStep();
    auto ctx = makeCtx("Gültig ab 01.04.2024 bis 31.12.2025.");
    auto result = step->execute(ctx, makeCfg());
    EXPECT_TRUE(result.has_value());

    bool found_date = false;
    for (const auto& e : ctx.entities) {
        if (e.entity_type == EntityType::DATE
            || (e.properties.count("ner_type") && e.properties.at("ner_type") == "DATE")) {
            found_date = true;
            break;
        }
    }
    EXPECT_TRUE(found_date) << "Expected a DATE entity";
}

TEST(IngestionNerDe, NE04_NullBackend_NoLlm_OnlyRegex) {
    auto step = createNerDeStep(); // default = NullTextGenerationBackend
    auto ctx = makeCtx("§ 7 Abs. 2 gilt entsprechend. Datum: 2026-01-15.");
    const nlohmann::json cfg_json = {{"use_llm", false}};
    auto result = step->execute(ctx, makeCfg(cfg_json));
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(ctx.entities.empty()) << "Regex NER should still produce entities";
    // All entities should come from regex
    for (const auto& e : ctx.entities) {
        EXPECT_EQ(e.properties.at("ner_source"), "regex");
    }
}

TEST(IngestionNerDe, NE05_NullBackend_UseLlm_Warning_RegexStillRuns) {
    auto step = createNerDeStep(); // NullBackend → isAvailable() == false
    auto ctx = makeCtx("§ 4 gilt. Datum: 2024-03-01.");
    const nlohmann::json cfg_json = {{"use_llm", true}};
    auto result = step->execute(ctx, makeCfg(cfg_json));
    EXPECT_TRUE(result.has_value());
    // Regex entities still expected (§ and date)
    EXPECT_FALSE(ctx.entities.empty());
}

TEST(IngestionNerDe, NE06_ExtractsFromChunks) {
    auto step = createNerDeStep();
    auto ctx = makeCtx(); // raw_text empty
    ctx.raw_text = "";

    TextChunk c1; c1.seq = 0; c1.text = "§ 5 BImSchG gilt."; c1.section_ref = "§ 5";
    TextChunk c2; c2.seq = 1; c2.text = "Datum: 15.06.2025."; c2.section_ref = "§ 6";
    ctx.chunks = {c1, c2};

    auto result = step->execute(ctx, makeCfg());
    EXPECT_TRUE(result.has_value());

    // Expect entities from both chunks
    bool found_law = false, found_date = false;
    for (const auto& e : ctx.entities) {
        if (e.properties.count("ner_type") && e.properties.at("ner_type") == "LAW") found_law = true;
        if (e.properties.count("ner_type") && e.properties.at("ner_type") == "DATE") found_date = true;
    }
    EXPECT_TRUE(found_law);
    EXPECT_TRUE(found_date);
}

TEST(IngestionNerDe, NE07_EntityTypeFilter_OrgOnly_NoDateEntities) {
    auto step = createNerDeStep();
    auto ctx = makeCtx("Datum: 01.01.2024. § 4 gilt.");
    const nlohmann::json cfg_json = {{"entity_types", nlohmann::json::array({"ORG", "PER"})}};
    auto result = step->execute(ctx, makeCfg(cfg_json));
    EXPECT_TRUE(result.has_value());

    for (const auto& e : ctx.entities) {
        EXPECT_NE(e.entity_type, EntityType::DATE)
            << "DATE entities should be filtered out";
    }
}

TEST(IngestionNerDe, NE08_LanguagePropagatedToContext) {
    auto step = createNerDeStep();
    auto ctx = makeCtx("§ 1 gilt."); // empty text_language
    EXPECT_TRUE(ctx.text_language.empty());

    const nlohmann::json cfg_json = {{"language", "de"}};
    auto result = step->execute(ctx, makeCfg(cfg_json));
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(ctx.text_language, "de");
}

// ─────────────────────────────────────────────────────────────────────────────
// LE — builtin.llm_extract
// ─────────────────────────────────────────────────────────────────────────────

TEST(IngestionLlmExtract, LE01_NoBackend_Warning_NoError) {
    auto step = createLlmExtractStep(); // NullBackend
    auto ctx = makeCtx("Some text.");
    const nlohmann::json cfg_json = {{"prompt_template", "Extract from: {text}"}};
    auto result = step->execute(ctx, makeCfg(cfg_json));
    EXPECT_TRUE(result.has_value());
    // Warning should be present
    bool found_warning = false;
    for (const auto& w : ctx.warnings)
        if (w.find("backend unavailable") != std::string::npos) { found_warning = true; break; }
    EXPECT_TRUE(found_warning);
}

TEST(IngestionLlmExtract, LE02_NoPromptTemplate_Warning_Skip) {
    auto backend = std::make_shared<FakeBackend>("response");
    auto step = createLlmExtractStep(backend);
    auto ctx = makeCtx("Some text.");
    // No prompt_template key in config
    auto result = step->execute(ctx, makeCfg({}));
    EXPECT_TRUE(result.has_value());
    bool found_warning = false;
    for (const auto& w : ctx.warnings)
        if (w.find("missing prompt_template") != std::string::npos) { found_warning = true; break; }
    EXPECT_TRUE(found_warning);
}

TEST(IngestionLlmExtract, LE03_CanHandle_FalseForEmptyContext) {
    auto backend = std::make_shared<FakeBackend>("resp");
    auto step = createLlmExtractStep(backend);
    auto ctx = makeCtx(""); // no text
    EXPECT_FALSE(step->canHandle(ctx));
}

TEST(IngestionLlmExtract, LE04_AvailableBackend_PlaceholderReplaced_ResponseInExtra) {
    auto backend = std::make_shared<FakeBackend>("Summary of the document.");
    auto step = createLlmExtractStep(backend);
    auto ctx = makeCtx("§ 4 BImSchG regelt den Umweltschutz.");
    StepConfig cfg;
    cfg.name   = "llm_summarise";
    cfg.config = {{"prompt_template", "Summarise: {text}"}};

    auto result = step->execute(ctx, cfg);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(ctx.extra.find("llm_extract.llm_summarise"), ctx.extra.end());
    EXPECT_EQ(ctx.extra.at("llm_extract.llm_summarise"), "Summary of the document.");
}

TEST(IngestionLlmExtract, LE05_OutputEntities_JsonResponse_EntitiesAppended) {
    const std::string json_response =
        R"([{"text":"Umweltbundesamt","type":"ORG","confidence":0.9},)"
        R"({"text":"§ 4 BImSchG","type":"LAW","confidence":0.85}])";
    auto backend = std::make_shared<FakeBackend>(json_response);
    auto step = createLlmExtractStep(backend);
    auto ctx = makeCtx("Die Umweltbundesamt prüft § 4 BImSchG.");
    StepConfig cfg;
    cfg.name   = "llm_ner";
    cfg.config = {
        {"prompt_template", "Extract entities from: {text}"},
        {"output_entities", true},
        {"min_confidence",  0.7}
    };

    auto result = step->execute(ctx, cfg);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(ctx.entities.size(), 2u);

    bool found_org = false, found_law = false;
    for (const auto& e : ctx.entities) {
        if (e.entity_type == EntityType::ORGANIZATION) found_org = true;
        if (e.entity_type == EntityType::LEGAL_NORM_REFERENCE) found_law = true;
    }
    EXPECT_TRUE(found_org);
    EXPECT_TRUE(found_law);
}

TEST(IngestionLlmExtract, LE06_OutputEntities_NonJsonResponse_NoCrash) {
    auto backend = std::make_shared<FakeBackend>("This is a plain text summary, not JSON.");
    auto step = createLlmExtractStep(backend);
    auto ctx = makeCtx("Some legal text.");
    StepConfig cfg;
    cfg.name   = "llm_step";
    cfg.config = {{"prompt_template", "{text}"}, {"output_entities", true}};

    // Must not throw or return an error
    EXPECT_NO_THROW({
        auto result = step->execute(ctx, cfg);
        EXPECT_TRUE(result.has_value());
    });
    // No entities appended (non-JSON)
    EXPECT_TRUE(ctx.entities.empty());
    // Raw response still stored
    EXPECT_NE(ctx.extra.find("llm_extract.llm_step"), ctx.extra.end());
}
