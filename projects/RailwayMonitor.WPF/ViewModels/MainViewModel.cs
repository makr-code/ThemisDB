using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Media;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RailwayMonitor.WPF.Services;
using RailwayMonitor.WPF.Models;

namespace RailwayMonitor.WPF.ViewModels;

/// <summary>
/// Main ViewModel for Railway Monitoring System
/// Includes Energy Management & Power Grid Control
/// </summary>
public partial class MainViewModel : ObservableObject
{
    private readonly IThemisDbService _themisDb;
    private readonly ITrainSimulatorService _simulator;
    private readonly ILlmService _llm;
    private readonly IEnergyManagementService _energyService;
    private readonly IMapService _mapService;
    private System.Timers.Timer? _updateTimer;

    public MainViewModel(
        IThemisDbService themisDb,
        ITrainSimulatorService simulator,
        ILlmService llm,
        IEnergyManagementService energyService,
        IMapService mapService)
    {
        _themisDb = themisDb;
        _simulator = simulator;
        _llm = llm;
        _energyService = energyService;
        _mapService = mapService;

        Trains = new ObservableCollection<TrainViewModel>();
        FilteredTrains = new ObservableCollection<TrainViewModel>();
        
        // Energy Management
        PowerSources = new ObservableCollection<PowerSourceViewModel>();
        Substations = new ObservableCollection<SubstationViewModel>();
    }

    public async Task InitializeAsync()
    {
        StatusMessage = "Initialisiere Railway Monitoring System...";
        
        // Connect to ThemisDB
        var connected = await _themisDb.ConnectAsync();
        if (connected)
        {
            ConnectionStatus = "Verbunden";
            ConnectionStatusColor = Brushes.Green;
        }
        else
        {
            ConnectionStatus = "Keine Verbindung";
            ConnectionStatusColor = Brushes.Red;
        }

        // Initialize Energy Management
        await InitializeEnergyManagementAsync();

        // Load initial data
        await LoadStationsAsync();
        await LoadTrainsAsync();

        // Start update timer
        _updateTimer = new System.Timers.Timer(1000); // 1 second
        _updateTimer.Elapsed += async (s, e) => await UpdateAsync();
        _updateTimer.Start();

        StatusMessage = "System bereit";
    }

    private async Task InitializeEnergyManagementAsync()
    {
        // Load Power Sources
        var sources = await _energyService.GetPowerSourcesAsync();
        PowerSources.Clear();
        foreach (var source in sources)
        {
            PowerSources.Add(new PowerSourceViewModel(source));
        }

        // Load Substations
        var substations = await _energyService.GetSubstationsAsync();
        Substations.Clear();
        foreach (var substation in substations)
        {
            Substations.Add(new SubstationViewModel(substation));
        }

        // Calculate initial energy forecast
        await UpdateEnergyForecastAsync();
    }

    private async Task UpdateAsync()
    {
        // Update trains
        var trains = await _themisDb.GetActiveTrainsAsync();
        
        Application.Current.Dispatcher.Invoke(() =>
        {
            Trains.Clear();
            foreach (var train in trains)
            {
                Trains.Add(new TrainViewModel(train));
            }
            
            UpdateStatistics();
            UpdateEnergyConsumption();
        });

        LastUpdateTime = DateTime.Now;
        TotalUpdates++;
    }

    private void UpdateStatistics()
    {
        ActiveTrains = Trains.Count;
        DelaysOver5Min = Trains.Count(t => t.DelayMin > 5);
        AverageDelay = Trains.Any() ? Trains.Average(t => t.DelayMin) : 0;
    }

    private async void UpdateEnergyConsumption()
    {
        // Calculate current total power consumption
        double totalPowerMw = 0;
        foreach (var train in Trains)
        {
            var energyData = await _energyService.CalculateTrainEnergyAsync(train.Model);
            train.InstantaneousPowerKw = energyData.InstantaneousPowerKw;
            totalPowerMw += energyData.InstantaneousPowerKw / 1000.0;
        }

        CurrentGridLoadMw = totalPowerMw;
        GridUtilizationPercent = (totalPowerMw / TotalGridCapacityMw) * 100;

        // Update substations
        await UpdateSubstationLoadsAsync();

        // Update power source allocation
        await OptimizePowerDispatchAsync();

        // Check for overload conditions
        CheckEnergyAlerts();
    }

    private async Task UpdateSubstationLoadsAsync()
    {
        foreach (var substation in Substations)
        {
            var trainsInRange = Trains.Where(t => 
                IsTrainInSubstationRange(t, substation));
            
            double substationLoad = trainsInRange.Sum(t => t.InstantaneousPowerKw / 1000.0);
            substation.CurrentLoadMw = substationLoad;
            substation.UtilizationPercent = (substationLoad / substation.CapacityMw) * 100;
        }
    }

    private bool IsTrainInSubstationRange(TrainViewModel train, SubstationViewModel substation)
    {
        // Check if train is on track supplied by substation
        // Simplified: distance-based
        var distance = CalculateDistance(train.Latitude, train.Longitude,
                                        substation.Latitude, substation.Longitude);
        return distance < substation.SupplyRangeKm;
    }

