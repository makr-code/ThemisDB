using System.Diagnostics;
using System.Net.Http.Json;
using RailwayMonitor.WPF.Models;

namespace RailwayMonitor.WPF.Services;

/// <summary>
/// Service for communicating with ThemisDB backend
/// </summary>
public interface IThemisDbService
{
    Task<bool> ConnectAsync();
    Task<List<Train>> GetActiveTrainsAsync();
    Task<List<Station>> GetStationsAsync();
    Task<Train?> GetTrainAsync(string trainNumber);
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
            // In production, this would query ThemisDB time-series data
            // For now, return sample data
            var response = await _httpClient.GetAsync("/api/trains/active");
            if (response.IsSuccessStatusCode)
            {
                var trains = await response.Content.ReadFromJsonAsync<List<Train>>();
                return trains ?? new List<Train>();
            }
        }
        catch
        {
            // If ThemisDB is not available, return empty list
        }
        
        return new List<Train>();
    }

    public async Task<List<Station>> GetStationsAsync()
    {
        try
        {
            var response = await _httpClient.GetAsync("/api/stations");
            if (response.IsSuccessStatusCode)
            {
                var stations = await response.Content.ReadFromJsonAsync<List<Station>>();
                return stations ?? new List<Station>();
            }
        }
        catch
        {
            // Return empty list if service unavailable
        }
        
        return new List<Station>();
    }

    public async Task<Train?> GetTrainAsync(string trainNumber)
    {
        try
        {
            var response = await _httpClient.GetAsync($"/api/trains/{trainNumber}");
            if (response.IsSuccessStatusCode)
            {
                return await response.Content.ReadFromJsonAsync<Train>();
            }
        }
        catch
        {
            // Return null if not found
        }
        
        return null;
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

/// <summary>
/// Service for map operations
/// </summary>
public interface IMapService
{
    // Map service would handle Mapsui operations
}

public class MapService : IMapService
{
    // Implementation for map operations
}

/// <summary>
/// WebSocket service for real-time updates
/// </summary>
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
        // WebSocket implementation
    }

    public async Task DisconnectAsync()
    {
        await Task.CompletedTask;
        // Disconnect implementation
    }
}
