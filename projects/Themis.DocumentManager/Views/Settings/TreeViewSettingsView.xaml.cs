/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TreeViewSettingsView.xaml.cs                       ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
