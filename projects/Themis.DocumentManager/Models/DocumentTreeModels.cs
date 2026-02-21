/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentTreeModels.cs                              ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     245                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Themis.DocumentManager.Models;

#nullable enable

/// <summary>
/// Represents a node in the document tree with configurable nesting depth
/// </summary>
public class DocumentTreeNode
{
    public string Id { get; set; } = string.Empty;
    public DocumentTreeNodeType Type { get; set; }
    public string Name { get; set; } = string.Empty;
    public string? Icon { get; set; }
    public int ChildCount { get; set; }
    public bool IsExpanded { get; set; }
    public DateTime? ModifiedAt { get; set; }
    public long? Size { get; set; }
    public DocumentStatus Status { get; set; }
    public ObservableCollection<DocumentTreeNode> Children { get; set; } = new();
    
    // Hierarchy tracking
    public int Level { get; set; }
    public string? ParentId { get; set; }
    public List<string> PathToRoot { get; set; } = new(); // IDs from root to this node
    
    // Metadata
    public string? ProcessId { get; set; }
    public string? FileReference { get; set; }
    public string? CreatedBy { get; set; }
    public List<string> Tags { get; set; } = new();
    public SecurityClassification SecurityLevel { get; set; }
    
    // UI Properties
    public bool IsProcess => Type == DocumentTreeNodeType.Process;
    public bool HasChildren => ChildCount > 0;
    public bool ShowDetails { get; set; } = true;
    
    // Custom nesting support
    public Dictionary<string, object> CustomMetadata { get; set; } = new();
}

public enum DocumentTreeNodeType
{
    // Standard hierarchy (7 levels)
    Authority,      // Behörde (Level 0)
    Filing,         // Aktenplan (Level 1)
    File,           // Akte (Level 2)
    SubFile,        // Unterakte (Level 3)
    Process,        // Vorgang (Level 4)
    Document,       // Dokument (Level 5)
    Attachment,     // Anhang (Level 6)
    
    // Additional types
    Inbox,          // Posteingang-Ordner
    InboxItem,      // Einzelne Eingangsnachricht
    Outbox,         // Postausgang-Ordner
    OutboxItem,     // Einzelne Ausgangsnachricht
    Folder,         // Allgemeiner Ordner
    
    // Extended nesting support (configurable)
    CustomLevel1,   // Benutzerdefinierte Ebene 1
    CustomLevel2,   // Benutzerdefinierte Ebene 2
    CustomLevel3,   // Benutzerdefinierte Ebene 3
    CustomLevel4,   // Benutzerdefinierte Ebene 4
    CustomLevel5    // Benutzerdefinierte Ebene 5
}

public enum DocumentStatus
{
    New,
    InProgress,
    Completed,
    Archived,
    Deleted
}

/// <summary>
/// Filter configuration for document tree
/// </summary>
public class DocumentTreeFilter
{
    public List<DocumentTreeNodeType> Types { get; set; } = new();
    public List<DocumentStatus> Statuses { get; set; } = new();
    public DateTime? DateFrom { get; set; }
    public DateTime? DateTo { get; set; }
    public string? SearchText { get; set; }
    public string? CreatedBy { get; set; }
    public List<string> Tags { get; set; } = new();
    public List<SecurityClassification> SecurityLevels { get; set; } = new();
}

/// <summary>
/// Configuration for document tree display with configurable nesting
/// </summary>
public class DocumentTreeConfiguration
{
    public TreeViewMode ViewMode { get; set; } = TreeViewMode.Tree;
    public TreeGrouping Grouping { get; set; } = TreeGrouping.ByProcess;
    public TreeSortField SortField { get; set; } = TreeSortField.Date;
    public SortDirection SortDirection { get; set; } = SortDirection.Descending;
    public IconSize IconSize { get; set; } = IconSize.Medium;
    public DetailLevel DetailLevel { get; set; } = DetailLevel.Standard;
    public AutoExpandMode AutoExpand { get; set; } = AutoExpandMode.ActiveProcess;
    
    // Configurable nesting depth
    public int MaxNestingDepth { get; set; } = 12; // Default: 7 standard + 5 custom levels
    public bool EnableUnlimitedNesting { get; set; } = false; // For extremely deep hierarchies
    public Dictionary<int, string> CustomLevelNames { get; set; } = new(); // Level -> Display Name
    
    // Performance settings for deep nesting
    public bool LazyLoadChildren { get; set; } = true; // Load children on demand
    public int InitialLoadDepth { get; set; } = 3; // Initially load only 3 levels
    public bool VirtualizeTree { get; set; } = true; // Use virtualization for performance
}

public enum TreeViewMode
{
    Tree,       // Hierarchische Baumansicht
    List,       // Flache Liste
    Tiles       // Kacheln
}

public enum TreeGrouping
{
    ByProcess,  // Nach Vorgang gruppieren
    ByType,     // Nach Dokumenttyp gruppieren
    ByDate,     // Nach Datum gruppieren
    Flat        // Keine Gruppierung
}

public enum TreeSortField
{
    Name,
    Date,
    Size,
    Type,
    Status,
    Relevance   // Basierend auf Zugriffshäufigkeit
}

public enum SortDirection
{
    Ascending,
    Descending
}

public enum IconSize
{
    Small,
    Medium,
    Large
}

public enum DetailLevel
{
    Compact,    // Nur Name
    Standard,   // Name + Icon + Count
    Extended    // Name + Icon + Count + Datum + Größe
}

public enum AutoExpandMode
{
    None,           // Nichts aufklappen
    ActiveProcess,  // Nur aktiver Vorgang
    All             // Alles aufklappen
}

/// <summary>
/// Represents an item in the task basket
/// </summary>
public class TaskBasketItem
{
    public string Id { get; set; } = string.Empty;
    public TaskBasketItemType Type { get; set; }
    public string Title { get; set; } = string.Empty;
    public string? Description { get; set; }
    public string? ProcessId { get; set; }
    public string? FileReference { get; set; }
    public DateTime? DueDate { get; set; }
    public TaskPriority Priority { get; set; }
    public bool IsOverdue { get; set; }
    public string Icon { get; set; } = string.Empty;
    public string Color { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; }
}

public enum TaskBasketItemType
{
    Reminder,       // Wiedervorlage
    Cosigning,      // Mitzeichnung
    InboxItem,      // Inbox-Eintrag
    Task,           // Aufgabe
    Deadline        // Frist
}

public enum TaskPriority
{
    Low,
    Normal,
    High,
    Urgent
}

/// <summary>
/// Summary statistics for task basket
/// </summary>
public class TaskBasketStatistics
{
    public int TotalTasks { get; set; }
    public int OverdueTasks { get; set; }
    public int DueToday { get; set; }
    public int DueThisWeek { get; set; }
    public int HighPriorityTasks { get; set; }
    public int UrgentTasks { get; set; }
}
