<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            archive-themisdb_wiki.php                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */


/**
 * Archive Wiki Pages Template
 * Template for displaying wiki page archives
 */

get_header(); ?>

<div class="wiki-container">
    <aside class="wiki-sidebar">
        <div class="wiki-search-sidebar">
            <?php 
            $search = new ThemisDB_Wiki_Search();
            echo $search->get_search_form();
            ?>
        </div>
        
        <div class="wiki-categories">
            <h3><?php _e('Categories', 'themisdb-wiki'); ?></h3>
            <?php
            $categories = get_terms(array(
                'taxonomy' => 'wiki_category',
                'hide_empty' => true
            ));
            
            if (!empty($categories) && !is_wp_error($categories)) {
                echo '<ul>';
                foreach ($categories as $category) {
                    echo '<li><a href="' . esc_url(get_term_link($category)) . '">' . esc_html($category->name) . ' (' . absint($category->count) . ')</a></li>';
                }
                echo '</ul>';
            }
            ?>
        </div>
    </aside>
    
    <main class="wiki-content">
        <header class="wiki-archive-header">
            <h1>
                <?php
                if (is_tax('wiki_category')) {
                    single_term_title();
                } elseif (is_search()) {
                    printf(__('Search Results for: %s', 'themisdb-wiki'), get_search_query());
                } else {
                    _e('Wiki Pages', 'themisdb-wiki');
                }
                ?>
            </h1>
            
            <?php if (is_tax('wiki_category')) : 
                $term_description = term_description();
                if (!empty($term_description)) :
            ?>
            <div class="wiki-category-description">
                <?php echo $term_description; ?>
            </div>
            <?php 
                endif;
            endif; 
            ?>
        </header>
        
        <?php if (have_posts()) : ?>
        
        <div class="wiki-page-list">
            <?php while (have_posts()) : the_post(); ?>
            
            <article id="post-<?php the_ID(); ?>" <?php post_class('wiki-page-item'); ?>>
                <h2 class="wiki-page-title">
                    <a href="<?php the_permalink(); ?>"><?php the_title(); ?></a>
                </h2>
                
                <div class="wiki-page-meta">
                    <span class="wiki-author">
                        <?php _e('By', 'themisdb-wiki'); ?> <?php the_author(); ?>
                    </span>
                    <span class="wiki-date">
                        <?php _e('Updated', 'themisdb-wiki'); ?> <?php echo get_the_modified_date(); ?>
                    </span>
                </div>
                
                <?php if (has_excerpt()) : ?>
                <div class="wiki-page-excerpt">
                    <?php the_excerpt(); ?>
                </div>
                <?php endif; ?>
                
                <?php
                // Show categories
                $categories = get_the_terms(get_the_ID(), 'wiki_category');
                if ($categories && !is_wp_error($categories)) :
                ?>
                <div class="wiki-page-categories">
                    <?php foreach ($categories as $category) : ?>
                    <a href="<?php echo esc_url(get_term_link($category)); ?>" class="wiki-category-badge">
                        <?php echo esc_html($category->name); ?>
                    </a>
                    <?php endforeach; ?>
                </div>
                <?php endif; ?>
            </article>
            
            <?php endwhile; ?>
        </div>
        
        <?php
        // Pagination
        the_posts_pagination(array(
            'mid_size' => 2,
            'prev_text' => __('← Previous', 'themisdb-wiki'),
            'next_text' => __('Next →', 'themisdb-wiki'),
        ));
        ?>
        
        <?php else : ?>
        
        <div class="wiki-no-results">
            <p><?php _e('No wiki pages found.', 'themisdb-wiki'); ?></p>
            
            <?php if (current_user_can('publish_posts')) : ?>
            <p>
                <a href="<?php echo admin_url('post-new.php?post_type=themisdb_wiki'); ?>" class="button">
                    <?php _e('Create First Wiki Page', 'themisdb-wiki'); ?>
                </a>
            </p>
            <?php endif; ?>
        </div>
        
        <?php endif; ?>
    </main>
</div>

<?php get_footer(); ?>
