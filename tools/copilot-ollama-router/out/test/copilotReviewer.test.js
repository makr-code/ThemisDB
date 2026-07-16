"use strict";
/**
 * Unit tests for CopilotReviewer.review()
 */
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const strict_1 = __importDefault(require("node:assert/strict"));
// eslint-disable-next-line @typescript-eslint/no-require-imports
const vscodeMock = require("./vscode-mock.js");
// eslint-disable-next-line @typescript-eslint/no-require-imports
const { CopilotReviewer } = require("../copilotReviewer.js");
function makeToken() {
    return {
        isCancellationRequested: false,
        onCancellationRequested: () => ({
            dispose: () => undefined,
        }),
    };
}
describe("CopilotReviewer.review", () => {
    it("returns false when no Copilot model is available", async () => {
        vscodeMock.lm.selectChatModels = async () => [];
        const reviewer = new CopilotReviewer();
        let collected = "";
        const ok = await reviewer.review("const x = 1;", "check this", (fragment) => {
            collected += fragment;
        }, makeToken());
        strict_1.default.equal(ok, false);
        strict_1.default.equal(collected, "");
    });
    it("streams fragments and returns true when Copilot responds", async () => {
        const fakeModel = {
            sendRequest: async () => ({
                text: (async function* () {
                    yield "- issue 1\n";
                    yield "- issue 2\n";
                })(),
            }),
        };
        vscodeMock.lm.selectChatModels = async () => [fakeModel];
        const reviewer = new CopilotReviewer();
        let collected = "";
        const ok = await reviewer.review("int main() {}", "review this", (fragment) => {
            collected += fragment;
        }, makeToken());
        strict_1.default.equal(ok, true);
        strict_1.default.equal(collected, "- issue 1\n- issue 2\n");
    });
    it("returns false and emits unavailable note on LanguageModelError", async () => {
        class MockLanguageModelError extends Error {
        }
        vscodeMock.LanguageModelError = MockLanguageModelError;
        const fakeModel = {
            sendRequest: async () => {
                throw new MockLanguageModelError("no access");
            },
        };
        vscodeMock.lm.selectChatModels = async () => [fakeModel];
        const reviewer = new CopilotReviewer();
        let collected = "";
        const ok = await reviewer.review("int main() {}", "review this", (fragment) => {
            collected += fragment;
        }, makeToken());
        strict_1.default.equal(ok, false);
        strict_1.default.match(collected, /Copilot review unavailable/i);
    });
});
//# sourceMappingURL=copilotReviewer.test.js.map