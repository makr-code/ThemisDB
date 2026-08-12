#include <gtest/gtest.h>
#include "exporters/format_template.h"
#include "exporters/jsonl_llm_exporter.h"
#include "exporters/exporter_interface.h"
#include "storage/base_entity.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <ctime>
#ifdef _WIN32
    #include <io.h>
    #define access _access
    #define F_OK 0
#else
    #include <unistd.h>
#endif

using namespace themis::exporters;
using namespace themis;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helper: build a fully-populated entity
// ---------------------------------------------------------------------------
static BaseEntity makeFullEntity(
    const std::string& pk,
    const std::string& instruction,
    const std::string& input,
    const std::string& output,
    const std::string& user_msg,
    const std::string& assistant_resp,
    const std::string& system = ""
) {
    BaseEntity e;
    e.setPrimaryKey(pk);
    e.setField("question",            instruction);
    e.setField("context",             input);
    e.setField("answer",              output);
    e.setField("user_message",        user_msg);
    e.setField("assistant_response",  assistant_resp);
    if (!system.empty()) {
        e.setField("system_prompt", system);
    }
    return e;
}

// ============================================================================
// Unit tests for IFormatTemplate implementations
// ============================================================================

class FormatTemplateTest : public ::testing::Test {
protected:
    FormatTemplateFieldMapping defaultMapping() const {
        return {};  // uses all default field names
    }
};

// ---------------------------------------------------------------------------
// AlpacaTemplate
// ---------------------------------------------------------------------------

TEST_F(FormatTemplateTest, AlpacaTemplateType) {
    auto tpl = makeFormatTemplate(FormatTemplateType::ALPACA);
    ASSERT_NE(tpl, nullptr);
    EXPECT_EQ(tpl->name(), "alpaca");
}

TEST_F(FormatTemplateTest, AlpacaRenderWithInput) {
    auto tpl = makeFormatTemplate(FormatTemplateType::ALPACA);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e1");
    e.setField("question", "What is 2+2?");
    e.setField("context",  "Basic arithmetic");
    e.setField("answer",   "4");

    auto line = tpl->render(e, defaultMapping());
    ASSERT_FALSE(line.empty());

    auto j = json::parse(line);
    EXPECT_EQ(j["instruction"], "What is 2+2?");
    EXPECT_EQ(j["input"],       "Basic arithmetic");
    EXPECT_EQ(j["output"],      "4");
}

TEST_F(FormatTemplateTest, AlpacaRenderEmptyInputStillIncludesKey) {
    auto tpl = makeFormatTemplate(FormatTemplateType::ALPACA);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e2");
    e.setField("question", "Summarize.");
    e.setField("answer",   "A summary.");
    // no "context" field set

    auto line = tpl->render(e, defaultMapping());
    ASSERT_FALSE(line.empty());

    auto j = json::parse(line);
    EXPECT_EQ(j["instruction"], "Summarize.");
    EXPECT_EQ(j["output"],      "A summary.");
    // Alpaca spec: "input" key is present but empty when not supplied
    EXPECT_TRUE(j.contains("input"));
    EXPECT_EQ(j["input"], "");
}

TEST_F(FormatTemplateTest, AlpacaValidateFieldsMissingInstruction) {
    auto tpl = makeFormatTemplate(FormatTemplateType::ALPACA);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e3");
    e.setField("answer", "Some output");
    // no "question" field

    std::vector<std::string> missing;
    EXPECT_FALSE(tpl->validateFields(e, defaultMapping(), &missing));
    EXPECT_FALSE(missing.empty());
    EXPECT_EQ(missing[0], "question");
}

TEST_F(FormatTemplateTest, AlpacaValidateFieldsOk) {
    auto tpl = makeFormatTemplate(FormatTemplateType::ALPACA);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e4");
    e.setField("question", "Q");
    e.setField("answer",   "A");

    EXPECT_TRUE(tpl->validateFields(e, defaultMapping()));
}

TEST_F(FormatTemplateTest, AlpacaRenderMissingRequiredReturnsEmpty) {
    auto tpl = makeFormatTemplate(FormatTemplateType::ALPACA);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e5");
    // neither "question" nor "answer"

    EXPECT_TRUE(tpl->render(e, defaultMapping()).empty());
}

