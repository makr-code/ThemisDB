/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AdministrativeStructureService.cs                  ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     747                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service für die Verwaltung der behördlichen Aktenstruktur
/// </summary>
public interface IAdministrativeStructureService
{
    // Authority
    Task<Authority> CreateAuthorityAsync(Authority authority);
    Task<Authority?> GetAuthorityByIdAsync(string id);
    Task<IEnumerable<Authority>> GetAllAuthoritiesAsync();
    
    // Filing
    Task<Filing> CreateFilingAsync(Filing filing);
    Task<IEnumerable<Filing>> GetFilingsByAuthorityAsync(string authorityId);
    
    // File
    Task<AdministrativeFile> CreateFileAsync(AdministrativeFile file);
    Task<AdministrativeFile?> GetFileByIdAsync(string fileId);
    Task<AdministrativeFile?> GetFileByNumberAsync(string fileNumber);
    Task<IEnumerable<AdministrativeFile>> GetFilesByFilingAsync(string filingId);
    Task<bool> CloseFileAsync(string fileId, DateTime closedAt);
    Task<bool> ArchiveFileAsync(string fileId);
    
    // SubFile
    Task<SubFile> CreateSubFileAsync(SubFile subFile);
    Task<IEnumerable<SubFile>> GetSubFilesByParentAsync(string parentFileId);
    
    // Process
    Task<AdministrativeProcess> CreateProcessAsync(AdministrativeProcess process);
    Task<AdministrativeProcess?> GetProcessAsync(string processId);
    Task<AdministrativeProcess?> GetProcessByIdAsync(string processId);
    Task<IEnumerable<AdministrativeProcess>> GetProcessesByFileAsync(string fileId);
    Task<bool> UpdateProcessStatusAsync(string processId, ProcessStatus status);
    Task<bool> AssignProcessAsync(string processId, string assignedTo);
    
    // Document
    Task<AdministrativeDocument> CreateDocumentAsync(AdministrativeDocument document);
    Task<AdministrativeDocument?> GetDocumentByIdAsync(string documentId);
    Task<IEnumerable<AdministrativeDocument>> GetDocumentsByProcessAsync(string processId);
    Task<bool> SignDocumentAsync(string documentId, Signature signature);
    
    // FileAttachment
    Task<FileAttachment> CreateAttachmentAsync(FileAttachment attachment);
    Task<IEnumerable<FileAttachment>> GetAttachmentsByDocumentAsync(string documentId);
}

/// <summary>
/// Service für die Prozess-Timeline
/// </summary>
public interface IProcessTimelineService
{
    Task<ProcessTimelineEvent> CreateEventAsync(ProcessTimelineEvent timelineEvent, CancellationToken cancellationToken = default);
    Task<IEnumerable<ProcessTimelineEvent>> GetAllEventsAsync();
    Task<IEnumerable<ProcessTimelineEvent>> GetEventsByFileAsync(string fileId, DateTime? startDate = null, DateTime? endDate = null);
    Task<IEnumerable<ProcessTimelineEvent>> GetEventsByProcessAsync(string processId, DateTime? startDate = null);
    Task<IEnumerable<ProcessTimelineEvent>> GetEventsByAuthorityAsync(string authorityId, DateTime? startDate = null, DateTime? endDate = null);
    Task<IEnumerable<ProcessTimelineEvent>> GetEventsByActorAsync(string actor, DateTime? startDate = null, DateTime? endDate = null);
    Task<IEnumerable<ProcessTimelineEvent>> GetEventsByTypeAsync(ProcessEventType eventType, DateTime? startDate = null, DateTime? endDate = null);
}

/// <summary>
/// Implementierung des Administrative Structure Service
/// </summary>
public class AdministrativeStructureService : IAdministrativeStructureService
{
    private readonly IThemisApiClient _apiClient;
    private readonly IProcessTimelineService _timelineService;

    public AdministrativeStructureService(
        IThemisApiClient apiClient,
        IProcessTimelineService timelineService)
    {
        _apiClient = apiClient;
        _timelineService = timelineService;
    }

    #region Authority

    public async Task<Authority> CreateAuthorityAsync(Authority authority)
    {
        authority.Id = authority.Id == string.Empty ? Guid.NewGuid().ToString() : authority.Id;
        authority.CreatedAt = DateTime.UtcNow;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{authority.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(authority) }
        );

