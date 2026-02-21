/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Services.cs                                        ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     1022                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Diagnostics;
using System.Net.Http.Json;
using System.Text.Json;
using RailwayMonitor.WPF.Models;

namespace RailwayMonitor.WPF.Services;

/// <summary>
/// Service for communicating with ThemisDB backend
/// </summary>
public interface IThemisDbService
{
    Task<bool> ConnectAsync();
    Task<List<Train>> GetActiveTrainsAsync();
    Task<List<Train>> GetDelayedTrainsAsync(int minDelayMinutes = 5);
    Task<List<Station>> GetStationsAsync();
    Task<Train?> GetTrainAsync(string trainNumber);
    Task<Dictionary<string, int>> GetTrainsByTypeAsync();
    Task<T?> QueryAqlAsync<T>(string aqlQuery) where T : class;
}

public class ThemisDbService : IThemisDbService
{
    private readonly HttpClient _httpClient;
    private readonly string _baseUrl;

    public ThemisDbService(IConfiguration config)
    {
        _baseUrl = config.ThemisDbUrl;
        _httpClient = new HttpClient
        {
            BaseAddress = new Uri(_baseUrl),
            Timeout = TimeSpan.FromSeconds(5)
        };
    }

    public async Task<bool> ConnectAsync()
    {
        try
        {
            var response = await _httpClient.GetAsync("/health");
            return response.IsSuccessStatusCode;
        }
        catch
        {
            return false;
        }
    }

    public async Task<List<Train>> GetActiveTrainsAsync()
    {
        try
        {
            // Query ThemisDB using AQL to get all active trains
            var aqlQuery = @"
                FOR train IN entities
                  FILTER train._key LIKE ""trains:%""
                  RETURN {
                    train_number: train.train_number,
                    category: train.type,
                    operator: train.operator,
                    latitude: train.lat,
                    longitude: train.lon,
                    altitude: train.altitude,
                    speed_kmh: train.speed,
                    heading: train.heading,
                    acceleration_mps2: train.acceleration,
                    origin: train.origin,
                    destination: train.destination,
                    current_segment: train.current_segment,
                    distance_traveled_km: train.distance_traveled,
                    delay_min: train.delay,
                    scheduled_arrival: train.scheduled_arrival,
                    estimated_arrival: train.estimated_arrival,
                    passenger_capacity: train.passenger_capacity,
                    passenger_count: train.passenger_count,
                    instantaneous_power_kw: train.power_kw,
                    cumulative_energy_kwh: train.energy_kwh,
                    efficiency_percent: train.efficiency,
                    status: train.status,
                    last_update: train.updated_at
                  }
            ";
            
            var request = new { query = aqlQuery };
            var response = await _httpClient.PostAsJsonAsync("/query/aql", request);
            
            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<AqlQueryResponse>();
                if (result?.Result != null)
                {
                    return result.Result.Select(item => new Train
                    {
                        TrainNumber = item.GetProperty("train_number").GetString() ?? "",
                        Category = item.GetProperty("category").GetString() ?? "",
                        Operator = item.GetProperty("operator").GetString() ?? "DB Fernverkehr AG",
                        Latitude = item.GetProperty("latitude").GetDouble(),
                        Longitude = item.GetProperty("longitude").GetDouble(),
                        Altitude = item.GetProperty("altitude").GetDouble(),
                        SpeedKmh = item.GetProperty("speed_kmh").GetDouble(),
                        Heading = item.GetProperty("heading").GetDouble(),
                        AccelerationMps2 = item.GetProperty("acceleration_mps2").GetDouble(),
                        Origin = item.GetProperty("origin").GetString() ?? "",
                        Destination = item.GetProperty("destination").GetString() ?? "",
                        CurrentSegment = item.GetProperty("current_segment").GetString() ?? "",
                        DistanceTraveledKm = item.GetProperty("distance_traveled_km").GetDouble(),
                        DelayMin = item.GetProperty("delay_min").GetInt32(),
                        InstantaneousPowerKw = item.GetProperty("instantaneous_power_kw").GetDouble(),
                        CumulativeEnergyKwh = item.GetProperty("cumulative_energy_kwh").GetDouble(),
                        EfficiencyPercent = item.GetProperty("efficiency_percent").GetDouble(),
                        Status = item.GetProperty("status").GetString() ?? "in_service",
                        PassengerCapacity = item.GetProperty("passenger_capacity").GetInt32(),
                        PassengerCount = item.GetProperty("passenger_count").GetInt32()
                    }).ToList();
                }
            }
        }
        catch (Exception ex)
        {
            // Log error if needed
            System.Diagnostics.Debug.WriteLine($"Error fetching trains: {ex.Message}");
        }
        
