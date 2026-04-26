/**
 * Ollama Bridge — Settings WebView Panel
 *
 * Full settings UI covering:
 *  - Ollama endpoint & connection health
 *  - Installed model list with per-model actions (set default, set reasoning, delete)
 *  - Download new models from the ranked catalog with live progress
 *  - All router configuration knobs (delegation mode, timeout, etc.)
 *  - Admin section: apply workspace config to .vscode/settings.json and
 *    .github/copilot-instructions.md
 */

import * as vscode from "vscode";
import { ModelSetupManager, RANKED_CODING_MODELS } from "./modelSetup.js";

// ---------------------------------------------------------------------------
// Types shared between extension host and WebView
// ---------------------------------------------------------------------------

interface InstalledModel {
  name: string;
  sizeGb: number | null;
  modifiedAt: string | null;
}

interface SettingsState {
  endpoint: string;
  defaultModel: string;
  reasoningModel: string;
  delegationMode: "auto" | "always" | "never";
  copilotReviewEnabled: boolean;
  requestTimeoutMs: number;
  contextTokenBudget: number;
  themisDbRules: boolean;
  routeBoilerplateToLocal: boolean;
  routeTestsToLocal: boolean;
  routeRefactorsToLocal: boolean;
  routeDocsToLocal: boolean;
  routeCmakeToLocal: boolean;
  languageProfiles: Record<string, string>;
  installedModels: InstalledModel[];
  connectionOk: boolean;
  connectionError: string;
}

type MessageToExtension =
  | { type: "refresh" }
  | { type: "saveSetting"; key: string; value: unknown }
  | { type: "setDefaultModel"; model: string }
  | { type: "setReasoningModel"; model: string }
  | { type: "downloadModel"; tag: string }
  | { type: "deleteModel"; name: string }
  | { type: "applyWorkspaceConfig" }
  | { type: "openSettings" };

type MessageToWebView =
  | { type: "state"; state: SettingsState }
  | { type: "downloadProgress"; tag: string; message: string; percent: number | null }
  | { type: "downloadDone"; tag: string; success: boolean; error?: string }
  | {
      type: "workspaceConfigResult";
      modified: string[];
      skipped: string[];
      previewed: string[];
    };

// ---------------------------------------------------------------------------
// Panel singleton
// ---------------------------------------------------------------------------

export class SettingsPanel {
  private static _instance: SettingsPanel | undefined;

  private readonly _panel: vscode.WebviewPanel;
  private _disposables: vscode.Disposable[] = [];

  static show(context: vscode.ExtensionContext): void {
    if (SettingsPanel._instance) {
      SettingsPanel._instance._panel.reveal(vscode.ViewColumn.One);
      return;
    }
    SettingsPanel._instance = new SettingsPanel(context);
  }

  private constructor(context: vscode.ExtensionContext) {
    this._panel = vscode.window.createWebviewPanel(
      "ollamaBridgeSettings",
      "Ollama Bridge — Settings",
      vscode.ViewColumn.One,
      {
        enableScripts: true,
        retainContextWhenHidden: true,
        localResourceRoots: [context.extensionUri],
      }
    );

    this._panel.webview.html = this._buildHtml();

    this._panel.webview.onDidReceiveMessage(
      (msg: MessageToExtension) => this._handleMessage(msg),
      undefined,
      this._disposables
    );

    this._panel.onDidDispose(() => this._dispose(), undefined, this._disposables);

    // Push initial state after the panel is ready
    void this._pushState();
  }

  private _dispose(): void {
    SettingsPanel._instance = undefined;
    for (const d of this._disposables) {
      d.dispose();
    }
    this._disposables = [];
  }

  // ---------------------------------------------------------------------------
  // State collection
  // ---------------------------------------------------------------------------

