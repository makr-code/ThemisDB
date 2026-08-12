/*
 * ThemisDB — ToolRegistry Plugin-Engine Integration Tests
 *
 * Tests for:
 *   ToolRegistry (static/built-in tools)    TR-01..TR-08
 *   ToolRegistry (IThemisTool interface)    TR-09..TR-13
 *   ToolRegistry (dynamic plugin loading)   TR-14..TR-15
 *
 * Acceptance criteria:
 *
 * Static registration (TR)
 *   TR-01  Empty registry: listTools() returns empty vector
 *   TR-02  registerTool() adds the tool; listTools() returns its name
 *   TR-03  getSpec() returns the spec for a registered tool
 *   TR-04  getSpec() returns nullopt for an unknown tool
 *   TR-05  isAllowed() returns false for an empty allowlist
 *   TR-06  isAllowed() returns true for wildcard "*" in allowlist
 *   TR-07  isAllowed() returns false when tool is on denylist
 *   TR-08  invokeTool() returns the handler's result for a permitted tool
 *
 * IThemisTool interface (TR-09..TR-13)
 *   TR-09  IThemisTool::getType() returns PluginType::AGENTIC_TOOL
 *   TR-10  IThemisTool::execute() is invoked through a ToolHandler wrapper
 *   TR-11  IThemisTool::inputSchema() is stored in ToolSpec::args_schema
 *   TR-12  IThemisTool::executeAsync() resolves to same result as execute()
 *   TR-13  registerPluginTool logic: name from getName(); handler calls execute()
 *
 * Dynamic loading guard tests (TR-14..TR-15)
 *   TR-14  loadToolPlugin() with nonexistent path returns an error Result
 *   TR-15  isPluginTool() returns false for statically registered tools
 */

#include <gtest/gtest.h>

#include "llm/ai_orchestrator.h"
#include "llm/themis_tool_interface.h"
#include "plugins/plugin_interface.h"
#include "utils/error_registry.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace themis;
using namespace themis::llm;
using namespace themis::plugins;

// =============================================================================
// Minimal stub implementations used by tests
// =============================================================================

/// A minimal in-process tool that echoes its input back.
class EchoTool : public IThemisTool {
public:
    // IThemisPlugin
    const char* getName()    const override { return "echo"; }
    const char* getVersion() const override { return "1.0.0"; }
    PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char* /*config_json*/) override { return true; }
    void  shutdown()                              override {}
    void* getInstance()                           override { return this; }

    // IThemisTool
    json inputSchema()  const override { return {{"type", "object"}}; }
    json outputSchema() const override { return {{"type", "object"}}; }
    json execute(const json& input) override { return input; }
};

/// A non-tool plugin — used to test the ERR_TOOL_PLUGIN_NOT_A_TOOL guard.
class NotATool : public IThemisPlugin {
public:
    const char* getName()    const override { return "not_a_tool"; }
    const char* getVersion() const override { return "0.0.1"; }
    PluginType  getType()    const override { return PluginType::CUSTOM; }
    PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }
};

// Convenience: build a permissive ModeSpec for dispatch tests.
static ModeSpec permissiveMode() {
    ModeSpec m;
    m.id = "test_mode";
    m.tools_allowed = {"*"};
    return m;
}

static ModeSpec restrictiveMode() {
    ModeSpec m;
    m.id = "restricted";
    // empty allowlist → no tools permitted
    return m;
}

// =============================================================================
// TR-01..TR-08  Static registration tests
// =============================================================================

TEST(ToolRegistry, TR01_EmptyRegistry_ListToolsIsEmpty) {
    ToolRegistry reg;
    EXPECT_TRUE(reg.listTools().empty());
}

TEST(ToolRegistry, TR02_RegisterTool_AppearsInList) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name = "my_tool";
    reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{{"ok", true}}; });

    auto tools = reg.listTools();
    ASSERT_EQ(tools.size(), 1u);
    EXPECT_EQ(tools[0], "my_tool");
}

