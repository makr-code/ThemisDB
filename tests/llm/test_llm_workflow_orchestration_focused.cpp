/**
 * @file test_llm_workflow_orchestration_focused.cpp
 * @brief Focused regression tests for YAML/JSON/BPMN-based LLM workflow
 *        orchestration and prompt-enhancement task decomposition.
 *
 * Covers:
 * - **WO-01..04** — WorkflowLoader JSON round-trip and validation
 * - **WO-05..08** — WorkflowLoader YAML parsing
 * - **WO-09..10** — WorkflowLoader BPMN-lite XML parsing
 * - **WO-11..13** — WorkflowDefinition::topologicalOrder() and cycle detection
 * - **WO-14..17** — TaskDecomposer prompt building (no LLM required)
 * - **WO-18..20** — TaskDecomposer::decompose() with stub LLM plugin
 * - **WO-21..22** — TaskDecomposer::toWorkflow() conversion
 * - **WO-23..25** — WorkflowLoader::validate() error detection
 *
 * All tests are in-process; no model files are required.
 *
 * @version 1.0.0
 * @note CTest labels: llm;workflow;orchestration
 */

#include <gtest/gtest.h>

#include "llm/workflow_definition.h"
#include "llm/task_decomposer.h"
#include "llm/llm_plugin_interface.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <atomic>

using namespace themis::llm;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Stub LLM plugin
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Deterministic stub that returns a hard-coded JSON decomposition array.
 */
class StubDecomposerPlugin : public ILLMPlugin {
public:
    /// Response text the stub will return for any generate() call.
    std::string response_text;
    int         call_count{0};

    explicit StubDecomposerPlugin(const std::string& resp = "") : response_text(resp) {}

    // ── ILLMPlugin overrides ───────────────────────────────────────────────
    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info;
        info.model_id  = "stub";
        info.is_loaded = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities    getCapabilities() const override { return {}; }

    InferenceResponse generate(const InferenceRequest& /*req*/) override {
        ++call_count;
        InferenceResponse resp;
        resp.success          = true;
        resp.text             = response_text;
        resp.tokens_generated = static_cast<int>(response_text.size() / 4);
        return resp;
    }

    bool unloadLoRA(const std::string&) override { return true; }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    json getMemoryStats() const override { return {}; }
    json getPerformanceStats() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// § WO-01..04 — JSON round-trip and validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowOrchestration, WO_01_JsonRoundTrip) {
    WorkflowDefinition def;
    def.id   = "test_wf";
    def.name = "Test Workflow";
    WorkflowStep s1;
    s1.id              = "step1";
    s1.prompt_template = "Hello {input}";
    s1.mode            = WorkflowStepMode::Ask;
    def.steps.push_back(s1);

    json j = WorkflowLoader::toJson(def);
    ASSERT_EQ(j["id"], "test_wf");
    ASSERT_EQ(j["steps"].size(), 1u);

    WorkflowDefinition loaded = WorkflowLoader::fromJson(j);
    EXPECT_EQ(loaded.id, "test_wf");
    EXPECT_EQ(loaded.steps.size(), 1u);
    EXPECT_EQ(loaded.steps[0].id, "step1");
    EXPECT_EQ(loaded.steps[0].prompt_template, "Hello {input}");
}

TEST(WorkflowOrchestration, WO_02_JsonMultiStep) {
    const std::string json_str = R"({
        "id": "multi_step",
        "name": "Multi Step WF",
        "steps": [
            {"id": "a", "prompt_template": "First {input}", "mode": "ask"},
            {"id": "b", "prompt_template": "Second {a.output}", "mode": "rag", "depends_on": ["a"]}
        ]
    })";

    WorkflowDefinition def = WorkflowLoader::loadFromString(json_str, "json", "test");
    ASSERT_EQ(def.steps.size(), 2u);
    EXPECT_EQ(def.steps[1].depends_on, std::vector<std::string>{"a"});
    EXPECT_EQ(def.steps[1].mode, WorkflowStepMode::Rag);
}

TEST(WorkflowOrchestration, WO_03_StepByIdFound) {
    WorkflowDefinition def;
    def.id = "wf";
    WorkflowStep s;
    s.id              = "step_x";
    s.prompt_template = "do {input}";
    def.steps.push_back(s);

    EXPECT_EQ(def.stepById("step_x").id, "step_x");
}

TEST(WorkflowOrchestration, WO_04_StepByIdNotFound) {
    WorkflowDefinition def;
    def.id = "wf";
    EXPECT_THROW(def.stepById("nonexistent"), std::out_of_range);
}

