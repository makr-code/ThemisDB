/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            App.xaml.cs                                        ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:54:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     97                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using System.Windows;
using Themis.GISViewer.ControlPanel.Services;
using Themis.GISViewer.ControlPanel.ViewModels;

namespace Themis.GISViewer.ControlPanel;

public partial class App : Application
{
    private readonly IHost _host;

    public App()
    {
        _host = Host.CreateDefaultBuilder()
            .ConfigureAppConfiguration((context, config) =>
            {
                config.AddJsonFile("appsettings.json", optional: false, reloadOnChange: true);
            })
            .ConfigureServices((context, services) =>
            {
                // Configuration
                services.Configure<ThemisDBConfiguration>(context.Configuration.GetSection("ThemisDB"));
                services.Configure<UnrealEngineConfiguration>(context.Configuration.GetSection("UnrealEngine"));
                
                // Services
                services.AddSingleton<IUnrealEngineConnector, UnrealEngineConnector>();
                services.AddSingleton<IThemisDBService, ThemisDBService>();
                services.AddSingleton<IPluginService, PluginService>();
                
                // ViewModels
                services.AddTransient<MainViewModel>();
                services.AddTransient<PluginManagerViewModel>();
                services.AddTransient<WindSimulationViewModel>();
                services.AddTransient<WaterFlowViewModel>();
                services.AddTransient<DisasterSimulationViewModel>();
                
                // Main Window
                services.AddSingleton<MainWindow>();
            })
            .Build();
    }

    protected override async void OnStartup(StartupEventArgs e)
    {
        await _host.StartAsync();

        var mainWindow = _host.Services.GetRequiredService<MainWindow>();
        mainWindow.Show();

        base.OnStartup(e);
    }

    protected override async void OnExit(ExitEventArgs e)
    {
        await _host.StopAsync();
        _host.Dispose();

        base.OnExit(e);
    }
}

public class ThemisDBConfiguration
{
    public string ApiUrl { get; set; } = "http://localhost:8765";
    public int Timeout { get; set; } = 30;
}

public class UnrealEngineConfiguration
{
    public string IPCMethod { get; set; } = "NamedPipes";
    public string PipeName { get; set; } = "ThemisGISViewer_IPC";
    public string ExecutablePath { get; set; } = "";
}
