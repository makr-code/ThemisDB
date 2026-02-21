/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentTreeService.cs                             ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     733                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Models;
using AsyncEnumerableExtensions = System.Linq.AsyncEnumerable;

namespace Themis.DocumentManager.Services;

#nullable enable

public interface IDocumentTreeService
{
    // Tree Operations
    Task<DocumentTreeNode> BuildTreeAsync(string processId, CancellationToken cancellationToken = default);
    Task<DocumentTreeNode> BuildTreeAsync(string processId, DocumentTreeConfiguration config, CancellationToken cancellationToken = default);
    Task<List<DocumentTreeNode>> GetFilteredNodesAsync(DocumentTreeFilter filter, CancellationToken cancellationToken = default);
    Task<List<DocumentTreeNode>> SearchTreeAsync(string query, CancellationToken cancellationToken = default);
    
    // Node Operations with lazy loading
    Task<DocumentTreeNode> GetNodeAsync(string nodeId, CancellationToken cancellationToken = default);
    Task<DocumentTreeNode> GetNodeWithChildrenAsync(string nodeId, int depth = 1, CancellationToken cancellationToken = default);
    Task<DocumentTreeNode> ExpandNodeAsync(string nodeId, CancellationToken cancellationToken = default);
    Task<DocumentTreeNode> CollapseNodeAsync(string nodeId, CancellationToken cancellationToken = default);
    Task<List<DocumentTreeNode>> LoadChildrenAsync(string parentId, CancellationToken cancellationToken = default);
    
    // Hierarchy operations
    Task<List<string>> GetPathToRootAsync(string nodeId, CancellationToken cancellationToken = default);
    Task<int> GetNodeDepthAsync(string nodeId, CancellationToken cancellationToken = default);
    Task<bool> ValidateNestingDepthAsync(string parentId, int maxDepth, CancellationToken cancellationToken = default);
    
    // Configuration
    Task<DocumentTreeConfiguration> GetConfigurationAsync(string userId, CancellationToken cancellationToken = default);
    Task SaveConfigurationAsync(string userId, DocumentTreeConfiguration config, CancellationToken cancellationToken = default);
    
    // Task Basket
    Task<List<TaskBasketItem>> GetTaskBasketItemsAsync(string userId, CancellationToken cancellationToken = default);
    Task<TaskBasketStatistics> GetTaskBasketStatisticsAsync(string userId, CancellationToken cancellationToken = default);
    Task<int> GetOverdueTasksCountAsync(string userId, CancellationToken cancellationToken = default);
}

public class DocumentTreeService : IDocumentTreeService
{
    private readonly IThemisDBService _themisDb;
    private readonly ILogger<DocumentTreeService> _logger;

    public DocumentTreeService(
        IThemisDBService themisDb,
        ILogger<DocumentTreeService> logger)
    {
        _themisDb = themisDb ?? throw new ArgumentNullException(nameof(themisDb));
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
    }

    public async Task<DocumentTreeNode> BuildTreeAsync(string processId, CancellationToken cancellationToken = default)
    {
        // Use default configuration
        var config = new DocumentTreeConfiguration();
        return await BuildTreeAsync(processId, config, cancellationToken);
    }

