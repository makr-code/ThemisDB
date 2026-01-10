# Changelog

All notable changes to the ThemisDB Graph Pattern Visualizer plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-09

### Added
- Initial release of Graph Pattern Visualizer plugin
- Neo4j Bloom-inspired options overlay panel
- Interactive graph visualization using vis-network.js
- Real-time node search with highlighting
- Group filters with show/hide checkboxes
- Dynamic color customization with color pickers
- Multiple layout algorithms (force-directed, hierarchical, circular)
- Zoom and pan controls
- Physics simulation with adjustable parameters
- Node spacing slider (50-300)
- Edge strength slider (1-10)
- Physics and labels toggle switches
- Fullscreen mode
- PNG export functionality
- JSON export functionality
- Node details panel on click
- Hover tooltips
- Sample ThemisDB architecture graph (7 groups, 20 nodes, 22 edges)
- Admin settings page with configuration options
- WordPress shortcode `[themisdb_graph]`
- Responsive design for mobile and desktop
- Comprehensive documentation (README.md, README_DE.md)
- MIT License

### Features
- **Search**: Find nodes by name with real-time filtering
- **Filter**: Show/hide node groups with checkboxes
- **Customize**: Change node colors dynamically
- **Layout**: Adjust spacing and physics with sliders
- **Export**: Download as PNG or JSON
- **Interactive**: Click, hover, drag, and zoom
- **Responsive**: Works on all screen sizes

### Technical
- WordPress 5.0+ compatibility
- PHP 7.4+ requirement
- jQuery dependency
- vis-network.js v9.1.2 integration
- AJAX data loading
- User meta storage for custom layouts
- Transient caching support

### Inspired By
- Neo4j Bloom graph exploration tool
- ThemisDB Architecture Diagrams plugin design pattern
- vis-network.js capabilities

---

## Future Releases (Planned)

### [1.1.0] - TBD
- Dynamic node expansion on double-click
- Import custom graph JSON files
- More color scheme presets
- Node shape customization options

### [1.2.0] - TBD
- Path finding between nodes
- Edge type filtering
- Clustering for large graphs (1000+ nodes)
- Time-based animations

### [1.3.0] - TBD
- Integration with actual ThemisDB data sources
- Real-time data updates
- Multi-language support (i18n)
- Accessibility improvements (WCAG 2.1 AA)

---

[1.0.0]: https://github.com/makr-code/ThemisDB/releases/tag/v1.0.0
