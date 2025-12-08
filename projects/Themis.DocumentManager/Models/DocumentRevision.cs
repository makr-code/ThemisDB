using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Document revision for audit trail and version control
/// </summary>
public class DocumentRevision
{
    public string Id { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty;
    public int RevisionNumber { get; set; }
    public DateTime CreatedAt { get; set; }
    public string Author { get; set; } = string.Empty;
    public string Comment { get; set; } = string.Empty;
    public string FilePath { get; set; } = string.Empty;
    public string? FileHash { get; set; }
    public long FileSize { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}
