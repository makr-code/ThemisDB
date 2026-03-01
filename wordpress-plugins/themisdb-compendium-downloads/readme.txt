=== ThemisDB Compendium Downloads ===
Contributors: themisdb-team
Tags: downloads, compendium, github, releases, themisdb
Requires at least: 5.0
Tested up to: 6.4
Requires PHP: 7.2
Stable tag: 1.0.0
License: MIT
License URI: https://opensource.org/licenses/MIT

Display ThemisDB Compendium download information with GitHub releases integration.

== Description ==

ThemisDB Compendium Downloads integrates with GitHub Releases API to display download information, version history, and file details for the ThemisDB Compendium documentation.

**Key Features:**

* GitHub Releases API integration
* Display all compendium versions
* Download statistics tracking
* File size information
* Release notes display
* Version history
* Transient caching
* Responsive download buttons
* AJAX functionality
* Shortcode support

**Shortcodes:**

* `[themisdb_compendium_downloads]` - Display all compendium downloads
* `[themisdb_compendium]` - Alias for downloads display

**Display Information:**

* Version numbers
* Release dates
* File sizes
* Download counts
* Release descriptions
* Asset listings

**Perfect for:**

* Documentation distribution
* Download centers
* Version history pages
* Software release pages
* Product documentation

== Installation ==

1. Upload the `themisdb-compendium-downloads` folder to the `/wp-content/plugins/` directory
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Use shortcode `[themisdb_compendium_downloads]` to display downloads
4. Configure GitHub repository in Settings → ThemisDB Compendium

== Frequently Asked Questions ==

= Do I need a GitHub account? =

No, the plugin accesses public GitHub API endpoints for public repositories.

= How often is the data updated? =

Data is cached for performance (default 1 hour) but automatically refreshes when cache expires.

= Can I track download statistics? =

Yes! The plugin includes download tracking functionality using AJAX.

= Does it show file sizes? =

Yes! File sizes are displayed for each downloadable asset when the option is enabled.

= Can I customize the button style? =

Yes! Multiple button styles are available in the plugin settings (modern, classic, minimal).

= Can I filter by version? =

The plugin displays all available releases. Users can browse the complete version history.

== Screenshots ==

1. Compendium downloads display
2. Version history list
3. Download button styles
4. Mobile responsive view
5. Admin settings panel

== Changelog ==

= 1.0.0 =
* Initial release
* GitHub Releases integration
* Version history display
* Download tracking
* File size display
* Multiple button styles
* Transient caching
* Responsive design
* Shortcode support

== Upgrade Notice ==

= 1.0.0 =
Initial release of ThemisDB Compendium Downloads.