        return authority;
    }

    public async Task<Authority?> GetAuthorityByIdAsync(string id)
    {
        var urn = $"urn:themis:authority:{id}";
        return await _apiClient.GetAsync<Authority>($"/entities/{urn}");
    }

    public async Task<IEnumerable<Authority>> GetAllAuthoritiesAsync()
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<Authority>>(
            "/query/aql",
            new
            {
                query = "FOR auth IN authorities RETURN auth"
            }
        );

        return response?.Results ?? Enumerable.Empty<Authority>();
    }

    #endregion

    #region Filing

    public async Task<Filing> CreateFilingAsync(Filing filing)
    {
        filing.Id = filing.Id == string.Empty ? Guid.NewGuid().ToString() : filing.Id;
        filing.CreatedAt = DateTime.UtcNow;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{filing.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(filing) }
        );

        return filing;
    }

    public async Task<IEnumerable<Filing>> GetFilingsByAuthorityAsync(string authorityId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<Filing>>(
            "/query/aql",
            new
            {
                query = "FOR filing IN filings FILTER filing.authorityId == @authorityId RETURN filing",
                bindVars = new { authorityId }
            }
        );

        return response?.Results ?? Enumerable.Empty<Filing>();
    }

    #endregion

    #region File

    public async Task<AdministrativeFile> CreateFileAsync(AdministrativeFile file)
    {
        file.Id = file.Id == string.Empty ? Guid.NewGuid().ToString() : file.Id;
        file.OpenedAt = DateTime.UtcNow;
        file.Status = FileStatus.Active;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{file.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(file) }
        );

        // Timeline Event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            AuthorityId = file.AuthorityId,
            FilingId = file.FilingId,
            FileId = file.Id,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.FileCreated,
            Description = $"Akte {file.FileNumber} angelegt: {file.Subject}",
            Actor = file.ResponsibleOfficer
        });

        return file;
    }

    public async Task<AdministrativeFile?> GetFileByIdAsync(string fileId)
    {
        // Need to construct URN - requires authorityId and filingId
        // For now, query by ID
        var response = await _apiClient.PostAsync<object, QueryResponse<AdministrativeFile>>(
            "/query/aql",
            new
            {
                query = "FOR file IN administrative_files FILTER file.id == @fileId RETURN file",
                bindVars = new { fileId }
            }
        );

        return response?.Results?.FirstOrDefault();
    }

    public async Task<AdministrativeFile?> GetFileByNumberAsync(string fileNumber)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<AdministrativeFile>>(
            "/query/aql",
            new
            {
                query = "FOR file IN administrative_files FILTER file.fileNumber == @fileNumber RETURN file",
                bindVars = new { fileNumber }
            }
        );

        return response?.Results?.FirstOrDefault();
    }

    public async Task<IEnumerable<AdministrativeFile>> GetFilesByFilingAsync(string filingId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<AdministrativeFile>>(
            "/query/aql",
            new
            {
                query = "FOR file IN administrative_files FILTER file.filingId == @filingId RETURN file",
                bindVars = new { filingId }
            }
        );

        return response?.Results ?? Enumerable.Empty<AdministrativeFile>();
    }

    public async Task<bool> CloseFileAsync(string fileId, DateTime closedAt)
    {
        var file = await GetFileByIdAsync(fileId);
        if (file == null) return false;

        file.Status = FileStatus.Closed;
        file.ClosedAt = closedAt;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{file.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(file) }
        );

        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            AuthorityId = file.AuthorityId,
            FilingId = file.FilingId,
            FileId = file.Id,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.FileClosed,
            Description = $"Akte {file.FileNumber} geschlossen"
        });

        return true;
    }

    public async Task<bool> ArchiveFileAsync(string fileId)
    {
        var file = await GetFileByIdAsync(fileId);
        if (file == null) return false;

        file.Status = FileStatus.Archived;
        file.ArchiveDate = DateTime.UtcNow;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{file.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(file) }
        );

        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            AuthorityId = file.AuthorityId,
            FilingId = file.FilingId,
            FileId = file.Id,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.FileArchived,
            Description = $"Akte {file.FileNumber} archiviert"
        });

        return true;
    }

    #endregion

    #region SubFile

    public async Task<SubFile> CreateSubFileAsync(SubFile subFile)
    {
        subFile.Id = subFile.Id == string.Empty ? Guid.NewGuid().ToString() : subFile.Id;
        subFile.CreatedAt = DateTime.UtcNow;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{subFile.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(subFile) }
        );

        return subFile;
    }

    public async Task<IEnumerable<SubFile>> GetSubFilesByParentAsync(string parentFileId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<SubFile>>(
            "/query/aql",
            new
            {
                query = "FOR subfile IN subfiles FILTER subfile.parentFileId == @parentFileId RETURN subfile",
                bindVars = new { parentFileId }
            }
        );

        return response?.Results ?? Enumerable.Empty<SubFile>();
    }

    #endregion

    #region Process

    public async Task<AdministrativeProcess> CreateProcessAsync(AdministrativeProcess process)
    {
        process.Id = process.Id == string.Empty ? Guid.NewGuid().ToString() : process.Id;
        process.CreatedAt = DateTime.UtcNow;
        process.Status = ProcessStatus.Draft;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{process.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(process) }
        );

        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            AuthorityId = process.AuthorityId,
            FilingId = process.FilingId,
            FileId = process.FileId,
            ProcessId = process.Id,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.ProcessCreated,
            Description = $"Vorgang {process.ProcessNumber} angelegt: {process.Subject}",
            Actor = process.InitiatedBy
        });

        return process;
    }

    public async Task<AdministrativeProcess?> GetProcessByIdAsync(string processId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<AdministrativeProcess>>(
            "/query/aql",
            new
            {
                query = "FOR process IN administrative_processes FILTER process.id == @processId RETURN process",
                bindVars = new { processId }
            }
        );

        return response?.Results?.FirstOrDefault();
    }

    public async Task<AdministrativeProcess?> GetProcessAsync(string processId)
    {
        return await GetProcessByIdAsync(processId);
    }

    public async Task<IEnumerable<AdministrativeProcess>> GetProcessesByFileAsync(string fileId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<AdministrativeProcess>>(
            "/query/aql",
            new
            {
                query = "FOR process IN administrative_processes FILTER process.fileId == @fileId RETURN process",
                bindVars = new { fileId }
            }
        );

        return response?.Results ?? Enumerable.Empty<AdministrativeProcess>();
    }

    public async Task<bool> UpdateProcessStatusAsync(string processId, ProcessStatus status)
    {
        var process = await GetProcessByIdAsync(processId);
        if (process == null) return false;

        var oldStatus = process.Status;
        process.Status = status;

        if (status == ProcessStatus.Completed)
            process.CompletedAt = DateTime.UtcNow;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{process.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(process) }
        );

        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            AuthorityId = process.AuthorityId,
            FilingId = process.FilingId,
            FileId = process.FileId,
            ProcessId = process.Id,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.ProcessStatusChanged,
            Description = $"Status geändert: {oldStatus} → {status}",
            PreviousValues = new Dictionary<string, object> { ["status"] = oldStatus },
            NewValues = new Dictionary<string, object> { ["status"] = status }
        });

        return true;
    }

    public async Task<bool> AssignProcessAsync(string processId, string assignedTo)
    {
        var process = await GetProcessByIdAsync(processId);
        if (process == null) return false;

        var previousAssignee = process.AssignedTo;
        process.AssignedTo = assignedTo;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{process.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(process) }
        );

        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            AuthorityId = process.AuthorityId,
            FilingId = process.FilingId,
            FileId = process.FileId,
            ProcessId = process.Id,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.ProcessAssigned,
            Description = $"Vorgang zugewiesen an: {assignedTo}",
            PreviousValues = new Dictionary<string, object> { ["assignedTo"] = previousAssignee },
            NewValues = new Dictionary<string, object> { ["assignedTo"] = assignedTo }
        });

        return true;
    }

    #endregion

    #region Document

    public async Task<AdministrativeDocument> CreateDocumentAsync(AdministrativeDocument document)
    {
        document.Id = document.Id == string.Empty ? Guid.NewGuid().ToString() : document.Id;
        document.CreatedAt = DateTime.UtcNow;
        document.Status = DocumentLifecycleStatus.Draft;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{document.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(document) }
        );

        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            AuthorityId = document.AuthorityId,
            FilingId = document.FilingId,
            FileId = document.FileId,
            ProcessId = document.ProcessId,
            DocumentId = document.Id,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.DocumentCreated,
            Description = $"Dokument erstellt: {document.Title}",
            Actor = document.Author
        });

        return document;
    }

    public async Task<AdministrativeDocument?> GetDocumentByIdAsync(string documentId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<AdministrativeDocument>>(
            "/query/aql",
            new
            {
                query = "FOR doc IN administrative_documents FILTER doc.id == @documentId RETURN doc",
                bindVars = new { documentId }
            }
        );

        return response?.Results?.FirstOrDefault();
    }

    public async Task<IEnumerable<AdministrativeDocument>> GetDocumentsByProcessAsync(string processId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<AdministrativeDocument>>(
            "/query/aql",
            new
            {
                query = "FOR doc IN administrative_documents FILTER doc.processId == @processId RETURN doc",
                bindVars = new { processId }
            }
        );

        return response?.Results ?? Enumerable.Empty<AdministrativeDocument>();
    }

    public async Task<bool> SignDocumentAsync(string documentId, Signature signature)
    {
        var document = await GetDocumentByIdAsync(documentId);
        if (document == null) return false;

        document.Signatures.Add(signature);
        document.Status = DocumentLifecycleStatus.Signed;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{document.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(document) }
        );

        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            AuthorityId = document.AuthorityId,
            FilingId = document.FilingId,
            FileId = document.FileId,
            ProcessId = document.ProcessId,
            DocumentId = document.Id,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.DocumentSigned,
            Description = $"Dokument unterschrieben von {signature.SignerName}",
            Actor = signature.SignerId
        });

        return true;
    }

    #endregion

    #region FileAttachment

    public async Task<FileAttachment> CreateAttachmentAsync(FileAttachment attachment)
    {
        attachment.Id = attachment.Id == string.Empty ? Guid.NewGuid().ToString() : attachment.Id;
        attachment.UploadedAt = DateTime.UtcNow;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{attachment.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(attachment) }
        );

        return attachment;
    }

    public async Task<IEnumerable<FileAttachment>> GetAttachmentsByDocumentAsync(string documentId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<FileAttachment>>(
            "/query/aql",
            new
            {
                query = "FOR att IN file_attachments FILTER att.documentId == @documentId RETURN att",
                bindVars = new { documentId }
            }
        );

        return response?.Results ?? Enumerable.Empty<FileAttachment>();
    }

    #endregion

    private class QueryResponse<T>
    {
        public List<T> Results { get; set; } = new();
    }
}

