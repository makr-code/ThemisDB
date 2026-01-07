/**
 * ThemisDB VSCode Extension
 * 
 * Main extension entry point.
 */

import * as vscode from 'vscode';

export function activate(context: vscode.ExtensionContext) {
    console.log('ThemisDB extension is now active');
    
    // Get configuration
    const config = vscode.workspace.getConfiguration('themisdb');
    const apiUrl = config.get<string>('apiUrl', 'http://localhost:8080');
    
    // Register commands
    const searchCommand = vscode.commands.registerCommand('themisdb.searchSnippets', async () => {
        const query = await vscode.window.showInputBox({
            prompt: 'Search for code snippets',
            placeHolder: 'e.g., async http request'
        });
        
        if (query) {
            vscode.window.showInformationMessage(`Searching for: ${query}`);
            // TODO: Implement actual search
        }
    });
    
    const saveCommand = vscode.commands.registerCommand('themisdb.saveSnippet', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            return;
        }
        
        const selection = editor.selection;
        const code = editor.document.getText(selection);
        
        if (!code) {
            vscode.window.showWarningMessage('Please select code to save');
            return;
        }
        
        const title = await vscode.window.showInputBox({
            prompt: 'Enter snippet title',
            placeHolder: 'My Awesome Snippet'
        });
        
        if (title) {
            vscode.window.showInformationMessage(`Saved snippet: ${title}`);
            // TODO: Implement actual save
        }
    });
    
    const similarCommand = vscode.commands.registerCommand('themisdb.findSimilar', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            return;
        }
        
        const selection = editor.selection;
        const code = editor.document.getText(selection);
        
        if (!code) {
            vscode.window.showWarningMessage('Please select code to find similar snippets');
            return;
        }
        
        vscode.window.showInformationMessage('Finding similar code...');
        // TODO: Implement actual search
    });
    
    const refreshCommand = vscode.commands.registerCommand('themisdb.refresh', () => {
        vscode.window.showInformationMessage('Refreshing snippets...');
        // TODO: Implement refresh
    });
    
    context.subscriptions.push(
        searchCommand,
        saveCommand,
        similarCommand,
        refreshCommand
    );
}

export function deactivate() {
    console.log('ThemisDB extension is now deactivated');
}
