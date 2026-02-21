/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Phase3ComplianceServices.cs                        ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     913                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

// ============================================================================
// Phase 3: Compliance & Integration Services
// ============================================================================

#region 4-Augen-Prinzip Service

public interface IFourEyesPrincipleService
{
    // Regeln verwalten
    Task<FourEyesPrincipleRule> CreateRuleAsync(FourEyesPrincipleRule rule, CancellationToken cancellationToken = default);
    Task<FourEyesPrincipleRule?> GetRuleAsync(string ruleId, CancellationToken cancellationToken = default);
    Task<List<FourEyesPrincipleRule>> GetActiveRulesAsync(CancellationToken cancellationToken = default);
    Task UpdateRuleAsync(FourEyesPrincipleRule rule, CancellationToken cancellationToken = default);
    Task DeleteRuleAsync(string ruleId, CancellationToken cancellationToken = default);
    
    // Prüfung ob 4-Augen-Prinzip erforderlich
    Task<bool> IsRequiredAsync(string processId, Dictionary<string, object> context, CancellationToken cancellationToken = default);
    Task<List<FourEyesPrincipleRule>> GetApplicableRulesAsync(string processId, Dictionary<string, object> context, CancellationToken cancellationToken = default);
    
    // Genehmigungen
    Task<FourEyesApproval> CreateApprovalAsync(FourEyesApproval approval, CancellationToken cancellationToken = default);
    Task<FourEyesApproval?> GetApprovalAsync(string approvalId, CancellationToken cancellationToken = default);
    Task<List<FourEyesApproval>> GetPendingApprovalsAsync(string userId, CancellationToken cancellationToken = default);
    Task ApproveAsync(string approvalId, string userId, string comment, CancellationToken cancellationToken = default);
    Task RejectAsync(string approvalId, string userId, string reason, CancellationToken cancellationToken = default);
    Task<bool> IsApprovedAsync(string approvalId, CancellationToken cancellationToken = default);
}

public class FourEyesPrincipleService : IFourEyesPrincipleService
{
    private readonly IThemisDBService _themisDb;
    private readonly IProcessTimelineService _timelineService;
    private readonly INotificationService? _notificationService;
    
    public FourEyesPrincipleService(
        IThemisDBService themisDb,
        IProcessTimelineService timelineService,
        INotificationService? notificationService = null)
    {
        ArgumentNullException.ThrowIfNull(themisDb);
        ArgumentNullException.ThrowIfNull(timelineService);
        
        _themisDb = themisDb;
        _timelineService = timelineService;
        _notificationService = notificationService;
    }
    
    public async Task<FourEyesPrincipleRule> CreateRuleAsync(FourEyesPrincipleRule rule, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(rule);
        
        rule.CreatedAt = DateTime.UtcNow;
        
        var query = "INSERT @rule INTO four_eyes_rules RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<FourEyesPrincipleRule>(
            query,
            new { rule },
            cancellationToken
        );
        
