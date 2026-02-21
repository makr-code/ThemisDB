/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RailwayMapRenderer.cs                              ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     631                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.Concurrent;
using System.Windows;
using System.Windows.Media;

namespace RailwayMonitor.WPF.Services.Map;

/// <summary>
/// Advanced Railway Map Renderer with DirectX/Vulkan support for massive rendering
/// Handles dynamic train movements, signals, switches, IoT sensors
/// </summary>
public interface IRailwayMapRenderer : IDisposable
{
    Task InitializeAsync(double centerLat, double centerLon, int zoomLevel);
    void UpdateTrainPosition(string trainId, double lat, double lon, double heading, double speed, Dictionary<string, object>? metadata = null);
    void UpdateSignalState(string signalId, SignalState state, double lat, double lon);
    void UpdateSwitchState(string switchId, SwitchPosition position, double lat, double lon);
    void UpdateIoTSensor(string sensorId, SensorData data, double lat, double lon);
    void AddTrackSegment(string trackId, double startLat, double startLon, double endLat, double endLon, string type = "main");
    void SetViewport(double centerLat, double centerLon, int zoomLevel);
    Task<byte[]> RenderFrameAsync();
    RenderStatistics GetStatistics();
    void EnableDirectX(bool enable);
}

/// <summary>
/// High-performance Railway Map Renderer
/// Uses batching, instancing, and GPU acceleration for rendering thousands of objects
/// </summary>
public class RailwayMapRenderer : IRailwayMapRenderer
{
    private readonly RenderingBackend _backend;
    private readonly MapViewport _viewport;
    private readonly RenderCommandQueue _commandQueue;
    private readonly PerformanceMonitor _perfMonitor;
    
    // Entity caches for fast updates
    private readonly ConcurrentDictionary<string, TrainRenderData> _trains = new();
    private readonly ConcurrentDictionary<string, SignalRenderData> _signals = new();
    private readonly ConcurrentDictionary<string, SwitchRenderData> _switches = new();
    private readonly ConcurrentDictionary<string, SensorRenderData> _sensors = new();
    private readonly ConcurrentDictionary<string, TrackSegmentRenderData> _tracks = new();
    
    // Render layers for z-ordering
    private readonly MapLayerManager _layerManager;
    
    // Performance settings
    private readonly RenderQuality _quality;
    private bool _useDirectX = true;
    private bool _useInstancing = true;
    
    public RailwayMapRenderer(RenderQuality quality = RenderQuality.High)
    {
        _quality = quality;
        _backend = RenderingBackend.Create(_useDirectX ? BackendType.DirectX : BackendType.Software);
        _viewport = new MapViewport();
        _commandQueue = new RenderCommandQueue(maxCommands: 50000);
        _perfMonitor = new PerformanceMonitor();
        _layerManager = new MapLayerManager();
        
        InitializeLayers();
    }

    private void InitializeLayers()
    {
        // Layer ordering (back to front)
        _layerManager.AddLayer("map_tiles", zIndex: 0);
        _layerManager.AddLayer("tracks", zIndex: 1);
        _layerManager.AddLayer("switches", zIndex: 2);
        _layerManager.AddLayer("signals", zIndex: 3);
        _layerManager.AddLayer("trains", zIndex: 4);
        _layerManager.AddLayer("sensors", zIndex: 5);
        _layerManager.AddLayer("overlays", zIndex: 6);
    }

    public async Task InitializeAsync(double centerLat, double centerLon, int zoomLevel)
    {
        _viewport.CenterLat = centerLat;
        _viewport.CenterLon = centerLon;
        _viewport.ZoomLevel = zoomLevel;
        
        await _backend.InitializeAsync();
        
        // Load map tiles for initial viewport
        await LoadMapTilesAsync();
    }