// ---------------------------------------------------------------------------
// ShareGPTTemplate
// ---------------------------------------------------------------------------

TEST_F(FormatTemplateTest, ShareGPTTemplateType) {
    auto tpl = makeFormatTemplate(FormatTemplateType::SHAREGPT);
    ASSERT_NE(tpl, nullptr);
    EXPECT_EQ(tpl->name(), "sharegpt");
}

TEST_F(FormatTemplateTest, ShareGPTRenderNoSystem) {
    auto tpl = makeFormatTemplate(FormatTemplateType::SHAREGPT);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e6");
    e.setField("user_message",       "Hello");
    e.setField("assistant_response", "Hi there");

    auto line = tpl->render(e, defaultMapping());
    ASSERT_FALSE(line.empty());

    auto j = json::parse(line);
    ASSERT_TRUE(j.contains("conversations"));
    auto& convs = j["conversations"];
    ASSERT_EQ(convs.size(), 2u);
    EXPECT_EQ(convs[0]["from"],  "human");
    EXPECT_EQ(convs[0]["value"], "Hello");
    EXPECT_EQ(convs[1]["from"],  "gpt");
    EXPECT_EQ(convs[1]["value"], "Hi there");
}

TEST_F(FormatTemplateTest, ShareGPTRenderWithSystem) {
    auto tpl = makeFormatTemplate(FormatTemplateType::SHAREGPT);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e7");
    e.setField("system_prompt",      "You are a helpful assistant.");
    e.setField("user_message",       "What is the capital of France?");
    e.setField("assistant_response", "Paris.");

    auto line = tpl->render(e, defaultMapping());
    ASSERT_FALSE(line.empty());

    auto j = json::parse(line);
    auto& convs = j["conversations"];
    ASSERT_EQ(convs.size(), 3u);
    EXPECT_EQ(convs[0]["from"],  "system");
    EXPECT_EQ(convs[0]["value"], "You are a helpful assistant.");
    EXPECT_EQ(convs[1]["from"],  "human");
    EXPECT_EQ(convs[2]["from"],  "gpt");
}

TEST_F(FormatTemplateTest, ShareGPTValidateFieldsMissingUser) {
    auto tpl = makeFormatTemplate(FormatTemplateType::SHAREGPT);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e8");
    e.setField("assistant_response", "A");

    std::vector<std::string> missing;
    EXPECT_FALSE(tpl->validateFields(e, defaultMapping(), &missing));
    EXPECT_FALSE(missing.empty());
}

// ---------------------------------------------------------------------------
// ChatMLTemplate
// ---------------------------------------------------------------------------

TEST_F(FormatTemplateTest, ChatMLTemplateType) {
    auto tpl = makeFormatTemplate(FormatTemplateType::CHATML);
    ASSERT_NE(tpl, nullptr);
    EXPECT_EQ(tpl->name(), "chatml");
}

TEST_F(FormatTemplateTest, ChatMLRenderNoSystem) {
    auto tpl = makeFormatTemplate(FormatTemplateType::CHATML);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e9");
    e.setField("user_message",       "What day is it?");
    e.setField("assistant_response", "It depends on when you ask.");

    auto line = tpl->render(e, defaultMapping());
    ASSERT_FALSE(line.empty());

    auto j = json::parse(line);
    ASSERT_TRUE(j.contains("messages"));
    auto& msgs = j["messages"];
    ASSERT_EQ(msgs.size(), 2u);
    EXPECT_EQ(msgs[0]["role"],    "user");
    EXPECT_EQ(msgs[0]["content"], "What day is it?");
    EXPECT_EQ(msgs[1]["role"],    "assistant");
    EXPECT_EQ(msgs[1]["content"], "It depends on when you ask.");
}

TEST_F(FormatTemplateTest, ChatMLRenderWithSystem) {
    auto tpl = makeFormatTemplate(FormatTemplateType::CHATML);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e10");
    e.setField("system_prompt",      "Always respond in French.");
    e.setField("user_message",       "How are you?");
    e.setField("assistant_response", "Je vais bien, merci.");

    auto line = tpl->render(e, defaultMapping());
    ASSERT_FALSE(line.empty());

    auto j = json::parse(line);
    auto& msgs = j["messages"];
    ASSERT_EQ(msgs.size(), 3u);
    EXPECT_EQ(msgs[0]["role"],    "system");
    EXPECT_EQ(msgs[0]["content"], "Always respond in French.");
    EXPECT_EQ(msgs[1]["role"],    "user");
    EXPECT_EQ(msgs[2]["role"],    "assistant");
}

