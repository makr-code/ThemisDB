/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IMetadataBindingService.cs                         ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     34                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