        return new List<Train>();
    }

    public async Task<List<Station>> GetStationsAsync()
    {
        try
        {
            // Query ThemisDB using AQL to get all stations
            var aqlQuery = @"
                FOR station IN entities
                  FILTER station._key LIKE ""stations:%""
                  RETURN {
                    station_id: station.station_id,
                    name: station.name,
                    eva_number: station.eva_number,
                    latitude: station.lat,
                    longitude: station.lon,
                    category: station.category,
                    operator: station.operator,
                    has_parking: station.has_parking,
                    has_bicycle_parking: station.has_bicycle_parking,
                    has_elevator: station.has_elevator,
                    has_db_information: station.has_db_information
                  }
            ";
            
            var request = new { query = aqlQuery };
            var response = await _httpClient.PostAsJsonAsync("/query/aql", request);
            
            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<AqlQueryResponse>();
                if (result?.Result != null)
                {
                    return result.Result.Select(item => new Station
                    {
                        StationId = item.GetProperty("station_id").GetString() ?? "",
                        Name = item.GetProperty("name").GetString() ?? "",
                        EvaNumber = item.GetProperty("eva_number").GetString() ?? "",
                        Latitude = item.GetProperty("latitude").GetDouble(),
                        Longitude = item.GetProperty("longitude").GetDouble(),
                        Category = item.GetProperty("category").GetString() ?? "",
                        Operator = item.GetProperty("operator").GetString() ?? "DB Station&Service AG",
                        HasParking = item.GetProperty("has_parking").GetBoolean(),
                        HasBicycleParking = item.GetProperty("has_bicycle_parking").GetBoolean(),
                        HasElevator = item.GetProperty("has_elevator").GetBoolean(),
                        HasDbInformation = item.GetProperty("has_db_information").GetBoolean()
                    }).ToList();
                }
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error fetching stations: {ex.Message}");
        }
        
        return new List<Station>();
    }

    public async Task<Train?> GetTrainAsync(string trainNumber)
    {
        try
        {
            // Query ThemisDB using AQL to get a specific train by train number
            var aqlQuery = $@"
                FOR train IN entities
                  FILTER train._key LIKE ""trains:%""
                  FILTER train.train_number == ""{trainNumber}""
                  LIMIT 1
                  RETURN {{
                    train_number: train.train_number,
                    category: train.type,
                    operator: train.operator,
                    latitude: train.lat,
                    longitude: train.lon,
                    altitude: train.altitude,
                    speed_kmh: train.speed,
                    heading: train.heading,
                    acceleration_mps2: train.acceleration,
                    origin: train.origin,
                    destination: train.destination,
                    current_segment: train.current_segment,
                    distance_traveled_km: train.distance_traveled,
                    delay_min: train.delay,
                    scheduled_arrival: train.scheduled_arrival,
                    estimated_arrival: train.estimated_arrival,
                    passenger_capacity: train.passenger_capacity,
                    passenger_count: train.passenger_count,
                    instantaneous_power_kw: train.power_kw,
                    cumulative_energy_kwh: train.energy_kwh,
                    efficiency_percent: train.efficiency,
                    status: train.status,
                    last_update: train.updated_at
                  }}
            ";
            
            var request = new { query = aqlQuery };
            var response = await _httpClient.PostAsJsonAsync("/query/aql", request);
            
            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<AqlQueryResponse>();
                if (result?.Result != null && result.Result.Count > 0)
                {
                    var item = result.Result[0];
                    return new Train
                    {
                        TrainNumber = item.GetProperty("train_number").GetString() ?? "",
                        Category = item.GetProperty("category").GetString() ?? "",
                        Operator = item.GetProperty("operator").GetString() ?? "DB Fernverkehr AG",
                        Latitude = item.GetProperty("latitude").GetDouble(),
                        Longitude = item.GetProperty("longitude").GetDouble(),
                        Altitude = item.GetProperty("altitude").GetDouble(),
                        SpeedKmh = item.GetProperty("speed_kmh").GetDouble(),
                        Heading = item.GetProperty("heading").GetDouble(),
                        AccelerationMps2 = item.GetProperty("acceleration_mps2").GetDouble(),
                        Origin = item.GetProperty("origin").GetString() ?? "",
                        Destination = item.GetProperty("destination").GetString() ?? "",
                        CurrentSegment = item.GetProperty("current_segment").GetString() ?? "",
                        DistanceTraveledKm = item.GetProperty("distance_traveled_km").GetDouble(),
                        DelayMin = item.GetProperty("delay_min").GetInt32(),
                        InstantaneousPowerKw = item.GetProperty("instantaneous_power_kw").GetDouble(),
                        CumulativeEnergyKwh = item.GetProperty("cumulative_energy_kwh").GetDouble(),
                        EfficiencyPercent = item.GetProperty("efficiency_percent").GetDouble(),
                        Status = item.GetProperty("status").GetString() ?? "in_service",
                        PassengerCapacity = item.GetProperty("passenger_capacity").GetInt32(),
                        PassengerCount = item.GetProperty("passenger_count").GetInt32()
                    };
                }
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error fetching train {trainNumber}: {ex.Message}");
        }
        
        return null;
    }

    public async Task<List<Train>> GetDelayedTrainsAsync(int minDelayMinutes = 5)
    {
        try
        {
            // Query ThemisDB for delayed trains
            var aqlQuery = $@"
                FOR train IN entities
                  FILTER train._key LIKE ""trains:%""
                  FILTER train.delay > {minDelayMinutes}
                  SORT train.delay DESC
                  LIMIT 20
                  RETURN {{
                    train_number: train.train_number,
                    category: train.type,
                    delay_min: train.delay,
                    origin: train.origin,
                    destination: train.destination,
                    latitude: train.lat,
                    longitude: train.lon,
                    status: train.status
                  }}
            ";
            
            var request = new { query = aqlQuery };
            var response = await _httpClient.PostAsJsonAsync("/query/aql", request);
            
            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<AqlQueryResponse>();
                if (result?.Result != null)
                {
                    return result.Result.Select(item => new Train
                    {
                        TrainNumber = item.GetProperty("train_number").GetString() ?? "",
                        Category = item.GetProperty("category").GetString() ?? "",
                        DelayMin = item.GetProperty("delay_min").GetInt32(),
                        Origin = item.GetProperty("origin").GetString() ?? "",
                        Destination = item.GetProperty("destination").GetString() ?? "",
                        Latitude = item.GetProperty("latitude").GetDouble(),
                        Longitude = item.GetProperty("longitude").GetDouble(),
                        Status = item.GetProperty("status").GetString() ?? "delayed"
                    }).ToList();
                }
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error fetching delayed trains: {ex.Message}");
        }
        
        return new List<Train>();
    }

    public async Task<Dictionary<string, int>> GetTrainsByTypeAsync()
    {
        try
        {
            // Query ThemisDB for trains grouped by type
            var aqlQuery = @"
                FOR train IN entities
                  FILTER train._key LIKE ""trains:%""
                  COLLECT type = train.type WITH COUNT INTO count
                  RETURN { type: type, count: count }
            ";
            
            var request = new { query = aqlQuery };
            var response = await _httpClient.PostAsJsonAsync("/query/aql", request);
            
            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<AqlQueryResponse>();
                if (result?.Result != null)
                {
                    return result.Result.ToDictionary(
                        item => item.GetProperty("type").GetString() ?? "Unknown",
                        item => item.GetProperty("count").GetInt32()
                    );
                }
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error fetching trains by type: {ex.Message}");
        }
        
        return new Dictionary<string, int>();
    }

    public async Task<T?> QueryAqlAsync<T>(string aqlQuery) where T : class
    {
        try
        {
            var request = new { query = aqlQuery };
            var response = await _httpClient.PostAsJsonAsync("/query/aql", request);
            
            if (response.IsSuccessStatusCode)
            {
                return await response.Content.ReadFromJsonAsync<T>();
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error executing AQL query: {ex.Message}");
        }
        
        return null;
    }

    // Helper class for AQL query responses
    private class AqlQueryResponse
    {
        public List<JsonElement> Result { get; set; } = new();
        public bool Error { get; set; }
        public string? ErrorMessage { get; set; }
    }
}