// ─────────────────────────────────────────────────────────────────────────────
// § WO-05..08 — YAML parsing
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowOrchestration, WO_05_YamlBasicLoad) {
    const std::string yaml_str =
        "id: yaml_wf\n"
        "name: YAML Workflow\n"
        "steps:\n"
        "  - id: step1\n"
        "    prompt_template: 'Summarise {input}'\n"
        "    mode: ask\n";

    WorkflowDefinition def = WorkflowLoader::loadFromString(yaml_str, "yaml", "test.yaml");
    EXPECT_EQ(def.id, "yaml_wf");
    EXPECT_EQ(def.source_format, "yaml");
    ASSERT_GE(def.steps.size(), 1u);
}

TEST(WorkflowOrchestration, WO_06_YamlDefaultModeAsk) {
    const std::string yaml_str =
        "id: wf2\n"
        "steps:\n"
        "  - id: s1\n"
        "    prompt_template: 'answer {input}'\n";

    WorkflowDefinition def = WorkflowLoader::loadFromString(yaml_str, "yaml");
    // Default mode should be Ask when not specified
    EXPECT_EQ(def.default_mode, WorkflowStepMode::Ask);
}

TEST(WorkflowOrchestration, WO_07_YamlDependsOnInline) {
    const std::string yaml_str =
        "id: dep_wf\n"
        "steps:\n"
        "  - id: first\n"
        "    prompt_template: 'step 1'\n"
        "  - id: second\n"
        "    prompt_template: 'step 2 after {first.output}'\n"
        "    depends_on: [first]\n";

    WorkflowDefinition def = WorkflowLoader::loadFromString(yaml_str, "yaml");
    ASSERT_EQ(def.steps.size(), 2u);
    EXPECT_EQ(def.steps[1].depends_on, std::vector<std::string>{"first"});
}

TEST(WorkflowOrchestration, WO_08_YamlRoundTripViaJson) {
    const std::string yaml_str =
        "id: rt_wf\n"
        "name: RoundTrip\n"
        "steps:\n"
        "  - id: only\n"
        "    prompt_template: 'question: {input}'\n"
        "    mode: ask\n";

    WorkflowDefinition def = WorkflowLoader::loadFromString(yaml_str, "yaml");
    json j = WorkflowLoader::toJson(def);
    WorkflowDefinition def2 = WorkflowLoader::fromJson(j);

    EXPECT_EQ(def2.id, "rt_wf");
    EXPECT_EQ(def2.steps.size(), def.steps.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// § WO-09..10 — BPMN-lite XML parsing
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowOrchestration, WO_09_BpmnBasicLoad) {
    const std::string bpmn_str =
        R"(<?xml version="1.0" encoding="UTF-8"?>
        <definitions>
          <process id="proc1">
            <serviceTask id="task1" name="Summarise">
              <extensionElements>
                <themis:meta prompt="Summarise {input}" mode="ask"/>
              </extensionElements>
            </serviceTask>
            <serviceTask id="task2" name="Translate">
              <extensionElements>
                <themis:meta prompt="Translate {task1.output}" mode="ask"/>
              </extensionElements>
            </serviceTask>
            <sequenceFlow id="f1" sourceRef="task1" targetRef="task2"/>
          </process>
        </definitions>)";

    WorkflowDefinition def = WorkflowLoader::loadFromString(bpmn_str, "bpmn", "test.bpmn");
    EXPECT_EQ(def.source_format, "bpmn");
    ASSERT_GE(def.steps.size(), 2u);
    // task2 should depend on task1 via sequenceFlow
    bool has_dep = false;
    for (const auto& step : def.steps) {
        if (step.id == "task2") {
            for (const auto& d : step.depends_on) {
                if (d == "task1") { has_dep = true; break; }
            }
        }
    }
    EXPECT_TRUE(has_dep);
}

