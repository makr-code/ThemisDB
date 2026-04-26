"use strict";
/**
 * Minimal vscode API mock for unit tests that run outside the Extension Host.
 *
 * Only the APIs actually used by ContextManager, OllamaClient and
 * CopilotReviewer are implemented here.  Everything else is left as a
 * no-op stub so TypeScript types are satisfied.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.Uri = exports.LanguageModelChatMessage = exports.ProgressLocation = exports.commands = exports.chat = exports.lm = exports.workspace = exports.languages = exports.window = exports.Selection = exports.Range = exports.Position = exports.DiagnosticSeverity = void 0;
exports.setActiveEditor = setActiveEditor;
exports.clearEditor = clearEditor;
// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------
var DiagnosticSeverity;
(function (DiagnosticSeverity) {
    DiagnosticSeverity[DiagnosticSeverity["Error"] = 0] = "Error";
    DiagnosticSeverity[DiagnosticSeverity["Warning"] = 1] = "Warning";
    DiagnosticSeverity[DiagnosticSeverity["Information"] = 2] = "Information";
    DiagnosticSeverity[DiagnosticSeverity["Hint"] = 3] = "Hint";
})(DiagnosticSeverity || (exports.DiagnosticSeverity = DiagnosticSeverity = {}));
// ---------------------------------------------------------------------------
// Value types
// ---------------------------------------------------------------------------
class Position {
    line;
    character;
    constructor(line, character) {
        this.line = line;
        this.character = character;
    }
}
exports.Position = Position;
class Range {
    start;
    end;
    constructor(start, end) {
        this.start = start;
        this.end = end;
    }
    get isEmpty() {
        return (this.start.line === this.end.line &&
            this.start.character === this.end.character);
    }
}
exports.Range = Range;
class Selection extends Range {
    get active() {
        return this.end;
    }
}
exports.Selection = Selection;
let _editor = null;
function setActiveEditor(e) {
    _editor = e;
}
function clearEditor() {
    _editor = null;
}
// ---------------------------------------------------------------------------
// vscode.window
// ---------------------------------------------------------------------------
exports.window = {
    get activeTextEditor() {
        if (!_editor) {
            return undefined;
        }
        const lines = _editor.content.split("\n");
        const doc = {
            fileName: _editor.fileName,
            languageId: _editor.languageId,
            lineCount: lines.length,
            uri: `file://${_editor.fileName}`,
            getText(range) {
                if (!range) {
                    return _editor.content;
                }
                const start = range.start.line;
                const end = Math.min(range.end.line, lines.length - 1);
                return lines.slice(start, end + 1).join("\n");
            },
        };
        let selection;
        if (_editor.selectedText) {
            // Simulate selection on lines 0–0 with some text
            selection = new Selection(new Position(0, 0), new Position(0, _editor.selectedText.length));
            // Override getText for selection range
            const origGetText = doc.getText.bind(doc);
            const selectedText = _editor.selectedText;
            doc.getText = function (range) {
                if (range &&
                    range.start.line === 0 &&
                    range.end.line === 0 &&
                    range.end.character === selectedText.length) {
                    return selectedText;
                }
                return origGetText(range);
            };
        }
        else {
            const cl = _editor.cursorLine;
            selection = new Selection(new Position(cl, 0), new Position(cl, 0));
        }
        return { document: doc, selection };
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
exports.languages = {
    getDiagnostics(uri) {
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
exports.workspace = {
    name: "TestWorkspace",
    getConfiguration: () => ({
        get: (key, defaultValue) => defaultValue,
    }),
};
// ---------------------------------------------------------------------------
// vscode.lm (stub — not used in ContextManager tests)
// ---------------------------------------------------------------------------
exports.lm = {
    selectChatModels: async () => [],
};
// ---------------------------------------------------------------------------
// vscode.chat (stub)
// ---------------------------------------------------------------------------
exports.chat = {
    createChatParticipant: () => ({ iconPath: undefined, dispose: () => undefined }),
};
// ---------------------------------------------------------------------------
// vscode.commands (stub)
// ---------------------------------------------------------------------------
exports.commands = {
    registerCommand: () => ({ dispose: () => undefined }),
    executeCommand: () => Promise.resolve(undefined),
};
// ---------------------------------------------------------------------------
// vscode.ProgressLocation (stub)
// ---------------------------------------------------------------------------
exports.ProgressLocation = { Notification: 15 };
// ---------------------------------------------------------------------------
// vscode.LanguageModelChatMessage (stub)
// ---------------------------------------------------------------------------
exports.LanguageModelChatMessage = {
    Assistant: (text) => ({ role: "assistant", content: text }),
    User: (text) => ({ role: "user", content: text }),
};
// ---------------------------------------------------------------------------
// vscode.Uri (stub)
// ---------------------------------------------------------------------------
exports.Uri = {
    joinPath: (_base, ...parts) => parts.join("/"),
    file: (path) => ({ fsPath: path }),
};
// ---------------------------------------------------------------------------
// Default export for CommonJS interop
// ---------------------------------------------------------------------------
exports.default = {
    DiagnosticSeverity,
    Position,
    Range,
    Selection,
    window: exports.window,
    languages: exports.languages,
    workspace: exports.workspace,
    lm: exports.lm,
    chat: exports.chat,
    commands: exports.commands,
    ProgressLocation: exports.ProgressLocation,
    LanguageModelChatMessage: exports.LanguageModelChatMessage,
    Uri: exports.Uri,
    setActiveEditor,
    clearEditor,
};
//# sourceMappingURL=vscode-mock.js.map