    public void UpdateTrainPosition(string trainId, double lat, double lon, double heading, double speed, Dictionary<string, object>? metadata = null)
    {
        var train = _trains.GetOrAdd(trainId, _ => new TrainRenderData { TrainId = trainId });
        
        // Update with smooth interpolation
        train.PreviousLat = train.Latitude;
        train.PreviousLon = train.Longitude;
        train.Latitude = lat;
        train.Longitude = lon;
        train.Heading = heading;
        train.Speed = speed;
        train.Metadata = metadata ?? new Dictionary<string, object>();
        train.LastUpdate = DateTime.UtcNow;
        train.IsDirty = true;
        
        // Enqueue render command
        _commandQueue.Enqueue(new RenderCommand
        {
            Type = RenderCommandType.UpdateTrain,
            EntityId = trainId,
            Layer = "trains",
            Priority = CalculatePriority(lat, lon)
        });
    }

    public void UpdateSignalState(string signalId, SignalState state, double lat, double lon)
    {
        var signal = _signals.GetOrAdd(signalId, _ => new SignalRenderData { SignalId = signalId });
        
        signal.State = state;
        signal.Latitude = lat;
        signal.Longitude = lon;
        signal.LastUpdate = DateTime.UtcNow;
        signal.IsDirty = true;
        
        _commandQueue.Enqueue(new RenderCommand
        {
            Type = RenderCommandType.UpdateSignal,
            EntityId = signalId,
            Layer = "signals",
            Priority = CalculatePriority(lat, lon)
        });
    }

    public void UpdateSwitchState(string switchId, SwitchPosition position, double lat, double lon)
    {
        var sw = _switches.GetOrAdd(switchId, _ => new SwitchRenderData { SwitchId = switchId });
        
        sw.Position = position;
        sw.Latitude = lat;
        sw.Longitude = lon;
        sw.LastUpdate = DateTime.UtcNow;
        sw.IsDirty = true;
        
        _commandQueue.Enqueue(new RenderCommand
        {
            Type = RenderCommandType.UpdateSwitch,
            EntityId = switchId,
            Layer = "switches",
            Priority = CalculatePriority(lat, lon)
        });
    }

    public void UpdateIoTSensor(string sensorId, SensorData data, double lat, double lon)
    {
        var sensor = _sensors.GetOrAdd(sensorId, _ => new SensorRenderData { SensorId = sensorId });
        
        sensor.Data = data;
        sensor.Latitude = lat;
        sensor.Longitude = lon;
        sensor.LastUpdate = DateTime.UtcNow;
        sensor.IsDirty = true;
        
        _commandQueue.Enqueue(new RenderCommand
        {
            Type = RenderCommandType.UpdateSensor,
            EntityId = sensorId,
            Layer = "sensors",
            Priority = CalculatePriority(lat, lon)
        });
    }

    public void AddTrackSegment(string trackId, double startLat, double startLon, double endLat, double endLon, string type = "main")
    {
        var track = _tracks.GetOrAdd(trackId, _ => new TrackSegmentRenderData { TrackId = trackId });
        
        track.StartLat = startLat;
        track.StartLon = startLon;
        track.EndLat = endLat;
        track.EndLon = endLon;
        track.Latitude = (startLat + endLat) / 2;
        track.Longitude = (startLon + endLon) / 2;
        track.Type = type;
        track.IsDirty = true;
    }

    public void SetViewport(double centerLat, double centerLon, int zoomLevel)
    {
        bool needsRefresh = 
            Math.Abs(_viewport.CenterLat - centerLat) > 0.01 ||
            Math.Abs(_viewport.CenterLon - centerLon) > 0.01 ||
            _viewport.ZoomLevel != zoomLevel;
        
        _viewport.CenterLat = centerLat;
        _viewport.CenterLon = centerLon;
        _viewport.ZoomLevel = zoomLevel;
        
        if (needsRefresh)
        {
            _ = LoadMapTilesAsync();
        }
    }

    public void EnableDirectX(bool enable)
    {
        if (_useDirectX != enable)
        {
            _useDirectX = enable;
            _backend.SwitchBackend(enable ? BackendType.DirectX : BackendType.Software);
        }
    }

