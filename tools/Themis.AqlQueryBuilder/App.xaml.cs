/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            App.xaml.cs                                        ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     97                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

﻿using System.Configuration;
using System.Data;
using System.Globalization;
using System.Net.Http;
using System.Windows;
using System.Windows.Data;
using Themis.AqlQueryBuilder.Infrastructure;
using Themis.AqlQueryBuilder.Services;
using Themis.AqlQueryBuilder.ViewModels;

namespace Themis.AqlQueryBuilder;

/// <summary>
/// Interaction logic for App.xaml
/// </summary>
public partial class App : Application
{
    private static ServiceContainer? _services;
    
    /// <summary>
    /// Gets the global service container instance
    /// </summary>
    public static ServiceContainer Services => _services ?? throw new InvalidOperationException("Service container not initialized");

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);
        
        // Initialize service container
        _services = new ServiceContainer();
        
        // Register services (singleton instances)
        _services.RegisterSingleton<ISchemaService, SchemaService>();
        _services.RegisterSingleton<IQueryHistoryService, QueryHistoryService>();
        
        // Register HttpClient as singleton
        _services.RegisterSingleton<HttpClient>(() => new HttpClient());
        
        // Register AqlQueryService with factory that injects HttpClient and server URL
        _services.RegisterSingleton<IAqlQueryService>(() => 
            new AqlQueryService(_services.Resolve<HttpClient>(), "http://localhost:8080"));
        
        // Note: MainViewModel is not registered here as it needs to be created by the window
        // The window will use the constructor to inject dependencies
    }
}

/// <summary>
/// Converter for bool to int (for ASC/DESC dropdown)
/// </summary>
public class BoolToIntConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is bool boolValue)
        {
            return boolValue ? 0 : 1; // true = ASC (0), false = DESC (1)
        }
        return 0;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is int intValue)
        {
            return intValue == 0; // 0 = ASC (true), 1 = DESC (false)
        }
        return true;
    }
}

