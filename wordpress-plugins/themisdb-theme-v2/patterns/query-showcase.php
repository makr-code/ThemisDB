<?php
/**
 * Title: Query Showcase – Code Demo
 * Slug: themisdb-v2/query-showcase
 * Categories: themisdb, themisdb-landing
 * Viewport Width: 1280
 * Description: Interactive query playground teaser with syntax-highlighted code example.
 */
?>
<!-- wp:group {"style":{"color":{"background":"#1a252f"},"spacing":{"padding":{"top":"var:preset|spacing|20","bottom":"var:preset|spacing|20"}}},"layout":{"type":"constrained","contentSize":"1100px"}} -->
<div class="wp-block-group" style="background-color:#1a252f;padding-top:var(--wp--preset--spacing--20);padding-bottom:var(--wp--preset--spacing--20)">
	<!-- wp:columns {"verticalAlignment":"center","style":{"spacing":{"blockGap":{"left":"var:preset|spacing|12"}}}} -->
	<div class="wp-block-columns are-vertically-aligned-center">
		<!-- wp:column {"verticalAlignment":"center"} -->
		<div class="wp-block-column is-vertically-aligned-center">
			<!-- wp:html --><div style="display:inline-block;background:rgba(52,152,219,0.15);color:#3498db;border-radius:100px;padding:0.2rem 0.75rem;font-size:0.75rem;font-weight:700;letter-spacing:0.06em;text-transform:uppercase;margin-bottom:1rem;">Interactive Playground</div><!-- /wp:html -->
			<!-- wp:heading {"level":2,"style":{"typography":{"fontSize":"clamp(1.5rem,3vw,2.25rem)","fontWeight":"700","letterSpacing":"-0.02em","lineHeight":"1.25"},"color":{"text":"#ffffff"},"spacing":{"margin":{"bottom":"var:preset|spacing|4"}}}} -->
			<h2 class="wp-block-heading" style="color:#ffffff;font-size:clamp(1.5rem,3vw,2.25rem);font-weight:700;letter-spacing:-0.02em;line-height:1.25;margin-bottom:var(--wp--preset--spacing--4)">Try ThemisDB in your browser</h2>
			<!-- /wp:heading -->
			<!-- wp:paragraph {"style":{"typography":{"fontSize":"1rem","lineHeight":"1.65"},"color":{"text":"rgba(255,255,255,0.65)"},"spacing":{"margin":{"bottom":"var:preset|spacing|6"}}}} -->
			<p style="color:rgba(255,255,255,0.65);font-size:1rem;line-height:1.65;margin-bottom:var(--wp--preset--spacing--6)">Write multi-model queries — relational joins, JSON traversal, vector search — and see results instantly. No installation needed.</p>
			<!-- /wp:paragraph -->
			<!-- wp:list {"style":{"typography":{"fontSize":"0.9375rem"},"color":{"text":"rgba(255,255,255,0.8)"},"spacing":{"margin":{"bottom":"var:preset|spacing|8"}}}} -->
			<ul class="wp-block-list" style="font-size:0.9375rem;color:rgba(255,255,255,0.8);margin-bottom:var(--wp--preset--spacing--8)">
				<li>✓ SQL + JSON + Graph queries</li>
				<li>✓ Live result set with explain plan</li>
				<li>✓ Shareable query links</li>
			</ul>
			<!-- /wp:list -->
			<!-- wp:buttons -->
			<div class="wp-block-buttons">
				<!-- wp:button {"style":{"color":{"background":"#3498db","text":"#ffffff"},"border":{"radius":"8px"},"spacing":{"padding":{"top":"0.75rem","bottom":"0.75rem","left":"1.75rem","right":"1.75rem"}}}} -->
				<div class="wp-block-button"><a href="/query-playground" class="wp-block-button__link wp-element-button" style="background-color:#3498db;color:#ffffff;border-radius:8px;padding:0.75rem 1.75rem;font-weight:600">Open Query Playground →</a></div>
				<!-- /wp:button -->
			</div>
			<!-- /wp:buttons -->
		</div>
		<!-- /wp:column -->
		<!-- wp:column -->
		<div class="wp-block-column">
			<!-- wp:html -->
			<div style="background:#0d1117;border-radius:12px;overflow:hidden;box-shadow:0 24px 48px rgba(0,0,0,0.4);">
				<div style="background:#161b22;padding:0.75rem 1rem;display:flex;align-items:center;gap:0.5rem;border-bottom:1px solid rgba(255,255,255,0.08);">
					<div style="width:12px;height:12px;border-radius:50%;background:#e74c3c;"></div>
					<div style="width:12px;height:12px;border-radius:50%;background:#f39c12;"></div>
					<div style="width:12px;height:12px;border-radius:50%;background:#27ae60;"></div>
					<span style="margin-left:0.5rem;font-family:monospace;font-size:0.75rem;color:rgba(255,255,255,0.4);">query.sql</span>
				</div>
				<pre style="margin:0;padding:1.5rem;font-family:'SFMono-Regular',Consolas,monospace;font-size:0.8125rem;line-height:1.7;color:#e2e8f0;overflow-x:auto;"><code><span style="color:#95a5a6;">-- Find products similar to SKU-42 using AI vector search</span>
<span style="color:#7c4dff;">SELECT</span>
  p.sku,
  p.name,
  p.price,
  <span style="color:#3498db;">VECTOR_SIMILARITY</span>(p.embedding, ref.embedding) <span style="color:#7c4dff;">AS</span> score
<span style="color:#7c4dff;">FROM</span>   products p
<span style="color:#7c4dff;">CROSS JOIN</span> (
  <span style="color:#7c4dff;">SELECT</span> embedding <span style="color:#7c4dff;">FROM</span> products <span style="color:#7c4dff;">WHERE</span> sku = <span style="color:#27ae60;">'SKU-42'</span>
) ref
<span style="color:#7c4dff;">WHERE</span>  p.status = <span style="color:#27ae60;">'active'</span>
  <span style="color:#7c4dff;">AND</span>  p.sku != <span style="color:#27ae60;">'SKU-42'</span>
<span style="color:#7c4dff;">ORDER BY</span> score <span style="color:#7c4dff;">DESC</span>
<span style="color:#7c4dff;">LIMIT</span> <span style="color:#f39c12;">10</span>;</code></pre>
				<div style="padding:0.875rem 1.5rem;border-top:1px solid rgba(255,255,255,0.08);font-family:monospace;font-size:0.75rem;color:rgba(255,255,255,0.4);display:flex;justify-content:space-between;">
					<span>✓ 10 rows · 3.2ms</span>
					<a href="/query-playground" style="color:#3498db;text-decoration:none;">Run in playground →</a>
				</div>
			</div>
			<!-- /wp:html -->
		</div>
		<!-- /wp:column -->
	</div>
	<!-- /wp:columns -->
</div>
<!-- /wp:group -->
