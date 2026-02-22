/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataCompactDisplay.cs                          ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     267                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.UI;

/// <summary>
/// Kompakte Badge-basierte Metadaten-Anzeige
/// Zeigt nur ausgefüllte Metadaten als farbliche Badges
/// </summary>
public class MetadataCompactDisplay : UserControl
{
    private WrapPanel? _badgePanel;

    public static readonly DependencyProperty BadgesProperty =
        DependencyProperty.Register(nameof(Badges), typeof(List<MetadataBadge>),
            typeof(MetadataCompactDisplay), new PropertyMetadata(null, OnBadgesChanged));

    public static readonly DependencyProperty MaxVisibleBadgesProperty =
        DependencyProperty.Register(nameof(MaxVisibleBadges), typeof(int),
            typeof(MetadataCompactDisplay), new PropertyMetadata(10));

    public List<MetadataBadge>? Badges
    {
        get => (List<MetadataBadge>?)GetValue(BadgesProperty);
        set => SetValue(BadgesProperty, value);
    }

    public int MaxVisibleBadges
    {
        get => (int)GetValue(MaxVisibleBadgesProperty);
        set => SetValue(MaxVisibleBadgesProperty, value);
    }

    public event EventHandler? ShowDetailsRequested;
    public event EventHandler? EditRequested;
    public event EventHandler<BadgeClickedEventArgs>? BadgeClicked;

    public MetadataCompactDisplay()
    {
        InitializeUI();
    }

    private void InitializeUI()
    {
        var mainPanel = new StackPanel { Orientation = Orientation.Vertical };

        // Badge Display Panel
        _badgePanel = new WrapPanel
        {
            Orientation = Orientation.Horizontal,
            Margin = new Thickness(0, 5, 0, 10)
        };

        mainPanel.Children.Add(_badgePanel);

        // Action Buttons
        var actionPanel = new StackPanel
        {
            Orientation = Orientation.Horizontal,
            HorizontalAlignment = HorizontalAlignment.Right,
            Margin = new Thickness(0, 5, 0, 0)
        };

        var showDetailsBtn = CreateActionButton("+ Weitere Metadaten anzeigen", "#2196F3");
        showDetailsBtn.Click += (s, e) => ShowDetailsRequested?.Invoke(this, EventArgs.Empty);

        var editBtn = CreateActionButton("✏️ Bearbeiten", "#4CAF50");
        editBtn.Click += (s, e) => EditRequested?.Invoke(this, EventArgs.Empty);

        actionPanel.Children.Add(showDetailsBtn);
        actionPanel.Children.Add(editBtn);
        mainPanel.Children.Add(actionPanel);

        Content = new Border
        {
            Child = mainPanel,
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(224, 224, 224)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4),
            Padding = new Thickness(12)
        };
    }

    private Button CreateActionButton(string text, string colorHex)
    {
        var color = ColorFromHex(colorHex);
        return new Button
        {
            Content = text,
            Padding = new Thickness(10, 5, 10, 5),
            Margin = new Thickness(5, 0, 0, 0),
            Background = new SolidColorBrush(color),
            Foreground = new SolidColorBrush(Colors.White),
            BorderThickness = new Thickness(0),
            FontSize = 11,
            Cursor = System.Windows.Input.Cursors.Hand
        };
    }

    private static void OnBadgesChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is MetadataCompactDisplay display && e.NewValue is List<MetadataBadge> badges)
        {
            display.RenderBadges(badges);
        }
    }

    public void RenderBadges(List<MetadataBadge> badges)
    {
        if (_badgePanel == null) return;
        _badgePanel.Children.Clear();

        var visibleBadges = badges.Take(MaxVisibleBadges).ToList();
        var hiddenCount = badges.Count - visibleBadges.Count;

        foreach (var badge in visibleBadges)
        {
            var badgeControl = CreateBadgeControl(badge);
            _badgePanel.Children.Add(badgeControl);
        }

        // "+X weitere" Badge wenn mehr als MaxVisibleBadges
        if (hiddenCount > 0)
        {
            var moreBadge = CreateMoreBadge(hiddenCount);
            _badgePanel.Children.Add(moreBadge);
        }
    }

    private Border CreateBadgeControl(MetadataBadge badge)
    {
        var bgColor = ColorFromHex(badge.Style?.BackgroundColor ?? "#E0E0E0");
        var textColor = ColorFromHex(badge.Style?.TextColor ?? "#000000");

        var badgeText = new TextBlock
        {
            Text = $"{badge.Style?.Icon} {badge.DisplayText}",
            Foreground = new SolidColorBrush(textColor),
            FontSize = 11,
            FontWeight = FontWeights.SemiBold,
            VerticalAlignment = VerticalAlignment.Center
        };

        var badgeBorder = new Border
        {
            Child = badgeText,
            Background = new SolidColorBrush(bgColor),
            BorderBrush = new SolidColorBrush(ColorFromHex(badge.Style?.BorderColor ?? "#CCCCCC")),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(12),
            Padding = new Thickness(10, 4, 10, 4),
            Margin = new Thickness(3, 2, 3, 2),
            Cursor = System.Windows.Input.Cursors.Hand,
            Tag = badge
        };

        // Tooltip
        if (badge.Style?.ShowTooltip == true && !string.IsNullOrEmpty(badge.Tooltip))
        {
            badgeBorder.ToolTip = badge.Tooltip;
        }

        // Click Handler
        badgeBorder.MouseLeftButtonDown += (s, e) =>
        {
            BadgeClicked?.Invoke(this, new BadgeClickedEventArgs { Badge = badge });
        };

        // Hover Effect
        badgeBorder.MouseEnter += (s, e) =>
        {
            badgeBorder.Background = new SolidColorBrush(DarkenColor(bgColor, 0.1f));
        };
        badgeBorder.MouseLeave += (s, e) =>
        {
            badgeBorder.Background = new SolidColorBrush(bgColor);
        };

        return badgeBorder;
    }

    private Border CreateMoreBadge(int count)
    {
        var badgeText = new TextBlock
        {
            Text = $"+{count} weitere",
            Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100)),
            FontSize = 10,
            FontStyle = FontStyles.Italic
        };

        return new Border
        {
            Child = badgeText,
            Background = new SolidColorBrush(Color.FromRgb(240, 240, 240)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(12),
            Padding = new Thickness(10, 4, 10, 4),
            Margin = new Thickness(3, 2, 3, 2),
            Cursor = System.Windows.Input.Cursors.Hand
        };
    }

    private Color ColorFromHex(string hex)
    {
        try
        {
            hex = hex.TrimStart('#');
            if (hex.Length == 6)
            {
                var r = byte.Parse(hex.Substring(0, 2), System.Globalization.NumberStyles.HexNumber);
                var g = byte.Parse(hex.Substring(2, 2), System.Globalization.NumberStyles.HexNumber);
                var b = byte.Parse(hex.Substring(4, 2), System.Globalization.NumberStyles.HexNumber);
                return Color.FromRgb(r, g, b);
            }
        }
        catch { }
        return Color.FromRgb(224, 224, 224);
    }

    private Color DarkenColor(Color color, float factor)
    {
        return Color.FromRgb(
            (byte)(color.R * (1 - factor)),
            (byte)(color.G * (1 - factor)),
            (byte)(color.B * (1 - factor))
        );
    }
}

public class BadgeClickedEventArgs : EventArgs
{
    public MetadataBadge? Badge { get; set; }
}
