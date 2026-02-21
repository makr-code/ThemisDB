/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DsmLocalDataStore.cs                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟠 BETA                                         ║
    • Quality Score:   43.0/100                                       ║
    • Total Lines:     338                                            ║
    • Open Issues:     TODOs: 0, Stubs: 12                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔧 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Lokaler DSM-Datastore mit Cache und optionaler Anbindung an ThemisDB.
/// Kann für Debug/Offline mit Stub-Daten befüllt werden und implementiert alle DSM-Store-Interfaces.
/// </summary>
public class DsmLocalDataStore : IDsmMetadataStore, IDsmGraphStore, IDsmVectorStore, IDsmGeoStore, IDsmTimelineStore, IDsmProcessLinkStore
{
    private readonly ConcurrentDictionary<string, DsmMetadataTemplate> _templates = new();
    private readonly ConcurrentDictionary<(string entityType, string entityId), List<DsmMetadataValue>> _values = new();
    private readonly ConcurrentDictionary<string, DsmMetadataAudit> _audits = new();
    private readonly ConcurrentDictionary<string, DsmEntityGraphEdge> _edges = new();
    private readonly ConcurrentDictionary<string, DsmEntityVector> _vectors = new();
    private readonly ConcurrentDictionary<(string entityType, string entityId), DsmEntityGeo> _geo = new();
    private readonly ConcurrentDictionary<string, DsmTimelineItem> _timeline = new();
    private readonly ConcurrentDictionary<string, DsmProcessLink> _processLinks = new();

    private readonly IThemisDBService? _remote; // optional

    public DsmLocalDataStore(IThemisDBService? remote = null)
    {
        _remote = remote;
    }

    private static string EnsureId(string? id) => string.IsNullOrWhiteSpace(id) ? Guid.NewGuid().ToString("N") : id!;

    #region Metadata
    public Task<DsmMetadataTemplate?> GetTemplateAsync(string templateId)
    {
        _templates.TryGetValue(templateId, out var t);
        return Task.FromResult(t);
    }

    public Task<IEnumerable<DsmMetadataTemplate>> ListTemplatesAsync(string? entityType = null)
    {
        var res = _templates.Values.AsEnumerable();
        if (!string.IsNullOrWhiteSpace(entityType))
        {
            res = res.Where(t => t.EntityTypes.Contains(entityType, StringComparer.OrdinalIgnoreCase));
        }
        return Task.FromResult(res);
    }

    public Task<DsmMetadataTemplate> UpsertTemplateAsync(DsmMetadataTemplate template)
    {
        _templates[template.Id] = template;
        return Task.FromResult(template);
    }

    public Task<DsmMetadataValue?> GetCurrentValuesAsync(string entityType, string entityId)
    {
        var key = (entityType, entityId);
        if (_values.TryGetValue(key, out var list))
        {
            return Task.FromResult<DsmMetadataValue?>(list.OrderByDescending(v => v.Version).FirstOrDefault());
        }
        return Task.FromResult<DsmMetadataValue?>(null);
    }

    public Task<IEnumerable<DsmMetadataValue>> GetAllValuesAsync(string entityType, string entityId)
    {
        var key = (entityType, entityId);
        if (_values.TryGetValue(key, out var list))
        {
            return Task.FromResult<IEnumerable<DsmMetadataValue>>(list.OrderByDescending(v => v.Version));
        }
        return Task.FromResult<IEnumerable<DsmMetadataValue>>(Enumerable.Empty<DsmMetadataValue>());
    }

    public Task<DsmMetadataValue> UpsertValuesAsync(DsmMetadataValue value)
    {
        var key = (value.EntityType, value.EntityId);
        var list = _values.GetOrAdd(key, _ => new List<DsmMetadataValue>());
        foreach (var v in list) v.IsCurrent = false;
        value.IsCurrent = true;
        list.Add(value);
        return Task.FromResult(value);
    }

    public Task AddAuditAsync(DsmMetadataAudit audit)
    {
        audit.Id = EnsureId(audit.Id);
        _audits[audit.Id] = audit;
        return Task.CompletedTask;
    }
    #endregion

    #region Graph
    public Task<DsmEntityGraphEdge> AddEdgeAsync(DsmEntityGraphEdge edge)
    {
        edge.Id = EnsureId(edge.Id);
        _edges[edge.Id] = edge;
        return Task.FromResult(edge);
    }

    public Task<IEnumerable<DsmEntityGraphEdge>> GetEdgesAsync(string entityType, string entityId)
    {
        var res = _edges.Values.Where(e => (e.SourceType.Equals(entityType, StringComparison.OrdinalIgnoreCase) && e.SourceId == entityId)
                                     || (e.TargetType.Equals(entityType, StringComparison.OrdinalIgnoreCase) && e.TargetId == entityId));
        return Task.FromResult(res);
    }
    #endregion

