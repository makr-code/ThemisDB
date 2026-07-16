/**
 * Unit tests for ModelSetupManager workspace-config generation.
 */

import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";

// eslint-disable-next-line @typescript-eslint/no-require-imports
const vscodeMock = require("./vscode-mock.js") as typeof import("./vscode-mock.js") & {
  workspace: {
    workspaceFolders?: Array<{ uri: { fsPath: string } }>;
  };
};
// eslint-disable-next-line @typescript-eslint/no-require-imports
const { ModelSetupManager } = require("../modelSetup.js") as typeof import("../modelSetup.js");

describe("ModelSetupManager.generateWorkspaceConfig", () => {
  let tmpRoot = "";

  beforeEach((): void => {
    tmpRoot = fs.mkdtempSync(path.join(os.tmpdir(), "ollama-router-test-"));
    vscodeMock.workspace.workspaceFolders = [{ uri: { fsPath: tmpRoot } }];
  });

  afterEach((): void => {
    try {
      fs.rmSync(tmpRoot, { recursive: true, force: true });
    } catch {
      // best-effort cleanup
    }
  });

  it("creates expected files on first run and is idempotent on second run", async () => {
    const mgr = new ModelSetupManager("http://localhost:11434");

    const first = await mgr.generateWorkspaceConfig();
    assert.equal(first.modifiedFiles.length, 3);
    assert.equal(first.skipped.length, 0);

    const second = await mgr.generateWorkspaceConfig();
    assert.equal(second.modifiedFiles.length, 0);
    assert.equal(second.skipped.length, 0);
  });

  it("preserves existing user settings while adding missing defaults", async () => {
    const vscodeDir = path.join(tmpRoot, ".vscode");
    fs.mkdirSync(vscodeDir, { recursive: true });

    const settingsPath = path.join(vscodeDir, "settings.json");
    fs.writeFileSync(
      settingsPath,
      JSON.stringify(
        {
          "copilotOllamaRouter.defaultModel": "custom:model",
          "editor.tabSize": 2,
        },
        null,
        2
      ),
      "utf8"
    );

    const mgr = new ModelSetupManager("http://localhost:11434");
    await mgr.generateWorkspaceConfig();

    const merged = JSON.parse(fs.readFileSync(settingsPath, "utf8")) as Record<string, unknown>;
    assert.equal(merged["copilotOllamaRouter.defaultModel"], "custom:model");
    assert.equal(merged["editor.tabSize"], 2);
    assert.equal(merged["copilotOllamaRouter.delegationMode"], "auto");
    assert.equal(merged["copilotOllamaRouter.contextTokenBudget"], 2048);
    assert.ok(Array.isArray(merged["github.copilot.chat.codeGeneration.instructions"]));
  });
});