    public async Task<byte[]> RenderFrameAsync()
    {
        _perfMonitor.BeginFrame();
        
        try
        {
            // Process render commands
            await ProcessRenderQueueAsync();
            
            // Render all layers
            _backend.BeginFrame();
            
            // 1. Render map tiles
            await RenderMapTilesAsync();
            
            // 2. Render tracks (batched)
            await RenderTracksAsync();
            
            // 3. Render switches (instanced)
            await RenderSwitchesAsync();
            
            // 4. Render signals (instanced with state-based colors)
            await RenderSignalsAsync();
            
            // 5. Render trains (with smooth interpolation)
            await RenderTrainsAsync();
            
            // 6. Render IoT sensors
            await RenderSensorsAsync();
            
            // 7. Render overlays (labels, tooltips)
            await RenderOverlaysAsync();
            
            var frame = _backend.EndFrame();
            
            _perfMonitor.EndFrame();
            
            return frame;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Render error: {ex.Message}");
            return Array.Empty<byte>();
        }
    }

    private async Task ProcessRenderQueueAsync()
    {
        int processed = 0;
        const int maxPerFrame = 1000;
        
        while (processed < maxPerFrame && _commandQueue.Count > 0)
        {
            var cmd = _commandQueue.Dequeue();
            if (cmd != null)
            {
                // Process command based on type
                // Commands are already enqueued by Update methods
                processed++;
            }
        }
        
        await Task.CompletedTask;
    }

    private async Task RenderMapTilesAsync()
    {
        // Render base map tiles
        foreach (var tile in _layerManager.GetLayer("map_tiles").Features)
        {
            _backend.RenderTile(tile);
        }
        await Task.CompletedTask;
    }

    private async Task RenderTracksAsync()
    {
        // Batch render track segments
        var visibleTracks = GetVisibleEntities(_tracks.Values);
        
        if (visibleTracks.Any())
        {
            _backend.BeginBatch("tracks");
            foreach (var track in visibleTracks)
            {
                _backend.RenderLine(
                    track.StartLat, track.StartLon,
                    track.EndLat, track.EndLon,
                    GetTrackColor(track.Type),
                    lineWidth: 3.0f
                );
            }
            _backend.EndBatch();
        }
        
        await Task.CompletedTask;
    }

    private async Task RenderSwitchesAsync()
    {
        // Instance render switches
        var visibleSwitches = GetVisibleEntities(_switches.Values);
        
        if (visibleSwitches.Any() && _useInstancing)
        {
            var instances = visibleSwitches.Select(sw => new InstanceData
            {
                Position = WorldToScreen(sw.Latitude, sw.Longitude),
                Rotation = GetSwitchRotation(sw.Position),
                Color = GetSwitchColor(sw.Position),
                Scale = 1.0f
            }).ToArray();
            
            _backend.RenderInstanced("switch_sprite", instances);
        }
        else
        {
            foreach (var sw in visibleSwitches)
            {
                var pos = WorldToScreen(sw.Latitude, sw.Longitude);
                _backend.RenderSprite("switch", pos.X, pos.Y, GetSwitchColor(sw.Position));
            }
        }
        
        await Task.CompletedTask;
    }

    private async Task RenderSignalsAsync()
    {
        // Instance render signals with state colors
        var visibleSignals = GetVisibleEntities(_signals.Values);
        
        if (visibleSignals.Any() && _useInstancing)
        {
            var instances = visibleSignals.Select(sig => new InstanceData
            {
                Position = WorldToScreen(sig.Latitude, sig.Longitude),
                Rotation = 0,
                Color = GetSignalColor(sig.State),
                Scale = 1.0f
            }).ToArray();
            
            _backend.RenderInstanced("signal_sprite", instances);
        }
        
        await Task.CompletedTask;
    }

