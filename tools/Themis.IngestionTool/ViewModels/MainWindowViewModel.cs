/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainWindowViewModel.cs                             ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:24:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     302                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Windows.Input;
using System.Windows.Media;
using Themis.IngestionTool.Models;
using Themis.IngestionTool.Services;

namespace Themis.IngestionTool.ViewModels
{
    public class MainWindowViewModel : BaseViewModel
    {
        private readonly IIngestionService _ingestionService;
        private readonly IThemisConnectionService _connectionService;
        private readonly ISettingsService _settingsService;
        private readonly ILoggerService _loggerService;
        private readonly ILlmStatusService _llmStatusService;

        private string _sourceFolder = string.Empty;
        private string _outputFile = "ingestion_output.json";
        private string _status = "Bereit";
        private bool _isRunning = false;
        private bool _isConnected = false;
        private bool _isDryRun = false;
        private int _fileCount = 0;
        private string _currentStage = "";
        private ObservableCollection<FileAnalysisResult> _liveResults = new();
        private int _processedCount = 0;
        
        // LLM Status Properties
        private bool _showLlmStatusInStatusBar = true;
        private string _llmStatusText = "Offline";
        private SolidColorBrush _llmStatusColor = new SolidColorBrush(Color.FromRgb(220, 53, 69)); // Rot für Offline

        public string SourceFolder
        {
            get => _sourceFolder;
            set => SetProperty(ref _sourceFolder, value);
        }

        public string OutputFile
        {
            get => _outputFile;
            set => SetProperty(ref _outputFile, value);
        }

        public string Status
        {
            get => _status;
            set => SetProperty(ref _status, value);
        }

        public bool IsRunning
        {
            get => _isRunning;
            set => SetProperty(ref _isRunning, value);
        }

        public bool IsConnected
        {
            get => _isConnected;
            set => SetProperty(ref _isConnected, value);
        }

        public int FileCount
        {
            get => _fileCount;
            set => SetProperty(ref _fileCount, value);
        }

        public int ProcessedCount
        {
            get => _processedCount;
            set => SetProperty(ref _processedCount, value);
        }

        public bool IsDryRun
        {
            get => _isDryRun;
            set => SetProperty(ref _isDryRun, value);
        }

        public string CurrentStage
        {
            get => _currentStage;
            set => SetProperty(ref _currentStage, value);
        }

        public ObservableCollection<FileAnalysisResult> LiveResults
        {
            get => _liveResults;
            set => SetProperty(ref _liveResults, value);
        }

        // LLM Status Properties
        public bool ShowLlmStatusInStatusBar
        {
            get => _showLlmStatusInStatusBar;
            set => SetProperty(ref _showLlmStatusInStatusBar, value);
        }

        public string LlmStatusText
        {
            get => _llmStatusText;
            set => SetProperty(ref _llmStatusText, value);
        }

        public SolidColorBrush LlmStatusColor
        {
            get => _llmStatusColor;
            set => SetProperty(ref _llmStatusColor, value);
        }

        public ICommand BrowseSourceCommand { get; }
        public ICommand StartIngestionCommand { get; }
        public ICommand CancelIngestionCommand { get; }
        public ICommand OpenSettingsCommand { get; }

        public MainWindowViewModel(
            IIngestionService ingestionService,
            IThemisConnectionService connectionService,
            ISettingsService settingsService,
            ILoggerService loggerService,
            ILlmStatusService llmStatusService)
        {
            _ingestionService = ingestionService;
            _connectionService = connectionService;
            _settingsService = settingsService;
            _loggerService = loggerService;
            _llmStatusService = llmStatusService;

            BrowseSourceCommand = new RelayCommand(BrowseSource);
            StartIngestionCommand = new RelayCommand(StartIngestion, () => !IsRunning && !string.IsNullOrEmpty(SourceFolder) && IsConnected);
            CancelIngestionCommand = new RelayCommand(CancelIngestion, () => IsRunning);
            OpenSettingsCommand = new RelayCommand(OpenSettings);

            LoadSettings();
            InitializeAsync();
        }

