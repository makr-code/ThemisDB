/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainWindow.xaml.cs                                 ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:57:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     346                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using System.Linq;
using Microsoft.Extensions.DependencyInjection;
using Themis.IngestionTool.ViewModels;
using Themis.IngestionTool.Models;
using System.Windows.Controls;

namespace Themis.IngestionTool.Views
{
    public partial class MainWindow : Window
    {
        private readonly MainWindowViewModel _viewModel;
        private FileDetailsView? _detailsView;
        private GridLength _sidebarExpandedWidth = new GridLength(380);
        private bool _isSidebarCollapsed = false;

        public MainWindow(MainWindowViewModel viewModel)
        {
            InitializeComponent();
            _viewModel = viewModel;
            DataContext = _viewModel;
        }

        private void OnToggleSidebar(object sender, RoutedEventArgs e)
        {
            // Toggle zwischen sichtbarer und versteckter Seitenleiste
            if (!_isSidebarCollapsed)
            {
                _sidebarExpandedWidth = LeftColumn.Width.Value > 0 ? LeftColumn.Width : new GridLength(380);
                LeftColumn.Width = new GridLength(0);
                SidebarToggle.Content = "Menu >";
                _isSidebarCollapsed = true;
            }
            else
            {
                LeftColumn.Width = _sidebarExpandedWidth.Value > 0 ? _sidebarExpandedWidth : new GridLength(380);
                SidebarToggle.Content = "Menu";
                _isSidebarCollapsed = false;
            }
        }

