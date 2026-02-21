/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IMetadataBindingService.cs                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     15                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