  private async _collectState(): Promise<SettingsState> {
    const cfg = vscode.workspace.getConfiguration("copilotOllamaRouter");
    const endpoint = cfg.get<string>("endpoint", "http://localhost:11434");

    const setupManager = new ModelSetupManager(endpoint);
    let installedModels: InstalledModel[] = [];
    let connectionOk = false;
    let connectionError = "";

    try {
      const raw = await (setupManager as unknown as {
        getJson(path: string): Promise<unknown>;
      }).getJson("/api/tags") as {
        models?: Array<{ name: string; size?: number; modified_at?: string }>;
      };
      connectionOk = true;
      installedModels = (raw.models ?? []).map((m) => ({
        name: m.name,
        sizeGb: m.size ? parseFloat((m.size / 1e9).toFixed(1)) : null,
        modifiedAt: m.modified_at ?? null,
      }));
    } catch (err) {
      connectionError = err instanceof Error ? err.message : String(err);
    }

    return {
      endpoint,
      defaultModel: cfg.get<string>("defaultModel", "codellama:13b"),
      reasoningModel: cfg.get<string>("reasoningModel", "llama3"),
      delegationMode: cfg.get<"auto" | "always" | "never">("delegationMode", "auto"),
      copilotReviewEnabled: cfg.get<boolean>("copilotReviewEnabled", true),
      requestTimeoutMs: cfg.get<number>("requestTimeoutMs", 60000),
      contextTokenBudget: cfg.get<number>("contextTokenBudget", 2048),
      themisDbRules: cfg.get<boolean>("themisDbRules", true),
      routeBoilerplateToLocal: cfg.get<boolean>("routeBoilerplateToLocal", true),
      routeTestsToLocal: cfg.get<boolean>("routeTestsToLocal", true),
      routeRefactorsToLocal: cfg.get<boolean>("routeRefactorsToLocal", true),
      routeDocsToLocal: cfg.get<boolean>("routeDocsToLocal", true),
      routeCmakeToLocal: cfg.get<boolean>("routeCmakeToLocal", true),
      languageProfiles: cfg.get<Record<string, string>>("languageProfiles", {}),
      installedModels,
      connectionOk,
      connectionError,
    };
  }

  private async _pushState(): Promise<void> {
    const state = await this._collectState();
    const msg: MessageToWebView = { type: "state", state };
    void this._panel.webview.postMessage(msg);
  }

  // ---------------------------------------------------------------------------
  // Message handler
  // ---------------------------------------------------------------------------

  private _handleMessage(msg: MessageToExtension): void {
    switch (msg.type) {
      case "refresh":
        void this._pushState();
        break;

      case "saveSetting":
        void this._saveSetting(msg.key, msg.value);
        break;

      case "setDefaultModel":
        void vscode.workspace
          .getConfiguration("copilotOllamaRouter")
          .update("defaultModel", msg.model, vscode.ConfigurationTarget.Workspace)
          .then(() => this._pushState());
        break;

      case "setReasoningModel":
        void vscode.workspace
          .getConfiguration("copilotOllamaRouter")
          .update("reasoningModel", msg.model, vscode.ConfigurationTarget.Workspace)
          .then(() => this._pushState());
        break;

      case "downloadModel":
        void this._downloadModel(msg.tag);
        break;

      case "deleteModel":
        void this._deleteModel(msg.name);
        break;

      case "applyWorkspaceConfig":
        void this._applyWorkspaceConfig();
        break;

      case "openSettings":
        void vscode.commands.executeCommand(
          "workbench.action.openSettings",
          "copilotOllamaRouter"
        );
        break;
    }
  }

  private async _saveSetting(key: string, value: unknown): Promise<void> {
    const cfg = vscode.workspace.getConfiguration("copilotOllamaRouter");
    await cfg.update(key, value, vscode.ConfigurationTarget.Workspace);
    await this._pushState();
  }

  private async _downloadModel(tag: string): Promise<void> {
    const cfg = vscode.workspace.getConfiguration("copilotOllamaRouter");
    const endpoint = cfg.get<string>("endpoint", "http://localhost:11434");
    const setupManager = new ModelSetupManager(endpoint);

    const sendProgress = (message: string, percent: number | null): void => {
      const msg: MessageToWebView = {
        type: "downloadProgress",
        tag,
        message,
        percent,
      };
      void this._panel.webview.postMessage(msg);
    };

    sendProgress("Connecting…", null);

    try {
      // Access internal streamPull via the public pullModel method we add
      await (setupManager as unknown as {
        pullModelForPanel(
          tag: string,
          onProgress: (message: string, percent: number | null) => void
        ): Promise<void>;
      }).pullModelForPanel(tag, sendProgress);

      const done: MessageToWebView = { type: "downloadDone", tag, success: true };
      void this._panel.webview.postMessage(done);
      await this._pushState();
    } catch (err) {
      const done: MessageToWebView = {
        type: "downloadDone",
        tag,
        success: false,
        error: err instanceof Error ? err.message : String(err),
      };
      void this._panel.webview.postMessage(done);
    }
  }

