/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ERDViewModel.cs                                    ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     349                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using CommunityToolkit.Mvvm.Input;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Features.ERDQueryEditor.Services;

namespace Themis.DocumentManager.Features.ERDQueryEditor.ViewModels;

/// <summary>
/// ViewModel for Entity-Relationship Diagram (ERD) visualization
/// </summary>
public class ERDViewModel : INotifyPropertyChanged
{
    private readonly ISchemaService _schemaService;
    private DatabaseSchema? _schema;
    private EntityDefinition? _selectedEntity;
    private RelationshipDefinition? _selectedRelationship;
    private bool _isLoading;
    private string _statusMessage = "Bereit";
    private double _zoomLevel = 1.0;
    private Point _panOffset = new Point(0, 0);

    public ERDViewModel(ISchemaService schemaService)
    {
        _schemaService = schemaService ?? throw new ArgumentNullException(nameof(schemaService));

        // Initialize commands
        LoadSchemaCommand = new AsyncRelayCommand(LoadSchemaAsync);
        RefreshSchemaCommand = new AsyncRelayCommand(RefreshSchemaAsync);
        SelectEntityCommand = new RelayCommand<EntityDefinition>(SelectEntity);
        SelectRelationshipCommand = new RelayCommand<RelationshipDefinition>(SelectRelationship);
        ZoomInCommand = new RelayCommand(ZoomIn);
        ZoomOutCommand = new RelayCommand(ZoomOut);
        ResetZoomCommand = new RelayCommand(ResetZoom);
        AutoLayoutCommand = new RelayCommand(AutoLayout);
        ExportDiagramCommand = new AsyncRelayCommand(ExportDiagramAsync);

        // Auto-load schema on initialization
        Task.Run(async () => 
        {
            try 
            {
                await LoadSchemaAsync();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error during initial schema load: {ex.Message}");
            }
        });
    }

    #region Properties

    public DatabaseSchema? Schema
    {
        get => _schema;
        set
        {
            if (_schema != value)
            {
                _schema = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(HasSchema));
                OnPropertyChanged(nameof(Entities));
                OnPropertyChanged(nameof(Relationships));
            }
        }
    }

    public ObservableCollection<EntityDefinition> Entities => Schema?.Entities ?? new ObservableCollection<EntityDefinition>();
    public ObservableCollection<RelationshipDefinition> Relationships => Schema?.Relationships ?? new ObservableCollection<RelationshipDefinition>();

    public bool HasSchema => Schema != null;

    public EntityDefinition? SelectedEntity
    {
        get => _selectedEntity;
        set
        {
            if (_selectedEntity != value)
            {
                _selectedEntity = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(HasSelectedEntity));
            }
        }
    }

    public bool HasSelectedEntity => SelectedEntity != null;

    public RelationshipDefinition? SelectedRelationship
    {
        get => _selectedRelationship;
        set
        {
            if (_selectedRelationship != value)
            {
                _selectedRelationship = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(HasSelectedRelationship));
            }
        }
    }

    public bool HasSelectedRelationship => SelectedRelationship != null;

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

    public double ZoomLevel
    {
        get => _zoomLevel;
        set
        {
            if (_zoomLevel != value)
            {
                _zoomLevel = Math.Max(0.1, Math.Min(5.0, value));
                OnPropertyChanged();
                OnPropertyChanged(nameof(ZoomPercentage));
            }
        }
    }

    public string ZoomPercentage => $"{ZoomLevel * 100:F0}%";

    public Point PanOffset
    {
        get => _panOffset;
        set
        {
            if (_panOffset != value)
            {
                _panOffset = value;
                OnPropertyChanged();
            }
        }
    }

    #endregion

    #region Commands

    public IAsyncRelayCommand LoadSchemaCommand { get; }
    public IAsyncRelayCommand RefreshSchemaCommand { get; }
    public IRelayCommand<EntityDefinition> SelectEntityCommand { get; }
    public IRelayCommand<RelationshipDefinition> SelectRelationshipCommand { get; }
    public IRelayCommand ZoomInCommand { get; }
    public IRelayCommand ZoomOutCommand { get; }
    public IRelayCommand ResetZoomCommand { get; }
    public IRelayCommand AutoLayoutCommand { get; }
    public IAsyncRelayCommand ExportDiagramCommand { get; }

    #endregion

    #region Methods

    private async Task LoadSchemaAsync()
    {
        IsLoading = true;
        StatusMessage = "Lade Datenbankschema...";

        try
        {
            Schema = await _schemaService.GetSchemaAsync();
            StatusMessage = $"Schema geladen: {Schema.Entities.Count} Entitäten, {Schema.Relationships.Count} Beziehungen";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler beim Laden: {ex.Message}";
            System.Diagnostics.Debug.WriteLine($"Schema load error: {ex}");
        }
        finally
        {
            IsLoading = false;
        }
    }

    private async Task RefreshSchemaAsync()
    {
        IsLoading = true;
        StatusMessage = "Aktualisiere Schema...";

        try
        {
            await _schemaService.RefreshSchemaAsync();
            Schema = await _schemaService.GetSchemaAsync();
            StatusMessage = $"Schema aktualisiert: {Schema.Entities.Count} Entitäten";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler beim Aktualisieren: {ex.Message}";
        }
        finally
        {
            IsLoading = false;
        }
    }

    private void SelectEntity(EntityDefinition? entity)
    {
        SelectedEntity = entity;
        SelectedRelationship = null;
        
        if (entity != null)
        {
            StatusMessage = $"Entität ausgewählt: {entity.Name} ({entity.Attributes.Count} Attribute)";
        }
    }

    private void SelectRelationship(RelationshipDefinition? relationship)
    {
        SelectedRelationship = relationship;
        SelectedEntity = null;

        if (relationship != null)
        {
            StatusMessage = $"Beziehung ausgewählt: {relationship.Name} ({relationship.Type})";
        }
    }

    private void ZoomIn()
    {
        ZoomLevel += 0.1;
    }

    private void ZoomOut()
    {
        ZoomLevel -= 0.1;
    }

    private void ResetZoom()
    {
        ZoomLevel = 1.0;
        PanOffset = new Point(0, 0);
    }

    private void AutoLayout()
    {
        if (Schema == null || !Schema.Entities.Any())
            return;

        // Simple grid layout algorithm
        const double spacing = 350;
        const double startX = 50;
        const double startY = 50;
        
        var entitiesPerRow = (int)Math.Ceiling(Math.Sqrt(Schema.Entities.Count));
        
        for (int i = 0; i < Schema.Entities.Count; i++)
        {
            var row = i / entitiesPerRow;
            var col = i % entitiesPerRow;
            
            var entity = Schema.Entities[i];
            entity.X = startX + (col * spacing);
            entity.Y = startY + (row * spacing);
        }

        OnPropertyChanged(nameof(Entities));
        StatusMessage = "Layout automatisch angepasst";
    }

    private async Task ExportDiagramAsync()
    {
        await Task.CompletedTask;
        StatusMessage = "Export-Funktion wird in zukünftiger Version implementiert";
        // Future: Export to PNG, SVG, or GraphML
    }

    public string GetEntityById(string entityId)
    {
        var entity = Entities.FirstOrDefault(e => e.Id == entityId);
        return entity?.Name ?? "Unknown";
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


