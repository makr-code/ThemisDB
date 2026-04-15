/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SettingsDialogViewModel.cs                         ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:19:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     415                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.IngestionTool.Models;
using Themis.IngestionTool.Services;

namespace Themis.IngestionTool.ViewModels
{
    public class SettingsDialogViewModel : BaseViewModel
    {
        private readonly ISettingsService _settingsService;
        private string _themisHost = "localhost";
        private int _themisPort = 8765;
        private string _databasePath = "ingestion_tracker.db";
        private int _maxFileSize = 100;
        private bool _enableVectorMetadata = true;
        private bool _enableGraphMetadata = true;
        private bool _enableRelationalMetadata = true;
        
        // LLM Settings
        private string _llamaEndpoint = "http://localhost:11434/api/generate";
        private string _llamaModel = "llama2";
        private int _llamaMaxTokens = 200;
        private double _llamaTemperature = 0.7;
        
        // Pipeline Settings
        private int _maxParallelFiles = 4;
        private bool _enableBatching = true;
        private int _batchSize = 10;
        private bool _enableCaching = true;
        
        // ThemisDB API Features
        private bool _useTransactions = true;
        private bool _useBatchOperations = true;
        private bool _storeVectors = true;
        private bool _trackTimeSeries = true;

        // LLM Status Monitoring
        private bool _enableLlmStatusMonitoring = true;
        private int _llmStatusCheckIntervalSeconds = 10;
        private bool _showLlmStatusInStatusBar = true;

        // Server Discovery
        private System.Collections.ObjectModel.ObservableCollection<string> _availableServers = new();
        private string? _selectedServer;
        private bool _isScanning = false;
        private string _scanStatus = "Bereit zum Scannen";

        public SettingsDialogViewModel(ISettingsService settingsService)
        {
            _settingsService = settingsService;
            LoadSettings();
        }

        // Themis Connection
        public string ThemisHost
        {
            get => _themisHost;
            set => SetProperty(ref _themisHost, value);
        }

        public int ThemisPort
        {
            get => _themisPort;
            set => SetProperty(ref _themisPort, value);
        }

        // Database
        public string DatabasePath
        {
            get => _databasePath;
            set => SetProperty(ref _databasePath, value);
        }

        public int MaxFileSize
        {
            get => _maxFileSize;
            set => SetProperty(ref _maxFileSize, value);
        }

        // Metadata Features
        public bool EnableVectorMetadata
        {
            get => _enableVectorMetadata;
            set => SetProperty(ref _enableVectorMetadata, value);
        }

        public bool EnableGraphMetadata
        {
            get => _enableGraphMetadata;
            set => SetProperty(ref _enableGraphMetadata, value);
        }

        public bool EnableRelationalMetadata
        {
            get => _enableRelationalMetadata;
            set => SetProperty(ref _enableRelationalMetadata, value);
        }

        // LLM Settings
        public string LlamaEndpoint
        {
            get => _llamaEndpoint;
            set => SetProperty(ref _llamaEndpoint, value);
        }

        public string LlamaModel
        {
            get => _llamaModel;
            set => SetProperty(ref _llamaModel, value);
        }

        public int LlamaMaxTokens
        {
            get => _llamaMaxTokens;
            set => SetProperty(ref _llamaMaxTokens, value);
        }

        public double LlamaTemperature
        {
            get => _llamaTemperature;
            set => SetProperty(ref _llamaTemperature, value);
        }

        // Pipeline Settings
        public int MaxParallelFiles
        {
            get => _maxParallelFiles;
            set => SetProperty(ref _maxParallelFiles, value);
        }

        public bool EnableBatching
        {
            get => _enableBatching;
            set => SetProperty(ref _enableBatching, value);
        }

        public int BatchSize
        {
            get => _batchSize;
            set => SetProperty(ref _batchSize, value);
        }

        public bool EnableCaching
        {
            get => _enableCaching;
            set => SetProperty(ref _enableCaching, value);
        }

        // ThemisDB API Features
        public bool UseTransactions
        {
            get => _useTransactions;
            set => SetProperty(ref _useTransactions, value);
        }

