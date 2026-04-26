/**
 * Unit tests for ContextManager.buildPrompt()
 *
 * `setup.ts` (loaded via --require) redirects `require('vscode')` to
 * vscode-mock.ts so this runs without a real VS Code Extension Host.
 */

import assert from "node:assert/strict";
// eslint-disable-next-line @typescript-eslint/no-require-imports
const vscodeMock = require("./vscode-mock.js") as typeof import("./vscode-mock.js");
// eslint-disable-next-line @typescript-eslint/no-require-imports
const { ContextManager } = require("../contextManager.js") as typeof import("../contextManager.js");

const { DiagnosticSeverity, Range, Position } = vscodeMock;

function setEditor(
  opts: {
    fileName?: string;
    languageId?: string;
    selectedText?: string;
    cursorLine?: number;
    content?: string;
    diagnostics?: Array<{ severity: typeof DiagnosticSeverity[keyof typeof DiagnosticSeverity]; line: number; message: string }>;
  } = {}
): void {
  vscodeMock.setActiveEditor({
    fileName: opts.fileName ?? "/project/src/foo.ts",
    languageId: opts.languageId ?? "typescript",
    selectedText: opts.selectedText ?? "",
    cursorLine: opts.cursorLine ?? 10,
    content: opts.content ?? "",
    diagnostics: (opts.diagnostics ?? []).map((d) => ({
      severity: d.severity,
      range: new Range(new Position(d.line, 0), new Position(d.line, 80)),
      message: d.message,
    })),
  });
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

describe("ContextManager — no active editor", () => {
  beforeEach(() => vscodeMock.clearEditor());

  it("returns the user text unchanged when no editor is open", async () => {
    const cm = new ContextManager();
    const result = await cm.buildPrompt("hello world");
    assert.ok(result.text.includes("hello world"));
  });

  it("activeLanguage defaults to 'plaintext' when no editor", async () => {
    const cm = new ContextManager();
    const result = await cm.buildPrompt("hello");
    assert.equal(result.activeLanguage, "plaintext");
  });
});

describe("ContextManager — with active editor", () => {
  beforeEach(() => vscodeMock.clearEditor());

  it("includes file path in preamble", async () => {
    setEditor({ fileName: "/repo/src/main.cpp", languageId: "cpp" });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("refactor this");
    assert.ok(result.text.includes("/repo/src/main.cpp"), "file path missing");
  });

  it("includes language identifier", async () => {
    setEditor({ languageId: "rust" });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("explain this");
    assert.ok(result.text.includes("rust"));
    assert.equal(result.activeLanguage, "rust");
  });

  it("includes selected text in a fenced code block", async () => {
    setEditor({
      languageId: "typescript",
      selectedText: "const x = 42;",
    });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("document this");
    assert.ok(result.text.includes("const x = 42;"), "selected text missing");
    assert.ok(result.text.includes("```typescript"), "code fence missing");
  });

  it("includes cursor window when nothing is selected", async () => {
    setEditor({
      content: Array.from({ length: 100 }, (_, i) => `line ${i + 1}`).join("\n"),
      languageId: "python",
      selectedText: "",
      cursorLine: 50,
    });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("help");
    assert.ok(
      result.text.includes("Code near cursor"),
      "cursor window section missing"
    );
  });

  it("includes ERROR diagnostics", async () => {
    setEditor({
      diagnostics: [
        { severity: DiagnosticSeverity.Error, line: 5, message: "Type mismatch" },
      ],
    });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("fix this");
    assert.ok(result.text.includes("Type mismatch"), "diagnostic message missing");
    assert.ok(result.text.includes("ERROR"), "severity label missing");
  });

  it("includes WARNING diagnostics", async () => {
    setEditor({
      diagnostics: [
        { severity: DiagnosticSeverity.Warning, line: 3, message: "Unused variable" },
      ],
    });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("fix warnings");
    assert.ok(result.text.includes("Unused variable"));
    assert.ok(result.text.includes("WARN"));
  });

  it("limits diagnostics to 10 entries", async () => {
    const diags = Array.from({ length: 20 }, (_, i) => ({
      severity: DiagnosticSeverity.Error,
      line: i,
      message: `Error ${i}`,
    }));
    setEditor({ diagnostics: diags });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("fix all");
    // Only first 10 errors should appear
    assert.ok(result.text.includes("Error 0"));
    assert.ok(!result.text.includes("Error 10"), "more than 10 diagnostics included");
  });

  it("includes user request verbatim", async () => {
    setEditor();
    const cm = new ContextManager();
    const prompt = "Please generate a RAII wrapper for FILE*";
    const result = await cm.buildPrompt(prompt);
    assert.ok(result.text.includes(prompt), "user request not in output");
  });

  it("returns correct activeLanguage from editor", async () => {
    setEditor({ languageId: "go" });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("test");
    assert.equal(result.activeLanguage, "go");
  });

  it("truncates context when token budget is very small", async () => {
    setEditor({
      languageId: "typescript",
      selectedText: "x".repeat(4000),
      diagnostics: [
        { severity: DiagnosticSeverity.Error, line: 1, message: "Big issue" },
      ],
    });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("short request", 64);
    assert.ok(result.text.includes("...[truncated]"));
  });

  it("keeps selected code with higher priority than file metadata when budget is tight", async () => {
    setEditor({
      fileName: "/very/long/path/" + "a".repeat(400) + "/file.ts",
      languageId: "typescript",
      selectedText: "const important = 42;",
      diagnostics: [],
    });
    const cm = new ContextManager();
    const result = await cm.buildPrompt("analyze", 96);
    assert.ok(result.text.includes("[Selected code]"), "expected selected code section");

    const selectedIndex = result.text.indexOf("[Selected code]");
    const fileIndex = result.text.indexOf("File      :");
    if (fileIndex !== -1) {
      assert.ok(selectedIndex < fileIndex, "selected section should appear before file metadata");
    }
  });
});
