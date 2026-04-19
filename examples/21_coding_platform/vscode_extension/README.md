> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB VSCode Extension

VSCode extension for ThemisDB Coding Platform integration.

## Features

- **Snippet Browser**: TreeView with hierarchical organization
- **Semantic Search**: Natural language code search
- **Quick Insert**: Insert snippets with one click
- **Context Menu**: Save and find similar code
- **Auto-Suggestions**: IntelliSense integration

## Installation

### From Source

```bash
npm install
npm run compile
```

Press `F5` in VSCode to open Extension Development Host.

### From VSIX

```bash
npm run package
code --install-extension themisdb-snippets-0.0.1.vsix
```

## Configuration

Open settings (`Ctrl+,`) and search for "ThemisDB":

```json
{
  "themisdb.apiUrl": "http://localhost:8080",
  "themisdb.autoSync": true,
  "themisdb.suggestOnType": true
}
```

## Usage

### Command Palette

- `Ctrl+Shift+P` → `ThemisDB: Search Snippets`
- `Ctrl+Shift+P` → `ThemisDB: Save as Snippet` (with selection)
- `Ctrl+Shift+P` → `ThemisDB: Find Similar Code` (with selection)

### Sidebar

Click the ThemisDB icon in the Activity Bar to browse snippets.

### Context Menu

Right-click in editor:
- Save selection as snippet
- Find similar code

## Development

```bash
# Install dependencies
npm install

# Compile
npm run compile

# Watch mode
npm run watch

# Package
npm run package
```

## Requirements

- VSCode 1.80.0 or higher
- ThemisDB server running on http://localhost:8080

## License

MIT
