# ThemisDB Benchmark Visualizer - WordPress Plugin

A WordPress plugin for interactive visualization of ThemisDB performance benchmarks. Compare ThemisDB performance against PostgreSQL, MongoDB, Neo4j, and other leading databases directly on your WordPress website.

## 📋 Overview

This plugin is designed following the **TCO Calculator** template pattern and provides comprehensive benchmark visualization capabilities with all the features needed for professional database performance comparison.

- **Shortcode-based Integration**: `[themisdb_benchmark_visualizer]`
- **Admin Settings Page**: Customize default values and data sources
- **Full Functionality**: All features for benchmark visualization
- **WordPress-optimized**: Uses WordPress best practices

## ✨ Features

### Comprehensive Benchmark Visualization
- 📊 **Interactive Charts**: Dynamic visualizations with Chart.js
- 🔍 **Multiple Metrics**: Latency, Throughput, Memory Usage
- 🎯 **10 Category Filters**: 
  - All Operations (comprehensive overview)
  - Vector Search & Embeddings (GNN embeddings support)
  - Graph Traversal & PageRank
  - Encryption & HSM (Hardware Security Module)
  - Compression
  - MVCC & Transactions (lock contention analysis)
  - Image Analysis (with latency metrics)
  - Advanced Patterns & AQL (hybrid queries, changefeeds)
  - GPU Backends (hardware acceleration)
  - Content Versioning & Indexing
- 📈 **Multiple Chart Types**: Bar, Line, and Radar charts
- 🔄 **Real-time Data**: Parses actual benchmark results from Google Benchmark JSON files
- 📊 **Statistics Dashboard**: View total benchmarks, files parsed, average/best/worst performance
- 💡 **Performance Insights**: Automated analysis with category-specific recommendations
- 🎨 **Smart Visualization**: Displays top 30 performers for optimal chart readability

### WordPress Integration
- 📝 **Shortcode**: Easy embedding via `[themisdb_benchmark_visualizer]`
- ⚙️ **Admin Panel**: Settings page under Settings → Benchmark Visualizer
- 🔗 **Plugin Action Links**: Direct access to settings from Plugins page
- 🧹 **Clean Uninstallation**: Automatic cleanup when plugin is deleted
- 🎨 **Theme-compatible**: Works with any WordPress theme
- 📱 **Responsive**: Optimized for all screen sizes

### Data Management
- 💾 **Real Benchmark Data**: Parses Google Benchmark JSON format from benchmark_results directory
- 📁 **19 Benchmark Files**: Comprehensive coverage of all ThemisDB features
- ⚡ **Smart Caching**: Configurable cache duration for optimal performance
- 🔄 **Auto-refresh**: Optional automatic data updates
- 📊 **Top Performers**: Displays best-performing benchmarks for clarity

### Export & Sharing
- 📥 **Export Functions**: CSV export
- 🖨️ **Print Support**: Optimized print layout
- 📄 **PDF Export**: Print-to-PDF functionality

## 🚀 Installation

### Method 1: Manual Installation

1. **Download the Plugin**
   ```bash
   cd /path/to/wordpress/wp-content/plugins/
   # Copy the plugin directory
   cp -r /path/to/ThemisDB/tools/benchmark-visualizer-wordpress ./themisdb-benchmark-visualizer
   ```

2. **Activate the Plugin**
   - Go to WordPress Admin → Plugins
   - Find "ThemisDB Benchmark Visualizer"
   - Click "Activate"

3. **Configure Settings**
   - Go to Settings → Benchmark Visualizer
   - Configure your preferences
   - Save changes

### Method 2: ZIP Installation

1. Create a ZIP file of the plugin directory
2. In WordPress Admin, go to Plugins → Add New
3. Click "Upload Plugin"
4. Choose the ZIP file and click "Install Now"
5. Activate the plugin

## 📖 Usage

### Basic Shortcode

Display the benchmark visualizer with default settings:

```php
[themisdb_benchmark_visualizer]
```

### Shortcode with Parameters

#### Filter by Category
```php
// Show only Vector Search benchmarks
[themisdb_benchmark_visualizer category="vector_search"]

// Show Graph Traversal benchmarks
[themisdb_benchmark_visualizer category="graph_traversal"]

// Show Encryption & HSM benchmarks
[themisdb_benchmark_visualizer category="encryption"]

// Show GPU Backend benchmarks
[themisdb_benchmark_visualizer category="gpu"]

// Show all available categories
[themisdb_benchmark_visualizer category="all"]
```

**Available Categories:**
- `all` - All Operations (8 key benchmark files)
- `vector_search` - Vector Search & GNN Embeddings
- `graph_traversal` - Graph algorithms including PageRank
- `encryption` - Encryption and HSM provider tests
- `compression` - Data compression benchmarks
- `transaction` - MVCC and lock contention tests
- `image_analysis` - AI-powered image processing
- `advanced` - Advanced patterns, AQL, changefeeds, hotspots
- `gpu` - GPU backend acceleration tests
- `content` - Content versioning and index rebuilding

#### Specify Metric
```php
// Display latency metrics
[themisdb_benchmark_visualizer metric="latency"]

// Display throughput metrics
[themisdb_benchmark_visualizer metric="throughput"]

// Display memory usage
[themisdb_benchmark_visualizer metric="memory"]
```

#### Choose Chart Type
```php
// Bar chart (default)
[themisdb_benchmark_visualizer chart_type="bar"]

// Line chart
[themisdb_benchmark_visualizer chart_type="line"]

// Radar chart for comparison
[themisdb_benchmark_visualizer chart_type="radar"]
```

