/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TreeViewSettings.cs                                ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
