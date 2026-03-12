=== ThemisDB Wiki Integration ===
Contributors: themisdb-team
Tags: documentation, wiki, github, markdown, themisdb
Requires at least: 5.0
Tested up to: 6.4
Requires PHP: 7.4
Stable tag: 1.0.0
License: MIT
License URI: https://opensource.org/licenses/MIT

Automatically integrates ThemisDB documentation/wiki from GitHub into WordPress. Fetches markdown files and displays them with proper formatting.

== Description ==

ThemisDB Wiki Integration is a WordPress plugin that enables automatic integration of ThemisDB documentation (wiki) from GitHub into your WordPress website. The plugin fetches markdown files directly from the GitHub repository and displays them formatted in WordPress.

**Key Features:**

* Automatic fetching of markdown documentation from GitHub
* Support for multiple languages (DE, EN, FR)
* Caching mechanism for better performance
* Automatic synchronization (hourly)
* Shortcodes for easy integration
* Table of contents generation
* Responsive design
* Dark mode support
* Admin panel for configuration

**Perfect for:**

* Technical documentation websites
* Open source projects
* Developer communities
* Multi-language documentation
* Knowledge bases

== Installation ==

1. Upload the `themisdb-wiki-integration` folder to the `/wp-content/plugins/` directory
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Go to Settings → ThemisDB Wiki to configure the plugin

For detailed installation instructions, see INSTALLATION.md

== Frequently Asked Questions ==

= Do I need a GitHub account? =

You don't need a GitHub account to use this plugin with public repositories. However, for private repositories or higher API rate limits, you'll need to create a GitHub Personal Access Token.

= How do I display documentation on my site? =

Use the shortcode `[themisdb_wiki file="README.md" lang="de"]` in any post or page. See the documentation for more examples.

= Can I use this with private GitHub repositories? =

Yes! Simply add a GitHub Personal Access Token in the plugin settings.

= How often is the documentation synchronized? =

By default, the documentation is synchronized hourly if auto-sync is enabled. You can also manually sync at any time using the "Sync Now" button in the admin panel.

= Can I customize the styling? =

Yes! The plugin includes CSS classes that you can override in your theme's custom CSS. See the documentation for details.

= Does this work with any GitHub repository? =

Yes! While designed for ThemisDB, you can use this plugin with any GitHub repository containing markdown documentation.

== Screenshots ==

1. Admin panel configuration
2. Documentation display with table of contents
3. Documentation list view
4. Grid layout for documentation files
5. Mobile responsive view

== Changelog ==

= 1.0.0 =
* Initial release
* GitHub API integration
* Markdown-to-HTML conversion
* Multi-language support (DE, EN, FR)
* Caching mechanism
* Auto-sync functionality
* Admin panel
* Responsive design
* Dark mode support

== Upgrade Notice ==

= 1.0.0 =
Initial release of ThemisDB Wiki Integration plugin.

== Usage ==

**Display Documentation:**

`[themisdb_wiki file="README.md" lang="de" show_toc="yes"]`

Parameters:
* file: Markdown file to display (e.g., README.md, features/FEATURES.md)
* lang: Language (de, en, fr) - defaults to plugin setting
* show_toc: Show table of contents (yes/no)

**List Documentation Files:**

`[themisdb_docs lang="de" layout="grid"]`

Parameters:
* lang: Language (de, en, fr)
* layout: Display layout (list, grid)

== Configuration ==

After activation, go to Settings → ThemisDB Wiki and configure:

* GitHub Repository: Format owner/repository (default: makr-code/wordpressPlugins)
* Branch: Repository branch (default: main)
* Documentation Path: Path to docs folder (default: docs)
* Default Language: de, en, or fr
* GitHub Token (Optional): For private repos or higher rate limits
* Auto-Sync: Enable for automatic hourly synchronization

== Technical Details ==

**API Rate Limits:**
* Without token: 60 requests/hour
* With token: 5,000 requests/hour

**Caching:**
* Transient cache with 1-hour default
* Manual cache clearing via admin panel
* Automatic cache clearing on auto-sync

**Performance:**
* Lazy loading of assets
* CDN compatible
* Responsive images
* Optimized for mobile

== Support ==

For support, please visit:
* GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
* Documentation: https://github.com/makr-code/wordpressPlugins/tree/main/docs

== License ==

This plugin is licensed under the MIT License. See LICENSE file for details.

== Credits ==

Developed by the ThemisDB Team
Inspired by the TCO Calculator Plugin
Based on GitHub Contents API