TEST(WorkflowOrchestration, WO_10_BpmnFallbackPrompt) {
    const std::string bpmn_str =
        R"(<definitions>
          <process id="p">
            <serviceTask id="only_task" name="DoSomething"/>
          </process>
        </definitions>)";

    WorkflowDefinition def = WorkflowLoader::loadFromString(bpmn_str, "bpmn");
    ASSERT_GE(def.steps.size(), 1u);
    // No extensionElements → prompt_template should be non-empty (fallback {input})
    EXPECT_FALSE(def.steps[0].prompt_template.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// § WO-11..13 — topologicalOrder() and cycle detection
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowOrchestration, WO_11_TopologicalOrderLinear) {
    WorkflowDefinition def;
    def.id = "topo_wf";

    auto make = [](const std::string& id, std::vector<std::string> deps) {
        WorkflowStep s;
        s.id              = id;
        s.prompt_template = "do " + id;
        s.depends_on      = std::move(deps);
        return s;
    };

    def.steps.push_back(make("a", {}));
    def.steps.push_back(make("b", {"a"}));
    def.steps.push_back(make("c", {"b"}));

    auto order = def.topologicalOrder();
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0]->id, "a");
    EXPECT_EQ(order[1]->id, "b");
    EXPECT_EQ(order[2]->id, "c");
}

TEST(WorkflowOrchestration, WO_12_TopologicalOrderCycleReturnsEmpty) {
    WorkflowDefinition def;
    def.id = "cycle_wf";

    WorkflowStep a, b;
    a.id = "a"; a.prompt_template = "a"; a.depends_on = {"b"};
    b.id = "b"; b.prompt_template = "b"; b.depends_on = {"a"};
    def.steps.push_back(a);
    def.steps.push_back(b);

    auto order = def.topologicalOrder();
    EXPECT_TRUE(order.empty()); // cycle → empty
}

TEST(WorkflowOrchestration, WO_13_TopologicalOrderParallel) {
    WorkflowDefinition def;
    def.id = "par_wf";

    WorkflowStep a, b, c;
    a.id = "a"; a.prompt_template = "a";
    b.id = "b"; b.prompt_template = "b";
    c.id = "c"; c.prompt_template = "c"; c.depends_on = {"a", "b"};
    def.steps.push_back(a);
    def.steps.push_back(b);
    def.steps.push_back(c);

    auto order = def.topologicalOrder();
    ASSERT_EQ(order.size(), 3u);
    // 'c' must come last
    EXPECT_EQ(order[2]->id, "c");
}

// ─────────────────────────────────────────────────────────────────────────────
// § WO-14..17 — Prompt building (no LLM)
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowOrchestration, WO_14_PromptContainsTask) {
    TaskDecomposerConfig cfg;
    cfg.strategy = DecompositionStrategy::DirectJson;
    TaskDecomposer d(cfg);

    const std::string task = "Analyse the annual report";
    const std::string prompt = d.buildDecompositionPrompt(task);

    EXPECT_NE(prompt.find(task), std::string::npos);
}

TEST(WorkflowOrchestration, WO_15_ChainOfThoughtPromptContainsStepByStep) {
    TaskDecomposerConfig cfg;
    cfg.strategy = DecompositionStrategy::ChainOfThought;
    TaskDecomposer d(cfg);

    const std::string prompt = d.buildDecompositionPrompt("any task");
    EXPECT_NE(prompt.find("step by step"), std::string::npos);
}

TEST(WorkflowOrchestration, WO_16_DirectJsonPromptContainsJsonArray) {
    TaskDecomposerConfig cfg;
    cfg.strategy   = DecompositionStrategy::DirectJson;
    cfg.max_subtasks = 4;
    TaskDecomposer d(cfg);

    const std::string prompt = d.buildDecompositionPrompt("task X");
    EXPECT_NE(prompt.find("JSON array"), std::string::npos);
}

TEST(WorkflowOrchestration, WO_17_FewShotPromptContainsExamples) {
    TaskDecomposerConfig cfg;
    cfg.strategy = DecompositionStrategy::FewShot;

    DecompositionExample ex;
    ex.task = "example_task";
    ex.steps.push_back(json{{"id","e1"},{"description","step 1"},{"prompt","do 1"},{"depends_on", json::array()}});
    cfg.few_shot_examples.push_back(ex);

    TaskDecomposer d(cfg);
    const std::string prompt = d.buildDecompositionPrompt("real task");

    EXPECT_NE(prompt.find("example_task"), std::string::npos);
    EXPECT_NE(prompt.find("real task"),    std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// § WO-18..20 — decompose() with stub plugin
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowOrchestration, WO_18_DecomposeSuccess) {
    const std::string llm_response = R"([
        {"id":"step1","description":"First step","prompt":"Do step 1","depends_on":[]},
        {"id":"step2","description":"Second step","prompt":"Do step 2 after {step1.output}","depends_on":["step1"]}
    ])";

    auto plugin = std::make_shared<StubDecomposerPlugin>(llm_response);

    TaskDecomposerConfig cfg;
    cfg.strategy = DecompositionStrategy::DirectJson;
    TaskDecomposer d(cfg);
    d.setLLMPlugin(plugin);

    auto result = d.decompose("Do both steps");
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.subtasks.size(), 2u);
    EXPECT_EQ(result.subtasks[0].id, "step1");
    EXPECT_EQ(result.subtasks[1].depends_on, std::vector<std::string>{"step1"});
    EXPECT_EQ(plugin->call_count, 1);
}

