# ThemisDB Graph Pattern Visualizer - WordPress Plugin

Interactive graph pattern visualization with filtering, searching, and color-coded node groups. Inspired by Neo4j Bloom.

## Features

### 🎨 Neo4j Bloom-Inspired Interface
- **Options Overlay Panel**: Slide-out panel with comprehensive controls
- **Search**: Find nodes by name with real-time highlighting
- **Group Filters**: Show/hide node categories with checkboxes
- **Color Customization**: Change group colors dynamically with color pickers
- **Node Details**: Click nodes to view properties and connections

### 📊 Interactive Visualization
- **Multiple Layouts**: Force-directed, hierarchical (top-down/left-right), circular
- **Zoom & Pan**: Intuitive navigation controls
- **Physics Simulation**: Natural graph layouts with adjustable parameters
- **Smooth Edges**: Curved connections for better visualization
- **Hover Effects**: Tooltips and highlighting

### 🎛️ Advanced Controls
- **Sliders**: Adjust node spacing and edge strength in real-time
- **Toggle Switches**: Enable/disable physics, labels, and more
- **Layout Selector**: Switch between different layout algorithms
- **Fullscreen Mode**: Immersive graph exploration

### 📥 Export Capabilities
- **PNG Export**: Download high-quality images
- **JSON Export**: Save graph data for analysis
- **Print Support**: Optimized for printing

### 🔍 Best Practices from Neo4j Bloom
- **Node Grouping**: Color-coded categories (Client, API, Query, Storage, etc.)
- **Search with Filtering**: Real-time node search with results highlighting
- **Expand/Collapse**: Double-click to expand (future feature)
- **Save Layouts**: Custom layout preservation (per user)
- **Legend**: Clear visual guide to node colors

## Installation

### Manual Installation

1. **Download or Copy the Plugin**
   ```bash
   cd /path/to/wordpress/wp-content/plugins/
   cp -r /path/to/ThemisDB/wordpress-plugin/graph-pattern-wordpress ./themisdb-graph-pattern
   ```

2. **Activate the Plugin**
   - Go to WordPress Admin → Plugins
   - Find "ThemisDB Graph Pattern Visualizer"
   - Click "Activate"

3. **Configure Settings** (Optional)
   - Go to Settings → Graph Pattern
   - Customize default layout, colors, and features

## Usage

### Basic Shortcode

```php
[themisdb_graph]
```

### Shortcode with Parameters

```php
[themisdb_graph data_source="default" layout="force_directed" height="600px" show_controls="true" show_overlay="true"]
```

### Available Parameters

| Parameter | Description | Default | Options |
|-----------|-------------|---------|---------|
| `data_source` | Data source identifier | `"default"` | Any string |
| `layout` | Layout algorithm | `"force_directed"` | `force_directed`, `hierarchical_top`, `hierarchical_left`, `circular` |
| `height` | Graph container height | `"600px"` | Any CSS height value |
| `show_controls` | Show control bar | `"true"` | `"true"`, `"false"` |
| `show_overlay` | Show options overlay | `"true"` | `"true"`, `"false"` |

## Examples

### Simple Visualization
```php
[themisdb_graph]
```

### Hierarchical Layout
```php
[themisdb_graph layout="hierarchical_top" height="800px"]
```

### Custom Data Source
```php
[themisdb_graph data_source="my_custom_graph"]
```

## User Interface

### Main Controls
- **Layout Selector**: Switch between different graph layouts
- **Zoom Controls**: + / - / Fit to screen
- **Fullscreen**: Immersive viewing mode
- **Node Counter**: Shows visible/total nodes

### Options Overlay Panel
Located on the right side of the graph:

1. **Search Box**
   - Type node names to search
   - Results are highlighted automatically
   - First result is focused

2. **Node Groups**
   - Checkboxes to show/hide groups
   - Color indicators for each group
   - Node count per group
   - Color pickers to customize colors

3. **Layout Settings**
   - Physics simulation toggle
   - Show/hide labels toggle
   - Node spacing slider (50-300)
   - Edge strength slider (1-10)

4. **Export**
   - Export as PNG button
   - Export as JSON button

