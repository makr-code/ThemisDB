# Changelog - ThemisDB Architecture Diagrams Plugin

## [1.0.2] - 2026-01-08

### Fixed
- **Script Loading Order Issue**: Fixed "Failed to load Mermaid library" error
  - Changed Mermaid.js CDN script to load in header instead of footer to prevent timing issues
  - Increased timeout for Mermaid library loading from 5 seconds to 10 seconds
  - Improved error messages to provide more helpful troubleshooting information
  - Added debug logging to console for Mermaid library load status

### Technical Details
- Changed `wp_enqueue_script()` 5th parameter from `true` (footer) to `false` (header)
- Extended `MAX_MERMAID_LOAD_ATTEMPTS` from 50 to 100 (10 seconds total)
- Added detailed error messages mentioning network, content blockers, and firewall issues
- Added console logging to help diagnose load timing issues

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
