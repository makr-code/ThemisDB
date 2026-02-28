# Changelog

All notable changes to the ThemisDB Wiki Integration plugin will be documented in this file.

## [1.0.0] - 2026-02-11

### Added
- **Custom Post Type**: Native WordPress wiki pages with custom post type `themisdb_wiki`
- **Markdown Editor**: SimpleMDE integration for rich markdown editing in WordPress admin
- **WikiLinks Support**: Full [[WikiLink]] syntax parser with multiple formats:
  - `[[Page Name]]` - Basic wiki links
  - `[[Page Name|Display Text]]` - Custom link text
  - `[[Page Name#Section]]` - Links to page sections
  - `[[Category:Name]]` - Category assignment
  - `[[File:image.png|options]]` - Image embedding with options
- **Table of Contents**: Auto-generated TOC for pages with 3+ headings
- **Version History**: Full revision management with diff viewer
- **GitHub Wiki Sync**: Bidirectional synchronization with GitHub Wiki
  - Push WordPress pages to GitHub
  - Pull GitHub wiki pages to WordPress
  - Bulk sync all pages
  - Manual and automatic sync modes
- **Full-Text Search**: Advanced search with live suggestions
  - Search titles, content, and markdown source
  - AJAX-powered live search suggestions
  - Related pages based on categories
- **Contributors Tracking**: Automatic tracking of page contributors
- **Backlinks**: Show pages that link to current page
- **Custom Taxonomy**: Wiki categories for organization
- **Responsive Design**: Mobile-friendly interface with Themis brand colors
- **Shortcodes**: Multiple shortcodes for flexible content display
  - `[themisdb_wiki_page]` - Embed wiki pages
  - `[themisdb_wiki_index]` - Show wiki index
  - `[themisdb_wiki_recent]` - Recent changes
  - `[themisdb_wiki_search]` - Search form
  - `[themisdb_wiki_toc]` - Table of contents
- **Frontend Templates**: Custom templates for single and archive pages
- **Admin Interface**: Comprehensive settings page with sync controls

### Technical Features
- **Object-Oriented Architecture**: Clean class-based structure
- **Separation of Concerns**: Distinct classes for wiki, wikilinks, versions, sync, search, and admin
- **WordPress Best Practices**: Follows WordPress coding standards
- **Security**: Proper nonce verification and capability checks
- **Caching**: Efficient caching for GitHub API calls
- **Extensibility**: Multiple hooks and filters for customization

### Documentation
- Comprehensive README with examples
- Inline code documentation
- WikiLink syntax guide
- GitHub sync instructions
- Shortcode reference

### Assets
- SimpleMDE markdown editor
- jsdiff for diff viewing
- Custom CSS with Themis colors
- JavaScript for interactive features
- Responsive grid layout

## [Unreleased]

### Future Enhancements
- [ ] Page templates support
- [ ] WikiLink autocomplete in editor
- [ ] Page preview on link hover
- [ ] Export wiki to PDF/Markdown
- [ ] Advanced permissions per wiki page
- [ ] Wiki page translations
- [ ] Custom fields for wiki pages
- [ ] Integration with other plugins
- [ ] REST API endpoints
- [ ] CLI commands for bulk operations

---

**Note**: This is the initial release of the completely redesigned Wiki Integration plugin, replacing the previous documentation fetcher with a full-featured wiki system.