#### Compare Specific Databases
```php
// Compare with PostgreSQL and MongoDB
[themisdb_benchmark_visualizer compare="postgresql,mongodb"]

// Compare with Neo4j
[themisdb_benchmark_visualizer compare="neo4j"]
```

#### Combined Parameters
```php
[themisdb_benchmark_visualizer 
    category="vector_search" 
    metric="latency" 
    chart_type="bar"
    compare="postgresql,mongodb"]
```

## ⚙️ Settings

Access plugin settings at **Settings → Benchmark Visualizer**:

### Data Source
- **Local Files**: Load benchmark data from plugin directory
- **GitHub Repository**: Fetch latest benchmarks from GitHub

### Default Values
- **Default Category**: Initial category to display
- **Default Metric**: Initial metric (latency, throughput, memory)
- **Default Comparison Databases**: Which databases to compare by default
- **Chart Theme**: Light or dark theme for charts

### Performance
- **Cache Duration**: How long to cache benchmark data (1 hour to 1 week)

## 🛠️ Technical Details

### File Structure

```
themisdb-benchmark-visualizer/
├── themisdb-benchmark-visualizer.php    # Main plugin file
├── assets/
│   ├── css/
│   │   └── benchmark-visualizer.css     # Styling (based on TCO Calculator)
│   └── js/
│       └── benchmark-visualizer.js      # JavaScript logic with Chart.js
├── templates/
│   ├── visualizer.php                   # Main template
│   └── admin-settings.php               # Admin settings page
├── data/
│   └── (benchmark data files)           # Local benchmark data
├── README.md                            # This file
└── LICENSE                              # MIT License
```

### Technologies Used

- **PHP**: WordPress plugin development (7.4+)
- **JavaScript**: ES5+ with jQuery
- **Chart.js**: Version 4.4.0 for visualizations
- **CSS3**: Modern, responsive styling
- **WordPress API**: Settings API, Shortcode API, AJAX

### Design Principles

Based on the **TCO Calculator** design pattern:

1. **Clean & Modern UI**: Minimalistic design with clear colors
2. **Responsive Layout**: Mobile-first approach
3. **Chart.js Integration**: Consistent visualizations
4. **Interactive Elements**: Filters, dropdowns, dynamic updates
5. **Export Functions**: CSV, PDF, Print support

### CSS Classes (Reusable)

```css
.themisdb-benchmark-wrapper    /* Main container */
.themisdb-section             /* Content sections */
.themisdb-chart-container     /* Chart areas */
.themisdb-btn-primary         /* Primary buttons */
.themisdb-btn-secondary       /* Secondary buttons */
.themisdb-filter-group        /* Filter controls */
.themisdb-results-table       /* Results display */
.themisdb-insights            /* Insights section */
```

### JavaScript API

```javascript
window.ThemisDBBenchmarks = {
    init: function() { /* Initialize visualizer */ },
    loadData: function() { /* Load benchmark data */ },
    renderChart: function() { /* Render Chart.js visualization */ },
    exportCSV: function() { /* Export data as CSV */ },
    exportPDF: function() { /* Export as PDF */ }
};
```

## 🔒 Security

- **Nonce Verification**: All AJAX requests verified with WordPress nonces
- **Capability Checks**: Admin functions require proper permissions
- **Input Sanitization**: All user inputs sanitized
- **Output Escaping**: All outputs properly escaped
- **XSS Protection**: Protection against cross-site scripting

## 🎨 Customization

### Custom Styling

Override plugin styles in your theme:

```css
/* In your theme's style.css */
.themisdb-benchmark-wrapper {
    /* Your custom styles */
}
```

### Custom Data

Add your own benchmark data in the `data/` directory using JSON format:

```json
{
    "labels": ["Operation 1", "Operation 2"],
    "datasets": [
        {
            "label": "ThemisDB",
            "data": [2.3, 1.5],
            "backgroundColor": "rgba(46, 164, 79, 0.8)"
        }
    ]
}
```

## 🐛 Troubleshooting

### Chart Not Displaying

1. Check browser console for JavaScript errors
2. Verify Chart.js is loading correctly
3. Ensure shortcode is properly formatted
4. Check WordPress debug log

### Data Not Loading

1. Verify data source settings
2. Check file permissions on data directory
3. Clear WordPress cache
4. Check AJAX request in browser network tab

### Styling Issues

1. Clear browser cache
2. Check for theme conflicts
3. Verify CSS file is loading
4. Check for CSS specificity issues

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This plugin is open source and available under the [MIT License](LICENSE).

## 🔗 Links

- **GitHub Repository**: [makr-code/wordpressPlugins](https://github.com/makr-code/wordpressPlugins)
- **Plugin Path**: `/tools/benchmark-visualizer-wordpress/`
- **Documentation**: See `/docs/` directory in main repository

## 📞 Support

For issues, questions, or feature requests:

1. Open an issue on [GitHub](https://github.com/makr-code/wordpressPlugins/issues)
2. Include plugin version and WordPress version
3. Provide detailed description and steps to reproduce

## 🗺️ Roadmap

### Future Enhancements

- [ ] Historical benchmark comparison
- [ ] Custom benchmark upload
- [ ] More chart types (scatter, bubble)
- [ ] Advanced filtering options
- [ ] RESTful API integration
- [ ] Multi-language support
- [ ] Gutenberg block support

## 📊 Version History

### Version 1.0.0 (Initial Release)
- Interactive benchmark visualization
- Multiple metrics support (latency, throughput, memory)
- Category filtering
- Chart.js integration
- CSV export
- WordPress admin integration
- Responsive design
- Based on TCO Calculator template

---

**Powered by [ThemisDB](https://github.com/makr-code/wordpressPlugins)** - The Multi-Model Database with Native LLM Integration
