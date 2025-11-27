import * as vscode from 'vscode';

let serverUrl: string = 'http://localhost:8080';

export function activate(context: vscode.ExtensionContext) {
    console.log('RESPO extension activated');

    // Load configuration
    const config = vscode.workspace.getConfiguration('respo');
    serverUrl = config.get('serverUrl', 'http://localhost:8080');

    // Register commands
    context.subscriptions.push(
        vscode.commands.registerCommand('respo.chat', chatCommand),
        vscode.commands.registerCommand('respo.explain', explainCommand),
        vscode.commands.registerCommand('respo.review', reviewCommand),
        vscode.commands.registerCommand('respo.complete', completeCommand),
        vscode.commands.registerCommand('respo.research', researchCommand),
        vscode.commands.registerCommand('respo.setServer', setServerCommand),
    );

    // Configuration change listener
    context.subscriptions.push(
        vscode.workspace.onDidChangeConfiguration(e => {
            if (e.affectsConfiguration('respo.serverUrl')) {
                const config = vscode.workspace.getConfiguration('respo');
                serverUrl = config.get('serverUrl', 'http://localhost:8080');
            }
        })
    );
}

export function deactivate() {
    console.log('RESPO extension deactivated');
}

async function chatCommand() {
    const input = await vscode.window.showInputBox({
        prompt: 'Enter your question',
        placeHolder: 'How do I implement...',
    });

    if (!input) {
        return;
    }

    await sendRequest('/chat', { message: input });
}

async function explainCommand() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No active editor');
        return;
    }

    const selection = editor.selection;
    const code = editor.document.getText(selection);

    if (!code) {
        vscode.window.showErrorMessage('No code selected');
        return;
    }

    await sendRequest('/explain', { 
        code,
        language: editor.document.languageId,
    });
}

async function reviewCommand() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No active editor');
        return;
    }

    const selection = editor.selection;
    const code = editor.document.getText(selection);

    if (!code) {
        vscode.window.showErrorMessage('No code selected');
        return;
    }

    await sendRequest('/review', { 
        code,
        language: editor.document.languageId,
    });
}

async function completeCommand() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No active editor');
        return;
    }

    const position = editor.selection.active;
    const document = editor.document;
    const prefix = document.getText(new vscode.Range(new vscode.Position(0, 0), position));
    const suffix = document.getText(new vscode.Range(position, document.positionAt(document.getText().length)));

    await sendRequest('/complete', { 
        prefix,
        suffix,
        language: document.languageId,
    });
}

async function researchCommand() {
    const input = await vscode.window.showInputBox({
        prompt: 'Enter research query',
        placeHolder: 'Best practices for...',
    });

    if (!input) {
        return;
    }

    await sendMcpRequest('respo_research', { query: input });
}

async function setServerCommand() {
    const input = await vscode.window.showInputBox({
        prompt: 'Enter RESPO server URL',
        value: serverUrl,
    });

    if (input) {
        serverUrl = input;
        const config = vscode.workspace.getConfiguration('respo');
        await config.update('serverUrl', input, vscode.ConfigurationTarget.Global);
        vscode.window.showInformationMessage(`RESPO server set to: ${input}`);
    }
}

async function sendRequest(endpoint: string, body: object): Promise<void> {
    const outputChannel = vscode.window.createOutputChannel('RESPO');
    outputChannel.show();
    outputChannel.appendLine(`Request to ${endpoint}...`);

    try {
        const response = await fetch(`${serverUrl}${endpoint}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body),
        });

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }

        const data = await response.json() as Record<string, unknown>;
        outputChannel.appendLine('Response:');
        outputChannel.appendLine(JSON.stringify(data, null, 2));
    } catch (error) {
        outputChannel.appendLine(`Error: ${error}`);
        vscode.window.showErrorMessage(`RESPO request failed: ${error}`);
    }
}

async function sendMcpRequest(tool: string, args: object): Promise<void> {
    const outputChannel = vscode.window.createOutputChannel('RESPO');
    outputChannel.show();
    outputChannel.appendLine(`MCP request: ${tool}...`);

    try {
        const response = await fetch(`${serverUrl}/mcp`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                jsonrpc: '2.0',
                method: 'tools/call',
                params: {
                    name: tool,
                    arguments: args,
                },
                id: Date.now(),
            }),
        });

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }

        const data = await response.json() as Record<string, unknown>;
        outputChannel.appendLine('Response:');
        outputChannel.appendLine(JSON.stringify(data, null, 2));
    } catch (error) {
        outputChannel.appendLine(`Error: ${error}`);
        vscode.window.showErrorMessage(`RESPO MCP request failed: ${error}`);
    }
}
