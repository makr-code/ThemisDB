# RESPO VS Code Extension

VS Code extension for the RESPO RAG LLM Coding Assistant with MCP integration.

## Features

- **Chat**: Ask coding questions with RAG-enhanced responses
- **Explain Code**: Get explanations for selected code
- **Review Code**: Get code review suggestions
- **Complete Code**: AI-powered code completion
- **Deep Research**: Research complex topics using agentic planning

## Installation

### From Source

```bash
cd vscode-extension
npm install
npm run compile
```

Then press F5 in VS Code to launch the extension in a new window.

### Package

```bash
npm install -g @vscode/vsce
vsce package
# Install the .vsix file in VS Code
```

## Configuration

Open VS Code settings and search for "RESPO":

- `respo.serverUrl`: RESPO server URL (default: `http://localhost:8080`)
- `respo.enableMcp`: Enable MCP integration (default: `true`)

## Commands

| Command | Description |
|---------|-------------|
| `RESPO: Chat` | Open chat interface |
| `RESPO: Explain Code` | Explain selected code |
| `RESPO: Review Code` | Review selected code |
| `RESPO: Complete Code` | Complete code at cursor |
| `RESPO: Deep Research` | Research a topic |
| `RESPO: Set Server URL` | Configure server URL |

## Context Menu

Right-click on selected code to access:
- Explain Code
- Review Code

## Requirements

- RESPO server running (default: `http://localhost:8080`)
- VS Code 1.85.0 or higher
