/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LRUCacheService.cs                                 ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:24:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     276                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// LRU Cache für Embeddings und LLM-Responses
    /// </summary>
    public interface ICacheService
    {
        bool TryGetEmbedding(string text, out double[]? embedding);
        void SetEmbedding(string text, double[] embedding);
        
        bool TryGetLLMResponse(string prompt, out string? response);
        void SetLLMResponse(string prompt, string response);
        
        void Clear();
        CacheStatistics GetStatistics();
    }

    public class LRUCacheService : ICacheService
    {
        private readonly ConcurrentDictionary<string, CacheEntry<double[]>> _embeddingCache;
        private readonly ConcurrentDictionary<string, CacheEntry<string>> _llmResponseCache;
        private readonly int _maxCacheSize;
        private readonly TimeSpan _cacheTTL;
        private readonly ILoggerService _loggerService;

        // Cache-Statistiken
        private int _embeddingHits;
        private int _embeddingMisses;
        private int _llmResponseHits;
        private int _llmResponseMisses;

        private class CacheEntry<T>
        {
            public T Value { get; set; } = default!;
            public DateTime CreatedAt { get; set; }
            public DateTime LastAccessedAt { get; set; }
            public int AccessCount { get; set; }
        }

        public LRUCacheService(ILoggerService loggerService, int maxCacheSize = 1000, int ttlMinutes = 60)
        {
            _loggerService = loggerService;
            _maxCacheSize = maxCacheSize;
            _cacheTTL = TimeSpan.FromMinutes(ttlMinutes);
            
            _embeddingCache = new ConcurrentDictionary<string, CacheEntry<double[]>>();
            _llmResponseCache = new ConcurrentDictionary<string, CacheEntry<string>>();

            _loggerService.LogInfo($"LRUCacheService initialized: maxSize={maxCacheSize}, ttl={ttlMinutes}min");
        }

        public bool TryGetEmbedding(string text, out double[]? embedding)
        {
            if (string.IsNullOrEmpty(text))
            {
                embedding = null;
                return false;
            }

            var key = HashText(text);

            if (_embeddingCache.TryGetValue(key, out var entry))
            {
                // Prüfe TTL
                if (DateTime.UtcNow - entry.CreatedAt > _cacheTTL)
                {
                    _embeddingCache.TryRemove(key, out _);
                    embedding = null;
                    _embeddingMisses++;
                    return false;
                }

                // Update access info
                entry.LastAccessedAt = DateTime.UtcNow;
                entry.AccessCount++;

                embedding = entry.Value;
                _embeddingHits++;
                return true;
            }

            embedding = null;
            _embeddingMisses++;
            return false;
        }

        public void SetEmbedding(string text, double[] embedding)
        {
            if (string.IsNullOrEmpty(text) || embedding == null || embedding.Length == 0)
                return;

            // LRU: Wenn Cache voll, entferne least recently used
            if (_embeddingCache.Count >= _maxCacheSize)
            {
                EvictLRUEmbedding();
            }

            var key = HashText(text);
            var entry = new CacheEntry<double[]>
            {
                Value = embedding,
                CreatedAt = DateTime.UtcNow,
                LastAccessedAt = DateTime.UtcNow,
                AccessCount = 0
            };

            _embeddingCache.AddOrUpdate(key, entry, (k, old) => entry);
        }

        public bool TryGetLLMResponse(string prompt, out string? response)
        {
            if (string.IsNullOrEmpty(prompt))
            {
                response = null;
                return false;
            }

            var key = HashText(prompt);

            if (_llmResponseCache.TryGetValue(key, out var entry))
            {
                // Prüfe TTL
                if (DateTime.UtcNow - entry.CreatedAt > _cacheTTL)
                {
                    _llmResponseCache.TryRemove(key, out _);
                    response = null;
                    _llmResponseMisses++;
                    return false;
                }

                // Update access info
                entry.LastAccessedAt = DateTime.UtcNow;
                entry.AccessCount++;

                response = entry.Value;
                _llmResponseHits++;
                return true;
            }

            response = null;
            _llmResponseMisses++;
            return false;
        }

        public void SetLLMResponse(string prompt, string response)
        {
            if (string.IsNullOrEmpty(prompt) || string.IsNullOrEmpty(response))
                return;

            // LRU: Wenn Cache voll, entferne least recently used
            if (_llmResponseCache.Count >= _maxCacheSize)
            {
                EvictLRULLMResponse();
            }

            var key = HashText(prompt);
            var entry = new CacheEntry<string>
            {
                Value = response,
                CreatedAt = DateTime.UtcNow,
                LastAccessedAt = DateTime.UtcNow,
                AccessCount = 0
            };

            _llmResponseCache.AddOrUpdate(key, entry, (k, old) => entry);
        }

        public void Clear()
        {
            _embeddingCache.Clear();
            _llmResponseCache.Clear();
            _loggerService.LogInfo("Cache cleared");
        }

        public CacheStatistics GetStatistics()
        {
            var embeddingTotal = _embeddingHits + _embeddingMisses;
            var llmTotal = _llmResponseHits + _llmResponseMisses;

            return new CacheStatistics
            {
                EmbeddingCacheSize = _embeddingCache.Count,
                LLMResponseCacheSize = _llmResponseCache.Count,
                EmbeddingHits = _embeddingHits,
                EmbeddingMisses = _embeddingMisses,
                EmbeddingHitRate = embeddingTotal > 0 ? (double)_embeddingHits / embeddingTotal : 0,
                LLMResponseHits = _llmResponseHits,
                LLMResponseMisses = _llmResponseMisses,
                LLMResponseHitRate = llmTotal > 0 ? (double)_llmResponseHits / llmTotal : 0
            };
        }

        private void EvictLRUEmbedding()
        {
            var lruEntry = _embeddingCache
                .OrderBy(x => x.Value.LastAccessedAt)
                .FirstOrDefault();

            if (!lruEntry.Equals(default(KeyValuePair<string, CacheEntry<double[]>>)))
            {
                _embeddingCache.TryRemove(lruEntry.Key, out _);
                _loggerService.LogInfo("LRU evicted from embedding cache");
            }
        }

        private void EvictLRULLMResponse()
        {
            var lruEntry = _llmResponseCache
                .OrderBy(x => x.Value.LastAccessedAt)
                .FirstOrDefault();

            if (!lruEntry.Equals(default(KeyValuePair<string, CacheEntry<string>>)))
            {
                _llmResponseCache.TryRemove(lruEntry.Key, out _);
                _loggerService.LogInfo("LRU evicted from LLM response cache");
            }
        }

        private string HashText(string text)
        {
            // Einfacher Hash für Cache-Keys
            using (var sha256 = System.Security.Cryptography.SHA256.Create())
            {
                var hash = sha256.ComputeHash(System.Text.Encoding.UTF8.GetBytes(text));
                return Convert.ToBase64String(hash);
            }
        }
    }

    public class CacheStatistics
    {
        public int EmbeddingCacheSize { get; set; }
        public int LLMResponseCacheSize { get; set; }
        public int EmbeddingHits { get; set; }
        public int EmbeddingMisses { get; set; }
        public double EmbeddingHitRate { get; set; }
        public int LLMResponseHits { get; set; }
        public int LLMResponseMisses { get; set; }
        public double LLMResponseHitRate { get; set; }
        public long TotalEmbeddingsGenerated { get; set; } = 0;
        public long TotalLLMResponsesGenerated { get; set; } = 0;

        public override string ToString()
        {
            return $"Cache Stats - Embeddings: {EmbeddingCacheSize} items, {EmbeddingHitRate:P} hit rate | " +
                   $"LLM Responses: {LLMResponseCacheSize} items, {LLMResponseHitRate:P} hit rate";
        }
    }
}
