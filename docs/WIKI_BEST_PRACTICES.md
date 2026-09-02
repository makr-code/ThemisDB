# GitHub Wiki Best Practices Guide

This guide provides recommendations and best practices for generating and designing a high-quality GitHub Wiki using the ThemisDB wiki generation pipeline.

## Table of Contents
1. [Architecture & Navigation](#architecture--navigation)
2. [Content Organization](#content-organization)
3. [Writing & Formatting](#writing--formatting)
4. [Maintenance & Governance](#maintenance--governance)
5. [Performance & Accessibility](#performance--accessibility)

---

## Architecture & Navigation

### 1. Breadcrumb Navigation
**Why:** Breadcrumbs help users understand their location within the wiki hierarchy and provide quick navigation back to parent categories.

**Best Practice:**
- Enable breadcrumbs on all pages except top-level hub pages (Home, Wiki-Index, Module-Index)
- Breadcrumbs should follow a consistent pattern: `Home > Category > Subcategory > Current Page`
- Use category index pages as breadcrumb targets (e.g., `Module-Index`, `Tutorial-Index`)
- Breadcrumbs should always link to valid wiki pages

**Implementation:**
```bash
# Enable breadcrumbs (default behavior)
python scripts/build_wiki.py --output /tmp/wiki-staging --enable-breadcrumbs

# Disable breadcrumbs if not desired
python scripts/build_wiki.py --output /tmp/wiki-staging --disable-breadcrumbs
```

### 2. Hierarchical Organization
**Why:** A clear hierarchy reduces cognitive load and helps users find information quickly.

**Structure:**
```
📚 Getting Started (Home, Quickstart, Setup, FAQ, Quick-Reference)
📖 Learning (Tutorials, Guides, Examples, Training)
🔧 API & Integration (API Reference, AQL, SDKs, Clients)
🏗️ Architecture (Design docs, ADRs, Module architecture, Design patterns)
⚙️ Operations (Deployment, Docker, Kubernetes, Scaling, Runbooks)
🔒 Security (Security policy, Hardening, Access control, Encryption)
🛠️ Development (Developer resources, Build guide, Contributing)
📋 Governance (Roadmap, Changelog, Release strategy, Versioning)
```

### 3. Sidebar Navigation
**Best Practice:**
- Keep sidebar to 3-4 levels deep maximum for readability
- Use thematic grouping rather than alphabetical ordering
- Link to category index pages that then list subcategories
- Update sidebar only during wiki rebuild (automated)

### 4. Cross-References & Link Strategy
**Best Practice:**
- Use [[Wiki Page Name]] links for internal wiki references (auto-converted from relative MD links)
- Link to category index pages for browsing related content
- Avoid orphaned pages by ensuring all pages appear in at least one navigation path
- Use link validation in the build pipeline to catch broken references

---

## Content Organization

### 5. Document Currency & Freshness
**Why:** Outdated documentation confuses users and erodes trust.

**Best Practice:**
- Keep modification dates in document headers or metadata:
  ```markdown
  # Document Title
  Last Modified: 2026-09-02
  Status: Current
  ```
- Sort documents by currency during wiki generation for visibility of recent updates:
  ```bash
  python scripts/build_wiki.py --output /tmp/wiki-staging --sort-by currency
  ```
- Review and update documents at least quarterly
- Archive outdated content explicitly (move to a separate "Legacy" section)

**Currency Scoring:**
- 0-7 days old: High priority (90-100 points)
- 7-30 days old: Medium-high priority (70-90 points)
- 30-90 days old: Medium priority (50-70 points)
- 90+ days old: Low priority (0-50 points)

### 6. Module Documentation
**Structure:** Each module SHOULD have:
- `ROADMAP.md` — Current status, implementation phases, acceptance criteria
- `ARCHITECTURE.md` — Design decisions, internal structure
- `CHANGELOG.md` — Version history and breaking changes
- `FUTURE_ENHANCEMENTS.md` — Planned features and research

**Wiki Mapping:**
- `src/<module>/ROADMAP.md` → `Module-<module>-Roadmap.md`
- `src/<module>/ARCHITECTURE.md` → `Module-<module>-Architecture.md`
- `src/<module>/CHANGELOG.md` → `Module-<module>-Changelog.md`
- `src/<module>/FUTURE_ENHANCEMENTS.md` → `Module-<module>-Future.md`

### 7. Index Pages
**Best Practice:**
- Create category index pages that list all pages in that category
- Example indexes:
  - `Module-Index` — All modules and their status
  - `Tutorial-Index` — All tutorials by topic
  - `Guide-Index` — All guides by category
  - `Wiki-Index` — Complete page directory
- Update indexes automatically during wiki rebuild

### 8. Audience-First Documentation
**Organization by Audience:**

| Audience | Entry Points | Recommended Pages |
|----------|-------------|-------------------|
| **New Users** | Home, Quickstart, Setup, Tutorial-* | Getting Started section |
| **API Users** | API-Reference, AQL-Reference, SDK-* | API & Integration section |
| **Operators** | Operations, Ops-*, Deployment | Operations & Deployment |
| **Contributors** | Contributing, Developer Resources, Build Guide | Development section |
| **Architects** | Architecture, ADRs, Module docs | Architecture section |
| **Security Teams** | Security-*, DSGVO-SOC2-Checklist | Security section |

---

## Writing & Formatting

### 9. Markdown Style & Consistency
**Best Practice:**
- Use consistent heading hierarchy: `#` = Page title, `##` = Major sections, `###` = Subsections
- Start with a summary paragraph (100-150 words) for discoverability
- Use code fences with language identifiers for syntax highlighting
- Remove badges and CI status indicators (cleaned by build pipeline)

**Example Structure:**
```markdown
# Page Title

Brief 1-2 sentence summary of what this page covers.

## Overview
More detailed explanation of the topic.

## Key Concepts
### Concept 1
Description...

### Concept 2
Description...

## Implementation Guide
Step-by-step instructions...

## Troubleshooting
Common issues and solutions...

## Related Pages
- [[Related-Page-1|Related-Page-1]]
- [[Related-Page-2|Related-Page-2]]
```

### 10. Status Badges & Lifecycle
**Best Practice:**
- Include clear status indicators in module roadmaps:
  - `[x]` Complete
  - `[~]` In progress
  - `[ ]` Planned
  - `[I]` Issue filed
  - `[P]` Pull request open
  - `[?]` Blocked / needs clarification

**Example:**
```markdown
## Current Status
- [x] Phase 1: Core implementation (v1.0)
- [x] Phase 2: Error handling (v1.1)
- [~] Phase 3: Performance optimization (in progress)
- [ ] Phase 4: Advanced features (Q4 2026)
```

### 11. Code Examples & Snippets
**Best Practice:**
- Include minimal working examples for all APIs
- Use real-world scenarios, not toy examples
- Test examples against the actual codebase (where feasible)
- Link examples to source code references

**Pattern:**
```markdown
## Example: Basic Usage
```cpp
#include <themisdb/client.h>

int main() {
    // Initialize client
    auto client = themisdb::Client::create("localhost:5432");
    
    // Execute query
    auto result = client->query("SELECT * FROM documents");
    
    // Process results
    for (const auto& row : result) {
        std::cout << row.id << "\n";
    }
}
```

Refer to [examples/basic_usage.cpp](https://github.com/makr-code/ThemisDB/blob/develop/examples/basic_usage.cpp) for the complete implementation.
```

### 12. Diagrams & Visual Aids
**Best Practice:**
- Use inline diagrams for complex concepts (ASCII art, Mermaid, or referenced SVG)
- Provide alternative text descriptions for accessibility
- For large diagrams, create separate documentation files
- Keep diagrams up-to-date with code changes

---

## Maintenance & Governance

### 13. Community Guardrails (Fail-Closed)
**Why:** Prevents accidental disclosure of private implementation details and credentials.

**Blocked Patterns:**
- `plugins/private/` — Private plugin code paths
- `internal/private` — Internal private structures
- `private_api_key=` — Credentials and secrets
- `SECRET=` — Configuration secrets

**Best Practice:**
- The build pipeline automatically blocks files containing these patterns
- Use separate private repositories for private plugins
- Never commit credentials to any documentation files
- Run `scripts/build_wiki.py` with `--dry-run` to validate before publishing

### 14. Wiki Governance Workflow
**Pipeline Steps:**

1. **Local Development**
   ```bash
   # Validate locally before committing
   python scripts/build_wiki.py --output /tmp/wiki-staging --dry-run --sort-by currency
   ```

2. **Pre-Commit Checks**
   - Private content guardrail validation
   - Link validation (internal [[links]] and GitHub URLs)
   - Breadcrumb hierarchy consistency
   - Currency scoring

3. **Automated Publishing** (via `publish-wiki.yml`)
   - Runs nightly (3:00 AM UTC) or on manual dispatch
   - Builds staging, validates links, publishes to GitHub Wiki
   - Generates a workflow summary with page count and any warnings

4. **Post-Publish Verification**
   - Review wiki changes on GitHub
   - Verify breadcrumb navigation works
   - Check for any broken links

### 15. Versioning & Changelog
**Best Practice:**
- Link to specific repository versions in stable docs
- Use version-aware docs with fallback to `develop` branch for examples
- Include version info in Changelogs (e.g., "Since v1.8.0")
- Mark breaking changes clearly

**Pattern:**
```markdown
## Changed in v2.0.0
- **Breaking:** `old_api()` removed; use `new_api()` instead
- **Deprecated:** `legacy_method()` marked deprecated; will be removed in v2.1.0
- **Added:** New streaming API for large result sets
```

### 16. Documentation Audit Trail
**Best Practice:**
- Each generated wiki page includes an HTML comment header with:
  - Page name
  - Source file path
  - Generation timestamp
  - ThemisDB version
- Example:
  ```html
  <!-- wiki-page: Module-LLM-Roadmap | source: src/llm/ROADMAP.md | generated: 2026-09-02T05:04:07Z | themisdb: 1.9.0-beta -->
  ```
- Useful for tracking which wiki pages come from which source files

---

## Performance & Accessibility

### 17. Page Performance
**Best Practice:**
- Keep individual pages under 5,000 words (split into multiple pages if needed)
- Use section headers to enable ToC generation
- Avoid excessive nesting (max 4 levels)
- GitHub Wiki rendering is fast, but dense pages are harder to navigate

### 18. Accessibility
**Best Practice:**
- Use semantic markdown (`#` headings, not `**bold** heading`)
- Provide alt text for images/diagrams
- Use sufficient contrast in code blocks
- Avoid relying on color alone for information
- Test with keyboard navigation only

### 19. Mobile-Friendly Content
**Best Practice:**
- Code snippets: use monospace, limit line length to 80 chars where possible
- Tables: keep them narrow (3-5 columns max)
- Lists: avoid excessive nesting (max 3 levels)
- Links: use descriptive text, not "click here"

### 20. Search Optimization
**Best Practice:**
- Use descriptive page titles (H1) that include keywords
- Front-load important information in summary paragraphs
- Use consistent terminology (avoid synonyms for the same concept)
- Include a "Related Pages" section at the end
- Add keyword-rich metadata in source files (comments with `Last Modified`, `Tags`, etc.)

---

## Workflow Integration

### Publishing Your Wiki Changes

**Manual Dispatch (on-demand):**
```bash
# Via GitHub UI:
# 1. Go to Actions → Publish: GitHub Wiki
# 2. Click "Run workflow"
# 3. Choose options:
#    - dry_run: false (to publish) or true (to preview)
#    - enable_ai_enrichment: false (or true for LLM overviews)
#    - fail_on_broken_links: true (recommended)
```

**Automatic Publish (nightly):**
- Runs automatically at 03:00 UTC every day
- Publishes all changes to the GitHub Wiki
- Creates a commit with page count and trigger info

**Dry-Run Testing (local):**
```bash
python scripts/build_wiki.py \
  --output /tmp/wiki-staging \
  --repo-root . \
  --dry-run \
  --enable-breadcrumbs \
  --sort-by currency
```

---

## Troubleshooting

### Issue: Broken Wiki Links
**Solution:**
- Run link validation: `python scripts/validate_wiki_links.py --wiki-dir /tmp/wiki-staging`
- Check that all [[Page-Name]] references have corresponding .md files
- Use the dry-run mode to preview link warnings

### Issue: Private Content Blocking
**Solution:**
- Review the blocked files list in the workflow output
- Remove any credentials, secrets, or private paths
- Ensure all private plugin references are removed
- Re-run the build

### Issue: Missing Category Index
**Solution:**
- Breadcrumbs reference category indexes (e.g., `Module-Index`)
- These are auto-generated; ensure at least one page uses the category prefix
- Check `_BREADCRUMB_HIERARCHY` in `build_wiki.py` for defined categories

### Issue: Document Not Appearing in Wiki
**Solution:**
- Verify file path matches the mapping in `_collect_entries()` in `build_wiki.py`
- Check that file doesn't contain private patterns
- Ensure file extension is `.md` (or `.ebnf` for AQL grammar)
- Run with `--dry-run` to see if file is collected

---

## Configuration Reference

### build_wiki.py Options
```bash
python scripts/build_wiki.py \
  --output DIR                      # Output directory (required)
  --repo-root DIR                   # Repository root (default: .)
  --dry-run                         # Preview only, don't write
  --enable-breadcrumbs              # Add breadcrumbs (default: enabled)
  --disable-breadcrumbs             # Disable breadcrumbs
  --sort-by {currency|modification-date|none}  # Sorting strategy (default: currency)
```

### Breadcrumb Categories (from _BREADCRUMB_HIERARCHY)
- **Getting Started** → Home, Quickstart, Setup, FAQ
- **Learning** → Tutorials, Guides, Examples
- **API & Integration** → API Reference, AQL, SDKs
- **Architecture** → Design docs, ADRs, Modules
- **Operations** → Deployment, Docker, Kubernetes
- **Security** → Security policy, Hardening, Access control
- **Development** → Developer resources, Build, Contributing
- **Governance** → Roadmap, Changelog, Release strategy

---

## Additional Resources

- [GitHub Wiki Documentation](https://docs.github.com/en/communities/documenting-your-project-with-wikis)
- [Markdown Syntax Guide](https://www.markdownguide.org/)
- [ThemisDB Contributing Guide](../CONTRIBUTING.md)
- [Repository Architecture](../ARCHITECTURE.md)

---

**Last Updated:** 2026-09-02  
**Status:** Active  
**Maintained By:** ThemisDB Engineering
