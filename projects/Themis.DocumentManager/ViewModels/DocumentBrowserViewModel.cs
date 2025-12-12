using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.ViewModels;

/// <summary>
/// ViewModel für Document Browser mit RBAC-Integration
/// </summary>
public partial class DocumentBrowserViewModel : ObservableObject
{
    private readonly IDocumentService _documentService;
    private readonly IRoleBasedPermissionService _permissionService;
    private readonly IContextMenuService _contextMenuService;
    private readonly IAuthenticationService _authService;
    private readonly IAuditLoggingService _auditLoggingService;

    [ObservableProperty]
    private ObservableCollection<DocumentItemViewModel> documents = new();

    [ObservableProperty]
    private DocumentItemViewModel? selectedDocument;

    [ObservableProperty]
    private List<ContextMenuAction> contextMenuActions = new();

    [ObservableProperty]
    private bool isLoading = false;

    [ObservableProperty]
    private string? currentUserId;

    public DocumentBrowserViewModel(
        IDocumentService documentService,
        IRoleBasedPermissionService permissionService,
        IContextMenuService contextMenuService,
        IAuthenticationService authService,
        IAuditLoggingService auditLoggingService)
    {
        _documentService = documentService;
        _permissionService = permissionService;
        _contextMenuService = contextMenuService;
        _authService = authService;
        _auditLoggingService = auditLoggingService;
        
        CurrentUserId = authService.CurrentUserId ?? "urn:themis:user:local-admin";
    }

    [RelayCommand]
    public async Task LoadDocumentsAsync()
    {
        IsLoading = true;
        try
        {
            var documents = await _documentService.GetAllDocumentsAsync();
            Documents.Clear();
            foreach (var doc in documents)
            {
                var itemVm = new DocumentItemViewModel(doc)
                {
                    CanEdit = await _permissionService.CanUpdateAsync(CurrentUserId!, doc.Id, EntityType.Dokument),
                    CanDelete = await _permissionService.CanDeleteAsync(CurrentUserId!, doc.Id, EntityType.Dokument),
                    CanCreate = await _permissionService.CanCreateAsync(CurrentUserId!, EntityType.Dokument)
                };
                Documents.Add(itemVm);
            }
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    public async Task OnDocumentRightClickAsync(DocumentItemViewModel document)
    {
        SelectedDocument = document;
        
        try
        {
            var actions = await _contextMenuService.GetContextMenuActionsAsync(
                CurrentUserId!,
                EntityType.Dokument,
                document.Id
            );
            ContextMenuActions = actions;

            // Audit-Log: Context Menu angefordert
            await _auditLoggingService.LogActionAsync(new AuditLogEntry
            {
                UserId = CurrentUserId!,
                ActionType = "ContextMenu",
                EntityType = "Dokument",
                EntityId = document.Id,
                Details = $"Context menu requested for document: {document.Title}",
                Timestamp = DateTime.UtcNow
            });
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error loading context menu: {ex.Message}");
        }
    }

    [RelayCommand]
    public async Task ExecuteContextMenuActionAsync(ContextMenuAction action)
    {
        if (SelectedDocument == null) return;

        try
        {
            switch (action.Action)
            {
                case "View":
                    await ViewDocumentAsync(SelectedDocument.Id);
                    break;
                case "Edit":
                    await EditDocumentAsync(SelectedDocument.Id);
                    break;
                case "Delete":
                    await DeleteDocumentAsync(SelectedDocument.Id);
                    break;
                case "Download":
                    await DownloadDocumentAsync(SelectedDocument.Id);
                    break;
                case "ViewRevisions":
                    await ViewRevisionsAsync(SelectedDocument.Id);
                    break;
                case "CreateRevision":
                    await CreateRevisionAsync(SelectedDocument.Id);
                    break;
                case "AttachProcess":
                    await AttachProcessAsync(SelectedDocument.Id);
                    break;
                case "AddFavorite":
                    await AddToFavoritesAsync(SelectedDocument.Id);
                    break;
                case "AddTag":
                    await AddTagAsync(SelectedDocument.Id);
                    break;
                case "ShowProperties":
                    await ShowPropertiesAsync(SelectedDocument.Id);
                    break;
            }

            // Audit-Log: Aktion ausgeführt
            await _auditLoggingService.LogActionAsync(new AuditLogEntry
            {
                UserId = CurrentUserId!,
                ActionType = action.Action,
                EntityType = "Dokument",
                EntityId = SelectedDocument.Id,
                Details = $"Action '{action.Label}' executed on document: {SelectedDocument.Title}",
                Timestamp = DateTime.UtcNow
            });
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error executing action: {ex.Message}");
        }
    }

    private async Task ViewDocumentAsync(string docId)
    {
        // Implementation: Öffne Dokumentdetails
        await Task.Delay(100); // Placeholder
    }

    private async Task EditDocumentAsync(string docId)
    {
        // Implementation: Öffne Edit-Dialog
        await Task.Delay(100); // Placeholder
    }

    private async Task DeleteDocumentAsync(string docId)
    {
        // Implementation: Lösche Dokument mit Bestätigung
        await Task.Delay(100); // Placeholder
    }

    private async Task DownloadDocumentAsync(string docId)
    {
        // Implementation: Download ausführen
        await Task.Delay(100); // Placeholder
    }

    private async Task ViewRevisionsAsync(string docId)
    {
        // Implementation: Zeige Versionshistorie
        await Task.Delay(100); // Placeholder
    }

    private async Task CreateRevisionAsync(string docId)
    {
        // Implementation: Erstelle neue Version
        await Task.Delay(100); // Placeholder
    }

    private async Task AttachProcessAsync(string docId)
    {
        // Implementation: Zeige Prozess-Auswahl-Dialog
        // In echter Implementierung würde hier ein Window.ShowDialog() aufgerufen
        System.Diagnostics.Debug.WriteLine($"[UI] Process Attachment Dialog for document: {docId}");
        await Task.Delay(100); // Placeholder
    }

    private async Task AddToFavoritesAsync(string docId)
    {
        // Implementation: Zu Favoriten hinzufügen
        await Task.Delay(100); // Placeholder
    }

    private async Task AddTagAsync(string docId)
    {
        // Implementation: Tag-Dialog anzeigen
        await Task.Delay(100); // Placeholder
    }

    private async Task ShowPropertiesAsync(string docId)
    {
        // Implementation: Eigenschaften-Dialog anzeigen
        await Task.Delay(100); // Placeholder
    }
}

/// <summary>
/// Item ViewModel für einzelne Dokumente im Browser
/// </summary>
public class DocumentItemViewModel
{
    private readonly Document _document;

    public string Id => _document.Id;
    public string Title => _document.Title ?? "Untitled";
    public string Filename => _document.Filename ?? "unknown";
    public DateTime CreatedAt => _document.CreatedAt;
    public long SizeBytes => _document.SizeBytes;
    public bool CanEdit { get; set; }
    public bool CanDelete { get; set; }
    public bool CanCreate { get; set; }

    public DocumentItemViewModel(Document document)
    {
        _document = document;
    }
}