    #region Vector
    public Task<DsmEntityVector> AddVectorAsync(DsmEntityVector vector)
    {
        vector.Id = EnsureId(vector.Id);
        _vectors[vector.Id] = vector;
        return Task.FromResult(vector);
    }

    public Task<IEnumerable<DsmEntityVector>> GetVectorsAsync(string entityType, string entityId)
    {
        var res = _vectors.Values.Where(v => v.EntityType.Equals(entityType, StringComparison.OrdinalIgnoreCase) && v.EntityId == entityId);
        return Task.FromResult(res);
    }
    #endregion

    #region Geo
    public Task<DsmEntityGeo> UpsertGeoAsync(DsmEntityGeo geo)
    {
        _geo[(geo.EntityType, geo.EntityId)] = geo;
        return Task.FromResult(geo);
    }

    public Task<DsmEntityGeo?> GetGeoAsync(string entityType, string entityId)
    {
        _geo.TryGetValue((entityType, entityId), out var g);
        return Task.FromResult(g);
    }
    #endregion

    #region Timeline
    public Task<DsmTimelineItem> AddItemAsync(DsmTimelineItem item)
    {
        item.Id = EnsureId(item.Id);
        _timeline[item.Id] = item;
        return Task.FromResult(item);
    }

    public Task<IEnumerable<DsmTimelineItem>> GetItemsAsync(string entityType, string entityId)
    {
        var res = _timeline.Values.Where(i => i.EntityType.Equals(entityType, StringComparison.OrdinalIgnoreCase) && i.EntityId == entityId)
                                   .OrderBy(i => i.Timestamp)
                                   .ToList()
                                   .AsEnumerable();
        return Task.FromResult(res);
    }
    #endregion

    #region Process Links
    public Task AddLinkAsync(DsmProcessLink link)
    {
        link.ProcessId = string.IsNullOrWhiteSpace(link.ProcessId) ? "default" : link.ProcessId;
        var id = EnsureId(link.ProcessId + "::" + link.EntityType + "::" + link.EntityId);
        _processLinks[id] = link;
        return Task.CompletedTask;
    }

    public Task<IEnumerable<DsmProcessLink>> GetLinksAsync(string processId)
    {
        var res = _processLinks.Values.Where(l => l.ProcessId == processId);
        return Task.FromResult(res);
    }
    #endregion

    #region Snapshots
    public Task<IReadOnlyCollection<DsmEntityGraphEdge>> GetAllEdgesAsync()
    {
        return Task.FromResult<IReadOnlyCollection<DsmEntityGraphEdge>>(_edges.Values.ToList());
    }

    public Task<IReadOnlyCollection<DsmEntityVector>> GetAllVectorsAsync()
    {
        return Task.FromResult<IReadOnlyCollection<DsmEntityVector>>(_vectors.Values.ToList());
    }

    public Task<IReadOnlyCollection<DsmEntityGeo>> GetAllGeosAsync()
    {
        return Task.FromResult<IReadOnlyCollection<DsmEntityGeo>>(_geo.Values.ToList());
    }

    public Task<IReadOnlyCollection<DsmTimelineItem>> GetAllTimelineItemsAsync()
    {
        return Task.FromResult<IReadOnlyCollection<DsmTimelineItem>>(_timeline.Values.ToList());
    }

    public Task<IReadOnlyCollection<DsmProcessLink>> GetAllProcessLinksAsync()
    {
        return Task.FromResult<IReadOnlyCollection<DsmProcessLink>>(_processLinks.Values.ToList());
    }
    #endregion

    #region Stub/Sync Helpers
    /// <summary>
    /// Befüllt den lokalen Store mit Stub-Daten (z.B. für Debug/Offline).
    /// </summary>
    public Task LoadStubsAsync(DsmStubData stub)
    {
        foreach (var t in stub.Templates)
        {
            t.Id = EnsureId(t.Id);
            _templates[t.Id] = t;
        }
        foreach (var v in stub.Values)
        {
            v.Id = EnsureId(v.Id);
            var key = (v.EntityType, v.EntityId);
            var list = _values.GetOrAdd(key, _ => new List<DsmMetadataValue>());
            foreach (var vv in list) vv.IsCurrent = false;
            v.IsCurrent = true;
            list.Add(v);
        }
        foreach (var a in stub.Audits)
        {
            a.Id = EnsureId(a.Id);
            _audits[a.Id] = a;
        }
        foreach (var e in stub.Edges)
        {
            e.Id = EnsureId(e.Id);
            _edges[e.Id] = e;
        }
        foreach (var vec in stub.Vectors)
        {
            vec.Id = EnsureId(vec.Id);
            _vectors[vec.Id] = vec;
        }
        foreach (var g in stub.Geos)
        {
            g.Id = EnsureId(g.Id);
            _geo[(g.EntityType, g.EntityId)] = g;
        }
        foreach (var tl in stub.Timeline)
        {
            tl.Id = EnsureId(tl.Id);
            _timeline[tl.Id] = tl;
        }
        foreach (var pl in stub.ProcessLinks)
        {
            pl.ProcessId = string.IsNullOrWhiteSpace(pl.ProcessId) ? "default" : pl.ProcessId;
            var id = EnsureId(pl.ProcessId + "::" + pl.EntityType + "::" + pl.EntityId);
            _processLinks[id] = pl;
        }
        return Task.CompletedTask;
    }