/// <summary>
/// Implementierung des Process Timeline Service
/// </summary>
public class ProcessTimelineService : IProcessTimelineService
{
    private readonly IThemisApiClient _apiClient;

    public ProcessTimelineService(IThemisApiClient apiClient)
    {
        _apiClient = apiClient;
    }

    public async Task<ProcessTimelineEvent> CreateEventAsync(ProcessTimelineEvent timelineEvent, CancellationToken cancellationToken = default)
    {
        timelineEvent.Id = timelineEvent.Id == string.Empty ? Guid.NewGuid().ToString() : timelineEvent.Id;
        timelineEvent.Timestamp = timelineEvent.Timestamp == default ? DateTime.UtcNow : timelineEvent.Timestamp;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{timelineEvent.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(timelineEvent) }
        );

        return timelineEvent;
    }

    public async Task<IEnumerable<ProcessTimelineEvent>> GetEventsByFileAsync(string fileId, DateTime? startDate = null, DateTime? endDate = null)
    {
        var query = "FOR event IN process_timeline_events FILTER event.fileId == @fileId";
        
        if (startDate.HasValue)
            query += " AND event.timestamp >= @startDate";
        if (endDate.HasValue)
            query += " AND event.timestamp <= @endDate";
            
        query += " SORT event.timestamp DESC RETURN event";

        var bindVars = new Dictionary<string, object> { ["fileId"] = fileId };
        if (startDate.HasValue) bindVars["startDate"] = startDate.Value.ToString("yyyy-MM-ddTHH:mm:ssZ");
        if (endDate.HasValue) bindVars["endDate"] = endDate.Value.ToString("yyyy-MM-ddTHH:mm:ssZ");

        var response = await _apiClient.PostAsync<object, QueryResponse<ProcessTimelineEvent>>(
            "/query/aql",
            new { query, bindVars }
        );

        return response?.Results ?? Enumerable.Empty<ProcessTimelineEvent>();
    }

    public async Task<IEnumerable<ProcessTimelineEvent>> GetEventsByProcessAsync(string processId, DateTime? startDate = null)
    {
        var query = "FOR event IN process_timeline_events FILTER event.processId == @processId";
        var bindVars = new Dictionary<string, object> { ["processId"] = processId };
        
        if (startDate.HasValue)
        {
            query += " AND event.timestamp >= @startDate";
            bindVars["startDate"] = startDate.Value.ToString("yyyy-MM-ddTHH:mm:ssZ");
        }
        
        query += " SORT event.timestamp DESC RETURN event";

        var response = await _apiClient.PostAsync<object, QueryResponse<ProcessTimelineEvent>>(
            "/query/aql",
            new { query, bindVars }
        );

        return response?.Results ?? Enumerable.Empty<ProcessTimelineEvent>();
    }

    public async Task<IEnumerable<ProcessTimelineEvent>> GetAllEventsAsync()
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<ProcessTimelineEvent>>(
            "/query/aql",
            new
            {
                query = "FOR event IN process_timeline_events SORT event.timestamp DESC RETURN event",
                bindVars = new { }
            }
        );

        return response?.Results ?? Enumerable.Empty<ProcessTimelineEvent>();
    }

    public async Task<IEnumerable<ProcessTimelineEvent>> GetEventsByAuthorityAsync(string authorityId, DateTime? startDate = null, DateTime? endDate = null)
    {
        var query = "FOR event IN process_timeline_events FILTER event.authorityId == @authorityId";
        
        if (startDate.HasValue)
            query += " AND event.timestamp >= @startDate";
        if (endDate.HasValue)
            query += " AND event.timestamp <= @endDate";
            
        query += " SORT event.timestamp DESC RETURN event";

        var bindVars = new Dictionary<string, object> { ["authorityId"] = authorityId };
        if (startDate.HasValue) bindVars["startDate"] = startDate.Value.ToString("yyyy-MM-ddTHH:mm:ssZ");
        if (endDate.HasValue) bindVars["endDate"] = endDate.Value.ToString("yyyy-MM-ddTHH:mm:ssZ");

        var response = await _apiClient.PostAsync<object, QueryResponse<ProcessTimelineEvent>>(
            "/query/aql",
            new { query, bindVars }
        );

        return response?.Results ?? Enumerable.Empty<ProcessTimelineEvent>();
    }

    public async Task<IEnumerable<ProcessTimelineEvent>> GetEventsByActorAsync(string actor, DateTime? startDate = null, DateTime? endDate = null)
    {
        var query = "FOR event IN process_timeline_events FILTER event.actor == @actor";
        
        if (startDate.HasValue)
            query += " AND event.timestamp >= @startDate";
        if (endDate.HasValue)
            query += " AND event.timestamp <= @endDate";
            
        query += " SORT event.timestamp DESC RETURN event";

        var bindVars = new Dictionary<string, object> { ["actor"] = actor };
        if (startDate.HasValue) bindVars["startDate"] = startDate.Value.ToString("yyyy-MM-ddTHH:mm:ssZ");
        if (endDate.HasValue) bindVars["endDate"] = endDate.Value.ToString("yyyy-MM-ddTHH:mm:ssZ");

        var response = await _apiClient.PostAsync<object, QueryResponse<ProcessTimelineEvent>>(
            "/query/aql",
            new { query, bindVars }
        );

        return response?.Results ?? Enumerable.Empty<ProcessTimelineEvent>();
    }

    public async Task<IEnumerable<ProcessTimelineEvent>> GetEventsByTypeAsync(ProcessEventType eventType, DateTime? startDate = null, DateTime? endDate = null)
    {
        var query = "FOR event IN process_timeline_events FILTER event.eventType == @eventType";
        
        if (startDate.HasValue)
            query += " AND event.timestamp >= @startDate";
        if (endDate.HasValue)
            query += " AND event.timestamp <= @endDate";
            
        query += " SORT event.timestamp DESC RETURN event";

        var bindVars = new Dictionary<string, object> { ["eventType"] = eventType.ToString() };
        if (startDate.HasValue) bindVars["startDate"] = startDate.Value.ToString("yyyy-MM-ddTHH:mm:ssZ");
        if (endDate.HasValue) bindVars["endDate"] = endDate.Value.ToString("yyyy-MM-ddTHH:mm:ssZ");

        var response = await _apiClient.PostAsync<object, QueryResponse<ProcessTimelineEvent>>(
            "/query/aql",
            new { query, bindVars }
        );

        return response?.Results ?? Enumerable.Empty<ProcessTimelineEvent>();
    }

    private class QueryResponse<T>
    {
        public List<T> Results { get; set; } = new();
    }
}
