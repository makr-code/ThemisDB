/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MapTypes.cs                                        ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     301                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace RailwayMonitor.WPF.Services.Map;

// Enums

public enum SignalState
{
    Red,
    Yellow,
    Green,
    Flashing,
    Off
}

public enum SwitchPosition
{
    Straight,
    Diverging,
    Moving,
    Unknown
}

public enum RenderCommandType
{
    UpdateTrain,
    UpdateSignal,
    UpdateSwitch,
    UpdateSensor,
    UpdateTrack
}

public enum RenderQuality
{
    Low,
    Medium,
    High,
    Ultra
}

public enum BackendType
{
    Software,
    DirectX,
    Vulkan
}

// Data classes

public class SensorData
{
    public string Type { get; set; } = "";
    public double Value { get; set; }
    public DateTime Timestamp { get; set; }
    public Dictionary<string, object> Properties { get; set; } = new();
}

public abstract class RenderDataBase
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public DateTime LastUpdate { get; set; }
    public bool IsDirty { get; set; }
}

public class TrainRenderData : RenderDataBase
{
    public string TrainId { get; set; } = "";
    public double PreviousLat { get; set; }
    public double PreviousLon { get; set; }
    public double Heading { get; set; }
    public double Speed { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}

public class SignalRenderData : RenderDataBase
{
    public string SignalId { get; set; } = "";
    public SignalState State { get; set; }
}

public class SwitchRenderData : RenderDataBase
{
    public string SwitchId { get; set; } = "";
    public SwitchPosition Position { get; set; }
}

public class SensorRenderData : RenderDataBase
{
    public string SensorId { get; set; } = "";
    public SensorData Data { get; set; } = new();
}

public class TrackSegmentRenderData : RenderDataBase
{
    public string TrackId { get; set; } = "";
    public double StartLat { get; set; }
    public double StartLon { get; set; }
    public double EndLat { get; set; }
    public double EndLon { get; set; }
    public string Type { get; set; } = "main";
}

public class RenderCommand
{
    public RenderCommandType Type { get; set; }
    public string EntityId { get; set; } = "";
    public string Layer { get; set; } = "";
    public int Priority { get; set; }
}

public class RenderStatistics
{
    public double FPS { get; set; }
    public int TotalEntities { get; set; }
    public int VisibleEntities { get; set; }
    public int DrawCalls { get; set; }
    public double RenderTimeMs { get; set; }
    public string BackendType { get; set; } = "";
}

public class InstanceData
{
    public System.Windows.Point Position { get; set; }
    public float Rotation { get; set; }
    public System.Windows.Media.Color Color { get; set; }
    public float Scale { get; set; }
}

// Performance monitoring

public class PerformanceMonitor
{
    private DateTime _frameStart;
    private int _frameCount;
    private double _totalFrameTime;
    private readonly Queue<double> _frameTimes = new(60);

    public void BeginFrame()
    {
        _frameStart = DateTime.UtcNow;
    }

    public void EndFrame()
    {
        var elapsed = (DateTime.UtcNow - _frameStart).TotalMilliseconds;
        _frameTimes.Enqueue(elapsed);
        if (_frameTimes.Count > 60) _frameTimes.Dequeue();
        
        _frameCount++;
        _totalFrameTime += elapsed;
    }

    public (double FPS, int VisibleEntities, int DrawCalls, double RenderTimeMs) GetStatistics()
    {
        var avgFrameTime = _frameTimes.Any() ? _frameTimes.Average() : 0;
        var fps = avgFrameTime > 0 ? 1000.0 / avgFrameTime : 0;
        
        return (fps, 0, 0, avgFrameTime);
    }
}

// Render command queue

public class RenderCommandQueue
{
    private readonly Queue<RenderCommand> _commands = new();
    private readonly int _maxCommands;

    public RenderCommandQueue(int maxCommands = 10000)
    {
        _maxCommands = maxCommands;
    }

    public void Enqueue(RenderCommand command)
    {
        lock (_commands)
        {
            if (_commands.Count < _maxCommands)
            {
                _commands.Enqueue(command);
            }
        }
    }

    public RenderCommand? Dequeue()
    {
        lock (_commands)
        {
            return _commands.Count > 0 ? _commands.Dequeue() : null;
        }
    }

    public int Count
    {
        get
        {
            lock (_commands)
            {
                return _commands.Count;
            }
        }
    }

    public void Clear()
    {
        lock (_commands)
        {
            _commands.Clear();
        }
    }
}

// Map viewport

public class MapViewport
{
    public double CenterLat { get; set; }
    public double CenterLon { get; set; }
    public int ZoomLevel { get; set; }
    public double Width { get; set; } = 1920;
    public double Height { get; set; } = 1080;

    public (double MinLat, double MaxLat, double MinLon, double MaxLon) GetBounds()
    {
        var scale = Math.Pow(2, ZoomLevel);
        var latRange = 180.0 / scale;
        var lonRange = 360.0 / scale;
        
        return (
            CenterLat - latRange / 2,
            CenterLat + latRange / 2,
            CenterLon - lonRange / 2,
            CenterLon + lonRange / 2
        );
    }
}

// Map layer management

public class MapLayer
{
    public string Name { get; set; } = "";
    public int ZIndex { get; set; }
    public List<object> Features { get; set; } = new();
    public bool Visible { get; set; } = true;

    public void AddFeature(object feature)
    {
        Features.Add(feature);
    }

    public void Clear()
    {
        Features.Clear();
    }
}

public class MapLayerManager
{
    private readonly Dictionary<string, MapLayer> _layers = new();

    public void AddLayer(string name, int zIndex)
    {
        _layers[name] = new MapLayer { Name = name, ZIndex = zIndex };
    }

    public MapLayer GetLayer(string name)
    {
        return _layers.TryGetValue(name, out var layer) ? layer : new MapLayer();
    }

    public IEnumerable<MapLayer> GetLayersOrdered()
    {
        return _layers.Values.OrderBy(l => l.ZIndex);
    }
}