    /// <summary>
    /// Platzhalter für spätere Synchronisation mit ThemisDB.
    /// Aktuell: no-op. Später: _remote nutzen, um Collections zu ziehen und Cache zu füllen.
    /// </summary>
    public async Task SyncFromRemoteAsync(CancellationToken cancellationToken = default)
    {
        if (_remote == null) return;

        // Einfacher Vollabzug; spätere Optimierung via Incremental/updatedAt möglich.
        var templates = await _remote.ExecuteQueryAsync<DsmMetadataTemplate>("FOR d IN dsm_metadata_templates RETURN d", null, cancellationToken);
        foreach (var t in templates)
        {
            t.Id = EnsureId(t.Id);
            _templates[t.Id] = t;
        }

        var values = await _remote.ExecuteQueryAsync<DsmMetadataValue>("FOR d IN dsm_metadata_values RETURN d", null, cancellationToken);
        foreach (var v in values)
        {
            v.Id = EnsureId(v.Id);
            var key = (v.EntityType, v.EntityId);
            var list = _values.GetOrAdd(key, _ => new List<DsmMetadataValue>());
            foreach (var vv in list) vv.IsCurrent = false;
            v.IsCurrent = true;
            list.Add(v);
        }

        var audits = await _remote.ExecuteQueryAsync<DsmMetadataAudit>("FOR d IN dsm_metadata_audits RETURN d", null, cancellationToken);
        foreach (var a in audits)
        {
            a.Id = EnsureId(a.Id);
            _audits[a.Id] = a;
        }

        var edges = await _remote.ExecuteQueryAsync<DsmEntityGraphEdge>("FOR d IN dsm_entity_graph_edges RETURN d", null, cancellationToken);
        foreach (var e in edges)
        {
            e.Id = EnsureId(e.Id);
            _edges[e.Id] = e;
        }

        var vectors = await _remote.ExecuteQueryAsync<DsmEntityVector>("FOR d IN dsm_entity_vectors RETURN d", null, cancellationToken);
        foreach (var v in vectors)
        {
            v.Id = EnsureId(v.Id);
            _vectors[v.Id] = v;
        }

        var geos = await _remote.ExecuteQueryAsync<DsmEntityGeo>("FOR d IN dsm_entity_geo RETURN d", null, cancellationToken);
        foreach (var g in geos)
        {
            g.Id = EnsureId(g.Id);
            _geo[(g.EntityType, g.EntityId)] = g;
        }

        var timeline = await _remote.ExecuteQueryAsync<DsmTimelineItem>("FOR d IN dsm_timeline RETURN d", null, cancellationToken);
        foreach (var t in timeline)
        {
            t.Id = EnsureId(t.Id);
            _timeline[t.Id] = t;
        }

        var processLinks = await _remote.ExecuteQueryAsync<DsmProcessLink>("FOR d IN dsm_process_links RETURN d", null, cancellationToken);
        foreach (var pl in processLinks)
        {
            pl.ProcessId = string.IsNullOrWhiteSpace(pl.ProcessId) ? "default" : pl.ProcessId;
            var id = EnsureId(pl.ProcessId + "::" + pl.EntityType + "::" + pl.EntityId);
            _processLinks[id] = pl;
        }
    }
    #endregion
}

public class DsmStubData
{
    public List<DsmMetadataTemplate> Templates { get; set; } = new();
    public List<DsmMetadataValue> Values { get; set; } = new();
    public List<DsmMetadataAudit> Audits { get; set; } = new();
    public List<DsmEntityGraphEdge> Edges { get; set; } = new();
    public List<DsmEntityVector> Vectors { get; set; } = new();
    public List<DsmEntityGeo> Geos { get; set; } = new();
    public List<DsmTimelineItem> Timeline { get; set; } = new();
    public List<DsmProcessLink> ProcessLinks { get; set; } = new();
}
