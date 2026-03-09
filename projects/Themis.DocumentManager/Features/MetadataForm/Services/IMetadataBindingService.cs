/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IMetadataBindingService.cs                         ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:56:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     34                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Threading.Tasks;
using Themis.DocumentManager.Models;
using System.Collections.Generic;

namespace Themis.DocumentManager.Features.MetadataForm.Services;

public interface IMetadataBindingService
{
    DocumentMetadataBinding GetOrCreateBinding(string documentId);
    Task<bool> SaveBindingAsync(DocumentMetadataBinding binding);
    Task<bool> UpdateFieldAsync(string documentId, string fieldName, string newValue);
    Task<bool> FinalizeBindingAsync(string documentId);
    void InvalidateCache(string documentId);
    void ClearCache();
}
