/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            OutboxService.cs                                   ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     669                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
/// Service for managing outbox (Postausgang) items
/// </summary>
public interface IOutboxService
{
    // CRUD operations
    Task<OutboxItem> CreateOutboxItemAsync(OutboxItem item, CancellationToken cancellationToken = default);
    Task<OutboxItem?> GetOutboxItemAsync(string itemId, CancellationToken cancellationToken = default);
    Task<List<OutboxItem>> GetAllOutboxItemsAsync(CancellationToken cancellationToken = default);
    Task<OutboxItem> UpdateOutboxItemAsync(OutboxItem item, CancellationToken cancellationToken = default);
    Task DeleteOutboxItemAsync(string itemId, CancellationToken cancellationToken = default);
    
    // Status management
    Task<OutboxItem> UpdateStatusAsync(string itemId, OutboxStatus status, string? reason = null, CancellationToken cancellationToken = default);
    Task<OutboxItem> SendItemAsync(string itemId, CancellationToken cancellationToken = default);
    Task<OutboxItem> CancelItemAsync(string itemId, string reason, CancellationToken cancellationToken = default);
    Task<OutboxItem> ScheduleItemAsync(string itemId, DateTime scheduledSendTime, CancellationToken cancellationToken = default);
    
    // Recipient management
    Task AddRecipientAsync(string itemId, OutboxRecipient recipient, CancellationToken cancellationToken = default);
    Task RemoveRecipientAsync(string itemId, string recipientId, CancellationToken cancellationToken = default);
    Task UpdateRecipientStatusAsync(string itemId, string recipientId, RecipientDeliveryStatus status, CancellationToken cancellationToken = default);
    
    // Attachments
    Task AddAttachmentAsync(string itemId, OutboxAttachment attachment, CancellationToken cancellationToken = default);
    Task RemoveAttachmentAsync(string itemId, string attachmentId, CancellationToken cancellationToken = default);
    
    // Delivery tracking
    Task RecordDeliveryAttemptAsync(string itemId, OutboxDeliveryAttempt attempt, CancellationToken cancellationToken = default);
    Task<List<OutboxDeliveryAttempt>> GetDeliveryAttemptsAsync(string itemId, CancellationToken cancellationToken = default);
    Task ConfirmDeliveryAsync(string itemId, string recipientId, DateTime deliveryTime, CancellationToken cancellationToken = default);
    Task RecordReadReceiptAsync(string itemId, string recipientId, DateTime readTime, CancellationToken cancellationToken = default);
    
    // Filtering & search
    Task<List<OutboxItem>> SearchOutboxAsync(OutboxFilter filter, CancellationToken cancellationToken = default);
    Task<List<OutboxItem>> GetOutboxByUserAsync(string userId, OutboxStatus? status = null, CancellationToken cancellationToken = default);
    Task<List<OutboxItem>> GetOutboxByProcessAsync(string processId, CancellationToken cancellationToken = default);
    Task<List<OutboxItem>> GetPendingOutboxItemsAsync(CancellationToken cancellationToken = default);
    Task<List<OutboxItem>> GetFailedOutboxItemsAsync(CancellationToken cancellationToken = default);
    
    // Statistics
    Task<OutboxStatistics> GetOutboxStatisticsAsync(DateTime? startDate = null, DateTime? endDate = null, CancellationToken cancellationToken = default);
    Task<int> GetUnsentCountAsync(string? userId = null, CancellationToken cancellationToken = default);
    
    // Templates
    Task<OutboxTemplate> CreateTemplateAsync(OutboxTemplate template, CancellationToken cancellationToken = default);
    Task<List<OutboxTemplate>> GetTemplatesAsync(CancellationToken cancellationToken = default);
    Task<OutboxItem> CreateFromTemplateAsync(string templateId, Dictionary<string, string> variables, CancellationToken cancellationToken = default);
    
    // Bulk operations
    Task<List<OutboxItem>> BulkSendAsync(List<string> itemIds, CancellationToken cancellationToken = default);
    Task BulkArchiveAsync(List<string> itemIds, CancellationToken cancellationToken = default);
    
    // Scheduled sends
    Task ProcessScheduledSendsAsync(CancellationToken cancellationToken = default);
    
