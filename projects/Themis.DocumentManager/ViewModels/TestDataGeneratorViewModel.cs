/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TestDataGeneratorViewModel.cs                      ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     563                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Input;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.TestData;
using MediatR;
using Themis.DocumentManager.Domain.Events;

namespace Themis.DocumentManager.ViewModels;

/// <summary>
/// ViewModel für Testdaten-Generator UI
/// Ermöglicht schnelle Generierung von Testdaten im laufenden Betrieb
/// </summary>
public class TestDataGeneratorViewModel : INotifyPropertyChanged
{
    private readonly ThemisTestDataGenerator _generator;
    private readonly ThemisDbSeeder _seeder;
    private readonly OllamaContentGeneratorService _ollamaService;
    private readonly IMediator? _mediator;
    
    private int _documentCount = 10;
    private bool _includeSpecialized = true;
    private int _specializedCount = 5;
    private bool _isGenerating;
    private int _progressCurrent;
    private int _progressTotal;
    private string _statusMessage = "Bereit";
    private TestDataStatistics? _lastStatistics;
    private ExportMode _selectedExportMode = ExportMode.Json;
    private bool _useLlm = true;
    private bool _ollamaAvailable = false;

    public event PropertyChangedEventHandler? PropertyChanged;

    public TestDataGeneratorViewModel() : this(null)
    {
    }

    public TestDataGeneratorViewModel(IMediator? mediator)
    {
        _mediator = mediator;
        _ollamaService = new OllamaContentGeneratorService();
        _generator = new ThemisTestDataGenerator(_ollamaService);
        _seeder = new ThemisDbSeeder();

        GenerateCommand = new AsyncCommand(async () => await GenerateDataAsync(), () => !IsGenerating);
        GenerateQuickCommand = new AsyncCommand(async () => await GenerateQuickAsync(), () => !IsGenerating);
        GenerateBImSchGCommand = new AsyncCommand(async () => await GenerateBImSchGDataAsync(), () => !IsGenerating);
        GenerateWithContentCommand = new AsyncCommand(async () => await GenerateWithAuthenticContentAsync(), () => !IsGenerating);
        ClearCommand = new SyncCommand(() => ClearStatistics());
        
        // Prüfe Ollama-Verfügbarkeit asynchron
        _ = CheckOllamaAvailabilityAsync();
    }
    
    private async Task CheckOllamaAvailabilityAsync()
    {
        OllamaAvailable = await _ollamaService.CheckAvailabilityAsync();
    }

    #region Properties

    public int DocumentCount
    {
        get => _documentCount;
        set { _documentCount = value; OnPropertyChanged(); }
    }

    public bool IncludeSpecialized
    {
        get => _includeSpecialized;
        set { _includeSpecialized = value; OnPropertyChanged(); }
    }

    public int SpecializedCount
    {
        get => _specializedCount;
        set { _specializedCount = value; OnPropertyChanged(); }
    }

    public bool IsGenerating
    {
        get => _isGenerating;
        set
        {
            _isGenerating = value;
            OnPropertyChanged();
            CommandManager.InvalidateRequerySuggested();
        }
    }

    public string StatusMessage
    {
        get => _statusMessage;
        set { _statusMessage = value; OnPropertyChanged(); }
    }

    public TestDataStatistics? LastStatistics
    {
        get => _lastStatistics;
        set { _lastStatistics = value; OnPropertyChanged(); }
    }

    public ExportMode SelectedExportMode
    {
        get => _selectedExportMode;
        set { _selectedExportMode = value; OnPropertyChanged(); }
    }
    
    public int ProgressCurrent
    {
        get => _progressCurrent;
        set { _progressCurrent = value; OnPropertyChanged(); }
    }
    
    public int ProgressTotal
    {
        get => _progressTotal;
        set { _progressTotal = value; OnPropertyChanged(); }
    }
    
    public bool UseLlm
    {
        get => _useLlm;
        set { _useLlm = value; OnPropertyChanged(); }
    }
    
    public bool OllamaAvailable
    {
        get => _ollamaAvailable;
        set { _ollamaAvailable = value; OnPropertyChanged(); }
    }

