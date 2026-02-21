/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ISmartMetadataLayoutEngine.cs                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     12                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.Generic;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Features.MetadataForm.Services;

public interface ISmartMetadataLayoutEngine
{
    List<MetadataFieldGroup> CreateOptimalLayout(DocumentMetadataBinding metadata);
    List<MetadataFieldGroup> CreateCompactLayout(DocumentMetadataBinding metadata);
    MetadataFieldGroup DetectFieldGroup(MetadataField field);
    Dictionary<BadgeCategory, List<MetadataField>> CreateBadgeGrouping(DocumentMetadataBinding metadata);
}
