/**
 * VS Code Extension Entry Point — Copilot→Ollama Delegation Bridge
 *
 * Registers:
 *   1. A chat participant `@ollama` that intercepts Copilot chat requests
 *      and routes them to a local Ollama model or back to Copilot.
 *   2. Three palette commands for manual routing control.
 *   3. A health-check command that verifies the Ollama endpoint.
 */

import * as vscode from "vscode";
import { DelegationRouter, type RoutingPolicyConfig } from "./router.js";
import { OllamaClient } from "./ollamaClient.js";
import { ContextManager } from "./contextManager.js";
import { CopilotReviewer } from "./copilotReviewer.js";
import { ModelSetupManager } from "./modelSetup.js";
import { SettingsPanel } from "./settingsPanel.js";

// ---------------------------------------------------------------------------
// Helper: read config
// ---------------------------------------------------------------------------

interface BridgeConfig {
  endpoint: string;
  defaultModel: string;
  reasoningModel: string;
  delegationMode: "auto" | "always" | "never";
  copilotReviewEnabled: boolean;
  requestTimeoutMs: number;
  themisDbRules: boolean;
  routeBoilerplateToLocal: boolean;
  routeTestsToLocal: boolean;
  routeRefactorsToLocal: boolean;
  routeDocsToLocal: boolean;
  routeCmakeToLocal: boolean;
}

function readRoutingPolicies(cfg: vscode.WorkspaceConfiguration): RoutingPolicyConfig {
  return {
    routeBoilerplateToLocal: cfg.get<boolean>("routeBoilerplateToLocal", true),
    routeTestsToLocal: cfg.get<boolean>("routeTestsToLocal", true),
    routeRefactorsToLocal: cfg.get<boolean>("routeRefactorsToLocal", true),
    routeDocsToLocal: cfg.get<boolean>("routeDocsToLocal", true),
    routeCmakeToLocal: cfg.get<boolean>("routeCmakeToLocal", true),
  };
}

function readConfig(): BridgeConfig {
  const cfg = vscode.workspace.getConfiguration("copilotOllamaRouter");
  return {
    endpoint: cfg.get<string>("endpoint", "http://localhost:11434"),
    defaultModel: cfg.get<string>("defaultModel", "codellama:13b"),
    reasoningModel: cfg.get<string>("reasoningModel", "llama3"),
    delegationMode: cfg.get<"auto" | "always" | "never">(
      "delegationMode",
      "auto"
    ),
    copilotReviewEnabled: cfg.get<boolean>("copilotReviewEnabled", true),
    requestTimeoutMs: cfg.get<number>("requestTimeoutMs", 60_000),
    themisDbRules: cfg.get<boolean>("themisDbRules", true),
    ...readRoutingPolicies(cfg),
  };
}

// ---------------------------------------------------------------------------
// Activate
// ---------------------------------------------------------------------------

