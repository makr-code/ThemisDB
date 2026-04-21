"use strict";
/**
 * Context Manager
 *
 * Enriches a raw user prompt with workspace context before it is sent to
 * either Ollama or Copilot:
 *
 *  - Active editor file path and language identifier
 *  - Selected text (if any)
 *  - Visible diagnostics (errors/warnings) for the active file
 *  - Workspace name
 *
 * All context is prepended as a structured preamble so the model has
 * relevant information without the user needing to paste it manually.
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
exports.ContextManager = void 0;
const vscode = __importStar(require("vscode"));
class ContextManager {
    /**
     * Builds an enriched prompt from the current editor state + raw user text.
     */
    buildPrompt(userText) {
        const editor = vscode.window.activeTextEditor;
        const parts = [];
        let activeLanguage = "plaintext";
        if (editor) {
            const doc = editor.document;
            activeLanguage = doc.languageId;
            parts.push(`[Context]`);
            parts.push(`File      : ${doc.fileName}`);
            parts.push(`Language  : ${doc.languageId}`);
            parts.push(`Workspace : ${vscode.workspace.name ?? "(no workspace)"}`);
            // Selected text
            const selection = editor.selection;
            if (!selection.isEmpty) {
                const selected = doc.getText(selection);
                parts.push(`\n[Selected code]\n\`\`\`${doc.languageId}\n${selected}\n\`\`\``);
            }
            else {
                // Provide a window around the cursor (±30 lines) for context
                const cursor = selection.active;
                const startLine = Math.max(0, cursor.line - 30);
                const endLine = Math.min(doc.lineCount - 1, cursor.line + 30);
                const window = doc.getText(new vscode.Range(new vscode.Position(startLine, 0), new vscode.Position(endLine, Number.MAX_SAFE_INTEGER)));
                parts.push(`\n[Code near cursor (lines ${startLine + 1}–${endLine + 1})]\n\`\`\`${doc.languageId}\n${window}\n\`\`\``);
            }
            // Diagnostics for the active file
            const diagnostics = vscode.languages
                .getDiagnostics(doc.uri)
                .filter((d) => d.severity === vscode.DiagnosticSeverity.Error ||
                d.severity === vscode.DiagnosticSeverity.Warning)
                .slice(0, 10); // limit to top-10 to keep prompt size reasonable
            if (diagnostics.length > 0) {
                const diagText = diagnostics
                    .map((d) => {
                    const sev = d.severity === vscode.DiagnosticSeverity.Error ? "ERROR" : "WARN";
                    return `  [${sev}] line ${d.range.start.line + 1}: ${d.message}`;
                })
                    .join("\n");
                parts.push(`\n[Diagnostics]\n${diagText}`);
            }
        }
        parts.push(`\n[User request]\n${userText}`);
        return {
            text: parts.join("\n"),
            activeLanguage,
        };
    }
}
exports.ContextManager = ContextManager;
//# sourceMappingURL=contextManager.js.map