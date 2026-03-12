<?php
/**
 * Title: Feature Cards – 3×2 Grid
 * Slug: themisdb-v2/feature-cards
 * Categories: themisdb, themisdb-landing
 * Viewport Width: 1280
 * Description: Six product/capability feature cards in a responsive grid.
 */
?>
<!-- wp:group {"style":{"spacing":{"padding":{"top":"var:preset|spacing|20","bottom":"var:preset|spacing|20"}}},"layout":{"type":"constrained","contentSize":"1100px"}} -->
<div class="wp-block-group" style="padding-top:var(--wp--preset--spacing--20);padding-bottom:var(--wp--preset--spacing--20)">
	<!-- wp:heading {"level":2,"textAlign":"center","style":{"typography":{"fontSize":"clamp(1.75rem,3vw,2.5rem)","fontWeight":"700","letterSpacing":"-0.02em"},"color":{"text":"#2c3e50"},"spacing":{"margin":{"bottom":"var:preset|spacing|4"}}}} -->
	<h2 class="wp-block-heading has-text-align-center" style="color:#2c3e50;font-size:clamp(1.75rem,3vw,2.5rem);font-weight:700;letter-spacing:-0.02em;margin-bottom:var(--wp--preset--spacing--4)">Everything you need in one database</h2>
	<!-- /wp:heading -->
	<!-- wp:paragraph {"textAlign":"center","style":{"typography":{"fontSize":"1.0625rem"},"color":{"text":"#7f8c8d"},"spacing":{"margin":{"bottom":"var:preset|spacing|12"}}}} -->
	<p class="has-text-align-center" style="color:#7f8c8d;font-size:1.0625rem;margin-bottom:var(--wp--preset--spacing--12)">Designed from the ground up for modern applications that need speed, flexibility, and intelligence.</p>
	<!-- /wp:paragraph -->
	<!-- wp:html -->
	<div class="themis-product-grid" style="display:grid;grid-template-columns:repeat(3,1fr);gap:1.5rem;">
		<a href="/features/multi-model" class="themis-product-card" style="display:flex;flex-direction:column;gap:0.75rem;padding:1.5rem;border:1px solid #dee2e6;border-radius:10px;text-decoration:none;background:#fff;">
			<div style="width:48px;height:48px;background:linear-gradient(135deg,#3498db,#2980b9);border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:1.5rem;">🗄️</div>
			<div style="font-size:1.0625rem;font-weight:700;color:#2c3e50;">Multi-Model Storage</div>
			<div style="font-size:0.875rem;color:#7f8c8d;line-height:1.55;">Relational, document, graph, time-series — one engine, zero compromise.</div>
			<div style="font-size:0.8125rem;color:#3498db;font-weight:600;margin-top:auto;">Learn more →</div>
		</a>
		<a href="/features/ai-integration" class="themis-product-card" style="display:flex;flex-direction:column;gap:0.75rem;padding:1.5rem;border:1px solid #dee2e6;border-radius:10px;text-decoration:none;background:#fff;">
			<div style="width:48px;height:48px;background:linear-gradient(135deg,#7c4dff,#651fff);border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:1.5rem;">🤖</div>
			<div style="font-size:1.0625rem;font-weight:700;color:#2c3e50;">Native AI/LLM Integration</div>
			<div style="font-size:0.875rem;color:#7f8c8d;line-height:1.55;">Vector search, semantic queries, and LLM pipelines in the database layer.</div>
			<div style="font-size:0.8125rem;color:#3498db;font-weight:600;margin-top:auto;">Learn more →</div>
		</a>
		<a href="/benchmarks" class="themis-product-card" style="display:flex;flex-direction:column;gap:0.75rem;padding:1.5rem;border:1px solid #dee2e6;border-radius:10px;text-decoration:none;background:#fff;">
			<div style="width:48px;height:48px;background:linear-gradient(135deg,#27ae60,#1e8449);border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:1.5rem;">⚡</div>
			<div style="font-size:1.0625rem;font-weight:700;color:#2c3e50;">Extreme Performance</div>
			<div style="font-size:0.875rem;color:#7f8c8d;line-height:1.55;">Up to 10× faster than traditional relational databases. See benchmarks.</div>
			<div style="font-size:0.8125rem;color:#3498db;font-weight:600;margin-top:auto;">View benchmarks →</div>
		</a>
		<a href="/docker" class="themis-product-card" style="display:flex;flex-direction:column;gap:0.75rem;padding:1.5rem;border:1px solid #dee2e6;border-radius:10px;text-decoration:none;background:#fff;">
			<div style="width:48px;height:48px;background:linear-gradient(135deg,#0db7ed,#0a7cb5);border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:1.5rem;">🐳</div>
			<div style="font-size:1.0625rem;font-weight:700;color:#2c3e50;">Docker Ready</div>
			<div style="font-size:0.875rem;color:#7f8c8d;line-height:1.55;">Official images. Deploy on any platform in under 60 seconds.</div>
			<div style="font-size:0.8125rem;color:#3498db;font-weight:600;margin-top:auto;">Docker Hub →</div>
		</a>
		<a href="/query-playground" class="themis-product-card" style="display:flex;flex-direction:column;gap:0.75rem;padding:1.5rem;border:1px solid #dee2e6;border-radius:10px;text-decoration:none;background:#fff;">
			<div style="width:48px;height:48px;background:linear-gradient(135deg,#f39c12,#e67e22);border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:1.5rem;">🧪</div>
			<div style="font-size:1.0625rem;font-weight:700;color:#2c3e50;">Interactive Query Playground</div>
			<div style="font-size:0.875rem;color:#7f8c8d;line-height:1.55;">Write and execute queries in-browser without installing anything.</div>
			<div style="font-size:0.8125rem;color:#3498db;font-weight:600;margin-top:auto;">Try it now →</div>
		</a>
		<a href="/features/analytics" class="themis-product-card" style="display:flex;flex-direction:column;gap:0.75rem;padding:1.5rem;border:1px solid #dee2e6;border-radius:10px;text-decoration:none;background:#fff;">
			<div style="width:48px;height:48px;background:linear-gradient(135deg,#e74c3c,#c0392b);border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:1.5rem;">📊</div>
			<div style="font-size:1.0625rem;font-weight:700;color:#2c3e50;">Built-in Analytics</div>
			<div style="font-size:0.875rem;color:#7f8c8d;line-height:1.55;">Real-time metrics, dashboards, and query analytics built in.</div>
			<div style="font-size:0.8125rem;color:#3498db;font-weight:600;margin-top:auto;">Learn more →</div>
		</a>
	</div>
	<!-- /wp:html -->
</div>
<!-- /wp:group -->