    public async Task<DocumentTreeNode> BuildTreeAsync(string processId, DocumentTreeConfiguration config, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(processId);
        ArgumentNullException.ThrowIfNull(config);

        try
        {
            _logger.LogInformation("Building document tree for process {ProcessId} with max depth {MaxDepth}", 
                processId, config.MaxNestingDepth);

            // Determine initial load depth
            var loadDepth = config.LazyLoadChildren ? config.InitialLoadDepth : config.MaxNestingDepth;

            // Query ThemisDB für Prozess-Hierarchie mit konfigurierbarer Tiefe
            var query = @"
                LET process = DOCUMENT(CONCAT('processes/', @processId))
                LET file = DOCUMENT(process.fileId)
                
                // Rekursive Hierarchie-Abfrage mit Tiefenbegrenzung
                LET hierarchy = (
                    FOR v, e, p IN 0..@maxDepth OUTBOUND file._id GRAPH 'document_hierarchy'
                        OPTIONS {uniqueVertices: 'path', bfs: true}
                        RETURN {
                            node: v,
                            edge: e,
                            path: p.vertices[*]._id,
                            level: LENGTH(p.edges)
                        }
                )
                
                // Vorgänge
                LET processes = (
                    FOR p IN processes
                        FILTER p.fileId == file._id
                        LIMIT @limit
                        RETURN p
                )
                
                // Dokumente mit Lazy Loading
                LET documents = @lazyLoad ? [] : (
                    FOR d IN documents
                        FILTER d.processId == process._id
                        LIMIT @limit
                        RETURN d
                )
                
                // Inbox
                LET inbox = (
                    FOR i IN inbox_items
                        FILTER i.processId == process._id
                        LIMIT @limit
                        RETURN i
                )
                
                // Outbox
                LET outbox = (
                    FOR o IN outbox_items
                        FILTER o.processId == process._id
                        LIMIT @limit
                        RETURN o
                )
                
                RETURN {
                    file: file,
                    hierarchy: hierarchy,
                    processes: processes,
                    documents: documents,
                    inbox: inbox,
                    outbox: outbox
                }
            ";

            var bindVars = new Dictionary<string, object>
            { 
                ["processId"] = processId,
                ["maxDepth"] = loadDepth,
                ["lazyLoad"] = config.LazyLoadChildren,
                ["limit"] = config.VirtualizeTree ? 1000 : 10000
            };
            
            var result = await _themisDb.QueryAsync<dynamic>(query, bindVars, cancellationToken);
            
            var data = await AsyncEnumerableExtensions.FirstOrDefaultAsync(result, cancellationToken);
            if (data == null)
            {
                throw new InvalidOperationException($"Process {processId} not found");
            }

            // Build tree structure with hierarchy tracking
            var rootNode = BuildFileNodeWithHierarchy(data.file, 0);
            
            // Add hierarchy nodes if available
            if (data.hierarchy != null && data.hierarchy.Length > 0)
            {
                foreach (var hierarchyItem in data.hierarchy)
                {
                    var node = MapToDocumentTreeNodeWithLevel(hierarchyItem.node, hierarchyItem.level, hierarchyItem.path);
                    AddToHierarchy(rootNode, node, hierarchyItem.path);
                }
            }
            else
            {
                // Fallback to simple structure
                rootNode.Children.Add(BuildProcessesNode(data.processes, data.documents));
                rootNode.Children.Add(BuildInboxNode(data.inbox));
                rootNode.Children.Add(BuildOutboxNode(data.outbox));
            }

            return rootNode;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error building document tree for process {ProcessId}", processId);
            throw;
        }
    }

    public async Task<List<DocumentTreeNode>> GetFilteredNodesAsync(DocumentTreeFilter filter, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(filter);

        try
        {
            // Build AQL query with filters
            var conditions = new List<string>();
            var bindVars = new Dictionary<string, object>();

            if (filter.Types.Any())
            {
                conditions.Add("d.type IN @types");
                bindVars["types"] = filter.Types.Select(t => t.ToString()).ToList();
            }

            if (filter.Statuses.Any())
            {
                conditions.Add("d.status IN @statuses");
                bindVars["statuses"] = filter.Statuses.Select(s => s.ToString()).ToList();
            }

            if (filter.DateFrom.HasValue)
            {
                conditions.Add("d.modifiedAt >= @dateFrom");
                bindVars["dateFrom"] = filter.DateFrom.Value;
            }

            if (filter.DateTo.HasValue)
            {
                conditions.Add("d.modifiedAt <= @dateTo");
                bindVars["dateTo"] = filter.DateTo.Value;
            }

            if (!string.IsNullOrEmpty(filter.SearchText))
            {
                conditions.Add("LOWER(d.name) LIKE @searchText");
                bindVars["searchText"] = $"%{filter.SearchText.ToLower()}%";
            }

            var whereClause = conditions.Any() ? "FILTER " + string.Join(" AND ", conditions) : "";

            var query = $@"
                FOR d IN documents
                    {whereClause}
                    LIMIT 1000
                    RETURN d
            ";

            var results = await _themisDb.QueryAsync<dynamic>(query, bindVars, cancellationToken);
            var resultList = await AsyncEnumerableExtensions.ToListAsync(results, cancellationToken);
            
            return resultList.Select(MapToDocumentTreeNode).ToList();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error filtering document tree");
            throw;
        }
    }

