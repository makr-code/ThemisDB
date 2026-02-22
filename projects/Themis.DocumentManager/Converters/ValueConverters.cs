/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ValueConverters.cs                                 ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🔴 ALPHA                                        ║
    • Quality Score:   22.0/100                                       ║
    • Total Lines:     426                                            ║
    • Open Issues:     TODOs: 0, Stubs: 17                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🚧 Early Development                                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Converters
{
    /// <summary>
    /// Collection of value converters for UI binding
    /// </summary>
    
    public class BoolToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is bool boolValue)
                return boolValue ? Visibility.Visible : Visibility.Collapsed;
            return Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is Visibility visibility && visibility == Visibility.Visible;
        }
    }

    public class InverseBoolConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is bool boolValue ? !boolValue : false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is bool boolValue ? !boolValue : false;
        }
    }

    public class StatusToColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is InboxStatus status)
            {
                return status switch
                {
                    InboxStatus.New => new SolidColorBrush(Color.FromRgb(59, 130, 246)), // Blue
                    InboxStatus.Assigned => new SolidColorBrush(Color.FromRgb(234, 179, 8)), // Yellow
                    InboxStatus.InProgress => new SolidColorBrush(Color.FromRgb(249, 115, 22)), // Orange
                    InboxStatus.Completed => new SolidColorBrush(Color.FromRgb(34, 197, 94)), // Green
                    InboxStatus.Archived => new SolidColorBrush(Color.FromRgb(156, 163, 175)), // Gray
                    _ => new SolidColorBrush(Colors.Gray)
                };
            }
            return new SolidColorBrush(Colors.Gray);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class PriorityToColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is InboxPriority priority)
            {
                return priority switch
                {
                    InboxPriority.Urgent => new SolidColorBrush(Color.FromRgb(220, 38, 38)), // Red
                    InboxPriority.High => new SolidColorBrush(Color.FromRgb(249, 115, 22)), // Orange
                    InboxPriority.Normal => new SolidColorBrush(Color.FromRgb(34, 197, 94)), // Green
                    InboxPriority.Low => new SolidColorBrush(Color.FromRgb(156, 163, 175)), // Gray
                    _ => new SolidColorBrush(Colors.Gray)
                };
            }
            return new SolidColorBrush(Colors.Gray);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class BadgeTypeToColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is BadgeType badgeType)
            {
                return badgeType switch
                {
                    BadgeType.Date => new SolidColorBrush(Color.FromRgb(59, 130, 246)), // Blue
                    BadgeType.Department => new SolidColorBrush(Color.FromRgb(249, 115, 22)), // Orange
                    BadgeType.ProcessType => new SolidColorBrush(Color.FromRgb(168, 85, 247)), // Purple
                    BadgeType.FileReference => new SolidColorBrush(Color.FromRgb(34, 197, 94)), // Green
                    BadgeType.Status => new SolidColorBrush(Color.FromRgb(234, 179, 8)), // Yellow
                    BadgeType.Priority => new SolidColorBrush(Color.FromRgb(220, 38, 38)), // Red
                    BadgeType.Person => new SolidColorBrush(Color.FromRgb(147, 197, 253)), // Light Blue
                    BadgeType.Organization => new SolidColorBrush(Color.FromRgb(244, 114, 182)), // Pink
                    BadgeType.Location => new SolidColorBrush(Color.FromRgb(45, 212, 191)), // Teal
                    BadgeType.Topic => new SolidColorBrush(Color.FromRgb(134, 239, 172)), // Light Green
                    BadgeType.Action => new SolidColorBrush(Color.FromRgb(251, 146, 60)), // Orange-Red
                    BadgeType.Deadline => new SolidColorBrush(Color.FromRgb(185, 28, 28)), // Dark Red
                    _ => new SolidColorBrush(Color.FromRgb(156, 163, 175)) // Gray
                };
            }
            return new SolidColorBrush(Colors.Gray);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class BadgeTypeToIconConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is BadgeType badgeType)
            {
                return badgeType switch
                {
                    BadgeType.Date => "📅",
                    BadgeType.Department => "🏢",
                    BadgeType.ProcessType => "⚙️",
                    BadgeType.FileReference => "📁",
                    BadgeType.Status => "⚡",
                    BadgeType.Priority => "🔥",
                    BadgeType.Person => "👤",
                    BadgeType.Organization => "🏛️",
                    BadgeType.Location => "📍",
                    BadgeType.Topic => "🏷️",
                    BadgeType.Action => "✅",
                    BadgeType.Deadline => "⏰",
                    _ => "📌"
                };
            }
            return "📌";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NullToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value != null ? Visibility.Visible : Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class CountToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is int count)
                return count > 0 ? Visibility.Visible : Visibility.Collapsed;
            return Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NotificationTypeToIconConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is NotificationType notifType)
            {
                return notifType switch
                {
                    NotificationType.Info => "ℹ️",
                    NotificationType.Warning => "⚠️",
                    NotificationType.Error => "❌",
                    NotificationType.Success => "✅",
                    NotificationType.Deadline => "⏰",
                    NotificationType.Task => "📋",
                    NotificationType.Cosigning => "✍️",
                    NotificationType.Escalation => "🔺",
                    NotificationType.System => "⚙️",
                    _ => "📬"
                };
            }
            return "📬";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ConfidenceToOpacityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is double confidence)
            {
                return Math.Max(0.3, Math.Min(1.0, confidence));
            }
            return 1.0;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Convert ChatMessage Role to Background Color (User=Blue, AI=Gray)
    /// </summary>
    public class RoleToColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is string role)
            {
                return role.ToLower() == "user" 
                    ? new SolidColorBrush(Color.FromArgb(255, 220, 240, 255))  // Light Blue
                    : new SolidColorBrush(Color.FromArgb(255, 240, 240, 240)); // Light Gray
            }
            return new SolidColorBrush(Colors.White);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Convert string to Visibility (NonEmpty=Visible, Empty=Collapsed)
    /// Example: "text" -> Visible, null/empty -> Collapsed
    /// </summary>
    public class StringToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is string str)
            {
                return string.IsNullOrEmpty(str) ? Visibility.Collapsed : Visibility.Visible;
            }
            return Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NullToBoolConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value != null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class BoolToColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is bool boolValue && parameter is string colorParams)
            {
                var colors = colorParams.Split('|');
                if (colors.Length == 2)
                {
                    var trueBrush = new SolidColorBrush((Color)ColorConverter.ConvertFromString(colors[0]));
                    var falseBrush = new SolidColorBrush((Color)ColorConverter.ConvertFromString(colors[1]));
                    return boolValue ? trueBrush : falseBrush;
                }
            }
            return new SolidColorBrush(Colors.Transparent);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class EmptyToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is System.Collections.ICollection collection)
            {
                return collection.Count == 0 ? Visibility.Visible : Visibility.Collapsed;
            }
            return Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class TypeToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is string fieldType && parameter is string targetType_str)
            {
                return fieldType == targetType_str ? Visibility.Visible : Visibility.Collapsed;
            }
            return Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Inverts BoolToVisibilityConverter
    /// </summary>
    public class InverseBoolToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is bool boolValue)
                return boolValue ? Visibility.Collapsed : Visibility.Visible;
            return Visibility.Visible;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is Visibility visibility && visibility == Visibility.Collapsed;
        }
    }

    /// <summary>
    /// Converts bool to FontWeight (true=Bold, false=Normal)
    /// </summary>
    public class BoolToFontWeightConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is bool boolValue)
                return boolValue ? FontWeights.Bold : FontWeights.Normal;
            return FontWeights.Normal;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Converts node type string to icon
    /// </summary>
    public class StringToIconConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is string nodeType)
            {
                return nodeType switch
                {
                    "Folder" => "📁",
                    "Document" => "📄",
                    "File" => "📎",
                    "Process" => "⚙️",
                    "Task" => "✓",
                    "User" => "👤",
                    _ => "📌"
                };
            }
            return "📌";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}

