/**
 * Minimal vscode API mock for unit tests that run outside the Extension Host.
 *
 * Only the APIs actually used by ContextManager, OllamaClient and
 * CopilotReviewer are implemented here.  Everything else is left as a
 * no-op stub so TypeScript types are satisfied.
 */

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

export enum DiagnosticSeverity {
  Error = 0,
  Warning = 1,
  Information = 2,
  Hint = 3,
}

// ---------------------------------------------------------------------------
// Value types
// ---------------------------------------------------------------------------

export class Position {
  constructor(
    public readonly line: number,
    public readonly character: number
  ) {}
}

export class Range {
  constructor(
    public readonly start: Position,
    public readonly end: Position
  ) {}
  get isEmpty(): boolean {
    return (
      this.start.line === this.end.line &&
      this.start.character === this.end.character
    );
  }
}

export class Selection extends Range {
  get active(): Position {
    return this.end;
  }
}

// ---------------------------------------------------------------------------
// Editor state (mutable between tests)
// ---------------------------------------------------------------------------

interface MockDiagnostic {
  severity: DiagnosticSeverity;
  range: Range;
  message: string;
}

interface MockEditor {
  fileName: string;
  languageId: string;
  selectedText: string;
  cursorLine: number;
  content: string;
  diagnostics: MockDiagnostic[];
}

let _editor: MockEditor | null = null;

export function setActiveEditor(e: MockEditor): void {
  _editor = e;
}

export function clearEditor(): void {
  _editor = null;
}

// ---------------------------------------------------------------------------
// vscode.window
// ---------------------------------------------------------------------------

export const window = {
  get activeTextEditor(): { document: { fileName: string; languageId: string; lineCount: number; getText(range?: Range): string; uri: string }; selection: Selection; } | undefined {
    if (!_editor) {
      return undefined;
    }
    const lines = _editor.content.split("\n");
    const doc = {
      fileName: _editor.fileName,
      languageId: _editor.languageId,
      lineCount: lines.length,
      uri: `file://${_editor.fileName}`,
      getText(range?: Range): string {
        if (!range) {
          return (_editor as MockEditor).content;
        }
        const start = range.start.line;
        const end = Math.min(range.end.line, lines.length - 1);
        return lines.slice(start, end + 1).join("\n");
      },
    };

    let selection: Selection;
    if (_editor.selectedText) {
      // Simulate selection on lines 0–0 with some text
      selection = new Selection(new Position(0, 0), new Position(0, _editor.selectedText.length));
      // Override getText for selection range
      const origGetText = doc.getText.bind(doc);
      const selectedText = _editor.selectedText;
      doc.getText = function (range?: Range): string {
        if (
          range &&
          range.start.line === 0 &&
          range.end.line === 0 &&
          range.end.character === selectedText.length
        ) {
          return selectedText;
        }
        return origGetText(range);
      };
    } else {
      const cl = _editor.cursorLine;
      selection = new Selection(new Position(cl, 0), new Position(cl, 0));
    }

    return { document: doc as never, selection };
  },
  showInformationMessage: () => Promise.resolve(undefined),
  showErrorMessage: () => Promise.resolve(undefined),
  showWarningMessage: () => Promise.resolve(undefined),
  showInputBox: () => Promise.resolve(undefined),
  createOutputChannel: () => ({ appendLine: () => undefined, show: () => undefined, dispose: () => undefined }),
  createStatusBarItem: () => ({
    text: "", tooltip: "", command: "", name: "",
    backgroundColor: undefined,
    show: () => undefined, dispose: () => undefined,
  }),
  withProgress: () => Promise.resolve(undefined),
};

// ---------------------------------------------------------------------------
// vscode.languages
// ---------------------------------------------------------------------------

export const languages = {
  getDiagnostics(uri: unknown): MockDiagnostic[] {
    if (!_editor) {
      return [];
    }
    // Match by file URI or return all diagnostics for the active file
    if (typeof uri === "string" && !uri.includes(_editor.fileName)) {
      return [];
    }
    return _editor.diagnostics;
  },
};

// ---------------------------------------------------------------------------
// vscode.workspace
// ---------------------------------------------------------------------------

export const workspace = {
  name: "TestWorkspace",
  getConfiguration: () => ({
    get: <T>(key: string, defaultValue: T) => defaultValue,
  }),
};

// ---------------------------------------------------------------------------
// vscode.lm (stub — not used in ContextManager tests)
// ---------------------------------------------------------------------------

export const lm = {
  selectChatModels: async () => [] as unknown[],
};

// ---------------------------------------------------------------------------
// vscode.chat (stub)
// ---------------------------------------------------------------------------

export const chat = {
  createChatParticipant: () => ({ iconPath: undefined, dispose: () => undefined }),
};

// ---------------------------------------------------------------------------
// vscode.commands (stub)
// ---------------------------------------------------------------------------

export const commands = {
  registerCommand: () => ({ dispose: () => undefined }),
  executeCommand: () => Promise.resolve(undefined),
};

// ---------------------------------------------------------------------------
// vscode.ProgressLocation (stub)
// ---------------------------------------------------------------------------

export const ProgressLocation = { Notification: 15 };

// ---------------------------------------------------------------------------
// vscode.LanguageModelChatMessage (stub)
// ---------------------------------------------------------------------------

export const LanguageModelChatMessage = {
  Assistant: (text: string) => ({ role: "assistant", content: text }),
  User: (text: string) => ({ role: "user", content: text }),
};

// ---------------------------------------------------------------------------
// vscode.Uri (stub)
// ---------------------------------------------------------------------------

export const Uri = {
  joinPath: (_base: unknown, ...parts: string[]) => parts.join("/"),
  file: (path: string) => ({ fsPath: path }),
};

// ---------------------------------------------------------------------------
// Default export for CommonJS interop
// ---------------------------------------------------------------------------

export default {
  DiagnosticSeverity,
  Position,
  Range,
  Selection,
  window,
  languages,
  workspace,
  lm,
  chat,
  commands,
  ProgressLocation,
  LanguageModelChatMessage,
  Uri,
  setActiveEditor,
  clearEditor,
};
