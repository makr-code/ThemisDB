=== ThemisDB Architecture Diagrams ===
Contributors: themisdb-team
Tags: diagrams, architecture, mermaid, visualization, themisdb
Requires at least: 5.0
Tested up to: 6.4
Requires PHP: 7.4
Stable tag: 1.1.0
License: MIT
License URI: https://opensource.org/licenses/MIT

Interactive architecture diagrams for ThemisDB using Mermaid.js with dark mode support and conditional loading.

== Description ==

ThemisDB Architecture Diagrams visualizes ThemisDB's multi-model architecture, storage layer, LLM integration, and sharding strategy using the powerful Mermaid.js library.

**Key Features:**

* Mermaid.js-powered diagram rendering
* Multiple architecture diagrams
* Dark mode automatic detection
* Conditional asset loading
* Interactive diagram navigation
* Responsive SVG output
* Multiple diagram types (flowchart, sequence, class)
* Color scheme detection
* GitHub integration
* Copy-to-clipboard functionality

**Available Diagrams:**

* Multi-Model Architecture
* Storage Layer Architecture
* LLM Integration Flow
* Sharding Strategy
* Query Processing Pipeline
* Data Model Relationships

**Perfect for:**

* Technical documentation
* Architecture presentations
* Developer onboarding
* System design docs
* Educational content

== Installation ==

1. Upload the `themisdb-architecture-diagrams` folder to the `/wp-content/plugins/` directory
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Use the shortcode `[themisdb_architecture]` or `[themisdb_architecture type="storage"]`
4. Diagrams automatically adapt to your site's color scheme

== Frequently Asked Questions ==

= What is Mermaid.js? =

Mermaid is a JavaScript-based diagramming tool that renders text-based diagram definitions into interactive SVG diagrams.

= How do I choose which diagram to display? =

Use the type attribute: `[themisdb_architecture type="storage"]` for storage layer or `[themisdb_architecture type="llm"]` for LLM integration.

= Does it support dark mode? =

Yes! The plugin automatically detects your site's color scheme and renders diagrams in the appropriate theme.

= Can I customize the diagrams? =

The diagrams are loaded from the ThemisDB repository. You can modify colors through theme settings, but diagram structure modifications require code changes.

= Are the diagrams interactive? =

Yes! Mermaid.js creates interactive SVG diagrams that users can explore.

= Does it slow down my page? =

No! The plugin uses conditional loading - Mermaid.js is only loaded on pages containing the architecture shortcode.

== Screenshots ==

1. Multi-model architecture diagram
2. Storage layer visualization
3. LLM integration flow
4. Dark mode rendering
5. Mobile responsive view
6. Interactive diagram elements

== Changelog ==

= 1.1.0 =
* Added conditional asset loading
* Performance optimization
* Improved color scheme detection
* Enhanced dark mode support
* Added multiple diagram types

= 1.0.0 =
* Initial release
* Mermaid.js integration
* Architecture diagram rendering
* Basic dark mode support

== Upgrade Notice ==

= 1.1.0 =
Performance improvements with conditional loading and enhanced dark mode support.

= 1.0.0 =
Initial release of ThemisDB Architecture Diagrams.
