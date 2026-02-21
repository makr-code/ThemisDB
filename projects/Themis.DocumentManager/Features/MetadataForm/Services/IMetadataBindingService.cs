/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IMetadataBindingService.cs                         ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     39                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
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