/// <summary>
/// Service for train simulation control
/// </summary>
public interface ITrainSimulatorService
{
    Task<bool> StartAsync(int trainCount = 50);
    Task StopAsync();
    bool IsRunning { get; }
}

public class TrainSimulatorService : ITrainSimulatorService
{
    private Process? _simulatorProcess;
    
    public bool IsRunning => _simulatorProcess?.HasExited == false;

    public async Task<bool> StartAsync(int trainCount = 50)
    {
        if (IsRunning)
            return true;

        try
        {
            var startInfo = new ProcessStartInfo
            {
                FileName = "python",
                Arguments = $"scripts/railway/train_simulator.py --trains {trainCount}",
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true
            };

            _simulatorProcess = Process.Start(startInfo);
            await Task.Delay(2000); // Wait for startup
            
            return _simulatorProcess?.HasExited == false;
        }
        catch
        {
            return false;
        }
    }

    public async Task StopAsync()
    {
        if (_simulatorProcess != null && !_simulatorProcess.HasExited)
        {
            _simulatorProcess.Kill();
            await _simulatorProcess.WaitForExitAsync();
            _simulatorProcess = null;
        }
    }
}

/// <summary>
/// Service for LLM-based analysis using Ollama
/// </summary>
public interface ILlmService
{
    Task<string> QueryAsync(string query, object? context = null);
}

