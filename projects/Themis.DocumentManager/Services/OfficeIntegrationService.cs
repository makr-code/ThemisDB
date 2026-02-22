/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            OfficeIntegrationService.cs                        ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     703                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Office integration service interface
/// </summary>
public interface IOfficeIntegrationService
{
    Task<OfficeDocumentResult> CreateNewWordDocumentAsync(string? templatePath = null);
    Task<OfficeDocumentResult> OpenWordDocumentAsync(string documentPath);
    Task<OfficeDocumentResult> CreateNewExcelWorkbookAsync(string? templatePath = null);
    Task<OfficeDocumentResult> OpenExcelWorkbookAsync(string workbookPath);
    Task<OfficeDocumentResult> CreateNewPowerPointPresentationAsync(string? templatePath = null);
    Task<OfficeDocumentResult> OpenPowerPointPresentationAsync(string presentationPath);
    Task<OfficeDocumentResult> CreateNewOutlookEmailAsync();
    Task<OfficeDocumentResult> CreateNewOneNotePageAsync(string? notebookName = null);
    Task<bool> SaveDocumentRevisionAsync(string documentPath, string documentId);
    Task<IEnumerable<DocumentRevision>> GetDocumentRevisionsAsync(string documentId);
}

/// <summary>
/// Result of Office document operation
/// </summary>
public class OfficeDocumentResult
{
    public bool Success { get; set; }
    public string? DocumentPath { get; set; }
    public string? DocumentId { get; set; }
    public string? ErrorMessage { get; set; }
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

/// <summary>
/// Office integration service implementation with revision-safe processing
/// Implements seamless integration with Microsoft Office applications
/// and ensures revision tracking for compliance
/// </summary>
public class OfficeIntegrationService : IOfficeIntegrationService
{
    private readonly IDocumentService _documentService;
    private readonly IRevisionService _revisionService;
    private readonly string _workingDirectory;

    public OfficeIntegrationService(
        IDocumentService documentService,
        IRevisionService revisionService)
    {
        _documentService = documentService;
        _revisionService = revisionService;
        _workingDirectory = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),
            "ThemisDB", "Documents");
        
