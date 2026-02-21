/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NoOpNotificationService.cs                         ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     94                                             ║
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

using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Einfacher No-Op NotificationService, um ViewModels mit Benachrichtigungen zu versorgen,
/// ohne externe Abhängigkeiten zu erfordern. Alle Operationen sind in-memory und best effort.
/// </summary>
public class NoOpNotificationService : INotificationService
{
    public Task<Notification> CreateNotificationAsync(Notification notification)
    {
        return Task.FromResult(notification);
    }

    public Task<IEnumerable<Notification>> GetNotificationsByUserAsync(string userId, bool includeRead = false)
    {
        return Task.FromResult<IEnumerable<Notification>>(new List<Notification>());
    }

    public Task<IEnumerable<Notification>> GetUnreadNotificationsAsync(string userId)
    {
        return Task.FromResult<IEnumerable<Notification>>(new List<Notification>());
    }

    public Task<bool> MarkAsReadAsync(string notificationId)
    {
        return Task.FromResult(true);
    }

    public Task<bool> MarkAllAsReadAsync(string userId)
    {
        return Task.FromResult(true);
    }

    public Task<bool> DismissNotificationAsync(string notificationId)
    {
        return Task.FromResult(true);
    }

    public Task<int> GetUnreadCountAsync(string userId)
    {
        return Task.FromResult(0);
    }

    public Task ShowNotificationAsync(Notification notification, CancellationToken cancellationToken = default)
    {
        return Task.CompletedTask;
    }

    public Task SendDeadlineReminderAsync(string userId, Reminder reminder)
    {
        return Task.CompletedTask;
    }

    public Task SendTaskAssignedAsync(string userId, string processId, string assignedBy)
    {
        return Task.CompletedTask;
    }

    public Task SendCosigningRequestAsync(string userId, Cosigning cosigning)
    {
        return Task.CompletedTask;
    }
}