        return result.FirstOrDefault() ?? rule;
    }
    
    public async Task<FourEyesPrincipleRule?> GetRuleAsync(string ruleId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(ruleId);
        
        var query = "FOR rule IN four_eyes_rules FILTER rule.Id == @ruleId RETURN rule";
        var result = await _themisDb.ExecuteQueryAsync<FourEyesPrincipleRule>(
            query,
            new { ruleId },
            cancellationToken
        );
        
        return result.FirstOrDefault();
    }
    
    public async Task<List<FourEyesPrincipleRule>> GetActiveRulesAsync(CancellationToken cancellationToken = default)
    {
        var query = "FOR rule IN four_eyes_rules FILTER rule.IsActive == true RETURN rule";
        return await _themisDb.ExecuteQueryAsync<FourEyesPrincipleRule>(query, null, cancellationToken);
    }
    
    public async Task UpdateRuleAsync(FourEyesPrincipleRule rule, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(rule);
        
        var query = "UPDATE @rule IN four_eyes_rules RETURN NEW";
        await _themisDb.ExecuteQueryAsync<FourEyesPrincipleRule>(
            query,
            new { rule },
            cancellationToken
        );
    }
    
    public async Task DeleteRuleAsync(string ruleId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(ruleId);
        
        var query = "FOR rule IN four_eyes_rules FILTER rule.Id == @ruleId REMOVE rule IN four_eyes_rules";
        await _themisDb.ExecuteQueryAsync<object>(query, new { ruleId }, cancellationToken);
    }
    
    public async Task<bool> IsRequiredAsync(string processId, Dictionary<string, object> context, CancellationToken cancellationToken = default)
    {
        var applicableRules = await GetApplicableRulesAsync(processId, context, cancellationToken);
        return applicableRules.Count > 0;
    }
    
    public async Task<List<FourEyesPrincipleRule>> GetApplicableRulesAsync(string processId, Dictionary<string, object> context, CancellationToken cancellationToken = default)
    {
        var activeRules = await GetActiveRulesAsync(cancellationToken);
        var applicableRules = new List<FourEyesPrincipleRule>();
        
        foreach (var rule in activeRules)
        {
            bool isApplicable = rule.TriggerType switch
            {
                FourEyesTriggerType.AmountThreshold => CheckAmountThreshold(rule, context),
                FourEyesTriggerType.SecurityClassification => CheckSecurityLevel(rule, context),
                FourEyesTriggerType.ProcessType => CheckProcessType(rule, context),
                FourEyesTriggerType.DocumentType => CheckDocumentType(rule, context),
                FourEyesTriggerType.CombinedCriteria => CheckCombinedCriteria(rule, context),
                _ => false
            };
            
            if (isApplicable)
            {
                applicableRules.Add(rule);
            }
        }
        
        return applicableRules;
    }
    
    private static bool CheckAmountThreshold(FourEyesPrincipleRule rule, Dictionary<string, object> context)
    {
        if (!rule.AmountThreshold.HasValue) return false;
        if (!context.TryGetValue("amount", out var amountObj)) return false;
        if (amountObj is not decimal amount) return false;
        
        return amount >= rule.AmountThreshold.Value;
    }
    
    private static bool CheckSecurityLevel(FourEyesPrincipleRule rule, Dictionary<string, object> context)
    {
        if (!rule.SecurityLevel.HasValue) return false;
        if (!context.TryGetValue("securityLevel", out var levelObj)) return false;
        if (levelObj is not SecurityClassification level) return false;
        
        return level >= rule.SecurityLevel.Value;
    }
    
    private static bool CheckProcessType(FourEyesPrincipleRule rule, Dictionary<string, object> context)
    {
        if (string.IsNullOrEmpty(rule.ProcessType)) return false;
        if (!context.TryGetValue("processType", out var typeObj)) return false;
        if (typeObj is not string processType) return false;
        
        return rule.ProcessType == processType;
    }
    
    private static bool CheckDocumentType(FourEyesPrincipleRule rule, Dictionary<string, object> context)
    {
        if (string.IsNullOrEmpty(rule.DocumentType)) return false;
        if (!context.TryGetValue("documentType", out var typeObj)) return false;
        if (typeObj is not string documentType) return false;
        
        return rule.DocumentType == documentType;
    }
    
    private static bool CheckCombinedCriteria(FourEyesPrincipleRule rule, Dictionary<string, object> context)
    {
        return CheckAmountThreshold(rule, context) ||
               CheckSecurityLevel(rule, context) ||
               CheckProcessType(rule, context) ||
               CheckDocumentType(rule, context);
    }
    
    public async Task<FourEyesApproval> CreateApprovalAsync(FourEyesApproval approval, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(approval);
        
        approval.CreatedAt = DateTime.UtcNow;
        approval.Status = FourEyesApprovalStatus.Pending;
        
        var query = "INSERT @approval INTO four_eyes_approvals RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<FourEyesApproval>(
            query,
            new { approval },
            cancellationToken
        );
        
        // Benachrichtigungen senden
        if (_notificationService != null)
        {
            foreach (var approver in approval.Approvers)
            {
                // Notify approvers (implementation depends on notification service)
            }
        }
        
        return result.FirstOrDefault() ?? approval;
    }
    
    public async Task<FourEyesApproval?> GetApprovalAsync(string approvalId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(approvalId);
        
        var query = "FOR approval IN four_eyes_approvals FILTER approval.Id == @approvalId RETURN approval";
        var result = await _themisDb.ExecuteQueryAsync<FourEyesApproval>(
            query,
            new { approvalId },
            cancellationToken
        );
        
        return result.FirstOrDefault();
    }
    
    public async Task<List<FourEyesApproval>> GetPendingApprovalsAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(userId);
        
        var query = @"
            FOR approval IN four_eyes_approvals
            FILTER approval.Status == 'Pending'
            FOR approver IN approval.Approvers
            FILTER approver.UserId == @userId AND approver.Status == 'Pending'
            RETURN approval";
            
        return await _themisDb.ExecuteQueryAsync<FourEyesApproval>(
            query,
            new { userId },
            cancellationToken
        );
    }
    
    public async Task ApproveAsync(string approvalId, string userId, string comment, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(approvalId);
        ArgumentException.ThrowIfNullOrEmpty(userId);
        
        var approval = await GetApprovalAsync(approvalId, cancellationToken);
        if (approval == null) throw new InvalidOperationException($"Approval {approvalId} not found");
        
        var approver = approval.Approvers.FirstOrDefault(a => a.UserId == userId);
        if (approver == null) throw new InvalidOperationException($"User {userId} is not an approver");
        
        approver.Status = FourEyesApproverStatus.Approved;
        approver.ApprovedAt = DateTime.UtcNow;
        approver.Comment = comment;
        
        // Check if all approved
        if (approval.Approvers.Count(a => a.Status == FourEyesApproverStatus.Approved) >= approval.Approvers.Count)
        {
            approval.Status = FourEyesApprovalStatus.Approved;
            approval.CompletedAt = DateTime.UtcNow;
        }
        
        await UpdateRuleAsync(new FourEyesPrincipleRule { Id = approvalId }, cancellationToken);
        
        // Timeline event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            ProcessId = approval.ProcessId,
            Type = ProcessEventType.Approved,
            Actor = userId,
            Description = $"4-Augen-Prinzip: Genehmigung erteilt{(comment != null ? $" - {comment}" : "")}"
        }, cancellationToken);
    }
    
    public async Task RejectAsync(string approvalId, string userId, string reason, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(approvalId);
        ArgumentException.ThrowIfNullOrEmpty(userId);
        ArgumentException.ThrowIfNullOrEmpty(reason);
        
        var approval = await GetApprovalAsync(approvalId, cancellationToken);
        if (approval == null) throw new InvalidOperationException($"Approval {approvalId} not found");
        
        var approver = approval.Approvers.FirstOrDefault(a => a.UserId == userId);
        if (approver == null) throw new InvalidOperationException($"User {userId} is not an approver");
        
        approver.Status = FourEyesApproverStatus.Rejected;
        approver.ApprovedAt = DateTime.UtcNow;
        approver.RejectReason = reason;
        
        approval.Status = FourEyesApprovalStatus.Rejected;
        approval.CompletedAt = DateTime.UtcNow;
        
        await UpdateRuleAsync(new FourEyesPrincipleRule { Id = approvalId }, cancellationToken);
        
        // Timeline event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            ProcessId = approval.ProcessId,
            Type = ProcessEventType.Rejected,
            Actor = userId,
            Description = $"4-Augen-Prinzip: Ablehnung - {reason}"
        }, cancellationToken);
    }
    
    public async Task<bool> IsApprovedAsync(string approvalId, CancellationToken cancellationToken = default)
    {
        var approval = await GetApprovalAsync(approvalId, cancellationToken);
        return approval?.Status == FourEyesApprovalStatus.Approved;
    }
}

