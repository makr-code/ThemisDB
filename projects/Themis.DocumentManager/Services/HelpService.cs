/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            HelpService.cs                                     ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     492                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

// ============================================================================
// Integrated Help System Service
// ============================================================================

#region Help Service

public interface IHelpService
{
    // Articles
    Task<HelpArticle?> GetArticleAsync(string articleId, CancellationToken cancellationToken = default);
    Task<List<HelpArticle>> GetArticlesByCategoryAsync(string category, CancellationToken cancellationToken = default);
    Task<List<HelpArticle>> GetAllArticlesAsync(CancellationToken cancellationToken = default);
    Task<HelpArticle> CreateArticleAsync(HelpArticle article, CancellationToken cancellationToken = default);
    Task UpdateArticleAsync(HelpArticle article, CancellationToken cancellationToken = default);
    
    // Search
    Task<List<HelpSearchResult>> SearchAsync(HelpSearchQuery query, CancellationToken cancellationToken = default);
    Task<List<HelpArticle>> GetRelatedArticlesAsync(string articleId, int limit = 5, CancellationToken cancellationToken = default);
    
    // Contextual Help
    Task<ContextualHelp?> GetContextualHelpAsync(string context, CancellationToken cancellationToken = default);
    Task<List<HelpShortcut>> GetShortcutsAsync(string? category = null, CancellationToken cancellationToken = default);
    
    // Tours
    Task<InteractiveTour?> GetTourAsync(string tourId, CancellationToken cancellationToken = default);
    Task<List<InteractiveTour>> GetAvailableToursAsync(CancellationToken cancellationToken = default);
    Task<TourProgress?> GetTourProgressAsync(string userId, string tourId, CancellationToken cancellationToken = default);
    Task UpdateTourProgressAsync(TourProgress progress, CancellationToken cancellationToken = default);
    
    // Feedback
    Task<HelpFeedback> SubmitFeedbackAsync(HelpFeedback feedback, CancellationToken cancellationToken = default);
    Task<HelpRequest> SubmitQuestionAsync(HelpRequest request, CancellationToken cancellationToken = default);
    Task<List<HelpRequest>> GetUserQuestionsAsync(string userId, CancellationToken cancellationToken = default);
    
    // Analytics
    Task RecordViewAsync(string articleId, string userId, CancellationToken cancellationToken = default);
    Task<HelpUsageStatistics?> GetStatisticsAsync(string articleId, CancellationToken cancellationToken = default);
    
    // Navigation
    Task<List<HelpNavigationNode>> GetNavigationAsync(CancellationToken cancellationToken = default);
}

public class HelpService : IHelpService
{
    private readonly IThemisDBService _themisDb;
    private readonly List<HelpArticle> _builtInArticles;
    private readonly List<HelpShortcut> _builtInShortcuts;
    
    public HelpService(IThemisDBService themisDb)
    {
        ArgumentNullException.ThrowIfNull(themisDb);
        _themisDb = themisDb;
        
        // Load built-in content
        _builtInArticles = ThemisDBHelpArticles.GetQuickStartArticles();
        _builtInShortcuts = ThemisDBHelpArticles.GetStandardShortcuts();
    }
    
    public async Task<HelpArticle?> GetArticleAsync(string articleId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(articleId);
        
        // Check built-in articles first
        var builtIn = _builtInArticles.FirstOrDefault(a => a.Id == articleId);
        if (builtIn != null) return builtIn;
        
        // Query database
        var query = "FOR article IN help_articles FILTER article.Id == @articleId RETURN article";
        var result = await _themisDb.ExecuteQueryAsync<HelpArticle>(
            query,
            new { articleId },
            cancellationToken
        );
        
        return result.FirstOrDefault();
    }
    
    public async Task<List<HelpArticle>> GetArticlesByCategoryAsync(string category, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(category);
        
        var articles = new List<HelpArticle>();
        
        // Built-in articles
        articles.AddRange(_builtInArticles.Where(a => a.Category == category));
        
        // Database articles
        var query = "FOR article IN help_articles FILTER article.Category == @category RETURN article";
        var dbArticles = await _themisDb.ExecuteQueryAsync<HelpArticle>(
            query,
            new { category },
            cancellationToken
        );
        
        articles.AddRange(dbArticles);
        
        return articles.OrderBy(a => a.Title).ToList();
    }
    
    public async Task<List<HelpArticle>> GetAllArticlesAsync(CancellationToken cancellationToken = default)
    {
        var articles = new List<HelpArticle>();
        
        // Built-in articles
        articles.AddRange(_builtInArticles);
        
        // Database articles
        var query = "FOR article IN help_articles RETURN article";
        var dbArticles = await _themisDb.ExecuteQueryAsync<HelpArticle>(query, null, cancellationToken);
        
        articles.AddRange(dbArticles);
        
        return articles.OrderBy(a => a.Category).ThenBy(a => a.Title).ToList();
    }
    
