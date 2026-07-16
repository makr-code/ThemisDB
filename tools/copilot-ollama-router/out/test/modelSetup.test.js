"use strict";
/**
 * Unit tests for ModelSetupManager workspace-config generation.
 */
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const strict_1 = __importDefault(require("node:assert/strict"));
const node_fs_1 = __importDefault(require("node:fs"));
const node_os_1 = __importDefault(require("node:os"));
const node_path_1 = __importDefault(require("node:path"));
// eslint-disable-next-line @typescript-eslint/no-require-imports
const vscodeMock = require("./vscode-mock.js");
// eslint-disable-next-line @typescript-eslint/no-require-imports
const { ModelSetupManager } = require("../modelSetup.js");
describe("ModelSetupManager.generateWorkspaceConfig", () => {
    let tmpRoot = "";
    beforeEach(() => {
        tmpRoot = node_fs_1.default.mkdtempSync(node_path_1.default.join(node_os_1.default.tmpdir(), "ollama-router-test-"));
        vscodeMock.workspace.workspaceFolders = [{ uri: { fsPath: tmpRoot } }];
    });
    afterEach(() => {
        try {
            node_fs_1.default.rmSync(tmpRoot, { recursive: true, force: true });
        }
        catch {
            // best-effort cleanup
        }
    });
    it("creates expected files on first run and is idempotent on second run", async () => {
        const mgr = new ModelSetupManager("http://localhost:11434");
        const first = await mgr.generateWorkspaceConfig();
        strict_1.default.equal(first.modifiedFiles.length, 3);
        strict_1.default.equal(first.skipped.length, 0);
        const second = await mgr.generateWorkspaceConfig();
        strict_1.default.equal(second.modifiedFiles.length, 0);
        strict_1.default.equal(second.skipped.length, 0);
    });
    it("preserves existing user settings while adding missing defaults", async () => {
        const vscodeDir = node_path_1.default.join(tmpRoot, ".vscode");
        node_fs_1.default.mkdirSync(vscodeDir, { recursive: true });
        const settingsPath = node_path_1.default.join(vscodeDir, "settings.json");
        node_fs_1.default.writeFileSync(settingsPath, JSON.stringify({
            "copilotOllamaRouter.defaultModel": "custom:model",
            "editor.tabSize": 2,
        }, null, 2), "utf8");
        const mgr = new ModelSetupManager("http://localhost:11434");
        await mgr.generateWorkspaceConfig();
        const merged = JSON.parse(node_fs_1.default.readFileSync(settingsPath, "utf8"));
        strict_1.default.equal(merged["copilotOllamaRouter.defaultModel"], "custom:model");
        strict_1.default.equal(merged["editor.tabSize"], 2);
        strict_1.default.equal(merged["copilotOllamaRouter.delegationMode"], "auto");
        strict_1.default.equal(merged["copilotOllamaRouter.contextTokenBudget"], 2048);
        strict_1.default.ok(Array.isArray(merged["github.copilot.chat.codeGeneration.instructions"]));
    });
});
//# sourceMappingURL=modelSetup.test.js.map