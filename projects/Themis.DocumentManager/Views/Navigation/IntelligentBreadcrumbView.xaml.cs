/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IntelligentBreadcrumbView.xaml.cs                  ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     89                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
    • b01a2e3c3  2025-12-10  Add intelligent breadcrumb navigation and configurable fa... ║
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
