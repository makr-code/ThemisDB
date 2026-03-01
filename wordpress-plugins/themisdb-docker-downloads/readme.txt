=== ThemisDB Docker Downloads ===
Contributors: themisdb-team
Tags: docker, downloads, docker-hub, api, themisdb
Requires at least: 5.0
Tested up to: 6.4
Requires PHP: 7.2
Stable tag: 1.0.0
License: MIT
License URI: https://opensource.org/licenses/MIT

Display Docker Hub tags and download information with real-time API integration.

== Description ==

ThemisDB Docker Downloads integrates with Docker Hub API to display available Docker image tags, version information, and download statistics for ThemisDB Docker images.

**Key Features:**

* Docker Hub API integration
* Display all available tags
* Show latest Docker image version
* Real-time download statistics
* Image size information
* Last update timestamps
* Transient caching for performance
* Responsive tag display
* AJAX-powered updates
* Shortcode integration

**Shortcodes:**

* `[themisdb_docker_tags]` - Display all available Docker tags
* `[themisdb_docker_latest]` - Show only the latest Docker image

**Perfect for:**

* Software distribution pages
* Docker image documentation
* Download centers
* DevOps documentation
* Containerization guides

== Installation ==

1. Upload the `themisdb-docker-downloads` folder to the `/wp-content/plugins/` directory
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Use shortcodes to display Docker information: `[themisdb_docker_tags]`
4. Optionally configure Docker Hub repository in plugin settings

== Frequently Asked Questions ==

= Do I need a Docker Hub account? =

No, the plugin accesses public Docker Hub API endpoints and doesn't require authentication for public repositories.

= How often is the data updated? =

The plugin uses WordPress transients to cache data for performance. By default, data refreshes every hour or when cache expires.

= Can I use this with private Docker repositories? =

The current version is designed for public repositories. Private repository support would require Docker Hub API authentication.

= Is the data shown in real-time? =

Data is cached for performance but can be refreshed. The plugin balances real-time accuracy with site performance.

= Can I customize the display? =

Yes! The plugin includes CSS classes that can be styled through your theme's custom CSS.

== Screenshots ==

1. Docker tags display
2. Latest version information
3. Download statistics
4. Mobile responsive view

== Changelog ==

= 1.0.0 =
* Initial release
* Docker Hub API integration
* Tag listing functionality
* Latest version display
* Transient caching
* Responsive design
* Shortcode support

== Upgrade Notice ==

= 1.0.0 =
Initial release of ThemisDB Docker Downloads.