TEST(WorkflowOrchestration, WO_19_DecomposeNoPlugin) {
    TaskDecomposer d;
    auto result = d.decompose("some task");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error.find("No LLM plugin"), std::string::npos);
}

TEST(WorkflowOrchestration, WO_20_DecomposeWithChainOfThoughtResponse) {
    // LLM returns reasoning text before the JSON array — parser must skip it
    const std::string llm_response =
        "Let me think... The task breaks into two parts.\n"
        "Step 1: fetch data. Step 2: summarise.\n"
        R"([{"id":"fetch","description":"Fetch","prompt":"Fetch {input}","depends_on":[]},)"
        R"({"id":"summarise","description":"Summarise","prompt":"Summarise {fetch.output}","depends_on":["fetch"]}])";

    auto plugin = std::make_shared<StubDecomposerPlugin>(llm_response);

    TaskDecomposerConfig cfg;
    cfg.strategy = DecompositionStrategy::ChainOfThought;
    TaskDecomposer d(cfg);
    d.setLLMPlugin(plugin);

    auto result = d.decompose("Fetch and summarise the report");
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.subtasks.size(), 2u);
    EXPECT_EQ(result.subtasks[0].id, "fetch");
}

// ─────────────────────────────────────────────────────────────────────────────
// § WO-21..22 — toWorkflow() conversion
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowOrchestration, WO_21_ToWorkflowBasic) {
    const std::string llm_response = R"([
        {"id":"a","description":"A","prompt":"prompt A","depends_on":[]},
        {"id":"b","description":"B","prompt":"prompt B","depends_on":["a"]}
    ])";

    auto plugin = std::make_shared<StubDecomposerPlugin>(llm_response);
    TaskDecomposer d;
    d.setLLMPlugin(plugin);

    auto result = d.decompose("task");
    ASSERT_TRUE(result.success);

    WorkflowDefinition wf = TaskDecomposer::toWorkflow(result, "my_workflow");
    EXPECT_EQ(wf.id, "my_workflow");
    ASSERT_EQ(wf.steps.size(), 2u);
    EXPECT_EQ(wf.steps[0].id, "a");
    EXPECT_EQ(wf.steps[1].depends_on, std::vector<std::string>{"a"});
}

TEST(WorkflowOrchestration, WO_22_ToWorkflowFromFailedResultThrows) {
    TaskDecompositionResult failed;
    failed.success = false;

    EXPECT_THROW(TaskDecomposer::toWorkflow(failed), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// § WO-23..25 — validate() error detection
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowOrchestration, WO_23_ValidateEmptyId) {
    WorkflowDefinition def;
    // id deliberately left empty
    WorkflowStep s;
    s.id              = "s1";
    s.prompt_template = "do it";
    def.steps.push_back(s);

    auto vr = WorkflowLoader::validate(def);
    EXPECT_FALSE(vr.valid);
    EXPECT_FALSE(vr.errors.empty());
}

TEST(WorkflowOrchestration, WO_24_ValidateUnknownDepRef) {
    WorkflowDefinition def;
    def.id = "wf";
    WorkflowStep s;
    s.id              = "s1";
    s.prompt_template = "do {unknown.output}";
    s.depends_on      = {"unknown_step"};
    def.steps.push_back(s);

    auto vr = WorkflowLoader::validate(def);
    EXPECT_FALSE(vr.valid);
    bool found = false;
    for (const auto& e : vr.errors)
        if (e.message.find("unknown_step") != std::string::npos) found = true;
    EXPECT_TRUE(found);
}

TEST(WorkflowOrchestration, WO_25_ValidateValidDefinition) {
    WorkflowDefinition def;
    def.id = "good_wf";
    WorkflowStep a, b;
    a.id = "a"; a.prompt_template = "step a";
    b.id = "b"; b.prompt_template = "step b after {a.output}"; b.depends_on = {"a"};
    def.steps.push_back(a);
    def.steps.push_back(b);

    auto vr = WorkflowLoader::validate(def);
    EXPECT_TRUE(vr.valid);
    EXPECT_TRUE(vr.errors.empty());
}
