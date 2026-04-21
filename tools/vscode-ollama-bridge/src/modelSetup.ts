/**
 * Model Setup Wizard
 *
 * Provides:
 *  - A ranked catalog of the best coding-focused Ollama models (based on
 *    HumanEval / MultiPL-E / BigCode benchmark results, April 2026).
 *  - `queryInstalledModels()` — lists models already pulled locally.
 *  - `pickAndPullModels()` — interactive VS Code QuickPick that lets the
 *    developer select models to download; shows live streaming progress.
 *  - `ensureRecommendedModel()` — silent bootstrap: pulls the top-ranked
 *    model if nothing is installed yet.
 */

import * as vscode from "vscode";
import * as https from "https";
import * as http from "http";
import * as fs from "fs";
import * as path from "path";
import { URL } from "url";

// ---------------------------------------------------------------------------
// Ranked coding-model catalog
// ---------------------------------------------------------------------------

export interface ModelEntry {
  /** Ollama model tag (as used in `ollama pull <tag>`). */
  tag: string;
  /** Human-readable display name. */
  label: string;
  /** One-line description of strengths. */
  description: string;
  /** Approximate VRAM requirement (GB). */
  vramGb: number;
  /** HumanEval pass@1 score (%, approximate). */
  humanEvalScore: number;
  /** Primary task focus. */
  focus: "code" | "code+reasoning" | "general";
}

/**
 * Ranked list: highest `humanEvalScore` first.
 * Sources: HumanEval leaderboard, BigCode Leaderboard (2025-Q4 / 2026-Q1).
 */
export const RANKED_CODING_MODELS: ReadonlyArray<ModelEntry> = [
  {
    tag: "deepseek-coder-v2:16b",
    label: "DeepSeek-Coder-V2 16B",
    description: "State-of-art open coding model, MoE architecture (Apr 2026 leader)",
    vramGb: 10,
    humanEvalScore: 90,
    focus: "code",
  },
  {
    tag: "qwen2.5-coder:14b",
    label: "Qwen2.5-Coder 14B",
    description: "Alibaba's 14B coding model — top MultiPL-E scores",
    vramGb: 9,
    humanEvalScore: 88,
    focus: "code",
  },
  {
    tag: "qwen2.5-coder:7b",
    label: "Qwen2.5-Coder 7B",
    description: "Best-in-class 7B for code — fits 6 GB VRAM",
    vramGb: 5,
    humanEvalScore: 84,
    focus: "code",
  },
  {
    tag: "codellama:34b",
    label: "CodeLlama 34B",
    description: "Meta's largest CodeLlama — excellent C++/Python",
    vramGb: 20,
    humanEvalScore: 74,
    focus: "code",
  },
  {
    tag: "codellama:13b",
    label: "CodeLlama 13B",
    description: "Solid default: C++, Python, TypeScript",
    vramGb: 8,
    humanEvalScore: 67,
    focus: "code",
  },
  {
    tag: "starcoder2:15b",
    label: "StarCoder2 15B",
    description: "BigCode model, strong Fill-in-Middle support",
    vramGb: 10,
    humanEvalScore: 72,
    focus: "code",
  },
  {
    tag: "starcoder2:7b",
    label: "StarCoder2 7B",
    description: "Lighter StarCoder2 variant",
    vramGb: 5,
    humanEvalScore: 62,
    focus: "code",
  },
  {
    tag: "codellama:7b",
    label: "CodeLlama 7B",
    description: "Fast, low-VRAM code model",
    vramGb: 4,
    humanEvalScore: 54,
    focus: "code",
  },
  {
    tag: "deepseek-coder:6.7b",
    label: "DeepSeek-Coder 6.7B",
    description: "Efficient 6.7B code specialist",
    vramGb: 4,
    humanEvalScore: 65,
    focus: "code",
  },
  {
    tag: "llama3.1:8b",
    label: "Llama 3.1 8B",
    description: "Meta's instruction model — good for reasoning + docs",
    vramGb: 5,
    humanEvalScore: 58,
    focus: "code+reasoning",
  },
  {
    tag: "phi3.5:mini",
    label: "Phi-3.5 Mini",
    description: "Microsoft's compact model — fast on CPU/low-VRAM",
    vramGb: 3,
    humanEvalScore: 55,
    focus: "code+reasoning",
  },
  {
    tag: "mistral:7b",
    label: "Mistral 7B",
    description: "General-purpose — good for docs and refactoring",
    vramGb: 4,
    humanEvalScore: 43,
    focus: "general",
  },
];