        Directory.CreateDirectory(_workingDirectory);
    }

    #region Word Integration

    public async Task<OfficeDocumentResult> CreateNewWordDocumentAsync(string? templatePath = null)
    {
        try
        {
            // Create Word application instance via COM Interop
            dynamic? wordApp = null;
            dynamic? document = null;

            try
            {
                var wordType = Type.GetTypeFromProgID("Word.Application");
                if (wordType == null)
                    return new OfficeDocumentResult 
                    { 
                        Success = false, 
                        ErrorMessage = "Microsoft Word is not installed" 
                    };

                wordApp = Activator.CreateInstance(wordType);
                if (wordApp == null)
                    return new OfficeDocumentResult { Success = false, ErrorMessage = "Failed to create Word instance" };
                    
                wordApp.Visible = true;

                // Create new document from template or blank
                if (!string.IsNullOrEmpty(templatePath) && File.Exists(templatePath))
                {
                    document = wordApp.Documents.Add(templatePath);
                }
                else
                {
                    document = wordApp.Documents.Add();
                }

                // Generate document path
                var fileName = $"Document_{DateTime.Now:yyyyMMdd_HHmmss}.docx";
                var documentPath = Path.Combine(_workingDirectory, fileName);

                // Save document
                document.SaveAs2(documentPath);

                // Create document entry in ThemisDB
                var doc = new Document
                {
                    Id = Guid.NewGuid().ToString(),
                    Title = Path.GetFileNameWithoutExtension(fileName),
                    Filename = fileName,
                    MimeType = "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
                    CreatedAt = DateTime.UtcNow,
                    ModifiedAt = DateTime.UtcNow,
                    BlobPath = documentPath,
                    Metadata = new Dictionary<string, object>
                    {
                        ["OfficeApplication"] = "Word",
                        ["OfficeVersion"] = wordApp.Version,
                        ["RevisionTracking"] = true
                    }
                };

                await _documentService.CreateDocumentAsync(doc);

                // Create initial revision
                await _revisionService.CreateRevisionAsync(new DocumentRevision
                {
                    Id = Guid.NewGuid().ToString(),
                    DocumentId = doc.Id,
                    RevisionNumber = 1,
                    CreatedAt = DateTime.UtcNow,
                    Author = Environment.UserName,
                    Comment = "Initial document creation",
                    FilePath = documentPath
                });

                // Enable track changes for revision safety
                document.TrackRevisions = true;
                document.Save();

                return new OfficeDocumentResult
                {
                    Success = true,
                    DocumentPath = documentPath,
                    DocumentId = doc.Id
                };
            }
            finally
            {
                // Cleanup COM objects
                if (document != null) Marshal.ReleaseComObject(document);
                // Note: Don't close wordApp as it should remain visible for user
            }
        }
        catch (Exception ex)
        {
            return new OfficeDocumentResult
            {
                Success = false,
                ErrorMessage = $"Error creating Word document: {ex.Message}"
            };
        }
    }

    public async Task<OfficeDocumentResult> OpenWordDocumentAsync(string documentPath)
    {
        try
        {
            var wordType = Type.GetTypeFromProgID("Word.Application");
            if (wordType == null)
                return new OfficeDocumentResult 
                { 
                    Success = false, 
                    ErrorMessage = "Microsoft Word is not installed" 
                };

            var wordAppObj = Activator.CreateInstance(wordType);
            if (wordAppObj == null)
                return new OfficeDocumentResult { Success = false, ErrorMessage = "Failed to create Word instance" };
            dynamic wordApp = wordAppObj;
                
            wordApp.Visible = true;

            var document = wordApp.Documents.Open(documentPath);
            if (document == null)
                return new OfficeDocumentResult { Success = false, ErrorMessage = "Failed to open document" };
            
            // Enable track changes for revision safety
            document.TrackRevisions = true;

            return new OfficeDocumentResult
            {
                Success = true,
                DocumentPath = documentPath
            };
        }
        catch (Exception ex)
        {
            return new OfficeDocumentResult
            {
                Success = false,
                ErrorMessage = $"Error opening Word document: {ex.Message}"
            };
        }
    }

    #endregion

    #region Excel Integration

    public async Task<OfficeDocumentResult> CreateNewExcelWorkbookAsync(string? templatePath = null)
    {
        try
        {
            var excelType = Type.GetTypeFromProgID("Excel.Application");
            if (excelType == null)
                return new OfficeDocumentResult 
                { 
                    Success = false, 
                    ErrorMessage = "Microsoft Excel is not installed" 
                };

            var excelAppObj = Activator.CreateInstance(excelType);
            if (excelAppObj == null)
                return new OfficeDocumentResult { Success = false, ErrorMessage = "Failed to create Excel instance" };
            dynamic excelApp = excelAppObj;
                
            excelApp.Visible = true;

            dynamic workbook;
            if (!string.IsNullOrEmpty(templatePath) && File.Exists(templatePath))
            {
                workbook = excelApp.Workbooks.Add(templatePath);
            }
            else
            {
                workbook = excelApp.Workbooks.Add();
            }

            var fileName = $"Workbook_{DateTime.Now:yyyyMMdd_HHmmss}.xlsx";
            var workbookPath = Path.Combine(_workingDirectory, fileName);

            workbook.SaveAs(workbookPath);

            // Create document entry in ThemisDB
            var doc = new Document
            {
                Id = Guid.NewGuid().ToString(),
                Title = Path.GetFileNameWithoutExtension(fileName),
                Filename = fileName,
                MimeType = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
                CreatedAt = DateTime.UtcNow,
                ModifiedAt = DateTime.UtcNow,
                BlobPath = workbookPath,
                Metadata = new Dictionary<string, object>
                {
                    ["OfficeApplication"] = "Excel",
                    ["OfficeVersion"] = excelApp.Version,
                    ["RevisionTracking"] = true
                }
            };

            await _documentService.CreateDocumentAsync(doc);

            // Create initial revision
            await _revisionService.CreateRevisionAsync(new DocumentRevision
            {
                Id = Guid.NewGuid().ToString(),
                DocumentId = doc.Id,
                RevisionNumber = 1,
                CreatedAt = DateTime.UtcNow,
                Author = Environment.UserName,
                Comment = "Initial workbook creation",
                FilePath = workbookPath
            });

            // Enable change tracking
            workbook.TrackRevisions = true;
            workbook.Save();

            return new OfficeDocumentResult
            {
                Success = true,
                DocumentPath = workbookPath,
                DocumentId = doc.Id
            };
        }
        catch (Exception ex)
        {
            return new OfficeDocumentResult
            {
                Success = false,
                ErrorMessage = $"Error creating Excel workbook: {ex.Message}"
            };
        }
    }

    public async Task<OfficeDocumentResult> OpenExcelWorkbookAsync(string workbookPath)
    {
        try
        {
            var excelType = Type.GetTypeFromProgID("Excel.Application");
            if (excelType == null)
                return new OfficeDocumentResult 
                { 
                    Success = false, 
                    ErrorMessage = "Microsoft Excel is not installed" 
                };

            var excelAppObj = Activator.CreateInstance(excelType);
            if (excelAppObj == null)
                return new OfficeDocumentResult { Success = false, ErrorMessage = "Failed to create Excel instance" };
            dynamic excelApp = excelAppObj;
                
            excelApp.Visible = true;

            var workbook = excelApp.Workbooks.Open(workbookPath);
            workbook.TrackRevisions = true;

            return new OfficeDocumentResult
            {
                Success = true,
                DocumentPath = workbookPath
            };
        }
        catch (Exception ex)
        {
            return new OfficeDocumentResult
            {
                Success = false,
                ErrorMessage = $"Error opening Excel workbook: {ex.Message}"
            };
        }
    }

    #endregion

    #region PowerPoint Integration

    public async Task<OfficeDocumentResult> CreateNewPowerPointPresentationAsync(string? templatePath = null)
    {
        try
        {
            var pptType = Type.GetTypeFromProgID("PowerPoint.Application");
            if (pptType == null)
                return new OfficeDocumentResult 
                { 
                    Success = false, 
                    ErrorMessage = "Microsoft PowerPoint is not installed" 
                };

            var pptAppObj = Activator.CreateInstance(pptType);
            if (pptAppObj == null)
                return new OfficeDocumentResult { Success = false, ErrorMessage = "Failed to create PowerPoint instance" };
            dynamic pptApp = pptAppObj;
                
            pptApp.Visible = true;

            dynamic presentation;
            if (!string.IsNullOrEmpty(templatePath) && File.Exists(templatePath))
            {
                presentation = pptApp.Presentations.Open(templatePath);
            }
            else
            {
                presentation = pptApp.Presentations.Add();
            }

            var fileName = $"Presentation_{DateTime.Now:yyyyMMdd_HHmmss}.pptx";
            var presentationPath = Path.Combine(_workingDirectory, fileName);

            presentation.SaveAs(presentationPath);

            // Create document entry in ThemisDB
            var doc = new Document
            {
                Id = Guid.NewGuid().ToString(),
                Title = Path.GetFileNameWithoutExtension(fileName),
                Filename = fileName,
                MimeType = "application/vnd.openxmlformats-officedocument.presentationml.presentation",
                CreatedAt = DateTime.UtcNow,
                ModifiedAt = DateTime.UtcNow,
                BlobPath = presentationPath,
                Metadata = new Dictionary<string, object>
                {
                    ["OfficeApplication"] = "PowerPoint",
                    ["OfficeVersion"] = pptApp.Version,
                    ["RevisionTracking"] = true
                }
            };

            await _documentService.CreateDocumentAsync(doc);

            // Create initial revision
            await _revisionService.CreateRevisionAsync(new DocumentRevision
            {
                Id = Guid.NewGuid().ToString(),
                DocumentId = doc.Id,
                RevisionNumber = 1,
                CreatedAt = DateTime.UtcNow,
                Author = Environment.UserName,
                Comment = "Initial presentation creation",
                FilePath = presentationPath
            });

            return new OfficeDocumentResult
            {
                Success = true,
                DocumentPath = presentationPath,
                DocumentId = doc.Id
            };
        }
        catch (Exception ex)
        {
            return new OfficeDocumentResult
            {
                Success = false,
                ErrorMessage = $"Error creating PowerPoint presentation: {ex.Message}"
            };
        }
    }

    public async Task<OfficeDocumentResult> OpenPowerPointPresentationAsync(string presentationPath)
    {
        try
        {
            var pptType = Type.GetTypeFromProgID("PowerPoint.Application");
            if (pptType == null)
                return new OfficeDocumentResult 
                { 
                    Success = false, 
                    ErrorMessage = "Microsoft PowerPoint is not installed" 
                };

            var pptAppObj = Activator.CreateInstance(pptType);
            if (pptAppObj == null)
                return new OfficeDocumentResult { Success = false, ErrorMessage = "Failed to create PowerPoint instance" };
            dynamic pptApp = pptAppObj;
                
            pptApp.Visible = true;

            var presentation = pptApp.Presentations.Open(presentationPath);

            return new OfficeDocumentResult
            {
                Success = true,
                DocumentPath = presentationPath
            };
        }
        catch (Exception ex)
        {
            return new OfficeDocumentResult
            {
                Success = false,
                ErrorMessage = $"Error opening PowerPoint presentation: {ex.Message}"
            };
        }
    }

    #endregion

    #region Outlook Integration

    public async Task<OfficeDocumentResult> CreateNewOutlookEmailAsync()
    {
        try
        {
            var outlookType = Type.GetTypeFromProgID("Outlook.Application");
            if (outlookType == null)
                return new OfficeDocumentResult 
                { 
                    Success = false, 
                    ErrorMessage = "Microsoft Outlook is not installed" 
                };

            var outlookAppObj = Activator.CreateInstance(outlookType);
            if (outlookAppObj == null)
                return new OfficeDocumentResult { Success = false, ErrorMessage = "Failed to create Outlook instance" };
            dynamic outlookApp = outlookAppObj;
                
            dynamic mailItem = outlookApp.CreateItem(0); // 0 = olMailItem

            mailItem.Display();

            return new OfficeDocumentResult
            {
                Success = true,
                DocumentPath = "Outlook Email Draft"
            };
        }
        catch (Exception ex)
        {
            return new OfficeDocumentResult
            {
                Success = false,
                ErrorMessage = $"Error creating Outlook email: {ex.Message}"
            };
        }
    }

    #endregion

    #region OneNote Integration

    public async Task<OfficeDocumentResult> CreateNewOneNotePageAsync(string? notebookName = null)
    {
        try
        {
            var oneNoteType = Type.GetTypeFromProgID("OneNote.Application");
            if (oneNoteType == null)
                return new OfficeDocumentResult 
                { 
                    Success = false, 
                    ErrorMessage = "Microsoft OneNote is not installed" 
                };

            var oneNoteAppObj = Activator.CreateInstance(oneNoteType);
            if (oneNoteAppObj == null)
                return new OfficeDocumentResult { Success = false, ErrorMessage = "Failed to create OneNote instance" };
            dynamic oneNoteApp = oneNoteAppObj;

            // Get the default notebook's section
            string notebookXml = string.Empty;
            oneNoteApp.GetHierarchy("", 2, out notebookXml); // 2 = hsNotebooks
            
            // Parse XML to get section ID (simplified - in production use proper XML parsing)
            // For now, create page in default section
            string sectionId = string.Empty;
            
            // Get first section from hierarchy
            if (!string.IsNullOrEmpty(notebookXml))
            {
                var xml = new System.Xml.XmlDocument();
                xml.LoadXml(notebookXml);
                var nsmgr = new System.Xml.XmlNamespaceManager(xml.NameTable);
                nsmgr.AddNamespace("one", "http://schemas.microsoft.com/office/onenote/2013/onenote");
                var sectionNode = xml.SelectSingleNode("//one:Section", nsmgr);
                sectionId = sectionNode?.Attributes?["ID"]?.Value ?? string.Empty;
            }

            if (string.IsNullOrEmpty(sectionId))
            {
                return new OfficeDocumentResult
                {
                    Success = false,
                    ErrorMessage = "Could not find a valid OneNote section. Please create a notebook first."
                };
            }

            // Create new page
            string pageId = string.Empty;
            oneNoteApp.CreateNewPage(sectionId, out pageId);

            // Navigate to new page
            oneNoteApp.NavigateTo(pageId);

            var doc = new Document
            {
                Id = Guid.NewGuid().ToString(),
                Title = $"OneNote Page {DateTime.Now:yyyy-MM-dd HH:mm}",
                Filename = "OneNote",
                MimeType = "application/onenote",
                CreatedAt = DateTime.UtcNow,
                ModifiedAt = DateTime.UtcNow,
                Metadata = new Dictionary<string, object>
                {
                    ["OfficeApplication"] = "OneNote",
                    ["PageId"] = pageId,
                    ["SectionId"] = sectionId,
                    ["RevisionTracking"] = true
                }
            };

            await _documentService.CreateDocumentAsync(doc);

            return new OfficeDocumentResult
            {
                Success = true,
                DocumentPath = pageId,
                DocumentId = doc.Id
            };
        }
        catch (Exception ex)
        {
            return new OfficeDocumentResult
            {
                Success = false,
                ErrorMessage = $"Error creating OneNote page: {ex.Message}"
            };
        }
    }

    #endregion

    #region Revision Management

    public async Task<bool> SaveDocumentRevisionAsync(string documentPath, string documentId)
    {
        try
        {
            if (!File.Exists(documentPath))
                return false;

            // Get current document
            var document = await _documentService.GetDocumentByIdAsync(documentId);
            if (document == null)
                return false;

            // Get revision count - use atomic transaction if possible
            // For now, we'll get the latest revision number and increment
            var latestRevision = await _revisionService.GetLatestRevisionAsync(documentId);
            var nextRevisionNumber = (latestRevision?.RevisionNumber ?? 0) + 1;

            // Create revision backup
            var revisionFileName = $"{Path.GetFileNameWithoutExtension(documentPath)}_rev{nextRevisionNumber}{Path.GetExtension(documentPath)}";
            var revisionPath = Path.Combine(_workingDirectory, "Revisions", documentId);
            Directory.CreateDirectory(revisionPath);

            var revisionFilePath = Path.Combine(revisionPath, revisionFileName);
            
            // Use lock to prevent race condition during file copy and revision creation
            var lockObj = GetDocumentLock(documentId);
            await Task.Run(() =>
            {
                lock (lockObj)
                {
                    File.Copy(documentPath, revisionFilePath, true);
                }
            });

            // Create revision entry with file hash
            var fileHash = CalculateFileHash(revisionFilePath);
            var fileInfo = new FileInfo(revisionFilePath);
            
            await _revisionService.CreateRevisionAsync(new DocumentRevision
            {
                Id = Guid.NewGuid().ToString(),
                DocumentId = documentId,
                RevisionNumber = nextRevisionNumber,
                CreatedAt = DateTime.UtcNow,
                Author = Environment.UserName,
                Comment = $"Revision {nextRevisionNumber}",
                FilePath = revisionFilePath,
                FileHash = fileHash,
                FileSize = fileInfo.Length
            });

            return true;
        }
        catch (Exception)
        {
            return false;
        }
    }

    private static readonly System.Collections.Concurrent.ConcurrentDictionary<string, object> _documentLocks = new();
    
    private static object GetDocumentLock(string documentId)
    {
        return _documentLocks.GetOrAdd(documentId, _ => new object());
    }

    public async Task<IEnumerable<DocumentRevision>> GetDocumentRevisionsAsync(string documentId)
    {
        return await _revisionService.GetDocumentRevisionsAsync(documentId);
    }

    private string CalculateFileHash(string filePath)
    {
        using var sha256 = System.Security.Cryptography.SHA256.Create();
        using var stream = File.OpenRead(filePath);
        var hash = sha256.ComputeHash(stream);
        return BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
    }

    #endregion
}