TEST_F(FormatTemplateTest, ChatMLValidateFieldsOk) {
    auto tpl = makeFormatTemplate(FormatTemplateType::CHATML);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e11");
    e.setField("user_message",       "U");
    e.setField("assistant_response", "A");

    EXPECT_TRUE(tpl->validateFields(e, defaultMapping()));
}

// ---------------------------------------------------------------------------
// OpenAI fine-tuning template
// ---------------------------------------------------------------------------

TEST_F(FormatTemplateTest, OpenAIFineTuningType) {
    auto tpl = makeFormatTemplate(FormatTemplateType::OPENAI_FINETUNING);
    ASSERT_NE(tpl, nullptr);
    EXPECT_EQ(tpl->name(), "openai_finetuning");
}

TEST_F(FormatTemplateTest, OpenAIFineTuningRenderWithSystem) {
    auto tpl = makeFormatTemplate(FormatTemplateType::OPENAI_FINETUNING);
    ASSERT_NE(tpl, nullptr);

    BaseEntity e;
    e.setPrimaryKey("e12");
    e.setField("system_prompt",      "You are a concise assistant.");
    e.setField("user_message",       "Translate: Hello");
    e.setField("assistant_response", "Hola");

    auto line = tpl->render(e, defaultMapping());
    ASSERT_FALSE(line.empty());

    auto j = json::parse(line);
    ASSERT_TRUE(j.contains("messages"));
    auto& msgs = j["messages"];
    ASSERT_EQ(msgs.size(), 3u);
    EXPECT_EQ(msgs[0]["role"], "system");
    EXPECT_EQ(msgs[1]["role"], "user");
    EXPECT_EQ(msgs[2]["role"], "assistant");
}

// ---------------------------------------------------------------------------
// Factory: NONE returns nullptr
// ---------------------------------------------------------------------------

TEST_F(FormatTemplateTest, FactoryNoneReturnsNull) {
    EXPECT_EQ(makeFormatTemplate(FormatTemplateType::NONE), nullptr);
}

// ============================================================================
// Integration: JSONLLLMExporter with format templates
// ============================================================================

class FormatTemplateExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto tmp = std::filesystem::temp_directory_path();
        // Include PID to avoid collisions when tests run in parallel
        std::string unique = std::to_string(std::time(nullptr)) + "_" +
                             std::to_string(static_cast<int>(::getpid()));
        test_dir_ = (tmp / ("themis_fmttpl_" + unique)).string();
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::vector<BaseEntity> makeSampleEntities(int count = 5) const {
        std::vector<BaseEntity> entities;
        for (int i = 0; i < count; ++i) {
            entities.push_back(makeFullEntity(
                "ent_" + std::to_string(i),
                "Instruction " + std::to_string(i),
                "Input " + std::to_string(i),
                "Output " + std::to_string(i),
                "User message " + std::to_string(i),
                "Assistant response " + std::to_string(i),
                "System prompt"
            ));
        }
        return entities;
    }

    std::vector<std::string> readLines(const std::string& path) const {
        std::vector<std::string> lines;
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) lines.push_back(line);
        }
        return lines;
    }

    std::string test_dir_;
};

TEST_F(FormatTemplateExporterTest, AlpacaViaExporter) {
    JSONLLLMConfig cfg;
    cfg.format_template_type = FormatTemplateType::ALPACA;
    cfg.template_field_mapping.instruction_field = "question";
    cfg.template_field_mapping.input_field       = "context";
    cfg.template_field_mapping.output_field      = "answer";
    cfg.quality.min_text_length = 0;  // Allow short test answers

    std::vector<BaseEntity> entities;
    for (int i = 0; i < 3; ++i) {
        BaseEntity e;
        e.setPrimaryKey("a" + std::to_string(i));
        e.setField("question", "Q" + std::to_string(i));
        e.setField("context",  "C" + std::to_string(i));
        e.setField("answer",   "A" + std::to_string(i));
        entities.push_back(e);
    }

    JSONLLLMExporter exporter(cfg);
    ExportOptions opts;
    opts.output_path = test_dir_ + "/alpaca.jsonl";

    auto stats = exporter.exportEntities(entities, opts);
    EXPECT_EQ(stats.exported_entities, 3u);

    auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 3u);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("instruction"));
        EXPECT_TRUE(j.contains("input"));
        EXPECT_TRUE(j.contains("output"));
    }
}

