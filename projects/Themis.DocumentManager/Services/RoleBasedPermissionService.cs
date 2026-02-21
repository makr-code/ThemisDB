/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RoleBasedPermissionService.cs                      ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     199                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 362722340  2025-12-12  chore: workspace reorganization and build system consolid... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Rollenbasiertes Berechtigungssystem für CRUD-Operationen.
/// Kontrolliert Zugriff nach Benutzerrolle auf verschiedene Entitätstypen.
/// </summary>
public interface IRoleBasedPermissionService
{
    Task<bool> CanCreateAsync(string userId, EntityType entityType, CancellationToken cancellationToken = default);
    Task<bool> CanReadAsync(string userId, string entityId, EntityType entityType, CancellationToken cancellationToken = default);
    Task<bool> CanUpdateAsync(string userId, string entityId, EntityType entityType, CancellationToken cancellationToken = default);
    Task<bool> CanDeleteAsync(string userId, string entityId, EntityType entityType, CancellationToken cancellationToken = default);
    Task<List<string>> GetPermittedActionsAsync(string userId, EntityType entityType, CancellationToken cancellationToken = default);
    Task<UserRole> GetUserRoleAsync(string userId, CancellationToken cancellationToken = default);
}

public class RoleBasedPermissionService : IRoleBasedPermissionService
{
    private readonly IAuthenticationService _authService;

    private static readonly Dictionary<UserRole, Dictionary<EntityType, List<CrudOperation>>> RolePermissions =
        new()
        {
            {
                UserRole.Admin,
                new Dictionary<EntityType, List<CrudOperation>>
                {
                    { EntityType.Datei, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update, CrudOperation.Delete } },
                    { EntityType.Dokument, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update, CrudOperation.Delete } },
                    { EntityType.Vorgang, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update, CrudOperation.Delete } },
                    { EntityType.Akte, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update, CrudOperation.Delete } },
                    { EntityType.Ablage, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update, CrudOperation.Delete } }
                }
            },
            {
                UserRole.Manager,
                new Dictionary<EntityType, List<CrudOperation>>
                {
                    { EntityType.Datei, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update } },
                    { EntityType.Dokument, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update } },
                    { EntityType.Vorgang, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update } },
                    { EntityType.Akte, new List<CrudOperation> { CrudOperation.Read, CrudOperation.Update } },
                    { EntityType.Ablage, new List<CrudOperation> { CrudOperation.Read, CrudOperation.Update } }
                }
            },
            {
                UserRole.Editor,
                new Dictionary<EntityType, List<CrudOperation>>
                {
                    { EntityType.Datei, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update } },
                    { EntityType.Dokument, new List<CrudOperation> { CrudOperation.Create, CrudOperation.Read, CrudOperation.Update } },
                    { EntityType.Vorgang, new List<CrudOperation> { CrudOperation.Read, CrudOperation.Update } },
                    { EntityType.Akte, new List<CrudOperation> { CrudOperation.Read } },
                    { EntityType.Ablage, new List<CrudOperation> { CrudOperation.Read } }
                }
            },
            {
                UserRole.User,
                new Dictionary<EntityType, List<CrudOperation>>
                {
                    { EntityType.Datei, new List<CrudOperation> { CrudOperation.Read } },
                    { EntityType.Dokument, new List<CrudOperation> { CrudOperation.Read } },
                    { EntityType.Vorgang, new List<CrudOperation> { CrudOperation.Read } },
                    { EntityType.Akte, new List<CrudOperation> { CrudOperation.Read } },
                    { EntityType.Ablage, new List<CrudOperation> { CrudOperation.Read } }
                }
            },
            {
                UserRole.Viewer,
                new Dictionary<EntityType, List<CrudOperation>>
                {
                    { EntityType.Datei, new List<CrudOperation> { CrudOperation.Read } },
                    { EntityType.Dokument, new List<CrudOperation> { CrudOperation.Read } },
                    { EntityType.Vorgang, new List<CrudOperation> { CrudOperation.Read } },
                    { EntityType.Akte, new List<CrudOperation> { CrudOperation.Read } },
                    { EntityType.Ablage, new List<CrudOperation> { CrudOperation.Read } }
                }
            }
        };

    public RoleBasedPermissionService(IAuthenticationService authService)
    {
        _authService = authService;
    }

    public async Task<bool> CanCreateAsync(string userId, EntityType entityType, CancellationToken cancellationToken = default)
    {
        var role = await GetUserRoleAsync(userId, cancellationToken);
        return HasPermission(role, entityType, CrudOperation.Create);
    }

    public async Task<bool> CanReadAsync(string userId, string entityId, EntityType entityType, CancellationToken cancellationToken = default)
    {
        var role = await GetUserRoleAsync(userId, cancellationToken);
        return HasPermission(role, entityType, CrudOperation.Read);
    }

    public async Task<bool> CanUpdateAsync(string userId, string entityId, EntityType entityType, CancellationToken cancellationToken = default)
    {
        var role = await GetUserRoleAsync(userId, cancellationToken);
        return HasPermission(role, entityType, CrudOperation.Update);
    }

    public async Task<bool> CanDeleteAsync(string userId, string entityId, EntityType entityType, CancellationToken cancellationToken = default)
    {
        var role = await GetUserRoleAsync(userId, cancellationToken);
        return HasPermission(role, entityType, CrudOperation.Delete);
    }

    public async Task<List<string>> GetPermittedActionsAsync(string userId, EntityType entityType, CancellationToken cancellationToken = default)
    {
        var role = await GetUserRoleAsync(userId, cancellationToken);
        if (!RolePermissions.ContainsKey(role) || !RolePermissions[role].ContainsKey(entityType))
        {
            return new List<string>();
        }

        return RolePermissions[role][entityType]
            .Select(op => op.ToString())
            .ToList();
    }

    public async Task<UserRole> GetUserRoleAsync(string userId, CancellationToken cancellationToken = default)
    {
        var userInfo = await _authService.GetCurrentUserAsync(cancellationToken);
        if (userInfo?.Roles?.Length > 0)
        {
            var roleString = userInfo.Roles[0];
            if (Enum.TryParse<UserRole>(roleString, ignoreCase: true, out var role))
            {
                return role;
            }
        }

        return UserRole.Viewer;
    }

    private static bool HasPermission(UserRole role, EntityType entityType, CrudOperation operation)
    {
        return RolePermissions.ContainsKey(role) &&
               RolePermissions[role].ContainsKey(entityType) &&
               RolePermissions[role][entityType].Contains(operation);
    }
}

#region Enums

public enum UserRole
{
    Admin,
    Manager,
    Editor,
    User,
    Viewer
}

public enum EntityType
{
    Datei,
    Dokument,
    Vorgang,
    Akte,
    Ablage
}

public enum CrudOperation
{
    Create,
    Read,
    Update,
    Delete
}

#endregion