// ---------------------------------------------------------------------------
// API helpers
// ---------------------------------------------------------------------------

interface OllamaTagsResponse {
  models?: Array<{ name: string; size?: number }>;
}

interface OllamaPullProgress {
  status?: string;
  completed?: number;
  total?: number;
  error?: string;
}

// ---------------------------------------------------------------------------
// Workspace config constants
// ---------------------------------------------------------------------------

const BRIDGE_SECTION_BEGIN = "<!-- OLLAMA-BRIDGE-SECTION:BEGIN -->";
const BRIDGE_SECTION_END   = "<!-- OLLAMA-BRIDGE-SECTION:END -->";

/**
 * Content injected into .github/copilot-instructions.md between the sentinel
 * markers. Replaces any existing bridge section on re-run.
 */
const COPILOT_INSTRUCTIONS_BRIDGE_SECTION = `${BRIDGE_SECTION_BEGIN}

## 9) Ollama Delegation Bridge — AI Agent Instructions

The repository ships a VS Code extension at \`tools/vscode-ollama-bridge/\` that
routes code-generation tasks to a **local Ollama model** instead of Copilot
cloud. Every AI agent (Copilot, GitHub Copilot in VS Code, this coding agent)
MUST follow these rules when working on ThemisDB:

### 9.1 When to use local Ollama (via \`@ollama\` chat participant)

Use \`@ollama\` for all of the following:

| Task | Example prompt |
|---|---|
| C++ boilerplate / class scaffolding | \`@ollama Generate a RAII wrapper for FILE*\` |
| Unit test generation | \`@ollama Write GTest cases for KnowledgeGraphRetriever::neighbours()\` |
| Refactoring | \`@ollama Refactor this function to use std::expected\` |
| Documentation / Doxygen comments | \`@ollama Add Doxygen docs to LLMPluginManager::loadModel()\` |
| CMakeLists.txt edits | \`@ollama Add a new test target for test_foo.cpp\` |
| Repetitive code patterns | \`@ollama Implement getters/setters for all fields in struct X\` |

### 9.2 When to use Copilot cloud (standard chat)

Use the normal Copilot chat (no \`@ollama\`) for:

- Security review and vulnerability analysis
- Architecture and design decisions
- Complex multi-file debugging
- Code review with quality judgement
- Any task requiring up-to-date knowledge (CVEs, new APIs)

### 9.3 ThemisDB-specific routing rules (always enforced)

- **C++ files** (\`.cpp\`, \`.hpp\`, \`.h\`, \`.cc\`) → always \`@ollama /local\`
- **Security / audit** prompts → always standard Copilot (cloud)
- **CMakeLists.txt / build system** → \`@ollama\`
- **ROADMAP.md / FUTURE_ENHANCEMENTS.md updates** → standard Copilot

### 9.4 Delegation mode configuration

Set in VS Code \`settings.json\` (workspace or user):

\`\`\`jsonc
{
  "ollamaBridge.delegationMode": "auto",
  "ollamaBridge.defaultModel": "deepseek-coder-v2:16b",
  "ollamaBridge.reasoningModel": "qwen2.5-coder:14b",
  "ollamaBridge.copilotReviewEnabled": true,
  "ollamaBridge.themisDbRules": true
}
\`\`\`

${BRIDGE_SECTION_END}`;

// ---------------------------------------------------------------------------
// Workspace config generator
// ---------------------------------------------------------------------------

