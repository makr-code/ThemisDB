> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Quick Start Guide - ThemisDB Coding Platform

Get started with the ThemisDB Coding Platform in under 5 minutes!

## Prerequisites

- Python 3.8 or higher
- pip (Python package manager)
- Optional: Node.js 16+ (for VSCode extension)

## Installation

### 1. Install Python Dependencies

```bash
cd examples/21_coding_platform
pip install -r requirements.txt
```

**Note**: You can skip dependencies for a basic demo using the mock implementation:
```bash
# The app works without dependencies for basic functionality
# Install only requests if you want to test web scraping
pip install requests beautifulsoup4
```

### 2. Start the Application

```bash
python main.py
```

The application will start with a **mock ThemisDB client**, so it works without a running server!

## First Steps

### Create Your First Snippet

1. Click the **"Snippets"** tab
2. Click **"New"** button
3. Fill in the form:
   - **Title**: "Hello World in Python"
   - **Language**: "python"
   - **Code**: 
     ```python
     def hello():
         print("Hello, World!")
     
     if __name__ == "__main__":
         hello()
     ```
4. Click **"Save Snippet"**

### Search for Snippets

1. Click the **"Search"** tab
2. Enter a query: "hello world"
3. Press Enter or click **"Search"**
4. View results with similarity scores

### Try Web Scraping

1. Click the **"Web Scraper"** tab
2. Select **"GitHub Repository"**
3. Enter URL: `https://github.com/fastapi/fastapi`
4. Click **"Start Scraping"**
5. Watch the log for progress

**Note**: Web scraping uses mock implementation by default. For real scraping:
- Install full dependencies
- Set GitHub token: `export GITHUB_TOKEN=your_token`
- See [WEB_SCRAPING.md](WEB_SCRAPING.md) for details

## Using with Real ThemisDB Server

To connect to an actual ThemisDB server:

1. Start ThemisDB:
   ```bash
   docker run -d -p 8080:8080 -p 18765:18765 themisdb/themisdb:latest
   ```

2. Set environment variable:
   ```bash
   export USE_REAL_THEMISDB=true
   ```

3. Run the app:
   ```bash
   python main.py
   ```

## VSCode Extension Setup

### Quick Install

```bash
cd vscode_extension
npm install
npm run compile
```

Then in VSCode:
- Press `F5` to open Extension Development Host
- Or: Run `npm run package` and install the `.vsix` file

### Usage

1. **Search Snippets**: `Ctrl+Shift+P` → "ThemisDB: Search Snippets"
2. **Save Code**: Select code → Right-click → "ThemisDB: Save as Snippet"
3. **Find Similar**: Select code → Right-click → "ThemisDB: Find Similar Code"

See [VSCODE_INTEGRATION.md](VSCODE_INTEGRATION.md) for full details.

## Features Showcase

### 1. Semantic Search

The platform uses code embeddings for semantic search:

```
Query: "async http request python"
Results:
  1. Async HTTP with aiohttp (95% match)
  2. Concurrent requests (87% match)
  3. FastAPI endpoint (82% match)
```

### 2. Multi-Source Scraping

Collect code from:
- GitHub repositories
- Stack Overflow Q&A
- Documentation sites

### 3. VSCode Integration

- Browse snippets in sidebar
- Insert code with one click
- Context-aware suggestions

## Key Files

- **[README.md](README.md)** - Complete feature overview and documentation
- **[HOW_TO.md](HOW_TO.md)** - Detailed step-by-step guide
- **[WEB_SCRAPING.md](WEB_SCRAPING.md)** - Web scraping setup and usage
- **[VSCODE_INTEGRATION.md](VSCODE_INTEGRATION.md)** - VSCode extension guide
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - System design and architecture

## Troubleshooting

### "ModuleNotFoundError"

Install missing dependencies:
```bash
pip install -r requirements.txt
```

Or for basic demo (no ML features):
```bash
pip install requests beautifulsoup4
```

### "Connection refused" (when using real ThemisDB)

Make sure ThemisDB server is running:
```bash
docker ps | grep themisdb
curl http://localhost:8080/health
```

### VSCode Extension not loading

```bash
cd vscode_extension
rm -rf node_modules
npm install
npm run compile
```

## Next Steps

1. **Read Full Documentation**: Start with [README.md](README.md)
2. **Follow Tutorials**: Check [HOW_TO.md](HOW_TO.md)
3. **Set Up Web Scraping**: See [WEB_SCRAPING.md](WEB_SCRAPING.md)
4. **Install VSCode Extension**: Follow [VSCODE_INTEGRATION.md](VSCODE_INTEGRATION.md)
5. **Understand Architecture**: Read [ARCHITECTURE.md](ARCHITECTURE.md)

## Example Use Cases

### Personal Code Library

Collect and organize your own code snippets:
```python
# Save frequently used patterns
# Search with natural language
# Find similar implementations
```

### Team Knowledge Base

Share code knowledge in your team:
```python
# Import team projects
# Document internal standards
# Distribute VSCode extension
```

### Learning Platform

Learn from real-world code:
```python
# Scrape top GitHub repos
# Search by algorithm type
# Compare implementations
```

## Support

- **Issues**: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Documentation**: [ThemisDB Docs](../../docs/)

---

**Happy Coding!** 🚀
