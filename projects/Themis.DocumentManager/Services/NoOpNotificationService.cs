/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NoOpNotificationService.cs                         ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     87                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