#endregion

#region Akteneinsichts-Protokoll Service

public interface IFileAccessLogService
{
    Task<FileAccessLog> LogAccessAsync(FileAccessLog accessLog, CancellationToken cancellationToken = default);
    Task<List<FileAccessLog>> GetAccessLogsAsync(string fileId, CancellationToken cancellationToken = default);
    Task<List<FileAccessLog>> GetUserAccessLogsAsync(string userId, DateTime? from = null, DateTime? to = null, CancellationToken cancellationToken = default);
    Task<FileAccessRequest> CreateAccessRequestAsync(FileAccessRequest request, CancellationToken cancellationToken = default);
    Task ApproveAccessRequestAsync(string requestId, string approvedBy, CancellationToken cancellationToken = default);
    Task RejectAccessRequestAsync(string requestId, string rejectedBy, string reason, CancellationToken cancellationToken = default);
    Task<List<FileAccessRequest>> GetPendingRequestsAsync(CancellationToken cancellationToken = default);
}

public class FileAccessLogService : IFileAccessLogService
{
    private readonly IThemisDBService _themisDb;
    private readonly IProcessTimelineService _timelineService;
    
    public FileAccessLogService(IThemisDBService themisDb, IProcessTimelineService timelineService)
    {
        ArgumentNullException.ThrowIfNull(themisDb);
        ArgumentNullException.ThrowIfNull(timelineService);
        
        _themisDb = themisDb;
        _timelineService = timelineService;
    }
    
