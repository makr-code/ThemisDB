/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TreeViewSettingsViewModel.cs                       ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     307                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.ViewModels.Settings;

public partial class TreeViewSettingsViewModel : ObservableObject
{
    [ObservableProperty]
    private ObservableCollection<TreeViewItemConfigViewModel> _items = new();

    [ObservableProperty]
    private TreeViewItemConfigViewModel? _selectedItem;

    [ObservableProperty]
    private bool _showIcons = true;

    [ObservableProperty]
    private bool _expandOnClick = true;

    [ObservableProperty]
    private bool _rememberExpansionState = true;

    public TreeViewSettingsViewModel()
    {
        LoadDefaultItems();
    }

    [RelayCommand]
    private void AddRootItem()
    {
        var newItem = new TreeViewItemConfigViewModel
        {
            Header = "Neues Element",
            Icon = "📁",
            ItemType = TreeViewItemType.Custom
        };
        Items.Add(newItem);
        SelectedItem = newItem;
    }

    [RelayCommand]
    private void AddChildItem()
    {
        if (SelectedItem == null)
        {
            AddRootItem();
            return;
        }

        var newChild = new TreeViewItemConfigViewModel
        {
            Header = "Neues Unterelement",
            Icon = "📄",
            ItemType = TreeViewItemType.Custom,
            Parent = SelectedItem
        };
        SelectedItem.Children.Add(newChild);
        SelectedItem.IsExpanded = true;
    }

    [RelayCommand]
    private void DeleteItem()
    {
        if (SelectedItem == null) return;

        if (SelectedItem.Parent != null)
        {
            SelectedItem.Parent.Children.Remove(SelectedItem);
        }
        else
        {
            Items.Remove(SelectedItem);
        }
        SelectedItem = null;
    }

    [RelayCommand]
    private void MoveUp()
    {
        if (SelectedItem == null) return;

        var collection = SelectedItem.Parent?.Children ?? Items;
        var index = collection.IndexOf(SelectedItem);
        if (index > 0)
        {
            collection.Move(index, index - 1);
        }
    }

    [RelayCommand]
    private void MoveDown()
    {
        if (SelectedItem == null) return;

        var collection = SelectedItem.Parent?.Children ?? Items;
        var index = collection.IndexOf(SelectedItem);
        if (index < collection.Count - 1)
        {
            collection.Move(index, index + 1);
        }
    }

    [RelayCommand]
    private void ResetToDefault()
    {
        Items.Clear();
        LoadDefaultItems();
    }

    private void LoadDefaultItems()
    {
        Items.Add(new TreeViewItemConfigViewModel
        {
            Header = "Dashboard",
            Icon = "📊",
            ItemType = TreeViewItemType.Dashboard,
            TargetView = "Dashboard"
        });

        var documents = new TreeViewItemConfigViewModel
        {
            Header = "Dokumente",
            Icon = "📄",
            ItemType = TreeViewItemType.Documents,
            IsExpanded = true
        };
        documents.Children.Add(new TreeViewItemConfigViewModel
        {
            Header = "Meine Dokumente",
            Icon = "📝",
            ItemType = TreeViewItemType.Custom,
            Parent = documents
        });
        documents.Children.Add(new TreeViewItemConfigViewModel
        {
            Header = "Zuletzt verwendet",
            Icon = "🕐",
            ItemType = TreeViewItemType.Custom,
            Parent = documents
        });
        documents.Children.Add(new TreeViewItemConfigViewModel
        {
            Header = "Favoriten",
            Icon = "⭐",
            ItemType = TreeViewItemType.Favorites,
            Parent = documents
        });
        Items.Add(documents);

        var projects = new TreeViewItemConfigViewModel
        {
            Header = "Projekte",
            Icon = "📂",
            ItemType = TreeViewItemType.Projects
        };
        projects.Children.Add(new TreeViewItemConfigViewModel
        {
            Header = "Aktive Projekte",
            Icon = "✅",
            ItemType = TreeViewItemType.Custom,
            Parent = projects
        });
        projects.Children.Add(new TreeViewItemConfigViewModel
        {
            Header = "Archivierte Projekte",
            Icon = "📦",
            ItemType = TreeViewItemType.Custom,
            Parent = projects
        });
        Items.Add(projects);

        Items.Add(new TreeViewItemConfigViewModel
        {
            Header = "Posteingang",
            Icon = "📥",
            ItemType = TreeViewItemType.Inbox,
            TargetView = "Inbox"
        });

        Items.Add(new TreeViewItemConfigViewModel
        {
            Header = "Wiedervorlagen",
            Icon = "📅",
            ItemType = TreeViewItemType.Reminders,
            TargetView = "Reminders"
        });

        Items.Add(new TreeViewItemConfigViewModel
        {
            Header = "Mitzeichnungen",
            Icon = "✍️",
            ItemType = TreeViewItemType.Cosigning,
            TargetView = "Cosigning"
        });
    }

    public TreeViewSettings ToSettings()
    {
        return new TreeViewSettings
        {
            RootItems = new ObservableCollection<TreeViewItemConfig>(
                Items.Select(i => i.ToConfig())),
            ShowIcons = ShowIcons,
            ExpandOnClick = ExpandOnClick,
            RememberExpansionState = RememberExpansionState
        };
    }

    public void LoadFromSettings(TreeViewSettings settings)
    {
        Items.Clear();
        foreach (var item in settings.RootItems)
        {
            Items.Add(TreeViewItemConfigViewModel.FromConfig(item, null));
        }
        ShowIcons = settings.ShowIcons;
        ExpandOnClick = settings.ExpandOnClick;
        RememberExpansionState = settings.RememberExpansionState;
    }
}

public partial class TreeViewItemConfigViewModel : ObservableObject
{
    [ObservableProperty]
    private string _header = string.Empty;

    [ObservableProperty]
    private string _icon = "📄";

    [ObservableProperty]
    private bool _isExpanded;

    [ObservableProperty]
    private TreeViewItemType _itemType = TreeViewItemType.Custom;

    [ObservableProperty]
    private string? _targetView;

    [ObservableProperty]
    private string? _navigationPath;

    [ObservableProperty]
    private ObservableCollection<TreeViewItemConfigViewModel> _children = new();

    public TreeViewItemConfigViewModel? Parent { get; set; }

    public string Id { get; set; } = Guid.NewGuid().ToString();

    public TreeViewItemConfig ToConfig()
    {
        return new TreeViewItemConfig
        {
            Id = Id,
            Header = Header,
            Icon = Icon,
            IsExpanded = IsExpanded,
            ItemType = ItemType,
            TargetView = TargetView,
            NavigationPath = NavigationPath,
            Children = new ObservableCollection<TreeViewItemConfig>(
                Children.Select(c => c.ToConfig()))
        };
    }

    public static TreeViewItemConfigViewModel FromConfig(TreeViewItemConfig config, TreeViewItemConfigViewModel? parent)
    {
        var vm = new TreeViewItemConfigViewModel
        {
            Id = config.Id,
            Header = config.Header,
            Icon = config.Icon,
            IsExpanded = config.IsExpanded,
            ItemType = config.ItemType,
            TargetView = config.TargetView,
            NavigationPath = config.NavigationPath,
            Parent = parent
        };

        foreach (var child in config.Children)
        {
            vm.Children.Add(FromConfig(child, vm));
        }

        return vm;
    }
}