5. **Node Details**
   - Appears when a node is clicked
   - Shows ID, group, level, connections

### Keyboard Shortcuts
- **Arrow Keys**: Navigate the graph
- **Scroll**: Zoom in/out
- **Ctrl + Click**: Multi-select nodes (built into vis-network)

## Technical Details

### Dependencies
- **vis-network.js**: Graph visualization library (v9.1.2)
- **jQuery**: WordPress standard
- **WordPress**: 5.0+
- **PHP**: 7.4+

### Data Structure

#### Nodes
```javascript
{
    id: 1,
    label: "Node Name",
    group: "category_id",
    level: 1,
    size: 25
}
```

#### Edges
```javascript
{
    from: 1,
    to: 2,
    label: "Connection",
    dashes: false
}
```

#### Groups
```javascript
{
    id: "category_id",
    label: "Category Name",
    color: "#2ea44f",
    visible: true
}
```

### Architecture
The plugin follows the established ThemisDB WordPress plugin pattern:

```
graph-pattern-wordpress/
├── themisdb-graph-pattern.php    # Main plugin file
├── assets/
│   ├── css/
│   │   └── graph-pattern.css     # Styles
│   └── js/
│       └── graph-pattern.js      # Client-side logic
├── templates/
│   ├── graph.php                 # Main visualization template
│   └── admin-settings.php        # Admin settings page
├── includes/                     # (Future: Additional PHP classes)
├── README.md                     # This file
├── LICENSE                       # MIT License
└── uninstall.php                # Cleanup on uninstall
```

## Sample Graph Data

The plugin includes a default ThemisDB architecture graph with:
- **7 Node Groups**: Client, API, Query, LLM, Transaction, Index, Storage
- **20 Nodes**: Representing ThemisDB components
- **22 Edges**: Component relationships
- **Color-coded**: Each group has a distinct color

## Customization

### Adding Custom Graph Data

To add your own graph data, modify the `get_graph_data()` method in the main plugin file:

```php
private function get_graph_data($data_source = 'default') {
    if ($data_source === 'my_custom_graph') {
        return array(
            'nodes' => [...],
            'edges' => [...],
            'groups' => [...]
        );
    }
    // Default graph data
    return array(...);
}
```

### Custom Colors

Colors can be customized:
1. In admin settings (Settings → Graph Pattern)
2. Via the color picker in the overlay panel
3. By modifying the default groups in PHP

### Layout Algorithms

Four layout algorithms are available:
1. **Force-Directed**: Natural clustering, best for general use
2. **Hierarchical Top-Down**: Organized layers, good for system architecture
3. **Hierarchical Left-Right**: Horizontal flow, good for process flows
4. **Circular**: Nodes arranged in a circle

## Performance

### Optimization Tips
- Limit nodes to 500 for smooth performance (configurable)
- Disable physics for very large graphs
- Use hierarchical layout for better organization of large graphs
- Hide less important groups to reduce visual complexity

### Browser Requirements
- Modern browsers (Chrome, Firefox, Safari, Edge)
- JavaScript enabled
- Canvas support

## Inspiration & References

This plugin is inspired by:
- **Neo4j Bloom**: Graph exploration and visualization tool
- **ThemisDB Architecture Diagrams Plugin**: Design pattern and styling
- **vis-network.js**: Powerful graph visualization library

## Roadmap

Future enhancements:
- [ ] Dynamic node expansion on double-click
- [ ] Save custom layouts per user
- [ ] Import custom graph JSON files
- [ ] More color schemes
- [ ] Node shape customization
- [ ] Edge type filtering
- [ ] Path finding between nodes
- [ ] Clustering for large graphs
- [ ] Time-based animations
- [ ] Multi-language support

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

MIT License - See LICENSE file for details

## Support

- **Repository**: https://github.com/makr-code/ThemisDB
- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Documentation**: See the main ThemisDB documentation

## Credits

- **ThemisDB Team**: Plugin development
- **vis-network.js**: Graph visualization library
- **Neo4j Bloom**: Interface inspiration

---

**Version**: 1.0.0  
**Last Updated**: January 2026
