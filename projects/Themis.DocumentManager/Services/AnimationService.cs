/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AnimationService.cs                                ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     232                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Windows;
using System.Windows.Media.Animation;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service für UI-Animationen und Transitions.
/// Phase 28 - Animation System.
/// </summary>
public class AnimationService : IAnimationService
{
    private bool _isEnabled = true;

    /// <summary>
    /// Animationen aktiviert/deaktiviert
    /// </summary>
    public bool IsEnabled
    {
        get => _isEnabled;
        set => _isEnabled = value;
    }

    /// <summary>
    /// Fade-In Animation
    /// </summary>
    public void FadeIn(UIElement element, int durationMs = 300)
    {
        if (!_isEnabled || element == null) return;

        var animation = new DoubleAnimation
        {
            From = 0,
            To = 1,
            Duration = TimeSpan.FromMilliseconds(durationMs),
            EasingFunction = new QuadraticEase { EasingMode = EasingMode.EaseOut }
        };

        element.BeginAnimation(UIElement.OpacityProperty, animation);
    }

    /// <summary>
    /// Fade-Out Animation
    /// </summary>
    public void FadeOut(UIElement element, int durationMs = 300, Action? onComplete = null)
    {
        if (!_isEnabled || element == null)
        {
            onComplete?.Invoke();
            return;
        }

        var animation = new DoubleAnimation
        {
            From = 1,
            To = 0,
            Duration = TimeSpan.FromMilliseconds(durationMs),
            EasingFunction = new QuadraticEase { EasingMode = EasingMode.EaseIn }
        };

        if (onComplete != null)
        {
            animation.Completed += (s, e) => onComplete();
        }

        element.BeginAnimation(UIElement.OpacityProperty, animation);
    }

    /// <summary>
    /// Slide-In Animation (von rechts)
    /// </summary>
    public void SlideIn(FrameworkElement element, int durationMs = 300)
    {
        if (!_isEnabled || element == null) return;

        var transform = new System.Windows.Media.TranslateTransform(20, 0);
        element.RenderTransform = transform;

        var animation = new DoubleAnimation
        {
            From = 20,
            To = 0,
            Duration = TimeSpan.FromMilliseconds(durationMs),
            EasingFunction = new QuadraticEase { EasingMode = EasingMode.EaseOut }
        };

        transform.BeginAnimation(System.Windows.Media.TranslateTransform.XProperty, animation);
    }

    /// <summary>
    /// Slide-Out Animation (nach rechts)
    /// </summary>
    public void SlideOut(FrameworkElement element, int durationMs = 300, Action? onComplete = null)
    {
        if (!_isEnabled || element == null)
        {
            onComplete?.Invoke();
            return;
        }

        var transform = element.RenderTransform as System.Windows.Media.TranslateTransform 
                       ?? new System.Windows.Media.TranslateTransform(0, 0);
        
        element.RenderTransform = transform;

        var animation = new DoubleAnimation
        {
            From = 0,
            To = 20,
            Duration = TimeSpan.FromMilliseconds(durationMs),
            EasingFunction = new QuadraticEase { EasingMode = EasingMode.EaseIn }
        };

        if (onComplete != null)
        {
            animation.Completed += (s, e) => onComplete();
        }

        transform.BeginAnimation(System.Windows.Media.TranslateTransform.XProperty, animation);
    }

    /// <summary>
    /// Scale Animation (Zoom-Effekt)
    /// </summary>
    public void Scale(FrameworkElement element, double fromScale, double toScale, int durationMs = 300)
    {
        if (!_isEnabled || element == null) return;

        var transform = new System.Windows.Media.ScaleTransform(fromScale, fromScale);
        element.RenderTransform = transform;
        element.RenderTransformOrigin = new Point(0.5, 0.5);

        var animationX = new DoubleAnimation
        {
            From = fromScale,
            To = toScale,
            Duration = TimeSpan.FromMilliseconds(durationMs),
            EasingFunction = new QuadraticEase { EasingMode = EasingMode.EaseOut }
        };

        var animationY = new DoubleAnimation
        {
            From = fromScale,
            To = toScale,
            Duration = TimeSpan.FromMilliseconds(durationMs),
            EasingFunction = new QuadraticEase { EasingMode = EasingMode.EaseOut }
        };

        transform.BeginAnimation(System.Windows.Media.ScaleTransform.ScaleXProperty, animationX);
        transform.BeginAnimation(System.Windows.Media.ScaleTransform.ScaleYProperty, animationY);
    }

    /// <summary>
    /// Pulse Animation (Hervorhebung)
    /// </summary>
    public void Pulse(FrameworkElement element, int durationMs = 600)
    {
        if (!_isEnabled || element == null) return;

        var storyboard = new Storyboard();
        
        // Scale up
        var scaleUpX = new DoubleAnimation
        {
            From = 1.0,
            To = 1.05,
            Duration = TimeSpan.FromMilliseconds(durationMs / 2),
            AutoReverse = true
        };

        var scaleUpY = new DoubleAnimation
        {
            From = 1.0,
            To = 1.05,
            Duration = TimeSpan.FromMilliseconds(durationMs / 2),
            AutoReverse = true
        };

        var transform = new System.Windows.Media.ScaleTransform(1, 1);
        element.RenderTransform = transform;
        element.RenderTransformOrigin = new Point(0.5, 0.5);

        Storyboard.SetTarget(scaleUpX, element);
        Storyboard.SetTargetProperty(scaleUpX, new PropertyPath("RenderTransform.ScaleX"));
        Storyboard.SetTarget(scaleUpY, element);
        Storyboard.SetTargetProperty(scaleUpY, new PropertyPath("RenderTransform.ScaleY"));

        storyboard.Children.Add(scaleUpX);
        storyboard.Children.Add(scaleUpY);
        storyboard.Begin();
    }
}

/// <summary>
/// Interface für Animation Service
/// </summary>
public interface IAnimationService
{
    bool IsEnabled { get; set; }
    void FadeIn(UIElement element, int durationMs = 300);
    void FadeOut(UIElement element, int durationMs = 300, Action? onComplete = null);
    void SlideIn(FrameworkElement element, int durationMs = 300);
    void SlideOut(FrameworkElement element, int durationMs = 300, Action? onComplete = null);
    void Scale(FrameworkElement element, double fromScale, double toScale, int durationMs = 300);
    void Pulse(FrameworkElement element, int durationMs = 600);
}
