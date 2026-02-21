/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IntelligentBreadcrumbViewModel.cs                  ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     224                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MediatR;
using System.Collections.ObjectModel;
using Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath;
using Themis.DocumentManager.Application.Navigation.Queries.GetRelatedEntities;

namespace Themis.DocumentManager.ViewModels.Navigation;

/// <summary>
/// ViewModel for intelligent breadcrumb navigation
/// Shows: Behörde > Ablage > Akte > Vorgang > Dokument
/// With AI-powered dropdown suggestions for related entities
/// </summary>
public partial class IntelligentBreadcrumbViewModel : ObservableObject
{
    private readonly IMediator _mediator;

    [ObservableProperty]
    private ObservableCollection<BreadcrumbItemViewModel> _breadcrumbItems = new();

    [ObservableProperty]
    private bool _isLoading;

    public IntelligentBreadcrumbViewModel(IMediator mediator)
    {
        _mediator = mediator;
    }

    [RelayCommand]
    private async Task LoadNavigationPathAsync(NavigationContext context)
    {
        IsLoading = true;
        try
        {
            // Get navigation path
            var query = new GetNavigationPathQuery
            {
                EntityId = context.EntityId,
                EntityType = context.EntityType
            };

            var path = await _mediator.Send(query);

            // Build breadcrumb items
            BreadcrumbItems.Clear();
            foreach (var item in path.Items)
            {
                var breadcrumbItem = new BreadcrumbItemViewModel
                {
                    Id = item.Id,
                    Name = item.Name,
                    Type = item.Type,
                    Level = item.Level,
                    IsCurrentItem = item.IsCurrentItem,
                    Icon = GetIconForEntityType(item.Type)
                };

                // Load related entities for dropdown if not current item
                if (!item.IsCurrentItem)
                {
                    await LoadRelatedEntitiesAsync(breadcrumbItem);
                }

                BreadcrumbItems.Add(breadcrumbItem);
            }
        }
        finally
        {
            IsLoading = false;
        }
    }

    private async Task LoadRelatedEntitiesAsync(BreadcrumbItemViewModel breadcrumbItem)
    {
        var query = new GetRelatedEntitiesQuery
        {
            EntityId = breadcrumbItem.Id,
            EntityType = breadcrumbItem.Type,
            UserId = "current-user" // TODO: Get from auth service
        };

        var result = await _mediator.Send(query);

        breadcrumbItem.RelatedEntityGroups.Clear();
        foreach (var group in result.Groups)
        {
            var viewModelGroup = new RelatedEntityGroupViewModel
            {
                GroupName = group.GroupName,
                TargetType = group.TargetType
            };

            foreach (var entity in group.Entities.OrderByDescending(e => e.RelevanceScore))
            {
                viewModelGroup.Entities.Add(new RelatedEntityViewModel
                {
                    Id = entity.Id,
                    Name = entity.Name,
                    Type = entity.Type,
                    RelevanceScore = entity.RelevanceScore,
                    RelevanceReason = entity.RelevanceReason,
                    Icon = GetIconForEntityType(entity.Type),
                    LastAccessedAt = entity.LastAccessedAt,
                    AccessCount = entity.AccessCount,
                    IsFrequentlyAccessed = entity.IsFrequentlyAccessed,
                    IsSimilar = entity.IsSimilar,
                    RelevanceIndicator = GetRelevanceIndicator(entity.RelevanceScore)
                });
            }

            breadcrumbItem.RelatedEntityGroups.Add(viewModelGroup);
        }
    }

    private string GetIconForEntityType(EntityType type) => type switch
    {
        EntityType.Authority => "🏛️",
        EntityType.Repository => "📁",
        EntityType.File => "📂",
        EntityType.Process => "📋",
        EntityType.Document => "📄",
        _ => "•"
    };

    private string GetRelevanceIndicator(double score) => score switch
    {
        >= 0.9 => "🔥", // Very relevant
        >= 0.75 => "⭐", // Highly relevant
        >= 0.6 => "▶️", // Relevant
        _ => "•"
    };

    [RelayCommand]
    private void NavigateToEntity(RelatedEntityViewModel entity)
    {
        // Navigate to selected entity
        var context = new NavigationContext
        {
            EntityId = entity.Id,
            EntityType = entity.Type
        };

        _ = LoadNavigationPathAsync(context);
    }

    [RelayCommand]
    private void NavigateToBreadcrumbItem(BreadcrumbItemViewModel item)
    {
        // Navigate to breadcrumb level
        var context = new NavigationContext
        {
            EntityId = item.Id,
            EntityType = item.Type
        };

        _ = LoadNavigationPathAsync(context);
    }
}

public class BreadcrumbItemViewModel : ObservableObject
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public EntityType Type { get; set; }
    public int Level { get; set; }
    public bool IsCurrentItem { get; set; }
    public string Icon { get; set; } = string.Empty;
    public ObservableCollection<RelatedEntityGroupViewModel> RelatedEntityGroups { get; set; } = new();
}

public class RelatedEntityGroupViewModel : ObservableObject
{
    public string GroupName { get; set; } = string.Empty;
    public EntityType TargetType { get; set; }
    public ObservableCollection<RelatedEntityViewModel> Entities { get; set; } = new();
}

public class RelatedEntityViewModel : ObservableObject
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public EntityType Type { get; set; }
    public double RelevanceScore { get; set; }
    public string RelevanceReason { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public DateTime? LastAccessedAt { get; set; }
    public int AccessCount { get; set; }
    public bool IsFrequentlyAccessed { get; set; }
    public bool IsSimilar { get; set; }
    public string RelevanceIndicator { get; set; } = string.Empty;
}

public class NavigationContext
{
    public string EntityId { get; set; } = string.Empty;
    public EntityType EntityType { get; set; }
}
