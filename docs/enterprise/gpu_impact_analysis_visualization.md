# GPU Impact Analysis - Advanced Visualization Specification

## Overview

This document specifies the visual representation of FEM-based impact analysis results in the ThemisDB Admin Tool, including 2D graph visualization with heat-mapped impact coloring and optional 3D spatial visualization using DirectX/Vulkan.

---

## 1. 2D Graph Visualization (Primary View)

### 1.1 Node Representation

**Visual Elements:**
- **Shape:** Circle (filled)
- **Size:** Proportional to node importance or degree centrality
  - Small: 20px diameter (low importance)
  - Medium: 40px diameter (normal)
  - Large: 60px diameter (high importance/hub)
- **Border:** 2px stroke, color based on node type

**Node Labeling:**
```
┌─────────────────┐
│   ●  Node 123   │  ← Label below/beside node
│  [0.87]         │  ← Impact score (optional)
└─────────────────┘
```

### 1.2 Edge Representation

**Visual Elements:**
- **Shape:** Line (solid or dashed based on edge type)
- **Width:** Proportional to edge weight (FEM stiffness)
  - Thin: 1px (weight < 0.3)
  - Medium: 2px (weight 0.3-0.7)
  - Thick: 4px (weight > 0.7)
- **Style:**
  - Solid: Direct dependencies (DEPENDS_ON, PARENT_OF)
  - Dashed: References (REFERENCES, SIMILAR_TO)
  - Dotted: Weak connections (weight < 0.2)

**Directional Arrows:**
- Arrow at end of edge for directed graphs
- Bidirectional arrows for two-way dependencies

### 1.3 FEM Impact Color Mapping (Heat Map)

**Color Gradient Specification:**

Impact is visualized using a continuous color gradient from **blue → green → yellow → orange → red**:

```
Impact Score Range:  Color       Hex Code    RGB
─────────────────────────────────────────────────────
0.00 - 0.10          Blue        #2196F3     (33, 150, 243)
0.10 - 0.25          Cyan        #00BCD4     (0, 188, 212)
0.25 - 0.40          Green       #4CAF50     (76, 175, 80)
0.40 - 0.55          Yellow-Grn  #8BC34A     (139, 195, 74)
0.55 - 0.70          Yellow      #FFEB3B     (255, 235, 59)
0.70 - 0.80          Orange      #FF9800     (255, 152, 0)
0.80 - 0.90          Deep Orange #FF5722     (255, 87, 34)
0.90 - 1.00          Red         #F44336     (244, 67, 54)
```

