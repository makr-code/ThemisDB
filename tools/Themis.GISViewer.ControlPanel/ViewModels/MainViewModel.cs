/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainViewModel.cs                                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:23:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   74.0/100                                       ║
    • Total Lines:     278                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;
using System.Windows.Media;
using Themis.GISViewer.ControlPanel.Services;

namespace Themis.GISViewer.ControlPanel.ViewModels;

public partial class MainViewModel : ObservableObject
{
    private readonly IUnrealEngineConnector _unrealConnector;
    private readonly IThemisDBService _themisDBService;
    private readonly IPluginService _pluginService;

    [ObservableProperty]
    private string _connectionStatus = "Nicht verbunden";

    [ObservableProperty]
    private SolidColorBrush _connectionStatusColor = new(Colors.Red);

    [ObservableProperty]
    private string _unrealEngineStatus = "Nicht verbunden";

    [ObservableProperty]
    private string _themisDBStatus = "Unbekannt";

    [ObservableProperty]
    private string _statusMessage = "Bereit";

    [ObservableProperty]
    private DateTime _currentTime = DateTime.Now;

    [ObservableProperty]
    private ObservableCollection<PluginInfo> _loadedPlugins = new();

    // Wind Simulation Properties
    [ObservableProperty]
    private double _windSpeed = 10.0;

    [ObservableProperty]
    private double _windDirection = 0.0;

    [ObservableProperty]
    private string _visualizationMode = "Partikel";

    [ObservableProperty]
    private double _maxWindSpeed = 0.0;

    [ObservableProperty]
    private double _averageWindSpeed = 0.0;

    [ObservableProperty]
    private int _particleCount = 0;

    // Water Flow Properties
    [ObservableProperty]
    private double _rainfallIntensity = 0.0;

    [ObservableProperty]
    private double _simulationDuration = 30.0;

    [ObservableProperty]
    private double _floodedAreaKm2 = 0.0;

    [ObservableProperty]
    private double _maxWaterDepth = 0.0;

    [ObservableProperty]
    private int _affectedBuildings = 0;

    // Disaster Simulation Properties
    [ObservableProperty]
    private string _disasterType = "Erdbeben";

    [ObservableProperty]
    private int _disasterIntensity = 5;

    [ObservableProperty]
    private decimal _totalDamageEuro = 0;

    [ObservableProperty]
    private int _collapsedBuildings = 0;

    [ObservableProperty]
    private int _casualtiesEstimate = 0;

    [ObservableProperty]
    private ObservableCollection<PluginInfo> _availablePlugins = new();

    public MainViewModel(
        IUnrealEngineConnector unrealConnector,
        IThemisDBService themisDBService,
        IPluginService pluginService)
    {
        _unrealConnector = unrealConnector;
        _themisDBService = themisDBService;
        _pluginService = pluginService;

        // Start time update timer
        var timer = new System.Windows.Threading.DispatcherTimer
        {
            Interval = TimeSpan.FromSeconds(1)
        };
        timer.Tick += (s, e) => CurrentTime = DateTime.Now;
        timer.Start();
    }

    [RelayCommand]
    private async Task ConnectToUnrealAsync()
    {
        StatusMessage = "Verbinde mit Unreal Engine...";
        var success = await _unrealConnector.ConnectAsync();

        if (success)
        {
            UnrealEngineStatus = "Verbunden";
            ConnectionStatus = "Verbunden";
            ConnectionStatusColor = new SolidColorBrush(Colors.Green);
            StatusMessage = "Erfolgreich mit Unreal Engine verbunden";

            // Load available plugins
            var plugins = await _pluginService.GetAvailablePluginsAsync();
            LoadedPlugins = new ObservableCollection<PluginInfo>(plugins);
        }
        else
        {
            UnrealEngineStatus = "Verbindung fehlgeschlagen";
            ConnectionStatus = "Fehler";
            ConnectionStatusColor = new SolidColorBrush(Colors.Red);
            StatusMessage = "Verbindung zu Unreal Engine fehlgeschlagen";
        }
    }

