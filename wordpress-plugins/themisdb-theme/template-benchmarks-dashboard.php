<?php
/**
 * Template Name: Benchmarks Dashboard
 * Description: Full-width template for displaying ThemisDB performance benchmarks
 *
 * @package ThemisDB
 * @since 1.0.0
 */

get_header();
?>

<div id="primary" class="content-area">
    <main id="main" class="site-main benchmarks-dashboard">

        <?php
        while ( have_posts() ) :
            the_post();
        ?>

            <article id="post-<?php the_ID(); ?>" <?php post_class(); ?>>
                
                <header class="entry-header">
                    <?php the_title( '<h1 class="entry-title">', '</h1>' ); ?>
                    
                    <?php if ( get_the_content() ) : ?>
                        <div class="entry-intro">
                            <?php the_content(); ?>
                        </div>
                    <?php endif; ?>
                </header>

                <div class="entry-content benchmarks-content">
                    
                    <?php if ( shortcode_exists( 'themisdb_benchmark_visualizer' ) ) : ?>
                        
                        <!-- Main Benchmark Visualizer -->
                        <div class="benchmark-section">
                            <h2>📊 Performance Overview</h2>
                            <p>Comprehensive view of all ThemisDB performance benchmarks across different categories.</p>
                            <?php echo do_shortcode( '[themisdb_benchmark_visualizer category="all" metric="latency"]' ); ?>
                        </div>

                        <!-- Category Tabs -->
                        <div class="benchmark-categories">
                            <h2>🎯 Explore by Category</h2>
                            
                            <div class="category-grid">
                                
                                <div class="category-card">
                                    <div class="category-icon">🔍</div>
                                    <h3>Vector Search</h3>
                                    <p>Embeddings and similarity search performance</p>
                                    <a href="#vector-search" class="category-link">View Benchmarks →</a>
                                </div>

                                <div class="category-card">
                                    <div class="category-icon">🕸️</div>
                                    <h3>Graph Traversal</h3>
                                    <p>BFS, DFS, and PageRank algorithms</p>
                                    <a href="#graph-traversal" class="category-link">View Benchmarks →</a>
                                </div>

                                <div class="category-card">
                                    <div class="category-icon">🔒</div>
                                    <h3>Encryption & Security</h3>
                                    <p>Cryptographic operations and HSM</p>
                                    <a href="#encryption" class="category-link">View Benchmarks →</a>
                                </div>

                                <div class="category-card">
                                    <div class="category-icon">⚡</div>
                                    <h3>GPU Acceleration</h3>
                                    <p>Hardware-accelerated operations</p>
                                    <a href="#gpu" class="category-link">View Benchmarks →</a>
                                </div>

                                <div class="category-card">
                                    <div class="category-icon">💼</div>
                                    <h3>Transactions</h3>
                                    <p>MVCC and ACID guarantees</p>
                                    <a href="#transaction" class="category-link">View Benchmarks →</a>
                                </div>

                                <div class="category-card">
                                    <div class="category-icon">🖼️</div>
                                    <h3>Image Analysis</h3>
                                    <p>AI-powered image processing</p>
                                    <a href="#image" class="category-link">View Benchmarks →</a>
                                </div>

                            </div>
                        </div>

                        <!-- Individual Category Sections -->
                        <div id="vector-search" class="benchmark-section">
                            <h2>🔍 Vector Search & Embeddings</h2>
                            <?php echo do_shortcode( '[themisdb_benchmark_visualizer category="vector_search" metric="latency" chart_type="bar"]' ); ?>
                        </div>

                        <div id="graph-traversal" class="benchmark-section">
                            <h2>🕸️ Graph Traversal & PageRank</h2>
                            <?php echo do_shortcode( '[themisdb_benchmark_visualizer category="graph_traversal" metric="latency" chart_type="line"]' ); ?>
                        </div>

                        <div id="encryption" class="benchmark-section">
                            <h2>🔒 Encryption & HSM</h2>
                            <?php echo do_shortcode( '[themisdb_benchmark_visualizer category="encryption" metric="latency" chart_type="bar"]' ); ?>
                        </div>

                        <div id="gpu" class="benchmark-section">
                            <h2>⚡ GPU Backends</h2>
                            <?php echo do_shortcode( '[themisdb_benchmark_visualizer category="gpu" metric="latency" chart_type="radar"]' ); ?>
                        </div>

                        <div id="transaction" class="benchmark-section">
                            <h2>💼 MVCC & Transactions</h2>
                            <?php echo do_shortcode( '[themisdb_benchmark_visualizer category="transaction" metric="latency" chart_type="bar"]' ); ?>
                        </div>

                        <div id="image" class="benchmark-section">
                            <h2>🖼️ Image Analysis</h2>
                            <?php echo do_shortcode( '[themisdb_benchmark_visualizer category="image_analysis" metric="latency" chart_type="line"]' ); ?>
                        </div>

                        <!-- Additional Info -->
                        <div class="benchmark-info">
                            <h2>ℹ️ About These Benchmarks</h2>
                            <div class="info-grid">
                                <div class="info-card">
                                    <h3>📋 Methodology</h3>
                                    <p>Benchmarks are performed using Google Benchmark framework with multiple iterations for accuracy.</p>
                                </div>
                                <div class="info-card">
                                    <h3>🖥️ Hardware</h3>
                                    <p>Tests run on 20-core CPU @ 3.7GHz with 20MB L3 cache for consistent results.</p>
                                </div>
                                <div class="info-card">
                                    <h3>📊 Metrics</h3>
                                    <p>Latency (ms), throughput (ops/sec), and memory usage measured across 19 benchmark suites.</p>
                                </div>
                                <div class="info-card">
                                    <h3>🔄 Updates</h3>
                                    <p>Benchmark data is updated regularly. Results cached for 24 hours for optimal performance.</p>
                                </div>
                            </div>
                        </div>

                    <?php else : ?>
                        
                        <div class="plugin-notice">
                            <p>⚠️ The ThemisDB Benchmark Visualizer plugin is not active. Please activate it to view benchmark data.</p>
                            <?php if ( current_user_can( 'activate_plugins' ) ) : ?>
                                <p><a href="<?php echo admin_url( 'plugins.php' ); ?>" class="button">Go to Plugins</a></p>
                            <?php endif; ?>
                        </div>

                    <?php endif; ?>

                </div>

            </article>

        <?php
        endwhile;
        ?>

    </main>
