/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Phase1ServiceImplementations2.cs                   ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     358                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
/// Phase 1 VIS-Features: Aktenplan & Notification Service Implementations
/// </summary>

#region Filing Plan Service Implementation

public class FilingPlanService : IFilingPlanService
{
    private readonly IThemisApiClient _apiClient;

    public FilingPlanService(IThemisApiClient apiClient)
    {
        _apiClient = apiClient;
    }

    public async Task<FilingPlan> CreateFilingPlanAsync(FilingPlan plan)
    {
        plan.Id = plan.Id == string.Empty ? Guid.NewGuid().ToString() : plan.Id;
        plan.CreatedAt = DateTime.UtcNow;
        plan.IsActive = true;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{plan.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(plan) }
        );

        return plan;
    }

    public async Task<FilingPlan?> GetFilingPlanByIdAsync(string id)
    {
        var urn = $"urn:themis:filingplan:{id}";
        return await _apiClient.GetAsync<FilingPlan>($"/entities/{urn}");
    }

    public async Task<IEnumerable<FilingPlan>> GetFilingPlansByAuthorityAsync(string authorityId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<FilingPlan>>(
            "/query/aql",
            new
            {
                query = "FOR plan IN filing_plans FILTER plan.authorityId == @authorityId RETURN plan",
                bindVars = new { authorityId }
            }
        );

        return response?.Results ?? Enumerable.Empty<FilingPlan>();
    }

    public async Task<FilingPlan?> GetActiveFilingPlanAsync(string authorityId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<FilingPlan>>(
            "/query/aql",
            new
            {
                query = @"FOR plan IN filing_plans 
                         FILTER plan.authorityId == @authorityId 
                         AND plan.isActive == true 
                         LIMIT 1 
                         RETURN plan",
                bindVars = new { authorityId }
            }
        );

        return response?.Results?.FirstOrDefault();
    }

    public async Task<FilingPlanNode> CreateNodeAsync(FilingPlanNode node)
    {
        node.Id = node.Id == string.Empty ? Guid.NewGuid().ToString() : node.Id;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{node.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(node) }
        );

        return node;
    }

    public async Task<FilingPlanNode?> GetNodeByIdAsync(string planId, string nodeId)
    {
        var urn = $"urn:themis:filingplan:{planId}:node:{nodeId}";
        return await _apiClient.GetAsync<FilingPlanNode>($"/entities/{urn}");
    }

    public async Task<IEnumerable<FilingPlanNode>> GetNodesByPlanAsync(string planId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<FilingPlanNode>>(
            "/query/aql",
            new
            {
                query = "FOR node IN filing_plan_nodes FILTER node.filingPlanId == @planId RETURN node",
                bindVars = new { planId }
            }
        );

        return response?.Results ?? Enumerable.Empty<FilingPlanNode>();
    }

    public async Task<IEnumerable<FilingPlanNode>> GetChildNodesAsync(string planId, string parentId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<FilingPlanNode>>(
            "/query/aql",
            new
            {
                query = @"FOR node IN filing_plan_nodes 
                         FILTER node.filingPlanId == @planId 
                         AND node.parentId == @parentId 
                         RETURN node",
                bindVars = new { planId, parentId }
            }
        );

        return response?.Results ?? Enumerable.Empty<FilingPlanNode>();
    }

    public async Task<string> GenerateFileNumberAsync(string planId, string nodeId, int year)
    {
        var node = await GetNodeByIdAsync(planId, nodeId);
        if (node == null) return "";

        // Get count of existing files for this node
        var response = await _apiClient.PostAsync<object, CountResponse>(
            "/query/aql",
            new
            {
                query = @"FOR file IN administrative_files 
                         FILTER CONTAINS(file.fileNumber, @nodeCode) 
                         AND CONTAINS(file.fileNumber, @year) 
                         COLLECT WITH COUNT INTO fileCount 
                         RETURN fileCount",
                bindVars = new { nodeCode = node.Code, year = year.ToString() }
            }
        );

        var count = response?.Result ?? 0;
        var sequenceNumber = count + 1;

        // Format: {NodeCode} - {SequenceNumber}/{Year}
        // Example: IV C 5 - 123/2024
        return $"{node.Code} - {sequenceNumber:D3}/{year}";
    }

    private class QueryResponse<T>
    {
        public List<T> Results { get; set; } = new();
    }

    private class CountResponse
    {
        public int Result { get; set; }
    }
}

#endregion

#region Notification Service Implementation