public class OllamaService : ILlmService
{
    private readonly HttpClient _httpClient;
    private readonly string _baseUrl;

    public OllamaService(IConfiguration config)
    {
        _baseUrl = config.OllamaUrl;
        _httpClient = new HttpClient
        {
            BaseAddress = new Uri(_baseUrl),
            Timeout = TimeSpan.FromSeconds(30)
        };
    }

    public async Task<string> QueryAsync(string query, object? context = null)
    {
        try
        {
            var prompt = query;
            if (context != null)
            {
                var contextJson = System.Text.Json.JsonSerializer.Serialize(context);
                prompt = $"Context: {contextJson}\n\nQuestion: {query}";
            }

            var request = new
            {
                model = "llama3.2",
                prompt = prompt,
                stream = false
            };

            var response = await _httpClient.PostAsJsonAsync("/api/generate", request);
            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<OllamaResponse>();
                return result?.Response ?? "No response from LLM";
            }
        }
        catch (Exception ex)
        {
            return $"Error querying LLM: {ex.Message}";
        }
        
        return "LLM service unavailable";
    }

    private class OllamaResponse
    {
        public string Response { get; set; } = "";
    }
}

/// <summary>
/// Service for energy management calculations
/// </summary>
public interface IEnergyManagementService
{
    Task<List<PowerSource>> GetPowerSourcesAsync();
    Task<List<Substation>> GetSubstationsAsync();
    Task<EnergyData> CalculateTrainEnergyAsync(Train train);
    Task<List<PowerForecastPoint>> ForecastPowerDemandAsync(int hoursAhead);
    Task<PowerDispatchResult> OptimizeDispatchAsync(double demandMw, string optimizeFor = "cost");
}