TEST_F(FormatTemplateExporterTest, ShareGPTViaExporter) {
    JSONLLLMConfig cfg;
    cfg.format_template_type = FormatTemplateType::SHAREGPT;

    auto entities = makeSampleEntities(3);

    JSONLLLMExporter exporter(cfg);
    ExportOptions opts;
    opts.output_path = test_dir_ + "/sharegpt.jsonl";

    auto stats = exporter.exportEntities(entities, opts);
    EXPECT_EQ(stats.exported_entities, 3u);

    auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 3u);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("conversations"));
        EXPECT_GE(j["conversations"].size(), 2u);
    }
}

TEST_F(FormatTemplateExporterTest, ChatMLViaExporter) {
    JSONLLLMConfig cfg;
    cfg.format_template_type = FormatTemplateType::CHATML;

    auto entities = makeSampleEntities(3);

    JSONLLLMExporter exporter(cfg);
    ExportOptions opts;
    opts.output_path = test_dir_ + "/chatml.jsonl";

    auto stats = exporter.exportEntities(entities, opts);
    EXPECT_EQ(stats.exported_entities, 3u);

    auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 3u);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("messages"));
    }
}

TEST_F(FormatTemplateExporterTest, OpenAIViaExporter) {
    JSONLLLMConfig cfg;
    cfg.format_template_type = FormatTemplateType::OPENAI_FINETUNING;

    auto entities = makeSampleEntities(2);

    JSONLLLMExporter exporter(cfg);
    ExportOptions opts;
    opts.output_path = test_dir_ + "/openai.jsonl";

    auto stats = exporter.exportEntities(entities, opts);
    EXPECT_EQ(stats.exported_entities, 2u);

    auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 2u);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        ASSERT_TRUE(j.contains("messages"));
        // Every message must have "role" and "content"
        for (const auto& msg : j["messages"]) {
            EXPECT_TRUE(msg.contains("role"));
            EXPECT_TRUE(msg.contains("content"));
        }
    }
}

TEST_F(FormatTemplateExporterTest, SetConfigRecreatesTemplate) {
    JSONLLLMConfig cfg;
    cfg.format_template_type = FormatTemplateType::NONE;

    JSONLLLMExporter exporter(cfg);

    // Reconfigure to Alpaca
    JSONLLLMConfig cfg2;
    cfg2.format_template_type = FormatTemplateType::ALPACA;
    cfg2.quality.min_text_length = 0;  // Allow short test answers
    exporter.setConfig(cfg2);

    std::vector<BaseEntity> entities;
    BaseEntity e;
    e.setPrimaryKey("x1");
    e.setField("question", "Hello?");
    e.setField("answer",   "World");
    entities.push_back(e);

    ExportOptions opts;
    opts.output_path = test_dir_ + "/setconfig.jsonl";
    auto stats = exporter.exportEntities(entities, opts);
    EXPECT_EQ(stats.exported_entities, 1u);

    auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 1u);
    auto j = json::parse(lines[0]);
    EXPECT_TRUE(j.contains("instruction"));
    EXPECT_TRUE(j.contains("output"));
}

TEST_F(FormatTemplateExporterTest, MissingRequiredFieldsSkipsEntity) {
    JSONLLLMConfig cfg;
    cfg.format_template_type = FormatTemplateType::ALPACA;

    std::vector<BaseEntity> entities;
    BaseEntity e;
    e.setPrimaryKey("missing");
    // "answer" (output) is missing

    e.setField("question", "Q?");
    entities.push_back(e);

    JSONLLLMExporter exporter(cfg);
    ExportOptions opts;
    opts.output_path = test_dir_ + "/missing.jsonl";

    auto stats = exporter.exportEntities(entities, opts);
    // Entity is skipped because template render returns empty
    EXPECT_EQ(stats.exported_entities, 0u);
}

// ============================================================================
// validate_template dry-run API (free function)
// ============================================================================