    private double CalculateDistance(double lat1, double lon1, double lat2, double lon2)
    {
        // Haversine formula
        var R = 6371; // Earth radius in km
        var dLat = (lat2 - lat1) * Math.PI / 180;
        var dLon = (lon2 - lon1) * Math.PI / 180;
        var a = Math.Sin(dLat / 2) * Math.Sin(dLat / 2) +
                Math.Cos(lat1 * Math.PI / 180) * Math.Cos(lat2 * Math.PI / 180) *
                Math.Sin(dLon / 2) * Math.Sin(dLon / 2);
        var c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));
        return R * c;
    }

    private async Task OptimizePowerDispatchAsync()
    {
        // Get power demand forecast
        var forecast = await _energyService.ForecastPowerDemandAsync(24);
        
        // Optimize power source allocation
        var dispatch = await _energyService.OptimizeDispatchAsync(
            CurrentGridLoadMw,
            optimizeFor: "cost" // or "co2", "reliability"
        );

        // Update power source allocations
        Application.Current.Dispatcher.Invoke(() =>
        {
            foreach (var allocation in dispatch.Allocations)
            {
                var source = PowerSources.FirstOrDefault(s => s.Type == allocation.Key);
                if (source != null)
                {
                    source.CurrentOutputMw = allocation.Value;
                    source.UtilizationPercent = (allocation.Value / source.CapacityMw) * 100;
                }
            }

            // Update grid metrics
            RenewableSharePercent = dispatch.RenewablePercent;
            CurrentCo2KgPerMwh = dispatch.Co2KgPerMwh;
            EstimatedCostEur = dispatch.TotalCostEur;
        });
    }

    private async Task UpdateEnergyForecastAsync()
    {
        var forecast = await _energyService.ForecastPowerDemandAsync(24);
        
        // Update forecast chart data
        Application.Current.Dispatcher.Invoke(() =>
        {
            PowerForecast = new ObservableCollection<PowerForecastPoint>(forecast);
        });
    }

    private void CheckEnergyAlerts()
    {
        // Check for overloaded substations
        var overloaded = Substations.Where(s => s.UtilizationPercent > 90).ToList();
        if (overloaded.Any())
        {
            var message = $"⚠️ {overloaded.Count} Unterwerk(e) überlastet!";
            StatusMessage = message;
            // Trigger alert
        }

        // Check grid utilization
        if (GridUtilizationPercent > 85)
        {
            StatusMessage = $"⚠️ Netzauslastung kritisch: {GridUtilizationPercent:F1}%";
        }

        // Check renewable target
        if (RenewableSharePercent < 50)
        {
            StatusMessage = $"ℹ️ Grünstrom-Anteil niedrig: {RenewableSharePercent:F1}%";
        }
    }

    // Properties
    [ObservableProperty]
    private int activeTrains;

    [ObservableProperty]
    private int delaysOver5Min;

    [ObservableProperty]
    private double averageDelay;

    [ObservableProperty]
    private string connectionStatus = "Getrennt";

    [ObservableProperty]
    private Brush connectionStatusColor = Brushes.Red;

    [ObservableProperty]
    private string statusMessage = "";

    [ObservableProperty]
    private DateTime lastUpdateTime;

    [ObservableProperty]
    private int totalUpdates;

    // Energy Management Properties
    [ObservableProperty]
    private double currentGridLoadMw;

    [ObservableProperty]
    private double totalGridCapacityMw = 1000; // MW

    [ObservableProperty]
    private double gridUtilizationPercent;

    [ObservableProperty]
    private double renewableSharePercent;

    [ObservableProperty]
    private double currentCo2KgPerMwh;

    [ObservableProperty]
    private double estimatedCostEur;

    // Collections
    public ObservableCollection<TrainViewModel> Trains { get; }
    public ObservableCollection<TrainViewModel> FilteredTrains { get; }
    public ObservableCollection<PowerSourceViewModel> PowerSources { get; }
    public ObservableCollection<SubstationViewModel> Substations { get; }
    
    [ObservableProperty]
    private ObservableCollection<PowerForecastPoint>? powerForecast;

    // Commands
    [RelayCommand]
    private async Task AnalyzeWithLlm()
    {
        if (string.IsNullOrWhiteSpace(LlmQuery))
            return;

        LlmAnswer = "Analysiere...";
        
        var answer = await _llm.QueryAsync(LlmQuery, new
        {
            trains = Trains.Take(10),
            energy = new { CurrentGridLoadMw, RenewableSharePercent }
        });

        LlmAnswer = answer;
    }

    [ObservableProperty]
    private string llmQuery = "";

    [ObservableProperty]
    private string llmAnswer = "";

    [RelayCommand]
    private async Task OptimizeEnergyForCost()
    {
        StatusMessage = "Optimiere Energieverbrauch für minimale Kosten...";
        await OptimizePowerDispatchAsync();
        StatusMessage = $"Optimierung abgeschlossen. Geschätzte Kosten: {EstimatedCostEur:F2} EUR/h";
    }

    [RelayCommand]
    private async Task OptimizeEnergyForCo2()
    {
        StatusMessage = "Optimiere Energieverbrauch für minimale CO₂-Emissionen...";
        var dispatch = await _energyService.OptimizeDispatchAsync(
            CurrentGridLoadMw,
            optimizeFor: "co2"
        );
        StatusMessage = $"CO₂-optimiert. Emissionen: {dispatch.Co2KgPerMwh:F1} kg/MWh";
    }

    private async Task LoadStationsAsync()
    {
        // Load from ThemisDB
        var stations = await _themisDb.GetStationsAsync();
        // Add to map
    }

    private async Task LoadTrainsAsync()
    {
        var trains = await _themisDb.GetActiveTrainsAsync();
        // Update collection
    }
}