    public async Task<List<DocumentTreeNode>> SearchTreeAsync(string query, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(query);

        var filter = new DocumentTreeFilter
        {
            SearchText = query
        };

        return await GetFilteredNodesAsync(filter, cancellationToken);
    }

    public async Task<DocumentTreeNode> GetNodeAsync(string nodeId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(nodeId);

        var query = "RETURN DOCUMENT(@nodeId)";
        var bindVars = new Dictionary<string, object> { ["nodeId"] = nodeId };
        
        var result = await _themisDb.QueryAsync<dynamic>(query, bindVars, cancellationToken);
        var data = await AsyncEnumerableExtensions.FirstOrDefaultAsync(result, cancellationToken);
        
        if (data == null)
        {
            throw new InvalidOperationException($"Node {nodeId} not found");
        }

        return MapToDocumentTreeNode(data);
    }

    public async Task<DocumentTreeNode> ExpandNodeAsync(string nodeId, CancellationToken cancellationToken = default)
    {
        var node = await GetNodeAsync(nodeId, cancellationToken);
        node.IsExpanded = true;
        return node;
    }

    public async Task<DocumentTreeNode> CollapseNodeAsync(string nodeId, CancellationToken cancellationToken = default)
    {
        var node = await GetNodeAsync(nodeId, cancellationToken);
        node.IsExpanded = false;
        return node;
    }

    public async Task<DocumentTreeConfiguration> GetConfigurationAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(userId);

        var query = "FOR u IN users FILTER u._key == @userId RETURN u.documentTreeConfig";
        var bindVars = new Dictionary<string, object> { ["userId"] = userId };
        
