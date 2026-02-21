/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            KeyboardNavigationService.cs                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     152                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Windows;
using System.Windows.Input;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service für erweiterte Keyboard-Navigation und Accessibility.
/// Phase 30 - Keyboard Navigation & Accessibility.
/// </summary>
public interface IKeyboardNavigationService
{
    /// <summary>
    /// Aktiviert erweiterte Keyboard-Navigation mit Custom-Key-Bindings.
    /// </summary>
    void EnableKeyboardNavigation();

    /// <summary>
    /// Fokussiert das nächste Element in der Tab-Order.
    /// </summary>
    void FocusNext();

    /// <summary>
    /// Fokussiert das vorherige Element in der Tab-Order.
    /// </summary>
    void FocusPrevious();

    /// <summary>
    /// Fokussiert ein spezifisches Element by Name.
    /// </summary>
    void FocusElement(string elementName);

    /// <summary>
    /// Event: Keyboard Shortcut wurde ausgeführt.
    /// </summary>
    event EventHandler<KeyboardShortcutEventArgs>? ShortcutExecuted;
}

public class KeyboardNavigationService : IKeyboardNavigationService
{
    private Window? _mainWindow;

    public event EventHandler<KeyboardShortcutEventArgs>? ShortcutExecuted;

    public void EnableKeyboardNavigation()
    {
        _mainWindow = System.Windows.Application.Current?.MainWindow;
        
        if (_mainWindow == null) return;

        // Set IsTabStop for all focusable elements
        ConfigureTabNavigation(_mainWindow);
    }

    public void FocusNext()
    {
        if (_mainWindow == null) return;

        var currentElement = Keyboard.FocusedElement as UIElement;
        if (currentElement != null)
        {
            currentElement.MoveFocus(new TraversalRequest(FocusNavigationDirection.Next));
            RaiseShortcutExecuted("FocusNext");
        }
    }

    public void FocusPrevious()
    {
        if (_mainWindow == null) return;

        var currentElement = Keyboard.FocusedElement as UIElement;
        if (currentElement != null)
        {
            currentElement.MoveFocus(new TraversalRequest(FocusNavigationDirection.Previous));
            RaiseShortcutExecuted("FocusPrevious");
        }
    }

    public void FocusElement(string elementName)
    {
        if (_mainWindow == null) return;

        var element = _mainWindow.FindName(elementName) as UIElement;
        if (element != null && element.Focusable)
        {
            element.Focus();
            RaiseShortcutExecuted($"FocusElement:{elementName}");
        }
    }

    private void ConfigureTabNavigation(DependencyObject parent)
    {
        for (int i = 0; i < System.Windows.Media.VisualTreeHelper.GetChildrenCount(parent); i++)
        {
            var child = System.Windows.Media.VisualTreeHelper.GetChild(parent, i);

            if (child is UIElement element)
            {
                // Set TabIndex for logical tab order
                if (element.Focusable && KeyboardNavigation.GetTabIndex(element) == int.MaxValue)
                {
                    KeyboardNavigation.SetTabNavigation(element, KeyboardNavigationMode.Continue);
                }

                // Set AutomationProperties for screen readers
                if (child is FrameworkElement frameworkElement)
                {
                    ConfigureAutomationProperties(frameworkElement);
                }
            }

            ConfigureTabNavigation(child);
        }
    }

    private void ConfigureAutomationProperties(FrameworkElement element)
    {
        // Set Name if not already set
        if (string.IsNullOrEmpty(System.Windows.Automation.AutomationProperties.GetName(element)))
        {
            // Try to use element name or type as fallback
            var name = element.Name;
            if (string.IsNullOrEmpty(name))
            {
                name = element.GetType().Name;
            }
            System.Windows.Automation.AutomationProperties.SetName(element, name);
        }

        // Set HelpText from ToolTip if available
        if (element.ToolTip != null && 
            string.IsNullOrEmpty(System.Windows.Automation.AutomationProperties.GetHelpText(element)))
        {
            System.Windows.Automation.AutomationProperties.SetHelpText(element, element.ToolTip.ToString() ?? string.Empty);
        }
    }

    private void RaiseShortcutExecuted(string shortcutName)
    {
        ShortcutExecuted?.Invoke(this, new KeyboardShortcutEventArgs(shortcutName));
    }
}

public class KeyboardShortcutEventArgs : EventArgs
{
    public string ShortcutName { get; }

    public KeyboardShortcutEventArgs(string shortcutName)
    {
        ShortcutName = shortcutName;
    }
}