        private void LoadSettings()
        {
            try
            {
                var settings = _settingsService.LoadSettings();
                SourceFolder = settings.LastSourceFolder ?? string.Empty;
                OutputFile = settings.LastOutputFile ?? "ingestion_output.json";
                ShowLlmStatusInStatusBar = settings.ShowLlmStatusInStatusBar && settings.EnableLlmStatusMonitoring;
                
                // Lade Verbindungseinstellungen
                if (!string.IsNullOrEmpty(settings.ThemisHost) && settings.ThemisPort > 0)
                {
                    _connectionService.UpdateConnectionSettings(settings.ThemisHost, settings.ThemisPort);
                    System.Diagnostics.Debug.WriteLine($"[SETTINGS] Loaded ThemisDB: {settings.ThemisHost}:{settings.ThemisPort}");
                }
                else
                {
                    System.Diagnostics.Debug.WriteLine("[SETTINGS] Using default ThemisDB settings");
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler beim Laden der Einstellungen: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"[SETTINGS ERROR] {ex}");
            }
        }

        private void SaveCurrentSettings()
        {
            var settings = _settingsService.LoadSettings();
            settings.LastSourceFolder = SourceFolder;
            settings.LastOutputFile = OutputFile;
            _settingsService.SaveSettings(settings);
        }

        private async void InitializeAsync()
        {
            try
            {
                Status = "Verbindung wird überprüft...";
                IsConnected = await _connectionService.CheckConnectionAsync();
                Status = IsConnected ? "Verbunden" : "Verbindung fehlgeschlagen";
                
                // Event-Handler für automatische Updates registrieren
                _connectionService.ConnectionStatusChanged += (sender, args) =>
                {
                    IsConnected = args.IsConnected;
                    Status = args.IsConnected 
                        ? "Themis-Verbindung hergestellt" 
                        : "Themis-Verbindung getrennt";
                };

                // Registriere LLM Status Event-Handler
                if (ShowLlmStatusInStatusBar)
                {
                    _llmStatusService.StatusChanged += (sender, args) =>
                    {
                        UpdateLlmStatus(args.Status);
                    };

                    // Initiale LLM-Status-Prüfung
                    var initialStatus = await _llmStatusService.GetLlmStatusAsync();
                    UpdateLlmStatus(initialStatus);
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler bei Initialisierung: {ex.Message}");
                Status = "Fehler bei Verbindungsprüfung";
            }
        }

        private void UpdateLlmStatus(LlmStatus status)
        {
            LlmStatusText = status.IsAvailable && status.IsModelLoaded 
                ? $"Aktiv: {status.LoadedModel}"
                : "Offline";

            if (status.IsAvailable && status.IsModelLoaded)
            {
                // Grün für aktiv
                LlmStatusColor = new SolidColorBrush(Color.FromRgb(40, 167, 69));
            }
            else if (status.IsAvailable)
            {
                // Orange für verfügbar aber Modell nicht geladen
                LlmStatusColor = new SolidColorBrush(Color.FromRgb(255, 193, 7));
            }
            else
            {
                // Rot für Offline
                LlmStatusColor = new SolidColorBrush(Color.FromRgb(220, 53, 69));
            }
        }

        private void BrowseSource()
        {
            // Dialog wird von MainWindow.xaml.cs gehandhabt
        }

        private async void StartIngestion()
        {
            if (string.IsNullOrEmpty(SourceFolder) || !IsConnected)
                return;

            IsRunning = true;
            Status = "Ingestion läuft...";
            ProcessedCount = 0;

            try
            {
                SaveCurrentSettings();
                var result = await _ingestionService.StartIngestionAsync(SourceFolder, OutputFile);
                ProcessedCount = result.ProcessedFiles;
                Status = $"Ingestion abgeschlossen: {ProcessedCount} Dateien verarbeitet";
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Fehler bei Ingestion: {ex.Message}");
                Status = $"Fehler: {ex.Message}";
            }
            finally
            {
                IsRunning = false;
            }
        }

        private void CancelIngestion()
        {
            _ingestionService.CancelIngestion();
            IsRunning = false;
            Status = "Ingestion abgebrochen";
        }

        private void OpenSettings()
        {
            // Wird von MainWindow gehandhabt
        }
    }
}