  private async _deleteModel(name: string): Promise<void> {
    const cfg = vscode.workspace.getConfiguration("copilotOllamaRouter");
    const endpoint = cfg.get<string>("endpoint", "http://localhost:11434");

    const confirm = await vscode.window.showWarningMessage(
      `Delete model "${name}" from Ollama?`,
      { modal: true },
      "Delete"
    );
    if (confirm !== "Delete") {
      return;
    }

    try {
      const setupManager = new ModelSetupManager(endpoint);
      await (setupManager as unknown as {
        deleteModel(name: string): Promise<void>;
      }).deleteModel(name);
      void vscode.window.showInformationMessage(`Model "${name}" deleted.`);
    } catch (err) {
      void vscode.window.showErrorMessage(
        `Could not delete "${name}": ${err instanceof Error ? err.message : String(err)}`
      );
    }

    await this._pushState();
  }

  private async _applyWorkspaceConfig(): Promise<void> {
    const cfg = vscode.workspace.getConfiguration("copilotOllamaRouter");
    const endpoint = cfg.get<string>("endpoint", "http://localhost:11434");
    const setupManager = new ModelSetupManager(endpoint);

    const preview = await setupManager.previewWorkspaceConfig();
    const previewed = preview.previews
      .filter((item) => item.changed)
      .map((item) => item.filePath);
    const result = await setupManager.applyWorkspaceConfigWithConfirmation();
    const msg: MessageToWebView = {
      type: "workspaceConfigResult",
      modified: result.modifiedFiles,
      skipped: result.skipped,
      previewed,
    };
    void this._panel.webview.postMessage(msg);
  }

  // ---------------------------------------------------------------------------
  // HTML
  // ---------------------------------------------------------------------------

