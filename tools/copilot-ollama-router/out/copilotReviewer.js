"use strict";
/**
 * Copilot Reviewer
 *
 * Optional quality-check pass: after Ollama returns its response, this
 * module sends a short review request to Copilot via the VS Code Language
 * Model API (`vscode.lm`) and streams any suggestions back.
 *
 * The review prompt deliberately asks for *concise* feedback so the UX
 * remains responsive.  It does NOT re-generate the code; it only annotates
 * potential issues.
 */
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.CopilotReviewer = void 0;
const vscode = __importStar(require("vscode"));
const REVIEW_SYSTEM_PROMPT = `You are a senior code reviewer. \
The user will provide an AI-generated code snippet. \
Your job is to briefly identify any bugs, security issues, \
or major quality problems — in at most five bullet points. \
If the code looks correct, say "LGTM" and stop. \
Do NOT rewrite the code; only describe the issues.`;
class CopilotReviewer {
    /**
     * Sends `ollamaOutput` to Copilot for a brief quality review.
     * Streams each fragment back via `onFragment`.
     *
     * Returns `false` if the Language Model API is unavailable or if the
     * request is cancelled.
     */
    async review(ollamaOutput, userPrompt, onFragment, token) {
        // Requires VS Code ≥ 1.90 and GitHub Copilot installed
        const models = await vscode.lm.selectChatModels({
            vendor: "copilot",
            family: "gpt-4o",
        });
        if (models.length === 0) {
            // Silently skip if no Copilot model is available
            return false;
        }
        const model = models[0];
        const messages = [
            vscode.LanguageModelChatMessage.Assistant(REVIEW_SYSTEM_PROMPT),
            vscode.LanguageModelChatMessage.User(`Original request: ${userPrompt}\n\nGenerated code:\n\`\`\`\n${ollamaOutput}\n\`\`\``),
        ];
        try {
            const response = await model.sendRequest(messages, {}, token);
            for await (const fragment of response.text) {
                if (token.isCancellationRequested) {
                    break;
                }
                onFragment(fragment);
            }
            return true;
        }
        catch (err) {
            if (err instanceof vscode.LanguageModelError) {
                // Token limit, content policy, or no access — treat as unavailable
                onFragment(`\n\n*(Copilot review unavailable: ${err.message})*`);
            }
            return false;
        }
    }
}
exports.CopilotReviewer = CopilotReviewer;
//# sourceMappingURL=copilotReviewer.js.map