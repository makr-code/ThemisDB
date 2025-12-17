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
