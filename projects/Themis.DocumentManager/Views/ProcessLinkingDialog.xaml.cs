/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ProcessLinkingDialog.xaml.cs                       ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Themis.DocumentManager.ViewModels;

namespace Themis.DocumentManager.Views;

public partial class ProcessLinkingDialog : UserControl
{
    public ProcessLinkingDialog()
    {
        InitializeComponent();
    }

    private void TemplateItem_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (sender is Border border && border.DataContext is ProcessTemplateViewModel template)
        {
            var vm = this.DataContext as ProcessLinkingDialogViewModel;
            if (vm != null)
            {
                vm.SelectedTemplate = template;
            }
        }
    }
}
