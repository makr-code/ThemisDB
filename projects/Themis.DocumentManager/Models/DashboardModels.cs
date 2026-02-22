/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DashboardModels.cs                                 ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     157                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
/// Recent document or file accessed by the user
/// </summary>
public class RecentItem
{
    public string Id { get; set; } = string.Empty;
    public RecentItemType Type { get; set; }
    public string Name { get; set; } = string.Empty;
    public string? FileReference { get; set; }
    public DateTime LastAccessedAt { get; set; }
    public string? IconPath { get; set; }
    public string? PreviewText { get; set; }
    public bool IsPinned { get; set; }
}

public enum RecentItemType
{
    Document,
    File,
    Process,
    Folder
}

/// <summary>
/// User favorite item
/// </summary>
public class FavoriteItem
{
    public string Id { get; set; } = string.Empty;
    public FavoriteItemType Type { get; set; }
    public string Name { get; set; } = string.Empty;
    public string? Description { get; set; }
    public string? Icon { get; set; }
    public string TargetId { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; }
    public int Order { get; set; }
    public DateTime LastModifiedAt { get; set; } = DateTime.UtcNow;
}

public enum FavoriteItemType
{
    Document,
    File,
    Process,
    Search,
    View,
    Report
}

/// <summary>
/// User-configurable quick action
/// </summary>
public class QuickAction
{
    public string Id { get; set; } = string.Empty;
    public string Label { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public QuickActionType Type { get; set; }
    public string TargetCommand { get; set; } = string.Empty;
    public Dictionary<string, object> Parameters { get; set; } = new();
    public bool IsEnabled { get; set; } = true;
    public int Order { get; set; }
    public DateTime LastModifiedAt { get; set; } = DateTime.UtcNow;
}

public enum QuickActionType
{
    OpenView,
    CreateProcess,
    CreateDocument,
    Search,
    CustomCommand
}

/// <summary>
/// ThemisDB connection status and quality metrics
/// </summary>
public class ConnectionStatus
{
    public ConnectionState State { get; set; }
    public int Latency { get; set; } // in milliseconds
    public DateTime LastChecked { get; set; }
    public string? ServerVersion { get; set; }
    public ConnectionQuality Quality { get; set; }
    public string? ErrorMessage { get; set; }
}

public enum ConnectionState
{
    Connected,
    Connecting,
    Disconnected,
    Error
}

public enum ConnectionQuality
{
    Excellent,  // <50ms
    Good,       // 50-150ms
    Fair,       // 150-500ms
    Poor        // >500ms
}

/// <summary>
/// Dashboard widget configuration
/// </summary>
public class DashboardWidget
{
    public string Id { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public WidgetType Type { get; set; }
    public int Row { get; set; }
    public int Column { get; set; }
    public int RowSpan { get; set; } = 1;
    public int ColumnSpan { get; set; } = 1;
    public bool IsVisible { get; set; } = true;
    public Dictionary<string, object> Settings { get; set; } = new();
}

public enum WidgetType
{
    RecentDocuments,
    RecentFiles,
    Favorites,
    QuickActions,
    Statistics,
    Calendar,
    Tasks,
    Notifications
}