/**
 * Result returned by generateWorkspaceConfig() describing what was done.
 */
export interface WorkspaceConfigResult {
  /** Path of each file that was created or modified. */
  modifiedFiles: string[];
  /** Skipped files with reason (e.g. "no workspace folder"). */
  skipped: string[];
}

export class ModelSetupManager {
  constructor(private readonly endpoint: string) {}

  // ---------------------------------------------------------------------------
  // Workspace config
  // ---------------------------------------------------------------------------

  /**
   * Idempotently writes/updates the three workspace config files consumed by
   * VS Code and GitHub Copilot:
   *
   *   - `.vscode/settings.json`         — adds missing `ollamaBridge.*` keys
   *                                        and `github.copilot.chat.*.instructions`
   *                                        file references; existing user values
   *                                        are preserved.
   *   - `.vscode/extensions.json`        — adds `themisdb.vscode-ollama-bridge`
   *                                        to `recommendations` if not present.
   *   - `.github/copilot-instructions.md` — inserts/replaces only the sentinel-
   *                                        delimited bridge section; all other
   *                                        content is left untouched.
   *
   * Safe to call multiple times.
   */
  async generateWorkspaceConfig(): Promise<WorkspaceConfigResult> {
    const result: WorkspaceConfigResult = { modifiedFiles: [], skipped: [] };

    const folders = vscode.workspace.workspaceFolders;
    if (!folders || folders.length === 0) {
      result.skipped.push("No workspace folder open.");
      return result;
    }

    const root = folders[0].uri.fsPath;
    const vscodeDirPath = path.join(root, ".vscode");
    const githubDirPath = path.join(root, ".github");

    // Ensure directories exist
    fs.mkdirSync(vscodeDirPath, { recursive: true });
    fs.mkdirSync(githubDirPath, { recursive: true });

    await Promise.all([
      this.mergeVscodeSettings(vscodeDirPath, result),
      this.mergeExtensionsJson(vscodeDirPath, result),
      this.mergeCopilotInstructions(githubDirPath, result),
    ]);

    return result;
  }

  // ---------------------------------------------------------------------------
  // .vscode/settings.json — merge missing keys only
  // ---------------------------------------------------------------------------

  private async mergeVscodeSettings(
    vscodeDirPath: string,
    result: WorkspaceConfigResult
  ): Promise<void> {
    const filePath = path.join(vscodeDirPath, "settings.json");

    // Read existing content (tolerates JSONC comments via a relaxed parse)
    let existing: Record<string, unknown> = {};
    if (fs.existsSync(filePath)) {
      try {
        const raw = fs.readFileSync(filePath, "utf8");
        existing = JSON.parse(stripJsonComments(raw)) as Record<string, unknown>;
      } catch {
        // Malformed JSON — leave existing content untouched; bail out
        result.skipped.push(`${filePath}: could not parse existing JSON, skipped.`);
        return;
      }
    }

    const defaults: Record<string, unknown> = {
      "ollamaBridge.delegationMode": "auto",
      "ollamaBridge.defaultModel": "deepseek-coder-v2:16b",
      "ollamaBridge.reasoningModel": "qwen2.5-coder:14b",
      "ollamaBridge.copilotReviewEnabled": true,
      "ollamaBridge.themisDbRules": true,
      "github.copilot.chat.codeGeneration.instructions": [
        { "file": ".github/copilot-instructions.md" },
      ],
      "github.copilot.chat.testGeneration.instructions": [
        { "file": ".github/copilot-instructions.md" },
      ],
      "github.copilot.chat.reviewSelection.instructions": [
        { "file": ".github/copilot-instructions.md" },
      ],
    };

    let changed = false;

    for (const [key, value] of Object.entries(defaults)) {
      if (!(key in existing)) {
        existing[key] = value;
        changed = true;
      } else if (
        key.startsWith("github.copilot.chat.") &&
        Array.isArray(existing[key]) &&
        Array.isArray(value)
      ) {
        // Merge instruction file refs: add missing entries
        const arr = existing[key] as Array<Record<string, unknown>>;
        for (const entry of value as Array<Record<string, unknown>>) {
          const alreadyPresent = arr.some(
            (e) => JSON.stringify(e) === JSON.stringify(entry)
          );
          if (!alreadyPresent) {
            arr.push(entry);
            changed = true;
          }
        }
      }
    }

    if (changed) {
      fs.writeFileSync(
        filePath,
        JSON.stringify(existing, null, 2) + "\n",
        "utf8"
      );
      result.modifiedFiles.push(filePath);
    }
  }

