# WordPress Theme Benchmark Integration Guide

This guide explains how to integrate the ThemisDB Benchmark Visualizer plugin with the WordPress theme to display benchmark data.

## Overview

The benchmark visualizer plugin now reads real benchmark data from the `benchmarks/benchmark_results` directory and displays:
- **19 different benchmark files** with comprehensive performance metrics
- **10 category filters** for different types of operations
- **Real-time statistics** showing total benchmarks, average performance, best/worst times
- **Interactive charts** with Bar, Line, and Radar visualizations

## Quick Start

### 1. Install the Plugin

Copy the plugin to your WordPress plugins directory:

```bash
cp -r wordpress-plugin/benchmark-visualizer-wordpress /path/to/wordpress/wp-content/plugins/themisdb-benchmark-visualizer
```

### 2. Activate the Plugin

1. Go to WordPress Admin → Plugins
2. Find "ThemisDB Benchmark Visualizer"
3. Click "Activate"

### 3. Add Benchmark Data

The plugin automatically looks for benchmark data in these locations (in order):
1. `../../benchmarks/benchmark_results/20251223_085556` (relative to plugin)
2. `/home/runner/work/ThemisDB/ThemisDB/benchmarks/benchmark_results/20251223_085556` (absolute)
3. Latest dated directory in `benchmarks/benchmark_results/`

**Important**: Ensure the benchmark data directory is accessible by the web server.

### 4. Embed in a Page

Create a new page or post and add the shortcode:

```
[themisdb_benchmark_visualizer]
```

Or with specific options:

```
[themisdb_benchmark_visualizer category="vector_search" metric="latency" chart_type="bar"]
```

## Available Categories

The plugin now supports **10 different benchmark categories**:

| Category | Description | Files Included |
|----------|-------------|----------------|
| `all` | All Operations | Core performance, comprehensive, graph traversal, compression, encryption, MVCC, image analysis, advanced patterns |
| `vector_search` | Vector Search & Embeddings | Comprehensive benchmarks, GNN embeddings |
| `graph_traversal` | Graph Algorithms | Graph traversal, PageRank |
| `encryption` | Security Operations | Encryption benchmarks, HSM provider |
| `compression` | Data Compression | Compression algorithms |
| `transaction` | ACID Guarantees | MVCC, lock contention |
| `image_analysis` | Image Processing | Image analysis, latency metrics |
| `advanced` | Advanced Features | Advanced patterns, hybrid AQL, changefeeds, hotspots |
| `gpu` | GPU Acceleration | GPU backend benchmarks |
| `content` | Content Management | Content versioning, index rebuilding |

## Benchmark Files Parsed

The plugin parses these 19 benchmark files:

1. **bench_comprehensive.json** - General vector and embedding operations
2. **bench_core_performance.json** - Core database operations
3. **bench_graph_traversal.json** - BFS/DFS graph algorithms
4. **bench_advanced_patterns.json** - Complex query patterns
5. **bench_compression.json** - Compression algorithms
6. **bench_content_versioning.json** - Version control operations
7. **bench_encryption.json** - Cryptographic operations
8. **bench_gnn_embeddings.json** - Graph Neural Network embeddings
9. **bench_gpu_backends.json** - GPU acceleration
10. **bench_hotspots_micro.json** - Performance hotspot analysis
11. **bench_hsm_provider.json** - Hardware Security Module tests
12. **bench_hybrid_aql_sugar.json** - Hybrid query language
13. **bench_image_analysis.json** - Image processing operations
14. **bench_image_analysis_latency.json** - Image latency metrics
15. **bench_index_rebuild.json** - Index maintenance
16. **bench_lock_contention.json** - Concurrency control
17. **bench_mvcc.json** - Multi-Version Concurrency Control
18. **bench_pagerank.json** - PageRank algorithm
19. **bench_changefeed_throughput.json** - Real-time change feeds

## Integration with Theme

### Option 1: Create a Dedicated Benchmarks Page

1. In WordPress Admin, go to Pages → Add New
2. Title: "Performance Benchmarks"
3. Add the shortcode in the content area:
   ```
   [themisdb_benchmark_visualizer]
   ```
4. Publish the page

### Option 2: Add to Homepage

Edit your theme's template (e.g., `front-page.php` or `page.php`):

```php
<?php
// After main content
if (shortcode_exists('themisdb_benchmark_visualizer')) {
    echo do_shortcode('[themisdb_benchmark_visualizer category="all"]');
}
?>
```

### Option 3: Create Multiple Category Pages

Create separate pages for each category:

**Vector Search Page:**
```
[themisdb_benchmark_visualizer category="vector_search" chart_type="bar"]
```