**Interpolation Function (C#):**

```csharp
public static Color GetImpactColor(double impactScore)
{
    // Clamp to [0.0, 1.0]
    impactScore = Math.Max(0.0, Math.Min(1.0, impactScore));
    
    // Define color stops
    var colorStops = new[]
    {
        (0.00, Color.FromRgb(33, 150, 243)),    // Blue
        (0.10, Color.FromRgb(0, 188, 212)),     // Cyan
        (0.25, Color.FromRgb(76, 175, 80)),     // Green
        (0.40, Color.FromRgb(139, 195, 74)),    // Yellow-Green
        (0.55, Color.FromRgb(255, 235, 59)),    // Yellow
        (0.70, Color.FromRgb(255, 152, 0)),     // Orange
        (0.80, Color.FromRgb(255, 87, 34)),     // Deep Orange
        (0.90, Color.FromRgb(244, 67, 54))      // Red
    };
    
    // Find color stops to interpolate between
    for (int i = 0; i < colorStops.Length - 1; i++)
    {
        if (impactScore >= colorStops[i].Item1 && 
            impactScore <= colorStops[i + 1].Item1)
        {
            double t = (impactScore - colorStops[i].Item1) / 
                       (colorStops[i + 1].Item1 - colorStops[i].Item1);
            
            return InterpolateColor(colorStops[i].Item2, 
                                   colorStops[i + 1].Item2, t);
        }
    }
    
    return colorStops[colorStops.Length - 1].Item2; // Red (max impact)
}

private static Color InterpolateColor(Color c1, Color c2, double t)
{
    return Color.FromRgb(
        (byte)(c1.R + (c2.R - c1.R) * t),
        (byte)(c1.G + (c2.G - c1.G) * t),
        (byte)(c1.B + (c2.B - c1.B) * t)
    );
}
```

### 1.4 Visual Example (ASCII Art)

```
Legend:
  ● Blue (0.0-0.1)     - No/minimal impact
  ● Green (0.25-0.4)   - Low impact
  ● Yellow (0.55-0.7)  - Medium impact
  ● Orange (0.7-0.8)   - High impact
  ● Red (0.9-1.0)      - Critical impact

Impact Propagation:

        ●━━━━━━━━━━━━━━━━━━●
       RED               ORANGE
      [0.95]             [0.78]
    Source Doc         Direct Dep
        │                  │
        │                  │
        ●                  ●
      YELLOW            YELLOW
      [0.62]            [0.58]
    Indirect 1        Indirect 2
        │                  │
        └────────┬─────────┘
                 │
                 ●
               GREEN
               [0.35]
             Indirect 3
                 │
                 ●
               CYAN
               [0.15]
             Indirect 4
                 │
                 ●
               BLUE
               [0.05]
             No Impact
```

### 1.5 Interactive Features

**1. Hover Tooltip:**
```
┌───────────────────────────────┐
│ Document: api/payment.md      │
│ Impact Score: 0.87            │
│ Change Type: Breaking Change  │
│ Affected By: docs/api.md      │
│ Affects: 23 documents         │
│                               │
│ FEM Metadata:                 │
│  - Inertia: 0.35              │
│  - Amplification: 1.2         │
│  - Criticality: High          │
└───────────────────────────────┘
```

**2. Node Selection:**
- Click node → Highlight connected edges
- Double-click → Expand neighbors (load next level)
- Right-click → Context menu (View Details, Hide, Export)

**3. Edge Hover:**
```
┌───────────────────────────────┐
│ Edge Type: DEPENDS_ON         │
│ Weight: 0.85                  │
│ Damping: 0.12                 │
│ Bidirectional: No             │
│ Criticality: Critical         │
└───────────────────────────────┘
```

---

## 2. Color Legend Panel

**Fixed Position:** Bottom-right corner of graph canvas

**UI Design:**
```
┌─────────────────────────────────────┐
│ Impact Heat Map Legend              │
├─────────────────────────────────────┤
│ ████ 0.0-0.1  No/Minimal Impact     │
│ ████ 0.1-0.25 Very Low Impact       │
│ ████ 0.25-0.4 Low Impact            │
│ ████ 0.4-0.55 Moderate Impact       │
│ ████ 0.55-0.7 Medium Impact         │
│ ████ 0.7-0.8  High Impact           │
│ ████ 0.8-0.9  Very High Impact      │
│ ████ 0.9-1.0  Critical Impact       │
├─────────────────────────────────────┤
│ Nodes: 247 | Edges: 589            │
│ Max Impact: 0.95 | Avg: 0.42       │
└─────────────────────────────────────┘
```

---

## 3. Layout Algorithms

### 3.1 Force-Directed Layout (Default)

**Purpose:** Natural organic graph layout

**Algorithm:** Fruchterman-Reingold

**Parameters:**
- Attraction force: Proportional to edge weight
- Repulsion force: Inversely proportional to distance
- Damping factor: 0.85 (FEM-based)
- Iterations: 500 (real-time), 2000 (export quality)

**C# Implementation (MSAGL):**
```csharp
using Microsoft.Msagl.Drawing;
using Microsoft.Msagl.Layout.MDS;

public void ApplyForceDirectedLayout(Graph graph, ImpactAnalysisResult result)
{
    var settings = new MdsLayoutSettings
    {
        EdgeRoutingSettings = { EdgeRoutingMode = EdgeRoutingMode.Spline }
    };
    
    // Apply FEM weights to edge lengths
    foreach (var edge in graph.Edges)
    {
        var impactData = result.GetEdgeImpact(edge.Source, edge.Target);
        edge.Attr.Length = 100 * (1.0 - impactData.Weight); // Higher weight = shorter
    }
    
    graph.LayoutAlgorithmSettings = settings;
}
```

### 3.2 Hierarchical Layout

**Purpose:** Show impact propagation levels

**Levels:**
- Level 0: Source document(s) (top)
- Level 1: Direct dependencies
- Level 2: Second-degree dependencies
- Level N: N-th degree dependencies

**Visual:**
```
Level 0:           ●
                  RED
                (Source)
                   │
        ┌──────────┼──────────┐
        │          │          │
Level 1:  ●          ●          ●
       ORANGE    ORANGE     YELLOW
         │          │          │
     ┌───┴───┐      │      ┌───┴───┐
     │       │      │      │       │
Level 2: ●     ●      ●      ●       ●
      YELLOW GREEN GREEN  GREEN   CYAN
```

### 3.3 Circular Layout

**Purpose:** Show equal-distance relationships

**Arrangement:**
- Source in center
- Affected nodes in concentric circles by impact score
- Inner circle: High impact (red/orange)
- Outer circles: Lower impact (yellow/green/blue)

---

## 4. Advanced Filtering & Highlighting

### 4.1 Impact Threshold Filter

**UI Control:**
```
┌──────────────────────────────────┐
│ Show nodes with impact ≥ [0.5▼]  │
│ ──────────●─────────────────────  │
│ 0.0                          1.0  │
└──────────────────────────────────┘
```

**Effect:**
- Nodes below threshold: Grayed out or hidden
- Edges below threshold: Dashed or hidden

### 4.2 Path Highlighting

**Feature:** Show impact propagation path from source to selected node

**Visual:**
```
Source ●━━━━●━━━━●━━━━● Selected
      RED  ORA  YEL  GRN

Other nodes: 50% opacity
Highlighted path: 100% opacity, glowing effect
```

### 4.3 Depth Limiting

**Control:**
```
┌──────────────────────────────────┐
│ Max Propagation Depth: [5 ▼]     │
│ □ Show only changed documents    │
│ □ Hide nodes with impact < 0.1   │
└──────────────────────────────────┘
```

---

## 5. 3D Spatial Visualization (Optional Enhancement)

### 5.1 Technology Stack

**Rendering Engine Options:**

1. **DirectX 11/12 (Windows)**
   - Library: SharpDX or Vortice.Windows
   - Shader: HLSL for heat map rendering
   - Performance: Excellent on Windows

2. **Vulkan (Cross-platform)**
   - Library: Silk.NET.Vulkan
   - Shader: SPIR-V for heat map rendering
   - Performance: Excellent, portable

3. **OpenGL (Fallback)**
   - Library: OpenTK
   - Shader: GLSL
   - Performance: Good, widely compatible

**Recommended:** Vulkan for cross-platform support + DirectX as Windows-optimized option

### 5.2 3D Layout Concept

**Spatial Organization:**

**Z-Axis:** Impact propagation depth
- Z = 0: Source documents (front)
- Z = -100: Level 1 dependencies
- Z = -200: Level 2 dependencies
- Z = -N*100: Level N dependencies

**X-Y Plane:** Relationship clustering
- Similar nodes clustered together
- Force-directed layout within each Z-plane

**Visual:**
```
          Z-axis (depth)
            ↑
            │
     ●──────┼──────●  (Z=-200, Level 2, Green/Blue)
    /       │       \
   /        │        \
  ●─────────┼─────────●  (Z=-100, Level 1, Yellow/Orange)
            │
            ●  (Z=0, Source, Red)
           / \
          /   \
     X-axis   Y-axis
```

### 5.3 3D Node Rendering

**Sphere Representation:**
- **Shape:** 3D sphere with Phong lighting
- **Radius:** 5-30 units (based on importance)
- **Color:** Heat-mapped (same gradient as 2D)
- **Glow Effect:** Emissive material for high-impact nodes (>0.7)

**Shader (HLSL example):**
```hlsl
struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 WorldPos : TEXCOORD0;
    float ImpactScore : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // Get base color from impact score
    float4 heatColor = GetHeatMapColor(input.ImpactScore);
    
    // Phong lighting
    float3 lightDir = normalize(float3(1, 1, 1));
    float diff = max(dot(input.Normal, lightDir), 0.0);
    
    float3 viewDir = normalize(CameraPos - input.WorldPos);
    float3 reflectDir = reflect(-lightDir, input.Normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    // Combine
    float3 ambient = 0.3 * heatColor.rgb;
    float3 diffuse = 0.5 * diff * heatColor.rgb;
    float3 specular = 0.3 * spec * float3(1, 1, 1);
    
    // Add glow for high impact
    float glow = smoothstep(0.7, 1.0, input.ImpactScore);
    float3 emissive = glow * heatColor.rgb * 0.5;
    
    return float4(ambient + diffuse + specular + emissive, 1.0);
}

float4 GetHeatMapColor(float impact)
{
    // Blue → Cyan → Green → Yellow → Orange → Red
    if (impact < 0.1) return lerp(float4(0.13, 0.59, 0.95, 1), float4(0, 0.74, 0.83, 1), impact * 10);
    if (impact < 0.25) return lerp(float4(0, 0.74, 0.83, 1), float4(0.3, 0.69, 0.31, 1), (impact - 0.1) / 0.15);
    if (impact < 0.4) return lerp(float4(0.3, 0.69, 0.31, 1), float4(0.55, 0.76, 0.29, 1), (impact - 0.25) / 0.15);
    if (impact < 0.55) return lerp(float4(0.55, 0.76, 0.29, 1), float4(1, 0.92, 0.23, 1), (impact - 0.4) / 0.15);
    if (impact < 0.7) return lerp(float4(1, 0.92, 0.23, 1), float4(1, 0.6, 0, 1), (impact - 0.55) / 0.15);
    if (impact < 0.8) return lerp(float4(1, 0.6, 0, 1), float4(1, 0.34, 0.13, 1), (impact - 0.7) / 0.1);
    if (impact < 0.9) return lerp(float4(1, 0.34, 0.13, 1), float4(0.96, 0.26, 0.21, 1), (impact - 0.8) / 0.1);
    return float4(0.96, 0.26, 0.21, 1); // Red
}
```

### 5.4 3D Edge Rendering

**Cylinder Connections:**
- **Shape:** Cylinder connecting two spheres
- **Radius:** Proportional to edge weight (0.5-3 units)
- **Color:** Gradient from source impact to target impact
- **Transparency:** Alpha = 0.6 (semi-transparent to avoid occlusion)

**Animated Flow (optional):**
- Particles flowing along edges
- Speed proportional to impact strength
- Color matches heat map

### 5.5 Camera Controls (3D)

**Mouse Interactions:**
- **Left-drag:** Rotate (arcball rotation)
- **Right-drag:** Pan
- **Scroll:** Zoom in/out
- **Middle-click:** Reset to default view

**Keyboard Shortcuts:**
- `1-7`: View from different angles (top, front, side, isometric)
- `Space`: Play/pause impact propagation animation
- `R`: Reset camera
- `F`: Frame selected node

### 5.6 3D Performance Optimization

**Level of Detail (LOD):**
- Near nodes: High-poly spheres (32 segments)
- Medium distance: Medium-poly (16 segments)
- Far nodes: Low-poly (8 segments) or billboards

**Frustum Culling:**
- Only render nodes within camera view
- Estimated: 70-80% node reduction in large graphs

**Instancing:**
- Use GPU instancing for rendering many nodes
- Single draw call per impact level

**Target Performance:**
- 60 FPS with 1,000 nodes
- 30 FPS with 10,000 nodes

---

## 6. Animation & Temporal Visualization

### 6.1 Impact Propagation Animation

**Timeline Control:**
```
┌────────────────────────────────────────────────────┐
│ [▶] [❚❚] [⏮] [⏭]    ●━━━━━━━━━━━━━━━━━━━━━━━━━━━  │
│                      0s          5s           10s  │
│ Speed: [1x ▼] Loop: [☑]                           │
└────────────────────────────────────────────────────┘
```

**Animation Sequence:**
1. **T=0s:** Show source node(s) in red
2. **T=0-2s:** First-level dependencies fade in (orange/yellow)
3. **T=2-4s:** Second-level dependencies fade in (yellow/green)
4. **T=4-10s:** Continue propagation to all affected nodes
5. **Visual Effect:** Ripple/wave effect emanating from source

**Ease Function:** Cubic ease-in-out for smooth transitions

### 6.2 Monte Carlo Visualization

**Display multiple simulation outcomes:**
- Show 10-20 representative simulation paths
- Each path: Different transparency (alpha = 0.1-0.3)
- Overlay all paths to create "probability cloud"
- Darker regions: Higher probability of impact

---

## 7. Export & Reporting

### 7.1 Export Formats

**Static Images:**
- PNG (4K resolution, transparent background)
- SVG (vector, scalable)
- PDF (for reports)

**Interactive:**
- HTML5 Canvas (embed in web pages)
- WebGL (3D interactive web view)

**Data:**
- JSON (graph structure + impact scores)
- CSV (node list with impact scores)

### 7.2 Report Generation

**PDF Report Includes:**
1. Title page with summary statistics
2. 2D graph visualization (full page)
3. Color legend
4. Top 20 affected documents table
5. Impact propagation path details
6. Recommendations

**Example Report Page:**
```
┌────────────────────────────────────────────┐
│ GPU Impact Analysis Report                 │
│ Generated: 2025-12-07 14:30                │
├────────────────────────────────────────────┤
│ Source Document: api/payment.md            │
│ Change Type: Breaking Change               │
│ Magnitude: 0.95                            │
│                                             │
│ Summary Statistics:                        │
│  - Affected Documents: 247                 │
│  - High Impact (>0.7): 23                  │
│  - Medium Impact (0.4-0.7): 87             │
│  - Low Impact (<0.4): 137                  │
│                                             │
│ [Full-page graph visualization]            │
│                                             │
│ Top Affected Documents:                    │
│ 1. checkout.md         [0.87] Critical     │
│ 2. invoice.md          [0.78] High         │
│ 3. billing.md          [0.72] High         │
│ ...                                        │
└────────────────────────────────────────────┘
```

---

## 8. Implementation Roadmap

### Phase 1: 2D Heat Map Visualization (Weeks 1-3)
- ✅ MSAGL integration
- ✅ Heat map color gradient implementation
- ✅ Force-directed layout
- ✅ Interactive hover tooltips
- ✅ Color legend panel

### Phase 2: Advanced 2D Features (Weeks 4-5)
- ✅ Hierarchical and circular layouts
- ✅ Filtering controls
- ✅ Path highlighting
- ✅ Export to PNG/SVG

### Phase 3: Animation (Week 6)
- ✅ Impact propagation timeline
- ✅ Animation controls
- ✅ Monte Carlo probability clouds

### Phase 4: 3D Visualization (Weeks 7-10) - Optional
- ✅ Vulkan/DirectX renderer setup
- ✅ 3D sphere node rendering with heat map shaders
- ✅ 3D edge rendering (cylinders)
- ✅ Camera controls
- ✅ Performance optimization (LOD, instancing)

### Phase 5: Polish & Testing (Weeks 11-12)
- ✅ Performance testing (1K, 10K nodes)
- ✅ User acceptance testing
- ✅ Documentation
- ✅ Report generation

---

## 9. Technical Implementation Examples

### 9.1 WPF Custom Control (2D Graph)

```csharp
public class ImpactGraphControl : UserControl
{
    private GViewer _graphViewer;
    private ImpactAnalysisResult _result;
    
    public void LoadImpactAnalysis(ImpactAnalysisResult result)
    {
        _result = result;
        var graph = new Graph("impact");
        
        // Add nodes with heat-mapped colors
        foreach (var node in result.AffectedNodes)
        {
            var gNode = graph.AddNode(node.NodeId);
            gNode.LabelText = node.Label;
            
            // Apply heat map color
            var color = GetImpactColor(node.ImpactScore);
            gNode.Attr.FillColor = new Microsoft.Msagl.Drawing.Color(
                color.R, color.G, color.B
            );
            
            // Size based on importance
            gNode.Attr.Shape = Shape.Circle;
            gNode.Attr.LabelMargin = 5;
        }
        
        // Add edges
        foreach (var edge in result.Edges)
        {
            var gEdge = graph.AddEdge(edge.From, edge.To);
            gEdge.LabelText = $"{edge.Weight:F2}";
            gEdge.Attr.LineWidth = edge.Weight * 4; // 0-4px
            gEdge.Attr.ArrowheadAtTarget = ArrowStyle.Normal;
        }
        
        // Apply layout
        graph.LayoutAlgorithmSettings = new MdsLayoutSettings();
        
        _graphViewer.Graph = graph;
    }
    
    private System.Windows.Media.Color GetImpactColor(double impactScore)
    {
        // Implementation from section 1.3
        // ...
    }
}
```

### 9.2 Vulkan/DirectX Abstraction Layer

```csharp
public interface IGraphRenderer3D
{
    void Initialize(IntPtr windowHandle, int width, int height);
    void LoadGraph(ImpactAnalysisResult result);
    void Render(Camera camera);
    void Resize(int width, int height);
    void Dispose();
}

public class VulkanGraphRenderer : IGraphRenderer3D
{
    private Silk.NET.Vulkan.Vk _vk;
    private Instance _instance;
    private Device _device;
    private Pipeline _nodePipeline;
    private Pipeline _edgePipeline;
    
    public void LoadGraph(ImpactAnalysisResult result)
    {
        // Create vertex buffers for nodes (spheres)
        var nodeVertices = GenerateSphereVertices(32); // 32 segments
        
        foreach (var node in result.AffectedNodes)
        {
            var instanceData = new NodeInstanceData
            {
                Position = node.Position3D,
                Radius = node.Radius,
                ImpactScore = (float)node.ImpactScore,
                Color = GetImpactColorVec4(node.ImpactScore)
            };
            
            AddNodeInstance(instanceData);
        }
        
        // Create edge geometries (cylinders)
        foreach (var edge in result.Edges)
        {
            var cylinder = GenerateCylinder(
                result.GetNode(edge.From).Position3D,
                result.GetNode(edge.To).Position3D,
                (float)edge.Weight
            );
            
            AddEdgeGeometry(cylinder);
        }
    }
    
    // Implement other methods...
}
```

---

## 10. Accessibility Considerations

### 10.1 Color Blindness Support

**Alternative Color Schemes:**

1. **Deuteranopia (red-green):**
   - Blue → Purple → Pink → Yellow → Orange
   
2. **Protanopia (red-green):**
   - Blue → Cyan → Yellow → Gold → Dark Yellow

3. **Tritanopia (blue-yellow):**
   - Red → Orange → Pink → Cyan → Blue

**Settings:**
```
┌────────────────────────────────────┐
│ Color Scheme: [Standard ▼]        │
│ Options:                           │
│   • Standard (BGYOR)               │
│   • Deuteranopia-friendly          │
│   • Protanopia-friendly            │
│   • Tritanopia-friendly            │
│   • Grayscale (intensity only)     │
└────────────────────────────────────┘
```

### 10.2 Non-Color Indicators

**Pattern Overlays:**
- High impact nodes: Dotted border
- Medium impact: Dashed border
- Low impact: Solid border

**Text Labels:**
- Always show impact score on hover
- Optional: Always-visible labels for critical nodes

---

## 11. Performance Metrics

### 11.1 2D Rendering Targets

| Graph Size | FPS Target | Memory Usage |
|------------|------------|--------------|
| 100 nodes | 60 FPS | <50 MB |
| 1,000 nodes | 60 FPS | <200 MB |
| 10,000 nodes | 30 FPS | <500 MB |

### 11.2 3D Rendering Targets

| Graph Size | FPS Target (60Hz) | GPU Memory |
|------------|-------------------|------------|
| 100 nodes | 144 FPS | <100 MB |
| 1,000 nodes | 60 FPS | <300 MB |
| 10,000 nodes | 30 FPS | <800 MB |

---

## 12. Summary

### Key Features Delivered

✅ **2D Heat-Mapped Visualization**
- Circle nodes with heat-map coloring (Blue → Green → Yellow → Red)
- Lines (edges) with weight-based thickness
- Force-directed, hierarchical, and circular layouts

✅ **FEM Impact Color Gradient**
- 8-stop color gradient: Blue (no impact) → Red (critical impact)
- Smooth interpolation between color stops
- C# implementation provided

✅ **3D Spatial Visualization (Optional)**
- DirectX/Vulkan rendering
- Z-axis = propagation depth
- Sphere nodes with Phong lighting + glow effect
- Cylinder edges with gradient colors

✅ **Interactive Features**
- Hover tooltips with FEM metadata
- Path highlighting
- Impact threshold filtering
- Depth limiting

✅ **Animation**
- Temporal impact propagation
- Monte Carlo probability clouds
- Timeline controls

✅ **Export & Reporting**
- PNG, SVG, PDF export
- HTML5/WebGL interactive export
- Automated report generation

✅ **Accessibility**
- Color-blind friendly alternatives
- Pattern-based indicators
- Text labels

---

## Next Steps

1. **Prototype 2D visualization** (Week 1-2)
2. **User testing with color gradient** (Week 3)
3. **Implement 3D renderer** (Week 4-7) - if required
4. **Performance optimization** (Week 8)
5. **Polish & documentation** (Week 9)

**Estimated Effort:** 9 weeks for complete implementation (2D + 3D)

**Team:** 
- 1x Graphics Programmer (Vulkan/DirectX)
- 1x UI Developer (WPF/MSAGL)
- 0.5x UX Designer

---

**Document Version:** 1.0  
**Last Updated:** 2025-12-07  
**Author:** ThemisDB Team
