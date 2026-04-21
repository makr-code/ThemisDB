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
    async buildPrompt(userText, contextTokenBudget = 2048) {
        const editor = vscode.window.activeTextEditor;
        const parts = [];
        let activeLanguage = "plaintext";
        // Rough estimate: 1 token ~= 4 chars. Only context preamble is budgeted.
        const contextCharBudget = Math.max(256, contextTokenBudget * 4);
        if (editor) {
            const doc = editor.document;
            activeLanguage = doc.languageId;
            const contextSections = [];
            // Low-priority metadata (kept last when budget is tight).
            contextSections.push({
                priority: 1,
                text: `[Context]\nFile      : ${doc.fileName}\nLanguage  : ${doc.languageId}\nWorkspace : ${vscode.workspace.name ?? "(no workspace)"}`,
            });
            // Selected text
            const selection = editor.selection;
            if (!selection.isEmpty) {
                const selected = doc.getText(selection);
                contextSections.push({
                    priority: 4,
                    text: `\n[Selected code]\n\`\`\`${doc.languageId}\n${selected}\n\`\`\``,
                });
            }
            else {
                // Provide a window around the cursor (±30 lines) for context
                const cursor = selection.active;
                const startLine = Math.max(0, cursor.line - 30);
                const endLine = Math.min(doc.lineCount - 1, cursor.line + 30);
                const window = doc.getText(new vscode.Range(new vscode.Position(startLine, 0), new vscode.Position(endLine, Number.MAX_SAFE_INTEGER)));
                contextSections.push({
                    priority: 2,
                    text: `\n[Code near cursor (lines ${startLine + 1}–${endLine + 1})]\n\`\`\`${doc.languageId}\n${window}\n\`\`\``,
                });
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
                contextSections.push({
                    priority: 3,
                    text: `\n[Diagnostics]\n${diagText}`,
                });
            }
            const related = await this.collectRelatedContext(doc);
            if (related) {
                contextSections.push({
                    priority: 2,
                    text: related,
                });
            }
            // Keep highest-priority sections first when context exceeds budget.
            let usedChars = 0;
            const ordered = contextSections.sort((a, b) => b.priority - a.priority);
            for (const section of ordered) {
                if (usedChars >= contextCharBudget) {
                    break;
                }
                const remaining = contextCharBudget - usedChars;
                let text = section.text;
                if (text.length > remaining) {
                    if (remaining < 64) {
                        continue;
                    }
                    text = text.slice(0, remaining - 16) + "\n...[truncated]";
                }
                parts.push(text);
                usedChars += text.length;
            }
        }
        parts.push(`\n[User request]\n${userText}`);
        return {
            text: parts.join("\n"),
            activeLanguage,
        };
    }
    async collectRelatedContext(doc) {
        const ws = vscode.workspace;
        if (typeof ws.findFiles !== "function" || typeof ws.openTextDocument !== "function") {
            return undefined;
        }
        const currentText = doc.getText();
        const candidateNames = this.extractCandidateNames(currentText);
        if (candidateNames.length === 0) {
            return undefined;
        }
        const relatedUris = [];
        const seen = new Set();
        for (const name of candidateNames) {
            if (relatedUris.length >= 3) {
                break;
            }
            const escaped = name.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
            const glob = `**/*${escaped}*.*`;
            const found = await ws.findFiles(glob, "**/node_modules/**", 3);
            for (const uri of found) {
                const key = uri.toString();
                if (key === doc.uri.toString() || seen.has(key)) {
                    continue;
                }
                seen.add(key);
                relatedUris.push(uri);
                if (relatedUris.length >= 3) {
                    break;
                }
            }
        }
        if (relatedUris.length === 0) {
            return undefined;
        }
        const lines = ["", "[Related files context]"];
        for (const uri of relatedUris) {
            const relatedDoc = await ws.openTextDocument(uri);
            const summary = await this.summariseSymbols(uri);
            const maxLine = Math.min(49, Math.max(0, relatedDoc.lineCount - 1));
            const snippet = relatedDoc.getText(new vscode.Range(new vscode.Position(0, 0), new vscode.Position(maxLine, Number.MAX_SAFE_INTEGER)));
            lines.push(`- File: ${relatedDoc.fileName}`);
            if (summary.length > 0) {
                lines.push(`  Symbols: ${summary.join(", ")}`);
            }
            lines.push(`  Snippet:\n\`\`\`${relatedDoc.languageId}\n${snippet}\n\`\`\``);
        }
        return lines.join("\n");
    }
    async summariseSymbols(uri) {
        const cmds = vscode.commands;
        if (typeof cmds.executeCommand !== "function") {
            return [];
        }
        try {
            const symbols = await cmds.executeCommand("vscode.executeDocumentSymbolProvider", uri);
            if (!symbols || symbols.length === 0) {
                return [];
            }
            return symbols.slice(0, 6).map((s) => `${s.name}@${s.range.start.line + 1}`);
        }
        catch {
            return [];
        }
    }
    extractCandidateNames(content) {
        const results = new Set();
        const importRegex = /(?:import|from|require\()\s*["']([^"']+)["']/g;
        for (const match of content.matchAll(importRegex)) {
            const raw = match[1] ?? "";
            const clean = raw.split(/[\\/]/).pop() ?? "";
            const name = clean.replace(/\.[a-zA-Z0-9]+$/, "");
            if (name.length >= 2) {
                results.add(name);
            }
        }
        const includeRegex = /#include\s*[<"]([^">]+)[">]/g;
        for (const match of content.matchAll(includeRegex)) {
            const raw = match[1] ?? "";
            const clean = raw.split(/[\\/]/).pop() ?? "";
            const name = clean.replace(/\.[a-zA-Z0-9]+$/, "");
            if (name.length >= 2) {
                results.add(name);
            }
        }
        const classRegex = /\bclass\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?::\s*(?:public\s+)?([A-Za-z_][A-Za-z0-9_]*))?/g;
        for (const match of content.matchAll(classRegex)) {
            const own = match[1] ?? "";
            const base = match[2] ?? "";
            if (own.length >= 2) {
                results.add(own);
            }
            if (base.length >= 2) {
                results.add(base);
            }
        }
        return Array.from(results).slice(0, 12);
    }
}
exports.ContextManager = ContextManager;
//# sourceMappingURL=contextManager.js.map