public class EnergyManagementService : IEnergyManagementService
{
    public async Task<List<PowerSource>> GetPowerSourcesAsync()
    {
        // Return realistic power sources based on DB Energie GmbH data
        await Task.CompletedTask;
        
        return new List<PowerSource>
        {
            new() { Type = "hydro", Name = "Wasserkraft", CapacityMw = 200, CurrentOutputMw = 200, CostEurPerMwh = 25, Co2GPerKwh = 0 },
            new() { Type = "wind", Name = "Windkraft", CapacityMw = 150, CurrentOutputMw = 120, CostEurPerMwh = 30, Co2GPerKwh = 0 },
            new() { Type = "solar", Name = "Solarenergie", CapacityMw = 100, CurrentOutputMw = 60, CostEurPerMwh = 35, Co2GPerKwh = 0 },
            new() { Type = "battery", Name = "Batteriespeicher", CapacityMw = 50, CurrentOutputMw = 40, CostEurPerMwh = 80, Co2GPerKwh = 0 },
            new() { Type = "gas", Name = "Gaskraftwerk", CapacityMw = 300, CurrentOutputMw = 150, CostEurPerMwh = 120, Co2GPerKwh = 350 }
        };
    }

    public async Task<List<Substation>> GetSubstationsAsync()
    {
        // Return sample substations
        await Task.CompletedTask;
        
        return new List<Substation>
        {
            new() { SubstationId = "UW_FFM_SUED", Name = "Unterwerk Frankfurt Süd", Latitude = 50.0234, Longitude = 8.5678, CapacityMw = 45, CurrentLoadMw = 32.5 },
            new() { SubstationId = "UW_MUC_OST", Name = "Unterwerk München Ost", Latitude = 48.1455, Longitude = 11.5821, CapacityMw = 45, CurrentLoadMw = 38.2 },
            new() { SubstationId = "UW_HAM_NORD", Name = "Unterwerk Hamburg Nord", Latitude = 53.5511, Longitude = 9.9937, CapacityMw = 45, CurrentLoadMw = 34.5 }
        };
    }

    public async Task<EnergyData> CalculateTrainEnergyAsync(Train train)
    {
        await Task.CompletedTask;
        
        // Realistic energy calculation based on ICE 3 data
        double speedKmh = train.SpeedKmh;
        double massKg = 435000; // ICE 3 mass
        
        // Traction power (simplified): proportional to speed³
        double tractionPowerKw = (massKg / 100000.0) * Math.Pow(speedKmh / 100.0, 3) * 1000;
        
        // Auxiliary power (HVAC, lighting, etc.)
        double auxiliaryPowerKw = 400;
        
        // Total instantaneous power
        double instantaneousPowerKw = tractionPowerKw + auxiliaryPowerKw;
        
        // Efficiency
        double efficiencyPercent = 87.3;
        
        return new EnergyData
        {
            InstantaneousPowerKw = instantaneousPowerKw,
            TractionPowerKw = tractionPowerKw,
            AuxiliaryPowerKw = auxiliaryPowerKw,
            RecuperationPowerKw = 0,
            CumulativeEnergyKwh = train.CumulativeEnergyKwh,
            EfficiencyPercent = efficiencyPercent
        };
    }

    public async Task<List<PowerForecastPoint>> ForecastPowerDemandAsync(int hoursAhead)
    {
        await Task.CompletedTask;
        
        var forecast = new List<PowerForecastPoint>();
        var random = new Random();
        
        for (int hour = 0; hour < hoursAhead; hour++)
        {
            // Realistic demand profile: peaks at 7-9am and 5-7pm
            double baseDemand = 400;
            double peakMultiplier = 1.0;
            
            int hourOfDay = (DateTime.Now.Hour + hour) % 24;
            if (hourOfDay >= 7 && hourOfDay <= 9)
                peakMultiplier = 1.8; // Morning peak
            else if (hourOfDay >= 17 && hourOfDay <= 19)
                peakMultiplier = 1.7; // Evening peak
            else if (hourOfDay >= 0 && hourOfDay <= 5)
                peakMultiplier = 0.4; // Night
            
            double powerMw = baseDemand * peakMultiplier + random.Next(-50, 50);
            
            forecast.Add(new PowerForecastPoint
            {
                Hour = hour,
                PowerMw = powerMw,
                TrainsCount = (int)(powerMw / 0.6), // Avg 0.6 MW per train
                Confidence = 0.85 + random.NextDouble() * 0.1
            });
        }
        
        return forecast;
    }