    public async Task<HelpArticle> CreateArticleAsync(HelpArticle article, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(article);
        
        article.CreatedAt = DateTime.UtcNow;
        article.UpdatedAt = DateTime.UtcNow;
        
        var query = "INSERT @article INTO help_articles RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<HelpArticle>(
            query,
            new { article },
            cancellationToken
        );
        
        return result.FirstOrDefault() ?? article;
    }
    
    public async Task UpdateArticleAsync(HelpArticle article, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(article);
        
        article.UpdatedAt = DateTime.UtcNow;
        
        var query = "UPDATE @article IN help_articles RETURN NEW";
        await _themisDb.ExecuteQueryAsync<HelpArticle>(
            query,
            new { article },
            cancellationToken
        );
    }
    
    public async Task<List<HelpSearchResult>> SearchAsync(HelpSearchQuery query, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(query);
        
        var allArticles = await GetAllArticlesAsync(cancellationToken);
        var results = new List<HelpSearchResult>();
        
        var searchTerms = query.Query.ToLower().Split(' ', StringSplitOptions.RemoveEmptyEntries);
        
        foreach (var article in allArticles)
        {
            var relevance = CalculateRelevance(article, searchTerms);
            
            if (relevance > 0)
            {
                // Apply filters
                if (query.Categories.Any() && !query.Categories.Contains(article.Category))
                    continue;
                    
                if (query.Types.Any() && !query.Types.Contains(article.Type))
                    continue;
                    
                if (query.Audience.HasValue && article.Audience != HelpTargetAudience.All && article.Audience != query.Audience.Value)
                    continue;
                
                results.Add(new HelpSearchResult
                {
                    ArticleId = article.Id,
                    Title = article.Title,
                    Excerpt = article.Excerpt,
                    Type = article.Type,
                    Category = article.Category,
                    Relevance = relevance,
                    MatchedKeywords = article.Keywords.Where(k => searchTerms.Any(t => k.ToLower().Contains(t))).ToList()
                });
            }
        }
        
        return results
            .OrderByDescending(r => r.Relevance)
            .Take(query.MaxResults)
            .ToList();
    }
    
    private static double CalculateRelevance(HelpArticle article, string[] searchTerms)
    {
        double relevance = 0;
        
        foreach (var term in searchTerms)
        {
            // Title match (highest weight)
            if (article.Title.ToLower().Contains(term))
                relevance += 10;
            
            // Keywords match
            if (article.Keywords.Any(k => k.ToLower().Contains(term)))
                relevance += 5;
            
            // Tags match
            if (article.Tags.Any(t => t.ToLower().Contains(term)))
                relevance += 3;
            
            // Content match (lowest weight)
            if (article.Content.ToLower().Contains(term))
                relevance += 1;
        }
        
        return relevance;
    }
    
    public async Task<List<HelpArticle>> GetRelatedArticlesAsync(string articleId, int limit = 5, CancellationToken cancellationToken = default)
    {
        var article = await GetArticleAsync(articleId, cancellationToken);
        if (article == null) return new List<HelpArticle>();
        
        var allArticles = await GetAllArticlesAsync(cancellationToken);
        
        // Find articles with matching tags or same category
        var related = allArticles
            .Where(a => a.Id != articleId)
            .Where(a => a.Category == article.Category || a.Tags.Any(t => article.Tags.Contains(t)))
            .OrderByDescending(a => a.Tags.Count(t => article.Tags.Contains(t)))
            .Take(limit)
            .ToList();
        
        return related;
    }
    
    public Task<ContextualHelp?> GetContextualHelpAsync(string context, CancellationToken cancellationToken = default)
    {
        // Placeholder - would load context-specific help
        var contextHelp = new ContextualHelp
        {
            Context = context,
            QuickTip = $"Hilfe für {context}",
            Tips = new List<HelpTip>
            {
                new HelpTip
                {
                    Title = "Tipp",
                    Content = $"Nutzen Sie F1 für weitere Hilfe zu {context}",
                    Level = HelpTipLevel.Info
                }
            }
        };
        
        return Task.FromResult<ContextualHelp?>(contextHelp);
    }
    
    public Task<List<HelpShortcut>> GetShortcutsAsync(string? category = null, CancellationToken cancellationToken = default)
    {
        var shortcuts = _builtInShortcuts;
        
        if (!string.IsNullOrEmpty(category))
        {
            shortcuts = shortcuts.Where(s => s.Category == category).ToList();
        }
        
        return Task.FromResult(shortcuts);
    }
    
    public async Task<InteractiveTour?> GetTourAsync(string tourId, CancellationToken cancellationToken = default)
    {
        var query = "FOR tour IN interactive_tours FILTER tour.Id == @tourId RETURN tour";
        var result = await _themisDb.ExecuteQueryAsync<InteractiveTour>(
            query,
            new { tourId },
            cancellationToken
        );
        
        return result.FirstOrDefault();
    }
    
    public async Task<List<InteractiveTour>> GetAvailableToursAsync(CancellationToken cancellationToken = default)
    {
        var query = "FOR tour IN interactive_tours SORT tour.IsRequired DESC, tour.Title ASC RETURN tour";
        return await _themisDb.ExecuteQueryAsync<InteractiveTour>(query, null, cancellationToken);
    }
    
    public async Task<TourProgress?> GetTourProgressAsync(string userId, string tourId, CancellationToken cancellationToken = default)
    {
        var query = @"
            FOR progress IN tour_progress
            FILTER progress.UserId == @userId AND progress.TourId == @tourId
            RETURN progress";
            
        var result = await _themisDb.ExecuteQueryAsync<TourProgress>(
            query,
            new { userId, tourId },
            cancellationToken
        );
        
        return result.FirstOrDefault();
    }
    
    public async Task UpdateTourProgressAsync(TourProgress progress, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(progress);
        
        progress.LastAccessedAt = DateTime.UtcNow;
        
        var query = "UPSERT { UserId: @userId, TourId: @tourId } INSERT @progress UPDATE @progress IN tour_progress";
        await _themisDb.ExecuteQueryAsync<object>(
            query,
            new { userId = progress.UserId, tourId = progress.TourId, progress },
            cancellationToken
        );
    }
    
    public async Task<HelpFeedback> SubmitFeedbackAsync(HelpFeedback feedback, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(feedback);
        
        feedback.CreatedAt = DateTime.UtcNow;
        
        var query = "INSERT @feedback INTO help_feedback RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<HelpFeedback>(
            query,
            new { feedback },
            cancellationToken
        );
        
        return result.FirstOrDefault() ?? feedback;
    }
    
    public async Task<HelpRequest> SubmitQuestionAsync(HelpRequest request, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(request);
        
        request.CreatedAt = DateTime.UtcNow;
        request.Status = HelpRequestStatus.Open;
        
        var query = "INSERT @request INTO help_requests RETURN NEW";
        var result = await _themisDb.ExecuteQueryAsync<HelpRequest>(
            query,
            new { request },
            cancellationToken
        );
        
        return result.FirstOrDefault() ?? request;
    }
    
    public async Task<List<HelpRequest>> GetUserQuestionsAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(userId);
        
        var query = @"
            FOR request IN help_requests
            FILTER request.UserId == @userId
            SORT request.CreatedAt DESC
            RETURN request";
            
        return await _themisDb.ExecuteQueryAsync<HelpRequest>(
            query,
            new { userId },
            cancellationToken
        );
    }
    
    public async Task RecordViewAsync(string articleId, string userId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(articleId);
        ArgumentException.ThrowIfNullOrEmpty(userId);
        
        // Increment view count
        var query = @"
            FOR article IN help_articles
            FILTER article.Id == @articleId
            UPDATE article WITH { Views: article.Views + 1 } IN help_articles";
            
        await _themisDb.ExecuteQueryAsync<object>(
            query,
            new { articleId },
            cancellationToken
        );
    }
    
    public async Task<HelpUsageStatistics?> GetStatisticsAsync(string articleId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(articleId);
        
        var article = await GetArticleAsync(articleId, cancellationToken);
        if (article == null) return null;
        
        // Get feedback
        var feedbackQuery = @"
            FOR feedback IN help_feedback
            FILTER feedback.ArticleId == @articleId
            RETURN feedback";
            
        var feedbacks = await _themisDb.ExecuteQueryAsync<HelpFeedback>(
            feedbackQuery,
            new { articleId },
            cancellationToken
        );
        
        var helpfulCount = feedbacks.Count(f => f.IsHelpful);
        var notHelpfulCount = feedbacks.Count(f => !f.IsHelpful);
        var ratings = feedbacks.Where(f => f.Rating.HasValue).Select(f => f.Rating!.Value).ToList();
        var avgRating = ratings.Any() ? ratings.Average() : 0.0;
        
        return new HelpUsageStatistics
        {
            ArticleId = articleId,
            TotalViews = article.Views,
            UniqueViews = article.Views, // Simplified
            AverageRating = avgRating,
            HelpfulCount = helpfulCount,
            NotHelpfulCount = notHelpfulCount
        };
    }
    
    public Task<List<HelpNavigationNode>> GetNavigationAsync(CancellationToken cancellationToken = default)
    {
        var categories = ThemisDBHelpCategories.GetStandardCategories();
        var navigation = new List<HelpNavigationNode>();
        
        foreach (var category in categories)
        {
            navigation.Add(new HelpNavigationNode
            {
                Title = category,
                Icon = GetCategoryIcon(category),
                Children = new List<HelpNavigationNode>()
            });
        }
        
        return Task.FromResult(navigation);
    }
    
    private static string GetCategoryIcon(string category)
    {
        return category switch
        {
            "Erste Schritte" => "🚀",
            "Posteingang & Postausgang" => "📬",
            "Vorgangsbearbeitung" => "📋",
            "Dokumentenverwaltung" => "📄",
            "Suche & Filter" => "🔍",
            "Timeline & Gantt" => "📅",
            "AI-Assistent" => "🤖",
            "Benachrichtigungen" => "🔔",
            "Administration" => "⚙️",
            "Tastenkombinationen" => "⌨️",
            "FAQs" => "❓",
            _ => "📖"
        };
    }
}

#endregion
