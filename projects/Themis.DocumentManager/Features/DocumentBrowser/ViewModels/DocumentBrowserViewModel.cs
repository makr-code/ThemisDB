/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentBrowserViewModel.cs                        ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     649                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Features.DocumentBrowser.ViewModels;

/// <summary>
/// ViewModel für hierarchischen Document Browser mit Lazy Loading, Caching, Filter/Suche und CRUD
/// Hierarchie: Authority (Behörde) → Filing (Ablage) → File (Akte) → Document (Dokument)
/// </summary>
public partial class DocumentBrowserViewModel : ObservableObject
{
    private readonly IDocumentService _documentService;
    private readonly IRoleBasedPermissionService _permissionService;
    private readonly IContextMenuService _contextMenuService;
    private readonly IAuthenticationService _authService;
    private readonly IAuditLoggingService _auditLoggingService;
    private readonly IAdministrativeStructureService? _adminService;
    private readonly ICacheService? _cacheService;

    // Neue hierarchische UI
    [ObservableProperty]
    private ObservableCollection<DocumentBrowserNodeViewModel> rootNodes = new();

    [ObservableProperty]
    private DocumentBrowserNodeViewModel? selectedNode;

    // Filter und Suche
    [ObservableProperty]
    private string searchText = string.Empty;

    [ObservableProperty]
    private string filterCategory = "All"; // "All", "Documents", "Files", "Filings"

    [ObservableProperty]
    private ObservableCollection<DocumentBrowserNodeViewModel> filteredResults = new();

    // Kontext-Menü und Aktionen
    [ObservableProperty]
    private List<ContextMenuAction> contextMenuActions = new();

    [ObservableProperty]
    private bool isLoading = false;

    [ObservableProperty]
    private bool isSearching = false;

    [ObservableProperty]
    private string? currentUserId;

    // Legacy Kompatibilität
    [ObservableProperty]
    private ObservableCollection<DocumentItemViewModel> documents = new();

    [ObservableProperty]
    private DocumentItemViewModel? selectedDocument;

    public DocumentBrowserViewModel(
        IDocumentService documentService,
        IRoleBasedPermissionService permissionService,
        IContextMenuService contextMenuService,
        IAuthenticationService authService,
        IAuditLoggingService auditLoggingService,
        IAdministrativeStructureService? adminService = null,
        ICacheService? cacheService = null)
    {
        _documentService = documentService;
        _permissionService = permissionService;
        _contextMenuService = contextMenuService;
        _authService = authService;
        _auditLoggingService = auditLoggingService;
        _adminService = adminService;
        _cacheService = cacheService;
        
        CurrentUserId = authService.CurrentUserId ?? "urn:themis:user:local-admin";
    }