        public bool UseBatchOperations
        {
            get => _useBatchOperations;
            set => SetProperty(ref _useBatchOperations, value);
        }

        public bool StoreVectors
        {
            get => _storeVectors;
            set => SetProperty(ref _storeVectors, value);
        }

        public bool TrackTimeSeries
        {
            get => _trackTimeSeries;
            set => SetProperty(ref _trackTimeSeries, value);
        }

        // LLM Status Monitoring Settings
        public bool EnableLlmStatusMonitoring
        {
            get => _enableLlmStatusMonitoring;
            set => SetProperty(ref _enableLlmStatusMonitoring, value);
        }

        public int LlmStatusCheckIntervalSeconds
        {
            get => _llmStatusCheckIntervalSeconds;
            set => SetProperty(ref _llmStatusCheckIntervalSeconds, value);
        }

        public bool ShowLlmStatusInStatusBar
        {
            get => _showLlmStatusInStatusBar;
            set => SetProperty(ref _showLlmStatusInStatusBar, value);
        }

        // Server Discovery Properties
        public System.Collections.ObjectModel.ObservableCollection<string> AvailableServers
        {
            get => _availableServers;
            set => SetProperty(ref _availableServers, value);
        }

        public string? SelectedServer
        {
            get => _selectedServer;
            set => SetProperty(ref _selectedServer, value);
        }

        public bool IsScanning
        {
            get => _isScanning;
            set => SetProperty(ref _isScanning, value);
        }

        public string ScanStatus
        {
            get => _scanStatus;
            set => SetProperty(ref _scanStatus, value);
        }

        private void LoadSettings()
        {
            var settings = _settingsService.LoadSettings();
            ThemisHost = settings.ThemisHost;
            ThemisPort = settings.ThemisPort;
            DatabasePath = settings.DatabasePath;
            MaxFileSize = settings.MaxFileSize;
            EnableVectorMetadata = settings.EnableVectorMetadata;
            EnableGraphMetadata = settings.EnableGraphMetadata;
            EnableRelationalMetadata = settings.EnableRelationalMetadata;
            
            LlamaEndpoint = settings.LlamaEndpoint;
            LlamaModel = settings.LlamaModel;
            LlamaMaxTokens = settings.LlamaMaxTokens;
            LlamaTemperature = settings.LlamaTemperature;
            
            MaxParallelFiles = settings.MaxParallelFiles;
            EnableBatching = settings.EnableBatching;
            BatchSize = settings.BatchSize;
            EnableCaching = settings.EnableCaching;
            
            UseTransactions = settings.UseTransactions;
            UseBatchOperations = settings.UseBatchOperations;
            StoreVectors = settings.StoreVectors;
            TrackTimeSeries = settings.TrackTimeSeries;
            
            EnableLlmStatusMonitoring = settings.EnableLlmStatusMonitoring;
            LlmStatusCheckIntervalSeconds = settings.LlmStatusCheckIntervalSeconds;
            ShowLlmStatusInStatusBar = settings.ShowLlmStatusInStatusBar;
        }

        public void SaveSettings()
        {
            var settings = new AppSettings
            {
                ThemisHost = ThemisHost,
                ThemisPort = ThemisPort,
                DatabasePath = DatabasePath,
                MaxFileSize = MaxFileSize,
                EnableVectorMetadata = EnableVectorMetadata,
                EnableGraphMetadata = EnableGraphMetadata,
                EnableRelationalMetadata = EnableRelationalMetadata,
                
                LlamaEndpoint = LlamaEndpoint,
                LlamaModel = LlamaModel,
                LlamaMaxTokens = LlamaMaxTokens,
                LlamaTemperature = LlamaTemperature,
                
                MaxParallelFiles = MaxParallelFiles,
                EnableBatching = EnableBatching,
                BatchSize = BatchSize,
                EnableCaching = EnableCaching,
                
                UseTransactions = UseTransactions,
                UseBatchOperations = UseBatchOperations,
                StoreVectors = StoreVectors,
                TrackTimeSeries = TrackTimeSeries,
                
                EnableLlmStatusMonitoring = EnableLlmStatusMonitoring,
                LlmStatusCheckIntervalSeconds = LlmStatusCheckIntervalSeconds,
                ShowLlmStatusInStatusBar = ShowLlmStatusInStatusBar
            };
            _settingsService.SaveSettings(settings);
        }