  // ---------------------------------------------------------------------------
  // .vscode/extensions.json — merge recommendations
  // ---------------------------------------------------------------------------

  private async mergeExtensionsJson(
    vscodeDirPath: string,
    result: WorkspaceConfigResult
  ): Promise<void> {
    const filePath = path.join(vscodeDirPath, "extensions.json");

    let existing: { recommendations?: string[]; [k: string]: unknown } = {};
    if (fs.existsSync(filePath)) {
      try {
        const raw = fs.readFileSync(filePath, "utf8");
        existing = JSON.parse(stripJsonComments(raw)) as typeof existing;
      } catch {
        result.skipped.push(`${filePath}: could not parse existing JSON, skipped.`);
        return;
      }
    }

    const recommendations: string[] = existing.recommendations ?? [];
    const ourId = "themisdb.vscode-ollama-bridge";

    if (!recommendations.includes(ourId)) {
      recommendations.push(ourId);
      existing.recommendations = recommendations;
      fs.writeFileSync(
        filePath,
        JSON.stringify(existing, null, 2) + "\n",
        "utf8"
      );
      result.modifiedFiles.push(filePath);
    }
  }

  // ---------------------------------------------------------------------------
  // .github/copilot-instructions.md — sentinel-marker section update
  // ---------------------------------------------------------------------------

  private async mergeCopilotInstructions(
    githubDirPath: string,
    result: WorkspaceConfigResult
  ): Promise<void> {
    const filePath = path.join(githubDirPath, "copilot-instructions.md");

    let content = "";
    if (fs.existsSync(filePath)) {
      content = fs.readFileSync(filePath, "utf8");
    }

    const beginIdx = content.indexOf(BRIDGE_SECTION_BEGIN);
    const endIdx   = content.indexOf(BRIDGE_SECTION_END);

    let updated: string;

    if (beginIdx !== -1 && endIdx !== -1 && endIdx > beginIdx) {
      // Section already exists — replace only what is between the markers
      const before = content.slice(0, beginIdx);
      const after  = content.slice(endIdx + BRIDGE_SECTION_END.length);
      updated = before + COPILOT_INSTRUCTIONS_BRIDGE_SECTION + after;
    } else {
      // First run — append the section (preserve any existing content)
      const separator = content.endsWith("\n") ? "\n" : "\n\n";
      updated = content + separator + COPILOT_INSTRUCTIONS_BRIDGE_SECTION + "\n";
    }

    if (updated !== content) {
      fs.writeFileSync(filePath, updated, "utf8");
      result.modifiedFiles.push(filePath);
    }
  }

  /** Returns the tags of all locally installed models. */
  async queryInstalledModels(): Promise<string[]> {
    try {
      const data = await this.getJson("/api/tags") as OllamaTagsResponse;
      return (data.models ?? []).map((m) => m.name);
    } catch {
      return [];
    }
  }