public class NotificationService : INotificationService
{
    private readonly IThemisApiClient _apiClient;

    public NotificationService(IThemisApiClient apiClient)
    {
        _apiClient = apiClient;
    }

    public async Task<Notification> CreateNotificationAsync(Notification notification)
    {
        notification.Id = notification.Id == string.Empty ? Guid.NewGuid().ToString() : notification.Id;
        notification.CreatedAt = DateTime.UtcNow;
        notification.IsRead = false;
        notification.IsDismissed = false;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{notification.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(notification) }
        );

        return notification;
    }

    public async Task<IEnumerable<Notification>> GetNotificationsByUserAsync(string userId, bool includeRead = false)
    {
        var query = "FOR notif IN notifications FILTER notif.recipientId == @userId";
        
        if (!includeRead)
            query += " AND notif.isRead == false";

        query += " AND notif.isDismissed == false SORT notif.createdAt DESC RETURN notif";

        var response = await _apiClient.PostAsync<object, QueryResponse<Notification>>(
            "/query/aql",
            new
            {
                query,
                bindVars = new { userId }
            }
        );

        return response?.Results ?? Enumerable.Empty<Notification>();
    }

    public async Task<IEnumerable<Notification>> GetUnreadNotificationsAsync(string userId)
    {
        return await GetNotificationsByUserAsync(userId, includeRead: false);
    }

    public async Task<bool> MarkAsReadAsync(string notificationId)
    {
        var urn = $"urn:themis:notification:{notificationId}";
        var notification = await _apiClient.GetAsync<Notification>($"/entities/{urn}");
        
        if (notification == null) return false;

        notification.IsRead = true;
        notification.ReadAt = DateTime.UtcNow;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(notification) }
        );

        return true;
    }

    public async Task<bool> MarkAllAsReadAsync(string userId)
    {
        var notifications = await GetUnreadNotificationsAsync(userId);
        
        foreach (var notification in notifications)
        {
            await MarkAsReadAsync(notification.Id);
        }

        return true;
    }

    public async Task<bool> DismissNotificationAsync(string notificationId)
    {
        var urn = $"urn:themis:notification:{notificationId}";
        var notification = await _apiClient.GetAsync<Notification>($"/entities/{urn}");
        
        if (notification == null) return false;

        notification.IsDismissed = true;
        notification.DismissedAt = DateTime.UtcNow;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(notification) }
        );

        return true;
    }

    public async Task<int> GetUnreadCountAsync(string userId)
    {
        var notifications = await GetUnreadNotificationsAsync(userId);
        return notifications.Count();
    }

    public async Task ShowNotificationAsync(Notification notification, CancellationToken cancellationToken = default)
    {
        if (notification != null)
        {
            await CreateNotificationAsync(notification);
        }
    }

    #region Helper Methods for Automatic Notifications

    public async Task SendDeadlineReminderAsync(string userId, Reminder reminder)
    {
        await CreateNotificationAsync(new Notification
        {
            RecipientId = userId,
            Type = NotificationType.DeadlineReminder,
            Priority = NotificationPriority.High,
            Title = "Frist-Erinnerung",
            Message = $"{reminder.Subject} fällig am {reminder.DueDate:dd.MM.yyyy}",
            ReminderId = reminder.Id,
            ProcessId = reminder.ProcessId,
            FileId = reminder.FileId
        });
    }

    public async Task SendTaskAssignedAsync(string userId, string processId, string assignedBy)
    {
        await CreateNotificationAsync(new Notification
        {
            RecipientId = userId,
            Type = NotificationType.TaskAssigned,
            Priority = NotificationPriority.Normal,
            Title = "Neue Aufgabe zugewiesen",
            Message = $"Sie wurden von {assignedBy} einer neuen Aufgabe zugewiesen.",
            ProcessId = processId
        });
    }

    public async Task SendCosigningRequestAsync(string userId, Cosigning cosigning)
    {
        await CreateNotificationAsync(new Notification
        {
            RecipientId = userId,
            Type = NotificationType.CosigningRequest,
            Priority = NotificationPriority.High,
            Title = "Mitzeichnung erforderlich",
            Message = $"Bitte zeichnen Sie mit: {cosigning.Subject}",
            ProcessId = cosigning.ProcessId,
            DocumentId = cosigning.DocumentId
        });
    }

    #endregion

    private class QueryResponse<T>
    {
        public List<T> Results { get; set; } = new();
    }
}

#endregion
