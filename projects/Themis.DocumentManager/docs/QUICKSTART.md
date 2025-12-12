# DirectX 11 3D Graph Renderer - Quick Start Guide

## 🚀 Schnellstart

### Installation & Setup

```powershell
cd c:\VCC\themis\projects\Themis.DocumentManager
dotnet build Themis.DocumentManager.csproj -c Debug
# Output: ✅ 0 Fehler, 30 Warnungen
```

### Launch App

```powershell
dotnet run --project Themis.DocumentManager.csproj --configuration Debug
```

## 📊 3D Graph anschauen

1. **Starten Sie die App**
2. **Navigieren Sie zur GraphView3D** (in Ihrer UI)
3. **Automatisch:** 10-Node Example Graph wird geladen
4. **Interaktion:**
   - **Linke Maustaste + Drag** → Graph rotieren
   - **Mausrad** → Zoom in/out
   - **Statusbar** → FPS & Node/Edge Statistik

## 📁 Wichtige Dateien

| File | Purpose |
|------|---------|
| `Services/DirectX/DirectXCore.cs` | D3D11 Device & Shader Management |
| `Services/DirectX/MeshGenerator.cs` | Sphere/Cylinder Mesh Generation |
| `Services/DirectX/RenderingPipeline.cs` | Math, Camera, Lighting, Performance |
| `Services/DirectX/AdvancedDirectX3DGraphRenderer.cs` | Main Renderer Implementation |
| `Views/GraphView3D.xaml.cs` | WPF Integration & Render Loop |
| `Services/DirectX/DirectXServiceCollectionExtensions.cs` | DI Container Setup |

## 🔧 Verwendung in eigenem Code

### Renderer injizieren

```csharp
public MyView()
{
    InitializeComponent();
    _renderer = App.GetService<IDirectX3DGraphRenderer>()
        ?? throw new InvalidOperationException("Renderer not registered");
}
```

### Graph rendern

```csharp
// Create graph
var graph = new Graph
{
    Nodes = new List<GraphNode>
    {
        new GraphNode { Id = "1", Label = "Node 1", Position = Vector3D.Zero },
        new GraphNode { Id = "2", Label = "Node 2", Position = new Vector3D { X = 2, Y = 0, Z = 0 } }
    },
    Edges = new List<GraphEdge>
    {
        new GraphEdge { SourceNodeId = "1", TargetNodeId = "2" }
    }
};

// Render
_renderer.Initialize(hwnd, 1024, 768);
_renderer.Render(graph);
```

### Kamera kontrollieren

```csharp
_renderer.SetCameraPosition(0, 0, 10);  // Position
_renderer.Rotate(15f, 30f);              // Rotation (X, Y)
_renderer.Zoom(-0.5f);                   // Zoom
```

## 📐 Architektur-Übersicht

```
Application
    ↓
DI Container (AddDirectX3DServices)
    ├─ IDirectX3DGraphRenderer → AdvancedDirectX3DGraphRenderer
    ├─ IDirectXDevice → DirectXDevice
    ├─ IShaderManager → ShaderManager
    └─ IGraphVisualizationService → GraphVisualizationService
    
    ↓
GraphView3D (WPF UserControl)
    ├─ Initialize() → Get hwnd
    ├─ Render Loop (16ms = 60 FPS)
    └─ Mouse Events (Rotate/Zoom)
    
    ↓
AdvancedDirectX3DGraphRenderer
    ├─ Camera3D (View Matrix)
    ├─ Light3D (Diffuse + Ambient)
    ├─ RenderCommandQueue (Batching)
    ├─ MeshGenerator (Sphere/Cylinder)
    └─ RenderPerformanceMonitor (FPS)
```

## 📊 Performance Monitoring

```csharp
// Automatisch gesammelt in _performanceMonitor
// Output im Debug Window:
// "Frame: Nodes=10 Edges=18 Queue=28 | FPS: 60.0 | Min: 16.5ms | Max: 17.2ms"
```

## 🎨 Farben & Styling

