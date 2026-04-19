> **Hinweis:** API-Signaturen gegen aktuelle Endpunkte/Typen prüfen. Abweichungen mit `<!-- TODO: verify API -->` markieren.

# VSCode Integration Guide

Dieser Guide erklärt, wie Sie die ThemisDB VSCode Extension installieren, konfigurieren und verwenden.

## 📋 Inhaltsverzeichnis

1. [Extension Overview](#extension-overview)
2. [Installation](#installation)
3. [Konfiguration](#konfiguration)
4. [Features](#features)
5. [Extension Development](#extension-development)
6. [API Integration](#api-integration)

## 1. Extension Overview

Die ThemisDB VSCode Extension bietet nahtlose Integration zwischen Ihrer IDE und der ThemisDB Coding Platform.

### Features

- **Snippet Browser** - TreeView mit hierarchischer Snippet-Organisation
- **Semantic Search** - Code suchen mit natürlicher Sprache
- **Quick Insert** - Snippets mit einem Klick einfügen
- **Context-Aware Suggestions** - Intelligente Vorschläge basierend auf aktuellem Code
- **Snippet Sync** - Lokale Snippets mit ThemisDB synchronisieren
- **Multi-Language Support** - Python, JavaScript, TypeScript, Java, C++, Go, Rust, etc.

### Architektur

```
┌─────────────────────────────────────┐
│         VSCode Extension            │
│                                     │
│  ┌──────────────────────────────┐  │
│  │     Snippet TreeView         │  │
│  │  - Languages                 │  │
│  │    - Python                  │  │
│  │      - FastAPI               │  │
│  │        - JWT Auth            │  │
│  └──────────────────────────────┘  │
│                                     │
│  ┌──────────────────────────────┐  │
│  │     Command Palette          │  │
│  │  - Search Snippets           │  │
│  │  - Insert Snippet            │  │
│  │  - Save Snippet              │  │
│  │  - Sync                      │  │
│  └──────────────────────────────┘  │
│                                     │
│  ┌──────────────────────────────┐  │
│  │     CodeLens Provider        │  │
│  │  - Similar Code              │  │
│  │  - Refactor Suggestions      │  │
│  └──────────────────────────────┘  │
└──────────────┬──────────────────────┘
               │ HTTP REST API
               │
      ┌────────▼────────┐
      │  ThemisDB API   │
      │  Port: 8080     │
      └─────────────────┘
```

## 2. Installation

### Voraussetzungen

```bash
# Node.js 16+ und npm
node --version  # v16.0.0 or higher
npm --version   # 8.0.0 or higher

# VSCode 1.80+
code --version
```

### Methode 1: Aus Quellcode

```bash
# Repository klonen
cd examples/21_coding_platform/vscode_extension

# Dependencies installieren
npm install

# Extension kompilieren
npm run compile

# In VSCode laden
# Drücken Sie F5 in VSCode im extension-Ordner
# Oder: Extension Developer Window öffnen
```

### Methode 2: VSIX Package

```bash
# Extension paketieren
npm run package

# Installieren
code --install-extension themisdb-snippets-0.0.1.vsix
```

### Methode 3: Marketplace (zukünftig)

```bash
# Wenn veröffentlicht
code --install-extension themisdb.themisdb-snippets
```

### Verifizierung

Nach Installation:
1. VSCode neu starten
2. `Ctrl+Shift+P` → "ThemisDB" eingeben
3. Sie sollten ThemisDB-Befehle sehen
4. ThemisDB Icon in Activity Bar sollte sichtbar sein

## 3. Konfiguration

### Extension Settings

**Öffnen**: `Ctrl+,` → "ThemisDB" suchen

**Verfügbare Einstellungen**:

```json
{
  // ThemisDB Server URL
  "themisdb.apiUrl": "http://localhost:8080",
  
  // API Token (optional, für private Snippets)
  "themisdb.apiToken": "",
  
  // Auto-Sync beim Start
  "themisdb.autoSync": true,
  
  // Sync-Intervall (Minuten)
  "themisdb.syncInterval": 30,
  
  // Suggestions aktivieren
  "themisdb.suggestOnType": true,
  
  // Minimum Similarity für Suggestions
  "themisdb.minSimilarity": 0.7,
  
  // Max Anzahl Suggestions
  "themisdb.maxSuggestions": 5,
  
  // CodeLens aktivieren
  "themisdb.enableCodeLens": true,
  
  // Sprachen für Suggestions
  "themisdb.enabledLanguages": [
    "python",
    "javascript",
    "typescript",
    "java",
    "cpp",
    "go",
    "rust"
  ],
  
  // Log Level
  "themisdb.logLevel": "info",  // debug, info, warn, error
  
  // Cache aktivieren
  "themisdb.enableCache": true,
  
  // Cache-Größe (MB)
  "themisdb.cacheSize": 100
}
```

### Settings.json

**User Settings** (`~/.config/Code/User/settings.json`):
```json
{
  "themisdb.apiUrl": "http://localhost:8080",
  "themisdb.autoSync": true,
  "themisdb.suggestOnType": true
}
```

**Workspace Settings** (`.vscode/settings.json`):
```json
{
  "themisdb.apiUrl": "http://company-themisdb:8080",
  "themisdb.apiToken": "${env:THEMISDB_TOKEN}"
}
```

### Keybindings anpassen

**Öffnen**: `Ctrl+K Ctrl+S` → "ThemisDB" suchen

**Standard Keybindings**:
```json
[
  {
    "key": "ctrl+shift+t s",
    "command": "themisdb.searchSnippets",
    "when": "editorTextFocus"
  },
  {
    "key": "ctrl+shift+t i",
    "command": "themisdb.insertSnippet",
    "when": "editorTextFocus"
  },
  {
    "key": "ctrl+shift+t n",
    "command": "themisdb.saveSnippet",
    "when": "editorTextFocus && editorHasSelection"
  },
  {
    "key": "ctrl+shift+t f",
    "command": "themisdb.findSimilar",
    "when": "editorTextFocus && editorHasSelection"
  }
]
```

**Custom Keybindings** (`keybindings.json`):
```json
[
  {
    "key": "alt+s",
    "command": "themisdb.searchSnippets"
  }
]
```

## 4. Features

### Snippet Browser (TreeView)

**Öffnen**: Klicken Sie auf ThemisDB Icon in Activity Bar

**Struktur**:
```
📁 ThemisDB Snippets
  📁 Python (234)
    📁 FastAPI (45)
      📄 JWT Authentication
      📄 Async Endpoints
      📄 Database Connection
    📁 Django (67)
      📄 Custom Middleware
      📄 Model Serializer
    📁 General (122)
      📄 File Reading
      📄 Async HTTP Request
  📁 JavaScript (189)
    📁 React (89)
      📄 Custom Hook
      📄 Context Provider
    📁 Node.js (100)
      📄 Express Middleware
      📄 MongoDB Connection
  📁 TypeScript (145)
    📁 NestJS (45)
    📁 General (100)
```

**Aktionen**:
- **Klick**: Preview anzeigen
- **Doppelklick**: Code einfügen
- **Rechtsklick**:
  - Insert at Cursor
  - Copy to Clipboard
  - View Details
  - Edit Online
  - Delete

**Suche in TreeView**:
- Type-to-search aktiviert
- Filter nach Sprache
- Filter nach Tags
- Sortierung (Name, Date, Popularity)

### Command Palette

**Verfügbare Commands** (`Ctrl+Shift+P`):

#### `ThemisDB: Search Snippets`
Suche nach Snippets mit natürlicher Sprache.

**Verwendung**:
1. Command ausführen
2. Suchbegriff eingeben: "async HTTP request"
3. Snippet aus Liste auswählen
4. Snippet wird an Cursor eingefügt

**Quick Pick Options**:
```
> async http request python

  🔍 Results (5):
  
  1. ⭐ Async HTTP with aiohttp (95% match)
     import aiohttp; async def fetch(url): ...
  
  2. ⭐ Concurrent requests with asyncio (87% match)
     import asyncio, requests; async def fetch_all(): ...
  
  3. ⚡ FastAPI async endpoint (82% match)
     from fastapi import FastAPI; @app.get("/async") ...
  
  4. 📦 httpx async client (78% match)
     import httpx; async def fetch(url): ...
  
  5. 🌐 requests-async wrapper (72% match)
     import requests_async as requests; ...
```

#### `ThemisDB: Insert Snippet`
Snippet direkt einfügen (mit Snippet-ID oder Name).

#### `ThemisDB: Save Snippet`
Markierten Code als neues Snippet speichern.

**Verwendung**:
1. Code im Editor markieren
2. Command ausführen
3. Formular ausfüllen:
   ```
   Title: JWT Authentication Decorator
   Description: Decorator for protecting routes with JWT
   Tags: auth, jwt, decorator, fastapi
   Language: (auto-detected: python)
   Framework: FastAPI
   Visibility: public / private
   ```
4. Enter → Snippet wird zu ThemisDB hochgeladen

#### `ThemisDB: Find Similar Code`
Ähnlichen Code zu markiertem Code finden.

**Verwendung**:
1. Code markieren
2. Command ausführen
3. Ähnliche Snippets werden angezeigt
4. Snippet auswählen zum Vergleichen

**Side-by-Side Vergleich**:
```
┌─────────────────────────────┬─────────────────────────────┐
│ Your Code                   │ Similar Snippet (87%)       │
├─────────────────────────────┼─────────────────────────────┤
│ async def download(url):    │ async def fetch_file(url):  │
│     async with session.get  │     async with aiohttp...   │
│         (url) as resp:      │         .get(url) as r:     │
│         return await resp   │         content = await r   │
│             .read()         │             .read()         │
│                            │         return content      │
└─────────────────────────────┴─────────────────────────────┘
```

#### `ThemisDB: Sync Snippets`
Lokale Snippets mit ThemisDB Server synchronisieren.

#### `ThemisDB: Refresh`
TreeView aktualisieren.

### CodeLens Provider

**Was ist CodeLens?**
Inline-Links über Funktionen/Klassen mit kontextbezogenen Aktionen.

**Beispiel**:
```python
# [👁️ 5 similar] [🔄 Refactor suggestion]
async def fetch_data(url: str) -> dict:
    async with aiohttp.ClientSession() as session:
        async with session.get(url) as resp:
            return await resp.json()
```

**Klick auf "5 similar"**:
- Öffnet Quick Pick mit ähnlichen Implementierungen
- Zeigt Unterschiede und Best Practices

**Klick auf "Refactor suggestion"**:
- Schlägt Verbesserungen vor
- Zeigt Code-Review von ThemisDB Community

### Auto-Suggestions (IntelliSense)

**Aktivierung**: `themisdb.suggestOnType: true`

**Wie es funktioniert**:
1. Sie schreiben Code
2. Extension analysiert Kontext
3. Semantic Search findet relevante Snippets
4. Suggestions erscheinen in Completion List

**Beispiel**:

Sie schreiben:
```python
# JWT authenti
```

**IntelliSense zeigt**:
```
💡 Suggestions:

  🔐 JWT Authentication Decorator (ThemisDB)
      Decorator for protecting FastAPI routes
      
      def require_auth(token: str = Depends(oauth2_scheme)):
          ...
  
  🔐 JWT Token Validation (ThemisDB)
      Validate and decode JWT tokens
      
      def verify_token(token: str) -> dict:
          ...
```

**Tab/Enter zum Einfügen**

### Context Menu

**Rechtsklick im Editor**:
```
Cut
Copy
Paste
───────────────────────
ThemisDB ▶
  Save as Snippet
  Find Similar Code
  Search in ThemisDB
  Insert Snippet...
```

### Status Bar

**Zeigt**:
- Connection Status: ✅ Connected / ❌ Disconnected
- Sync Status: 🔄 Syncing... / ✓ Up to date
- Snippet Count: 📚 1,234 snippets

**Klick**: Öffnet Quick Actions

## 5. Extension Development

### Projekt-Struktur

```
vscode_extension/
├── package.json              # Extension Manifest
├── tsconfig.json             # TypeScript Config
├── .vscodeignore            # Publish Exclusions
├── README.md                # Extension Docs
├── CHANGELOG.md             # Version History
├── src/
│   ├── extension.ts         # Main Entry Point
│   ├── commands/
│   │   ├── searchSnippets.ts
│   │   ├── insertSnippet.ts
│   │   ├── saveSnippet.ts
│   │   └── findSimilar.ts
│   ├── providers/
│   │   ├── snippetTreeProvider.ts
│   │   ├── codeLensProvider.ts
│   │   └── completionProvider.ts
│   ├── api/
│   │   ├── client.ts        # ThemisDB API Client
│   │   └── types.ts         # Type Definitions
│   ├── ui/
│   │   ├── quickPick.ts
│   │   └── webview.ts
│   └── utils/
│       ├── cache.ts
│       ├── logger.ts
│       └── similarity.ts
└── test/
    ├── suite/
    └── runTest.ts
```

### Extension Entry Point

**src/extension.ts**:
```typescript
import * as vscode from 'vscode';
import { ThemisDBClient } from './api/client';
import { SnippetTreeProvider } from './providers/snippetTreeProvider';
import { CodeLensProvider } from './providers/codeLensProvider';
import { CompletionProvider } from './providers/completionProvider';

export function activate(context: vscode.ExtensionContext) {
    console.log('ThemisDB extension activated');
    
    // Configuration
    const config = vscode.workspace.getConfiguration('themisdb');
    const apiUrl = config.get<string>('apiUrl', 'http://localhost:8080');
    const apiToken = config.get<string>('apiToken', '');
    
    // API Client
    const client = new ThemisDBClient(apiUrl, apiToken);
    
    // TreeView Provider
    const treeProvider = new SnippetTreeProvider(client);
    vscode.window.registerTreeDataProvider('themisdbSnippets', treeProvider);
    
    // CodeLens Provider
    const codeLensProvider = new CodeLensProvider(client);
    vscode.languages.registerCodeLensProvider('*', codeLensProvider);
    
    // Completion Provider
    const completionProvider = new CompletionProvider(client);
    vscode.languages.registerCompletionItemProvider(
        { scheme: 'file' },
        completionProvider,
        '.'
    );
    
    // Commands
    context.subscriptions.push(
        vscode.commands.registerCommand('themisdb.searchSnippets', () => {
            searchSnippets(client);
        })
    );
    
    context.subscriptions.push(
        vscode.commands.registerCommand('themisdb.saveSnippet', () => {
            saveSnippet(client);
        })
    );
    
    // Auto-sync
    if (config.get<boolean>('autoSync', true)) {
        setInterval(() => {
            treeProvider.refresh();
        }, config.get<number>('syncInterval', 30) * 60 * 1000);
    }
}

export function deactivate() {
    console.log('ThemisDB extension deactivated');
}
```

### API Client

**src/api/client.ts**:
```typescript
import axios, { AxiosInstance } from 'axios';

export interface CodeSnippet {
    id: string;
    title: string;
    code: string;
    language: string;
    tags: string[];
    metadata: any;
}

export class ThemisDBClient {
    private api: AxiosInstance;
    
    constructor(baseURL: string, token?: string) {
        this.api = axios.create({
            baseURL,
            headers: token ? { 'Authorization': `Bearer ${token}` } : {}
        });
    }
    
    async searchSnippets(query: string, language?: string): Promise<CodeSnippet[]> {
        const response = await this.api.get('/api/snippets/search', {
            params: { q: query, language }
        });
        return response.data;
    }
    
    async getSnippet(id: string): Promise<CodeSnippet> {
        const response = await this.api.get(`/api/snippets/${id}`);
        return response.data;
    }
    
    async createSnippet(snippet: Partial<CodeSnippet>): Promise<CodeSnippet> {
        const response = await this.api.post('/api/snippets', snippet);
        return response.data;
    }
    
    async findSimilar(code: string, language: string): Promise<CodeSnippet[]> {
        const response = await this.api.post('/api/snippets/similar', {
            code,
            language
        });
        return response.data;
    }
    
    async getSnippetsByLanguage(language: string): Promise<CodeSnippet[]> {
        const response = await this.api.get('/api/snippets', {
            params: { language }
        });
        return response.data;
    }
}
```

### TreeView Provider

**src/providers/snippetTreeProvider.ts**:
```typescript
import * as vscode from 'vscode';
import { ThemisDBClient } from '../api/client';

export class SnippetTreeProvider implements vscode.TreeDataProvider<SnippetItem> {
    private _onDidChangeTreeData = new vscode.EventEmitter<SnippetItem | undefined>();
    readonly onDidChangeTreeData = this._onDidChangeTreeData.event;
    
    constructor(private client: ThemisDBClient) {}
    
    refresh(): void {
        this._onDidChangeTreeData.fire(undefined);
    }
    
    getTreeItem(element: SnippetItem): vscode.TreeItem {
        return element;
    }
    
    async getChildren(element?: SnippetItem): Promise<SnippetItem[]> {
        if (!element) {
            // Root level: languages
            return this.getLanguages();
        } else if (element.type === 'language') {
            // Language level: frameworks or snippets
            return this.getFrameworks(element.label as string);
        } else if (element.type === 'framework') {
            // Framework level: snippets
            return this.getSnippets(element.language!, element.label as string);
        }
        return [];
    }
    
    private async getLanguages(): Promise<SnippetItem[]> {
        // Implementation
        const languages = ['Python', 'JavaScript', 'TypeScript', 'Java'];
        return languages.map(lang => new SnippetItem(
            lang,
            'language',
            vscode.TreeItemCollapsibleState.Collapsed
        ));
    }
    
    private async getFrameworks(language: string): Promise<SnippetItem[]> {
        // Implementation
        return [];
    }
    
    private async getSnippets(language: string, framework: string): Promise<SnippetItem[]> {
        const snippets = await this.client.getSnippetsByLanguage(language);
        return snippets.map(s => new SnippetItem(
            s.title,
            'snippet',
            vscode.TreeItemCollapsibleState.None,
            s
        ));
    }
}

class SnippetItem extends vscode.TreeItem {
    constructor(
        public readonly label: string,
        public readonly type: 'language' | 'framework' | 'snippet',
        public readonly collapsibleState: vscode.TreeItemCollapsibleState,
        public readonly snippet?: any,
        public readonly language?: string
    ) {
        super(label, collapsibleState);
        
        if (type === 'snippet') {
            this.command = {
                command: 'themisdb.insertSnippet',
                title: 'Insert Snippet',
                arguments: [snippet]
            };
        }
    }
}
```

### Build & Package

**package.json Scripts**:
```json
{
  "scripts": {
    "compile": "tsc -p ./",
    "watch": "tsc -watch -p ./",
    "pretest": "npm run compile",
    "test": "node ./out/test/runTest.js",
    "package": "vsce package",
    "publish": "vsce publish"
  }
}
```

**Build**:
```bash
npm run compile
```

**Watch Mode** (für Development):
```bash
npm run watch
```

**Package**:
```bash
npm run package
# Erstellt themisdb-snippets-0.0.1.vsix
```

## 6. API Integration

### REST API Endpoints

**Base URL**: `http://localhost:8080/api`

#### Snippets

```typescript
// Search snippets
GET /snippets/search?q=async+http&language=python

// Get snippet by ID
GET /snippets/:id

// Create snippet
POST /snippets
Body: {
  title: string,
  code: string,
  language: string,
  tags: string[]
}

// Update snippet
PUT /snippets/:id

// Delete snippet
DELETE /snippets/:id

// Find similar
POST /snippets/similar
Body: {
  code: string,
  language: string,
  limit: number
}

// Get by language
GET /snippets?language=python&framework=fastapi
```

### WebSocket (Real-time Updates)

```typescript
import { io } from 'socket.io-client';

const socket = io('http://localhost:8080');

// Subscribe to snippet updates
socket.on('snippet:created', (snippet) => {
    console.log('New snippet:', snippet);
    treeProvider.refresh();
});

socket.on('snippet:updated', (snippet) => {
    console.log('Updated snippet:', snippet);
});

socket.on('snippet:deleted', (id) => {
    console.log('Deleted snippet:', id);
});
```

---

## 📚 Weitere Ressourcen

- [VSCode Extension API](https://code.visualstudio.com/api)
- [TreeView API](https://code.visualstudio.com/api/extension-guides/tree-view)
- [CodeLens Provider](https://code.visualstudio.com/api/language-extensions/programmatic-language-features#codelens)
- [ThemisDB API Docs](../../docs/api/)

**Fragen?** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
