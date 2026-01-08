# Changelog - ThemisDB Architecture Diagrams Plugin

## [1.0.1] - 2026-01-08

### Fixed
- **Mermaid.js Rendering Issue**: Fixed graph code not being converted to graphics
  - Updated `mermaid.run()` API usage from v9 `querySelector` parameter to v10+ `nodes` array parameter
  - Added proper waiting mechanism for Mermaid library to load before initialization
  - Added error handling for rendering failures with informative error messages
  - Added removal of `data-processed` attribute to enable re-rendering of diagrams
  - Improved timing to prevent race conditions during library loading

### Technical Details
- Changed from `mermaid.run({ querySelector: '#selector' })` to `mermaid.run({ nodes: [element] })`
- Implemented `waitForMermaid()` promise-based loader with timeout protection
- Added `.catch()` error handlers for graceful failure handling
- Clear and reset diagram container before each render to prevent artifacts

## [1.0.0] - Initial Release
- Complete architecture visualization system
- Multiple views: High-Level, Storage Layer, LLM Integration, Sharding/RAID
- Comparison diagrams: Database, LLM Services, Performance, TCO, Feature Matrix
- Interactive components with zoom, fullscreen, and export capabilities
- WordPress integration with shortcode and admin settings