    public async Task<PowerDispatchResult> OptimizeDispatchAsync(double demandMw, string optimizeFor = "cost")
    {
        await Task.CompletedTask;
        
        var sources = await GetPowerSourcesAsync();
        var allocations = new Dictionary<string, double>();
        
        // Merit-order dispatch: cheapest first
        var sortedSources = optimizeFor == "co2" 
            ? sources.OrderBy(s => s.Co2GPerKwh).ToList()
            : sources.OrderBy(s => s.CostEurPerMwh).ToList();
        
        double remainingDemand = demandMw;
        double totalCost = 0;
        double totalCo2 = 0;
        double renewableOutput = 0;
        
        foreach (var source in sortedSources)
        {
            if (remainingDemand <= 0) break;
            
            double allocated = Math.Min(source.CapacityMw, remainingDemand);
            allocations[source.Type] = allocated;
            
            totalCost += allocated * source.CostEurPerMwh;
            totalCo2 += allocated * source.Co2GPerKwh;
            
            if (source.IsRenewable)
                renewableOutput += allocated;
            
            remainingDemand -= allocated;
        }
        
        double renewablePercent = demandMw > 0 ? (renewableOutput / demandMw) * 100 : 0;
        double co2PerMwh = demandMw > 0 ? (totalCo2 / demandMw) : 0;
        
        return new PowerDispatchResult
        {
            Allocations = allocations,
            TotalCostEur = totalCost,
            Co2KgPerMwh = co2PerMwh,
            RenewablePercent = renewablePercent
        };
    }
}

using RailwayMonitor.WPF.Services.Map;

namespace RailwayMonitor.WPF.Services;

/// <summary>
/// Service for map operations with DirectX/Vulkan-accelerated rendering
/// </summary>
public interface IMapService : IDisposable
{
    Task<bool> InitializeAsync();
    IRailwayMapRenderer GetRenderer();
}

public class MapService : IMapService
{
    private readonly IRailwayMapRenderer _renderer;
    
    public MapService()
    {
        // Create high-performance renderer with DirectX support
        _renderer = new RailwayMapRenderer(RenderQuality.High);
    }
    
    public async Task<bool> InitializeAsync()
    {
        // Initialize with Germany center coordinates
        await _renderer.InitializeAsync(
            centerLat: 51.1657,  // Germany center
            centerLon: 10.4515,
            zoomLevel: 6
        );
        return true;
    }
    
    public IRailwayMapRenderer GetRenderer()
    {
        return _renderer;
    }
    
    public void Dispose()
    {
        _renderer?.Dispose();
    }
}

/// <summary>
/// WebSocket service for real-time updates
/// </summary>
/// <summary>
/// Service for real-time Change Data Capture (CDC) streaming from ThemisDB
/// Uses Server-Sent Events (EventSource) to receive live entity updates
/// </summary>
public interface IChangeFeedService
{
    Task ConnectAsync(string keyPrefix = "trains:");
    Task DisconnectAsync();
    bool IsConnected { get; }
    event EventHandler<TrainUpdateEventArgs>? TrainUpdated;
    event EventHandler<ConnectionStateEventArgs>? ConnectionStateChanged;
}

public class TrainUpdateEventArgs : EventArgs
{
    public Train Train { get; set; } = new();
    public string UpdateType { get; set; } = "update"; // "insert", "update", "delete"
}

public class ConnectionStateEventArgs : EventArgs
{
    public bool IsConnected { get; set; }
    public string? ErrorMessage { get; set; }
}

