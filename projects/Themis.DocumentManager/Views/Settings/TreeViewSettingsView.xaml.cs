/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TreeViewSettingsView.xaml.cs                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using System.Windows.Controls;
using Themis.DocumentManager.ViewModels.Settings;

namespace Themis.DocumentManager.Views.Settings;

public partial class TreeViewSettingsView : UserControl
{
    public TreeViewSettingsView()
    {
        InitializeComponent();
        DataContext = new TreeViewSettingsViewModel();
    }

    private void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        if (DataContext is TreeViewSettingsViewModel vm && e.NewValue is TreeViewItemConfigViewModel item)
        {
            vm.SelectedItem = item;
        }
    }

    private void IconButton_Click(object sender, RoutedEventArgs e)
    {
        if (sender is Button btn && DataContext is TreeViewSettingsViewModel vm && vm.SelectedItem != null)
        {
            vm.SelectedItem.Icon = btn.Content?.ToString() ?? "📄";
        }
    }

    private void SaveButton_Click(object sender, RoutedEventArgs e)
    {
        if (DataContext is TreeViewSettingsViewModel vm)
        {
            var settings = vm.ToSettings();
            var settingsService = App.GetService<Services.ISettingsService>();
            settingsService?.SaveTreeViewSettings(settings);
            
            MessageBox.Show("TreeView-Konfiguration gespeichert!", "Erfolgreich", 
                MessageBoxButton.OK, MessageBoxImage.Information);
        }
    }
}
