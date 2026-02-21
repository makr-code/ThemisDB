/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TreeViewSettingsView.xaml.cs                       ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