public class ChangeFeedService : IChangeFeedService, IDisposable
{
    private readonly HttpClient _httpClient;
    private readonly string _baseUrl;
    private CancellationTokenSource? _cancellationTokenSource;
    private Task? _streamTask;
    private bool _isConnected;

    public event EventHandler<TrainUpdateEventArgs>? TrainUpdated;
    public event EventHandler<ConnectionStateEventArgs>? ConnectionStateChanged;

    public bool IsConnected => _isConnected;

    public ChangeFeedService(IConfiguration config)
    {
        _baseUrl = config.ThemisDbUrl;
        _httpClient = new HttpClient
        {
            BaseAddress = new Uri(_baseUrl),
            Timeout = TimeSpan.FromMinutes(30) // Long timeout for streaming
        };
    }

    public async Task ConnectAsync(string keyPrefix = "trains:")
    {
        if (_isConnected)
        {
            await DisconnectAsync();
        }

        _cancellationTokenSource = new CancellationTokenSource();
        _streamTask = StreamChangeFeedAsync(keyPrefix, _cancellationTokenSource.Token);
        
        // Wait a bit to ensure connection is established
        await Task.Delay(500);
    }

    public async Task DisconnectAsync()
    {
        if (_cancellationTokenSource != null)
        {
            _cancellationTokenSource.Cancel();
            
            if (_streamTask != null)
            {
                try
                {
                    await _streamTask;
                }
                catch (OperationCanceledException)
                {
                    // Expected when cancelling
                }
            }

            _cancellationTokenSource?.Dispose();
            _cancellationTokenSource = null;
            _streamTask = null;
        }

        _isConnected = false;
        OnConnectionStateChanged(false, null);
    }

    private async Task StreamChangeFeedAsync(string keyPrefix, CancellationToken cancellationToken)
    {
        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                var url = $"/changefeed/stream?key_prefix={keyPrefix}";
                using var request = new HttpRequestMessage(HttpMethod.Get, url);
                request.Headers.Accept.Add(new System.Net.Http.Headers.MediaTypeWithQualityHeaderValue("text/event-stream"));

                using var response = await _httpClient.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, cancellationToken);
                response.EnsureSuccessStatusCode();

                _isConnected = true;
                OnConnectionStateChanged(true, null);

                using var stream = await response.Content.ReadAsStreamAsync(cancellationToken);
                using var reader = new StreamReader(stream);

                string? line;
                string? eventType = null;
                string? data = null;

