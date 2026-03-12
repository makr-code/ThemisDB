# ThemisDB Downloads WordPress Plugin - Changelog

All notable changes to this WordPress plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.0] - 2026-01-08

### Added
- **Enhanced Markdown to HTML Conversion**: Complete rewrite of markdown parsing with full Markdown support
  - Headers (H1-H6)
  - Text formatting (bold, italic, strikethrough)
  - Links with title attributes
  - Images with alt text and titles
  - Code blocks with language detection (JavaScript, Python, PHP, etc.)
  - Inline code
  - Ordered and unordered lists with nesting support
  - Blockquotes
  - Tables with column alignment (left, center, right)
  - Horizontal rules
- **Mermaid Diagram Support**: Embedded diagrams are automatically rendered
  - Integration with Mermaid.js v10 via CDN
  - Support for flowcharts, sequence diagrams, Gantt charts, etc.
  - Secure rendering with strict security level
- **Shared Markdown Converter Class**: New `ThemisDB_Markdown_Converter` for code reuse
  - Used across all WordPress plugins
  - Centralized conversion logic
  - Proper HTML escaping and XSS protection
- **Enhanced CSS Styling**: Comprehensive styling for all markdown elements
  - Tables with alternating row colors
  - Responsive images
  - Styled code blocks and inline code
  - Mermaid diagram containers
  - Nested list support
  - Proper spacing and typography

### Changed
- Refactored `markdown_to_html()` method to use shared converter class
- Updated JavaScript to initialize Mermaid.js on page load
- Improved regex for language identifier detection (now supports JavaScript, C++, PHP8, etc.)

### Security
- Changed Mermaid security level from 'loose' to 'strict' to prevent XSS attacks
- Enhanced HTML escaping in markdown converter
- Proper URL validation for links and images

### Fixed
- Markdown files from GitHub (README, CHANGELOG) now display properly formatted instead of raw text
- Links in markdown now open in new tabs with proper security attributes
- Nested lists now render correctly
- Tables with alignment now display properly

## [1.2.0] - 2026-01-07

### Added
- **Automatic Content-Based Tags and Categories**: Plugin now automatically extracts and assigns WordPress tags and categories from post/page content
- New `class-taxonomy-manager.php` for intelligent content analysis and taxonomy extraction
- Advanced text analysis techniques:
  - **Word Frequency Analysis**: Most frequently used words become tags
  - **Title Weighting**: Words in the title get 3x priority
  - **Relevance Scoring**: Longer words (>6 chars) and capitalized words get bonus scores
  - **Stop Word Filtering**: Common German and English filler words are excluded
  - **Phrase Recognition**: 2-3 word combinations are extracted as categories
- Auto-generated tags based on:
  - Word frequency in content
  - Position in title vs body
  - Word length and capitalization
  - Up to 15 most relevant terms
- Auto-generated categories based on:
  - Multi-word phrase extraction (bigrams and trigrams)
  - Phrase frequency and title occurrence
  - Up to 5 most relevant phrases
- Admin settings for controlling auto-taxonomy feature:
  - Toggle to enable/disable automatic taxonomies
  - Separate controls for tags and categories
  - Detailed descriptions and examples in admin panel
- Automatic taxonomy assignment:
  - Triggers when saving any post or page
  - Analyzes title and content text
  - Creates new tags/categories if they don't exist
  - Appends to existing taxonomies without removing them

### Changed
- Updated plugin version to 1.2.0
- Enhanced plugin description to mention content-based tag and category extraction
- Extended admin panel with new "Automatische Schlagwörter und Kategorien" section
- Updated README.md with comprehensive documentation on content analysis feature
- Completely rewritten taxonomy manager to use NLP-style content analysis instead of GitHub API data

### Technical Details
- New activation hooks to set default auto-taxonomy options (enabled by default)
- Integration with WordPress `save_post` action for automatic processing on save
- Text tokenization with Unicode support for German umlauts
- Intelligent word and phrase extraction algorithms
- Best practices for automatic tagging following WordPress standards

## [1.1.0] - 2026-01-07

