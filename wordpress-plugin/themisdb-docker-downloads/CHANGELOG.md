# Changelog

All notable changes to the ThemisDB Docker Downloads WordPress plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-08

### Added
- Initial release of ThemisDB Docker Downloads plugin
- Docker Hub API integration for fetching image tags
- Support for displaying multiple Docker image architectures (amd64, arm64, arm, i386, etc.)
- SHA256 digest display for each architecture
- Three display styles: default, compact, and table
- Architecture filtering support
- Docker pull command with one-click copy functionality
- Digest copy functionality
- Admin panel for configuration
- Settings for Docker Hub namespace and repository
- Configurable cache duration
- Docker Hub token support for higher rate limits
- Connection test tool in admin panel
- Cache clearing functionality
- Responsive design for all device sizes
- WordPress shortcodes for easy integration:
  - `[themisdb_docker_tags]` - Display Docker tags
  - `[themisdb_docker_latest]` - Display latest tag name
- Automatic caching of Docker Hub API responses
- Human-readable file size formatting
- Platform-specific icons for different architectures
- Error handling and user-friendly error messages
- AJAX-based admin tools

### Documentation
- Comprehensive README.md with installation and usage instructions
- Admin panel with inline help and shortcode examples
- Code comments and documentation

### Security
- Input sanitization
- Output escaping
- Nonce verification for AJAX requests
- Capability checks for admin functions
- No direct file access allowed

## [Unreleased]

### Planned Features
- Multi-language support (German and English)
- Additional architecture support
- Image vulnerability scanning integration
- Download statistics
- Image layer information display
- Custom CSS theme support
- Import/export plugin settings
- Integration with WordPress Gutenberg blocks
- REST API endpoints for external integrations