**Graph Traversal Page:**
```
[themisdb_benchmark_visualizer category="graph_traversal" chart_type="line"]
```

**GPU Performance Page:**
```
[themisdb_benchmark_visualizer category="gpu" chart_type="radar"]
```

## Customization

### Styling

Override plugin styles in your theme's `style.css`:

```css
/* Custom colors for benchmark visualizer */
.themisdb-benchmark-wrapper {
    --themisdb-primary: #your-primary-color;
    --themisdb-secondary: #your-secondary-color;
}

/* Custom stat card colors */
.themisdb-stat-card.success {
    border-color: #your-success-color;
}
```

### Widget Area

Add benchmark visualizer to a widget area:

```php
// In functions.php
function themisdb_custom_widgets() {
    register_sidebar(array(
        'name'          => 'Benchmark Dashboard',
        'id'            => 'benchmark-dashboard',
        'before_widget' => '<div class="widget benchmark-widget">',
        'after_widget'  => '</div>',
    ));
}
add_action('widgets_init', 'themisdb_custom_widgets');

// In your template
<?php if (is_active_sidebar('benchmark-dashboard')) : ?>
    <?php dynamic_sidebar('benchmark-dashboard'); ?>
<?php endif; ?>
```

Then use a Text widget with the shortcode.

## Features

### Statistics Display

The visualizer shows:
- **Total Benchmarks**: Number of benchmark tests executed
- **Files Parsed**: Number of benchmark files loaded
- **Average Performance**: Mean latency/throughput across all tests
- **Best Performance**: Fastest operation time
- **Slowest Operation**: Longest operation time

### Smart Data Limiting

When a category has more than 30 benchmarks, the visualizer automatically shows only the **top 30 best-performing** operations for better chart readability. A note indicates when data is limited.

### Category-Specific Insights

Each category displays tailored performance insights:
- Vector Search: Native vector capabilities and similarity search
- Graph Traversal: Relationship queries and graph analytics
- Encryption: Security overhead and cryptographic performance
- GPU: Hardware acceleration benefits
- And more...

### Export Functionality

Users can:
- **Export CSV**: Download benchmark data as CSV
- **Print**: Print-friendly layout
- **PDF Export**: Save as PDF (via print dialog)

## Troubleshooting

### Benchmarks Not Loading

1. **Check file permissions**: Ensure web server can read benchmark files
   ```bash
   chmod 644 benchmarks/benchmark_results/*/*.json
   chmod 755 benchmarks/benchmark_results/*/
   ```

2. **Verify path**: Check plugin settings at Settings → Benchmark Visualizer

3. **Enable debug mode**: Add to `wp-config.php`:
   ```php
   define('WP_DEBUG', true);
   define('WP_DEBUG_LOG', true);
   ```

4. **Check cache**: Clear WordPress cache and browser cache

### Charts Not Displaying

1. Check browser console for JavaScript errors
2. Verify Chart.js is loading (check Network tab in browser dev tools)
3. Ensure no JavaScript conflicts with other plugins
4. Try disabling other plugins temporarily

### Performance Issues

1. **Reduce displayed benchmarks**: The plugin automatically limits to 30 items
2. **Increase cache duration**: Settings → Benchmark Visualizer → Cache Duration
3. **Use specific categories**: Instead of "all", use specific categories
4. **Enable server-side caching**: Use a WordPress caching plugin

## Advanced Configuration

### Custom Benchmark Directory

To use a custom benchmark directory, modify the plugin's `find_benchmark_directory()` function:

```php
// In themisdb-benchmark-visualizer.php
private function find_benchmark_directory() {
    $custom_path = '/your/custom/path/to/benchmarks';
    if (is_dir($custom_path)) {
        return $custom_path;
    }
    // Fallback to default paths...
}
```

### Add Custom Categories

Edit the `get_benchmark_files()` function in the plugin:

```php
'custom_category' => array(
    'bench_custom1.json',
    'bench_custom2.json',
),
```

Then add to the template dropdown:

```php
<option value="custom_category">Custom Category</option>
```

## Best Practices

1. **Use Caching**: Set appropriate cache duration (default: 24 hours)
2. **Category Pages**: Create separate pages for different audiences
3. **Mobile Testing**: Verify responsive design on mobile devices
4. **Performance**: Monitor page load times with benchmarks
5. **Updates**: Keep benchmark data current by running regular benchmarks

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: See README.md in plugin directory

## Version Information

- **Plugin Version**: 1.0.0
- **WordPress Required**: 5.0+
- **PHP Required**: 7.4+
- **Benchmark Files**: 19
- **Categories**: 10
- **Chart Types**: 3 (Bar, Line, Radar)
