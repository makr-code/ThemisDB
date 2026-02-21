/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ComponentViewModels.cs                             ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     221                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows.Media;
using CommunityToolkit.Mvvm.ComponentModel;
using RailwayMonitor.WPF.Models;

namespace RailwayMonitor.WPF.ViewModels;

/// <summary>
/// ViewModel wrapper for Train model
/// </summary>
public partial class TrainViewModel : ObservableObject
{
    public Train Model { get; }

    public TrainViewModel(Train train)
    {
        Model = train;
    }

    public string TrainNumber => Model.TrainNumber;
    public string Category => Model.Category;
    public double Latitude => Model.Latitude;
    public double Longitude => Model.Longitude;
    public double SpeedKmh => Model.SpeedKmh;
    public int DelayMin => Model.DelayMin;
    public string Route => $"{Model.Origin} → {Model.Destination}";
    
    public string DelayText => DelayMin switch
    {
        0 => "Pünktlich",
        > 0 => $"+{DelayMin} min",
        < 0 => $"{DelayMin} min"
    };
    
    public Brush DelayColor => DelayMin switch
    {
        0 => new SolidColorBrush(Color.FromRgb(46, 204, 113)), // Green
        <= 5 => new SolidColorBrush(Color.FromRgb(241, 196, 15)), // Yellow
        _ => new SolidColorBrush(Color.FromRgb(231, 76, 60)) // Red
    };
    
    public Brush DelayTextColor => DelayMin switch
    {
        0 => Brushes.White,
        <= 5 => Brushes.Black,
        _ => Brushes.White
    };
    
    public Brush CategoryColor => Category switch
    {
        "ICE" => new SolidColorBrush(Color.FromRgb(231, 76, 60)), // Red
        "IC" => new SolidColorBrush(Color.FromRgb(243, 156, 18)), // Orange
        "RE" => new SolidColorBrush(Color.FromRgb(52, 152, 219)), // Blue
        "RB" => new SolidColorBrush(Color.FromRgb(46, 204, 113)), // Green
        _ => Brushes.Gray
    };

    [ObservableProperty]
    private double instantaneousPowerKw;
}

/// <summary>
/// ViewModel wrapper for PowerSource model
/// </summary>
public partial class PowerSourceViewModel : ObservableObject
{
    private readonly PowerSource _model;

    public PowerSourceViewModel(PowerSource source)
    {
        _model = source;
    }

    public string Type => _model.Type;
    public string Name => _model.Name;
    public double CapacityMw => _model.CapacityMw;
    
    [ObservableProperty]
    private double currentOutputMw;
    
    [ObservableProperty]
    private double utilizationPercent;
    
    public bool IsRenewable => _model.IsRenewable;
    
    public Brush TypeColor => Type switch
    {
        "hydro" => new SolidColorBrush(Color.FromRgb(52, 152, 219)), // Blue
        "wind" => new SolidColorBrush(Color.FromRgb(149, 165, 166)), // Gray
        "solar" => new SolidColorBrush(Color.FromRgb(241, 196, 15)), // Yellow
        "battery" => new SolidColorBrush(Color.FromRgb(155, 89, 182)), // Purple
        "gas" => new SolidColorBrush(Color.FromRgb(231, 76, 60)), // Red
        _ => Brushes.Gray
    };
}

/// <summary>
/// ViewModel wrapper for Substation model
/// </summary>
public partial class SubstationViewModel : ObservableObject
{
    private readonly Substation _model;

    public SubstationViewModel(Substation substation)
    {
        _model = substation;
    }

    public string SubstationId => _model.SubstationId;
    public string Name => _model.Name;
    public double Latitude => _model.Latitude;
    public double Longitude => _model.Longitude;
    public double CapacityMw => _model.CapacityMw;
    public double SupplyRangeKm => _model.SupplyRangeKm;
    
    [ObservableProperty]
    private double currentLoadMw;
    
    [ObservableProperty]
    private double utilizationPercent;
    
    public Brush StatusColor => UtilizationPercent switch
    {
        < 75 => Brushes.Green,
        < 90 => Brushes.Yellow,
        _ => Brushes.Red
    };
}

/// <summary>
/// ViewModel for map view
/// </summary>
public partial class MapViewModel : ObservableObject
{
    [ObservableProperty]
    private bool showTrains = true;
    
    [ObservableProperty]
    private bool showStations = true;
    
    [ObservableProperty]
    private bool showTracks = false;
    
    [ObservableProperty]
    private bool showSignals = false;
}

/// <summary>
/// ViewModel for train list
/// </summary>
public partial class TrainListViewModel : ObservableObject
{
    [ObservableProperty]
    private string searchText = "";
}

/// <summary>
/// ViewModel for delay analysis
/// </summary>
public partial class DelayAnalysisViewModel : ObservableObject
{
    [ObservableProperty]
    private string query = "";
    
    [ObservableProperty]
    private string answer = "";
}

/// <summary>
/// ViewModel for network status
/// </summary>
public partial class NetworkStatusViewModel : ObservableObject
{
    [ObservableProperty]
    private string themisDbStatus = "Disconnected";
    
    [ObservableProperty]
    private Brush themisDbStatusColor = Brushes.Red;
    
    [ObservableProperty]
    private string ollamaStatus = "Disconnected";
    
    [ObservableProperty]
    private Brush ollamaStatusColor = Brushes.Red;
    
    [ObservableProperty]
    private string simulatorStatus = "Stopped";
    
    [ObservableProperty]
    private Brush simulatorStatusColor = Brushes.Gray;
    
    [ObservableProperty]
    private bool simulatorRunning = false;
    
    public bool SimulatorNotRunning => !SimulatorRunning;
}
