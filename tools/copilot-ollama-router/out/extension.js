"use strict";
/**
 * VS Code Extension Entry Point — Copilot→Ollama Delegation Bridge
 *
 * Registers:
 *   1. A chat participant `@ollama` that intercepts Copilot chat requests
 *      and routes them to a local Ollama model or back to Copilot.
 *   2. Three palette commands for manual routing control.
 *   3. A health-check command that verifies the Ollama endpoint.
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
exports.activate = activate;
exports.deactivate = deactivate;
const vscode = __importStar(require("vscode"));
const router_js_1 = require("./router.js");
const ollamaClient_js_1 = require("./ollamaClient.js");
const contextManager_js_1 = require("./contextManager.js");
const copilotReviewer_js_1 = require("./copilotReviewer.js");
const modelSetup_js_1 = require("./modelSetup.js");
function readConfig() {
    const cfg = vscode.workspace.getConfiguration("copilotOllamaRouter");
    return {
        endpoint: cfg.get("endpoint", "http://localhost:11434"),
        defaultModel: cfg.get("defaultModel", "codellama:13b"),
        reasoningModel: cfg.get("reasoningModel", "llama3"),
        delegationMode: cfg.get("delegationMode", "auto"),
        copilotReviewEnabled: cfg.get("copilotReviewEnabled", true),
        requestTimeoutMs: cfg.get("requestTimeoutMs", 60_000),
        themisDbRules: cfg.get("themisDbRules", true),
    };
}
// ---------------------------------------------------------------------------
// Activate
// ---------------------------------------------------------------------------
function activate(context) {
    const router = new router_js_1.DelegationRouter();
    const contextManager = new contextManager_js_1.ContextManager();
    const reviewer = new copilotReviewer_js_1.CopilotReviewer();
    // ── Chat participant ──────────────────────────────────────────────────────
    const participant = vscode.chat.createChatParticipant("copilot-ollama-router.delegate", async (request, _chatContext, stream, token) => {
        const config = readConfig();
        const ollamaClient = new ollamaClient_js_1.OllamaClient(config.endpoint);
        // Build enriched prompt with editor context
        const { text: enrichedPrompt, activeLanguage } = contextManager.buildPrompt(request.prompt);
        // Determine effective delegation mode from slash command override
        let effectiveMode = config.delegationMode;
        if (request.command === "local") {
            effectiveMode = "always";
        }
        else if (request.command === "cloud") {
            effectiveMode = "never";
        }
        // Classify and route
        const decision = router.classify(request.prompt, activeLanguage, config.themisDbRules, effectiveMode, config.defaultModel, config.reasoningModel);
        if (decision.destination === "ollama") {
            await handleOllamaRequest(enrichedPrompt, request.prompt, decision.suggestedModel ?? config.defaultModel, config, ollamaClient, reviewer, stream, token, decision.reason);
        }
        else {
            // Let the user know we are redirecting to Copilot
            stream.markdown(`> 🔀 **Routed to Copilot (cloud)** — *${decision.reason}*\n\n`);
            // Re-submit the prompt through the Language Model API so the answer
            // arrives in the same chat thread.
            await handleCopilotFallback(request.prompt, stream, token);
        }
    });
    participant.iconPath = vscode.Uri.joinPath(context.extensionUri, "media", "ollama-icon.png");
    context.subscriptions.push(participant);
    // ── Commands ──────────────────────────────────────────────────────────────
    context.subscriptions.push(vscode.commands.registerCommand("copilotOllamaRouter.delegateToOllama", async () => {
        const prompt = await vscode.window.showInputBox({
            prompt: "Enter your request (will be sent to local Ollama)",
            placeHolder: "e.g. Write unit tests for the selected function",
        });
        if (!prompt) {
            return;
        }
        const config = readConfig();
        const ollamaClient = new ollamaClient_js_1.OllamaClient(config.endpoint);
        const { text: enrichedPrompt } = contextManager.buildPrompt(prompt);
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: "Ollama Bridge",
            cancellable: true,
        }, async (progress, cancelToken) => {
            progress.report({ message: `Querying ${config.defaultModel}…` });
            const abortController = new AbortController();
            cancelToken.onCancellationRequested(() => abortController.abort());
            let result = "";
            try {
                result = await ollamaClient.generate({
                    model: config.defaultModel,
                    prompt: enrichedPrompt,
                    onToken: () => undefined,
                    signal: abortController.signal,
                    timeoutMs: config.requestTimeoutMs,
                });
            }
            catch (err) {
                void vscode.window.showErrorMessage(`Ollama error: ${err instanceof Error ? err.message : String(err)}`);
                return;
            }
            const doc = await vscode.workspace.openTextDocument({
                content: result,
                language: "markdown",
            });
            await vscode.window.showTextDocument(doc);
        });
    }));
    context.subscriptions.push(vscode.commands.registerCommand("copilotOllamaRouter.askCopilot", async () => {
        // Open a new chat panel and pre-fill the prompt — the simplest UX
        await vscode.commands.executeCommand("workbench.action.chat.open", { query: "@workspace " });
    }));
    context.subscriptions.push(vscode.commands.registerCommand("copilotOllamaRouter.autoRoute", async () => {
        const prompt = await vscode.window.showInputBox({
            prompt: "Enter your request (will be auto-classified)",
            placeHolder: "e.g. Refactor this function to use RAII",
        });
        if (!prompt) {
            return;
        }
        const config = readConfig();
        const { activeLanguage } = contextManager.buildPrompt(prompt);
        const decision = router.classify(prompt, activeLanguage, config.themisDbRules, config.delegationMode, config.defaultModel, config.reasoningModel);
        const label = decision.destination === "ollama"
            ? `🖥️  Local Ollama (${decision.suggestedModel ?? config.defaultModel})`
            : "☁️  Copilot (cloud)";
        void vscode.window.showInformationMessage(`Auto-route decision: ${label}\nReason: ${decision.reason}`);
    }));
    context.subscriptions.push(vscode.commands.registerCommand("copilotOllamaRouter.checkOllamaHealth", async () => {
        const config = readConfig();
        const ollamaClient = new ollamaClient_js_1.OllamaClient(config.endpoint);
        const result = await ollamaClient.health();
        if (result.ok) {
            const modelList = result.models.length > 0
                ? result.models.join(", ")
                : "(none pulled yet)";
            void vscode.window.showInformationMessage(`✅ Ollama reachable at ${config.endpoint}.\nModels: ${modelList}`);
        }
        else {
            void vscode.window.showErrorMessage(`❌ Cannot reach Ollama at ${config.endpoint}: ${result.error ?? "unknown error"}`);
        }
    }));
    context.subscriptions.push(vscode.commands.registerCommand("copilotOllamaRouter.setupModels", async () => {
        const config = readConfig();
        const setupManager = new modelSetup_js_1.ModelSetupManager(config.endpoint);
        // 1. Generate / update workspace config files (idempotent)
        const cfgResult = await setupManager.generateWorkspaceConfig();
        if (cfgResult.modifiedFiles.length > 0) {
            void vscode.window.showInformationMessage(`Ollama Bridge: workspace config updated (${cfgResult.modifiedFiles.length} file(s)).`);
        }
        // 2. Interactive model download wizard
        await setupManager.pickAndPullModels();
    }));
    context.subscriptions.push(vscode.commands.registerCommand("copilotOllamaRouter.listInstalledModels", async () => {
        const config = readConfig();
        const setupManager = new modelSetup_js_1.ModelSetupManager(config.endpoint);
        const installed = await setupManager.queryInstalledModels();
        if (installed.length === 0) {
            const action = await vscode.window.showWarningMessage("No Ollama models installed yet.", "Set up models…");
            if (action === "Set up models…") {
                await vscode.commands.executeCommand("copilotOllamaRouter.setupModels");
            }
            return;
        }
        void vscode.window.showInformationMessage(`Installed Ollama models (${installed.length}): ${installed.join(", ")}`);
    }));
}
function deactivate() {
    // No persistent resources to clean up.
}
// ---------------------------------------------------------------------------
// Internal handlers
// ---------------------------------------------------------------------------
async function handleOllamaRequest(enrichedPrompt, rawPrompt, model, config, ollamaClient, reviewer, stream, token, routingReason) {
    stream.markdown(`> 🖥️  **Routed to local Ollama** (\`${model}\`) — *${routingReason}*\n\n`);
    const abortController = new AbortController();
    token.onCancellationRequested(() => abortController.abort());
    let ollamaOutput = "";
    try {
        ollamaOutput = await ollamaClient.generate({
            model,
            prompt: enrichedPrompt,
            onToken: (t) => stream.markdown(t),
            signal: abortController.signal,
            timeoutMs: config.requestTimeoutMs,
        });
    }
    catch (err) {
        const msg = err instanceof Error ? err.message : String(err);
        stream.markdown(`\n\n⚠️ **Ollama error**: ${msg}`);
        return;
    }
    if (config.copilotReviewEnabled && ollamaOutput.trim().length > 0) {
        stream.markdown("\n\n---\n### 🔍 Copilot Quality Review\n");
        await reviewer.review(ollamaOutput, rawPrompt, (fragment) => stream.markdown(fragment), token);
    }
}
async function handleCopilotFallback(prompt, stream, token) {
    const models = await vscode.lm.selectChatModels({
        vendor: "copilot",
        family: "gpt-4o",
    });
    if (models.length === 0) {
        stream.markdown("⚠️ No Copilot model available. Please install GitHub Copilot and sign in.");
        return;
    }
    const model = models[0];
    const messages = [vscode.LanguageModelChatMessage.User(prompt)];
    try {
        const response = await model.sendRequest(messages, {}, token);
        for await (const fragment of response.text) {
            if (token.isCancellationRequested) {
                break;
            }
            stream.markdown(fragment);
        }
    }
    catch (err) {
        const msg = err instanceof vscode.LanguageModelError
            ? err.message
            : String(err);
        stream.markdown(`\n\n⚠️ **Copilot error**: ${msg}`);
    }
}
//# sourceMappingURL=extension.js.map