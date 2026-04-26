# Themis Impact Analysis Viewer

## Overview

The Themis Impact Analysis Viewer is a WPF-based 3D visualization tool for analyzing the impact of document changes across multiple architectural layers (document, process, API, database, UI, infrastructure).

## Features

### 2D Graph Visualization
- Heat-mapped impact coloring (green → yellow → orange → red)
- Node sizing based on impact score
- Interactive zooming and panning
- Configurable node labels with auto-positioning
- Layer-specific coloring and filtering

### Multi-Layer Support
- Visualize cross-layer dependencies
- Layer-specific filtering (show/hide layers)
- Color-coded layer representation
- Cross-layer edge highlighting
- Layer transition statistics

### 3D Visualization (Optional)
- 3D sphere rendering with DirectX/HelixToolkit
- Camera controls (rotate, pan, zoom)
- Phong lighting model
- Layer height separation (Y-axis)
- Animated transitions

### Analysis Features
- Real-time impact score calculation
- Critical path highlighting
- Node search and selection
- Export to PNG/PDF
- Performance metrics display

## Architecture

```
Themis.ImpactAnalysisViewer/
├── Models/              # Data models for impact analysis
│   ├── ImpactAnalysisResult.cs
│   ├── NodeImpact.cs
│   ├── LayerMetadata.cs
│   └── DocumentChange.cs
├── ViewModels/          # MVVM view models
│   ├── MainViewModel.cs
│   ├── Graph2DViewModel.cs
│   └── Graph3DViewModel.cs
├── Views/               # WPF views
│   ├── MainWindow.xaml
│   ├── Graph2DView.xaml
│   └── Graph3DView.xaml
├── Controls/            # Custom WPF controls
│   ├── ImpactGraphControl.cs
│   └── MultiLayerGraph3DControl.cs
├── Services/            # Backend communication
│   ├── ImpactAnalysisService.cs
│   └── PluginApiClient.cs
└── Styles/              # WPF resources
    ├── Colors.xaml
    └── Buttons.xaml
```

## Usage

### Running the Tool

```bash
cd tools/Themis.ImpactAnalysisViewer
dotnet run
```

### Connecting to ThemisDB

The tool connects to the GPU Impact Analysis Plugin via HTTP API:

```csharp
var service = new ImpactAnalysisService("http://localhost:8529");
var change = new DocumentChange
{
    DocumentId = "api/v2/payment/process",
    ChangeType = "breaking_change",
    Magnitude = 0.95,
    SourceLayer = "api"
};

var result = await service.AnalyzeMultiLayerImpactAsync(change, 
    new[] { "process", "api", "database", "ui" });
```

### Visualizing Results

The 2D view automatically renders:
- Affected nodes colored by impact score
- Edges showing propagation paths
- Layer indicators
- Interactive tooltips

The 3D view provides:
- Spatial layer separation (Y-axis)
- Rotatable camera
- Depth perception via lighting
- Animated impact wave

## Dependencies

- .NET 8.0 Windows
- WPF Framework
- MSAGL (Microsoft Automatic Graph Layout)
- SharpDX (Direct3D 11)
- HelixToolkit.Wpf (3D rendering)
- CommunityToolkit.Mvvm

## Configuration

Edit `appsettings.json`:

```json
{
  "ThemisDB": {
    "BaseUrl": "http://localhost:8529",
    "Database": "_system",
    "PluginEndpoint": "/api/analytics/gpu-impact"
  },
  "Visualization": {
    "DefaultView": "2D",
    "EnableGPURendering": true,
    "MaxNodes": 10000,
    "AnimationSpeed": 1.0
  }
}
```

## Building

```bash
dotnet build
dotnet publish -c Release -r win-x64 --self-contained
```

## Testing

```bash
dotnet test
```

## Future Enhancements

- Real-time streaming updates
- VR support via OpenXR
- Multi-monitor support
- Custom shader effects
- Performance profiling overlay

## License

ThemisDB Enterprise - Internal Tool