</div>

<style>
/* Benchmark Dashboard Styles */
.benchmarks-dashboard {
    max-width: 1400px;
    margin: 0 auto;
}

.benchmark-section {
    margin: 60px 0;
    padding: 40px;
    background: white;
    border-radius: 8px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.1);
}

.benchmark-section h2 {
    margin-bottom: 20px;
    color: var(--primary-color, #2c3e50);
    font-size: 2rem;
}

.category-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 24px;
    margin: 30px 0;
}

.category-card {
    background: white;
    padding: 30px;
    border-radius: 8px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    text-align: center;
    transition: transform 0.3s, box-shadow 0.3s;
}

.category-card:hover {
    transform: translateY(-4px);
    box-shadow: 0 4px 16px rgba(0,0,0,0.15);
}

.category-icon {
    font-size: 3rem;
    margin-bottom: 16px;
}

.category-card h3 {
    margin: 16px 0 12px;
    color: var(--primary-color, #2c3e50);
}

.category-card p {
    color: #666;
    margin-bottom: 16px;
}

.category-link {
    display: inline-block;
    padding: 8px 20px;
    background: var(--primary-color, #2c3e50);
    color: white;
    text-decoration: none;
    border-radius: 4px;
    transition: background 0.3s;
}

.category-link:hover {
    background: var(--secondary-color, #3498db);
}

.info-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
    gap: 20px;
    margin-top: 24px;
}

.info-card {
    background: #f8f9fa;
    padding: 24px;
    border-radius: 8px;
    border-left: 4px solid var(--primary-color, #2c3e50);
}

.info-card h3 {
    margin: 0 0 12px 0;
    color: var(--primary-color, #2c3e50);
}

.info-card p {
    margin: 0;
    color: #666;
}

.plugin-notice {
    background: #fff3cd;
    border: 1px solid #ffc107;
    padding: 20px;
    border-radius: 8px;
    text-align: center;
}

.entry-intro {
    margin-bottom: 40px;
    padding: 20px;
    background: #f8f9fa;
    border-radius: 8px;
}

/* Smooth scrolling for anchor links */
html {
    scroll-behavior: smooth;
}

/* Responsive */
@media (max-width: 768px) {
    .benchmark-section {
        padding: 20px;
        margin: 30px 0;
    }
    
    .category-grid {
        grid-template-columns: 1fr;
    }
    
    .info-grid {
        grid-template-columns: 1fr;
    }
}
</style>

<?php
get_footer();
