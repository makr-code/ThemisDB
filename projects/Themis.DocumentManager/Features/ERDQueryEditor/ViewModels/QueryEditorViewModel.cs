/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            QueryEditorViewModel.cs                            ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     449                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows.Input;
using CommunityToolkit.Mvvm.Input;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Features.ERDQueryEditor.Services;

namespace Themis.DocumentManager.Features.ERDQueryEditor.ViewModels;

/// <summary>
/// ViewModel for the Query Editor
/// </summary>
public class QueryEditorViewModel : INotifyPropertyChanged
{
    private readonly IQueryService _queryService;
    private readonly ISchemaService _schemaService;
    
    private string _queryText = string.Empty;
    private QueryLanguage _selectedLanguage = QueryLanguage.AQL;
    private QueryResult? _lastResult;
    private SavedQuery? _selectedSavedQuery;
    private bool _isExecuting;
    private bool _isLoading;
    private string _statusMessage = "Bereit";
    private string _validationMessage = string.Empty;

    public QueryEditorViewModel(IQueryService queryService, ISchemaService schemaService)
    {
        _queryService = queryService ?? throw new ArgumentNullException(nameof(queryService));
        _schemaService = schemaService ?? throw new ArgumentNullException(nameof(schemaService));

        // Initialize collections
        SavedQueries = new ObservableCollection<SavedQuery>();
        
        // Initialize commands
        ExecuteQueryCommand = new AsyncRelayCommand(ExecuteQueryAsync, CanExecuteQuery);
        ValidateQueryCommand = new AsyncRelayCommand(ValidateQueryAsync);
        SaveCurrentQueryCommand = new AsyncRelayCommand(SaveCurrentQueryAsync, () => !string.IsNullOrWhiteSpace(QueryText));
        LoadQueryCommand = new RelayCommand<SavedQuery>(LoadQuery);
        DeleteQueryCommand = new AsyncRelayCommand<SavedQuery>(DeleteQueryAsync);
        NewQueryCommand = new RelayCommand(NewQuery);
        FormatQueryCommand = new RelayCommand(FormatQuery);
        ClearResultsCommand = new RelayCommand(ClearResults);

        // Load saved queries - use Task.Run to avoid fire-and-forget warning
        Task.Run(async () => 
        {
            try 
            {
                await LoadSavedQueriesAsync();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error during initial query load: {ex.Message}");
            }
        });
        
        // Set default query
        QueryText = "FOR doc IN documents\n  LIMIT 10\n  RETURN doc";
    }

    #region Properties

    public string QueryText
    {
        get => _queryText;
        set
        {
            if (_queryText != value)
            {
                _queryText = value;
                OnPropertyChanged();
                ExecuteQueryCommand.NotifyCanExecuteChanged();
                SaveCurrentQueryCommand.NotifyCanExecuteChanged();
                // Trigger validation asynchronously
                Task.Run(async () => 
                {
                    try 
                    {
                        await ValidateQueryAsync();
                    }
                    catch (Exception ex)
                    {
                        System.Diagnostics.Debug.WriteLine($"Validation error: {ex.Message}");
                    }
                });
            }
        }
    }

    public QueryLanguage SelectedLanguage
    {
        get => _selectedLanguage;
        set
        {
            if (_selectedLanguage != value)
            {
                _selectedLanguage = value;
                OnPropertyChanged();
                // Trigger validation asynchronously
                Task.Run(async () => 
                {
                    try 
                    {
                        await ValidateQueryAsync();
                    }
                    catch (Exception ex)
                    {
                        System.Diagnostics.Debug.WriteLine($"Validation error: {ex.Message}");
                    }
                });
            }
        }
    }

