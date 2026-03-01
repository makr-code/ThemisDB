=== ThemisDB Formula Renderer ===
Contributors: themisdb-team
Tags: latex, math, katex, formulas, equations
Requires at least: 5.0
Tested up to: 6.4
Requires PHP: 7.2
Stable tag: 1.1.0
License: MIT
License URI: https://opensource.org/licenses/MIT

Render mathematical formulas in LaTeX notation ($$...$$) with KaTeX. Supports inline and block formulas with conditional loading.

== Description ==

ThemisDB Formula Renderer is a lightweight plugin that renders mathematical formulas written in LaTeX notation using the fast KaTeX library. It provides automatic detection and rendering of formulas with smart conditional asset loading for optimal performance.

**Key Features:**

* KaTeX-powered LaTeX rendering
* Support for inline formulas ($...$)
* Support for block formulas ($$...$$)
* Conditional asset loading (only loads when needed)
* Multiple shortcodes: [themisdb_formula], [formula], [latex], [math]
* Custom delimiter configuration
* Auto-render mode
* Formula library with common examples
* Admin settings panel
* Zero-configuration setup

**Supported Notation:**

* Inline: `$E = mc^2$`
* Block: `$$\int_{0}^{\infty} e^{-x^2} dx$$`
* Shortcode: `[formula]x^2 + y^2 = z^2[/formula]`

**Perfect for:**

* Mathematics education
* Scientific documentation
* Technical blogs
* Academic websites
* Engineering content
* Statistical analysis posts

== Installation ==

1. Upload the `themisdb-formula-renderer` folder to the `/wp-content/plugins/` directory
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Write LaTeX formulas using $ or $$ delimiters
4. Optionally, configure settings in Settings → ThemisDB Formula

== Frequently Asked Questions ==

= What is KaTeX? =

KaTeX is the fastest math typesetting library for the web, compatible with LaTeX and faster than MathJax.

= How do I write a formula? =

Use single dollar signs for inline formulas: `$x^2$` or double dollar signs for block formulas: `$$\sum_{i=1}^{n} i$$`

= Can I customize delimiters? =

Yes! Go to Settings → ThemisDB Formula to change the inline and block delimiters.

= Does it slow down my site? =

No! The plugin uses conditional loading - KaTeX assets are only loaded on pages that actually contain formulas.

= Can I use this with Gutenberg? =

Yes! You can use LaTeX notation in any text block or use the shortcode in a Shortcode block.

= Is there a formula library? =

Yes! The plugin includes a formula library with common mathematical formulas that you can reference.

== Screenshots ==

1. Inline formula rendering
2. Block formula display
3. Formula library
4. Admin settings panel
5. Auto-render configuration

== Changelog ==

= 1.1.0 =
* Added conditional asset loading
* Performance optimization
* Improved delimiter detection
* Added formula library
* Enhanced admin settings

= 1.0.0 =
* Initial release
* KaTeX integration
* Multiple shortcode support
* Basic auto-render functionality

== Upgrade Notice ==

= 1.1.0 =
Performance improvements with conditional loading. Formulas now only load KaTeX when needed.

= 1.0.0 =
Initial release of ThemisDB Formula Renderer.
