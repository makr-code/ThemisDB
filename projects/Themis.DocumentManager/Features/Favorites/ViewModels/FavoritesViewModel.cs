/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FavoritesViewModel.cs                              ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     260                                            ║
    • Open Issues:     TODOs: 5, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MediatR;
using System.Collections.ObjectModel;
using Themis.DocumentManager.Application.Favorites.Commands.AddToFavorites;
using Themis.DocumentManager.Application.Favorites.Commands.RemoveFromFavorites;
using Themis.DocumentManager.Application.Favorites.Queries.GetFavorites;
using Themis.DocumentManager.Application.Favorites.Queries.IsFavorite;
using Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath;
using NavEntityType = Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath.EntityType;

namespace Themis.DocumentManager.Features.Favorites.ViewModels;

/// <summary>
/// ViewModel for Favorites management
/// Allows adding/removing favorites and viewing favorites list
/// </summary>
public partial class FavoritesViewModel : ObservableObject
{
    private readonly IMediator _mediator;

    [ObservableProperty]
    private ObservableCollection<FavoriteGroupViewModel> _favoriteGroups = new();

    [ObservableProperty]
    private ObservableCollection<FavoriteItemViewModel> _allFavorites = new();

    [ObservableProperty]
    private FavoritesSortBy _sortBy = FavoritesSortBy.RecentlyAdded;

    [ObservableProperty]
    private bool _groupByType = true;

    [ObservableProperty]
    private string _searchText = string.Empty;

    [ObservableProperty]
    private bool _isLoading;

    public FavoritesViewModel(IMediator mediator)
    {
        _mediator = mediator;
    }

    [RelayCommand]
    public async Task LoadFavoritesAsync()
    {
        IsLoading = true;
        try
        {
            var query = new GetFavoritesQuery
            {
                UserId = "current-user", // TODO: Get from auth service
                SortBy = SortBy,
                GroupByType = GroupByType
            };

            var result = await _mediator.Send(query);

            AllFavorites.Clear();
            foreach (var item in result.Items)
            {
                AllFavorites.Add(new FavoriteItemViewModel
                {
                    Id = item.Id,
                    EntityId = item.EntityId,
                    EntityType = item.EntityType,
                    EntityName = item.EntityName,
                    Icon = item.Icon,
                    Category = item.Category,
                    Tags = item.Tags,
                    Notes = item.Notes,
                    CreatedAt = item.CreatedAt,
                    LastAccessedAt = item.LastAccessedAt,
                    AccessCount = item.AccessCount
                });
            }

            // Build grouped view
            if (GroupByType)
            {
                FavoriteGroups.Clear();
                foreach (var group in result.GroupedByType)
                {
                    FavoriteGroups.Add(new FavoriteGroupViewModel
                    {
                        GroupName = GetGroupNameForEntityType(group.Key),
                        Icon = GetIconForEntityType(group.Key),
                        Items = new ObservableCollection<FavoriteItemViewModel>(
                            group.Value.Select(v => new FavoriteItemViewModel
                            {
                                Id = v.Id,
                                EntityId = v.EntityId,
                                EntityType = v.EntityType,
                                EntityName = v.EntityName,
                                Icon = v.Icon,
                                Category = v.Category,
                                Tags = v.Tags,
                                Notes = v.Notes,
                                CreatedAt = v.CreatedAt,
                                LastAccessedAt = v.LastAccessedAt,
                                AccessCount = v.AccessCount
                            })
                        )
                    });
                }
            }
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private async Task AddToFavoritesAsync(EntityContext entity)
    {
        var command = new AddToFavoritesCommand
        {
            EntityId = entity.EntityId,
            EntityType = entity.EntityType,
            EntityName = entity.EntityName,
            UserId = "current-user"
        };

        await _mediator.Send(command);
        await LoadFavoritesAsync();
    }

    [RelayCommand]
    private async Task RemoveFromFavoritesAsync(string entityId)
    {
        var command = new RemoveFromFavoritesCommand
        {
            EntityId = entityId,
            UserId = "current-user" // TODO: Get from AuthenticationService via handler
        };

        await _mediator.Send(command);
        await LoadFavoritesAsync();
    }

    [RelayCommand]
    private async Task<bool> IsFavoriteAsync(string entityId)
    {
        var query = new IsFavoriteQuery
        {
            EntityId = entityId,
            UserId = "current-user" // TODO: Get from AuthenticationService via handler
        };

        return await _mediator.Send(query);
    }

    [RelayCommand]
    private void NavigateToFavorite(FavoriteItemViewModel favorite)
    {
        // Navigate to entity
        // TODO: Implement navigation
    }

    [RelayCommand]
    private void ChangeSortOrder(FavoritesSortBy sortBy)
    {
        SortBy = sortBy;
        _ = LoadFavoritesAsync();
    }

    [RelayCommand]
    private void ToggleGrouping()
    {
        GroupByType = !GroupByType;
        _ = LoadFavoritesAsync();
    }

    private string GetGroupNameForEntityType(NavEntityType type) => type switch
    {
        NavEntityType.Authority => "Behörden",
        NavEntityType.Repository => "Ablagen",
        NavEntityType.File => "Akten",
        NavEntityType.Process => "Vorgänge",
        NavEntityType.Document => "Dokumente",
        _ => "Sonstige"
    };

    private string GetIconForEntityType(NavEntityType type) => type switch
    {
        NavEntityType.Authority => "🏛️",
        NavEntityType.Repository => "📁",
        NavEntityType.File => "📂",
        NavEntityType.Process => "📋",
        NavEntityType.Document => "📄",
        _ => "⭐"
    };

    partial void OnSearchTextChanged(string value)
    {
        // Filter favorites based on search text
        // TODO: Implement filtering
    }
}

public class FavoriteGroupViewModel : ObservableObject
{
    public string GroupName { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public ObservableCollection<FavoriteItemViewModel> Items { get; set; } = new();
}

public class FavoriteItemViewModel : ObservableObject
{
    public string Id { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public NavEntityType EntityType { get; set; }
    public string EntityName { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public string? Category { get; set; }
    public string? Tags { get; set; }
    public string? Notes { get; set; }
    public DateTime CreatedAt { get; set; }
    public DateTime? LastAccessedAt { get; set; }
    public int AccessCount { get; set; }
}

public class EntityContext
{
    public string EntityId { get; set; } = string.Empty;
    public NavEntityType EntityType { get; set; }
    public string EntityName { get; set; } = string.Empty;
}