  private _buildHtml(): string {
    const catalogJson = JSON.stringify(
      RANKED_CODING_MODELS.map((m) => ({
        tag: m.tag,
        label: m.label,
        description: m.description,
        vramGb: m.vramGb,
        humanEvalScore: m.humanEvalScore,
        focus: m.focus,
      }))
    );

    return /* html */ `<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="Content-Security-Policy"
      content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<title>Ollama Bridge — Settings</title>
<style>
  :root {
    --bg: var(--vscode-editor-background);
    --fg: var(--vscode-editor-foreground);
    --border: var(--vscode-panel-border);
    --input-bg: var(--vscode-input-background);
    --input-fg: var(--vscode-input-foreground);
    --input-border: var(--vscode-input-border);
    --btn-bg: var(--vscode-button-background);
    --btn-fg: var(--vscode-button-foreground);
    --btn-hover: var(--vscode-button-hoverBackground);
    --btn-sec-bg: var(--vscode-button-secondaryBackground);
    --btn-sec-fg: var(--vscode-button-secondaryForeground);
    --badge-bg: var(--vscode-badge-background);
    --badge-fg: var(--vscode-badge-foreground);
    --warn-fg: var(--vscode-editorWarning-foreground, #cca700);
    --ok-fg: var(--vscode-terminal-ansiGreen, #4ec94e);
    --err-fg: var(--vscode-errorForeground, #f44747);
    --section-bg: var(--vscode-sideBar-background);
  }

  * { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    font-family: var(--vscode-font-family);
    font-size: var(--vscode-font-size);
    color: var(--fg);
    background: var(--bg);
    padding: 20px 24px 40px;
    max-width: 900px;
  }

  h1 { font-size: 1.4em; font-weight: 600; margin-bottom: 20px; }
  h2 { font-size: 1.05em; font-weight: 600; margin-bottom: 12px; color: var(--vscode-settings-headerForeground, var(--fg)); }

  section {
    background: var(--section-bg);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 16px 18px;
    margin-bottom: 18px;
  }

  .row { display: flex; align-items: center; gap: 10px; margin-bottom: 10px; flex-wrap: wrap; }
  .row label { min-width: 200px; opacity: 0.85; }
  .row input[type=text], .row input[type=number], .row select {
    flex: 1; min-width: 200px; background: var(--input-bg); color: var(--input-fg);
    border: 1px solid var(--input-border, var(--border)); border-radius: 2px;
    padding: 4px 8px; font-size: inherit; font-family: inherit;
  }
  .row input[type=checkbox] { width: 16px; height: 16px; cursor: pointer; }

  button {
    background: var(--btn-bg); color: var(--btn-fg);
    border: none; border-radius: 2px;
    padding: 5px 12px; cursor: pointer; font-size: inherit; font-family: inherit;
    white-space: nowrap;
  }
  button:hover { background: var(--btn-hover); }
  button.secondary { background: var(--btn-sec-bg); color: var(--btn-sec-fg); }
  button.danger { background: var(--vscode-inputValidation-errorBackground, #5a1d1d); color: var(--err-fg); }
  button:disabled { opacity: 0.45; cursor: not-allowed; }

  .status-badge {
    display: inline-block;
    padding: 2px 8px; border-radius: 10px; font-size: 0.85em;
    background: var(--badge-bg); color: var(--badge-fg);
  }
  .status-badge.ok { background: rgba(78,201,78,0.15); color: var(--ok-fg); }
  .status-badge.err { background: rgba(244,71,71,0.15); color: var(--err-fg); }

  table { width: 100%; border-collapse: collapse; }
  th { text-align: left; opacity: 0.7; font-weight: 500; padding: 4px 8px; border-bottom: 1px solid var(--border); font-size: 0.9em; }
  td { padding: 6px 8px; border-bottom: 1px solid rgba(128,128,128,0.15); vertical-align: middle; }
  tr:last-child td { border-bottom: none; }

  .tag { font-family: var(--vscode-editor-font-family, monospace); font-size: 0.9em; }
  .score { font-size: 0.85em; opacity: 0.75; }
  .vram { font-size: 0.85em; opacity: 0.75; }
  .actions { display: flex; gap: 6px; flex-wrap: wrap; }

  .progress-row { margin-top: 6px; }
  .progress-bar-wrap { background: rgba(128,128,128,0.2); border-radius: 3px; height: 6px; overflow: hidden; margin-top: 4px; }
  .progress-bar { height: 100%; background: var(--btn-bg); transition: width 0.3s; }
  .progress-label { font-size: 0.82em; opacity: 0.75; }

  .info-box {
    margin-top: 8px; padding: 8px 12px; border-radius: 3px; font-size: 0.9em;
    background: rgba(128,128,128,0.1); border-left: 3px solid var(--btn-bg);
  }
  .info-box.warn { border-left-color: var(--warn-fg); }
  .info-box.ok { border-left-color: var(--ok-fg); }
  .info-box.err { border-left-color: var(--err-fg); }

  .chip {
    display: inline-block; font-size: 0.78em; padding: 1px 6px; border-radius: 8px;
    background: var(--badge-bg); color: var(--badge-fg); margin-left: 4px;
  }
  .chip.active { background: rgba(0,122,204,0.25); color: var(--vscode-textLink-activeForeground, #4fc1ff); }

  #spinner { display: inline-block; animation: spin 1s linear infinite; }
  @keyframes spin { to { transform: rotate(360deg); } }

  details > summary { cursor: pointer; font-weight: 500; user-select: none; margin-bottom: 10px; }
</style>
</head>
<body>
<h1>⚙️ Ollama Bridge — Settings</h1>

<!-- ── Connection ─────────────────────────────────────────────── -->
<section>
  <h2>🔌 Ollama Connection</h2>
  <div class="row">
    <label for="endpoint">Endpoint URL</label>
    <input type="text" id="endpoint" placeholder="http://localhost:11434">
    <button onclick="saveEndpoint()">Save</button>
    <button class="secondary" onclick="refresh()">↻ Refresh</button>
  </div>
  <div id="conn-status"></div>
</section>

<!-- ── Installed Models ───────────────────────────────────────── -->
<section>
  <h2>📦 Installed Models</h2>
  <div id="installed-models-wrap">
    <span id="spinner">⟳</span> Loading…
  </div>
</section>

<!-- ── Download Catalog ──────────────────────────────────────── -->
<section>
  <details open>
    <summary>⬇️ Download Models from Catalog</summary>
    <div id="catalog-wrap"></div>
  </details>
</section>

<!-- ── Router Settings ───────────────────────────────────────── -->
<section>
  <h2>🔀 Routing Configuration</h2>

  <div class="row">
    <label for="delegationMode">Delegation Mode</label>
    <select id="delegationMode" onchange="saveSetting('delegationMode', this.value)">
      <option value="auto">auto — classify each request</option>
      <option value="always">always — always use local Ollama</option>
      <option value="never">never — always use Copilot (cloud)</option>
    </select>
  </div>

  <div class="row">
    <label for="requestTimeoutMs">Request Timeout (ms)</label>
    <input type="number" id="requestTimeoutMs" min="5000" max="600000" step="1000"
           onchange="saveSetting('requestTimeoutMs', +this.value)">
  </div>

  <div class="row">
    <label for="contextTokenBudget">Context Token Budget</label>
    <input type="number" id="contextTokenBudget" min="128" max="32768" step="128"
           onchange="saveSetting('contextTokenBudget', +this.value)">
    <span style="opacity:0.75; font-size:0.9em">Approximate budget for prepended file context (1 token ≈ 4 chars)</span>
  </div>

  <div class="row">
    <label for="copilotReviewEnabled">Copilot Quality Review</label>
    <input type="checkbox" id="copilotReviewEnabled"
           onchange="saveSetting('copilotReviewEnabled', this.checked)">
    <span style="opacity:0.75; font-size:0.9em">Run Copilot review pass on Ollama output</span>
  </div>

  <div class="row">
    <label for="themisDbRules">ThemisDB Routing Rules</label>
    <input type="checkbox" id="themisDbRules"
           onchange="saveSetting('themisDbRules', this.checked)">
    <span style="opacity:0.75; font-size:0.9em">C++ → Ollama; Security/Architecture → Copilot</span>
  </div>

  <h2 style="margin-top:14px">🎯 Auto-Tag Policy (\`@ollama /local\`)</h2>
  <div class="row">
    <label for="routeBoilerplateToLocal">Boilerplate / Scaffolding</label>
    <input type="checkbox" id="routeBoilerplateToLocal"
           onchange="saveSetting('routeBoilerplateToLocal', this.checked)">
  </div>
  <div class="row">
    <label for="routeTestsToLocal">Tests / Specs / Mocks</label>
    <input type="checkbox" id="routeTestsToLocal"
           onchange="saveSetting('routeTestsToLocal', this.checked)">
  </div>
  <div class="row">
    <label for="routeRefactorsToLocal">Refactoring / Cleanup</label>
    <input type="checkbox" id="routeRefactorsToLocal"
           onchange="saveSetting('routeRefactorsToLocal', this.checked)">
  </div>
  <div class="row">
    <label for="routeDocsToLocal">Docs / Doxygen / README</label>
    <input type="checkbox" id="routeDocsToLocal"
           onchange="saveSetting('routeDocsToLocal', this.checked)">
  </div>
  <div class="row">
    <label for="routeCmakeToLocal">CMake / Build System</label>
    <input type="checkbox" id="routeCmakeToLocal"
           onchange="saveSetting('routeCmakeToLocal', this.checked)">
  </div>

  <div class="row" style="display:block; margin-top: 10px;">
    <label for="languageProfiles" style="display:block; margin-bottom:6px;">Language Profiles (lang → model)</label>
    <textarea id="languageProfiles" rows="6" style="width:100%; font-family: var(--vscode-editor-font-family, Consolas, monospace);"
      placeholder='{"cpp":"deepseek-coder-v2:16b","python":"qwen2.5-coder:7b"}'></textarea>
    <div style="margin-top:6px; display:flex; gap:8px; align-items:center; flex-wrap:wrap;">
      <button class="secondary" onclick="saveLanguageProfiles()">Save Language Profiles</button>
      <span id="languageProfilesStatus" style="opacity:0.8; font-size:0.9em"></span>
    </div>
    <p style="opacity:0.75; margin-top:6px; font-size:0.9em;">
      JSON object mapping VS Code language IDs to Ollama model tags. Example: <code>{"cpp":"deepseek-coder-v2:16b"}</code>
    </p>
  </div>
</section>

<!-- ── Admin ─────────────────────────────────────────────────── -->
<section>
  <h2>🛠️ Admin — Workspace Configuration</h2>
  <p style="opacity:0.8; margin-bottom:12px; font-size:0.92em">
    Writes/updates <code>.vscode/settings.json</code>, <code>.vscode/extensions.json</code>
    and <code>.github/copilot-instructions.md</code> in the current workspace root.
    Existing content is preserved — only missing entries are added.
  </p>
  <div class="row">
    <button onclick="applyWorkspaceConfig()">Apply Workspace Config</button>
    <button class="secondary" onclick="openVscodeSettings()">Open VS Code Settings</button>
  </div>
  <div id="admin-result"></div>
</section>

<script>
// --------------------------------------------------------------------------
// State
// --------------------------------------------------------------------------

const vscode = acquireVsCodeApi();
const CATALOG = ${catalogJson};
const downloadState = {}; // tag → { active, percent, message }

// --------------------------------------------------------------------------
// VSCode message bridge
// --------------------------------------------------------------------------

window.addEventListener('message', (event) => {
  const msg = event.data;
  if (msg.type === 'state') {
    renderState(msg.state);
  } else if (msg.type === 'downloadProgress') {
    handleDownloadProgress(msg.tag, msg.message, msg.percent);
  } else if (msg.type === 'downloadDone') {
    handleDownloadDone(msg.tag, msg.success, msg.error);
  } else if (msg.type === 'workspaceConfigResult') {
    renderAdminResult(msg.modified, msg.skipped, msg.previewed);
  }
});

function post(msg) { vscode.postMessage(msg); }
function refresh() { post({ type: 'refresh' }); }
function saveSetting(key, value) { post({ type: 'saveSetting', key, value }); }
function applyWorkspaceConfig() { post({ type: 'applyWorkspaceConfig' }); }
function openVscodeSettings() { post({ type: 'openSettings' }); }

function saveEndpoint() {
  const val = document.getElementById('endpoint').value.trim();
  if (val) saveSetting('endpoint', val);
}

function saveLanguageProfiles() {
  const statusEl = document.getElementById('languageProfilesStatus');
  const raw = document.getElementById('languageProfiles').value.trim();

  let parsed = {};
  if (raw.length > 0) {
    try {
      parsed = JSON.parse(raw);
    } catch (err) {
      if (statusEl) {
        const msg = err instanceof Error ? err.message : String(err);
        statusEl.innerHTML = '<span class="status-badge err">Invalid JSON: ' + escHtml(msg) + '</span>';
      }
      return;
    }
  }

  if (typeof parsed !== 'object' || parsed === null || Array.isArray(parsed)) {
    if (statusEl) {
      statusEl.innerHTML = '<span class="status-badge err">Must be a JSON object (e.g. {"cpp":"model:tag"})</span>';
    }
    return;
  }

  for (const [k, v] of Object.entries(parsed)) {
    if (typeof v !== 'string') {
      if (statusEl) {
        statusEl.innerHTML = '<span class="status-badge err">Value for "' + escHtml(k) + '" must be a string.</span>';
      }
      return;
    }
  }

  saveSetting('languageProfiles', parsed);
  if (statusEl) {
    statusEl.innerHTML = '<span class="status-badge ok">Saved</span>';
  }
}

// --------------------------------------------------------------------------
// Render state
// --------------------------------------------------------------------------

function renderState(state) {
  // Connection
  document.getElementById('endpoint').value = state.endpoint;
  const connEl = document.getElementById('conn-status');
  if (state.connectionOk) {
    connEl.innerHTML = \`<span class="status-badge ok">✅ Connected — \${state.installedModels.length} model(s) installed</span>\`;
  } else {
    connEl.innerHTML = \`<span class="status-badge err">❌ Cannot connect: \${escHtml(state.connectionError)}</span>
      <div class="info-box warn" style="margin-top:8px">
        Make sure Ollama is running: <code>ollama serve</code>
      </div>\`;
  }

  // Installed models
  renderInstalledModels(state);

  // Catalog
  renderCatalog(state);

  // Settings
  const dm = document.getElementById('delegationMode');
  if (dm) dm.value = state.delegationMode;
  setVal('requestTimeoutMs', state.requestTimeoutMs);
  setVal('contextTokenBudget', state.contextTokenBudget);
  setCheck('copilotReviewEnabled', state.copilotReviewEnabled);
  setCheck('themisDbRules', state.themisDbRules);
  setCheck('routeBoilerplateToLocal', state.routeBoilerplateToLocal);
  setCheck('routeTestsToLocal', state.routeTestsToLocal);
  setCheck('routeRefactorsToLocal', state.routeRefactorsToLocal);
  setCheck('routeDocsToLocal', state.routeDocsToLocal);
  setCheck('routeCmakeToLocal', state.routeCmakeToLocal);

  const lpEl = document.getElementById('languageProfiles');
  if (lpEl) {
    lpEl.value = JSON.stringify(state.languageProfiles ?? {}, null, 2);
  }

  const lpStatus = document.getElementById('languageProfilesStatus');
  if (lpStatus) {
    lpStatus.innerHTML = '';
  }
}

function setVal(id, v) {
  const el = document.getElementById(id);
  if (el) el.value = v;
}
function setCheck(id, v) {
  const el = document.getElementById(id);
  if (el) el.checked = v;
}

// --------------------------------------------------------------------------
// Installed models table
// --------------------------------------------------------------------------

function renderInstalledModels(state) {
  const wrap = document.getElementById('installed-models-wrap');
  if (!state.connectionOk) {
    wrap.innerHTML = '<span style="opacity:0.6">Not connected to Ollama.</span>';
    return;
  }
  if (state.installedModels.length === 0) {
    wrap.innerHTML = '<span style="opacity:0.6">No models installed yet. Use the catalog below to download one.</span>';
    return;
  }

  const rows = state.installedModels.map(m => {
    const isDefault = m.name === state.defaultModel || m.name.split(':')[0] === state.defaultModel.split(':')[0];
    const isReasoning = m.name === state.reasoningModel || m.name.split(':')[0] === state.reasoningModel.split(':')[0];
    const chips = [
      isDefault ? '<span class="chip active">default</span>' : '',
      isReasoning ? '<span class="chip active">reasoning</span>' : '',
    ].join('');
    const sizeStr = m.sizeGb != null ? \`\${m.sizeGb} GB\` : '';
    return \`<tr>
      <td><span class="tag">\${escHtml(m.name)}</span>\${chips}</td>
      <td class="vram">\${sizeStr}</td>
      <td>
        <div class="actions">
          \${!isDefault ? \`<button onclick="setDefault('\${escAttr(m.name)}')" title="Use as default code model">Set Default</button>\` : ''}
          \${!isReasoning ? \`<button class="secondary" onclick="setReasoning('\${escAttr(m.name)}')" title="Use as reasoning model">Set Reasoning</button>\` : ''}
          <button class="danger" onclick="deleteModel('\${escAttr(m.name)}')" title="Remove model from Ollama">Delete</button>
        </div>
      </td>
    </tr>\`;
  }).join('');

  wrap.innerHTML = \`<table>
    <thead><tr><th>Model</th><th>Size</th><th>Actions</th></tr></thead>
    <tbody>\${rows}</tbody>
  </table>\`;
}

function setDefault(name) {
  post({ type: 'setDefaultModel', model: name });
}
function setReasoning(name) {
  post({ type: 'setReasoningModel', model: name });
}
function deleteModel(name) {
  post({ type: 'deleteModel', name });
}

// --------------------------------------------------------------------------
// Catalog
// --------------------------------------------------------------------------

let _currentInstalledNames = new Set();

function renderCatalog(state) {
  _currentInstalledNames = new Set(state.installedModels.map(m => m.name.toLowerCase()));

  const rows = CATALOG.map(m => {
    const normTag = m.tag.toLowerCase();
    const isInstalled = _currentInstalledNames.has(normTag) ||
      [..._currentInstalledNames].some(n => n.startsWith(normTag.split(':')[0] + ':'));
    const dl = downloadState[m.tag];
    const isActive = dl && dl.active;

    let actionCell = '';
    if (isInstalled) {
      actionCell = '<span class="status-badge ok">Installed</span>';
    } else if (isActive) {
      const pct = dl.percent != null ? dl.percent : 0;
      actionCell = \`<div class="progress-row">
        <span class="progress-label">\${escHtml(dl.message)}</span>
        <div class="progress-bar-wrap">
          <div class="progress-bar" id="pb-\${cssId(m.tag)}" style="width:\${pct}%"></div>
        </div>
      </div>\`;
    } else {
      actionCell = \`<button onclick="downloadModel('\${escAttr(m.tag)}')" title="Download \${escAttr(m.tag)} from Ollama">⬇ Download</button>\`;
    }

    return \`<tr>
      <td>
        <strong>\${escHtml(m.label)}</strong><br>
        <span class="tag">\${escHtml(m.tag)}</span>
      </td>
      <td class="score">HumanEval \${m.humanEvalScore}%</td>
      <td class="vram">~\${m.vramGb} GB</td>
      <td style="opacity:0.75; font-size:0.87em">\${escHtml(m.description)}</td>
      <td id="cat-action-\${cssId(m.tag)}">\${actionCell}</td>
    </tr>\`;
  }).join('');

  document.getElementById('catalog-wrap').innerHTML = \`<table>
    <thead><tr><th>Model</th><th>Score</th><th>VRAM</th><th>Description</th><th></th></tr></thead>
    <tbody>\${rows}</tbody>
  </table>\`;
}

function downloadModel(tag) {
  downloadState[tag] = { active: true, percent: 0, message: 'Starting…' };
  updateCatalogRow(tag);
  post({ type: 'downloadModel', tag });
}

function handleDownloadProgress(tag, message, percent) {
  downloadState[tag] = { active: true, percent: percent ?? 0, message };
  const pb = document.getElementById('pb-' + cssId(tag));
  if (pb) {
    pb.style.width = (percent ?? 0) + '%';
    const label = pb.closest('.progress-row')?.querySelector('.progress-label');
    if (label) label.textContent = message;
  }
}

function handleDownloadDone(tag, success, error) {
  delete downloadState[tag];
  if (!success) {
    const cell = document.getElementById('cat-action-' + cssId(tag));
    if (cell) {
      cell.innerHTML = \`<span class="status-badge err">Failed: \${escHtml(error ?? '')}</span>
        <button style="margin-left:6px" onclick="downloadModel('\${escAttr(tag)}')">Retry</button>\`;
    }
  } else {
    refresh(); // re-query to get updated installed list
  }
}

function updateCatalogRow(tag) {
  const dl = downloadState[tag];
  const cell = document.getElementById('cat-action-' + cssId(tag));
  if (!cell || !dl) return;
  cell.innerHTML = \`<div class="progress-row">
    <span class="progress-label">\${escHtml(dl.message)}</span>
    <div class="progress-bar-wrap">
      <div class="progress-bar" id="pb-\${cssId(tag)}" style="width:\${dl.percent}%"></div>
    </div>
  </div>\`;
}

// --------------------------------------------------------------------------
// Admin result
// --------------------------------------------------------------------------

function renderAdminResult(modified, skipped, previewed) {
  const el = document.getElementById('admin-result');
  let html = '';
  if (previewed.length > 0) {
    html += \`<div class="info-box" style="margin-top:10px">
      <strong>Previewed (diff opened for \${previewed.length}):</strong><br>
      \${previewed.map(f => \`<code>\${escHtml(f)}</code>\`).join('<br>')}
    </div>\`;
  }
  if (modified.length > 0) {
    html += \`<div class="info-box ok" style="margin-top:10px">
      <strong>Updated (\${modified.length}):</strong><br>
      \${modified.map(f => \`<code>\${escHtml(f)}</code>\`).join('<br>')}
    </div>\`;
  } else {
    html += '<div class="info-box" style="margin-top:10px">All config files already up to date.</div>';
  }
  if (skipped.length > 0) {
    html += \`<div class="info-box warn" style="margin-top:8px">
      <strong>Skipped:</strong><br>
      \${skipped.map(s => \`<code>\${escHtml(s)}</code>\`).join('<br>')}
    </div>\`;
  }
  el.innerHTML = html;
}

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

function escHtml(s) {
  return String(s ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}
function escAttr(s) { return escHtml(s); }
function cssId(tag) { return tag.replace(/[^a-z0-9]/gi, '_'); }
</script>
</body>
</html>`;
  }
}
