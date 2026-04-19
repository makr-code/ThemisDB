> **Historischer Stand:** 2026-04-19

# Themis Impact Analysis Viewer - Implementation Guide

## Project Overview

The Themis Impact Analysis Viewer is a WPF-based desktop application for visualizing the results of FEM-based impact analysis performed by the ThemisDB GPU Impact Analysis Plugin. It provides both 2D and 3D visualization capabilities with support for multi-layer architectural analysis.

## Architecture

### Technology Stack

- **.NET 8.0 Windows** - Target framework
- **WPF** - Windows Presentation Foundation for UI
- **MVVM Pattern** - Using CommunityToolkit.Mvvm
- **MSAGL** - Microsoft Automatic Graph Layout for 2D visualization
- **SharpDX / HelixToolkit** - 3D DirectX visualization
- **HttpClient** - REST API communication with plugin

### Project Structure

```
Themis.ImpactAnalysisViewer/
├── Models/                          # Data models
│   ├── ImpactAnalysisResult.cs      # Analysis result container
│   ├── NodeImpact.cs                # Individual node impact data
│   ├── LayerMetadata.cs             # Layer configuration
│   └── DocumentChange.cs            # Change trigger data
├── ViewModels/                      # MVVM view models
│   └── MainViewModel.cs             # Main window view model
├── Views/                           # WPF views
│   └── MainWindow.xaml              # Main application window
├── Controls/                        # Custom WPF controls
│   ├── ImpactGraphControl.cs        # 2D graph visualization
│   └── MultiLayerGraph3DControl.cs  # 3D multi-layer view (TODO)
├── Services/                        # Backend services
│   └── ImpactAnalysisService.cs     # HTTP API client
└── Styles/                          # WPF resources
    ├── Colors.xaml                  # Color definitions
    └── Buttons.xaml                 # Button styles
```

## Implementation Status

### ✅ Completed (Phase 1)

**Core Infrastructure:**
- ✅ Project structure and build configuration
- ✅ NuGet package dependencies
- ✅ MVVM architecture setup
- ✅ Data models for impact analysis
- ✅ Layer metadata and configuration

**Services:**
- ✅ HTTP API client for plugin communication
- ✅ Impact analysis service with multi-layer support
- ✅ Connection testing and health checks

**UI Framework:**
- ✅ Main window layout
- ✅ Toolbar with view switching
- ✅ Connection management UI
- ✅ Configuration panel (left sidebar)
- ✅ Results summary display
- ✅ Status bar and loading overlay

**2D Visualization:**
- ✅ ImpactGraphControl custom control
- ✅ Heat-mapped impact coloring (6-level scale)
- ✅ Layer-specific node border coloring
- ✅ Node sizing based on impact score
- ✅ Edge rendering with weight visualization
- ✅ Cross-layer edge styling (dashed)
- ✅ Source node highlighting

**Styling:**
- ✅ Color scheme for heat maps
- ✅ Layer-specific colors (7 layers)
- ✅ UI color palette
- ✅ Button styles (primary, secondary, icon)

### 🚧 Pending (Phase 2-3)

**2D Visualization Enhancements:**
- ⏳ Interactive node selection
- ⏳ Zoom and pan controls
- ⏳ Node search functionality
- ⏳ Tooltip on hover
- ⏳ Layer filtering toggle
- ⏳ Export to PNG/SVG

**3D Visualization:**
- ⏳ DirectX/HelixToolkit integration
- ⏳ 3D sphere rendering
- ⏳ Camera controls (rotate, pan, zoom)
- ⏳ Phong lighting model
- ⏳ Layer height separation (Y-axis)
- ⏳ Animated transitions

**Data Integration:**
- ⏳ Value converters for XAML bindings
- ⏳ Real-time updates via WebSocket
- ⏳ Caching mechanism
- ⏳ Export functionality (PNG, PDF, JSON)

## Key Features

### Multi-Layer Support

The viewer fully supports the multi-layer architecture of ThemisDB:

**Supported Layers:**
- `document` - Document layer (Blue #4A90E2)
- `process` - BPMN/Workflow layer (Orange #F5A623)
- `api` - API specification layer (Green #7ED321)
- `database` - Database schema layer (Purple #BD10E0)
- `ui` - User interface layer (Cyan #50E3C2)
- `infrastructure` - Infrastructure layer (Light Green #B8E986)
- `custom` - Custom layers (Violet #9013FE)

**Layer Visualization:**
- Color-coded node borders
- Layer-specific filtering
- Cross-layer edge highlighting (dashed lines)
- Per-layer statistics display

### Heat Map Visualization

Impact scores are visualized using a 6-level heat map:

| Score Range | Color | Severity |
|------------|-------|----------|
| 0.00-0.25 | Green (#4CAF50) | Very Low |
| 0.25-0.50 | Light Green (#8BC34A) | Low |
| 0.50-0.70 | Amber (#FFC107) | Medium |
| 0.70-0.85 | Orange (#FF9800) | High |
| 0.85-0.95 | Red (#F44336) | Very High |
| 0.95-1.00 | Dark Red (#D32F2F) | Critical |

### Graph Layout

Uses Microsoft Automatic Graph Layout (MSAGL) with MDS (Multidimensional Scaling) algorithm:
- Automatic node positioning
- Edge routing with minimal crossings
- Hierarchical layout for directed graphs
- Force-directed layout for undirected graphs

## Usage Guide

### Running the Application

```bash
cd tools/Themis.ImpactAnalysisViewer
dotnet run
```

### Connecting to ThemisDB

1. Enter server URL (default: `http://localhost:8529`)
2. Click "Connect" button
3. Wait for green status indicator

### Analyzing Impact

1. Configure document change:
   - Document ID (e.g., `api/v2/payment/process`)
   - Change type (e.g., `breaking_change`)
   - Magnitude (0.0-1.0 slider)
   - Source layer (dropdown)

2. Configure analysis options:
   - Enable/disable hybrid search
   - Select target layers (checkboxes)

3. Click "Analyze Impact" button

4. View results:
   - 2D graph visualization
   - Summary statistics (right panel)
   - Per-layer breakdown

### Viewing Results

**2D View:**
- Nodes colored by impact score
- Node size proportional to impact
- Edges show propagation paths
- Source node marked with ⭐
- Cross-layer edges shown as dashed lines

**3D View (Coming Soon):**
- Layers separated vertically
- Rotatable camera
- Depth perception via lighting
- Animated impact propagation

## API Integration

### HTTP Endpoints

The viewer communicates with the GPU Impact Analysis Plugin via REST API:

**Analyze Impact:**
```
POST /api/analytics/gpu-impact/analyze
Content-Type: application/json

{
  "document_change": {
    "document_id": "api/v2/payment",
    "change_type": "breaking_change",
    "magnitude": 0.95
  },
  "config": {
    "use_hybrid_search": true,
    "max_depth": 5
  }
}
```

**Multi-Layer Analysis:**
```
POST /api/analytics/gpu-impact/analyze-multi-layer
Content-Type: application/json

{
  "document_change": {...},
  "target_layers": ["process", "api", "database"],
  "config": {...}
}
```

**Health Check:**
```
GET /api/analytics/gpu-impact/health
```

## Building and Deployment

### Development Build

```bash
dotnet build
```

### Release Build

```bash
dotnet publish -c Release -r win-x64 --self-contained
```

Output: `bin/Release/net8.0-windows/win-x64/publish/`

### Dependencies

All dependencies are managed via NuGet:

- `CommunityToolkit.Mvvm` - MVVM helpers
- `AutomaticGraphLayout.WpfGraphControl` - MSAGL graph visualization
- `SharpDX` / `SharpDX.Direct3D11` - DirectX 3D rendering
- `HelixToolkit.Wpf` - 3D visualization toolkit

## Next Steps (Phase 2)

### Week 1-2: Enhanced 2D Visualization
- [ ] Implement value converters for XAML
- [ ] Add interactive node selection
- [ ] Implement zoom/pan controls
- [ ] Add node search functionality
- [ ] Implement layer filtering

### Week 3-4: 3D Visualization Foundation
- [ ] Create MultiLayerGraph3DControl
- [ ] Implement DirectX/HelixToolkit integration
- [ ] Basic 3D sphere rendering
- [ ] Camera controls

### Week 5-6: 3D Advanced Features
- [ ] Layer height separation
- [ ] Phong lighting implementation
- [ ] Animated transitions
- [ ] Performance optimization

## Testing

### Unit Tests (TODO)
- Model serialization/deserialization
- Service HTTP communication
- ViewModel command logic
- Color calculation algorithms

### Integration Tests (TODO)
- End-to-end API communication
- Graph rendering performance
- Memory usage profiling

### Manual Testing Checklist
- [ ] Connect to local ThemisDB instance
- [ ] Analyze single-layer impact
- [ ] Analyze multi-layer impact
- [ ] Verify heat map colors
- [ ] Verify layer colors
- [ ] Check cross-layer edge styling
- [ ] Test view switching (2D/3D)
- [ ] Test export functionality

## Troubleshooting

### Common Issues

**Connection Failed:**
- Verify ThemisDB is running on `http://localhost:8529`
- Check firewall settings
- Verify plugin is loaded and initialized

**Graph Not Rendering:**
- Check that analysis returned results
- Verify MSAGL package is installed
- Check for exceptions in Output window

**Performance Issues:**
- Limit max nodes to 10,000
- Enable GPU rendering (config setting)
- Use hybrid search to reduce graph size

## Future Enhancements

- **Real-time Updates:** WebSocket support for live analysis
- **VR Support:** OpenXR integration for immersive visualization
- **Custom Shaders:** Advanced visual effects
- **Performance Profiling:** Built-in performance metrics overlay
- **Multi-Monitor:** Span visualization across displays

## References

<!-- TODO: verify against current source – the following enterprise docs are referenced but not yet found in the repo -->
- GPU Impact Analysis Plugin Docs – `docs/enterprise/gpu_impact_analysis_plugin.md` (not yet available)
- Multi-Layer Analysis Guide – `docs/enterprise/gpu_impact_analysis_multi_layer.md` (not yet available)
- Visualization Specification – `docs/enterprise/gpu_impact_analysis_visualization.md` (not yet available)
- [MSAGL Documentation](https://github.com/microsoft/automatic-graph-layout)
- [HelixToolkit Documentation](https://github.com/helix-toolkit/helix-toolkit)

## License

ThemisDB Enterprise - Internal Tool
Copyright (c) 2024-2025
