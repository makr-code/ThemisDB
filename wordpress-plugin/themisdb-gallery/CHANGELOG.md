# Changelog - ThemisDB Gallery Plugin

All notable changes to this project will be documented in this file.

## [1.0.1] - 2026-01-08

### Added
- **Media Library Tab Integration** - ThemisDB Gallery ist jetzt direkt im WordPress Dashboard 'Medien' als Tab verfügbar
  - Neuer Tab "ThemisDB Gallery" im Media-Upload-Dialog
  - Bildsuche direkt aus der Mediathek heraus
  - Nahtlose Integration in den WordPress Media Modal
  - Bilder können direkt beim Hochladen durchsucht und eingefügt werden
- `assets/js/media-tab.js` - Dedicated JavaScript for media library tab functionality

### Changed
- Erweiterte WordPress-Integration für bessere Benutzerfreundlichkeit
- Optimierte Bildauswahl-Workflow im Admin-Bereich

## [1.0.0] - 2026-01-07

### Added
- Initial release of ThemisDB Gallery plugin
- Image search integration with multiple providers:
  - Unsplash API integration
  - Pexels API integration
  - Pixabay API integration
- Automatic image attribution system
  - Photographer credits with links
  - License information
  - Stored in WordPress media metadata
- WordPress editor integration
  - Meta box in post/page editor
  - Direct image insertion into content
  - Visual image preview and selection
- AI image generation (optional)
  - OpenAI DALL-E integration
  - Generate images from text descriptions
  - Automatic "AI Generated" attribution
- Shortcodes for flexible display
  - `[themisdb_gallery]` - Display image galleries
  - `[themisdb_image_search]` - Frontend search widget
  - `[themisdb_image_attribution]` - Display attribution
- Admin settings page
  - API key configuration
  - Provider selection
  - Cache duration settings
  - Auto-attribution toggle
- Caching system
  - Reduces API calls
  - Configurable cache duration
  - Manual cache clearing
- Responsive design
  - Mobile-friendly layouts
  - Configurable column grids (1-4 columns)
  - Touch-optimized interface
- Frontend features
  - Simple lightbox for image viewing
  - Search widget for visitors
  - Gallery display options
- Gutenberg block support (basic)
  - Image search block
  - Gallery block
  - Block editor integration
- Multi-language ready
  - Text domain: themisdb-gallery
  - Translation-ready strings
  - German translations included
- Security features
  - Nonce verification for AJAX requests
  - Capability checks for user permissions
  - Input sanitization and validation
  - XSS protection

### Technical Details
- Minimum PHP version: 7.2
- Minimum WordPress version: 5.0
- Uses WordPress HTTP API for external requests
- Implements WordPress transients for caching
- Follow WordPress coding standards
- Object-oriented architecture with separate classes

### API Integrations
- Unsplash: 50 requests/hour (free tier)
- Pexels: 200 requests/hour (free tier)
- Pixabay: 5000 requests/hour (free tier)
- OpenAI: Usage-based pricing (optional)

### Files Added
- `themisdb-gallery.php` - Main plugin file
- `includes/class-image-api.php` - Image API handler
- `includes/class-admin.php` - Admin panel handler
- `includes/class-media-handler.php` - Media import handler
- `includes/class-shortcodes.php` - Shortcode handler
- `includes/class-gutenberg-block.php` - Gutenberg block handler
- `assets/css/style.css` - Frontend styles
- `assets/css/admin.css` - Admin styles
- `assets/css/blocks.css` - Block styles (placeholder)
- `assets/css/blocks-editor.css` - Block editor styles (placeholder)
- `assets/js/script.js` - Frontend JavaScript
- `assets/js/admin.js` - Admin JavaScript
- `assets/js/blocks.js` - Gutenberg blocks JavaScript
- `README.md` - Plugin documentation
- `INSTALLATION.md` - Installation guide
- `CHANGELOG.md` - This file
- `LICENSE` - MIT License

## [Unreleased]

### Planned Features
- Enhanced Gutenberg blocks with live preview
- Bulk image import functionality
- Advanced search filters (color, orientation, size)
- Image editing capabilities (crop, resize)
- Additional AI providers (Stable Diffusion, Midjourney)
- Custom attribution templates
- Multi-site support
- Performance analytics dashboard
- Image optimization on import
- Automatic alt text generation using AI
- Support for additional image sources
- Integration with WordPress block patterns
- Advanced caching strategies
- REST API endpoints for third-party integrations

### Known Issues
- None reported yet

---

## Version History

- **1.0.0** (2026-01-07) - Initial release

## Upgrade Notice

### 1.0.0
First release of ThemisDB Gallery. Configure your API keys in Settings → ThemisDB Gallery to start using the plugin.

---

For detailed installation and usage instructions, see [README.md](README.md) and [INSTALLATION.md](INSTALLATION.md).
