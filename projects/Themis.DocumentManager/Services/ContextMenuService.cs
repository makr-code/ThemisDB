/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ContextMenuService.cs                              ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     337                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Context-Menu Service für rechte Maus-Klicks
/// Verwaltet rollenabhängige Kontextmenü-Aktionen für verschiedene Entitätstypen
/// </summary>
public interface IContextMenuService
{
    Task<List<ContextMenuAction>> GetContextMenuActionsAsync(
        string userId,
        EntityType entityType,
        string entityId,
        CancellationToken cancellationToken = default
    );
}

public class ContextMenuService : IContextMenuService
{
    private readonly IRoleBasedPermissionService _permissionService;

    public ContextMenuService(IRoleBasedPermissionService permissionService)
    {
        _permissionService = permissionService;
    }

    public async Task<List<ContextMenuAction>> GetContextMenuActionsAsync(
        string userId,
        EntityType entityType,
        string entityId,
        CancellationToken cancellationToken = default
    )
    {
        var actions = new List<ContextMenuAction>();
        var permittedOps = await _permissionService.GetPermittedActionsAsync(userId, entityType, cancellationToken);

        // Standard-Aktionen basierend auf Berechtigungen
        if (permittedOps.Contains("Read"))
        {
            actions.Add(new ContextMenuAction
            {
                Id = "view",
                Label = "Anzeigen",
                Icon = "🔍",
                Action = "View",
                Group = "Basic"
            });
        }

        if (permittedOps.Contains("Update"))
        {
            actions.Add(new ContextMenuAction
            {
                Id = "edit",
                Label = "Bearbeiten",
                Icon = "✏️",
                Action = "Edit",
                Group = "Basic"
            });
        }

        if (permittedOps.Contains("Create"))
        {
            actions.Add(new ContextMenuAction
            {
                Id = "duplicate",
                Label = "Duplizieren",
                Icon = "📋",
                Action = "Duplicate",
                Group = "Basic"
            });
        }

        if (permittedOps.Contains("Delete"))
        {
            actions.Add(new ContextMenuAction
            {
                Id = "delete",
                Label = "Löschen",
                Icon = "🗑️",
                Action = "Delete",
                Group = "Danger"
            });
        }

        // Entity-spezifische Aktionen
        var entitySpecificActions = GetEntitySpecificActions(entityType, permittedOps);
        actions.AddRange(entitySpecificActions);

        // Organisation
        actions.Add(new ContextMenuAction
        {
            Id = "addFavorite",
            Label = "Zu Favoriten",
            Icon = "⭐",
            Action = "AddFavorite",
            Group = "Organization"
        });

        actions.Add(new ContextMenuAction
        {
            Id = "addTag",
            Label = "Markierung hinzufügen",
            Icon = "🏷️",
            Action = "AddTag",
            Group = "Organization"
        });

        // Info
        actions.Add(new ContextMenuAction
        {
            Id = "properties",
            Label = "Eigenschaften",
            Icon = "ℹ️",
            Action = "ShowProperties",
            Group = "Info"
        });

        return actions;
    }

    private static List<ContextMenuAction> GetEntitySpecificActions(EntityType entityType, List<string> permittedOps)
    {
        var actions = new List<ContextMenuAction>();

        switch (entityType)
        {
            case EntityType.Datei:
                if (permittedOps.Contains("Read"))
                {
                    actions.Add(new ContextMenuAction
                    {
                        Id = "download",
                        Label = "Herunterladen",
                        Icon = "⬇️",
                        Action = "Download",
                        Group = "FileOps"
                    });

                    actions.Add(new ContextMenuAction
                    {
                        Id = "preview",
                        Label = "Vorschau",
                        Icon = "👁️",
                        Action = "Preview",
                        Group = "FileOps"
                    });
                }

                if (permittedOps.Contains("Update"))
                {
                    actions.Add(new ContextMenuAction
                    {
                        Id = "rename",
                        Label = "Umbenennen",
                        Icon = "🔤",
                        Action = "Rename",
                        Group = "FileOps"
                    });
                }
                break;

            case EntityType.Dokument:
                if (permittedOps.Contains("Read"))
                {
                    actions.Add(new ContextMenuAction
                    {
                        Id = "download",
                        Label = "Herunterladen",
                        Icon = "⬇️",
                        Action = "Download",
                        Group = "DocOps"
                    });

                    actions.Add(new ContextMenuAction
                    {
                        Id = "viewRevisions",
                        Label = "Versionshistorie",
                        Icon = "📜",
                        Action = "ViewRevisions",
                        Group = "DocOps"
                    });
                }

                if (permittedOps.Contains("Update"))
                {
                    actions.Add(new ContextMenuAction
                    {
                        Id = "createRevision",
                        Label = "Neue Version",
                        Icon = "✨",
                        Action = "CreateRevision",
                        Group = "DocOps"
                    });

                    actions.Add(new ContextMenuAction
                    {
                        Id = "attachProcess",
                        Label = "Prozess anhängen",
                        Icon = "🔗",
                        Action = "AttachProcess",
                        Group = "DocOps"
                    });
                }
                break;

            case EntityType.Vorgang:
                if (permittedOps.Contains("Read"))
                {
                    actions.Add(new ContextMenuAction
                    {
                        Id = "viewTimeline",
                        Label = "Timeline anzeigen",
                        Icon = "📅",
                        Action = "ViewTimeline",
                        Group = "ProcOps"
                    });

                    actions.Add(new ContextMenuAction
                    {
                        Id = "viewComments",
                        Label = "Kommentare",
                        Icon = "💬",
                        Action = "ViewComments",
                        Group = "ProcOps"
                    });
                }

                if (permittedOps.Contains("Update"))
                {
                    actions.Add(new ContextMenuAction
                    {
                        Id = "assignTo",
                        Label = "Zuweisen",
                        Icon = "👤",
                        Action = "AssignTo",
                        Group = "ProcOps"
                    });

                    actions.Add(new ContextMenuAction
                    {
                        Id = "attachProcess",
                        Label = "Prozess anhängen",
                        Icon = "🔗",
                        Action = "AttachProcess",
                        Group = "ProcOps"
                    });
                }
                break;

            case EntityType.Akte:
            case EntityType.Ablage:
                if (permittedOps.Contains("Read"))
                {
                    actions.Add(new ContextMenuAction
                    {
                        Id = "viewContents",
                        Label = "Inhalte anzeigen",
                        Icon = "📂",
                        Action = "ViewContents",
                        Group = "FolderOps"
                    });
                }

                if (permittedOps.Contains("Create"))
                {
                    actions.Add(new ContextMenuAction
                    {
                        Id = "newSubfolder",
                        Label = "Unterordner erstellen",
                        Icon = "📁",
                        Action = "NewSubfolder",
                        Group = "FolderOps"
                    });
                }

                if (permittedOps.Contains("Update"))
                {
                    actions.Add(new ContextMenuAction
                    {
                        Id = "attachProcess",
                        Label = "Prozess anhängen",
                        Icon = "🔗",
                        Action = "AttachProcess",
                        Group = "FolderOps"
                    });
                }
                break;
        }

        return actions;
    }
}

#region DTO Models

public class ContextMenuAction
{
    public string Id { get; set; } = string.Empty;
    public string Label { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public string Action { get; set; } = string.Empty;
    public string Group { get; set; } = "General";
    public bool IsEnabled { get; set; } = true;
    public string? Description { get; set; }
}

#endregion
