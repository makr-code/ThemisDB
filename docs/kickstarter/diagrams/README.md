# ThemisDB Kickstarter Diagrams

Four key diagrams visualizing ThemisDB's architecture, capabilities, and certification roadmap.

## Files

- **01-problem-statement.mmd** - Mermaid source: Current AI infrastructure fragmentation problem
- **02-themisdb-unified.mmd** - Mermaid source: Unified ThemisDB solution with all 6 data models + native AI
- **03-ai-stack-integrated.mmd** - Mermaid source: Integrated AI stack and operational guarantees
- **04-donation-roadmap.mmd** - Mermaid source: Donation tiers mapped to certification milestones

- **01-problem-statement.html** - Interactive HTML version (view in browser, export to PNG)
- **02-themisdb-unified.html** - Interactive HTML version
- **03-ai-stack-integrated.html** - Interactive HTML version
- **04-donation-roadmap.html** - Interactive HTML version

## How to View & Export

### Option 1: HTML Viewer (Easiest)
1. Open any `.html` file in your web browser
2. Right-click on the diagram → "Open Image in New Tab" → Save as PNG
3. Alternative: Use browser screenshot tool (Ctrl+Shift+S in Chrome/Edge, Cmd+Shift+4 on Mac)

### Option 2: Mermaid CLI (Command Line)
```bash
# Install mermaid-cli if not already installed:
npm install -g @mermaid-js/mermaid-cli

# Convert a single diagram:
mmdc -i 01-problem-statement.mmd -o 01-problem-statement.png -s 2

# Convert all diagrams:
mmdc -i *.mmd -o ./png/
```

### Option 3: Mermaid Live Editor (Online)
1. Open https://mermaid.live
2. Copy-paste content from any `.mmd` file
3. Click "Export" → Download as PNG

## Technical Details

- **Format**: Mermaid diagram markup (.mmd files)
- **Themes**: Default (clean, professional)
- **Color Scheme**: 
  - Blue (#a8d8ea): Data models
  - Orange (#ffd6a5): AI components
  - Green (#c8e6c9): Governance/Compliance
  - Red (#d4a5a5): Problem/Warning states

## Integration

To embed diagrams in Kickstarter campaign pages, include:

```html
<script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>

<div class="mermaid">
[Paste diagram content from .mmd file]
</div>

<script>
  mermaid.initialize({ startOnLoad: true });
  mermaid.contentLoaded();
</script>
```

## Notes

- All diagrams are responsive and render correctly on desktop and mobile
- HTML files include export instructions for non-technical users
- Mermaid.js CDN is required for live rendering (diagrams won't render offline without local mermaid library)
- For static PDF/printing, export from HTML viewer in browser

---

*Generated as part of ThemisDB Kickstarter campaign materials*
*Last updated: 2025-Q1*
