/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentBrowserNodeViewModel.cs                    ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     135                                            ║
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

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;

namespace Themis.DocumentManager.Features.DocumentBrowser.ViewModels;

/// <summary>
/// Hierarchischer Knoten für TreeView im Document Browser
/// Unterstützt Lazy Loading, Caching und CRUD-Operationen
/// </summary>
public partial class DocumentBrowserNodeViewModel : ObservableObject
{
    [ObservableProperty]
    private string id = string.Empty;

    [ObservableProperty]
    private string name = string.Empty;

    [ObservableProperty]
    private string nodeType = string.Empty; // "Authority", "Filing", "File", "Document"

    [ObservableProperty]
    private int icon = 0; // Icon index

    [ObservableProperty]
    private ObservableCollection<DocumentBrowserNodeViewModel> children = new();

    [ObservableProperty]
    private bool isExpanded = false;

    [ObservableProperty]
    private bool isSelected = false;

    [ObservableProperty]
    private bool isLoading = false;

    [ObservableProperty]
    private bool hasChildren = true;

    [ObservableProperty]
    private bool isLoadedOnce = false;

    [ObservableProperty]
    private string? metadata = null;

    public DocumentBrowserNodeViewModel()
    {
    }

    public DocumentBrowserNodeViewModel(string id, string name, string nodeType, bool hasChildren = true)
    {
        Id = id;
        Name = name;
        NodeType = nodeType;
        HasChildren = hasChildren;
        Icon = GetIconForNodeType(nodeType);
    }

    /// <summary>
    /// Wird aufgerufen, wenn der Knoten expandiert wird (TreeView)
    /// </summary>
    [RelayCommand]
    public async Task OnExpandedAsync()
    {
        if (!IsLoadedOnce && HasChildren)
        {
            await LoadChildrenAsync();
        }
    }

    /// <summary>
    /// Lädt Kinder-Knoten (wird durch Parent-ViewModel implementiert)
    /// </summary>
    public Func<DocumentBrowserNodeViewModel, Task>? LoadChildrenFunc { get; set; }

    private async Task LoadChildrenAsync()
    {
        if (LoadChildrenFunc == null || IsLoadedOnce) return;

        IsLoading = true;
        try
        {
            await LoadChildrenFunc(this);
            IsLoadedOnce = true;
        }
        finally
        {
            IsLoading = false;
        }
    }

    private static int GetIconForNodeType(string nodeType)
    {
        return nodeType switch
        {
            "Authority" => 0,      // 🏛️ Behörde
            "Filing" => 1,         // 📂 Ablage
            "File" => 2,           // 📋 Akte
            "Document" => 3,       // 📄 Dokument
            _ => 4                 // ❓ Unbekannt
        };
    }
}

