/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TreeViewSettings.cs                                ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.ObjectModel;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Konfiguration für TreeView Navigation Items
/// </summary>
public class TreeViewSettings
{
    public ObservableCollection<TreeViewItemConfig> RootItems { get; set; } = new();
    public bool ShowIcons { get; set; } = true;
    public bool ExpandOnClick { get; set; } = true;
    public bool RememberExpansionState { get; set; } = true;
    public Dictionary<string, bool> ExpansionStates { get; set; } = new();
}

/// <summary>
/// Konfiguration für einzelnes TreeView-Item
/// </summary>
public class TreeViewItemConfig
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Header { get; set; } = string.Empty;
    public string Icon { get; set; } = "📄";
    public bool IsExpanded { get; set; }
    public TreeViewItemType ItemType { get; set; } = TreeViewItemType.Custom;
    public string? TargetView { get; set; }
    public string? NavigationPath { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
    public ObservableCollection<TreeViewItemConfig> Children { get; set; } = new();
}

public enum TreeViewItemType
{
    Dashboard,
    Documents,
    Tasks,
    Inbox,
    Favorites,
    Projects,
    Reminders,
    Cosigning,
    Classification,
    Collaboration,
    Custom,
    Folder,
    Link
}
