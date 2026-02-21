/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NoOpNotificationService.cs                         ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     94                                             ║
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