  /**
   * Interactive wizard:
   *  1. Queries installed models.
   *  2. Shows QuickPick of RANKED_CODING_MODELS (installed ones ticked).
   *  3. Pulls each selected-but-not-installed model with progress feedback.
   */
  async pickAndPullModels(): Promise<void> {
    const installed = await this.queryInstalledModels();

    // Normalise installed names (strip registry prefix if present)
    const normalise = (tag: string) =>
      tag.replace(/^library\//, "").toLowerCase();
    const installedNorm = new Set(installed.map(normalise));

    const items: vscode.QuickPickItem[] = RANKED_CODING_MODELS.map((m) => {
      const isInstalled = installedNorm.has(normalise(m.tag));
      return {
        label: `$(${isInstalled ? "check" : "cloud-download"}) ${m.label}`,
        description: `${m.tag}  |  HumanEval ${m.humanEvalScore}%  |  ~${m.vramGb} GB VRAM`,
        detail: m.description + (isInstalled ? "  ✅ already installed" : ""),
        picked: isInstalled,
        // Store tag in alwaysShow so we can retrieve it
        alwaysShow: true,
        // We attach the entry as metadata via a map below
        _tag: m.tag,
        _installed: isInstalled,
      } as vscode.QuickPickItem & { _tag: string; _installed: boolean };
    });

    const picked = await vscode.window.showQuickPick(items, {
      title: "Ollama Bridge — Select coding models to install",
      placeHolder:
        "Select models (ranked by HumanEval score). Pre-ticked = already installed.",
      canPickMany: true,
      matchOnDescription: true,
      matchOnDetail: true,
    });

    if (!picked || picked.length === 0) {
      return;
    }

    const toDownload = (
      picked as Array<vscode.QuickPickItem & { _tag: string; _installed: boolean }>
    ).filter((p) => !p._installed);

    if (toDownload.length === 0) {
      void vscode.window.showInformationMessage(
        "All selected models are already installed."
      );
      return;
    }

    // Pull each model sequentially with progress
    for (const item of toDownload) {
      await this.pullWithProgress(item._tag);
    }
  }

  /**
   * Ensures the top-ranked installed model exists; if nothing is installed,
   * silently pulls the lightweight default (`codellama:7b`).
   * Returns the tag of the model to use.
   */
  async ensureRecommendedModel(preferredTag: string): Promise<string> {
    const installed = await this.queryInstalledModels();
    if (installed.length === 0) {
      const fallback = "codellama:7b";
      void vscode.window.showInformationMessage(
        `No Ollama models found. Pulling ${fallback} in the background…`
      );
      // Fire-and-forget background pull
      void this.pullWithProgress(fallback);
      return fallback;
    }

    // Return preferred if installed, else first ranked match, else first installed
    const normInstalled = new Set(installed.map((t) => t.replace(/^library\//, "").toLowerCase()));
    if (normInstalled.has(preferredTag.toLowerCase())) {
      return preferredTag;
    }
    for (const entry of RANKED_CODING_MODELS) {
      if (normInstalled.has(entry.tag.toLowerCase())) {
        return entry.tag;
      }
    }
    return installed[0];
  }

  // ---------------------------------------------------------------------------
  // Pull implementation
  // ---------------------------------------------------------------------------

  private async pullWithProgress(tag: string): Promise<void> {
    await vscode.window.withProgress(
      {
        location: vscode.ProgressLocation.Notification,
        title: `Ollama: pulling ${tag}`,
        cancellable: true,
      },
      async (progress, cancelToken) => {
        const abortController = new AbortController();
        cancelToken.onCancellationRequested(() => abortController.abort());

        progress.report({ message: "Connecting…", increment: 0 });

        try {
          await this.streamPull(tag, abortController.signal, (p) => {
            if (p.error) {
              throw new Error(p.error);
            }
            if (p.total && p.completed) {
              const pct = Math.round((p.completed / p.total) * 100);
              progress.report({
                message: `${p.status ?? "downloading"} — ${pct}%`,
                increment: 0,
              });
            } else if (p.status) {
              progress.report({ message: p.status });
            }
          });
          void vscode.window.showInformationMessage(
            `✅ Ollama: ${tag} ready.`
          );
        } catch (err) {
          if (!abortController.signal.aborted) {
            void vscode.window.showErrorMessage(
              `Ollama pull failed for ${tag}: ${
                err instanceof Error ? err.message : String(err)
              }`
            );
          }
        }
      }
    );
  }

  private streamPull(
    tag: string,
    signal: AbortSignal,
    onProgress: (p: OllamaPullProgress) => void
  ): Promise<void> {
    const body = JSON.stringify({ name: tag, stream: true });
    const url = new URL("/api/pull", this.endpoint);
    const isHttps = url.protocol === "https:";
    const transport = isHttps ? https : http;

    return new Promise<void>((resolve, reject) => {
      const req = transport.request(
        {
          hostname: url.hostname,
          port: url.port || (isHttps ? 443 : 80),
          path: url.pathname,
          method: "POST",
          headers: {
            "Content-Type": "application/json",
            "Content-Length": Buffer.byteLength(body),
          },
        },
        (res) => {
          if (res.statusCode !== 200) {
            reject(new Error(`HTTP ${res.statusCode ?? "?"} from /api/pull`));
            return;
          }

          let buffer = "";
          res.setEncoding("utf8");

          res.on("data", (chunk: string) => {
            buffer += chunk;
            const lines = buffer.split("\n");
            buffer = lines.pop() ?? "";
            for (const line of lines) {
              const trimmed = line.trim();
              if (!trimmed) {
                continue;
              }
              try {
                const parsed = JSON.parse(trimmed) as OllamaPullProgress;
                onProgress(parsed);
              } catch {
                // Partial line — skip
              }
            }
          });

          res.on("end", () => resolve());
          res.on("error", reject);
        }
      );

      req.on("error", reject);
      signal.addEventListener("abort", () => {
        req.destroy();
        reject(new Error("Pull aborted."));
      });

      req.write(body);
      req.end();
    });
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  private getJson(path: string): Promise<unknown> {
    const url = new URL(path, this.endpoint);
    const isHttps = url.protocol === "https:";
    const transport = isHttps ? https : http;

    return new Promise<unknown>((resolve, reject) => {
      const req = transport.get(
        {
          hostname: url.hostname,
          port: url.port || (isHttps ? 443 : 80),
          path: url.pathname + url.search,
        },
        (res) => {
          let data = "";
          res.setEncoding("utf8");
          res.on("data", (chunk: string) => (data += chunk));
          res.on("end", () => {
            try {
              resolve(JSON.parse(data));
            } catch {
              reject(new Error("Failed to parse Ollama JSON."));
            }
          });
          res.on("error", reject);
        }
      );
      req.on("error", reject);
    });
  }
}

// ---------------------------------------------------------------------------
// Utility: strip single-line and block comments from JSONC
// ---------------------------------------------------------------------------

/**
 * Minimal JSONC comment stripper.
 * Removes single-line (//) and block (slash-star) comments while preserving
 * string literals that contain comment-like sequences.
 */
function stripJsonComments(jsonc: string): string {
  let result = "";
  let i = 0;
  let inString = false;

  while (i < jsonc.length) {
    const ch = jsonc[i];

    if (inString) {
      if (ch === "\\" && i + 1 < jsonc.length) {
        result += ch + jsonc[i + 1];
        i += 2;
        continue;
      }
      if (ch === '"') {
        inString = false;
      }
      result += ch;
      i++;
      continue;
    }

    if (ch === '"') {
      inString = true;
      result += ch;
      i++;
      continue;
    }

    // Single-line comment
    if (ch === "/" && jsonc[i + 1] === "/") {
      while (i < jsonc.length && jsonc[i] !== "\n") {
        i++;
      }
      continue;
    }

    // Block comment
    if (ch === "/" && jsonc[i + 1] === "*") {
      i += 2;
      while (i < jsonc.length - 1 && !(jsonc[i] === "*" && jsonc[i + 1] === "/")) {
        if (jsonc[i] === "\n") {
          result += "\n";
        }
        i++;
      }
      i += 2; // skip */
      continue;
    }

    result += ch;
    i++;
  }

  return result;
}