export function activate(context: vscode.ExtensionContext): void {
  const router = new DelegationRouter();
  const contextManager = new ContextManager();
  const reviewer = new CopilotReviewer();

  // ── Chat participant ──────────────────────────────────────────────────────

  const participant = vscode.chat.createChatParticipant(
    "copilot-ollama-router.delegate",
    async (
      request: vscode.ChatRequest,
      _chatContext: vscode.ChatContext,
      stream: vscode.ChatResponseStream,
      token: vscode.CancellationToken
    ) => {
      const config = readConfig();
      const ollamaClient = new OllamaClient(config.endpoint);

      // Build enriched prompt with editor context
      const { text: enrichedPrompt, activeLanguage } =
        contextManager.buildPrompt(request.prompt);

      // Determine effective delegation mode from slash command override
      let effectiveMode = config.delegationMode;
      if (request.command === "local") {
        effectiveMode = "always";
      } else if (request.command === "cloud") {
        effectiveMode = "never";
      }

      // Classify and route
      const decision = router.classify(
        request.prompt,
        activeLanguage,
        config.themisDbRules,
        effectiveMode,
        config.defaultModel,
        config.reasoningModel,
        config
      );

      if (decision.destination === "ollama") {
        await handleOllamaRequest(
          enrichedPrompt,
          request.prompt,
          decision.suggestedModel ?? config.defaultModel,
          config,
          ollamaClient,
          reviewer,
          stream,
          token,
          decision.reason
        );
      } else {
        // Let the user know we are redirecting to Copilot
        stream.markdown(
          `> 🔀 **Routed to Copilot (cloud)** — *${decision.reason}*\n\n`
        );
        // Re-submit the prompt through the Language Model API so the answer
        // arrives in the same chat thread.
        await handleCopilotFallback(request.prompt, stream, token);
      }
    }
  );

  participant.iconPath = vscode.Uri.joinPath(
    context.extensionUri,
    "media",
    "ollama-icon.png"
  );

  context.subscriptions.push(participant);

  // ── Commands ──────────────────────────────────────────────────────────────

  context.subscriptions.push(
    vscode.commands.registerCommand(
      "copilotOllamaRouter.delegateToOllama",
      async () => {
        const prompt = await vscode.window.showInputBox({
          prompt: "Enter your request (will be sent to local Ollama)",
          placeHolder: "e.g. Write unit tests for the selected function",
        });
        if (!prompt) {
          return;
        }
        const config = readConfig();
        const ollamaClient = new OllamaClient(config.endpoint);
        const { text: enrichedPrompt } = contextManager.buildPrompt(prompt);

        await vscode.window.withProgress(
          {
            location: vscode.ProgressLocation.Notification,
            title: "Ollama Bridge",
            cancellable: true,
          },
          async (progress, cancelToken) => {
            progress.report({ message: `Querying ${config.defaultModel}…` });
            const abortController = new AbortController();
            cancelToken.onCancellationRequested(() =>
              abortController.abort()
            );

            let result = "";
            try {
              result = await ollamaClient.generate({
                model: config.defaultModel,
                prompt: enrichedPrompt,
                onToken: () => undefined,
                signal: abortController.signal,
                timeoutMs: config.requestTimeoutMs,
              });
            } catch (err) {
              void vscode.window.showErrorMessage(
                `Ollama error: ${err instanceof Error ? err.message : String(err)}`
              );
              return;
            }

            const doc = await vscode.workspace.openTextDocument({
              content: result,
              language: "markdown",
            });
            await vscode.window.showTextDocument(doc);
          }
        );
      }
    )
  );

  context.subscriptions.push(
    vscode.commands.registerCommand("copilotOllamaRouter.askCopilot", async () => {
      const prompt = await vscode.window.showInputBox({
        prompt: "Enter your request (will be pre-tagged for Copilot chat based on routing policy)",
        placeHolder: "e.g. Generate boilerplate for a C++ RAII wrapper",
      });
      if (!prompt) {
        return;
      }

      const config = readConfig();
      const { activeLanguage } = contextManager.buildPrompt(prompt);
      const decision = router.classify(
        prompt,
        activeLanguage,
        config.themisDbRules,
        config.delegationMode,
        config.defaultModel,
        config.reasoningModel,
        config
      );

      const query =
        decision.destination === "ollama"
          ? `@ollama /local ${prompt}`
          : `@workspace ${prompt}`;

      await vscode.commands.executeCommand("workbench.action.chat.open", {
        query,
      });

      void vscode.window.showInformationMessage(
        decision.destination === "ollama"
          ? `Copilot chat opened with local Ollama tag. Reason: ${decision.reason}`
          : `Copilot chat opened without Ollama tag. Reason: ${decision.reason}`
      );
    })
  );

  context.subscriptions.push(
    vscode.commands.registerCommand("copilotOllamaRouter.autoRoute", async () => {
      const prompt = await vscode.window.showInputBox({
        prompt: "Enter your request (will be auto-classified)",
        placeHolder: "e.g. Refactor this function to use RAII",
      });
      if (!prompt) {
        return;
      }
      const config = readConfig();
      const { activeLanguage } = contextManager.buildPrompt(prompt);
      const decision = router.classify(
        prompt,
        activeLanguage,
        config.themisDbRules,
        config.delegationMode,
        config.defaultModel,
        config.reasoningModel,
        config
      );

      const label =
        decision.destination === "ollama"
          ? `🖥️  Local Ollama (${decision.suggestedModel ?? config.defaultModel})`
          : "☁️  Copilot (cloud)";

      void vscode.window.showInformationMessage(
        `Auto-route decision: ${label}\nReason: ${decision.reason}`
      );
    })
  );

  context.subscriptions.push(
    vscode.commands.registerCommand(
      "copilotOllamaRouter.checkOllamaHealth",
      async () => {
        const config = readConfig();
        const ollamaClient = new OllamaClient(config.endpoint);
        const result = await ollamaClient.health();

        if (result.ok) {
          const modelList =
            result.models.length > 0
              ? result.models.join(", ")
              : "(none pulled yet)";
          void vscode.window.showInformationMessage(
            `✅ Ollama reachable at ${config.endpoint}.\nModels: ${modelList}`
          );
        } else {
          void vscode.window.showErrorMessage(
            `❌ Cannot reach Ollama at ${config.endpoint}: ${result.error ?? "unknown error"}`
          );
        }
      }
    )
  );

  context.subscriptions.push(
    vscode.commands.registerCommand(
      "copilotOllamaRouter.setupModels",
      async () => {
        const config = readConfig();
        const setupManager = new ModelSetupManager(config.endpoint);

        // 1. Generate / update workspace config files (idempotent)
        const cfgResult = await setupManager.generateWorkspaceConfig();
        if (cfgResult.modifiedFiles.length > 0) {
          void vscode.window.showInformationMessage(
            `Ollama Bridge: workspace config updated (${cfgResult.modifiedFiles.length} file(s)).`
          );
        }

        // 2. Interactive model download wizard
        await setupManager.pickAndPullModels();
      }
    )
  );

  context.subscriptions.push(
    vscode.commands.registerCommand(
      "copilotOllamaRouter.listInstalledModels",
      async () => {
        const config = readConfig();
        const setupManager = new ModelSetupManager(config.endpoint);
        const installed = await setupManager.queryInstalledModels();

        if (installed.length === 0) {
          const action = await vscode.window.showWarningMessage(
            "No Ollama models installed yet.",
            "Set up models…"
          );
          if (action === "Set up models…") {
            await vscode.commands.executeCommand("copilotOllamaRouter.setupModels");
          }
          return;
        }

        void vscode.window.showInformationMessage(
          `Installed Ollama models (${installed.length}): ${installed.join(", ")}`
        );
      }
    )
  );

  context.subscriptions.push(
    vscode.commands.registerCommand(
      "copilotOllamaRouter.openSettings",
      () => {
        SettingsPanel.show(context);
      }
    )
  );

  context.subscriptions.push(
    vscode.commands.registerCommand(
      "copilotOllamaRouter.applyWorkspaceConfig",
      async () => {
        const config = readConfig();
        const setupManager = new ModelSetupManager(config.endpoint);
        const result = await setupManager.applyWorkspaceConfigWithConfirmation();

        if (result.modifiedFiles.length > 0) {
          void vscode.window.showInformationMessage(
            `Workspace config updated (${result.modifiedFiles.length} file(s)):\n${result.modifiedFiles.join("\n")}`
          );
        } else if (result.skipped.length > 0) {
          void vscode.window.showWarningMessage(
            `Workspace config: skipped — ${result.skipped.join("; ")}`
          );
        } else {
          void vscode.window.showInformationMessage(
            "Workspace config is already up to date."
          );
        }
      }
    )
  );
}