class ValidateTemplateTest : public ::testing::Test {
protected:
    FormatTemplateFieldMapping defaultMapping() const { return {}; }

    BaseEntity makeAlpacaEntity(const std::string& pk,
                                 bool with_instruction = true,
                                 bool with_output = true) const {
        BaseEntity e;
        e.setPrimaryKey(pk);
        if (with_instruction) e.setField("question", "Q");
        if (with_output)      e.setField("answer",   "A");
        return e;
    }

    BaseEntity makeChatEntity(const std::string& pk,
                               bool with_user = true,
                               bool with_assistant = true) const {
        BaseEntity e;
        e.setPrimaryKey(pk);
        if (with_user)      e.setField("user_message",       "Hello");
        if (with_assistant) e.setField("assistant_response", "Hi");
        return e;
    }
};

// NONE type always passes regardless of sample content.
TEST_F(ValidateTemplateTest, NoneTypeAlwaysValid) {
    std::vector<BaseEntity> sample = {makeAlpacaEntity("e1", false, false)};
    auto result = validateTemplate(FormatTemplateType::NONE, defaultMapping(), sample);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.missing_fields.empty());
    EXPECT_EQ(result.entities_checked, 0u);
    EXPECT_EQ(result.entities_failed, 0u);
}

// Empty sample: valid, no entities inspected.
TEST_F(ValidateTemplateTest, EmptySampleIsValid) {
    auto result = validateTemplate(FormatTemplateType::ALPACA, defaultMapping(), {});
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.missing_fields.empty());
    EXPECT_EQ(result.entities_checked, 0u);
    EXPECT_EQ(result.entities_failed, 0u);
}

// Alpaca — all entities fully populated.
TEST_F(ValidateTemplateTest, AlpacaAllEntitiesValid) {
    std::vector<BaseEntity> sample = {
        makeAlpacaEntity("e1"),
        makeAlpacaEntity("e2"),
        makeAlpacaEntity("e3"),
    };
    auto result = validateTemplate(FormatTemplateType::ALPACA, defaultMapping(), sample);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.missing_fields.empty());
    EXPECT_EQ(result.entities_checked, 3u);
    EXPECT_EQ(result.entities_failed, 0u);
}

// Alpaca — one entity missing the instruction field.
TEST_F(ValidateTemplateTest, AlpacaMissingInstructionField) {
    std::vector<BaseEntity> sample = {
        makeAlpacaEntity("e1"),
        makeAlpacaEntity("e2", /*with_instruction=*/false, /*with_output=*/true),
    };
    auto result = validateTemplate(FormatTemplateType::ALPACA, defaultMapping(), sample);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.entities_checked, 2u);
    EXPECT_EQ(result.entities_failed, 1u);
    ASSERT_FALSE(result.missing_fields.empty());
    EXPECT_EQ(result.missing_fields[0], "question");
}

// Alpaca — both required fields missing in all entities.
TEST_F(ValidateTemplateTest, AlpacaBothRequiredFieldsMissing) {
    std::vector<BaseEntity> sample = {
        makeAlpacaEntity("e1", false, false),
        makeAlpacaEntity("e2", false, false),
    };
    auto result = validateTemplate(FormatTemplateType::ALPACA, defaultMapping(), sample);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.entities_checked, 2u);
    EXPECT_EQ(result.entities_failed, 2u);
    // Both "question" and "answer" must appear; list is sorted.
    ASSERT_EQ(result.missing_fields.size(), 2u);
    EXPECT_EQ(result.missing_fields[0], "answer");
    EXPECT_EQ(result.missing_fields[1], "question");
}

// Missing fields are deduplicated across entities.
TEST_F(ValidateTemplateTest, MissingFieldsAreDeduplicated) {
    std::vector<BaseEntity> sample = {
        makeAlpacaEntity("e1", false, true),
        makeAlpacaEntity("e2", false, true),
        makeAlpacaEntity("e3", false, true),
    };
    auto result = validateTemplate(FormatTemplateType::ALPACA, defaultMapping(), sample);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.entities_failed, 3u);
    // "question" appears only once even though 3 entities are missing it.
    ASSERT_EQ(result.missing_fields.size(), 1u);
    EXPECT_EQ(result.missing_fields[0], "question");
}