    public async Task<FileAccessLog> LogAccessAsync(FileAccessLog accessLog, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(accessLog);
        
        accessLog.AccessTime = DateTime.UtcNow;
        
        var query = "INSERT @accessLog INTO file_access_logs RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<FileAccessLog>(
            query,
            new { accessLog },
            cancellationToken
        );
        
        // Timeline event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            ProcessId = accessLog.FileId,
            Type = ProcessEventType.FileAccessed,
            Actor = accessLog.UserId,
            Description = $"Akteneinsicht: {accessLog.AccessType} - {accessLog.Purpose}"
        }, cancellationToken);
        
        return result.FirstOrDefault() ?? accessLog;
    }
    
    public async Task<List<FileAccessLog>> GetAccessLogsAsync(string fileId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(fileId);
        
        var query = @"
            FOR log IN file_access_logs
            FILTER log.FileId == @fileId
            SORT log.AccessTime DESC
            RETURN log";
            
        return await _themisDb.ExecuteQueryAsync<FileAccessLog>(
            query,
            new { fileId },
            cancellationToken
        );
    }
    
    public async Task<List<FileAccessLog>> GetUserAccessLogsAsync(string userId, DateTime? from = null, DateTime? to = null, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(userId);
        
        var fromDate = from ?? DateTime.UtcNow.AddMonths(-12);
        var toDate = to ?? DateTime.UtcNow;
        
        var query = @"
            FOR log IN file_access_logs
            FILTER log.UserId == @userId
            FILTER log.AccessTime >= @fromDate AND log.AccessTime <= @toDate
            SORT log.AccessTime DESC
            RETURN log";
            
        return await _themisDb.ExecuteQueryAsync<FileAccessLog>(
            query,
            new { userId, fromDate, toDate },
            cancellationToken
        );
    }
    
    public async Task<FileAccessRequest> CreateAccessRequestAsync(FileAccessRequest request, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(request);
        
        request.RequestedAt = DateTime.UtcNow;
        request.Status = FileAccessRequestStatus.Pending;
        
        var query = "INSERT @request INTO file_access_requests RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<FileAccessRequest>(
            query,
            new { request },
            cancellationToken
        );
        
        return result.FirstOrDefault() ?? request;
    }
    
    public async Task ApproveAccessRequestAsync(string requestId, string approvedBy, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(requestId);
        ArgumentException.ThrowIfNullOrEmpty(approvedBy);
        
        var query = @"
            FOR request IN file_access_requests
            FILTER request.Id == @requestId
            UPDATE request WITH {
                Status: 'Approved',
                ApprovedBy: @approvedBy,
                ApprovedAt: @approvedAt
            } IN file_access_requests";
            
        await _themisDb.ExecuteQueryAsync<object>(
            query,
            new { requestId, approvedBy, approvedAt = DateTime.UtcNow },
            cancellationToken
        );
    }
    
    public async Task RejectAccessRequestAsync(string requestId, string rejectedBy, string reason, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(requestId);
        ArgumentException.ThrowIfNullOrEmpty(rejectedBy);
        ArgumentException.ThrowIfNullOrEmpty(reason);
        
        var query = @"
            FOR request IN file_access_requests
            FILTER request.Id == @requestId
            UPDATE request WITH {
                Status: 'Rejected',
                ApprovedBy: @rejectedBy,
                ApprovedAt: @approvedAt,
                RejectionReason: @reason
            } IN file_access_requests";
            
        await _themisDb.ExecuteQueryAsync<object>(
            query,
            new { requestId, rejectedBy, approvedAt = DateTime.UtcNow, reason },
            cancellationToken
        );
    }
    
    public async Task<List<FileAccessRequest>> GetPendingRequestsAsync(CancellationToken cancellationToken = default)
    {
        var query = @"
            FOR request IN file_access_requests
            FILTER request.Status == 'Pending'
            SORT request.RequestedAt ASC
            RETURN request";
            
        return await _themisDb.ExecuteQueryAsync<FileAccessRequest>(query, null, cancellationToken);
    }
}

