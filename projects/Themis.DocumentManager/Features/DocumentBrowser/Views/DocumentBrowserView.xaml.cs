/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentBrowserView.xaml.cs                        ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     76                                             ║
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
using System.Windows.Input;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Features.DocumentBrowser.ViewModels;

namespace Themis.DocumentManager.Features.DocumentBrowser.Views;

public partial class DocumentBrowserView : UserControl
{
    public DocumentBrowserView()
    {
        InitializeComponent();
        this.AddHandler(UIElement.PreviewMouseRightButtonDownEvent, 
            new MouseButtonEventHandler(OnPreviewMouseRightButtonDown), true);
    }

    private async void OnPreviewMouseRightButtonDown(object sender, MouseButtonEventArgs e)
    {
        // Finde das Border-Element und den DocumentItem
        var element = e.OriginalSource as FrameworkElement;
        while (element != null && !(element is Border))
        {
            element = element.Parent as FrameworkElement;
        }

        if (element is Border border && border.DataContext is DocumentItemViewModel docItem)
        {
            var vm = this.DataContext as DocumentBrowserViewModel;
            if (vm != null)
            {
                // Direkter Aufruf statt Command
                await vm.OnDocumentRightClickAsync(docItem);
            }
        }
    }

    private async void MenuItem_Click(object sender, RoutedEventArgs e)
    {
        if (sender is MenuItem menuItem && menuItem.Tag is ContextMenuAction action)
        {
            var vm = this.DataContext as DocumentBrowserViewModel;
            if (vm != null)
            {
                await vm.ExecuteContextMenuActionAsync(action);
            }
        }
    }
}
