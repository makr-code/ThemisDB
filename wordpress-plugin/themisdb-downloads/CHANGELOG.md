# ThemisDB Downloads WordPress Plugin - Changelog

All notable changes to this WordPress plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
- GitHub API integration for fetching releases from makr-code/ThemisDB
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
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: See README.md

---

## Version History

- **1.0.0** (2026-01-07) - Initial release

---

**Note:** This changelog follows the format recommended by [Keep a Changelog](https://keepachangelog.com/).
