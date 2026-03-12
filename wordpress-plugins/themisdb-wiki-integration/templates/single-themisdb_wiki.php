<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            single-themisdb_wiki.php                           ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     187                                            ║
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
 * Single Wiki Page Template
 * Template for displaying individual wiki pages
 */

get_header(); ?>

<div class="wiki-container">
    <aside class="wiki-sidebar">
        <div class="wiki-navigation">
            <h3><?php _e('Wiki Navigation', 'themisdb-wiki'); ?></h3>
            <?php
            // Display wiki menu if it exists
            if (has_nav_menu('wiki-menu')) {
                wp_nav_menu(array(
                    'theme_location' => 'wiki-menu',
                    'container' => 'nav',
                    'container_class' => 'wiki-nav'
                ));
            } else {
                // Show recent wiki pages
                $recent_pages = new WP_Query(array(
                    'post_type' => 'themisdb_wiki',
                    'posts_per_page' => 10,
                    'orderby' => 'title',
                    'order' => 'ASC'
                ));
                
                if ($recent_pages->have_posts()) {
                    echo '<ul class="wiki-nav-list">';
                    while ($recent_pages->have_posts()) {
                        $recent_pages->the_post();
                        echo '<li><a href="' . esc_url(get_permalink()) . '">' . esc_html(get_the_title()) . '</a></li>';
                    }
                    echo '</ul>';
                    wp_reset_postdata();
                }
            }
            ?>
        </div>
        
        <div class="wiki-search-sidebar">
            <?php 
            $search = new ThemisDB_Wiki_Search();
            echo $search->get_search_form();
            ?>
        </div>
    </aside>
    
    <main class="wiki-content">
        <?php while (have_posts()) : the_post(); ?>
        
        <article id="post-<?php the_ID(); ?>" <?php post_class('wiki-page'); ?>>
            <header class="wiki-header">
                <h1><?php the_title(); ?></h1>
                <div class="wiki-meta">
                    <span class="wiki-author">
                        <?php _e('By', 'themisdb-wiki'); ?> <?php the_author(); ?>
                    </span>
                    <span class="wiki-date">
                        <?php _e('Updated', 'themisdb-wiki'); ?> <?php echo get_the_modified_date(); ?>
                    </span>
                    <?php if (current_user_can('edit_post', get_the_ID())) : ?>
                    <a href="<?php echo get_edit_post_link(); ?>" class="wiki-edit">
                        ✏️ <?php _e('Edit', 'themisdb-wiki'); ?>
                    </a>
                    <?php endif; ?>
                </div>
                
                <?php
                // Show categories
                $categories = get_the_terms(get_the_ID(), 'wiki_category');
                if ($categories && !is_wp_error($categories)) :
                ?>
                <div class="wiki-categories">
                    <?php foreach ($categories as $category) : ?>
                    <a href="<?php echo esc_url(get_term_link($category)); ?>" class="wiki-category-badge">
                        <?php echo esc_html($category->name); ?>
                    </a>
                    <?php endforeach; ?>
                </div>
                <?php endif; ?>
            </header>
            
            <div class="wiki-body">
                <?php the_content(); ?>
            </div>
            
            <footer class="wiki-footer">
                <?php
                // Contributors
                $contributors = get_post_meta(get_the_ID(), '_wiki_contributors', true);
                if ($contributors && is_array($contributors) && count($contributors) > 0) :
                ?>
                <div class="wiki-contributors">
                    <h4><?php _e('Contributors', 'themisdb-wiki'); ?></h4>
                    <div class="contributor-list">
                    <?php foreach ($contributors as $user_id) : 
                        $user = get_userdata($user_id);
                        if ($user) :
                    ?>
                        <a href="<?php echo get_author_posts_url($user_id); ?>" title="<?php echo esc_attr($user->display_name); ?>">
                            <?php echo get_avatar($user_id, 32); ?>
                        </a>
                    <?php 
                        endif;
                    endforeach; 
                    ?>
                    </div>
                </div>
                <?php endif; ?>
                
                <?php
                // Related pages
                $search = new ThemisDB_Wiki_Search();
                $related = $search->get_related_pages(get_the_ID(), 5);
                if (!empty($related)) :
                ?>
                <div class="wiki-related">
                    <h4><?php _e('Related Pages', 'themisdb-wiki'); ?></h4>
                    <ul>
                    <?php foreach ($related as $page) : ?>
                        <li><a href="<?php echo esc_url($page['url']); ?>"><?php echo esc_html($page['title']); ?></a></li>
                    <?php endforeach; ?>
                    </ul>
                </div>
                <?php endif; ?>
                
                <?php
                // Backlinks
                $wikilinks = new ThemisDB_WikiLinks();
                $backlinks = $wikilinks->get_backlinks(get_post_field('post_name', get_the_ID()));
                if (!empty($backlinks)) :
                ?>
                <div class="wiki-backlinks">
                    <h4><?php _e('Pages linking here', 'themisdb-wiki'); ?></h4>
                    <ul>
                    <?php foreach ($backlinks as $page) : ?>
                        <li><a href="<?php echo get_permalink($page->ID); ?>"><?php echo esc_html($page->post_title); ?></a></li>
                    <?php endforeach; ?>
                    </ul>
                </div>
                <?php endif; ?>
            </footer>
            
            <?php
            // Comments if enabled
            if (comments_open() || get_comments_number()) :
                comments_template();
            endif;
            ?>
        </article>
        
        <?php endwhile; ?>
    </main>
</div>

<?php
// Track page view
$search = new ThemisDB_Wiki_Search();
$search->track_page_view(get_the_ID());
?>

<?php get_footer(); ?>
