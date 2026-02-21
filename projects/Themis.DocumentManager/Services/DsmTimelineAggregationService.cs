/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DsmTimelineAggregationService.cs                   ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     203                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
/// Timeline-Aggregation auf Basis des lokalen DSM-Caches (DsmLocalDataStore).
/// Nutzt ausschließlich DSM-Timeline-Items und wendet einfache Filter/Statistiken an.
/// </summary>
public class DsmTimelineAggregationService : ITimelineAggregationService
{
    private readonly DsmLocalDataStore _store;

    public DsmTimelineAggregationService(DsmLocalDataStore store)
    {
        _store = store;
    }

    public async Task<TimelineAggregationResult> AggregateAllItemsAsync(
        DateTime? startDate = null,
        DateTime? endDate = null,
        TimelineFilter? filter = null,
        CancellationToken cancellationToken = default)
    {
        var items = await LoadDsmTimelineAsync(null, startDate, endDate, cancellationToken);
        var filtered = filter != null ? ApplyFilters(items, filter) : items;
        return BuildAggregationResult(items, filtered);
    }

    public async Task<TimelineAggregationResult> AggregateProcessItemsAsync(
        string processId,
        DateTime? startDate = null,
        DateTime? endDate = null,
        bool showOthersDimmed = true,
        CancellationToken cancellationToken = default)
    {
        var items = await LoadDsmTimelineAsync(processId, startDate, endDate, cancellationToken);

        // mark process items
        ApplyProcessHighlighting(items, processId);
        if (!showOthersDimmed)
        {
            items = items.Where(i => i.IsProcessRelated).ToList();
        }

        return BuildAggregationResult(items, items);
    }

    public Task<List<TimelineItem>> GetInboxItemsAsync(DateTime? startDate = null, DateTime? endDate = null, CancellationToken cancellationToken = default)
        => Task.FromResult(new List<TimelineItem>());

    public Task<List<TimelineItem>> GetReminderItemsAsync(DateTime? startDate = null, DateTime? endDate = null, CancellationToken cancellationToken = default)
        => Task.FromResult(new List<TimelineItem>());

    public Task<List<TimelineItem>> GetProcessEventItemsAsync(string? processId = null, DateTime? startDate = null, DateTime? endDate = null, CancellationToken cancellationToken = default)
        => Task.FromResult(new List<TimelineItem>());

    public Task<List<TimelineItem>> GetDocumentEventItemsAsync(string? processId = null, DateTime? startDate = null, DateTime? endDate = null, CancellationToken cancellationToken = default)
        => Task.FromResult(new List<TimelineItem>());

    public Task<List<TimelineItem>> GetWorkflowItemsAsync(string? processId = null, DateTime? startDate = null, DateTime? endDate = null, CancellationToken cancellationToken = default)
        => Task.FromResult(new List<TimelineItem>());

    public List<TimelineItem> ApplyFilters(List<TimelineItem> items, TimelineFilter filter)
    {
        var query = items.AsEnumerable();

        if (filter.ObjectTypes.Any())
            query = query.Where(i => filter.ObjectTypes.Contains(i.ObjectType));

        if (filter.Priorities.Any())
            query = query.Where(i => filter.Priorities.Contains(i.Priority));

        if (filter.Statuses.Any())
            query = query.Where(i => filter.Statuses.Contains(i.Status));

        if (!string.IsNullOrWhiteSpace(filter.ProcessId))
            query = query.Where(i => i.ProcessId == filter.ProcessId);

        if (filter.ShowOnlyProcessRelated)
            query = query.Where(i => i.IsProcessRelated);

        if (!string.IsNullOrWhiteSpace(filter.SearchText))
        {
            var text = filter.SearchText.Trim().ToLowerInvariant();
            query = query.Where(i => (i.Title + " " + i.Description).ToLowerInvariant().Contains(text));
        }

        return query.ToList();
    }

    public void ApplyProcessHighlighting(List<TimelineItem> items, string processId)
    {
        foreach (var item in items)
        {
            item.IsProcessRelated = string.Equals(item.ProcessId, processId, StringComparison.OrdinalIgnoreCase);
            if (!item.IsProcessRelated)
            {
                item.Color = "#cccccc";
                item.IsHighlighted = false;
            }
            else
            {
                item.IsHighlighted = true;
            }
        }
    }

    public List<TimelineGroup> GroupItemsByDensity(List<TimelineItem> items, TimelineRange range, int densityThreshold = 5)
    {
        // Simple passthrough; grouping not required for DSM cache
        return new List<TimelineGroup>();
    }

    private TimelineAggregationResult BuildAggregationResult(List<TimelineItem> allItems, List<TimelineItem> filtered)
    {
        return new TimelineAggregationResult
        {
            Items = filtered,
            TotalCount = allItems.Count,
            FilteredCount = filtered.Count,
            CountByType = filtered.GroupBy(i => i.ObjectType).ToDictionary(g => g.Key, g => g.Count()),
            CountByPriority = filtered.GroupBy(i => i.Priority).ToDictionary(g => g.Key, g => g.Count()),
            CountByStatus = filtered.GroupBy(i => i.Status).ToDictionary(g => g.Key, g => g.Count())
        };
    }

    private async Task<List<TimelineItem>> LoadDsmTimelineAsync(string? processId, DateTime? start, DateTime? end, CancellationToken cancellationToken)
    {
        var items = await _store.GetAllTimelineItemsAsync();
        cancellationToken.ThrowIfCancellationRequested();

        var mapped = items
            .Where(i => (!start.HasValue || i.Timestamp >= start) && (!end.HasValue || i.Timestamp <= end))
            .Where(i => string.IsNullOrWhiteSpace(processId) || string.Equals(i.ProcessId, processId, StringComparison.OrdinalIgnoreCase))
            .Select(i => new TimelineItem
            {
                Id = i.Id,
                ObjectId = i.EntityId,
                ObjectType = !string.IsNullOrWhiteSpace(i.ProcessId) ? TimelineObjectType.Process : TimelineObjectType.Document,
                Title = i.Title,
                Description = i.Description,
                Date = i.Timestamp,
                Priority = TimelinePriority.Normal,
                Status = TimelineStatus.Open,
                ProcessId = i.ProcessId,
                IconCode = string.IsNullOrWhiteSpace(i.Icon) ? "📌" : i.Icon,
                Color = string.IsNullOrWhiteSpace(i.Category) ? "#3b82f6" : MapCategoryColor(i.Category),
                IsProcessRelated = !string.IsNullOrWhiteSpace(i.ProcessId),
                Metadata = new Dictionary<string, object>
                {
                    {"entityType", i.EntityType},
                    {"entityId", i.EntityId},
                    {"payload", i.PayloadJson}
                }
            })
            .OrderBy(i => i.Date)
            .ToList();

        return mapped;
    }

    private static string MapCategoryColor(string category)
    {
        return category.ToLowerInvariant() switch
        {
            "milestone" => "#10b981",
            "statuschange" => "#6366f1",
            "note" => "#f59e0b",
            _ => "#3b82f6"
        };
    }
}