export function deactivate(): void {
  // No persistent resources to clean up.
}

// ---------------------------------------------------------------------------
// Internal handlers
// ---------------------------------------------------------------------------

async function handleOllamaRequest(
  enrichedPrompt: string,
  rawPrompt: string,
  model: string,
  config: BridgeConfig,
  ollamaClient: OllamaClient,
  reviewer: CopilotReviewer,
  stream: vscode.ChatResponseStream,
  token: vscode.CancellationToken,
  routingReason: string
): Promise<void> {
  stream.markdown(
    `> 🖥️  **Routed to local Ollama** (\`${model}\`) — *${routingReason}*\n\n`
  );

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
  } catch (err) {
    const msg = err instanceof Error ? err.message : String(err);
    stream.markdown(`\n\n⚠️ **Ollama error**: ${msg}`);
    return;
  }

  if (config.copilotReviewEnabled && ollamaOutput.trim().length > 0) {
    stream.markdown("\n\n---\n### 🔍 Copilot Quality Review\n");
    await reviewer.review(
      ollamaOutput,
      rawPrompt,
      (fragment) => stream.markdown(fragment),
      token
    );
  }
}

async function handleCopilotFallback(
  prompt: string,
  stream: vscode.ChatResponseStream,
  token: vscode.CancellationToken
): Promise<void> {
  const models = await vscode.lm.selectChatModels({
    vendor: "copilot",
    family: "gpt-4o",
  });

  if (models.length === 0) {
    stream.markdown(
      "⚠️ No Copilot model available. Please install GitHub Copilot and sign in."
    );
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
  } catch (err) {
    const msg =
      err instanceof vscode.LanguageModelError
        ? err.message
        : String(err);
    stream.markdown(`\n\n⚠️ **Copilot error**: ${msg}`);
  }
}