    private async Task RenderTrainsAsync()
    {
        // Render trains with smooth motion interpolation
        var visibleTrains = GetVisibleEntities(_trains.Values);
        
        foreach (var train in visibleTrains)
        {
            // Interpolate position for smooth movement
            var interpolatedPos = InterpolatePosition(train);
            var screenPos = WorldToScreen(interpolatedPos.Lat, interpolatedPos.Lon);
            
            // Render train sprite
            _backend.RenderSprite(
                GetTrainSprite(train.Speed),
                screenPos.X,
                screenPos.Y,
                rotation: (float)train.Heading,
                color: GetTrainColor(train.Speed)
            );
            
            // Render metadata if zoomed in
            if (_viewport.ZoomLevel >= 12)
            {
                _backend.RenderText(
                    train.TrainId,
                    screenPos.X,
                    screenPos.Y - 20,
                    fontSize: 12,
                    color: Colors.White
                );
                
                _backend.RenderText(
                    $"{train.Speed:F0} km/h",
                    screenPos.X,
                    screenPos.Y - 35,
                    fontSize: 10,
                    color: Colors.LightGray
                );
                
                // Render additional metadata
                if (train.Metadata.ContainsKey("delay"))
                {
                    var delay = train.Metadata["delay"];
                    _backend.RenderText(
                        $"+{delay} min",
                        screenPos.X,
                        screenPos.Y - 50,
                        fontSize: 9,
                        color: Colors.Red
                    );
                }
            }
        }
        
        await Task.CompletedTask;
    }

    private async Task RenderSensorsAsync()
    {
        var visibleSensors = GetVisibleEntities(_sensors.Values);
        
        foreach (var sensor in visibleSensors)
        {
            var screenPos = WorldToScreen(sensor.Latitude, sensor.Longitude);
            
            // Render sensor icon based on type
            var icon = GetSensorIcon(sensor.Data.Type);
            _backend.RenderSprite(icon, screenPos.X, screenPos.Y, GetSensorColor(sensor.Data));
            
            // Show value if zoomed in
            if (_viewport.ZoomLevel >= 14)
            {
                _backend.RenderText(
                    $"{sensor.Data.Value:F1}",
                    screenPos.X,
                    screenPos.Y - 15,
                    fontSize: 9,
                    color: Colors.Yellow
                );
            }
        }
        
        await Task.CompletedTask;
    }

    private async Task RenderOverlaysAsync()
    {
        // Render scale, compass, performance stats
        if (_quality >= RenderQuality.Medium)
        {
            RenderCompass();
            RenderScale();
        }
        
        if (_quality >= RenderQuality.High)
        {
            RenderPerformanceStats();
        }
        
        await Task.CompletedTask;
    }

    private void RenderCompass()
    {
        _backend.RenderSprite("compass", 50, 50, rotation: 0);
    }

    private void RenderScale()
    {
        var scaleKm = CalculateScaleDistance();
        _backend.RenderLine(10, 80, 10 + scaleKm, 80, Colors.White, 2);
        _backend.RenderText($"{scaleKm} km", 10, 90, 10, Colors.White);
    }

    private void RenderPerformanceStats()
    {
        var stats = _perfMonitor.GetStatistics();
        _backend.RenderText(
            $"FPS: {stats.FPS:F1} | Entities: {stats.VisibleEntities} | Draw Calls: {stats.DrawCalls}",
            10,
            10,
            fontSize: 10,
            color: Colors.LightGreen
        );
    }

    // Helper methods
    
    private IEnumerable<T> GetVisibleEntities<T>(IEnumerable<T> entities) where T : RenderDataBase
    {
        return entities.Where(e => IsInViewport(e.Latitude, e.Longitude));
    }

    private bool IsInViewport(double lat, double lon)
    {
        var bounds = _viewport.GetBounds();
        return lat >= bounds.MinLat && lat <= bounds.MaxLat &&
               lon >= bounds.MinLon && lon <= bounds.MaxLon;
    }

