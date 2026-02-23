/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentBrowserView.xaml.cs                        ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
