=== ThemisDB Benchmark Visualizer ===
Contributors: themisdb-team
Tags: benchmarks, charts, performance, visualization, themisdb
Requires at least: 5.0
Tested up to: 6.4
Requires PHP: 7.4
Stable tag: 1.0.0
License: MIT
License URI: https://opensource.org/licenses/MIT

Interactive visualization of ThemisDB performance benchmarks with Chart.js. Compare against PostgreSQL, MongoDB, and Neo4j.

== Description ==

ThemisDB Benchmark Visualizer provides interactive performance benchmark visualizations comparing ThemisDB against other popular databases including PostgreSQL, MongoDB, and Neo4j using Chart.js.

**Key Features:**

* Chart.js-powered interactive charts
* 19+ benchmark scenarios
* Real-time data visualization
* Performance comparison across databases
* Multiple chart types (line, bar, radar)
* Metric filtering (latency, throughput, memory)
* Category filtering (OLTP, OLAP, Graph queries)
* Dark mode support
* Responsive design
* Conditional asset loading
* AJAX data loading
* GitHub integration for benchmark data

**Benchmark Categories:**

* OLTP (Online Transaction Processing)
* OLAP (Online Analytical Processing)
* Graph Queries
* Document Operations
* Key-Value Operations
* Mixed Workloads

**Compared Databases:**

* ThemisDB (Multi-Model)
* PostgreSQL (Relational)
* MongoDB (Document Store)
* Neo4j (Graph Database)

**Perfect for:**

* Performance documentation
* Marketing comparison pages
* Technical benchmarks
* Developer resources
* Product demonstrations

== Installation ==

1. Upload the `themisdb-benchmark-visualizer` folder to the `/wp-content/plugins/` directory
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Use the shortcode `[themisdb_benchmark_visualizer]` in any post or page
4. Configure display options in Settings → ThemisDB Benchmarks

== Frequently Asked Questions ==

= Where does the benchmark data come from? =

The plugin loads benchmark data from the ThemisDB GitHub repository, ensuring you always have the latest performance metrics.

= Can I customize the charts? =

Yes! Go to Settings → ThemisDB Benchmarks to configure chart theme, default metrics, and display categories.

= Is Chart.js loaded on every page? =

No! The plugin uses conditional loading - Chart.js is only loaded on pages containing the shortcode.

= Can I export the benchmark data? =

The data is displayed in interactive charts. For raw data, refer to the ThemisDB GitHub repository.

= Does it work on mobile devices? =

Yes! The visualizations are fully responsive and optimized for mobile viewing with touch interactions.

= Can I add custom benchmarks? =

The current version loads data from the official ThemisDB repository. Custom benchmarks would require modifying the data source.

== Screenshots ==

1. Interactive benchmark charts
2. Metric comparison view
3. Category filtering
4. Dark mode visualization
5. Mobile responsive layout
6. Admin settings panel

== Changelog ==

= 1.0.0 =
* Initial release
* Chart.js integration
* 19 benchmark scenarios
* Multi-database comparison
* Interactive filtering
* Dark mode support
* Conditional asset loading
* GitHub data integration
* Responsive design

== Upgrade Notice ==

= 1.0.0 =
Initial release of ThemisDB Benchmark Visualizer.