### Added
- README.md display from release assets with `[themisdb_readme]` shortcode
- CHANGELOG.md/RELEASE_NOTES.md display from release assets with `[themisdb_changelog]` shortcode
- Support for version-specific README/CHANGELOG display (e.g., `version="v1.3.4"`)
- Basic markdown to HTML conversion for README and CHANGELOG files
- Styling for formatted README and CHANGELOG display
- Raw text display mode for README/CHANGELOG with `style="raw"` attribute

### Changed
- Extended GitHub API handler to download and parse README/CHANGELOG files
- Consolidated file download methods for better code reuse
- Updated admin panel to show new shortcode options
- Enhanced documentation with README/CHANGELOG shortcode examples

## [1.0.0] - 2026-01-07

### Added
- Initial release of ThemisDB Downloads WordPress Plugin
- GitHub API integration for fetching releases from makr-code/wordpressPlugins
- Automatic parsing of release assets and SHA256SUMS files
- Frontend display with three styles: Standard, Compact, and Table
- Platform detection and filtering (Windows, Linux, Docker, QNAP, ARM, macOS)
- SHA256 checksum display for all download files
- Browser-based file verification using Web Crypto API
- Copy-to-clipboard functionality for SHA256 hashes
- Admin settings panel with configuration options:
  - GitHub repository configuration
  - Optional GitHub Personal Access Token support
  - Cache duration settings (default: 1 hour)
  - Number of releases to display
  - Pre-release toggle
- WordPress transient-based caching system
- Manual cache clearing functionality
- Multiple shortcodes:
  - `[themisdb_downloads]` - Display latest or all releases
  - `[themisdb_latest]` - Display version number only
  - `[themisdb_verify]` - Interactive verification tool
- Responsive design for mobile, tablet, and desktop
- Support for all ThemisDB release formats:
  - Windows (.zip, .exe)
  - Linux (.tar.gz, .deb, .rpm)
  - Docker (links to Docker Hub)
  - QNAP NAS packages
  - ARM builds
- Comprehensive documentation:
  - Installation guide (INSTALLATION.md)
  - User documentation (README.md)
  - Packaging guide (../PACKAGING.md)
  - Screenshot examples (../SCREENSHOTS.md)
- MIT License

### Security
- Input sanitization for all user inputs
- Output escaping for all displayed data
- Nonce verification for AJAX requests
- Capability checks for admin functions
- No direct file access protection

### Documentation
- Complete WordPress plugin documentation
- German language installation guide
- Integration with ThemisDB deployment strategy
- Shortcode usage examples
- API configuration guide
- Troubleshooting section

## [Unreleased]

### Planned Features
- Multi-language support (German, English)
- WordPress.org repository submission
- Widget support for sidebar display
- Gutenberg block for visual editor
- Email notifications for new releases
- RSS feed for releases
- Download statistics tracking
- Custom template system for advanced theming
- WP-CLI integration
- Docker container support detection
- Automated testing suite
- Performance improvements with object caching
- Support for private GitHub repositories
- Edition filtering (Community, Enterprise, Hyperscaler)

### Known Issues
- None reported yet

---

## Release Notes

### Version 1.0.0 - Initial Release

This is the first public release of the ThemisDB Downloads WordPress Plugin. It provides a complete solution for displaying ThemisDB releases on WordPress websites with automatic GitHub integration, SHA256 verification, and a modern, responsive design.

**Key Features:**
- Automatic release fetching from GitHub
- SHA256 checksum display and verification
- Multiple display styles
- Platform filtering
- Cache management
- Admin configuration panel

**Requirements:**
- WordPress 5.0+
- PHP 7.2+
- HTTPS recommended

**Installation:**
See [INSTALLATION.md](INSTALLATION.md) for detailed instructions.

**Support:**
- GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- Documentation: See README.md

---

## Version History

- **1.3.0** (2026-01-08) - Enhanced markdown conversion with Mermaid diagram support
- **1.2.0** (2026-01-07) - Auto tags and categories feature
- **1.1.0** (2026-01-07) - README and CHANGELOG shortcodes
- **1.0.0** (2026-01-07) - Initial release

---

**Note:** This changelog follows the format recommended by [Keep a Changelog](https://keepachangelog.com/).