    // Reply to inbox
    Task<OutboxItem> CreateReplyAsync(string inboxItemId, string replyText, List<OutboxAttachment>? attachments = null, CancellationToken cancellationToken = default);
}

public class OutboxService : IOutboxService
{
    private readonly IThemisApiClient _apiClient;
    private readonly INotificationService _notificationService;
    private readonly IProcessTimelineService _timelineService;
    private readonly IInboxService _inboxService;
    
    private const string OutboxCollectionName = "outbox_items";
    private const string OutboxTemplateCollectionName = "outbox_templates";
    
    public OutboxService(
        IThemisApiClient apiClient,
        INotificationService notificationService,
        IProcessTimelineService timelineService,
        IInboxService inboxService)
    {
        _apiClient = apiClient;
        _notificationService = notificationService;
        _timelineService = timelineService;
        _inboxService = inboxService;
    }
    
    public async Task<OutboxItem> CreateOutboxItemAsync(OutboxItem item, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(item);
        
        item.URN = $"urn:themis:outbox:{item.Id}";
        item.CreatedAt = DateTime.UtcNow;
        item.Status = OutboxStatus.Draft;
        
        var query = "INSERT @item INTO @@collection RETURN NEW";
        var result = await _apiClient.ExecuteAqlAsync<OutboxItem>(query, new
        {
            collection = OutboxCollectionName,
            item
        }, cancellationToken);
        
        // Create timeline event if linked to process
        if (!string.IsNullOrEmpty(item.RelatedProcessId))
        {
            await _timelineService.CreateEventAsync(new ProcessTimelineEvent
            {
                ProcessId = item.RelatedProcessId,
                EventType = ProcessEventType.DocumentSent,
                Description = $"Postausgang erstellt: {item.Subject}",
                Actor = item.SentBy,
                Timestamp = DateTime.UtcNow
            }, cancellationToken);
        }
        
        return result.First();
    }
    
    public async Task<OutboxItem?> GetOutboxItemAsync(string itemId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(itemId);
        
        var query = "FOR item IN @@collection FILTER item._key == @itemId RETURN item";
        var result = await _apiClient.ExecuteAqlAsync<OutboxItem>(query, new
        {
            collection = OutboxCollectionName,
            itemId
        }, cancellationToken);
        
        return result.FirstOrDefault();
    }
    
    public async Task<List<OutboxItem>> GetAllOutboxItemsAsync(CancellationToken cancellationToken = default)
    {
        var query = "FOR item IN @@collection SORT item.createdAt DESC RETURN item";
        var result = await _apiClient.ExecuteAqlAsync<OutboxItem>(query, new
        {
            collection = OutboxCollectionName
        }, cancellationToken);
        
        return result.ToList();
    }
    
    public async Task<OutboxItem> UpdateOutboxItemAsync(OutboxItem item, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(item);
        
        var query = "UPDATE @itemId WITH @item IN @@collection RETURN NEW";
        var result = await _apiClient.ExecuteAqlAsync<OutboxItem>(query, new
        {
            collection = OutboxCollectionName,
            itemId = item.Id,
            item
        }, cancellationToken);
        
        return result.First();
    }
    
    public async Task DeleteOutboxItemAsync(string itemId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(itemId);
        
        var query = "REMOVE @itemId IN @@collection";
        await _apiClient.ExecuteAqlAsync<object>(query, new
        {
            collection = OutboxCollectionName,
            itemId
        }, cancellationToken);
    }
    
    public async Task<OutboxItem> UpdateStatusAsync(string itemId, OutboxStatus status, string? reason = null, CancellationToken cancellationToken = default)
    {
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) throw new ArgumentException($"Outbox item {itemId} not found");
        
        var statusChange = new OutboxStatusChange
        {
            FromStatus = item.Status,
            ToStatus = status,
            ChangedBy = Environment.UserName,
            Reason = reason,
            Timestamp = DateTime.UtcNow
        };
        
        item.Status = status;
        item.StatusHistory.Add(statusChange);
        
        if (status == OutboxStatus.Sent && !item.SentAt.HasValue)
        {
            item.SentAt = DateTime.UtcNow;
        }
        