        private void OnResultSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ResultsDataGrid.SelectedItem is FileAnalysisResult selectedResult)
            {
                // Erstelle oder aktualisiere die Details-Ansicht
                if (_detailsView == null)
                {
                    _detailsView = new FileDetailsView();
                }
                
                _detailsView.ShowDetails(selectedResult);
                
                // Zeige die Details-Ansicht
                DetailsContent.Content = _detailsView;
                DetailsContent.Visibility = Visibility.Visible;
                DetailsPlaceholder.Visibility = Visibility.Collapsed;
                
                // Wechsle automatisch zum Details-Tab
                MainTabControl.SelectedItem = DetailsTab;
            }
            else
            {
                // Keine Auswahl - zeige Platzhalter
                DetailsContent.Visibility = Visibility.Collapsed;
                DetailsPlaceholder.Visibility = Visibility.Visible;
            }
        }

        private void OnExit(object sender, RoutedEventArgs e)
        {
            Application.Current.Shutdown();
        }

        private void OnAbout(object sender, RoutedEventArgs e)
        {
            MessageBox.Show(
                "ThemisDB Ingestion Tool v1.0.0\n\n" +
                "Ein modernes Tool zur Datei-Ingestion in ThemisDB\n\n" +
                "© 2026 ThemisDB Project",
                "Über ThemisDB Ingestion Tool",
                MessageBoxButton.OK,
                MessageBoxImage.Information);
        }

        private void OnBrowseSource(object sender, RoutedEventArgs e)
        {
            using (var dialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                dialog.Description = "Wählen Sie einen Ordner zum Ingesten";
                if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    _viewModel.SourceFolder = dialog.SelectedPath;
                    _viewModel.Status = $"Quellordner ausgewählt: {System.IO.Path.GetFileName(dialog.SelectedPath)}";
                }
            }
        }

        private void OnOpenSettings(object sender, RoutedEventArgs e)
        {
            var serviceProvider = (Application.Current as App)?.Services;
            if (serviceProvider != null)
            {
                var settingsDialog = serviceProvider.GetRequiredService<SettingsDialog>();
                if (settingsDialog.ShowDialog() == true)
                {
                    MessageBox.Show("Einstellungen wurden gespeichert.", "Erfolg", MessageBoxButton.OK, MessageBoxImage.Information);
                }
            }
        }

        private async void OnStartIngestion(object sender, RoutedEventArgs e)
        {
            // Debug: Prüfe Bedingungen
            System.Diagnostics.Debug.WriteLine($"[START] SourceFolder: {_viewModel.SourceFolder ?? "NULL"}");
            System.Diagnostics.Debug.WriteLine($"[START] IsConnected: {_viewModel.IsConnected}");
            System.Diagnostics.Debug.WriteLine($"[START] IsRunning: {_viewModel.IsRunning}");

            if (string.IsNullOrEmpty(_viewModel.SourceFolder))
            {
                MessageBox.Show("Bitte wählen Sie zuerst einen Quellordner aus.\n\nKlicken Sie auf '📁 Quelle' um einen Ordner zu wählen.", 
                    "Quellordner fehlt", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (!_viewModel.IsConnected)
            {
                MessageBox.Show("Nicht mit ThemisDB verbunden!\n\nÖffnen Sie die Einstellungen (⚙️) und verbinden Sie sich mit einem ThemisDB Server.", 
                    "Verbindung fehlt", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            _viewModel.IsRunning = true;
            _viewModel.Status = "Pipeline wird gestartet...";
            _viewModel.ProcessedCount = 0;
            _viewModel.FileCount = 0;

            try
            {
                var serviceProvider = (Application.Current as App)?.Services;
                if (serviceProvider == null)
                {
                    MessageBox.Show("Service Provider nicht verfügbar.", "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                var pipelineService = serviceProvider.GetRequiredService<Themis.IngestionTool.Services.IIngestionPipelineService>();

                var progress = new Progress<Themis.IngestionTool.Models.PipelineStage>(stage =>
                {
                    _viewModel.CurrentStage = $"{stage.Name}: {stage.Description}";
                    _viewModel.Status = stage.Description;
                    _viewModel.FileCount = stage.TotalCount;
                    _viewModel.ProcessedCount = stage.ProcessedCount;
                });

                var fileProgress = new Progress<Themis.IngestionTool.Models.FileAnalysisResult>(fileResult =>
                {
                    _viewModel.LiveResults.Add(fileResult);
                });

                var mode = _viewModel.IsDryRun ? "DryRun-Analyse" : "Ingestion";
                _viewModel.Status = $"{mode} gestartet...";
                _viewModel.LiveResults.Clear();

                var result = await pipelineService.ExecutePipelineAsync(
                    _viewModel.SourceFolder,
                    _viewModel.IsDryRun,
                    progress,
                    fileProgress);

                var summary = $"{mode} abgeschlossen!\n\n" +
                             $"Gesamt: {result.TotalFiles} Dateien\n" +
                             $"Verarbeitet: {result.ProcessedFiles}\n" +
                             $"Duplikate: {result.DuplicateFiles}\n" +
                             $"Übersprungen: {result.SkippedFiles}\n" +
                             $"Fehler: {result.ErrorFiles}\n" +
                             $"Zeit: {result.TotalTime:mm\\:ss}\n\n" +
                             $"Top 5 Dateien nach Relevanz:\n";

                var topFiles = result.Results
                    .Where(r => r.IsProcessed && !r.IsDuplicate)
                    .OrderByDescending(r => r.RelevanceScore)
                    .Take(5)
                    .ToList();

                foreach (var file in topFiles)
                {
                    summary += $"• {file.FileName}: {file.RelevanceScore:F2} (Impact: {file.ImpactScore:F2}, Quality: {file.QualityScore:F2})\n";
                }

                _viewModel.Status = $"{mode} abgeschlossen: {result.ProcessedFiles} Dateien";
                MessageBox.Show(summary, "Erfolg", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                _viewModel.Status = $"Fehler: {ex.Message}";
                MessageBox.Show($"Fehler bei der Pipeline:\n\n{ex.Message}\n\nStack:\n{ex.StackTrace}", 
                    "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            finally
            {
                _viewModel.IsRunning = false;
            }
        }

        private void OnCancelIngestion(object sender, RoutedEventArgs e)
        {
            var serviceProvider = (Application.Current as App)?.Services;
            if (serviceProvider != null)
            {
                var pipelineService = serviceProvider.GetRequiredService<Themis.IngestionTool.Services.IIngestionPipelineService>();
                pipelineService.CancelPipeline();
            }
            _viewModel.IsRunning = false;
            _viewModel.Status = "Pipeline abgebrochen";
            MessageBox.Show("Pipeline wurde abgebrochen.", "Abgebrochen", MessageBoxButton.OK, MessageBoxImage.Information);
        }

        private void OnToggleIngestion(object sender, RoutedEventArgs e)
        {
            if (_viewModel.IsRunning)
            {
                OnCancelIngestion(sender, e);
            }
            else
            {
                OnStartIngestion(sender, e);
            }
        }

        private async void OnStartRealIngestion(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_viewModel.SourceFolder))
            {
                MessageBox.Show("Bitte wählen Sie einen Quellordner aus.", "Fehler", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            _viewModel.IsRunning = true;
            _viewModel.Status = "Real Ingestion zu ThemisDB wird gestartet...";
            _viewModel.ProcessedCount = 0;
            _viewModel.FileCount = 0;

            try
            {
                var serviceProvider = (Application.Current as App)?.Services;
                if (serviceProvider == null)
                {
                    MessageBox.Show("Service Provider nicht verfügbar.", "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                // 1. Analyse durchführen
                var pipelineService = serviceProvider.GetRequiredService<Themis.IngestionTool.Services.IIngestionPipelineService>();
                _viewModel.Status = "Schritt 1/2: Datei-Analyse...";

                var progress = new Progress<Themis.IngestionTool.Models.PipelineStage>(stage =>
                {
                    _viewModel.CurrentStage = $"{stage.Name}: {stage.Description}";
                    _viewModel.Status = $"Analyse: {stage.Description}";
                    _viewModel.FileCount = stage.TotalCount;
                    _viewModel.ProcessedCount = stage.ProcessedCount;
                });

                var fileProgress = new Progress<Themis.IngestionTool.Models.FileAnalysisResult>(fileResult =>
                {
                    _viewModel.LiveResults.Add(fileResult);
                });

                var analysisResult = await pipelineService.ExecutePipelineAsync(
                    _viewModel.SourceFolder,
                    isDryRun: false,
                    progress,
                    fileProgress);

                // 2. Real Ingestion zu ThemisDB
                _viewModel.Status = "Schritt 2/2: Echte Ingestion zu ThemisDB...";
                _viewModel.ProcessedCount = 0;

                var realIngestionService = serviceProvider.GetRequiredService<Themis.IngestionTool.Services.IRealIngestionService>();

                var ingestionProgress = new Progress<Themis.IngestionTool.Services.RealIngestionProgress>(ip =>
                {
                    _viewModel.Status = ip.Message;
                    _viewModel.ProcessedCount = ip.ProcessedFiles;
                    _viewModel.FileCount = ip.TotalFiles > 0 ? ip.TotalFiles : analysisResult.TotalFiles;
                    _viewModel.CurrentStage = $"ThemisDB: {ip.StoredEntities} Entities, {ip.StoredVectors} Vectors, {ip.StoredTimeSeries} TimeSeries";
                });

                var ingestionResult = await realIngestionService.IngestFilesAsync(
                    analysisResult.Results,
                    useBatch: true,
                    batchSize: 50,
                    progress: ingestionProgress);

                // 3. Zeige Ergebnis
                var summary = $"Real Ingestion abgeschlossen!\n\n" +
                             $"Analyse: {analysisResult.ProcessedFiles} Dateien\n" +
                             $"Duplikate: {analysisResult.DuplicateFiles}\n\n" +
                             $"ThemisDB Speicherung:\n" +
                             $"✅ Entities:    {ingestionResult.StoredEntities}\n" +
                             $"📊 Vectors:     {ingestionResult.StoredVectors}\n" +
                             $"⏱️  TimeSeries:  {ingestionResult.StoredTimeSeries}\n" +
                             $"❌ Fehler:      {ingestionResult.FailedFiles}\n\n" +
                             $"Zeit: {ingestionResult.Duration.TotalSeconds:F1}s\n" +
                             $"Host: {ingestionResult.Settings.ThemisHost}:{ingestionResult.Settings.ThemisPort}";

                _viewModel.Status = "✅ Real Ingestion erfolgreich abgeschlossen!";
                MessageBox.Show(summary, "Erfolg", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                _viewModel.Status = $"❌ Fehler: {ex.Message}";
                MessageBox.Show($"Fehler bei Real Ingestion:\n\n{ex.Message}", 
                    "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            finally
            {
                _viewModel.IsRunning = false;
            }
        }
    }
}