#endregion

#region Stellvertretung Service

public interface ISubstitutionService
{
    Task<SubstitutionRule> CreateRuleAsync(SubstitutionRule rule, CancellationToken cancellationToken = default);
    Task<List<SubstitutionRule>> GetActiveRulesAsync(string userId, CancellationToken cancellationToken = default);
    Task<SubstitutionRule?> GetEffectiveSubstituteAsync(string userId, CancellationToken cancellationToken = default);
    Task<List<string>> GetUsersSubstitutedByAsync(string substituteUserId, CancellationToken cancellationToken = default);
    Task UpdateRuleAsync(SubstitutionRule rule, CancellationToken cancellationToken = default);
    Task DeleteRuleAsync(string ruleId, CancellationToken cancellationToken = default);
    Task<SubstitutionAction> LogActionAsync(SubstitutionAction action, CancellationToken cancellationToken = default);
}

public class SubstitutionService : ISubstitutionService
{
    private readonly IThemisDBService _themisDb;
    private readonly INotificationService? _notificationService;
    
    public SubstitutionService(IThemisDBService themisDb, INotificationService? notificationService = null)
    {
        ArgumentNullException.ThrowIfNull(themisDb);
        _themisDb = themisDb;
        _notificationService = notificationService;
    }
    
    public async Task<SubstitutionRule> CreateRuleAsync(SubstitutionRule rule, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(rule);
        
        rule.CreatedAt = DateTime.UtcNow;
        rule.Status = SubstitutionRuleStatus.Active;
        
        var query = "INSERT @rule INTO substitution_rules RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<SubstitutionRule>(
            query,
            new { rule },
            cancellationToken
        );
        
        // Send notifications
        if (_notificationService != null)
        {
            if (rule.NotifyOriginalUser)
            {
                // Notify original user
            }
            if (rule.NotifySubstitute)
            {
                // Notify substitute
            }
        }
        
        return result.FirstOrDefault() ?? rule;
    }
    
    public async Task<List<SubstitutionRule>> GetActiveRulesAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(userId);
        
        var now = DateTime.UtcNow;
        var query = @"
            FOR rule IN substitution_rules
            FILTER rule.UserId == @userId
            FILTER rule.Status == 'Active'
            FILTER rule.StartDate <= @now AND rule.EndDate >= @now
            RETURN rule";
            
        return await _themisDb.ExecuteQueryAsync<SubstitutionRule>(
            query,
            new { userId, now },
            cancellationToken
        );
    }
    
    public async Task<SubstitutionRule?> GetEffectiveSubstituteAsync(string userId, CancellationToken cancellationToken = default)
    {
        var activeRules = await GetActiveRulesAsync(userId, cancellationToken);
        return activeRules.FirstOrDefault(r => r.Scope == SubstitutionScope.Full);
    }
    
    public async Task<List<string>> GetUsersSubstitutedByAsync(string substituteUserId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(substituteUserId);
        
        var now = DateTime.UtcNow;
        var query = @"
            FOR rule IN substitution_rules
            FILTER rule.SubstituteUserId == @substituteUserId
            FILTER rule.Status == 'Active'
            FILTER rule.StartDate <= @now AND rule.EndDate >= @now
            RETURN rule.UserId";
            
        return await _themisDb.ExecuteQueryAsync<string>(
            query,
            new { substituteUserId, now },
            cancellationToken
        );
    }
    
    public async Task UpdateRuleAsync(SubstitutionRule rule, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(rule);
        
        var query = "UPDATE @rule IN substitution_rules RETURN NEW";
        await _themisDb.ExecuteQueryAsync<SubstitutionRule>(
            query,
            new { rule },
            cancellationToken
        );
    }
    
    public async Task DeleteRuleAsync(string ruleId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(ruleId);
        
        var query = "FOR rule IN substitution_rules FILTER rule.Id == @ruleId REMOVE rule IN substitution_rules";
        await _themisDb.ExecuteQueryAsync<object>(query, new { ruleId }, cancellationToken);
    }
    
    public async Task<SubstitutionAction> LogActionAsync(SubstitutionAction action, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(action);
        
        action.ActionTime = DateTime.UtcNow;
        
        var query = "INSERT @action INTO substitution_actions RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<SubstitutionAction>(
            query,
            new { action },
            cancellationToken
        );
        
        return result.FirstOrDefault() ?? action;
    }
}

