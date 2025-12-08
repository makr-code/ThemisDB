using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Themis.DocumentManager.Models;

#nullable enable

/// <summary>
/// Represents a node in the document tree
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
}

public enum DocumentTreeNodeType
{
    File,           // Akte
    Process,        // Vorgang
    Document,       // Dokument
    Inbox,          // Posteingang-Ordner
    InboxItem,      // Einzelne Eingangsnachricht
    Outbox,         // Postausgang-Ordner
    OutboxItem,     // Einzelne Ausgangsnachricht
    Attachment,     // Anhang
    Folder          // Allgemeiner Ordner
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
/// Configuration for document tree display
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
