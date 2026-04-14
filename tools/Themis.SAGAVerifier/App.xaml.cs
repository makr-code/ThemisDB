/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            App.xaml.cs                                        ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:54:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     98                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.IO;
using System.Net.Http;
using System.Text.Json;
using System.Windows;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Http;
using Themis.AdminTools.Shared.ApiClient;
using Themis.AdminTools.Shared.Models;
using Themis.SAGAVerifier.ViewModels;

namespace Themis.SAGAVerifier;

public partial class App : Application
{
    private ServiceProvider? _serviceProvider;

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        try
        {
            // Build configuration
            var configuration = new ConfigurationBuilder()
                .SetBasePath(Directory.GetCurrentDirectory())
                .AddJsonFile("appsettings.json", optional: false, reloadOnChange: true)
                .Build();

            // Setup DI
            var services = new ServiceCollection();

            // Register configuration
            var serverConfig = configuration.GetSection("ThemisServer").Get<ThemisServerConfig>() 
                ?? new ThemisServerConfig { BaseUrl = "http://localhost:8765", Timeout = 30 };
            services.AddSingleton(serverConfig);

            // Register API client
            services.AddTransient<ThemisApiClient>(sp =>
            {
                var config = sp.GetRequiredService<ThemisServerConfig>();
                var httpClient = new HttpClient
                {
                    BaseAddress = new Uri(config.BaseUrl),
                    Timeout = TimeSpan.FromSeconds(config.Timeout)
                };
                return new ThemisApiClient(httpClient, config);
            });

            // Register ViewModels
            services.AddTransient<MainViewModel>();
            
            // Register Windows
            services.AddTransient<MainWindow>();

            _serviceProvider = services.BuildServiceProvider();

            // Show main window
            var mainWindow = _serviceProvider.GetRequiredService<MainWindow>();
            mainWindow.Show();
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Failed to start application:\n\n{ex.Message}\n\n{ex.StackTrace}", 
                "Startup Error", MessageBoxButton.OK, MessageBoxImage.Error);
            Shutdown(1);
        }
    }

    protected override void OnExit(ExitEventArgs e)
    {
        _serviceProvider?.Dispose();
        base.OnExit(e);
    }
}