// ShareGPT — missing user and assistant fields.
TEST_F(ValidateTemplateTest, ShareGPTMissingUserField) {
    std::vector<BaseEntity> sample = {
        makeChatEntity("e1", /*with_user=*/false, /*with_assistant=*/true),
    };
    auto result = validateTemplate(FormatTemplateType::SHAREGPT, defaultMapping(), sample);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.missing_fields.empty());
    EXPECT_EQ(result.missing_fields[0], "user_message");
}

// ChatML — all valid.
TEST_F(ValidateTemplateTest, ChatMLAllEntitiesValid) {
    std::vector<BaseEntity> sample = {
        makeChatEntity("e1"),
        makeChatEntity("e2"),
    };
    auto result = validateTemplate(FormatTemplateType::CHATML, defaultMapping(), sample);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.entities_checked, 2u);
    EXPECT_EQ(result.entities_failed, 0u);
}

// OpenAI fine-tuning — missing assistant field.
TEST_F(ValidateTemplateTest, OpenAIFineTuningMissingAssistantField) {
    std::vector<BaseEntity> sample = {
        makeChatEntity("e1", true, false),
    };
    auto result = validateTemplate(FormatTemplateType::OPENAI_FINETUNING, defaultMapping(), sample);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.missing_fields.empty());
    EXPECT_EQ(result.missing_fields[0], "assistant_response");
}

// Custom field-name mapping is respected.
TEST_F(ValidateTemplateTest, CustomFieldMappingRespected) {
    FormatTemplateFieldMapping custom;
    custom.instruction_field = "my_instruction";
    custom.output_field      = "my_output";

    // Entity with default field names — should fail with custom mapping.
    BaseEntity e;
    e.setPrimaryKey("e1");
    e.setField("question", "Q");  // wrong field name
    e.setField("answer",   "A");  // wrong field name

    auto result = validateTemplate(FormatTemplateType::ALPACA, custom, {e});
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.entities_failed, 1u);
    // Both custom names must be in missing_fields.
    auto& mf = result.missing_fields;
    EXPECT_NE(std::find(mf.begin(), mf.end(), "my_instruction"), mf.end());
    EXPECT_NE(std::find(mf.begin(), mf.end(), "my_output"),      mf.end());
}

// ============================================================================
// validate_template via JSONLLLMExporter::validateTemplate()
// ============================================================================

TEST(ValidateTemplateExporterTest, ExporterDelegatesNoneType) {
    JSONLLLMConfig cfg;
    cfg.format_template_type = FormatTemplateType::NONE;
    JSONLLLMExporter exporter(cfg);

    BaseEntity e;
    e.setPrimaryKey("e1");
    // No fields at all — should still pass because type is NONE.
    auto result = exporter.validateTemplate({e});
    EXPECT_TRUE(result.valid);
}

TEST(ValidateTemplateExporterTest, ExporterReturnsCorrectMissingFields) {
    JSONLLLMConfig cfg;
    cfg.format_template_type = FormatTemplateType::ALPACA;
    // Keep default field mapping.
    JSONLLLMExporter exporter(cfg);

    BaseEntity ok_entity;
    ok_entity.setPrimaryKey("ok");
    ok_entity.setField("question", "Q");
    ok_entity.setField("answer",   "A");

    BaseEntity bad_entity;
    bad_entity.setPrimaryKey("bad");
    bad_entity.setField("question", "Q");
    // Missing "answer"

    auto result = exporter.validateTemplate({ok_entity, bad_entity});
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.entities_checked, 2u);
    EXPECT_EQ(result.entities_failed,  1u);
    ASSERT_EQ(result.missing_fields.size(), 1u);
    EXPECT_EQ(result.missing_fields[0], "answer");
}

TEST(ValidateTemplateExporterTest, ExporterUsesConfigFieldMapping) {
    JSONLLLMConfig cfg;
    cfg.format_template_type = FormatTemplateType::ALPACA;
    cfg.template_field_mapping.instruction_field = "prompt";
    cfg.template_field_mapping.output_field      = "completion";
    JSONLLLMExporter exporter(cfg);

    BaseEntity e;
    e.setPrimaryKey("e1");
    e.setField("prompt",     "P");
    e.setField("completion", "C");

    auto result = exporter.validateTemplate({e});
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.entities_checked, 1u);
    EXPECT_EQ(result.entities_failed,  0u);
}