TEST(ToolRegistry, TR03_GetSpec_ReturnsRegisteredSpec) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name        = "calc";
    spec.description = "A calculator tool";
    reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{}; });

    auto got = reg.getSpec("calc");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->name, "calc");
    EXPECT_EQ(got->description, "A calculator tool");
}

TEST(ToolRegistry, TR04_GetSpec_NulloptForUnknown) {
    ToolRegistry reg;
    EXPECT_FALSE(reg.getSpec("unknown_tool").has_value());
}

TEST(ToolRegistry, TR05_IsAllowed_FalseForEmptyAllowlist) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name = "t";
    reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{}; });

    EXPECT_FALSE(reg.isAllowed("t", restrictiveMode()));
}

TEST(ToolRegistry, TR06_IsAllowed_TrueForWildcard) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name = "t";
    reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{}; });

    EXPECT_TRUE(reg.isAllowed("t", permissiveMode()));
}

TEST(ToolRegistry, TR07_IsAllowed_FalseWhenOnDenylist) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name = "dangerous";
    reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{}; });

    ModeSpec mode;
    mode.tools_allowed = {"*"};
    mode.tools_denied  = {"dangerous"};
    EXPECT_FALSE(reg.isAllowed("dangerous", mode));
}

TEST(ToolRegistry, TR08_InvokeTool_ReturnsHandlerResult) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name = "greet";
    reg.registerTool(spec, [](const json& args, const ModeSpec&) -> json {
        return {{"greeting", "hello " + args.value("name", "world")}};
    });

    json result = reg.invokeTool("greet", {{"name", "Alice"}}, permissiveMode());
    EXPECT_EQ(result["greeting"], "hello Alice");
}

// =============================================================================
// TR-09..TR-13  IThemisTool interface tests
// =============================================================================

TEST(ToolRegistry, TR09_IThemisTool_GetType_IsAgenticTool) {
    EchoTool tool;
    EXPECT_EQ(tool.getType(), PluginType::AGENTIC_TOOL);
}

TEST(ToolRegistry, TR10_IThemisTool_Execute_ReturnsInput) {
    EchoTool tool;
    json input = {{"key", "value"}};
    EXPECT_EQ(tool.execute(input), input);
}

TEST(ToolRegistry, TR11_IThemisTool_InputSchema_StoredInSpec) {
    // Simulate what registerPluginTool does
    EchoTool tool;
    ToolSpec spec;
    spec.name        = tool.getName();
    spec.args_schema = tool.inputSchema();

    EXPECT_EQ(spec.args_schema, json({{"type", "object"}}));
}

TEST(ToolRegistry, TR12_IThemisTool_ExecuteAsync_SameResultAsExecute) {
    EchoTool tool;
    json input = {{"async", true}};
    auto fut = tool.executeAsync(input);
    EXPECT_EQ(fut.get(), tool.execute(input));
}

TEST(ToolRegistry, TR13_RegisterPluginTool_DispatchesViaHandler) {
    // Manually wire an IThemisTool as a handler (simulates registerPluginTool)
    EchoTool tool;
    ToolSpec spec;
    spec.name = tool.getName();

    ToolRegistry reg;
    reg.registerTool(spec, [&tool](const json& args, const ModeSpec&) -> json {
        return tool.execute(args);
    });

    json input  = {{"x", 42}};
    json result = reg.invokeTool("echo", input, permissiveMode());
    EXPECT_EQ(result, input);
}

// =============================================================================
// TR-14..TR-15  Dynamic loading guard tests
// =============================================================================

TEST(ToolRegistry, TR14_LoadToolPlugin_NonexistentPath_ReturnsError) {
    ToolRegistry reg;
    auto res = reg.loadToolPlugin("/nonexistent/path/libtool_fake.so");
    EXPECT_FALSE(res.has_value());
    // PluginManager will return a plugin load error (ERR_PLUGIN_LOAD_FAILED or
    // similar), not a tool-specific error — accept any failure here.
}

TEST(ToolRegistry, TR15_IsPluginTool_FalseForStaticTool) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name = "static_tool";
    reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{}; });

    EXPECT_FALSE(reg.isPluginTool("static_tool"));
    EXPECT_FALSE(reg.isPluginTool("nonexistent"));
}