        var result = await _themisDb.QueryAsync<DocumentTreeConfiguration>(query, bindVars, cancellationToken);
        return await AsyncEnumerableExtensions.FirstOrDefaultAsync(result, cancellationToken) ?? new DocumentTreeConfiguration();
    }

    public async Task SaveConfigurationAsync(string userId, DocumentTreeConfiguration config, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(userId);
        ArgumentNullException.ThrowIfNull(config);

        var query = @"
            UPDATE @userId WITH { documentTreeConfig: @config } IN users
            RETURN NEW
        ";
        
        var bindVars = new Dictionary<string, object> { ["userId"] = userId, ["config"] = config };
        await _themisDb.ExecuteAsync(query, bindVars, cancellationToken);
        
        _logger.LogInformation("Saved document tree configuration for user {UserId}", userId);
    }

    public async Task<List<TaskBasketItem>> GetTaskBasketItemsAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(userId);

        var query = @"
            LET reminders = (
                FOR r IN reminders
                    FILTER r.assignedTo == @userId AND r.status != 'Completed'
                    RETURN {
                        id: r._key,
                        type: 'Reminder',
                        title: r.subject,
                        processId: r.processId,
                        dueDate: r.dueDate,
                        priority: r.priority,
                        createdAt: r.createdAt
                    }
            )
            
            LET cosigning = (
                FOR c IN cosigning_steps
                    FILTER c.cosignerId == @userId AND c.status == 'Pending'
                    RETURN {
                        id: c._key,
                        type: 'Cosigning',
                        title: c.subject,
                        processId: c.processId,
                        dueDate: c.dueDate,
                        priority: c.priority,
                        createdAt: c.createdAt
                    }
            )
            
            LET inbox = (
                FOR i IN inbox_items
                    FILTER i.assignedTo == @userId AND i.status != 'Completed'
                    RETURN {
                        id: i._key,
                        type: 'InboxItem',
                        title: i.subject,
                        processId: i.processId,
                        dueDate: null,
                        priority: i.priority,
                        createdAt: i.createdAt
                    }
            )
            
            RETURN UNION(reminders, cosigning, inbox)
        ";

        var bindVars = new Dictionary<string, object> { ["userId"] = userId };
        var results = await _themisDb.QueryAsync<dynamic>(query, bindVars, cancellationToken);
        var resultList = await AsyncEnumerableExtensions.ToListAsync(results, cancellationToken);
        
        return resultList.Select(MapToTaskBasketItem).ToList();
    }

    public async Task<TaskBasketStatistics> GetTaskBasketStatisticsAsync(string userId, CancellationToken cancellationToken = default)
    {
        var items = await GetTaskBasketItemsAsync(userId, cancellationToken);
        var now = DateTime.UtcNow;
        
        return new TaskBasketStatistics
        {
            TotalTasks = items.Count,
            OverdueTasks = items.Count(i => i.DueDate.HasValue && i.DueDate.Value < now),
            DueToday = items.Count(i => i.DueDate.HasValue && i.DueDate.Value.Date == now.Date),
            DueThisWeek = items.Count(i => i.DueDate.HasValue && i.DueDate.Value <= now.AddDays(7)),
            HighPriorityTasks = items.Count(i => i.Priority == TaskPriority.High),
            UrgentTasks = items.Count(i => i.Priority == TaskPriority.Urgent)
        };
    }

    public async Task<int> GetOverdueTasksCountAsync(string userId, CancellationToken cancellationToken = default)
    {
        var stats = await GetTaskBasketStatisticsAsync(userId, cancellationToken);
        return stats.OverdueTasks;
    }

    public async Task<DocumentTreeNode> GetNodeWithChildrenAsync(string nodeId, int depth = 1, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(nodeId);

        var query = @"
            LET root = DOCUMENT(@nodeId)
            LET children = (
                FOR v, e, p IN 1..@depth OUTBOUND root._id GRAPH 'document_hierarchy'
                    OPTIONS {uniqueVertices: 'path', bfs: true}
                    RETURN {
                        node: v,
                        level: LENGTH(p.edges),
                        path: p.vertices[*]._id
                    }
            )
            RETURN {root: root, children: children}
        ";

        var bindVars = new Dictionary<string, object> { ["nodeId"] = nodeId, ["depth"] = depth };
        var result = await _themisDb.QueryAsync<dynamic>(query, bindVars, cancellationToken);
        var data = await AsyncEnumerableExtensions.FirstOrDefaultAsync(result, cancellationToken);

        if (data == null)
        {
            throw new InvalidOperationException($"Node {nodeId} not found");
        }

        var rootNode = MapToDocumentTreeNode(data.root);
        
        // Build hierarchy from children
        foreach (var child in data.children)
        {
            var childNode = MapToDocumentTreeNodeWithLevel(child.node, child.level, child.path);
            AddToHierarchy(rootNode, childNode, child.path);
        }

        return rootNode;
    }

    public async Task<List<DocumentTreeNode>> LoadChildrenAsync(string parentId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(parentId);

        var query = @"
            FOR v IN 1..1 OUTBOUND DOCUMENT(@parentId) GRAPH 'document_hierarchy'
                RETURN v
        ";

        var bindVars = new Dictionary<string, object> { ["parentId"] = parentId };
        var results = await _themisDb.QueryAsync<dynamic>(query, bindVars, cancellationToken);
        var resultList = await AsyncEnumerableExtensions.ToListAsync(results, cancellationToken);

        return resultList.Select(MapToDocumentTreeNode).ToList();
    }

    public async Task<List<string>> GetPathToRootAsync(string nodeId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(nodeId);

        var query = @"
            FOR v, e, p IN 0..100 INBOUND DOCUMENT(@nodeId) GRAPH 'document_hierarchy'
                OPTIONS {uniqueVertices: 'path', bfs: true}
                RETURN p.vertices[*]._id
        ";

        var bindVars = new Dictionary<string, object> { ["nodeId"] = nodeId };
        var result = await _themisDb.QueryAsync<List<string>>(query, bindVars, cancellationToken);

        return await AsyncEnumerableExtensions.FirstOrDefaultAsync(result, cancellationToken) ?? new List<string>();
    }

    public async Task<int> GetNodeDepthAsync(string nodeId, CancellationToken cancellationToken = default)
    {
        var path = await GetPathToRootAsync(nodeId, cancellationToken);
        return path.Count - 1; // Subtract 1 because path includes the node itself
    }

    public async Task<bool> ValidateNestingDepthAsync(string parentId, int maxDepth, CancellationToken cancellationToken = default)
    {
        var currentDepth = await GetNodeDepthAsync(parentId, cancellationToken);
        return currentDepth < maxDepth;
    }

    // Helper methods
    private DocumentTreeNode BuildFileNode(dynamic file)
    {
        return BuildFileNodeWithHierarchy(file, 0);
    }

    private DocumentTreeNode BuildFileNodeWithHierarchy(dynamic file, int level)
    {
        return new DocumentTreeNode
        {
            Id = file._key,
            Type = DocumentTreeNodeType.File,
            Name = $"{file.fileNumber} - {file.subject}",
            Icon = "📁",
            Status = Enum.Parse<DocumentStatus>(file.status ?? "New"),
            Level = level,
            ParentId = file.parentId,
            PathToRoot = new List<string> { file._id }
        };
    }

    private DocumentTreeNode MapToDocumentTreeNodeWithLevel(dynamic data, int level, List<string> path)
    {
        var node = MapToDocumentTreeNode(data);
        node.Level = level;
        node.PathToRoot = path;
        return node;
    }

    private void AddToHierarchy(DocumentTreeNode root, DocumentTreeNode node, List<string> path)
    {
        if (path == null || path.Count <= 1)
        {
            root.Children.Add(node);
            return;
        }

        // Find parent in hierarchy
        var parent = FindNodeInTree(root, path[path.Count - 2]);
        if (parent != null)
        {
            parent.Children.Add(node);
            parent.ChildCount = parent.Children.Count;
        }
        else
        {
            root.Children.Add(node);
        }
    }

    private DocumentTreeNode? FindNodeInTree(DocumentTreeNode root, string nodeId)
    {
        if (root.Id == nodeId)
            return root;

        foreach (var child in root.Children)
        {
            var found = FindNodeInTree(child, nodeId);
            if (found != null)
                return found;
        }

        return null;
    }

    private DocumentTreeNode BuildProcessesNode(dynamic[] processes, dynamic[] documents)
    {
        var node = new DocumentTreeNode
        {
            Id = "processes",
            Type = DocumentTreeNodeType.Folder,
            Name = "Vorgänge",
            Icon = "📋",
            ChildCount = processes.Length
        };

        foreach (var process in processes)
        {
            var processNode = new DocumentTreeNode
            {
                Id = process._key,
                Type = DocumentTreeNodeType.Process,
                Name = process.subject,
                Icon = "⚙️",
                ProcessId = process._key,
                FileReference = process.fileReference,
                Status = Enum.Parse<DocumentStatus>(process.status ?? "New")
            };

            var processDocs = documents.Where(d => d.processId == process._id).ToArray();
            processNode.ChildCount = processDocs.Length;

            foreach (var doc in processDocs)
            {
                processNode.Children.Add(MapToDocumentTreeNode(doc));
            }

            node.Children.Add(processNode);
        }

        return node;
    }

    private DocumentTreeNode BuildInboxNode(dynamic[] inboxItems)
    {
        var node = new DocumentTreeNode
        {
            Id = "inbox",
            Type = DocumentTreeNodeType.Inbox,
            Name = "Posteingang",
            Icon = "📥",
            ChildCount = inboxItems.Length
        };

        foreach (var item in inboxItems)
        {
            node.Children.Add(new DocumentTreeNode
            {
                Id = item._key,
                Type = DocumentTreeNodeType.InboxItem,
                Name = item.subject,
                Icon = "✉️",
                ModifiedAt = DateTime.Parse(item.createdAt),
                Status = Enum.Parse<DocumentStatus>(item.status ?? "New")
            });
        }

        return node;
    }

    private DocumentTreeNode BuildOutboxNode(dynamic[] outboxItems)
    {
        var node = new DocumentTreeNode
        {
            Id = "outbox",
            Type = DocumentTreeNodeType.Outbox,
            Name = "Postausgang",
            Icon = "📤",
            ChildCount = outboxItems.Length
        };

        foreach (var item in outboxItems)
        {
            node.Children.Add(new DocumentTreeNode
            {
                Id = item._key,
                Type = DocumentTreeNodeType.OutboxItem,
                Name = item.subject,
                Icon = "✉️",
                ModifiedAt = DateTime.Parse(item.createdAt),
                Status = Enum.Parse<DocumentStatus>(item.status ?? "New")
            });
        }

        return node;
    }

    private DocumentTreeNode MapToDocumentTreeNode(dynamic data)
    {
        return new DocumentTreeNode
        {
            Id = data._key,
            Type = Enum.Parse<DocumentTreeNodeType>(data.type ?? "Document"),
            Name = data.name ?? data.subject,
            Icon = GetIconForType(data.type),
            ModifiedAt = data.modifiedAt != null ? DateTime.Parse(data.modifiedAt) : null,
            Size = data.size,
            Status = Enum.Parse<DocumentStatus>(data.status ?? "New"),
            ProcessId = data.processId,
            FileReference = data.fileReference,
            CreatedBy = data.createdBy
        };
    }

    private TaskBasketItem MapToTaskBasketItem(dynamic data)
    {
        var type = Enum.Parse<TaskBasketItemType>(data.type);
        var priority = data.priority != null ? Enum.Parse<TaskPriority>(data.priority) : TaskPriority.Normal;
        DateTime? dueDate = data.dueDate != null ? DateTime.Parse(data.dueDate.ToString()) : null;
        
        return new TaskBasketItem
        {
            Id = data.id ?? string.Empty,
            Type = type,
            Title = data.title ?? string.Empty,
            ProcessId = data.processId ?? string.Empty,
            DueDate = dueDate,
            Priority = priority,
            IsOverdue = dueDate.HasValue && dueDate.Value < DateTime.UtcNow,
            Icon = GetIconForTaskType(type),
            Color = GetColorForPriority(priority),
            CreatedAt = data.createdAt != null ? DateTime.Parse(data.createdAt.ToString()) : DateTime.UtcNow
        };
    }

    private string GetIconForType(string? type)
    {
        return type switch
        {
            "Document" => "📄",
            "Process" => "⚙️",
            "Folder" => "📁",
            "InboxItem" => "✉️",
            "OutboxItem" => "✉️",
            _ => "📄"
        };
    }

    private string GetIconForTaskType(TaskBasketItemType type)
    {
        return type switch
        {
            TaskBasketItemType.Reminder => "⏰",
            TaskBasketItemType.Cosigning => "✍️",
            TaskBasketItemType.InboxItem => "📥",
            TaskBasketItemType.Task => "📋",
            TaskBasketItemType.Deadline => "⚠️",
            _ => "📋"
        };
    }

    private string GetColorForPriority(TaskPriority priority)
    {
        return priority switch
        {
            TaskPriority.Urgent => "#dc2626",
            TaskPriority.High => "#f97316",
            TaskPriority.Normal => "#22c55e",
            TaskPriority.Low => "#9ca3af",
            _ => "#22c55e"
        };
    }
}