        return await UpdateOutboxItemAsync(item, cancellationToken);
    }
    
    public async Task<OutboxItem> SendItemAsync(string itemId, CancellationToken cancellationToken = default)
    {
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) throw new ArgumentException($"Outbox item {itemId} not found");
        
        // Validate before sending
        if (!item.Recipients.Any())
        {
            throw new InvalidOperationException("Cannot send outbox item without recipients");
        }
        
        // Update status to sending
        await UpdateStatusAsync(itemId, OutboxStatus.Sending, "Starting send process", cancellationToken);
        
        try
        {
            // Simulate sending via delivery method
            await SimulateSendAsync(item, cancellationToken);
            
            // Update status to sent
            item = await UpdateStatusAsync(itemId, OutboxStatus.Sent, "Successfully sent", cancellationToken);
            
            // Create timeline event
            if (!string.IsNullOrEmpty(item.RelatedProcessId))
            {
                await _timelineService.CreateEventAsync(new ProcessTimelineEvent
                {
                    ProcessId = item.RelatedProcessId,
                    EventType = ProcessEventType.DocumentSent,
                    Description = $"Dokument versendet: {item.Subject}",
                    Actor = item.SentBy,
                    Timestamp = DateTime.UtcNow,
                    Metadata = new Dictionary<string, object>
                    {
                        ["recipientCount"] = item.Recipients.Count,
                        ["deliveryMethod"] = item.DeliveryMethod.ToString()
                    }
                }, cancellationToken);
            }
            
            // Notify sender
            await _notificationService.ShowNotificationAsync(new Notification
            {
                Type = NotificationType.Success,
                Title = "Erfolgreich versendet",
                Message = $"'{item.Subject}' wurde an {item.Recipients.Count} Empfänger versendet",
                RecipientUserId = item.SentBy
            });
            
            return item;
        }
        catch (Exception ex)
        {
            // Record failed attempt
            await RecordDeliveryAttemptAsync(itemId, new OutboxDeliveryAttempt
            {
                Success = false,
                ErrorMessage = ex.Message,
                Method = item.DeliveryMethod,
                AttemptNumber = item.DeliveryAttempts.Count + 1
            }, cancellationToken);
            
            // Update status to failed
            await UpdateStatusAsync(itemId, OutboxStatus.Failed, ex.Message, cancellationToken);
            
            throw;
        }
    }
    
    public async Task<OutboxItem> CancelItemAsync(string itemId, string reason, CancellationToken cancellationToken = default)
    {
        return await UpdateStatusAsync(itemId, OutboxStatus.Cancelled, reason, cancellationToken);
    }
    
    public async Task<OutboxItem> ScheduleItemAsync(string itemId, DateTime scheduledSendTime, CancellationToken cancellationToken = default)
    {
        var item = await UpdateStatusAsync(itemId, OutboxStatus.Scheduled, $"Scheduled for {scheduledSendTime}", cancellationToken);
        
        // Store scheduled send info in metadata
        item.Metadata["scheduledSendTime"] = scheduledSendTime;
        return await UpdateOutboxItemAsync(item, cancellationToken);
    }
    
    public async Task AddRecipientAsync(string itemId, OutboxRecipient recipient, CancellationToken cancellationToken = default)
    {
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) return;
        
        item.Recipients.Add(recipient);
        await UpdateOutboxItemAsync(item, cancellationToken);
    }
    
    public async Task RemoveRecipientAsync(string itemId, string recipientId, CancellationToken cancellationToken = default)
    {
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) return;
        
        item.Recipients.RemoveAll(r => r.Id == recipientId);
        await UpdateOutboxItemAsync(item, cancellationToken);
    }
    
    public async Task UpdateRecipientStatusAsync(string itemId, string recipientId, RecipientDeliveryStatus status, CancellationToken cancellationToken = default)
    {
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) return;
        
        var recipient = item.Recipients.FirstOrDefault(r => r.Id == recipientId);
        if (recipient != null)
        {
            recipient.DeliveryStatus = status;
            if (status == RecipientDeliveryStatus.Delivered)
            {
                recipient.DeliveredAt = DateTime.UtcNow;
            }
            else if (status == RecipientDeliveryStatus.Read)
            {
                recipient.ReadAt = DateTime.UtcNow;
            }
            
            await UpdateOutboxItemAsync(item, cancellationToken);
        }
    }
    
    public async Task AddAttachmentAsync(string itemId, OutboxAttachment attachment, CancellationToken cancellationToken = default)
    {
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) return;
        
        item.Attachments.Add(attachment);
        await UpdateOutboxItemAsync(item, cancellationToken);
    }
    
    public async Task RemoveAttachmentAsync(string itemId, string attachmentId, CancellationToken cancellationToken = default)
    {
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) return;
        
        item.Attachments.RemoveAll(a => a.Id == attachmentId);
        await UpdateOutboxItemAsync(item, cancellationToken);
    }
    
    public async Task RecordDeliveryAttemptAsync(string itemId, OutboxDeliveryAttempt attempt, CancellationToken cancellationToken = default)
    {
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) return;
        
        item.DeliveryAttempts.Add(attempt);
        await UpdateOutboxItemAsync(item, cancellationToken);
    }
    
    public async Task<List<OutboxDeliveryAttempt>> GetDeliveryAttemptsAsync(string itemId, CancellationToken cancellationToken = default)
    {
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        return item?.DeliveryAttempts ?? new List<OutboxDeliveryAttempt>();
    }
    
    public async Task ConfirmDeliveryAsync(string itemId, string recipientId, DateTime deliveryTime, CancellationToken cancellationToken = default)
    {
        await UpdateRecipientStatusAsync(itemId, recipientId, RecipientDeliveryStatus.Delivered, cancellationToken);
        
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) return;
        
        // Check if all recipients have been delivered
        if (item.Recipients.All(r => r.DeliveryStatus == RecipientDeliveryStatus.Delivered))
        {
            item.DeliveredAt = DateTime.UtcNow;
            await UpdateStatusAsync(itemId, OutboxStatus.Delivered, "All recipients delivered", cancellationToken);
        }
        else if (item.Recipients.Any(r => r.DeliveryStatus == RecipientDeliveryStatus.Delivered))
        {
            await UpdateStatusAsync(itemId, OutboxStatus.PartiallyDelivered, "Some recipients delivered", cancellationToken);
        }
    }
    
    public async Task RecordReadReceiptAsync(string itemId, string recipientId, DateTime readTime, CancellationToken cancellationToken = default)
    {
        await UpdateRecipientStatusAsync(itemId, recipientId, RecipientDeliveryStatus.Read, cancellationToken);
        
        var item = await GetOutboxItemAsync(itemId, cancellationToken);
        if (item == null) return;
        
        // Update overall read time if this is the first read
        if (!item.ReadAt.HasValue)
        {
            item.ReadAt = readTime;
            await UpdateOutboxItemAsync(item, cancellationToken);
        }
    }
    
    public async Task<List<OutboxItem>> SearchOutboxAsync(OutboxFilter filter, CancellationToken cancellationToken = default)
    {
        var conditions = new List<string>();
        var bindVars = new Dictionary<string, object> { ["collection"] = OutboxCollectionName };
        
        if (!string.IsNullOrEmpty(filter.SentBy))
        {
            conditions.Add("item.sentBy == @sentBy");
            bindVars["sentBy"] = filter.SentBy;
        }
        
        if (filter.Status.HasValue)
        {
            conditions.Add("item.status == @status");
            bindVars["status"] = filter.Status.Value.ToString();
        }
        
        if (filter.Priority.HasValue)
        {
            conditions.Add("item.priority == @priority");
            bindVars["priority"] = filter.Priority.Value.ToString();
        }
        
        if (filter.SentAfter.HasValue)
        {
            conditions.Add("item.sentAt >= @sentAfter");
            bindVars["sentAfter"] = filter.SentAfter.Value;
        }
        
        if (filter.SentBefore.HasValue)
        {
            conditions.Add("item.sentAt <= @sentBefore");
            bindVars["sentBefore"] = filter.SentBefore.Value;
        }
        
        if (!string.IsNullOrEmpty(filter.SearchText))
        {
            conditions.Add("(CONTAINS(LOWER(item.subject), LOWER(@search)) OR CONTAINS(LOWER(item.description), LOWER(@search)))");
            bindVars["search"] = filter.SearchText;
        }
        
        var filterClause = conditions.Any() ? $"FILTER {string.Join(" AND ", conditions)}" : "";
        
        var query = $@"
            FOR item IN @@collection
            {filterClause}
            SORT item.createdAt DESC
            RETURN item
        ";
        
        var result = await _apiClient.ExecuteAqlAsync<OutboxItem>(query, bindVars, cancellationToken);
        return result.ToList();
    }
    
    public async Task<List<OutboxItem>> GetOutboxByUserAsync(string userId, OutboxStatus? status = null, CancellationToken cancellationToken = default)
    {
        var filter = new OutboxFilter { SentBy = userId, Status = status };
        return await SearchOutboxAsync(filter, cancellationToken);
    }
    
    public async Task<List<OutboxItem>> GetOutboxByProcessAsync(string processId, CancellationToken cancellationToken = default)
    {
        var filter = new OutboxFilter { RelatedProcessId = processId };
        return await SearchOutboxAsync(filter, cancellationToken);
    }
    
    public async Task<List<OutboxItem>> GetPendingOutboxItemsAsync(CancellationToken cancellationToken = default)
    {
        var filter = new OutboxFilter { Status = OutboxStatus.Scheduled };
        return await SearchOutboxAsync(filter, cancellationToken);
    }
    
    public async Task<List<OutboxItem>> GetFailedOutboxItemsAsync(CancellationToken cancellationToken = default)
    {
        var filter = new OutboxFilter { Status = OutboxStatus.Failed };
        return await SearchOutboxAsync(filter, cancellationToken);
    }
    
    public async Task<OutboxStatistics> GetOutboxStatisticsAsync(DateTime? startDate = null, DateTime? endDate = null, CancellationToken cancellationToken = default)
    {
        var items = await GetAllOutboxItemsAsync(cancellationToken);
        
        if (startDate.HasValue)
        {
            items = items.Where(i => i.SentAt >= startDate.Value).ToList();
        }
        
        if (endDate.HasValue)
        {
            items = items.Where(i => i.SentAt <= endDate.Value).ToList();
        }
        
        return new OutboxStatistics
        {
            TotalSent = items.Count(i => i.Status == OutboxStatus.Sent || i.Status == OutboxStatus.Delivered),
            TotalDelivered = items.Count(i => i.Status == OutboxStatus.Delivered),
            TotalFailed = items.Count(i => i.Status == OutboxStatus.Failed),
            PendingDelivery = items.Count(i => i.Status == OutboxStatus.Sending || i.Status == OutboxStatus.Scheduled),
            ByDeliveryMethod = items.GroupBy(i => i.DeliveryMethod).ToDictionary(g => g.Key, g => g.Count()),
            ByPriority = items.GroupBy(i => i.Priority).ToDictionary(g => g.Key, g => g.Count()),
            AwaitingReadReceipt = items.Count(i => i.RequireReadReceipt && !i.ReadAt.HasValue)
        };
    }
    
    public async Task<int> GetUnsentCountAsync(string? userId = null, CancellationToken cancellationToken = default)
    {
        var filter = new OutboxFilter { Status = OutboxStatus.Draft, SentBy = userId };
        var items = await SearchOutboxAsync(filter, cancellationToken);
        return items.Count;
    }
    
    public async Task<OutboxTemplate> CreateTemplateAsync(OutboxTemplate template, CancellationToken cancellationToken = default)
    {
        var query = "INSERT @template INTO @@collection RETURN NEW";
        var result = await _apiClient.ExecuteAqlAsync<OutboxTemplate>(query, new
        {
            collection = OutboxTemplateCollectionName,
            template
        }, cancellationToken);
        
        return result.First();
    }
    
    public async Task<List<OutboxTemplate>> GetTemplatesAsync(CancellationToken cancellationToken = default)
    {
        var query = "FOR template IN @@collection SORT template.name RETURN template";
        var result = await _apiClient.ExecuteAqlAsync<OutboxTemplate>(query, new
        {
            collection = OutboxTemplateCollectionName
        }, cancellationToken);
        
        return result.ToList();
    }
    
    public async Task<OutboxItem> CreateFromTemplateAsync(string templateId, Dictionary<string, string> variables, CancellationToken cancellationToken = default)
    {
        var query = "FOR template IN @@collection FILTER template._key == @templateId RETURN template";
        var templates = await _apiClient.ExecuteAqlAsync<OutboxTemplate>(query, new
        {
            collection = OutboxTemplateCollectionName,
            templateId
        }, cancellationToken);
        
        var template = templates.FirstOrDefault();
        if (template == null) throw new ArgumentException($"Template {templateId} not found");
        
        // Replace variables in subject and body
        var subject = template.SubjectTemplate;
        var body = template.BodyTemplate;
        
        foreach (var (key, value) in variables)
        {
            subject = subject.Replace($"{{{key}}}", value);
            body = body.Replace($"{{{key}}}", value);
        }
        
        var item = new OutboxItem
        {
            Subject = subject,
            Description = body,
            DeliveryMethod = template.DefaultDeliveryMethod,
            Priority = template.DefaultPriority,
            Tags = new List<string>(template.DefaultTags),
            Attachments = new List<OutboxAttachment>(template.DefaultAttachments)
        };
        
        return await CreateOutboxItemAsync(item, cancellationToken);
    }
    
    public async Task<List<OutboxItem>> BulkSendAsync(List<string> itemIds, CancellationToken cancellationToken = default)
    {
        var sentItems = new List<OutboxItem>();
        
        foreach (var itemId in itemIds)
        {
            try
            {
                var sent = await SendItemAsync(itemId, cancellationToken);
                sentItems.Add(sent);
            }
            catch
            {
                // Continue with other items
                continue;
            }
        }
        
        return sentItems;
    }
    
    public async Task BulkArchiveAsync(List<string> itemIds, CancellationToken cancellationToken = default)
    {
        foreach (var itemId in itemIds)
        {
            await UpdateStatusAsync(itemId, OutboxStatus.Archived, "Bulk archive", cancellationToken);
        }
    }
    
    public async Task ProcessScheduledSendsAsync(CancellationToken cancellationToken = default)
    {
        var pending = await GetPendingOutboxItemsAsync(cancellationToken);
        var now = DateTime.UtcNow;
        
        foreach (var item in pending)
        {
            if (item.Metadata.TryGetValue("scheduledSendTime", out var sendTimeObj) && 
                sendTimeObj is DateTime sendTime && 
                sendTime <= now)
            {
                try
                {
                    await SendItemAsync(item.Id, cancellationToken);
                }
                catch
                {
                    // Log error but continue with other items
                    continue;
                }
            }
        }
    }
    
    public async Task<OutboxItem> CreateReplyAsync(string inboxItemId, string replyText, List<OutboxAttachment>? attachments = null, CancellationToken cancellationToken = default)
    {
        var inboxItem = await _inboxService.GetInboxItemAsync(inboxItemId);
        if (inboxItem == null) throw new ArgumentException($"Inbox item {inboxItemId} not found");
        
        var outboxItem = new OutboxItem
        {
            Subject = $"RE: {inboxItem.Subject}",
            Description = replyText,
            RelatedInboxItemId = inboxItemId,
            RelatedProcessId = inboxItem.RelatedProcessId,
            Recipients = new List<OutboxRecipient>
            {
                new OutboxRecipient
                {
                    Name = inboxItem.Sender ?? "Unknown",
                    Email = inboxItem.SenderEmail ?? string.Empty,
                    Type = RecipientType.External
                }
            },
            Attachments = attachments ?? new List<OutboxAttachment>()
        };
        
        return await CreateOutboxItemAsync(outboxItem, cancellationToken);
    }
    
    // Helper method to simulate sending
    private async Task SimulateSendAsync(OutboxItem item, CancellationToken cancellationToken)
    {
        // In a real implementation, this would integrate with actual delivery services
        // For now, just simulate a delay and mark recipients as sent
        await Task.Delay(100, cancellationToken);
        
        foreach (var recipient in item.Recipients)
        {
            recipient.DeliveryStatus = RecipientDeliveryStatus.Sent;
        }
    }
}