        public void ResetToDefaults()
        {
            ThemisHost = "localhost";
            ThemisPort = 8765;
            DatabasePath = "ingestion_tracker.db";
            MaxFileSize = 100;
            EnableVectorMetadata = true;
            EnableGraphMetadata = true;
            EnableRelationalMetadata = true;
            
            LlamaEndpoint = "http://localhost:11434/api/generate";
            LlamaModel = "llama2";
            LlamaMaxTokens = 200;
            LlamaTemperature = 0.7;
            
            MaxParallelFiles = 4;
            EnableBatching = true;
            BatchSize = 10;
            EnableCaching = true;
            
            UseTransactions = true;
            UseBatchOperations = true;
            StoreVectors = true;
            TrackTimeSeries = true;
            
            EnableLlmStatusMonitoring = true;
            LlmStatusCheckIntervalSeconds = 10;
            ShowLlmStatusInStatusBar = true;
        }

        public async System.Threading.Tasks.Task ScanForServersAsync()
        {
            IsScanning = true;
            ScanStatus = "Scanne Netzwerk...";
            AvailableServers.Clear();

            var commonPorts = new[] { 8765, 8081, 8082, 8083, 8084, 8085, 8086, 8087, 8088, 8089, 18766, 18767, 18768, 18769, 18770 };
            var endpoints = new[] { "/health", "/api/health", "/status", "/", "/api/v1/health" };
            
            int found = 0;
            int tested = 0;
            using (var client = new System.Net.Http.HttpClient { Timeout = System.TimeSpan.FromSeconds(2) })
            {
                foreach (var port in commonPorts)
                {
                    ScanStatus = $"Prüfe Port {port}... ({tested}/{commonPorts.Length * endpoints.Length})";
                    
                    foreach (var endpoint in endpoints)
                    {
                        tested++;
                        try
                        {
                            var response = await client.GetAsync($"http://localhost:{port}{endpoint}");
                            if (response.IsSuccessStatusCode)
                            {
                                var serverInfo = $"localhost:{port} ({endpoint}) - HTTP {(int)response.StatusCode}";
                                if (!AvailableServers.Contains(serverInfo))
                                {
                                    AvailableServers.Add(serverInfo);
                                    found++;
                                }
                                break; // Gefunden, nächster Port
                            }
                        }
                        catch (System.Net.Http.HttpRequestException ex)
                        {
                            // HTTP-Fehler (z.B. Connection refused, Reset)
                            if (ex.Message.Contains("actively refused") || ex.Message.Contains("connection was closed"))
                            {
                                // Port ist offen aber Server antwortet nicht richtig
                                continue;
                            }
                        }
                        catch
                        {
                            // Timeout oder anderer Fehler
                        }
                    }
                }
            }

            if (found == 0)
            {
                ScanStatus = $"⚠️ Keine Server gefunden. Getestet: {tested} Endpoints auf {commonPorts.Length} Ports. Stellen Sie sicher dass ThemisDB läuft.";
            }
            else
            {
                ScanStatus = $"✓ {found} Server gefunden";
            }
            IsScanning = false;
        }

        public void ApplySelectedServer()
        {
            if (string.IsNullOrEmpty(SelectedServer)) return;

            // Parse "localhost:8081 (/health)"
            var parts = SelectedServer.Split(' ');
            if (parts.Length > 0)
            {
                var hostPort = parts[0].Split(':');
                if (hostPort.Length == 2)
                {
                    ThemisHost = hostPort[0];
                    if (int.TryParse(hostPort[1], out int port))
                    {
                        ThemisPort = port;
                    }
                }
            }
        }
    }
}
