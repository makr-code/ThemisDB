/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ShortcutsOverlay.xaml.cs                           ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     184                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Views
{
    public partial class ShortcutsOverlay : Window
    {
        private readonly Dictionary<string, string> _categoryMap = new Dictionary<string, string>
        {
            // Metadata
            { "SaveMetadata", "Metadaten" },
            { "ReloadMetadata", "Metadaten" },
            { "FinalizeMetadata", "Metadaten" },
            
            // Tab Management
            { "NextTab", "Tab-Verwaltung" },
            { "PreviousTab", "Tab-Verwaltung" },
            { "CloseTab", "Tab-Verwaltung" },
            { "DuplicateTab", "Tab-Verwaltung" },
            { "CloseOthers", "Tab-Verwaltung" },
            { "OpenTabInNewWindow", "Tab-Verwaltung" },
            { "SearchTabs", "Tab-Verwaltung" },
            
            // Favorites
            { "FavoriteAdd", "Favoriten" },
            { "FavoriteRemove", "Favoriten" },
            
            // Visualization
            { "SwitchSidebarGraph", "Visualisierung" },
            { "SwitchSidebarMap", "Visualisierung" },
            
            // General
            { "OpenSettings", "Allgemein" },
            { "OpenSearch", "Allgemein" },
            { "ToggleTheme", "Allgemein" },
            { "ShowShortcutsOverlay", "Allgemein" }
        };

        public ShortcutsOverlay()
        {
            InitializeComponent();
            LoadShortcuts(false);
        }

        private void LoadShortcuts(bool useContextFilter)
        {
            var svc = KeyboardShortcutService.Instance;
            var converter = new KeyGestureConverter();
            
            var shortcuts = svc.Shortcuts
                .OrderBy(kv => _categoryMap.ContainsKey(kv.Key) ? _categoryMap[kv.Key] : "Sonstiges")
                .ThenBy(kv => kv.Key)
                .ToList();

            if (useContextFilter)
            {
                var currentView = GetCurrentViewContext();
                shortcuts = FilterByContext(shortcuts, currentView);
                FilterInfoText.Text = $"Kontextuelle Shortcuts ({currentView})";
            }
            else
            {
                FilterInfoText.Text = "Alle Shortcuts anzeigen";
            }

            ShortcutsContainer.Children.Clear();
            
            string? currentCategory = null;
            foreach (var item in shortcuts)
            {
                var category = _categoryMap.ContainsKey(item.Key) ? _categoryMap[item.Key] : "Sonstiges";
                
                if (category != currentCategory)
                {
                    // Neue Kategorie
                    var categoryHeader = new TextBlock
                    {
                        Text = category,
                        FontSize = 13,
                        FontWeight = FontWeights.Bold,
                        Foreground = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Colors.CornflowerBlue),
                        Margin = new Thickness(0, 12, 0, 6)
                    };
                    ShortcutsContainer.Children.Add(categoryHeader);
                    currentCategory = category;
                }

                // Shortcut Item
                var grid = new Grid { Margin = new Thickness(0, 2, 0, 2) };
                grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(220) });
                grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

                var keyText = new TextBlock
                {
                    Text = item.Key,
                    Foreground = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(203, 213, 225))
                };
                Grid.SetColumn(keyText, 0);
                grid.Children.Add(keyText);

                var valueText = new TextBlock
                {
                    Text = converter.ConvertToString(item.Value) ?? "",
                    Foreground = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(96, 165, 250))
                };
                Grid.SetColumn(valueText, 1);
                grid.Children.Add(valueText);

                ShortcutsContainer.Children.Add(grid);
            }
        }

        private string GetCurrentViewContext()
        {
            if (Owner is MainWindow mw)
            {
                var focusedElement = FocusManager.GetFocusedElement(mw);
                if (focusedElement is FrameworkElement elem)
                {
                    if (elem.Name.Contains("Metadata")) return "Metadaten";
                    if (elem.Name.Contains("Graph")) return "Visualisierung";
                    if (elem.Name.Contains("Map")) return "Visualisierung";
                    if (elem.Name.Contains("Tab")) return "Tab-Verwaltung";
                    if (elem.Name.Contains("Search")) return "Suche";
                }
            }
            return "Allgemein";
        }

        private List<KeyValuePair<string, KeyGesture>> FilterByContext(List<KeyValuePair<string, KeyGesture>> shortcuts, string context)
        {
            var contextRelevant = new Dictionary<string, List<string>>
            {
                { "Metadaten", new List<string> { "SaveMetadata", "ReloadMetadata", "FinalizeMetadata", "OpenSettings" } },
                { "Tab-Verwaltung", new List<string> { "NextTab", "PreviousTab", "CloseTab", "DuplicateTab", "CloseOthers", "OpenTabInNewWindow", "SearchTabs" } },
                { "Visualisierung", new List<string> { "SwitchSidebarGraph", "SwitchSidebarMap" } },
                { "Favoriten", new List<string> { "FavoriteAdd", "FavoriteRemove" } },
                { "Allgemein", new List<string> { "OpenSearch", "ToggleTheme", "ShowShortcutsOverlay" } }
            };

            if (contextRelevant.ContainsKey(context))
            {
                var relevantKeys = contextRelevant[context];
                return shortcuts.Where(s => relevantKeys.Contains(s.Key)).ToList();
            }
            return shortcuts;
        }

        private void ContextFilter_Changed(object sender, RoutedEventArgs e)
        {
            LoadShortcuts(ContextFilterCheckBox.IsChecked == true);
        }
    }
}