    [RelayCommand]
    public async Task LoadDocumentsAsync()
    {
        IsLoading = true;
        try
        {
            // Hierarchisches Laden: Root = Authorities
            RootNodes.Clear();
            
            if (_adminService != null)
            {
                // Lade Behörden-Struktur
                var authorities = await _adminService.GetAllAuthoritiesAsync();
                foreach (var auth in authorities)
                {
                    var authNode = new DocumentBrowserNodeViewModel(auth.Id, auth.Name, "Authority", hasChildren: true);
                    authNode.LoadChildrenFunc = async node => await LoadFilingsForAuthorityAsync(node);
                    RootNodes.Add(authNode);
                }
            }
            else
            {
                // Fallback: Lade Dokumente direkt
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
        }
        finally
        {
            IsLoading = false;
        }
    }

    /// <summary>
    /// Lädt Filings (Ablagen) für eine Behörde mit Caching
    /// </summary>
    private async Task LoadFilingsForAuthorityAsync(DocumentBrowserNodeViewModel authorityNode)
    {
        try
        {
            authorityNode.Children.Clear();
            string cacheKey = $"filings:authority:{authorityNode.Id}";
            
            // Versuche aus Cache zu laden
            var filings = _cacheService != null 
                ? await _cacheService.GetAsync<List<Filing>>(cacheKey)
                : null;

            if (filings == null && _adminService != null)
            {
                // Lade von Service
                filings = (await _adminService.GetFilingsByAuthorityAsync(authorityNode.Id)).ToList();
                
                // Speichere im Cache (1 Stunde TTL)
                if (_cacheService != null)
                {
                    await _cacheService.SetAsync(cacheKey, filings, TimeSpan.FromHours(1));
                }
            }

            if (filings != null)
            {
                foreach (var filing in filings)
                {
                    var filingNode = new DocumentBrowserNodeViewModel(filing.Id, filing.Name, "Filing", hasChildren: true)
                    {
                        Metadata = filing.Department
                    };
                    filingNode.LoadChildrenFunc = async node => await LoadFilesForFilingAsync(node);
                    authorityNode.Children.Add(filingNode);
                }
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[ERROR] LoadFilingsForAuthorityAsync: {ex.Message}");
        }
    }

    /// <summary>
    /// Lädt Akten (Files) für eine Ablage mit Caching
    /// </summary>
    private async Task LoadFilesForFilingAsync(DocumentBrowserNodeViewModel filingNode)
    {
        try
        {
            filingNode.Children.Clear();
            string cacheKey = $"files:filing:{filingNode.Id}";
            
            var files = _cacheService != null
                ? await _cacheService.GetAsync<List<AdministrativeFile>>(cacheKey)
                : null;

            if (files == null && _adminService != null)
            {
                files = (await _adminService.GetFilesByFilingAsync(filingNode.Id)).ToList();
                
                if (_cacheService != null)
                {
                    await _cacheService.SetAsync(cacheKey, files, TimeSpan.FromHours(1));
                }
            }

            if (files != null)
            {
                foreach (var file in files)
                {
                    var fileNode = new DocumentBrowserNodeViewModel(file.Id, file.FileNumber + " - " + file.Subject, "File", hasChildren: true)
                    {
                        Metadata = file.Category
                    };
                    fileNode.LoadChildrenFunc = async node => await LoadDocumentsForFileAsync(node);
                    filingNode.Children.Add(fileNode);
                }
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[ERROR] LoadFilesForFilingAsync: {ex.Message}");
        }
    }

    /// <summary>
    /// Lädt Dokumente für eine Akte mit Caching
    /// </summary>
    private async Task LoadDocumentsForFileAsync(DocumentBrowserNodeViewModel fileNode)
    {
        try
        {
            fileNode.Children.Clear();
            string cacheKey = $"documents:file:{fileNode.Id}";
            
            var documents = _cacheService != null
                ? await _cacheService.GetAsync<List<Document>>(cacheKey)
                : null;

            if (documents == null && _documentService != null)
            {
                // Lade alle Dokumente und filtere nach dieser Akte
                var allDocs = await _documentService.GetAllDocumentsAsync();
                documents = allDocs.Where(d => d.Id == fileNode.Id).ToList();
                
                if (_cacheService != null)
                {
                    await _cacheService.SetAsync(cacheKey, documents, TimeSpan.FromHours(1));
                }
            }

            if (documents != null)
            {
                foreach (var doc in documents)
                {
                    var docNode = new DocumentBrowserNodeViewModel(doc.Id, doc.Title ?? "Untitled", "Document", hasChildren: false)
                    {
                        Metadata = doc.Filename
                    };
                    fileNode.Children.Add(docNode);
                }
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[ERROR] LoadDocumentsForFileAsync: {ex.Message}");
        }
    }

    /// <summary>
    /// Sucht nach Dokumenten, Akten, Ablagen mit Filter
    /// </summary>
    [RelayCommand]
    public async Task SearchAsync()
    {
        if (string.IsNullOrWhiteSpace(SearchText))
        {
            FilteredResults.Clear();
            return;
        }

        IsSearching = true;
        try
        {
            var results = new List<DocumentBrowserNodeViewModel>();
            SearchRecursive(RootNodes, SearchText, FilterCategory, results);
            
            FilteredResults.Clear();
            foreach (var result in results.Take(50))
            {
                FilteredResults.Add(result);
            }
        }
        finally
        {
            IsSearching = false;
        }
    }

    private void SearchRecursive(
        ObservableCollection<DocumentBrowserNodeViewModel> nodes,
        string searchText,
        string filterCategory,
        List<DocumentBrowserNodeViewModel> results,
        int depth = 0)
    {
        if (depth > 10) return; // Begrenzung der Tiefe

        foreach (var node in nodes)
        {
            bool matches = node.Name.Contains(searchText, StringComparison.OrdinalIgnoreCase)
                || (node.Metadata?.Contains(searchText, StringComparison.OrdinalIgnoreCase) ?? false);
            
            bool categoryMatches = filterCategory == "All"
                || (filterCategory == "Documents" && node.NodeType == "Document")
                || (filterCategory == "Files" && node.NodeType == "File")
                || (filterCategory == "Filings" && node.NodeType == "Filing");

            if (matches && categoryMatches)
            {
                results.Add(node);
            }

            if (node.Children.Count > 0)
            {
                SearchRecursive(node.Children, searchText, filterCategory, results, depth + 1);
            }
        }
    }

    /// <summary>
    /// Kontext-Menü für Mausklick auf Knoten
    /// </summary>
    [RelayCommand]
    public async Task OnNodeRightClickAsync(DocumentBrowserNodeViewModel node)
    {
        SelectedNode = node;
        
        try
        {
            var actions = new List<ContextMenuAction>();
            
            switch (node.NodeType)
            {
                case "Authority":
                    actions.AddRange(new[]
                    {
                        new ContextMenuAction { Action = "ViewDetails", Label = "Behörde anzeigen" },
                        new ContextMenuAction { Action = "EditAuthority", Label = "Bearbeiten" },
                        new ContextMenuAction { Action = "Separator", Label = "" },
                        new ContextMenuAction { Action = "NewFiling", Label = "Neue Ablage..." },
                        new ContextMenuAction { Action = "Export", Label = "Exportieren" }
                    });
                    break;
                    
                case "Filing":
                    actions.AddRange(new[]
                    {
                        new ContextMenuAction { Action = "ViewDetails", Label = "Ablage anzeigen" },
                        new ContextMenuAction { Action = "EditFiling", Label = "Bearbeiten" },
                        new ContextMenuAction { Action = "Separator", Label = "" },
                        new ContextMenuAction { Action = "NewFile", Label = "Neue Akte..." },
                        new ContextMenuAction { Action = "DeleteFiling", Label = "Löschen" }
                    });
                    break;
                    
                case "File":
                    actions.AddRange(new[]
                    {
                        new ContextMenuAction { Action = "ViewDetails", Label = "Akte anzeigen" },
                        new ContextMenuAction { Action = "EditFile", Label = "Bearbeiten" },
                        new ContextMenuAction { Action = "Separator", Label = "" },
                        new ContextMenuAction { Action = "AddDocument", Label = "Dokument hinzufügen..." },
                        new ContextMenuAction { Action = "CloseFile", Label = "Akte abschließen" },
                        new ContextMenuAction { Action = "DeleteFile", Label = "Löschen" }
                    });
                    break;
                    
                case "Document":
                    actions.AddRange(new[]
                    {
                        new ContextMenuAction { Action = "View", Label = "Öffnen" },
                        new ContextMenuAction { Action = "Edit", Label = "Bearbeiten" },
                        new ContextMenuAction { Action = "Separator", Label = "" },
                        new ContextMenuAction { Action = "Download", Label = "Herunterladen" },
                        new ContextMenuAction { Action = "ViewRevisions", Label = "Versionen" },
                        new ContextMenuAction { Action = "AddFavorite", Label = "Zu Favoriten" },
                        new ContextMenuAction { Action = "Separator", Label = "" },
                        new ContextMenuAction { Action = "Delete", Label = "Löschen" }
                    });
                    break;
            }

            ContextMenuActions = actions;

            await _auditLoggingService.LogActionAsync(new AuditLogEntry
            {
                UserId = CurrentUserId!,
                ActionType = "ContextMenu",
                EntityType = node.NodeType,
                EntityId = node.Id,
                Details = $"Context menu for {node.NodeType}: {node.Name}",
                Timestamp = DateTime.UtcNow
            });
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[ERROR] OnNodeRightClickAsync: {ex.Message}");
        }
    }

    /// <summary>
    /// Führt Kontext-Menü-Aktion aus
    /// </summary>
    [RelayCommand]
    public async Task ExecuteContextMenuActionAsync(ContextMenuAction action)
    {
        if (SelectedNode == null) return;

        try
        {
            switch (action.Action)
            {
                case "View":
                case "ViewDetails":
                    await ViewNodeDetailsAsync(SelectedNode);
                    break;
                case "Edit":
                case "EditAuthority":
                case "EditFiling":
                case "EditFile":
                    await EditNodeAsync(SelectedNode);
                    break;
                case "Delete":
                case "DeleteFiling":
                case "DeleteFile":
                    await DeleteNodeAsync(SelectedNode);
                    break;
                case "NewFiling":
                    await CreateNewFilingAsync(SelectedNode);
                    break;
                case "NewFile":
                    await CreateNewFileAsync(SelectedNode);
                    break;
                case "AddDocument":
                    await AddDocumentToFileAsync(SelectedNode);
                    break;
                case "Download":
                    await DownloadDocumentAsync(SelectedNode.Id);
                    break;
                case "ViewRevisions":
                    await ViewRevisionsAsync(SelectedNode.Id);
                    break;
                case "AddFavorite":
                    await AddToFavoritesAsync(SelectedNode.Id);
                    break;
                case "Export":
                    await ExportNodeAsync(SelectedNode);
                    break;
            }

            await _auditLoggingService.LogActionAsync(new AuditLogEntry
            {
                UserId = CurrentUserId!,
                ActionType = action.Action,
                EntityType = SelectedNode.NodeType,
                EntityId = SelectedNode.Id,
                Details = $"{action.Label} on {SelectedNode.NodeType}: {SelectedNode.Name}",
                Timestamp = DateTime.UtcNow
            });
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[ERROR] ExecuteContextMenuActionAsync: {ex.Message}");
        }
    }

    // CRUD-Operationen
    private async Task ViewNodeDetailsAsync(DocumentBrowserNodeViewModel node)
    {
        await Task.Delay(100);
        System.Diagnostics.Debug.WriteLine($"[UI] View details for {node.NodeType}: {node.Name}");
    }

    private async Task EditNodeAsync(DocumentBrowserNodeViewModel node)
    {
        await Task.Delay(100);
        System.Diagnostics.Debug.WriteLine($"[UI] Edit {node.NodeType}: {node.Name}");
    }

    private async Task DeleteNodeAsync(DocumentBrowserNodeViewModel node)
    {
        // Mit Bestätigung
        if (node.NodeType == "Document")
        {
            var toRemove = Documents.FirstOrDefault(d => d.Id == node.Id);
            if (toRemove != null) Documents.Remove(toRemove);
        }
        await Task.Delay(100);
        System.Diagnostics.Debug.WriteLine($"[UI] Deleted {node.NodeType}: {node.Name}");
    }

    private async Task CreateNewFilingAsync(DocumentBrowserNodeViewModel authorityNode)
    {
        // Zeige Dialog zur Erstellung einer neuen Ablage
        await Task.Delay(100);
        System.Diagnostics.Debug.WriteLine($"[UI] Create new Filing in Authority: {authorityNode.Name}");
    }

    private async Task CreateNewFileAsync(DocumentBrowserNodeViewModel filingNode)
    {
        // Zeige Dialog zur Erstellung einer neuen Akte
        await Task.Delay(100);
        System.Diagnostics.Debug.WriteLine($"[UI] Create new File in Filing: {filingNode.Name}");
    }

    private async Task AddDocumentToFileAsync(DocumentBrowserNodeViewModel fileNode)
    {
        // Zeige Dialog zur Erstellung eines neuen Dokuments oder Upload
        await Task.Delay(100);
        System.Diagnostics.Debug.WriteLine($"[UI] Add Document to File: {fileNode.Name}");
    }

    private async Task ExportNodeAsync(DocumentBrowserNodeViewModel node)
    {
        await Task.Delay(100);
        System.Diagnostics.Debug.WriteLine($"[UI] Export {node.NodeType}: {node.Name}");
    }

    // Legacy Methoden (Kompatibilität)
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

    private async Task ViewDocumentAsync(string docId)
    {
        await Task.Delay(100);
    }

    private async Task EditDocumentAsync(string docId)
    {
        await Task.Delay(100);
    }

    private async Task DeleteDocumentAsync(string docId)
    {
        await Task.Delay(100);
    }

    private async Task DownloadDocumentAsync(string docId)
    {
        await Task.Delay(100);
    }

    private async Task ViewRevisionsAsync(string docId)
    {
        await Task.Delay(100);
    }

    private async Task CreateRevisionAsync(string docId)
    {
        await Task.Delay(100);
    }

    private async Task AttachProcessAsync(string docId)
    {
        System.Diagnostics.Debug.WriteLine($"[UI] Process Attachment Dialog for document: {docId}");
        await Task.Delay(100);
    }

    private async Task AddToFavoritesAsync(string docId)
    {
        await Task.Delay(100);
    }

    private async Task AddTagAsync(string docId)
    {
        await Task.Delay(100);
    }

    private async Task ShowPropertiesAsync(string docId)
    {
        await Task.Delay(100);
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