#endregion

#region eGov Service

public interface IEGovService
{
    Task<EGovMessage> CreateMessageAsync(EGovMessage message, CancellationToken cancellationToken = default);
    Task<EGovMessage> SendMessageAsync(string messageId, CancellationToken cancellationToken = default);
    Task<List<EGovMessage>> GetInboundMessagesAsync(DateTime? since = null, CancellationToken cancellationToken = default);
    Task<List<EGovMessage>> GetOutboundMessagesAsync(DateTime? since = null, CancellationToken cancellationToken = default);
    Task<EGovMessage?> GetMessageAsync(string messageId, CancellationToken cancellationToken = default);
    Task<EGovConfiguration> GetConfigurationAsync(EGovProtocol protocol, CancellationToken cancellationToken = default);
    Task SaveConfigurationAsync(EGovConfiguration config, CancellationToken cancellationToken = default);
}

public class EGovService : IEGovService
{
    private readonly IThemisDBService _themisDb;
    private readonly IProcessTimelineService _timelineService;
    
    public EGovService(IThemisDBService themisDb, IProcessTimelineService timelineService)
    {
        ArgumentNullException.ThrowIfNull(themisDb);
        ArgumentNullException.ThrowIfNull(timelineService);
        
        _themisDb = themisDb;
        _timelineService = timelineService;
    }
    
    public async Task<EGovMessage> CreateMessageAsync(EGovMessage message, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(message);
        
        message.CreatedAt = DateTime.UtcNow;
        message.Status = EGovMessageStatus.Draft;
        
        var query = "INSERT @message INTO egov_messages RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<EGovMessage>(
            query,
            new { message },
            cancellationToken
        );
        
        return result.FirstOrDefault() ?? message;
    }
    
    public async Task<EGovMessage> SendMessageAsync(string messageId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(messageId);
        
        var message = await GetMessageAsync(messageId, cancellationToken);
        if (message == null) throw new InvalidOperationException($"Message {messageId} not found");
        
        message.Status = EGovMessageStatus.Sending;
        message.SentAt = DateTime.UtcNow;
        
        // Hier würde die tatsächliche Versandlogik stehen (OSCI, XTA, etc.)
        // Für die Demonstration setzen wir den Status direkt auf "Sent"
        message.Status = EGovMessageStatus.Sent;
        
        var query = "UPDATE @message IN egov_messages RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<EGovMessage>(
            query,
            new { message },
            cancellationToken
        );
        
        // Timeline event
        if (message.ProcessId != null)
        {
            await _timelineService.CreateEventAsync(new ProcessTimelineEvent
            {
                ProcessId = message.ProcessId,
                Type = ProcessEventType.DocumentSent,
                Description = $"eGov-Nachricht versendet via {message.Protocol}: {message.Subject}"
            }, cancellationToken);
        }
        
        return result.FirstOrDefault() ?? message;
    }
    
    public async Task<List<EGovMessage>> GetInboundMessagesAsync(DateTime? since = null, CancellationToken cancellationToken = default)
    {
        var sinceDate = since ?? DateTime.UtcNow.AddMonths(-1);
        
        var query = @"
            FOR message IN egov_messages
            FILTER message.Direction == 'Inbound'
            FILTER message.ReceivedAt >= @sinceDate
            SORT message.ReceivedAt DESC
            RETURN message";
            
        return await _themisDb.ExecuteQueryAsync<EGovMessage>(
            query,
            new { sinceDate },
            cancellationToken
        );
    }
    
    public async Task<List<EGovMessage>> GetOutboundMessagesAsync(DateTime? since = null, CancellationToken cancellationToken = default)
    {
        var sinceDate = since ?? DateTime.UtcNow.AddMonths(-1);
        
        var query = @"
            FOR message IN egov_messages
            FILTER message.Direction == 'Outbound'
            FILTER message.SentAt >= @sinceDate
            SORT message.SentAt DESC
            RETURN message";
            
        return await _themisDb.ExecuteQueryAsync<EGovMessage>(
            query,
            new { sinceDate },
            cancellationToken
        );
    }
    
    public async Task<EGovMessage?> GetMessageAsync(string messageId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(messageId);
        
        var query = "FOR message IN egov_messages FILTER message.Id == @messageId RETURN message";
        var result = await _themisDb.ExecuteQueryAsync<EGovMessage>(
            query,
            new { messageId },
            cancellationToken
        );
        
        return result.FirstOrDefault();
    }
    
    public async Task<EGovConfiguration> GetConfigurationAsync(EGovProtocol protocol, CancellationToken cancellationToken = default)
    {
        var query = "FOR config IN egov_configurations FILTER config.Protocol == @protocol RETURN config";
        var result = await _themisDb.ExecuteQueryAsync<EGovConfiguration>(
            query,
            new { protocol = protocol.ToString() },
            cancellationToken
        );
        
        return result.FirstOrDefault() ?? new EGovConfiguration { Protocol = protocol };
    }
    
    public async Task SaveConfigurationAsync(EGovConfiguration config, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(config);
        
        var query = "UPSERT { Protocol: @protocol } INSERT @config UPDATE @config IN egov_configurations";
        await _themisDb.ExecuteQueryAsync<object>(
            query,
            new { protocol = config.Protocol.ToString(), config },
            cancellationToken
        );
    }
}

