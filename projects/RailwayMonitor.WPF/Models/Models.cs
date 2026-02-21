/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Models.cs                                          ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     165                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace RailwayMonitor.WPF.Models;

/// <summary>
/// Train model with real-time telemetry data
/// </summary>
public class Train
{
    public string TrainNumber { get; set; } = "";
    public string Category { get; set; } = ""; // ICE, IC, RE, RB
    public string Operator { get; set; } = "DB Fernverkehr AG";
    
    // Current position
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public double Altitude { get; set; }
    
    // Movement data
    public double SpeedKmh { get; set; }
    public double Heading { get; set; }
    public double AccelerationMps2 { get; set; }
    
    // Route information
    public string Origin { get; set; } = "";
    public string Destination { get; set; } = "";
    public string CurrentSegment { get; set; } = "";
    public double DistanceTraveledKm { get; set; }
    
    // Delay information
    public int DelayMin { get; set; }
    public bool HasDelay => DelayMin > 0;
    public string ScheduledArrival { get; set; } = "";
    public string EstimatedArrival { get; set; } = "";
    
    // Passenger data
    public int PassengerCapacity { get; set; }
    public int PassengerCount { get; set; }
    public double OccupancyPercent => PassengerCapacity > 0 
        ? (double)PassengerCount / PassengerCapacity * 100 : 0;
    
    // Energy data
    public double InstantaneousPowerKw { get; set; }
    public double CumulativeEnergyKwh { get; set; }
    public double EfficiencyPercent { get; set; }
    
    // Status
    public string Status { get; set; } = "in_service"; // in_service, delayed, cancelled
    public DateTime LastUpdate { get; set; } = DateTime.Now;
}

/// <summary>
/// Station model
/// </summary>
public class Station
{
    public string StationId { get; set; } = "";
    public string Name { get; set; } = "";
    public string EvaNumber { get; set; } = "";
    
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    
    public string Category { get; set; } = ""; // Fernverkehr, Regional, S-Bahn
    public string Operator { get; set; } = "DB Station&Service AG";
    
    // Facilities
    public bool HasParking { get; set; }
    public bool HasBicycleParking { get; set; }
    public bool HasElevator { get; set; }
    public bool HasDbInformation { get; set; }
}

/// <summary>
/// Power source model for energy management
/// </summary>
public class PowerSource
{
    public string Type { get; set; } = ""; // hydro, wind, solar, battery, gas
    public string Name { get; set; } = "";
    
    public double CapacityMw { get; set; }
    public double CurrentOutputMw { get; set; }
    public double UtilizationPercent => CapacityMw > 0 
        ? (CurrentOutputMw / CapacityMw) * 100 : 0;
    
    public double CostEurPerMwh { get; set; }
    public double Co2GPerKwh { get; set; }
    
    public bool IsRenewable => Type is "hydro" or "wind" or "solar";
}

/// <summary>
/// Substation (Unterwerk) model
/// </summary>
public class Substation
{
    public string SubstationId { get; set; } = "";
    public string Name { get; set; } = "";
    
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    
    public double CapacityMw { get; set; }
    public double CurrentLoadMw { get; set; }
    public double UtilizationPercent => CapacityMw > 0 
        ? (CurrentLoadMw / CapacityMw) * 100 : 0;
    
    public double SupplyRangeKm { get; set; } = 50.0;
    public string Status { get; set; } = "operational";
    
    public List<string> ConnectedTracks { get; set; } = new();
}

/// <summary>
/// Energy consumption data
/// </summary>
public class EnergyData
{
    public double InstantaneousPowerKw { get; set; }
    public double CumulativeEnergyKwh { get; set; }
    public double TractionPowerKw { get; set; }
    public double AuxiliaryPowerKw { get; set; }
    public double RecuperationPowerKw { get; set; }
    public double EfficiencyPercent { get; set; }
}

/// <summary>
/// Power forecast point
/// </summary>
public class PowerForecastPoint
{
    public int Hour { get; set; }
    public double PowerMw { get; set; }
    public int TrainsCount { get; set; }
    public double Confidence { get; set; }
}

/// <summary>
/// Power dispatch result
/// </summary>
public class PowerDispatchResult
{
    public Dictionary<string, double> Allocations { get; set; } = new();
    public double TotalCostEur { get; set; }
    public double Co2KgPerMwh { get; set; }
    public double RenewablePercent { get; set; }
}
