/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IntelligentBreadcrumbView.xaml.cs                  ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     96                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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

using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace Themis.DocumentManager.Views.Navigation;

/// <summary>
/// Interaction logic for IntelligentBreadcrumbView.xaml
/// Intelligent breadcrumb with AI-powered navigation suggestions
/// </summary>
public partial class IntelligentBreadcrumbView : UserControl
{
    private DispatcherTimer? _hoverTimer;
    private Button? _hoveredButton;

    public IntelligentBreadcrumbView()
    {
        InitializeComponent();
    }

    private void BreadcrumbItem_MouseEnter(object sender, MouseEventArgs e)
    {
        if (sender is not Button button) return;
        
        // Check if this breadcrumb item has related entities (dropdown)
        if (button.DataContext is ViewModels.Navigation.BreadcrumbItemViewModel item 
            && item.RelatedEntityGroups.Count > 0 
            && !item.IsCurrentItem)
        {
            _hoveredButton = button;
            
            // Start hover timer (show dropdown after 500ms hover)
            _hoverTimer?.Stop();
            _hoverTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(500)
            };
            _hoverTimer.Tick += (s, args) =>
            {
                _hoverTimer.Stop();
                ShowDropdown(button);
            };
            _hoverTimer.Start();
        }
    }

    private void BreadcrumbItem_MouseLeave(object sender, MouseEventArgs e)
    {
        // Cancel hover timer
        _hoverTimer?.Stop();
        _hoveredButton = null;
    }

    private void ShowDropdown(Button button)
    {
        if (button.ContextMenu == null) return;
        
        button.ContextMenu.PlacementTarget = button;
        button.ContextMenu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
        button.ContextMenu.IsOpen = true;
        
        // Auto-close after mouse leaves menu area
        button.ContextMenu.Closed += (s, e) =>
        {
            if (button.ContextMenu != null)
                button.ContextMenu.IsOpen = false;
        };
    }
}