#endregion

#region Transfer Note Service

public interface ITransferNoteService
{
    Task<TransferNote> CreateTransferNoteAsync(TransferNote note, CancellationToken cancellationToken = default);
    Task<List<TransferNote>> GetTransferNotesAsync(string fileId, CancellationToken cancellationToken = default);
    Task AcknowledgeTransferAsync(string noteId, string acknowledgedBy, string? comment = null, CancellationToken cancellationToken = default);
    Task<List<TransferNote>> GetPendingAcknowledgementsAsync(string userId, CancellationToken cancellationToken = default);
}

public class TransferNoteService : ITransferNoteService
{
    private readonly IThemisDBService _themisDb;
    private readonly IProcessTimelineService _timelineService;
    
    public TransferNoteService(IThemisDBService themisDb, IProcessTimelineService timelineService)
    {
        ArgumentNullException.ThrowIfNull(themisDb);
        ArgumentNullException.ThrowIfNull(timelineService);
        
        _themisDb = themisDb;
        _timelineService = timelineService;
    }
    
    public async Task<TransferNote> CreateTransferNoteAsync(TransferNote note, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(note);
        
        note.TransferDate = DateTime.UtcNow;
        
        var query = "INSERT @note INTO transfer_notes RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<TransferNote>(
            query,
            new { note },
            cancellationToken
        );
        
        // Timeline event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            ProcessId = note.FileId,
            Type = ProcessEventType.FileTransferred,
            Actor = note.TransferredFrom,
            Description = $"Akte übergeben von {note.TransferredFromDepartment} an {note.TransferredToDepartment} - {note.Reason}"
        }, cancellationToken);
        
        return result.FirstOrDefault() ?? note;
    }
    
    public async Task<List<TransferNote>> GetTransferNotesAsync(string fileId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(fileId);
        
        var query = @"
            FOR note IN transfer_notes
            FILTER note.FileId == @fileId
            SORT note.TransferDate DESC
            RETURN note";
            
        return await _themisDb.ExecuteQueryAsync<TransferNote>(
            query,
            new { fileId },
            cancellationToken
        );
    }
    
    public async Task AcknowledgeTransferAsync(string noteId, string acknowledgedBy, string? comment = null, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(noteId);
        ArgumentException.ThrowIfNullOrEmpty(acknowledgedBy);
        
        var query = @"
            FOR note IN transfer_notes
            FILTER note.Id == @noteId
            UPDATE note WITH {
                IsAcknowledged: true,
                AcknowledgedAt: @acknowledgedAt,
                AcknowledgedBy: @acknowledgedBy,
                AcknowledgementComment: @comment
            } IN transfer_notes RETURN NEW";
            
        await _themisDb.ExecuteQueryAsync<TransferNote>(
            query,
            new { noteId, acknowledgedBy, acknowledgedAt = DateTime.UtcNow, comment },
            cancellationToken
        );
    }
    
    public async Task<List<TransferNote>> GetPendingAcknowledgementsAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(userId);
        
        var query = @"
            FOR note IN transfer_notes
            FILTER note.TransferredTo == @userId
            FILTER note.IsAcknowledged == false
            SORT note.TransferDate ASC
            RETURN note";
            
        return await _themisDb.ExecuteQueryAsync<TransferNote>(
            query,
            new { userId },
            cancellationToken
        );
    }
}

#endregion