    [RelayCommand]
    private async Task TestThemisDBAsync()
    {
        StatusMessage = "Teste ThemisDB-Verbindung...";
        var success = await _themisDBService.TestConnectionAsync();

        ThemisDBStatus = success ? "Verbunden" : "Nicht erreichbar";
        StatusMessage = success
            ? "ThemisDB ist erreichbar"
            : "ThemisDB ist nicht erreichbar";
    }

    [RelayCommand]
    private async Task StartSimulationAsync()
    {
        if (!_unrealConnector.IsConnected)
        {
            StatusMessage = "Fehler: Nicht mit Unreal Engine verbunden";
            return;
        }

        StatusMessage = $"Starte Wind-Simulation (Geschwindigkeit: {WindSpeed} m/s, Richtung: {WindDirection}°)";

        await _unrealConnector.SendCommandAsync("StartWindSimulation", new Dictionary<string, object>
        {
            { "ModuleName", "WindSimulation" },
            { "Speed", WindSpeed },
            { "Direction", WindDirection },
            { "VisualizationMode", VisualizationMode }
        });

        StatusMessage = "Wind-Simulation läuft";
    }

    [RelayCommand]
    private async Task StartWaterSimulationAsync()
    {
        if (!_unrealConnector.IsConnected)
        {
            StatusMessage = "Fehler: Nicht mit Unreal Engine verbunden";
            return;
        }

        StatusMessage = $"Starte Wasser-Simulation (Niederschlag: {RainfallIntensity} mm/h)";

        await _unrealConnector.SendCommandAsync("StartWaterSimulation", new Dictionary<string, object>
        {
            { "ModuleName", "WaterFlow" },
            { "RainfallIntensity", RainfallIntensity },
            { "Duration", SimulationDuration }
        });

        StatusMessage = "Wasser-Simulation läuft";
    }

    [RelayCommand]
    private async Task StartDisasterSimulationAsync()
    {
        if (!_unrealConnector.IsConnected)
        {
            StatusMessage = "Fehler: Nicht mit Unreal Engine verbunden";
            return;
        }

        StatusMessage = $"Starte Katastrophen-Simulation ({DisasterType}, Intensität: {DisasterIntensity})";

        await _unrealConnector.SendCommandAsync("StartDisasterSimulation", new Dictionary<string, object>
        {
            { "ModuleName", "DisasterSimulation" },
            { "Type", DisasterType },
            { "Intensity", DisasterIntensity }
        });

        StatusMessage = "Katastrophen-Simulation läuft";
    }

    [RelayCommand]
    private async Task LoadPluginAsync()
    {
        // TODO: Show file dialog to select plugin DLL
        StatusMessage = "Plugin-Laden noch nicht implementiert";
        await Task.CompletedTask;
    }

    [RelayCommand]
    private async Task RefreshPluginsAsync()
    {
        if (!_unrealConnector.IsConnected)
        {
            StatusMessage = "Fehler: Nicht mit Unreal Engine verbunden";
            return;
        }

        StatusMessage = "Aktualisiere Plugin-Liste...";
        var plugins = await _pluginService.GetAvailablePluginsAsync();
        AvailablePlugins = new ObservableCollection<PluginInfo>(plugins);
        StatusMessage = $"{plugins.Count} Plugins gefunden";
    }

    partial void OnWindSpeedChanged(double value)
    {
        if (_unrealConnector.IsConnected)
        {
            _ = _unrealConnector.SendCommandAsync("SetWindSpeed", new Dictionary<string, object>
            {
                { "ModuleName", "WindSimulation" },
                { "Speed", value }
            });
        }
    }

    partial void OnWindDirectionChanged(double value)
    {
        if (_unrealConnector.IsConnected)
        {
            _ = _unrealConnector.SendCommandAsync("SetWindDirection", new Dictionary<string, object>
            {
                { "ModuleName", "WindSimulation" },
                { "Direction", value }
            });
        }
    }
}
