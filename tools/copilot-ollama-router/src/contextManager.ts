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

import * as vscode from "vscode";

export interface EnrichedPrompt {
  /** Full prompt with context preamble. */
  text: string;
  /** Language identifier of the active file (e.g. "cpp", "typescript"). */
  activeLanguage: string;
}

export class ContextManager {
  /**
   * Builds an enriched prompt from the current editor state + raw user text.
   */
  buildPrompt(userText: string): EnrichedPrompt {
    const editor = vscode.window.activeTextEditor;
    const parts: string[] = [];

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
      } else {
        // Provide a window around the cursor (±30 lines) for context
        const cursor = selection.active;
        const startLine = Math.max(0, cursor.line - 30);
        const endLine = Math.min(doc.lineCount - 1, cursor.line + 30);
        const window = doc.getText(
          new vscode.Range(
            new vscode.Position(startLine, 0),
            new vscode.Position(endLine, Number.MAX_SAFE_INTEGER)
          )
        );
        parts.push(
          `\n[Code near cursor (lines ${startLine + 1}–${endLine + 1})]\n\`\`\`${doc.languageId}\n${window}\n\`\`\``
        );
      }

      // Diagnostics for the active file
      const diagnostics = vscode.languages
        .getDiagnostics(doc.uri)
        .filter(
          (d) =>
            d.severity === vscode.DiagnosticSeverity.Error ||
            d.severity === vscode.DiagnosticSeverity.Warning
        )
        .slice(0, 10); // limit to top-10 to keep prompt size reasonable

      if (diagnostics.length > 0) {
        const diagText = diagnostics
          .map((d) => {
            const sev =
              d.severity === vscode.DiagnosticSeverity.Error ? "ERROR" : "WARN";
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
