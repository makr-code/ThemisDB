# Changelog

All notable changes to the ThemisDB Feature Matrix WordPress Plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-02-11

### Added
- Interactive feature comparison matrix between ThemisDB, PostgreSQL, MongoDB, and Neo4j
- Category-based filtering (Data Models, AI/ML, Performance, Compatibility, Licensing)
- Column sorting by database support level
- Hover tooltips for detailed feature information
- CSV export functionality with date-stamped filenames
- Mobile-responsive card view (automatic switch at < 768px)
- Sticky table header for better navigation
- ThemisDB column highlighting with "Recommended" badge
- Dark mode support (auto-detect via prefers-color-scheme)
- WCAG 2.1 AA accessibility compliance
  - Semantic HTML with proper ARIA labels
  - Keyboard navigation support
  - Screen reader support
  - Color contrast compliance (4.5:1 minimum)
  - Focus indicators on interactive elements
- Admin settings page under Settings → Feature Matrix
- Comprehensive feature data for all databases
  - 6 data models support comparison
  - 4 AI/ML features (including Embedded LLM, RAG, GPU)
  - 4 performance & scaling features
  - 5 protocol compatibility options
  - Licensing information
- Shortcode with customizable parameters
- Themis brand colors integration
- Print-optimized CSS

### Technical Details
- PHP 7.4+ required
- WordPress 5.8+ required
- Object-oriented architecture with separate classes
- Clean code structure following WordPress coding standards
- Sanitized inputs and escaped outputs for security
- Localization-ready (text domain: themisdb-feature-matrix)

## [0.1.0] - Initial Development (Replaced)

### Changed
- Complete rewrite from Mermaid.js-based visualization to interactive comparison table
- New architecture based on specification requirements
- Enhanced user experience with filtering and sorting capabilities
