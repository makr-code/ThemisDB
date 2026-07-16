"use strict";
/**
 * Unit tests for DelegationRouter.classify()
 *
 * Covers: all keyword sets, policy toggles, delegationMode overrides,
 * ThemisDB rules, edge cases (empty prompt, short prompt, mixed signals).
 *
 * Framework: Mocha (no VS Code API — DelegationRouter is framework-agnostic).
 */
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const strict_1 = __importDefault(require("node:assert/strict"));
const router_js_1 = require("../router.js");
// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
const ALL_POLICIES_ON = {
    routeBoilerplateToLocal: true,
    routeTestsToLocal: true,
    routeRefactorsToLocal: true,
    routeDocsToLocal: true,
    routeCmakeToLocal: true,
};
const ALL_POLICIES_OFF = {
    routeBoilerplateToLocal: false,
    routeTestsToLocal: false,
    routeRefactorsToLocal: false,
    routeDocsToLocal: false,
    routeCmakeToLocal: false,
};
function classify(prompt, opts = {}) {
    const router = new router_js_1.DelegationRouter();
    return router.classify(prompt, opts.lang ?? "", opts.themisDbRules ?? false, opts.mode ?? "auto", opts.defaultModel ?? "codellama:13b", opts.reasoningModel ?? "llama3", opts.policies ?? ALL_POLICIES_ON);
}
// ---------------------------------------------------------------------------
// delegationMode overrides
// ---------------------------------------------------------------------------
describe("DelegationRouter — delegationMode", () => {
    it("mode=always → ollama regardless of prompt", () => {
        const d = classify("security vulnerability audit", { mode: "always" });
        strict_1.default.equal(d.destination, "ollama");
    });
    it("mode=never → copilot regardless of prompt", () => {
        const d = classify("generate boilerplate class", { mode: "never" });
        strict_1.default.equal(d.destination, "copilot");
    });
    it("mode=always sets suggestedModel", () => {
        const d = classify("test", { mode: "always", defaultModel: "qwen2.5-coder:7b" });
        strict_1.default.equal(d.suggestedModel, "qwen2.5-coder:7b");
    });
    it("mode=never reason mentions delegationMode=never", () => {
        const d = classify("refactor this", { mode: "never" });
        strict_1.default.match(d.reason, /never/i);
    });
});
// ---------------------------------------------------------------------------
// ThemisDB rules — security always → copilot
// ---------------------------------------------------------------------------
describe("DelegationRouter — ThemisDB security override", () => {
    it("'security vulnerability' → copilot with themisDbRules=true", () => {
        const d = classify("review security vulnerability in this code", {
            themisDbRules: true,
            policies: ALL_POLICIES_ON,
        });
        strict_1.default.equal(d.destination, "copilot");
    });
    it("'audit cve' → copilot with themisDbRules=true", () => {
        const d = classify("audit this code for known CVE issues", {
            themisDbRules: true,
        });
        strict_1.default.equal(d.destination, "copilot");
    });
    it("security keywords without themisDbRules still route to copilot", () => {
        const d = classify("security audit of the login module", {
            themisDbRules: false,
        });
        strict_1.default.equal(d.destination, "copilot");
    });
});
// ---------------------------------------------------------------------------
// ThemisDB rules — C++ always → ollama
// ---------------------------------------------------------------------------
describe("DelegationRouter — ThemisDB C++ override", () => {
    it("activeLanguage=cpp → ollama with themisDbRules=true", () => {
        const d = classify("add a method to this class", {
            lang: "cpp",
            themisDbRules: true,
        });
        strict_1.default.equal(d.destination, "ollama");
    });
    it("activeLanguage=c → ollama with themisDbRules=true", () => {
        const d = classify("implement this function", {
            lang: "c",
            themisDbRules: true,
        });
        strict_1.default.equal(d.destination, "ollama");
    });
    it("prompt contains '.cpp' → ollama with themisDbRules=true", () => {
        const d = classify("generate a .cpp file for this class", {
            themisDbRules: true,
        });
        strict_1.default.equal(d.destination, "ollama");
    });
    it("C++ override does NOT trigger without themisDbRules", () => {
        // Without themisDbRules a pure ambiguous C++ prompt with no keyword set
        // may default to copilot; we only verify the override doesn't apply.
        const d = classify("describe what this does", {
            lang: "cpp",
            themisDbRules: false,
        });
        // No explicit C++ or test keyword — should not route to ollama via C++ override
        strict_1.default.notEqual(d.destination, "ollama" /* may differ if other keywords match */);
    });
});
// ---------------------------------------------------------------------------
// Boilerplate keyword set
// ---------------------------------------------------------------------------
describe("DelegationRouter — boilerplate routing", () => {
    const boilerplateCases = [
        "generate boilerplate for a REST controller",
        "scaffold a new TypeScript class",
        "create a class for entity management",
        "implement interface Observer for this class",
        "add getters and setters for all fields",
        "complete the constructor body",
        "fill in the remaining method stubs",
    ];
    for (const prompt of boilerplateCases) {
        it(`"${prompt.slice(0, 50)}" → ollama`, () => {
            const d = classify(prompt, { policies: ALL_POLICIES_ON });
            strict_1.default.equal(d.destination, "ollama", `Unexpected routing for: ${prompt}`);
        });
    }
    it("boilerplate prompt → copilot when routeBoilerplateToLocal=false", () => {
        const d = classify("generate boilerplate for a REST controller", {
            policies: { ...ALL_POLICIES_OFF, routeBoilerplateToLocal: false },
        });
        // Should not route via boilerplate keyword; other keywords absent → copilot
        strict_1.default.equal(d.destination, "copilot");
    });
});
// ---------------------------------------------------------------------------
// Test generation keyword set
// ---------------------------------------------------------------------------
describe("DelegationRouter — test routing", () => {
    const testCases = [
        "generate unit tests for this service",
        "write tests for the router class",
        "create mock for the database layer",
        "write a spec for this module",
        "add GTest fixture for the parser",
    ];
    for (const prompt of testCases) {
        it(`"${prompt.slice(0, 50)}" → ollama`, () => {
            const d = classify(prompt, { policies: ALL_POLICIES_ON });
            strict_1.default.equal(d.destination, "ollama");
        });
    }
    it("test prompt → copilot when routeTestsToLocal=false", () => {
        const d = classify("write unit tests for this class", {
            policies: { ...ALL_POLICIES_OFF, routeTestsToLocal: false },
        });
        strict_1.default.equal(d.destination, "copilot");
    });
});
// ---------------------------------------------------------------------------
// Refactor keyword set
// ---------------------------------------------------------------------------
describe("DelegationRouter — refactor routing", () => {
    const refactorCases = [
        "refactor this function to use std::expected",
        "extract method from this long function",
        "convert this loop to a range-based for",
        "translate this class from Python to TypeScript",
        "format code according to style guide",
        "clean up this function",
    ];
    for (const prompt of refactorCases) {
        it(`"${prompt.slice(0, 50)}" → ollama`, () => {
            const d = classify(prompt, { policies: ALL_POLICIES_ON });
            strict_1.default.equal(d.destination, "ollama");
        });
    }
    it("refactor prompt → copilot when routeRefactorsToLocal=false", () => {
        const d = classify("refactor this class to use RAII", {
            policies: { ...ALL_POLICIES_OFF },
        });
        strict_1.default.equal(d.destination, "copilot");
    });
});
// ---------------------------------------------------------------------------
// Documentation keyword set
// ---------------------------------------------------------------------------
describe("DelegationRouter — doc routing", () => {
    const docCases = [
        "add Doxygen documentation to this header",
        "write JSDoc comments for this function",
        "update the README for the API module",
        "generate documentation for this class",
    ];
    for (const prompt of docCases) {
        it(`"${prompt.slice(0, 50)}" → ollama`, () => {
            const d = classify(prompt, { policies: ALL_POLICIES_ON });
            strict_1.default.equal(d.destination, "ollama");
        });
    }
    it("doc prompt → copilot when routeDocsToLocal=false", () => {
        const d = classify("add Doxygen documentation", {
            policies: { ...ALL_POLICIES_OFF },
        });
        strict_1.default.equal(d.destination, "copilot");
    });
});
// ---------------------------------------------------------------------------
// CMake keyword set
// ---------------------------------------------------------------------------
describe("DelegationRouter — CMake routing", () => {
    const cmakeCases = [
        "edit the CMakeLists.txt to add a new test target",
        "update cmake build system configuration",
        "add a test target to the cmake preset",
        "how do I set up a cmake build system for this project?",
    ];
    for (const prompt of cmakeCases) {
        it(`"${prompt.slice(0, 50)}" → ollama`, () => {
            const d = classify(prompt, { policies: ALL_POLICIES_ON });
            strict_1.default.equal(d.destination, "ollama");
        });
    }
    it("cmake prompt → copilot when routeCmakeToLocal=false", () => {
        const d = classify("update the cmake build system", {
            policies: { ...ALL_POLICIES_OFF },
        });
        strict_1.default.equal(d.destination, "copilot");
    });
});
// ---------------------------------------------------------------------------
// Copilot-only keyword set (architecture, debugging)
// ---------------------------------------------------------------------------
describe("DelegationRouter — copilot-only routing", () => {
    const copilotCases = [
        "analyze the architecture of this distributed system",
        "discuss design patterns for this use case",
        "what is the CAP theorem trade-off here?",
        "debug this race condition in the multithreaded code",
        "identify the root cause of this deadlock",
        "do a scalability analysis of this system",
    ];
    for (const prompt of copilotCases) {
        it(`"${prompt.slice(0, 50)}" → copilot`, () => {
            const d = classify(prompt, { policies: ALL_POLICIES_ON });
            strict_1.default.equal(d.destination, "copilot", `Expected copilot for: ${prompt}`);
        });
    }
});
// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------
describe("DelegationRouter — edge cases", () => {
    it("empty prompt → copilot (no keywords match)", () => {
        const d = classify("", { policies: ALL_POLICIES_ON });
        strict_1.default.equal(d.destination, "copilot");
    });
    it("whitespace-only prompt → copilot", () => {
        const d = classify("   ", { policies: ALL_POLICIES_ON });
        strict_1.default.equal(d.destination, "copilot");
    });
    it("unknown/unrelated prompt → copilot", () => {
        const d = classify("what is the weather today?", { policies: ALL_POLICIES_ON });
        strict_1.default.equal(d.destination, "copilot");
    });
    it("all policies off + no copilot keyword → copilot", () => {
        const d = classify("generate some code", { policies: ALL_POLICIES_OFF });
        strict_1.default.equal(d.destination, "copilot");
    });
    it("decision always has a non-empty reason", () => {
        const prompts = ["", "security audit", "generate boilerplate", "refactor"];
        for (const p of prompts) {
            const d = classify(p, { policies: ALL_POLICIES_ON });
            strict_1.default.ok(d.reason.length > 0, `Empty reason for prompt: "${p}"`);
        }
    });
    it("ollama decision has suggestedModel set", () => {
        const d = classify("generate boilerplate for a class", {
            policies: ALL_POLICIES_ON,
            defaultModel: "deepseek-coder-v2:16b",
        });
        strict_1.default.equal(d.destination, "ollama");
        strict_1.default.ok(d.suggestedModel, "suggestedModel should be set for ollama destination");
    });
    it("copilot decision has no suggestedModel", () => {
        const d = classify("review the security of this module", {
            policies: ALL_POLICIES_ON,
        });
        strict_1.default.equal(d.destination, "copilot");
        strict_1.default.equal(d.suggestedModel, undefined);
    });
});
// ---------------------------------------------------------------------------
// Confidence score
// ---------------------------------------------------------------------------
describe("DelegationRouter — confidence score", () => {
    it("delegationMode=always → confidence 1.0", () => {
        const d = classify("anything", { mode: "always" });
        strict_1.default.equal(d.confidence, 1.0);
    });
    it("delegationMode=never → confidence 1.0", () => {
        const d = classify("anything", { mode: "never" });
        strict_1.default.equal(d.confidence, 1.0);
    });
    it("ThemisDB security override → confidence 1.0", () => {
        const d = classify("security audit", { themisDbRules: true });
        strict_1.default.equal(d.confidence, 1.0);
    });
    it("ThemisDB C++ override → confidence 1.0", () => {
        const d = classify("implement this", { lang: "cpp", themisDbRules: true });
        strict_1.default.equal(d.confidence, 1.0);
    });
    it("keyword match (boilerplate) → confidence 0.8", () => {
        const d = classify("generate boilerplate for a controller", { policies: ALL_POLICIES_ON });
        strict_1.default.equal(d.destination, "ollama");
        strict_1.default.equal(d.confidence, 0.8);
    });
    it("keyword match (copilot-cloud) → confidence 0.8", () => {
        const d = classify("review the security model", { policies: ALL_POLICIES_ON });
        strict_1.default.equal(d.destination, "copilot");
        strict_1.default.equal(d.confidence, 0.8);
    });
    it("fallback (no match) → confidence 0.5", () => {
        const d = classify("what is the weather today?", { policies: ALL_POLICIES_ON });
        strict_1.default.equal(d.destination, "copilot");
        strict_1.default.equal(d.confidence, 0.5);
    });
    it("confidence is always a number in [0, 1]", () => {
        const prompts = ["", "security", "refactor", "boilerplate", "cmake", "docs", "test"];
        for (const p of prompts) {
            const d = classify(p, { policies: ALL_POLICIES_ON });
            strict_1.default.ok(d.confidence >= 0 && d.confidence <= 1, `Out of range for "${p}": ${d.confidence}`);
        }
    });
});
// ---------------------------------------------------------------------------
// Language-aware routing profiles
// ---------------------------------------------------------------------------
describe("DelegationRouter — language profiles", () => {
    it("built-in: cpp → deepseek-coder-v2:16b for ollama decisions", () => {
        const d = classify("generate boilerplate for a class", {
            lang: "cpp",
            themisDbRules: false,
            policies: ALL_POLICIES_ON,
            defaultModel: "codellama:13b",
        });
        strict_1.default.equal(d.destination, "ollama");
        strict_1.default.equal(d.suggestedModel, "deepseek-coder-v2:16b");
    });
    it("built-in: typescript → qwen2.5-coder:7b for ollama decisions", () => {
        const d = classify("refactor this function", {
            lang: "typescript",
            policies: ALL_POLICIES_ON,
            defaultModel: "codellama:13b",
        });
        strict_1.default.equal(d.destination, "ollama");
        strict_1.default.equal(d.suggestedModel, "qwen2.5-coder:7b");
    });
    it("built-in: rust → deepseek-coder-v2:16b", () => {
        const d = classify("generate unit tests for this module", {
            lang: "rust",
            policies: ALL_POLICIES_ON,
            defaultModel: "codellama:13b",
        });
        strict_1.default.equal(d.destination, "ollama");
        strict_1.default.equal(d.suggestedModel, "deepseek-coder-v2:16b");
    });
    it("user profile overrides built-in", () => {
        const d = classify("refactor this function", {
            lang: "cpp",
            policies: { ...ALL_POLICIES_ON, languageProfiles: { cpp: "codellama:7b" } },
            defaultModel: "codellama:13b",
        });
        strict_1.default.equal(d.destination, "ollama");
        strict_1.default.equal(d.suggestedModel, "codellama:7b");
    });
    it("unknown language → fallback to defaultModel", () => {
        const d = classify("generate boilerplate for a class", {
            lang: "cobol",
            policies: ALL_POLICIES_ON,
            defaultModel: "my-custom-model:3b",
        });
        strict_1.default.equal(d.destination, "ollama");
        strict_1.default.equal(d.suggestedModel, "my-custom-model:3b");
    });
    it("empty languageProfiles → built-in profile applies", () => {
        const d = classify("generate unit tests for this module", {
            lang: "python",
            policies: { ...ALL_POLICIES_ON, languageProfiles: {} },
            defaultModel: "codellama:13b",
        });
        strict_1.default.equal(d.destination, "ollama");
        strict_1.default.equal(d.suggestedModel, "qwen2.5-coder:7b");
    });
});
//# sourceMappingURL=router.test.js.map