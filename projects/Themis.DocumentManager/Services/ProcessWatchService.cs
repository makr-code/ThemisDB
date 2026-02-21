/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ProcessWatchService.cs                             ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:40:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     901                                            ║
    • Open Issues:     TODOs: 2, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 60d127110  2025-12-09  feat: Add comprehensive test report for ThemisDB Document... ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service for managing process watches (Überwachen-Aufgabe)
/// Allows users to monitor processes without intervention and receive notifications on status changes
/// </summary>
public interface IProcessWatchService
{
    /// <summary>
    /// Create a new process watch
    /// </summary>
    Task<ProcessWatch> CreateProcessWatchAsync(
        string processId,
        string watchedBy,
        string reason,
        ProcessWatchNotificationSettings? settings = null,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get a specific process watch
    /// </summary>
    Task<ProcessWatch?> GetProcessWatchAsync(
        string watchId,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get all watches for a user
    /// </summary>
    Task<List<ProcessWatch>> GetWatchesByUserAsync(
        string userId,
        bool activeOnly = true,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get all watches for a process
    /// </summary>
    Task<List<ProcessWatch>> GetWatchesByProcessAsync(
        string processId,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Update watch notification settings
    /// </summary>
    Task<ProcessWatch> UpdateNotificationSettingsAsync(
        string watchId,
        ProcessWatchNotificationSettings settings,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Add event types to watch
    /// </summary>
    Task AddWatchedEventTypesAsync(
        string watchId,
        List<ProcessEventType> eventTypes,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Remove event types from watch
    /// </summary>
    Task RemoveWatchedEventTypesAsync(
        string watchId,
        List<ProcessEventType> eventTypes,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// End a process watch
    /// </summary>
    Task EndProcessWatchAsync(
        string watchId,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Reactivate an ended watch
    /// </summary>
    Task ReactivateProcessWatchAsync(
        string watchId,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Delete a process watch
    /// </summary>
    Task DeleteProcessWatchAsync(
        string watchId,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get unread notifications for a watch
    /// </summary>
    Task<List<ProcessWatchNotification>> GetUnreadNotificationsAsync(
        string watchId,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Mark notification as read
    /// </summary>
    Task MarkNotificationAsReadAsync(
        string notificationId,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Mark all notifications as read for a watch
    /// </summary>
    Task MarkAllNotificationsAsReadAsync(
        string watchId,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get watch summary with statistics
    /// </summary>
    Task<ProcessWatchSummary> GetWatchSummaryAsync(
        string watchId,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Process an event and generate notifications for active watches
    /// </summary>
    Task ProcessEventForWatchesAsync(
        string processId,
        ProcessEventType eventType,
        string eventDescription,
        string actor,
        Dictionary<string, object>? eventData = null,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Create bulk watches for multiple processes
    /// </summary>
    Task<List<ProcessWatch>> CreateBulkWatchesAsync(
        BulkProcessWatch bulkWatch,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Search watches with filters
    /// </summary>
    Task<List<ProcessWatch>> SearchWatchesAsync(
        ProcessWatchFilter filter,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Send pending notifications (for digest/batch mode)
    /// </summary>
    Task SendPendingNotificationsAsync(
        CancellationToken cancellationToken = default);
}

public class ProcessWatchService : IProcessWatchService
{
    private readonly IThemisApiClient _apiClient;
    private readonly INotificationService _notificationService;
    private readonly IProcessTimelineService _timelineService;
    private readonly IAdministrativeStructureService _adminService;

    private const string WatchCollectionName = "process_watches";
    private const string WatchNotificationCollectionName = "process_watch_notifications";

    public ProcessWatchService(
        IThemisApiClient apiClient,
        INotificationService notificationService,
        IProcessTimelineService timelineService,
        IAdministrativeStructureService adminService)
    {
        _apiClient = apiClient;
        _notificationService = notificationService;
        _timelineService = timelineService;
        _adminService = adminService;
    }

    public async Task<ProcessWatch> CreateProcessWatchAsync(
        string processId,
        string watchedBy,
        string reason,
        ProcessWatchNotificationSettings? settings = null,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(processId);
        ArgumentNullException.ThrowIfNull(watchedBy);

        // Get process details
        var process = await _adminService.GetProcessAsync(processId);
        if (process == null)
        {
            throw new ArgumentException($"Process {processId} not found", nameof(processId));
        }

        var watch = new ProcessWatch
        {
            Id = Guid.NewGuid().ToString(),
            URN = $"urn:themis:process-watch:{Guid.NewGuid()}",
            ProcessId = processId,
            ProcessNumber = process.ProcessNumber,
            WatchedBy = watchedBy,
            Reason = reason,
            NotificationSettings = settings ?? new ProcessWatchNotificationSettings(),
            CreatedAt = DateTime.UtcNow,
            IsActive = true
        };

        // Store in ThemisDB
        var query = @"
            INSERT {
                _key: @key,
                _urn: @urn,
                processId: @processId,
                processNumber: @processNumber,
                watchedBy: @watchedBy,
                reason: @reason,
                createdAt: @createdAt,
                isActive: @isActive,
                notificationSettings: @settings,
                watchAllEvents: true,
                totalNotifications: 0
            } INTO @@collection
            RETURN NEW
        ";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = WatchCollectionName,
            key = watch.Id,
            urn = watch.URN,
            processId = watch.ProcessId,
            processNumber = watch.ProcessNumber,
            watchedBy = watch.WatchedBy,
            reason = watch.Reason,
            createdAt = watch.CreatedAt,
            isActive = watch.IsActive,
            settings = watch.NotificationSettings
        }, cancellationToken);

        // Create timeline event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            ProcessId = processId,
            EventType = ProcessEventType.ProcessStarted, // Or create new WatchStarted event type
            Description = $"Überwachung gestartet von {watchedBy}: {reason}",
            Actor = watchedBy,
            Timestamp = DateTime.UtcNow
        }, cancellationToken);

        // Send confirmation notification
        await _notificationService.ShowNotificationAsync(new Notification
        {
            Type = NotificationType.Info,
            Title = "Überwachung aktiviert",
            Message = $"Sie überwachen jetzt Prozess {process.ProcessNumber}",
            RecipientUserId = watchedBy
        });

        return watch;
    }

    public async Task<ProcessWatch?> GetProcessWatchAsync(
        string watchId,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);

        var query = @"
            FOR watch IN @@collection
            FILTER watch._key == @watchId
            RETURN watch
        ";

        var result = await _apiClient.ExecuteAqlAsync<ProcessWatch>(query, new
        {
            collection = WatchCollectionName,
            watchId
        }, cancellationToken);

        return result.FirstOrDefault();
    }

    public async Task<List<ProcessWatch>> GetWatchesByUserAsync(
        string userId,
        bool activeOnly = true,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(userId);

        var query = activeOnly
            ? @"
                FOR watch IN @@collection
                FILTER watch.watchedBy == @userId AND watch.isActive == true
                SORT watch.createdAt DESC
                RETURN watch
              "
            : @"
                FOR watch IN @@collection
                FILTER watch.watchedBy == @userId
                SORT watch.createdAt DESC
                RETURN watch
              ";

        var result = await _apiClient.ExecuteAqlAsync<ProcessWatch>(query, new
        {
            collection = WatchCollectionName,
            userId
        }, cancellationToken);

        return result.ToList();
    }

    public async Task<List<ProcessWatch>> GetWatchesByProcessAsync(
        string processId,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(processId);

        var query = @"
            FOR watch IN @@collection
            FILTER watch.processId == @processId AND watch.isActive == true
            RETURN watch
        ";

        var result = await _apiClient.ExecuteAqlAsync<ProcessWatch>(query, new
        {
            collection = WatchCollectionName,
            processId
        }, cancellationToken);

        return result.ToList();
    }

    public async Task<ProcessWatch> UpdateNotificationSettingsAsync(
        string watchId,
        ProcessWatchNotificationSettings settings,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);
        ArgumentNullException.ThrowIfNull(settings);

        var query = @"
            UPDATE @watchId WITH {
                notificationSettings: @settings
            } IN @@collection
            RETURN NEW
        ";

        var result = await _apiClient.ExecuteAqlAsync<ProcessWatch>(query, new
        {
            collection = WatchCollectionName,
            watchId,
            settings
        }, cancellationToken);

        return result.First();
    }

    public async Task AddWatchedEventTypesAsync(
        string watchId,
        List<ProcessEventType> eventTypes,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);
        ArgumentNullException.ThrowIfNull(eventTypes);

        var watch = await GetProcessWatchAsync(watchId, cancellationToken);
        if (watch == null) return;

        watch.WatchedEventTypes.AddRange(eventTypes);
        watch.WatchAllEvents = false;

        var query = @"
            UPDATE @watchId WITH {
                watchedEventTypes: @eventTypes,
                watchAllEvents: false
            } IN @@collection
        ";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = WatchCollectionName,
            watchId,
            eventTypes = watch.WatchedEventTypes.Distinct().ToList()
        }, cancellationToken);
    }

    public async Task RemoveWatchedEventTypesAsync(
        string watchId,
        List<ProcessEventType> eventTypes,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);
        ArgumentNullException.ThrowIfNull(eventTypes);

        var watch = await GetProcessWatchAsync(watchId, cancellationToken);
        if (watch == null) return;

        watch.WatchedEventTypes = watch.WatchedEventTypes
            .Except(eventTypes)
            .ToList();

        var query = @"
            UPDATE @watchId WITH {
                watchedEventTypes: @eventTypes
            } IN @@collection
        ";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = WatchCollectionName,
            watchId,
            eventTypes = watch.WatchedEventTypes
        }, cancellationToken);
    }

    public async Task EndProcessWatchAsync(
        string watchId,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);

        var query = @"
            UPDATE @watchId WITH {
                isActive: false,
                endedAt: @endedAt
            } IN @@collection
            RETURN NEW
        ";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = WatchCollectionName,
            watchId,
            endedAt = DateTime.UtcNow
        }, cancellationToken);
    }

    public async Task ReactivateProcessWatchAsync(
        string watchId,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);

        var query = @"
            UPDATE @watchId WITH {
                isActive: true,
                endedAt: null
            } IN @@collection
        ";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = WatchCollectionName,
            watchId
        }, cancellationToken);
    }

    public async Task DeleteProcessWatchAsync(
        string watchId,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);

        var query = @"REMOVE @watchId IN @@collection";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = WatchCollectionName,
            watchId
        }, cancellationToken);
    }

    public async Task<List<ProcessWatchNotification>> GetUnreadNotificationsAsync(
        string watchId,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);

        var query = @"
            FOR notification IN @@collection
            FILTER notification.processWatchId == @watchId AND notification.isRead == false
            SORT notification.timestamp DESC
            RETURN notification
        ";

        var result = await _apiClient.ExecuteAqlAsync<ProcessWatchNotification>(query, new
        {
            collection = WatchNotificationCollectionName,
            watchId
        }, cancellationToken);

        return result.ToList();
    }

    public async Task MarkNotificationAsReadAsync(
        string notificationId,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(notificationId);

        var query = @"
            UPDATE @notificationId WITH {
                isRead: true,
                readAt: @readAt
            } IN @@collection
        ";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = WatchNotificationCollectionName,
            notificationId,
            readAt = DateTime.UtcNow
        }, cancellationToken);
    }

    public async Task MarkAllNotificationsAsReadAsync(
        string watchId,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);

        var query = @"
            FOR notification IN @@collection
            FILTER notification.processWatchId == @watchId AND notification.isRead == false
            UPDATE notification WITH {
                isRead: true,
                readAt: @readAt
            } IN @@collection
        ";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = WatchNotificationCollectionName,
            watchId,
            readAt = DateTime.UtcNow
        }, cancellationToken);
    }

    public async Task<ProcessWatchSummary> GetWatchSummaryAsync(
        string watchId,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(watchId);

        var watch = await GetProcessWatchAsync(watchId, cancellationToken);
        if (watch == null)
        {
            throw new ArgumentException($"Watch {watchId} not found", nameof(watchId));
        }

        var unreadCount = (await GetUnreadNotificationsAsync(watchId, cancellationToken)).Count;

        var summary = new ProcessWatchSummary
        {
            ProcessWatchId = watch.Id,
            ProcessNumber = watch.ProcessNumber,
            WatchStartedAt = watch.CreatedAt,
            UnreadNotifications = unreadCount,
            TotalEvents = watch.TotalNotifications,
            LastActivityAt = watch.LastNotificationAt
        };

        return summary;
    }

    public async Task ProcessEventForWatchesAsync(
        string processId,
        ProcessEventType eventType,
        string eventDescription,
        string actor,
        Dictionary<string, object>? eventData = null,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(processId);

        // Get all active watches for this process
        var watches = await GetWatchesByProcessAsync(processId, cancellationToken);

        foreach (var watch in watches)
        {
            // Check if this event type is watched
            if (!watch.WatchAllEvents && !watch.WatchedEventTypes.Contains(eventType))
            {
                continue;
            }

            // Check notification settings filters
            if (!ShouldNotify(watch, eventType))
            {
                continue;
            }

            // Create notification
            var notification = new ProcessWatchNotification
            {
                ProcessWatchId = watch.Id,
                ProcessId = processId,
                EventType = eventType,
                EventDescription = eventDescription,
                Actor = actor,
                Timestamp = DateTime.UtcNow,
                Priority = DeterminePriority(watch, eventType),
                EventData = eventData ?? new()
            };

            // Store notification
            await StoreNotificationAsync(notification, cancellationToken);

            // Send notification based on settings
            await SendNotificationAsync(watch, notification, cancellationToken);

            // Update watch statistics
            await UpdateWatchStatisticsAsync(watch.Id, cancellationToken);
        }
    }

    public async Task<List<ProcessWatch>> CreateBulkWatchesAsync(
        BulkProcessWatch bulkWatch,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(bulkWatch);

        var watches = new List<ProcessWatch>();

        foreach (var processId in bulkWatch.ProcessIds)
        {
            try
            {
                var watch = await CreateProcessWatchAsync(
                    processId,
                    bulkWatch.WatchedBy,
                    bulkWatch.Reason,
                    bulkWatch.NotificationSettings,
                    cancellationToken);

                if (bulkWatch.WatchedEventTypes.Any())
                {
                    await AddWatchedEventTypesAsync(watch.Id, bulkWatch.WatchedEventTypes, cancellationToken);
                }

                watches.Add(watch);
            }
            catch (Exception)
            {
                // Continue with other processes if one fails
                continue;
            }
        }

        return watches;
    }

    public async Task<List<ProcessWatch>> SearchWatchesAsync(
        ProcessWatchFilter filter,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(filter);

        var conditions = new List<string>();
        var bindVars = new Dictionary<string, object>
        {
            ["collection"] = WatchCollectionName
        };

        if (!string.IsNullOrEmpty(filter.WatchedBy))
        {
            conditions.Add("watch.watchedBy == @watchedBy");
            bindVars["watchedBy"] = filter.WatchedBy;
        }

        if (filter.IsActive.HasValue)
        {
            conditions.Add("watch.isActive == @isActive");
            bindVars["isActive"] = filter.IsActive.Value;
        }

        if (filter.CreatedAfter.HasValue)
        {
            conditions.Add("watch.createdAt >= @createdAfter");
            bindVars["createdAfter"] = filter.CreatedAfter.Value;
        }

        if (filter.CreatedBefore.HasValue)
        {
            conditions.Add("watch.createdAt <= @createdBefore");
            bindVars["createdBefore"] = filter.CreatedBefore.Value;
        }

        var filterClause = conditions.Any() 
            ? $"FILTER {string.Join(" AND ", conditions)}"
            : "";

        var query = $@"
            FOR watch IN @@collection
            {filterClause}
            SORT watch.createdAt DESC
            RETURN watch
        ";

        var result = await _apiClient.ExecuteAqlAsync<ProcessWatch>(query, bindVars, cancellationToken);
        return result.ToList();
    }

    public async Task SendPendingNotificationsAsync(CancellationToken cancellationToken = default)
    {
        // Get all notifications that haven't been sent yet
        var query = @"
            FOR notification IN @@collection
            FILTER notification.wasSent == false
            RETURN notification
        ";

        var pendingNotifications = await _apiClient.ExecuteAqlAsync<ProcessWatchNotification>(query, new
        {
            collection = WatchNotificationCollectionName
        }, cancellationToken);

        foreach (var notification in pendingNotifications)
        {
            var watch = await GetProcessWatchAsync(notification.ProcessWatchId, cancellationToken);
            if (watch != null && watch.IsActive)
            {
                await SendNotificationAsync(watch, notification, cancellationToken);
            }
        }
    }

    // Private helper methods

    private async Task StoreNotificationAsync(ProcessWatchNotification notification, CancellationToken cancellationToken)
    {
        var query = @"
            INSERT @notification INTO @@collection
        ";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = WatchNotificationCollectionName,
            notification
        }, cancellationToken);
    }

    private async Task SendNotificationAsync(ProcessWatch watch, ProcessWatchNotification notification, CancellationToken cancellationToken)
    {
        var settings = watch.NotificationSettings;

        // Check quiet hours
        if (settings.EnableQuietHours && IsQuietHours(settings))
        {
            if (settings.QueueDuringQuietHours)
            {
                return; // Will be sent later
            }
        }

        var channels = new List<string>();

        // In-app notification
        if (settings.EnableInAppNotification)
        {
            await _notificationService.ShowNotificationAsync(new Notification
            {
                Type = MapPriorityToNotificationType(notification.Priority),
                Title = $"Prozess-Überwachung: {watch.ProcessNumber}",
                Message = notification.EventDescription,
                RecipientUserId = watch.WatchedBy,
                Metadata = new Dictionary<string, object>
                {
                    ["processId"] = watch.ProcessId,
                    ["watchId"] = watch.Id,
                    ["eventType"] = notification.EventType.ToString()
                }
            });
            channels.Add("InApp");
        }

        // Email notification (would integrate with email service)
        if (settings.EnableEmail && !string.IsNullOrEmpty(settings.EmailAddress))
        {
            // TODO: Implement email sending
            channels.Add("Email");
        }

        // Desktop notification (would use Windows Toast)
        if (settings.EnableDesktopNotification)
        {
            // TODO: Implement desktop notifications
            channels.Add("Desktop");
        }

        // Mark as sent
        var updateQuery = @"
            UPDATE @notificationId WITH {
                wasSent: true,
                sentAt: @sentAt,
                sentChannels: @channels
            } IN @@collection
        ";

        await _apiClient.ExecuteAqlAsync<object>(updateQuery, new
        {
            collection = WatchNotificationCollectionName,
            notificationId = notification.Id,
            sentAt = DateTime.UtcNow,
            channels
        }, cancellationToken);
    }

    private async Task UpdateWatchStatisticsAsync(string watchId, CancellationToken cancellationToken)
    {
        var query = @"
            UPDATE @watchId WITH {
                totalNotifications: (SELECT VALUE COUNT(1) FROM @@notifCollection FILTER n.processWatchId == @watchId)[0],
                lastNotificationAt: @now
            } IN @@watchCollection
        ";

        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            watchCollection = WatchCollectionName,
            notifCollection = WatchNotificationCollectionName,
            watchId,
            now = DateTime.UtcNow
        }, cancellationToken);
    }

    private bool ShouldNotify(ProcessWatch watch, ProcessEventType eventType)
    {
        // Check specific notification flags
        return eventType.ToString().ToLowerInvariant() switch
        {
            var e when e.Contains("status") => watch.NotifyOnStatusChange,
            var e when e.Contains("document") => watch.NotifyOnDocumentAdded,
            var e when e.Contains("comment") => watch.NotifyOnCommentAdded,
            var e when e.Contains("approved") => watch.NotifyOnApproval,
            var e when e.Contains("rejected") => watch.NotifyOnRejection,
            var e when e.Contains("completed") => watch.NotifyOnCompletion,
            _ => true
        };
    }

    private ProcessWatchNotificationPriority DeterminePriority(ProcessWatch watch, ProcessEventType eventType)
    {
        // Determine notification priority based on event type
        return eventType.ToString().ToLowerInvariant() switch
        {
            var e when e.Contains("rejected") || e.Contains("cancelled") => ProcessWatchNotificationPriority.High,
            var e when e.Contains("deadline") || e.Contains("overdue") => ProcessWatchNotificationPriority.Urgent,
            var e when e.Contains("approved") || e.Contains("completed") => ProcessWatchNotificationPriority.Normal,
            _ => ProcessWatchNotificationPriority.Normal
        };
    }

    private bool IsQuietHours(ProcessWatchNotificationSettings settings)
    {
        var now = DateTime.Now.TimeOfDay;
        if (settings.QuietHoursStart < settings.QuietHoursEnd)
        {
            return now >= settings.QuietHoursStart && now <= settings.QuietHoursEnd;
        }
        else
        {
            // Overnight quiet hours
            return now >= settings.QuietHoursStart || now <= settings.QuietHoursEnd;
        }
    }

    private NotificationType MapPriorityToNotificationType(ProcessWatchNotificationPriority priority)
    {
        return priority switch
        {
            ProcessWatchNotificationPriority.Urgent => NotificationType.Error,
            ProcessWatchNotificationPriority.High => NotificationType.Warning,
            _ => NotificationType.Info
        };
    }
}