    public List<ExportModeItem> ExportModes { get; } = new()
    {
        new ExportModeItem { Mode = ExportMode.Json, DisplayName = "JSON-Datei", Icon = "📄" },
        new ExportModeItem { Mode = ExportMode.Csv, DisplayName = "CSV-Datei (Excel)", Icon = "📊" },
        new ExportModeItem { Mode = ExportMode.Sql, DisplayName = "SQL-Statements", Icon = "🗄️" },
        new ExportModeItem { Mode = ExportMode.ThemisDbApi, DisplayName = "Direkt zu ThemisDB", Icon = "🌐" }
    };

    #endregion

    #region Commands

    public ICommand GenerateCommand { get; }
    public ICommand GenerateQuickCommand { get; }
    public ICommand GenerateBImSchGCommand { get; }
    public ICommand GenerateWithContentCommand { get; }
    public ICommand ClearCommand { get; }

    #endregion

    #region Methods

    /// <summary>
    /// Generiert Testdaten mit aktuellen Einstellungen
    /// </summary>
    private async Task GenerateDataAsync()
    {
        try
        {
            IsGenerating = true;
            ProgressTotal = DocumentCount;
            ProgressCurrent = 0;
            StatusMessage = $"Generiere {DocumentCount} Datensätze mit Fortschritt...";

            // SeedOptions erstellen
            var options = new SeedOptions
            {
                DocumentCount = DocumentCount,
                IncludeSpecializedData = IncludeSpecialized,
                SpecializedCount = IncludeSpecialized ? SpecializedCount : 0,
                ExportMode = SelectedExportMode,
                OutputFileName = $"ThemisTestData_{DateTime.Now:yyyyMMdd_HHmmss}.json",
                ThemisDbUrl = "http://localhost:8765"
            };

            // Seeder verwenden
            var result = await _seeder.SeedDatabaseAsync(options);

            if (!result.Success)
            {
                throw new Exception(result.ErrorMessage ?? "Unbekannter Fehler beim Seeding");
            }

            LastStatistics = result.Statistics;
            StatusMessage = $"✅ {result.GeneratedDocuments} Dokumente generiert (Befüllungsrate: {result.Statistics?.AverageFillRate:F1}%)";
            
            // Publish events
            if (_mediator != null)
            {
                await _mediator.Publish(new TestDataGeneratedEvent(result.GeneratedDocuments, DateTime.UtcNow));
                
                // Bei ThemisDB API-Export: Refresh-Event senden
                if (SelectedExportMode == ExportMode.ThemisDbApi)
                {
                    await _mediator.Publish(new DocumentsRefreshRequestedEvent("TestDataGenerator", DateTime.UtcNow));
                }
            }
            
            var successMessage = SelectedExportMode == ExportMode.ThemisDbApi
                ? $"Testdaten erfolgreich zu ThemisDB hochgeladen!\n\n" +
                  $"Dokumente: {result.GeneratedDocuments}\n" +
                  $"Felder gesamt: {result.Statistics?.TotalFields}\n" +
                  $"Ausgefüllte Felder: {result.Statistics?.FilledFields}\n" +
                  $"Befüllungsrate: {result.Statistics?.AverageFillRate:F1}%\n\n" +
                  $"Die Dokumentenliste wird aktualisiert..."
                : $"Testdaten erfolgreich exportiert!\n\n" +
                  $"Dokumente: {result.GeneratedDocuments}\n" +
                  $"Felder gesamt: {result.Statistics?.TotalFields}\n" +
                  $"Ausgefüllte Felder: {result.Statistics?.FilledFields}\n" +
                  $"Befüllungsrate: {result.Statistics?.AverageFillRate:F1}%\n\n" +
                  $"Datei: {result.ExportPath}";

            MessageBox.Show(
                successMessage,
                "Testdaten-Generator",
                MessageBoxButton.OK,
                MessageBoxImage.Information
            );
        }
        catch (InvalidOperationException ex) when (ex.Message.Contains("ThemisDB"))
        {
            StatusMessage = "❌ ThemisDB nicht erreichbar";
            MessageBox.Show(
                ex.Message,
                "ThemisDB Verbindungsfehler",
                MessageBoxButton.OK,
                MessageBoxImage.Warning);
        }
        catch (Exception ex)
        {
            StatusMessage = $"❌ Fehler: {ex.Message}";
            var stackTrace = ex.StackTrace ?? "";
            var shortStack = stackTrace.Length > 200 ? stackTrace.Substring(0, 200) + "..." : stackTrace;
            MessageBox.Show(
                $"Unerwarteter Fehler:\n{ex.Message}\n\n" +
                $"StackTrace:\n{shortStack}",
                "Fehler",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
        finally
        {
            IsGenerating = false;
            ProgressCurrent = 0;
            ProgressTotal = 0;
        }
    }

    /// <summary>
    /// Schnell-Generierung: 10 Datensätze als JSON
    /// </summary>
    private async Task GenerateQuickAsync()
    {
        try
        {
            IsGenerating = true;
            ProgressTotal = 10;
            ProgressCurrent = 0;
            StatusMessage = "🚀 Quick-Generierung: 10 Datensätze...";

            var batch = new List<DocumentMetadataBinding>();
            for (int i = 0; i < 10; i++)
            {
                batch.Add(_generator.GenerateMetadata(i));
                ProgressCurrent = i + 1;
                StatusMessage = $"Quick-Seed: {i + 1}/10 Dokumente...";
                await Task.Delay(10);
            }

            var stats = _generator.GetStatistics(batch);
            LastStatistics = stats;
            StatusMessage = "✅ 10 Datensätze generiert";
            
            // Publish event to notify other ViewModels
            if (_mediator != null)
            {
                await _mediator.Publish(new TestDataGeneratedEvent(10, DateTime.UtcNow));
            }
            
            MessageBox.Show(
                $"Quick-Seed erfolgreich!\n\n" +
                $"10 Testdatensätze generiert\n" +
                $"Befüllungsrate: {stats.AverageFillRate:F1}%",
                "Testdaten-Generator",
                MessageBoxButton.OK,
                MessageBoxImage.Information
            );
        }
        catch (Exception ex)
        {
            StatusMessage = $"❌ Fehler: {ex.Message}";
            MessageBox.Show($"Fehler: {ex.Message}", "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
        }
        finally
        {
            IsGenerating = false;
            ProgressCurrent = 0;
            ProgressTotal = 0;
        }
    }

    /// <summary>
    /// Generiert spezialisierte BImSchG-Datensätze
    /// </summary>
    private async Task GenerateBImSchGDataAsync()
    {
        try
        {
            IsGenerating = true;
            ProgressTotal = 30; // 3 Typen × 10 Datensätze
            ProgressCurrent = 0;
            StatusMessage = "⚗️ Generiere 30 BImSchG-Datensätze...";

            var batch = new List<DocumentMetadataBinding>();
            var bimschgTypes = new List<string> { "BImSchG-Genehmigung", "BImSchG-Anzeige", "BImSchG-Überwachung" };
            int count = 0;

            foreach (var type in bimschgTypes)
            {
                for (int i = 0; i < 10; i++)
                {
                    batch.Add(_generator.GenerateSpecializedMetadata(type));
                    count++;
                    ProgressCurrent = count;
                    StatusMessage = $"BImSchG: {count}/30 ({type})...";
                    await Task.Delay(10);
                }
            }

            var stats = _generator.GetStatistics(batch);
            LastStatistics = stats;
            StatusMessage = $"✅ {batch.Count} BImSchG-Datensätze generiert";
            
            // Publish event to notify other ViewModels
            if (_mediator != null)
            {
                await _mediator.Publish(new TestDataGeneratedEvent(batch.Count, DateTime.UtcNow));
            }
            
            MessageBox.Show(
                $"BImSchG-Testdaten generiert!\n\n" +
                $"Genehmigungen: 10\n" +
                $"Anzeigen: 10\n" +
                $"Überwachungen: 10\n" +
                $"Gesamt: {batch.Count} Datensätze\n" +
                $"Befüllungsrate: {stats.AverageFillRate:F1}%",
                "BImSchG-Testdaten",
                MessageBoxButton.OK,
                MessageBoxImage.Information
            );
        }
        catch (Exception ex)
        {
            StatusMessage = $"❌ Fehler: {ex.Message}";
            MessageBox.Show($"Fehler: {ex.Message}", "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
        }
        finally
        {
            IsGenerating = false;
            ProgressCurrent = 0;
            ProgressTotal = 0;
        }
    }

    /// <summary>
    /// Generiert Testdaten mit authentischen Inhalten (Briefe, Notizen, Tabellen, Formulare)
    /// </summary>
    private async Task GenerateWithAuthenticContentAsync()
    {
        try
        {
            IsGenerating = true;
            ProgressTotal = DocumentCount;
            ProgressCurrent = 0;
            StatusMessage = "Generiere Dokumente mit authentischen Inhalten...";

            var batch = await _generator.GenerateWithAuthenticContentAsync(
                DocumentCount,
                UseLlm && OllamaAvailable,
                CancellationToken.None,
                (current, status) =>
                {
                    ProgressCurrent = current;
                    StatusMessage = status;
                }
            );

            var stats = _generator.GetStatistics(batch);
            LastStatistics = stats;

            StatusMessage = $"✅ {batch.Count} Dokumente mit Inhalten generiert";
            
            // Publish event to notify other ViewModels
            if (_mediator != null)
            {
                await _mediator.Publish(new TestDataGeneratedEvent(batch.Count, DateTime.UtcNow));
            }
            
            var ollamaStatus = (UseLlm && OllamaAvailable) 
                ? "✓ Mit Ollama LLM" 
                : "⚠ Fallback-Daten (Ollama nicht verfügbar)";
            
            var infoText = $@"Authentische Dokumente generiert!

Dokumente: {batch.Count}
Felder gesamt: {stats.TotalFields}
Ausgefüllte Felder: {stats.FilledFields}
Befüllungsrate: {stats.AverageFillRate:F1}%

{ollamaStatus}";

            MessageBox.Show(infoText, "Authentische Testdaten", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception ex)
        {
            StatusMessage = $"❌ Fehler: {ex.Message}";
            MessageBox.Show($"Fehler: {ex.Message}", "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
        }
        finally
        {
            IsGenerating = false;
            ProgressCurrent = 0;
            ProgressTotal = 0;
        }
    }

    private void ClearStatistics()
    {
        LastStatistics = null;
        StatusMessage = "Bereit";
    }

    private string GenerateFileName()
    {
        var ext = SelectedExportMode switch
        {
            ExportMode.Json => "json",
            ExportMode.Csv => "csv",
            ExportMode.Sql => "sql",
            _ => "json"
        };
        return $"themis_testdata_{DateTime.Now:yyyyMMdd_HHmmss}.{ext}";
    }

    #endregion

    protected void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

/// <summary>
/// Export-Modus Display-Item
/// </summary>
public class ExportModeItem
{
    public ExportMode Mode { get; set; }
    public string DisplayName { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
}

#region Command Helpers

internal class AsyncCommand : ICommand
{
    private readonly Func<Task> _execute;
    private readonly Func<bool>? _canExecute;

    public AsyncCommand(Func<Task> execute, Func<bool>? canExecute = null)
    {
        _execute = execute ?? throw new ArgumentNullException(nameof(execute));
        _canExecute = canExecute;
    }

    public event EventHandler? CanExecuteChanged
    {
        add => CommandManager.RequerySuggested += value;
        remove => CommandManager.RequerySuggested -= value;
    }

    public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;

    public async void Execute(object? parameter) => await _execute();
}

internal class SyncCommand : ICommand
{
    private readonly Action _execute;
    private readonly Func<bool>? _canExecute;

    public SyncCommand(Action execute, Func<bool>? canExecute = null)
    {
        _execute = execute ?? throw new ArgumentNullException(nameof(execute));
        _canExecute = canExecute;
    }

    public event EventHandler? CanExecuteChanged
    {
        add => CommandManager.RequerySuggested += value;
        remove => CommandManager.RequerySuggested -= value;
    }

    public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;

    public void Execute(object? parameter) => _execute();
}

#endregion

/// <summary>
/// Null to Boolean Converter
/// </summary>
public class NullToBooleanConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        return value == null;
    }

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}
