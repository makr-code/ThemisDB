/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            App.xaml.cs                                        ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     129                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using RailwayMonitor.WPF.ViewModels;
using RailwayMonitor.WPF.Services;
using Microsoft.Extensions.DependencyInjection;
using Serilog;

namespace RailwayMonitor.WPF;

/// <summary>
/// Railway Monitoring System - WPF Application
/// 
/// Echtzeit-Überwachungssystem für Zugverkehr der Deutschen Bahn
/// - Live-Karte mit OSM
/// - Echtzeit-Telemetrie von ThemisDB
/// - Verspätungsanalyse mit LLM
/// - Was-wäre-wenn Szenarien
/// </summary>
public partial class App : Application
{
    private ServiceProvider? _serviceProvider;

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        // Configure Serilog
        Log.Logger = new LoggerConfiguration()
            .MinimumLevel.Debug()
            .WriteTo.Console()
            .WriteTo.File("logs/railway-monitor-.log", rollingInterval: RollingInterval.Day)
            .CreateLogger();

        Log.Information("Railway Monitoring System starting...");

        // Configure Dependency Injection
        var services = new ServiceCollection();
        ConfigureServices(services);
        _serviceProvider = services.BuildServiceProvider();

        // Show Main Window
        var mainWindow = _serviceProvider.GetRequiredService<MainWindow>();
        mainWindow.Show();
    }

    private void ConfigureServices(IServiceCollection services)
    {
        // Configuration
        services.AddSingleton<IConfiguration>(sp => 
        {
            var config = new AppConfiguration
            {
                ThemisDbUrl = "http://localhost:8765",
                OllamaUrl = "http://localhost:11434",
                UpdateIntervalMs = 1000,
                MaxTrainsDisplay = 500
            };
            return config;
        });

        // Services
        services.AddSingleton<IThemisDbService, ThemisDbService>();
        services.AddSingleton<IEnergyManagementService, EnergyManagementService>();
        services.AddSingleton<IChangeFeedService, ChangeFeedService>();
        services.AddSingleton<ITrainSimulatorService, TrainSimulatorService>();
        services.AddSingleton<IMapService, MapService>();
        services.AddSingleton<ILlmService, OllamaService>();
        services.AddSingleton<IWebSocketService, WebSocketService>();

        // ViewModels
        services.AddTransient<MainViewModel>();
        services.AddTransient<MapViewModel>();
        services.AddTransient<TrainListViewModel>();
        services.AddTransient<DelayAnalysisViewModel>();
        services.AddTransient<NetworkStatusViewModel>();

        // Windows
        services.AddSingleton<MainWindow>();
    }

    protected override void OnExit(ExitEventArgs e)
    {
        Log.Information("Railway Monitoring System shutting down...");
        _serviceProvider?.Dispose();
        Log.CloseAndFlush();
        base.OnExit(e);
    }
}

public interface IConfiguration
{
    string ThemisDbUrl { get; }
    string OllamaUrl { get; }
    int UpdateIntervalMs { get; }
    int MaxTrainsDisplay { get; }
}

public class AppConfiguration : IConfiguration
{
    public string ThemisDbUrl { get; set; } = "http://localhost:8765";
    public string OllamaUrl { get; set; } = "http://localhost:11434";
    public int UpdateIntervalMs { get; set; } = 1000;
    public int MaxTrainsDisplay { get; set; } = 500;
}