    public QueryResult? LastResult
    {
        get => _lastResult;
        set
        {
            if (_lastResult != value)
            {
                _lastResult = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(HasResults));
                OnPropertyChanged(nameof(ResultCount));
            }
        }
    }

    public bool HasResults => LastResult?.Results?.Count > 0;
    public int ResultCount => LastResult?.Results?.Count ?? 0;

    public SavedQuery? SelectedSavedQuery
    {
        get => _selectedSavedQuery;
        set
        {
            if (_selectedSavedQuery != value)
            {
                _selectedSavedQuery = value;
                OnPropertyChanged();
            }
        }
    }

    public ObservableCollection<SavedQuery> SavedQueries { get; }

    public bool IsExecuting
    {
        get => _isExecuting;
        set
        {
            if (_isExecuting != value)
            {
                _isExecuting = value;
                OnPropertyChanged();
                ExecuteQueryCommand.NotifyCanExecuteChanged();
            }
        }
    }

    public bool IsLoading
    {
        get => _isLoading;
        set
        {
            if (_isLoading != value)
            {
                _isLoading = value;
                OnPropertyChanged();
            }
        }
    }

    public string StatusMessage
    {
        get => _statusMessage;
        set
        {
            if (_statusMessage != value)
            {
                _statusMessage = value;
                OnPropertyChanged();
            }
        }
    }

    public string ValidationMessage
    {
        get => _validationMessage;
        set
        {
            if (_validationMessage != value)
            {
                _validationMessage = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(IsQueryValid));
            }
        }
    }

    public bool IsQueryValid => string.IsNullOrEmpty(ValidationMessage);

    #endregion

    #region Commands

    public IAsyncRelayCommand ExecuteQueryCommand { get; }
    public IAsyncRelayCommand ValidateQueryCommand { get; }
    public IAsyncRelayCommand SaveCurrentQueryCommand { get; }
    public IRelayCommand<SavedQuery> LoadQueryCommand { get; }
    public IAsyncRelayCommand<SavedQuery> DeleteQueryCommand { get; }
    public IRelayCommand NewQueryCommand { get; }
    public IRelayCommand FormatQueryCommand { get; }
    public IRelayCommand ClearResultsCommand { get; }

    #endregion

    #region Methods

    private bool CanExecuteQuery() => !string.IsNullOrWhiteSpace(QueryText) && !IsExecuting;

    private async Task ExecuteQueryAsync()
    {
        IsExecuting = true;
        StatusMessage = "Führe Abfrage aus...";
        
        try
        {
            LastResult = await _queryService.ExecuteQueryAsync(QueryText, SelectedLanguage);
            
            if (LastResult.Success)
            {
                StatusMessage = $"Abfrage erfolgreich: {LastResult.RowCount} Zeilen in {LastResult.ExecutionTime.TotalMilliseconds:F2}ms";
            }
            else
            {
                StatusMessage = $"Fehler: {LastResult.ErrorMessage}";
            }
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler beim Ausführen: {ex.Message}";
            LastResult = new QueryResult
            {
                Success = false,
                ErrorMessage = ex.Message,
                Query = QueryText
            };
        }
        finally
        {
            IsExecuting = false;
        }
    }

    private async Task ValidateQueryAsync()
    {
        if (string.IsNullOrWhiteSpace(QueryText))
        {
            ValidationMessage = string.Empty;
            return;
        }

        try
        {
            ValidationMessage = await _queryService.ValidateQueryAsync(QueryText, SelectedLanguage);
            
            if (string.IsNullOrEmpty(ValidationMessage))
            {
                StatusMessage = "Abfrage ist gültig";
            }
        }
        catch (Exception ex)
        {
            ValidationMessage = $"Validierungsfehler: {ex.Message}";
        }
    }

    private async Task LoadSavedQueriesAsync()
    {
        IsLoading = true;
        
        try
        {
            var queries = await _queryService.GetSavedQueriesAsync();
            SavedQueries.Clear();
            foreach (var query in queries)
            {
                SavedQueries.Add(query);
            }
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler beim Laden gespeicherter Abfragen: {ex.Message}";
        }
        finally
        {
            IsLoading = false;
        }
    }

    private async Task SaveCurrentQueryAsync()
    {
        // Prompt for name and description (in real app, would show dialog)
        var query = new SavedQuery
        {
            Name = $"Query {DateTime.Now:yyyy-MM-dd HH:mm}",
            Description = "Manually saved query",
            QueryText = QueryText,
            Language = SelectedLanguage
        };

        try
        {
            var saved = await _queryService.SaveQueryAsync(query);
            
            if (!SavedQueries.Any(q => q.Id == saved.Id))
            {
                SavedQueries.Add(saved);
            }
            
            StatusMessage = $"Abfrage '{saved.Name}' gespeichert";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler beim Speichern: {ex.Message}";
        }
    }

    private void LoadQuery(SavedQuery? query)
    {
        if (query != null)
        {
            QueryText = query.QueryText;
            SelectedLanguage = query.Language;
            SelectedSavedQuery = query;
            StatusMessage = $"Abfrage '{query.Name}' geladen";
        }
    }

    private async Task DeleteQueryAsync(SavedQuery? query)
    {
        if (query != null)
        {
            try
            {
                var success = await _queryService.DeleteSavedQueryAsync(query.Id);
                
                if (success)
                {
                    SavedQueries.Remove(query);
                    StatusMessage = $"Abfrage '{query.Name}' gelöscht";
                    
                    if (SelectedSavedQuery?.Id == query.Id)
                    {
                        SelectedSavedQuery = null;
                    }
                }
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Löschen: {ex.Message}";
            }
        }
    }

    private void NewQuery()
    {
        QueryText = "FOR doc IN documents\n  LIMIT 10\n  RETURN doc";
        SelectedSavedQuery = null;
        LastResult = null;
        StatusMessage = "Neue Abfrage erstellt";
    }

    private void FormatQuery()
    {
        if (string.IsNullOrWhiteSpace(QueryText))
            return;

        try
        {
            // Basic AQL formatting
            var lines = QueryText.Split('\n');
            var formatted = string.Join("\n", lines.Select(l => l.Trim()));
            QueryText = formatted;
            StatusMessage = "Abfrage formatiert";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Formatierungsfehler: {ex.Message}";
        }
    }

    private void ClearResults()
    {
        LastResult = null;
        StatusMessage = "Ergebnisse gelöscht";
    }

    #endregion

    #region INotifyPropertyChanged

    public event PropertyChangedEventHandler? PropertyChanged;

    protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    #endregion
}


