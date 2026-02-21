/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            App.xaml.cs                                        ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:48:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     113                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bde12d3c6  2026-01-02  🔥 HOTFIX: Critical RocksDB Segmentation Fault Fix (v1.3.4) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Microsoft.Extensions.DependencyInjection;
using System;
using System.Net.Http;
using System.Windows;
using Themis.IngestionTool.Services;
using Themis.IngestionTool.ViewModels;
using Themis.IngestionTool.Views;

namespace Themis.IngestionTool
{
    public partial class App : Application
    {
        private readonly ServiceProvider _serviceProvider;

        public ServiceProvider Services => _serviceProvider;

        public App()
        {
            var services = new ServiceCollection();
            ConfigureServices(services);
            _serviceProvider = services.BuildServiceProvider();
        }

        private void ConfigureServices(IServiceCollection services)
        {
            // Core Services
            services.AddSingleton<ISettingsService, SettingsService>();
            services.AddSingleton<ILoggerService, LoggerService>();
            services.AddSingleton<IThemisConnectionService, ThemisConnectionService>();
            services.AddSingleton<ILlmStatusService, LlmStatusService>();
            services.AddSingleton<IIngestionService, IngestionService>();
            
            // Analysis Services
            services.AddSingleton<ILlamaService, LlamaHttpService>();
            services.AddSingleton<INlpAnalysisService, NlpAnalysisService>();
            services.AddSingleton<IGraphAnalysisService, GraphAnalysisService>();
            
            // Embedding Service (Real HTTP-based)
            services.AddSingleton<IEmbeddingService, OllamaEmbeddingService>();
            
            // Resilience & Caching
            services.AddSingleton<HttpClient>(new HttpClient());
            services.AddSingleton<IHttpResilienceService, PollyHttpResilienceService>();
            services.AddSingleton<ICacheService, LRUCacheService>();
            
            // ThemisDB API Service
            services.AddSingleton<IThemisApiService, ThemisApiService>();
            
            // Graph & Vector Query Services
            services.AddSingleton<IGraphQueryService, GraphQueryService>();
            services.AddSingleton<IVectorQueryService, VectorQueryService>();
            
            // Pipeline Service
            services.AddSingleton<IIngestionPipelineService, IngestionPipelineService>();
            
            // Real Ingestion Service (Direct ThemisDB Storage)
            services.AddSingleton<IRealIngestionService, RealIngestionService>();

            // Load Testing
            services.AddSingleton<ILoadTestRunner, LoadTestRunner>();
            services.AddSingleton<IPerformanceProfiler, PerformanceProfiler>();

            // ViewModels
            services.AddSingleton<MainWindowViewModel>();
            services.AddTransient<SettingsDialogViewModel>();
            services.AddTransient<GraphQueryDialogViewModel>();
            services.AddTransient<VectorQueryDialogViewModel>();
            services.AddTransient<CacheStatisticsViewModel>();
            services.AddTransient<LoadTestViewModel>();

            // Views
            services.AddSingleton<MainWindow>();
            services.AddTransient<SettingsDialog>();
        }

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);
            var mainWindow = _serviceProvider.GetRequiredService<MainWindow>();
            mainWindow.Show();
        }

        protected override void OnExit(ExitEventArgs e)
        {
            _serviceProvider?.Dispose();
            base.OnExit(e);
        }
    }
}