```csharp
// Hex-Farben werden automatisch zu RGBA konvertiert
new GraphNode { Color = "#2196F3" }  // Blau
new GraphNode { Color = "#4CAF50" }  // Grün
new GraphEdge { Color = "#666666" }  // Grau

// Hex-Parsing: "#2196F3" → (0.133f, 0.588f, 0.953f, 1.0f)
```

## 🐛 Debugging

### Debug Output aktivieren

```csharp
System.Diagnostics.Debug.WriteLine("Message");
// Output im Visual Studio Debug Window sichtbar
```

### Häufige Fehler

| Error | Lösung |
|-------|--------|
| "Window not found" | InitializeDirectX wird vor Window.Loaded aufgerufen |
| "Null reference" | Renderer nicht registriert in DI Container |
| "Width/Height 0" | Window noch nicht gemessen, Task.Delay(100) hilft |
| "Missing Graph" | Ensure graph.Nodes.Count > 0 |

## 📚 Weitere Ressourcen

- `docs/ADVANCED_RENDERING_PIPELINE.md` - Komplette Technische Dokumentation
- `docs/DIRECTX11_3D_GRAPH_RENDERING.md` - DirectX Wrapper Details
- `docs/IMPLEMENTATION_STATUS.md` - Feature Übersicht & Status
- `Examples/DirectXRendererUsageExample.cs` - Code Beispiele

## 🔮 Nächste Schritte

1. **GPU Shaders** - HLSL für Lighting auf GPU
2. **Depth Testing** - Z-Buffer für korrektes Rendering
3. **Node Picking** - Ray-Casting für Selection
4. **Animation** - Smooth transitions
5. **OSM Integration** - Map Overlay

## ⚙️ Build-Optionen

```powershell
# Debug Build
dotnet build -c Debug

# Release Build (Optimiert)
dotnet build -c Release

# Clean & Build
dotnet clean && dotnet build -c Debug

# Build mit spezifischem Output
dotnet build -c Debug -o ./bin/output
```

## 📦 Dependencies

- **Framework:** .NET 8.0 Windows Desktop
- **UI:** WPF mit MVVM Toolkit
- **Graphics:** DirectX 11 (P/Invoke zu d3d11.dll)
- **Math:** Eigenimplementiert (keine externe Libs)
- **DI:** Microsoft.Extensions.DependencyInjection

## ✅ Checklist vor Production

- [ ] Build erfolgreich (0 Errors)
- [ ] DirectX Renderer lädt (Window Handle korrekt)
- [ ] Example Graph wird angezeigt
- [ ] Maus-Interaktion funktioniert (Rotate/Zoom)
- [ ] FPS >= 55 im Status Bar
- [ ] Kein Memory Leak (Performance Monitor)
- [ ] Skaliert mit großeren Graphs (100+ Nodes)

## 🆘 Troubleshooting

### Renderer zeigt nichts

```csharp
// 1. Überprüfe, dass InitializeDirectX vollendet ist
System.Diagnostics.Debug.WriteLine(_isInitialized ? "Ready" : "Not Ready");

// 2. Überprüfe Window Handle
IntPtr hwnd = new WindowInteropHelper(Window.GetWindow(this)).Handle;
System.Diagnostics.Debug.WriteLine($"HWND: {hwnd}");

// 3. Überprüfe Dimensionen
System.Diagnostics.Debug.WriteLine($"Size: {RenderSurface.ActualWidth}x{RenderSurface.ActualHeight}");
```

### Niedriges FPS

```csharp
// Überprüfe Performance Monitor Output:
// "FPS: 30.0 | Min: 30.0ms | Max: 35.2ms"
// → Render zu langsam
// → Überprüfe Mesh Caching
// → Überprüfe Graph Size
```

### Absturz beim Starten

```csharp
// Überprüfe DI Registration
services.AddDirectX3DServices();  // Muss in App.xaml.cs aufgerufen sein
```

## 📞 Support

Bei Fehlern oder Fragen:

1. Überprüfe Build Output (0 Errors?)
2. Schau Debug Window (F5 in Visual Studio)
3. Überprüfe Beispiele in `Examples/`
4. Referenziere Dokumentation in `docs/`

---

**Status:** ✅ Ready for Development
**Last Updated:** [Current Session]
**Build:** 0 Errors, 30 Warnings (harmless)