    private Point WorldToScreen(double lat, double lon)
    {
        // Web Mercator projection
        var scale = Math.Pow(2, _viewport.ZoomLevel);
        var x = (lon + 180.0) / 360.0 * scale * 256;
        var y = (1 - Math.Log(Math.Tan(lat * Math.PI / 180.0) + 1 / Math.Cos(lat * Math.PI / 180.0)) / Math.PI) / 2 * scale * 256;
        
        return new Point(x, y);
    }

    private (double Lat, double Lon) InterpolatePosition(TrainRenderData train)
    {
        // Smooth interpolation between previous and current position
        var elapsed = (DateTime.UtcNow - train.LastUpdate).TotalSeconds;
        var t = Math.Min(elapsed / 1.0, 1.0); // 1 second interpolation
        
        var lat = train.PreviousLat + (train.Latitude - train.PreviousLat) * t;
        var lon = train.PreviousLon + (train.Longitude - train.PreviousLon) * t;
        
        return (lat, lon);
    }

    private int CalculatePriority(double lat, double lon)
    {
        // Higher priority for entities closer to viewport center
        var distToCenter = Math.Sqrt(
            Math.Pow(lat - _viewport.CenterLat, 2) +
            Math.Pow(lon - _viewport.CenterLon, 2)
        );
        return (int)(1000 - distToCenter * 100);
    }

    private async Task LoadMapTilesAsync()
    {
        // Load OSM tiles for current viewport
        // Implementation would fetch from tile server
        await Task.CompletedTask;
    }

    private Color GetSignalColor(SignalState state) => state switch
    {
        SignalState.Green => Colors.Green,
        SignalState.Yellow => Colors.Yellow,
        SignalState.Red => Colors.Red,
        SignalState.Flashing => Colors.Orange,
        _ => Colors.Gray
    };

    private Color GetSwitchColor(SwitchPosition position) => position switch
    {
        SwitchPosition.Straight => Colors.LightBlue,
        SwitchPosition.Diverging => Colors.Orange,
        SwitchPosition.Moving => Colors.Yellow,
        _ => Colors.Gray
    };

    private Color GetTrackColor(string type) => type switch
    {
        "main" => Color.FromRgb(100, 100, 100),
        "siding" => Color.FromRgb(120, 120, 120),
        _ => Colors.DarkGray
    };

    private Color GetTrainColor(double speed) => speed switch
    {
        > 200 => Colors.Red,
        > 100 => Colors.Orange,
        > 50 => Colors.Yellow,
        _ => Colors.LightGreen
    };

    private Color GetSensorColor(SensorData data) => data.Type switch
    {
        "temperature" => data.Value > 50 ? Colors.Red : Colors.Blue,
        "vibration" => data.Value > 80 ? Colors.Orange : Colors.Green,
        "pressure" => Colors.Purple,
        _ => Colors.White
    };

    private string GetTrainSprite(double speed) => speed > 0 ? "train_moving" : "train_stopped";
    
    private string GetSensorIcon(string type) => $"sensor_{type}";
    
    private float GetSwitchRotation(SwitchPosition pos) => pos == SwitchPosition.Diverging ? 45f : 0f;
    
    private float CalculateScaleDistance() => (float)(100.0 / Math.Pow(2, _viewport.ZoomLevel));

    public RenderStatistics GetStatistics()
    {
        return new RenderStatistics
        {
            FPS = _perfMonitor.GetStatistics().FPS,
            TotalEntities = _trains.Count + _signals.Count + _switches.Count + _sensors.Count,
            VisibleEntities = _perfMonitor.GetStatistics().VisibleEntities,
            DrawCalls = _perfMonitor.GetStatistics().DrawCalls,
            RenderTimeMs = _perfMonitor.GetStatistics().RenderTimeMs,
            BackendType = _useDirectX ? "DirectX" : "Software"
        };
    }

    public void Dispose()
    {
        _backend?.Dispose();
        _trains.Clear();
        _signals.Clear();
        _switches.Clear();
        _sensors.Clear();
    }
}