                while ((line = await reader.ReadLineAsync()) != null && !cancellationToken.IsCancellationRequested)
                {
                    if (string.IsNullOrWhiteSpace(line))
                    {
                        // Empty line indicates end of event
                        if (data != null)
                        {
                            ProcessEvent(eventType, data);
                            data = null;
                            eventType = null;
                        }
                        continue;
                    }

                    if (line.StartsWith("event:"))
                    {
                        eventType = line.Substring(6).Trim();
                    }
                    else if (line.StartsWith("data:"))
                    {
                        data = line.Substring(5).Trim();
                    }
                }
            }
            catch (OperationCanceledException)
            {
                throw; // Re-throw to exit loop
            }
            catch (Exception ex)
            {
                _isConnected = false;
                OnConnectionStateChanged(false, ex.Message);

                // Exponential backoff retry
                if (!cancellationToken.IsCancellationRequested)
                {
                    await Task.Delay(5000, cancellationToken);
                }
            }
        }
    }

    private void ProcessEvent(string? eventType, string data)
    {
        try
        {
            var jsonDoc = JsonDocument.Parse(data);
            var root = jsonDoc.RootElement;

            // Parse train data from CDC event
            var train = new Train();
            
            if (root.TryGetProperty("train_number", out var trainNumber))
                train.TrainNumber = trainNumber.GetString() ?? "";
            
            if (root.TryGetProperty("type", out var type))
                train.Category = type.GetString() ?? "";
            
            if (root.TryGetProperty("operator", out var op))
                train.Operator = op.GetString() ?? "DB Fernverkehr AG";
            
            if (root.TryGetProperty("lat", out var lat))
                train.Latitude = lat.GetDouble();
            
            if (root.TryGetProperty("lon", out var lon))
                train.Longitude = lon.GetDouble();
            
            if (root.TryGetProperty("altitude", out var alt))
                train.Altitude = alt.GetDouble();
            
            if (root.TryGetProperty("speed", out var speed))
                train.SpeedKmh = speed.GetDouble();
            
            if (root.TryGetProperty("heading", out var heading))
                train.Heading = heading.GetDouble();
            
            if (root.TryGetProperty("acceleration", out var accel))
                train.AccelerationMps2 = accel.GetDouble();
            
            if (root.TryGetProperty("origin", out var origin))
                train.Origin = origin.GetString() ?? "";
            
            if (root.TryGetProperty("destination", out var dest))
                train.Destination = dest.GetString() ?? "";
            
            if (root.TryGetProperty("current_segment", out var segment))
                train.CurrentSegment = segment.GetString() ?? "";
            
            if (root.TryGetProperty("distance_traveled", out var distance))
                train.DistanceTraveledKm = distance.GetDouble();
            
            if (root.TryGetProperty("delay", out var delay))
                train.DelayMin = delay.GetInt32();
            
            if (root.TryGetProperty("scheduled_arrival", out var schedArr))
                train.ScheduledArrival = schedArr.GetString() ?? "";
            
            if (root.TryGetProperty("estimated_arrival", out var estArr))
                train.EstimatedArrival = estArr.GetString() ?? "";
            
            if (root.TryGetProperty("passenger_capacity", out var passCap))
                train.PassengerCapacity = passCap.GetInt32();
            
            if (root.TryGetProperty("passenger_count", out var passCount))
                train.PassengerCount = passCount.GetInt32();
            
            if (root.TryGetProperty("power_kw", out var power))
                train.InstantaneousPowerKw = power.GetDouble();
            
            if (root.TryGetProperty("energy_kwh", out var energy))
                train.CumulativeEnergyKwh = energy.GetDouble();
            
            if (root.TryGetProperty("efficiency", out var efficiency))
                train.EfficiencyPercent = efficiency.GetDouble();
            
            if (root.TryGetProperty("status", out var status))
                train.Status = status.GetString() ?? "active";
            
            if (root.TryGetProperty("updated_at", out var updated))
                train.LastUpdate = updated.GetString() ?? DateTime.UtcNow.ToString("O");

            var updateType = eventType switch
            {
                "insert" => "insert",
                "delete" => "delete",
                _ => "update"
            };

            OnTrainUpdated(train, updateType);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error processing CDC event: {ex.Message}");
        }
    }

    private void OnTrainUpdated(Train train, string updateType)
    {
        TrainUpdated?.Invoke(this, new TrainUpdateEventArgs 
        { 
            Train = train, 
            UpdateType = updateType 
        });
    }

    private void OnConnectionStateChanged(bool isConnected, string? errorMessage)
    {
        ConnectionStateChanged?.Invoke(this, new ConnectionStateEventArgs 
        { 
            IsConnected = isConnected, 
            ErrorMessage = errorMessage 
        });
    }

    public void Dispose()
    {
        DisconnectAsync().GetAwaiter().GetResult();
        _httpClient?.Dispose();
    }
}

// Legacy WebSocket interface for backwards compatibility
public interface IWebSocketService
{
    Task ConnectAsync(string url);
    Task DisconnectAsync();
    event EventHandler<string>? MessageReceived;
}

public class WebSocketService : IWebSocketService
{
    public event EventHandler<string>? MessageReceived;
    
    public async Task ConnectAsync(string url)
    {
        await Task.CompletedTask;
        // Legacy WebSocket implementation - use ChangeFeedService instead
    }

    public async Task DisconnectAsync()
    {
        await Task.CompletedTask;
        // Legacy disconnect implementation
    }
}
