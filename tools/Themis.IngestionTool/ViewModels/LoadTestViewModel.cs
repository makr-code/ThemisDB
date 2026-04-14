/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LoadTestViewModel.cs                               ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:23:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     324                                            ║
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
using System.Linq;
using System.Windows.Input;
using Themis.IngestionTool.Services;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.ViewModels
{
    /// <summary>
    /// ViewModel für Load Testing Dashboard
    /// </summary>
    public class LoadTestViewModel : ViewModelBase
    {
        private readonly ILoadTestRunner _loadTestRunner;
        private readonly ILoggerService _loggerService;

        // Test Selection
        private LoadTestScenario _selectedScenario = LoadTestScenario.StandardLoad_500Files;
        private string _customTestFolder = string.Empty;
        private int _customFileCount = 100;
        private int _customParallelism = 4;

        // Progress
        private bool _isTestRunning = false;
        private string _currentStatus = "Bereit";
        private int _progressPercent = 0;
        private string _progressDetails = string.Empty;

        // Results
        private LoadTestMetrics? _lastTestMetrics = null;
        private bool _showDetailedResults = false;

        // Commands
        public ICommand RunStandardTestCommand { get; }
        public ICommand RunHighLoadTestCommand { get; }
        public ICommand RunCacheTestCommand { get; }
        public ICommand RunMetadataTestCommand { get; }
        public ICommand RunCustomTestCommand { get; }
        public ICommand SaveReportCommand { get; }
        public ICommand ClearResultsCommand { get; }

        // Results Display
        public ObservableCollection<FileProcessingInfo> DetailedResults { get; } = new();

        public LoadTestViewModel(
            ILoadTestRunner loadTestRunner,
            ILoggerService loggerService)
        {
            _loadTestRunner = loadTestRunner;
            _loggerService = loggerService;

            RunStandardTestCommand = new RelayCommand(async () => await RunTest(LoadTestScenario.StandardLoad_500Files));
            RunHighLoadTestCommand = new RelayCommand(async () => await RunTest(LoadTestScenario.HighLoad_1000Files));
            RunCacheTestCommand = new RelayCommand(async () => await RunTest(LoadTestScenario.CacheEfficiency_100x2));
            RunMetadataTestCommand = new RelayCommand(async () => await RunTest(LoadTestScenario.MetadataTest_250Files));
            RunCustomTestCommand = new RelayCommand(async () => await RunCustomTest());
            SaveReportCommand = new RelayCommand(() => ExecuteSaveReport(), () => LastTestMetrics != null);
            ClearResultsCommand = new RelayCommand(() => ExecuteClearResults());
        }

        #region Properties

        public LoadTestScenario SelectedScenario
        {
            get => _selectedScenario;
            set { SetProperty(ref _selectedScenario, value); }
        }

        public string CustomTestFolder
        {
            get => _customTestFolder;
            set { SetProperty(ref _customTestFolder, value); }
        }

        public int CustomFileCount
        {
            get => _customFileCount;
            set { SetProperty(ref _customFileCount, value); }
        }

        public int CustomParallelism
        {
            get => _customParallelism;
            set { SetProperty(ref _customParallelism, value); }
        }

        public bool IsTestRunning
        {
            get => _isTestRunning;
            set
            {
                SetProperty(ref _isTestRunning, value);
                OnPropertyChanged(nameof(CanRunTests));
            }
        }

        public bool CanRunTests => !IsTestRunning;

        public string CurrentStatus
        {
            get => _currentStatus;
            set { SetProperty(ref _currentStatus, value); }
        }

        public int ProgressPercent
        {
            get => _progressPercent;
            set { SetProperty(ref _progressPercent, value); }
        }

        public string ProgressDetails
        {
            get => _progressDetails;
            set { SetProperty(ref _progressDetails, value); }
        }

        public LoadTestMetrics? LastTestMetrics
        {
            get => _lastTestMetrics;
            set { SetProperty(ref _lastTestMetrics, value); }
        }

        public bool ShowDetailedResults
        {
            get => _showDetailedResults;
            set { SetProperty(ref _showDetailedResults, value); }
        }

        #endregion

        #region Methods

        private async Task RunTest(LoadTestScenario scenario)
        {
            if (IsTestRunning) return;

            IsTestRunning = true;
            CurrentStatus = $"🟡 {scenario}...";
            ProgressPercent = 0;
            ProgressDetails = string.Empty;
            DetailedResults.Clear();

            try
            {
                var progress = new Progress<string>(update =>
                {
                    ProgressDetails += update + "\n";
                    _loggerService.LogInfo(update);
                });

                LastTestMetrics = await _loadTestRunner.RunLoadTestAsync(scenario, progress);

                // Load detailed results
                var results = _loadTestRunner.GetLastTestResults();
                foreach (var result in results.OrderByDescending(r => r.ElapsedMilliseconds).Take(100))
                {
                    DetailedResults.Add(result);
                }

                CurrentStatus = $"✅ Test abgeschlossen: {scenario}";
                ProgressPercent = 100;
                ShowDetailedResults = true;

                _loggerService.LogInfo($"Load test completed: {LastTestMetrics.FilesProcessed} files in {LastTestMetrics.TotalDuration.TotalSeconds:F2}s, Keywords extracted from {_loadTestRunner.GetLastTestResults().Count(r => r.ExtractedKeywords.Count > 0)} documents");
            }
            catch (Exception ex)
            {
                CurrentStatus = $"❌ Fehler: {ex.Message}";
                ProgressDetails += $"\n❌ {ex.Message}";
                _loggerService.LogError($"Load test failed: {ex.Message}");
            }
            finally
            {
                IsTestRunning = false;
            }
        }

        private async Task RunCustomTest()
        {
            if (IsTestRunning || string.IsNullOrWhiteSpace(CustomTestFolder))
            {
                CurrentStatus = "❌ Folder erforderlich";
                return;
            }

            if (!Directory.Exists(CustomTestFolder))
            {
                CurrentStatus = $"❌ Folder nicht gefunden: {CustomTestFolder}";
                return;
            }

            IsTestRunning = true;
            CurrentStatus = "🟡 Custom Test läuft...";
            ProgressPercent = 0;
            ProgressDetails = string.Empty;
            DetailedResults.Clear();

            try
            {
                var progress = new Progress<string>(update =>
                {
                    ProgressDetails += update + "\n";
                    _loggerService.LogInfo(update);
                });

                LastTestMetrics = await _loadTestRunner.RunCustomLoadTestAsync(
                    CustomTestFolder,
                    CustomFileCount,
                    CustomParallelism,
                    progress);

                // Load detailed results
                var results = _loadTestRunner.GetLastTestResults();
                foreach (var result in results.OrderByDescending(r => r.ElapsedMilliseconds).Take(100))
                {
                    DetailedResults.Add(result);
                }

                CurrentStatus = $"✅ Custom Test abgeschlossen";
                ProgressPercent = 100;
                ShowDetailedResults = true;

                _loggerService.LogInfo($"Custom load test completed: {LastTestMetrics.FilesProcessed} files");
            }
            catch (Exception ex)
            {
                CurrentStatus = $"❌ Fehler: {ex.Message}";
                ProgressDetails += $"\n❌ {ex.Message}";
                _loggerService.LogError($"Custom load test failed: {ex.Message}");
            }
            finally
            {
                IsTestRunning = false;
            }
        }

        private void ExecuteSaveReport()
        {
            if (LastTestMetrics == null)
            {
                CurrentStatus = "⚠️ Keine Testergebnisse zum Speichern";
                return;
            }

            try
            {
                var reportPath = Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.Desktop),
                    $"LoadTest_{LastTestMetrics.Scenario}_{DateTime.Now:yyyyMMdd_HHmmss}.txt");

                _loadTestRunner.SaveTestReport(LastTestMetrics, reportPath);
                CurrentStatus = $"✅ Report gespeichert: {reportPath}";
                _loggerService.LogInfo($"Test report saved: {reportPath}");
            }
            catch (Exception ex)
            {
                CurrentStatus = $"❌ Fehler beim Speichern: {ex.Message}";
                _loggerService.LogError($"Failed to save report: {ex.Message}");
            }
        }

        private void ExecuteClearResults()
        {
            DetailedResults.Clear();
            LastTestMetrics = null;
            ProgressPercent = 0;
            ProgressDetails = string.Empty;
            CurrentStatus = "Bereit";
            ShowDetailedResults = false;
        }

        #endregion

        #region Helper Methods

        public string GetScenarioDescription(LoadTestScenario scenario) => scenario switch
        {
            LoadTestScenario.StandardLoad_500Files =>
                "Standard Load Test\n500 Dateien, 4 parallel, Cache enabled\nErwartet: 1.6-2.0 Dateien/Sekunde",
            
            LoadTestScenario.HighLoad_1000Files =>
                "High Load Test\n1000 Dateien (20% Duplikate), 8 parallel\nErwartet: 1.2-1.6 Dateien/Sekunde, 15-25% Cache Hit Rate",
            
            LoadTestScenario.CacheEfficiency_100x2 =>
                "Cache Efficiency Test\n100 Dateien 2x hintereinander\nErwartet: 2. Lauf 8-10x schneller",
            
            LoadTestScenario.MetadataTest_250Files =>
                "Metadata Test\n250 Dateien mit Vector+Graph Metadata\nErwartet: +20% längere Verarbeitung",
            
            LoadTestScenario.ResilienceTest_Fallback =>
                "Resilience Test\nOllama wird mid-run deaktiviert\nTest: Fallback zu Hash-Embeddings",
            
            _ => "Unknown Scenario"
        };

        public string FormatMetrics() => LastTestMetrics?.ToString() ?? "Keine Ergebnisse";

        #endregion
    }
}
