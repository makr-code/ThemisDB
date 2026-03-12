<?php
/**
 * Custom Widgets for ThemisDB Theme
 *
 * @package ThemisDB
 * @since 1.0.0
 */

if ( ! defined( 'ABSPATH' ) ) {
    exit; // Exit if accessed directly
}

/**
 * Featured Posts Slider Widget
 * Displays featured/sticky posts in a slider format
 */
class ThemisDB_Featured_Slider_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_featured_slider',
            esc_html__( 'ThemisDB: Featured Slider', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display featured posts in a slider to highlight articles', 'themisdb' ),
                'classname' => 'themisdb-featured-slider-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $title = apply_filters( 'widget_title', $title, $instance, $this->id_base );
        $count = ! empty( $instance['count'] ) ? absint( $instance['count'] ) : 3;

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        // Query for featured posts (sticky posts or posts with a meta key)
        $query_args = array(
            'posts_per_page' => $count,
            'post__in'       => get_option( 'sticky_posts' ),
            'ignore_sticky_posts' => 1,
        );

        // If no sticky posts, get recent posts
        if ( empty( get_option( 'sticky_posts' ) ) ) {
            $query_args = array(
                'posts_per_page' => $count,
                'orderby'        => 'date',
                'order'          => 'DESC',
            );
        }

        $featured_query = new WP_Query( $query_args );

        if ( $featured_query->have_posts() ) :
            ?>
            <div class="themisdb-slider-container">
                <div class="themisdb-slider">
                    <?php while ( $featured_query->have_posts() ) : $featured_query->the_post(); ?>
                        <div class="slider-item">
                            <?php if ( has_post_thumbnail() ) : ?>
                                <div class="slider-image">
                                    <a href="<?php the_permalink(); ?>">
                                        <?php the_post_thumbnail( 'themisdb-featured' ); ?>
                                    </a>
                                </div>
                            <?php endif; ?>
                            <div class="slider-content">
                                <h3 class="slider-title">
                                    <a href="<?php the_permalink(); ?>"><?php the_title(); ?></a>
                                </h3>
                                <div class="slider-meta">
                                    <span class="slider-date"><?php echo esc_html( get_the_date() ); ?></span>
                                </div>
                                <div class="slider-excerpt">
                                    <?php echo wp_trim_words( get_the_excerpt(), 20 ); ?>
                                </div>
                                <a href="<?php the_permalink(); ?>" class="slider-readmore">
                                    <?php esc_html_e( 'Read More', 'themisdb' ); ?> →
                                </a>
                            </div>
                        </div>
                    <?php endwhile; ?>
                </div>
                <?php if ( $featured_query->post_count > 1 ) : ?>
                    <button class="slider-nav slider-prev" aria-label="<?php esc_attr_e( 'Previous slide', 'themisdb' ); ?>">‹</button>
                    <button class="slider-nav slider-next" aria-label="<?php esc_attr_e( 'Next slide', 'themisdb' ); ?>">›</button>
                    <div class="slider-dots"></div>
                <?php endif; ?>
            </div>
            <?php
            wp_reset_postdata();
        endif;

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'Featured Posts', 'themisdb' );
        $count = ! empty( $instance['count'] ) ? absint( $instance['count'] ) : 3;
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'count' ) ); ?>">
                <?php esc_html_e( 'Number of posts:', 'themisdb' ); ?>
            </label>
            <input class="tiny-text" id="<?php echo esc_attr( $this->get_field_id( 'count' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'count' ) ); ?>" type="number" 
                   step="1" min="1" value="<?php echo esc_attr( $count ); ?>" size="3">
        </p>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        $instance['count'] = ( ! empty( $new_instance['count'] ) ) ? absint( $new_instance['count'] ) : 3;
        return $instance;
    }
}

/**
 * Recent Posts with Thumbnails Widget
 * Enhanced recent posts display with featured images
 */
class ThemisDB_Recent_Posts_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_recent_posts',
            esc_html__( 'ThemisDB: Recent Posts', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display recent posts with thumbnails', 'themisdb' ),
                'classname' => 'themisdb-recent-posts-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'Recent Posts', 'themisdb' );
        $title = apply_filters( 'widget_title', $title, $instance, $this->id_base );
        $count = ! empty( $instance['count'] ) ? absint( $instance['count'] ) : 5;
        $show_thumbnails = ! empty( $instance['show_thumbnails'] );

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        $recent_posts = new WP_Query( array(
            'posts_per_page'      => $count,
            'post_status'         => 'publish',
            'ignore_sticky_posts' => true,
        ) );

        if ( $recent_posts->have_posts() ) :
            ?>
            <ul class="themisdb-recent-posts">
                <?php while ( $recent_posts->have_posts() ) : $recent_posts->the_post(); ?>
                    <li class="recent-post-item">
                        <?php if ( $show_thumbnails && has_post_thumbnail() ) : ?>
                            <div class="recent-post-thumbnail">
                                <a href="<?php the_permalink(); ?>">
                                    <?php the_post_thumbnail( 'themisdb-thumbnail' ); ?>
                                </a>
                            </div>
                        <?php endif; ?>
                        <div class="recent-post-content">
                            <h4 class="recent-post-title">
                                <a href="<?php the_permalink(); ?>"><?php the_title(); ?></a>
                            </h4>
                            <span class="recent-post-date"><?php echo esc_html( get_the_date() ); ?></span>
                        </div>
                    </li>
                <?php endwhile; ?>
            </ul>
            <?php
            wp_reset_postdata();
        endif;

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'Recent Posts', 'themisdb' );
        $count = ! empty( $instance['count'] ) ? absint( $instance['count'] ) : 5;
        $show_thumbnails = ! empty( $instance['show_thumbnails'] );
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'count' ) ); ?>">
                <?php esc_html_e( 'Number of posts:', 'themisdb' ); ?>
            </label>
            <input class="tiny-text" id="<?php echo esc_attr( $this->get_field_id( 'count' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'count' ) ); ?>" type="number" 
                   step="1" min="1" value="<?php echo esc_attr( $count ); ?>" size="3">
        </p>
        <p>
            <input class="checkbox" type="checkbox" <?php checked( $show_thumbnails ); ?> 
                   id="<?php echo esc_attr( $this->get_field_id( 'show_thumbnails' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'show_thumbnails' ) ); ?>" />
            <label for="<?php echo esc_attr( $this->get_field_id( 'show_thumbnails' ) ); ?>">
                <?php esc_html_e( 'Display thumbnails', 'themisdb' ); ?>
            </label>
        </p>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        $instance['count'] = ( ! empty( $new_instance['count'] ) ) ? absint( $new_instance['count'] ) : 5;
        $instance['show_thumbnails'] = ( ! empty( $new_instance['show_thumbnails'] ) ) ? 1 : 0;
        return $instance;
    }
}

/**
 * Category Highlights Widget
 * Display posts from specific categories
 */
class ThemisDB_Category_Highlights_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_category_highlights',
            esc_html__( 'ThemisDB: Category Highlights', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Highlight posts from a specific category', 'themisdb' ),
                'classname' => 'themisdb-category-highlights-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $title = apply_filters( 'widget_title', $title, $instance, $this->id_base );
        $category = ! empty( $instance['category'] ) ? absint( $instance['category'] ) : 0;
        $count = ! empty( $instance['count'] ) ? absint( $instance['count'] ) : 3;

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        if ( $category ) {
            $category_posts = new WP_Query( array(
                'cat'            => $category,
                'posts_per_page' => $count,
                'post_status'    => 'publish',
            ) );

            if ( $category_posts->have_posts() ) :
                ?>
                <div class="themisdb-category-highlights">
                    <?php while ( $category_posts->have_posts() ) : $category_posts->the_post(); ?>
                        <article class="category-highlight-item">
                            <?php if ( has_post_thumbnail() ) : ?>
                                <div class="highlight-thumbnail">
                                    <a href="<?php the_permalink(); ?>">
                                        <?php the_post_thumbnail( 'themisdb-thumbnail' ); ?>
                                    </a>
                                </div>
                            <?php endif; ?>
                            <div class="highlight-content">
                                <h4 class="highlight-title">
                                    <a href="<?php the_permalink(); ?>"><?php the_title(); ?></a>
                                </h4>
                                <div class="highlight-excerpt">
                                    <?php echo wp_trim_words( get_the_excerpt(), 15 ); ?>
                                </div>
                            </div>
                        </article>
                    <?php endwhile; ?>
                </div>
                <?php
                wp_reset_postdata();
            endif;
        } else {
            echo '<p>' . esc_html__( 'Please select a category in widget settings.', 'themisdb' ) . '</p>';
        }

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $category = ! empty( $instance['category'] ) ? absint( $instance['category'] ) : 0;
        $count = ! empty( $instance['count'] ) ? absint( $instance['count'] ) : 3;
        
        $categories = get_categories( array( 'hide_empty' => false ) );
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'category' ) ); ?>">
                <?php esc_html_e( 'Category:', 'themisdb' ); ?>
            </label>
            <select class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'category' ) ); ?>" 
                    name="<?php echo esc_attr( $this->get_field_name( 'category' ) ); ?>">
                <option value="0"><?php esc_html_e( 'Select a category', 'themisdb' ); ?></option>
                <?php foreach ( $categories as $cat ) : ?>
                    <option value="<?php echo esc_attr( $cat->term_id ); ?>" <?php selected( $category, $cat->term_id ); ?>>
                        <?php echo esc_html( $cat->name ); ?>
                    </option>
                <?php endforeach; ?>
            </select>
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'count' ) ); ?>">
                <?php esc_html_e( 'Number of posts:', 'themisdb' ); ?>
            </label>
            <input class="tiny-text" id="<?php echo esc_attr( $this->get_field_id( 'count' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'count' ) ); ?>" type="number" 
                   step="1" min="1" value="<?php echo esc_attr( $count ); ?>" size="3">
        </p>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        $instance['category'] = ( ! empty( $new_instance['category'] ) ) ? absint( $new_instance['category'] ) : 0;
        $instance['count'] = ( ! empty( $new_instance['count'] ) ) ? absint( $new_instance['count'] ) : 3;
        return $instance;
    }
}

/**
 * Call-to-Action Widget
 * Highlight important content or links
 */
class ThemisDB_CTA_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_cta',
            esc_html__( 'ThemisDB: Call to Action', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display a highlighted call-to-action box', 'themisdb' ),
                'classname' => 'themisdb-cta-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $content = ! empty( $instance['content'] ) ? $instance['content'] : '';
        $button_text = ! empty( $instance['button_text'] ) ? $instance['button_text'] : '';
        $button_url = ! empty( $instance['button_url'] ) ? $instance['button_url'] : '';
        $style = ! empty( $instance['style'] ) ? $instance['style'] : 'primary';

        ?>
        <div class="themisdb-cta-box cta-style-<?php echo esc_attr( $style ); ?>">
            <?php if ( $title ) : ?>
                <h3 class="cta-title"><?php echo esc_html( $title ); ?></h3>
            <?php endif; ?>
            
            <?php if ( $content ) : ?>
                <div class="cta-content">
                    <?php echo wp_kses_post( wpautop( $content ) ); ?>
                </div>
            <?php endif; ?>
            
            <?php if ( $button_text && $button_url ) : ?>
                <a href="<?php echo esc_url( $button_url ); ?>" class="cta-button">
                    <?php echo esc_html( $button_text ); ?>
                </a>
            <?php endif; ?>
        </div>
        <?php

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $content = ! empty( $instance['content'] ) ? $instance['content'] : '';
        $button_text = ! empty( $instance['button_text'] ) ? $instance['button_text'] : '';
        $button_url = ! empty( $instance['button_url'] ) ? $instance['button_url'] : '';
        $style = ! empty( $instance['style'] ) ? $instance['style'] : 'primary';
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'content' ) ); ?>">
                <?php esc_html_e( 'Content:', 'themisdb' ); ?>
            </label>
            <textarea class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'content' ) ); ?>" 
                      name="<?php echo esc_attr( $this->get_field_name( 'content' ) ); ?>" rows="4"><?php echo esc_textarea( $content ); ?></textarea>
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'button_text' ) ); ?>">
                <?php esc_html_e( 'Button Text:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'button_text' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'button_text' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $button_text ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'button_url' ) ); ?>">
                <?php esc_html_e( 'Button URL:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'button_url' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'button_url' ) ); ?>" type="url" 
                   value="<?php echo esc_url( $button_url ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'style' ) ); ?>">
                <?php esc_html_e( 'Style:', 'themisdb' ); ?>
            </label>
            <select class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'style' ) ); ?>" 
                    name="<?php echo esc_attr( $this->get_field_name( 'style' ) ); ?>">
                <option value="primary" <?php selected( $style, 'primary' ); ?>><?php esc_html_e( 'Primary', 'themisdb' ); ?></option>
                <option value="secondary" <?php selected( $style, 'secondary' ); ?>><?php esc_html_e( 'Secondary', 'themisdb' ); ?></option>
                <option value="accent" <?php selected( $style, 'accent' ); ?>><?php esc_html_e( 'Accent', 'themisdb' ); ?></option>
                <option value="success" <?php selected( $style, 'success' ); ?>><?php esc_html_e( 'Success', 'themisdb' ); ?></option>
            </select>
        </p>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        $instance['content'] = ( ! empty( $new_instance['content'] ) ) ? wp_kses_post( $new_instance['content'] ) : '';
        $instance['button_text'] = ( ! empty( $new_instance['button_text'] ) ) ? sanitize_text_field( $new_instance['button_text'] ) : '';
        $instance['button_url'] = ( ! empty( $new_instance['button_url'] ) ) ? esc_url_raw( $new_instance['button_url'] ) : '';
        $instance['style'] = ( ! empty( $new_instance['style'] ) ) ? sanitize_text_field( $new_instance['style'] ) : 'primary';
        return $instance;
    }
}

/**
 * About/Info Box Widget
 * Display project information or about section
 */
class ThemisDB_About_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_about',
            esc_html__( 'ThemisDB: About Box', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display project information or about content', 'themisdb' ),
                'classname' => 'themisdb-about-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $content = ! empty( $instance['content'] ) ? $instance['content'] : '';
        $show_icon = ! empty( $instance['show_icon'] );

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        ?>
        <div class="themisdb-about-box">
            <?php if ( $show_icon ) : ?>
                <div class="about-icon">
                    <span class="icon-database">🗄️</span>
                </div>
            <?php endif; ?>
            <div class="about-content">
                <?php echo wp_kses_post( wpautop( $content ) ); ?>
            </div>
        </div>
        <?php

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'About ThemisDB', 'themisdb' );
        $content = ! empty( $instance['content'] ) ? $instance['content'] : '';
        $show_icon = ! empty( $instance['show_icon'] );
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'content' ) ); ?>">
                <?php esc_html_e( 'Content:', 'themisdb' ); ?>
            </label>
            <textarea class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'content' ) ); ?>" 
                      name="<?php echo esc_attr( $this->get_field_name( 'content' ) ); ?>" rows="6"><?php echo esc_textarea( $content ); ?></textarea>
        </p>
        <p>
            <input class="checkbox" type="checkbox" <?php checked( $show_icon ); ?> 
                   id="<?php echo esc_attr( $this->get_field_id( 'show_icon' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'show_icon' ) ); ?>" />
            <label for="<?php echo esc_attr( $this->get_field_id( 'show_icon' ) ); ?>">
                <?php esc_html_e( 'Show icon', 'themisdb' ); ?>
            </label>
        </p>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        $instance['content'] = ( ! empty( $new_instance['content'] ) ) ? wp_kses_post( $new_instance['content'] ) : '';
        $instance['show_icon'] = ( ! empty( $new_instance['show_icon'] ) ) ? 1 : 0;
        return $instance;
    }
}

/**
 * Social Media Links Widget
 * Display social media and community links
 */
class ThemisDB_Social_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_social',
            esc_html__( 'ThemisDB: Social Links', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display social media and community links', 'themisdb' ),
                'classname' => 'themisdb-social-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $github_url = ! empty( $instance['github_url'] ) ? $instance['github_url'] : '';
        $twitter_url = ! empty( $instance['twitter_url'] ) ? $instance['twitter_url'] : '';
        $discord_url = ! empty( $instance['discord_url'] ) ? $instance['discord_url'] : '';
        $linkedin_url = ! empty( $instance['linkedin_url'] ) ? $instance['linkedin_url'] : '';

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        $has_links = $github_url || $twitter_url || $discord_url || $linkedin_url;

        if ( $has_links ) :
            ?>
            <div class="themisdb-social-links">
                <?php if ( $github_url ) : ?>
                    <a href="<?php echo esc_url( $github_url ); ?>" class="social-link social-github" target="_blank" rel="noopener noreferrer" aria-label="GitHub">
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                            <path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z"/>
                        </svg>
                        <span>GitHub</span>
                    </a>
                <?php endif; ?>

                <?php if ( $twitter_url ) : ?>
                    <a href="<?php echo esc_url( $twitter_url ); ?>" class="social-link social-twitter" target="_blank" rel="noopener noreferrer" aria-label="Twitter">
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                            <path d="M23 3a10.9 10.9 0 01-3.14 1.53 4.48 4.48 0 00-7.86 3v1A10.66 10.66 0 013 4s-4 9 5 13a11.64 11.64 0 01-7 2c9 5 20 0 20-11.5a4.5 4.5 0 00-.08-.83A7.72 7.72 0 0023 3z"/>
                        </svg>
                        <span>Twitter</span>
                    </a>
                <?php endif; ?>

                <?php if ( $discord_url ) : ?>
                    <a href="<?php echo esc_url( $discord_url ); ?>" class="social-link social-discord" target="_blank" rel="noopener noreferrer" aria-label="Discord">
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                            <path d="M20.317 4.37a19.791 19.791 0 00-4.885-1.515.074.074 0 00-.079.037c-.21.375-.444.864-.608 1.25a18.27 18.27 0 00-5.487 0 12.64 12.64 0 00-.617-1.25.077.077 0 00-.079-.037A19.736 19.736 0 003.677 4.37a.07.07 0 00-.032.027C.533 9.046-.32 13.58.099 18.057a.082.082 0 00.031.057 19.9 19.9 0 005.993 3.03.078.078 0 00.084-.028c.462-.63.874-1.295 1.226-1.994a.076.076 0 00-.041-.106 13.107 13.107 0 01-1.872-.892.077.077 0 01-.008-.128 10.2 10.2 0 00.372-.292.074.074 0 01.077-.01c3.928 1.793 8.18 1.793 12.062 0a.074.074 0 01.078.01c.12.098.246.198.373.292a.077.077 0 01-.006.127 12.299 12.299 0 01-1.873.892.077.077 0 00-.041.107c.36.698.772 1.362 1.225 1.993a.076.076 0 00.084.028 19.839 19.839 0 006.002-3.03.077.077 0 00.032-.054c.5-5.177-.838-9.674-3.549-13.66a.061.061 0 00-.031-.03zM8.02 15.33c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.956-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.956 2.418-2.157 2.418zm7.975 0c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.955-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.946 2.418-2.157 2.418z"/>
                        </svg>
                        <span>Discord</span>
                    </a>
                <?php endif; ?>

                <?php if ( $linkedin_url ) : ?>
                    <a href="<?php echo esc_url( $linkedin_url ); ?>" class="social-link social-linkedin" target="_blank" rel="noopener noreferrer" aria-label="LinkedIn">
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                            <path d="M20.447 20.452h-3.554v-5.569c0-1.328-.027-3.037-1.852-3.037-1.853 0-2.136 1.445-2.136 2.939v5.667H9.351V9h3.414v1.561h.046c.477-.9 1.637-1.85 3.37-1.85 3.601 0 4.267 2.37 4.267 5.455v6.286zM5.337 7.433c-1.144 0-2.063-.926-2.063-2.065 0-1.138.92-2.063 2.063-2.063 1.14 0 2.064.925 2.064 2.063 0 1.139-.925 2.065-2.064 2.065zm1.782 13.019H3.555V9h3.564v11.452zM22.225 0H1.771C.792 0 0 .774 0 1.729v20.542C0 23.227.792 24 1.771 24h20.451C23.2 24 24 23.227 24 22.271V1.729C24 .774 23.2 0 22.222 0h.003z"/>
                        </svg>
                        <span>LinkedIn</span>
                    </a>
                <?php endif; ?>
            </div>
            <?php
        else :
            echo '<p>' . esc_html__( 'Please configure social media URLs in widget settings.', 'themisdb' ) . '</p>';
        endif;

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'Follow Us', 'themisdb' );
        $github_url = ! empty( $instance['github_url'] ) ? $instance['github_url'] : '';
        $twitter_url = ! empty( $instance['twitter_url'] ) ? $instance['twitter_url'] : '';
        $discord_url = ! empty( $instance['discord_url'] ) ? $instance['discord_url'] : '';
        $linkedin_url = ! empty( $instance['linkedin_url'] ) ? $instance['linkedin_url'] : '';
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'github_url' ) ); ?>">
                <?php esc_html_e( 'GitHub URL:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'github_url' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'github_url' ) ); ?>" type="url" 
                   value="<?php echo esc_url( $github_url ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'twitter_url' ) ); ?>">
                <?php esc_html_e( 'Twitter URL:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'twitter_url' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'twitter_url' ) ); ?>" type="url" 
                   value="<?php echo esc_url( $twitter_url ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'discord_url' ) ); ?>">
                <?php esc_html_e( 'Discord URL:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'discord_url' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'discord_url' ) ); ?>" type="url" 
                   value="<?php echo esc_url( $discord_url ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'linkedin_url' ) ); ?>">
                <?php esc_html_e( 'LinkedIn URL:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'linkedin_url' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'linkedin_url' ) ); ?>" type="url" 
                   value="<?php echo esc_url( $linkedin_url ); ?>">
        </p>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        $instance['github_url'] = ( ! empty( $new_instance['github_url'] ) ) ? esc_url_raw( $new_instance['github_url'] ) : '';
        $instance['twitter_url'] = ( ! empty( $new_instance['twitter_url'] ) ) ? esc_url_raw( $new_instance['twitter_url'] ) : '';
        $instance['discord_url'] = ( ! empty( $new_instance['discord_url'] ) ) ? esc_url_raw( $new_instance['discord_url'] ) : '';
        $instance['linkedin_url'] = ( ! empty( $new_instance['linkedin_url'] ) ) ? esc_url_raw( $new_instance['linkedin_url'] ) : '';
        return $instance;
    }
}

/**
 * Enhanced Tag Cloud Widget
 * Display tags with custom styling and counts
 */
class ThemisDB_Tag_Cloud_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_tag_cloud',
            esc_html__( 'ThemisDB: Tag Cloud', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display a cloud of your most used tags', 'themisdb' ),
                'classname' => 'themisdb-tag-cloud-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'Tags', 'themisdb' );
        $count = ! empty( $instance['count'] ) ? absint( $instance['count'] ) : 20;
        $show_count = ! empty( $instance['show_count'] );

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        $tags = get_tags( array(
            'orderby' => 'count',
            'order'   => 'DESC',
            'number'  => $count,
        ) );

        if ( $tags ) :
            ?>
            <div class="themisdb-tag-cloud">
                <?php foreach ( $tags as $tag ) : ?>
                    <a href="<?php echo esc_url( get_tag_link( $tag->term_id ) ); ?>" 
                       class="tag-item" 
                       title="<?php echo esc_attr( sprintf( _n( '%s topic', '%s topics', $tag->count, 'themisdb' ), $tag->count ) ); ?>">
                        <?php echo esc_html( $tag->name ); ?>
                        <?php if ( $show_count ) : ?>
                            <span class="tag-count">(<?php echo esc_html( $tag->count ); ?>)</span>
                        <?php endif; ?>
                    </a>
                <?php endforeach; ?>
            </div>
            <?php
        else :
            echo '<p>' . esc_html__( 'No tags found.', 'themisdb' ) . '</p>';
        endif;

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'Tags', 'themisdb' );
        $count = ! empty( $instance['count'] ) ? absint( $instance['count'] ) : 20;
        $show_count = ! empty( $instance['show_count'] );
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'count' ) ); ?>">
                <?php esc_html_e( 'Number of tags:', 'themisdb' ); ?>
            </label>
            <input class="tiny-text" id="<?php echo esc_attr( $this->get_field_id( 'count' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'count' ) ); ?>" type="number" 
                   step="1" min="1" max="50" value="<?php echo esc_attr( $count ); ?>" size="3">
        </p>
        <p>
            <input class="checkbox" type="checkbox" <?php checked( $show_count ); ?> 
                   id="<?php echo esc_attr( $this->get_field_id( 'show_count' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'show_count' ) ); ?>" />
            <label for="<?php echo esc_attr( $this->get_field_id( 'show_count' ) ); ?>">
                <?php esc_html_e( 'Show post count', 'themisdb' ); ?>
            </label>
        </p>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        $instance['count'] = ( ! empty( $new_instance['count'] ) ) ? absint( $new_instance['count'] ) : 20;
        $instance['show_count'] = ( ! empty( $new_instance['show_count'] ) ) ? 1 : 0;
        return $instance;
    }
}

/**
 * Documentation/Quick Links Widget
 * Display important documentation links
 */
class ThemisDB_Quick_Links_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_quick_links',
            esc_html__( 'ThemisDB: Quick Links', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display important documentation or quick access links', 'themisdb' ),
                'classname' => 'themisdb-quick-links-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $links = array();
        
        for ( $i = 1; $i <= 5; $i++ ) {
            $link_title = ! empty( $instance['link_title_' . $i] ) ? $instance['link_title_' . $i] : '';
            $link_url = ! empty( $instance['link_url_' . $i] ) ? $instance['link_url_' . $i] : '';
            
            if ( $link_title && $link_url ) {
                $links[] = array(
                    'title' => $link_title,
                    'url'   => $link_url,
                );
            }
        }

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        if ( ! empty( $links ) ) :
            ?>
            <ul class="themisdb-quick-links">
                <?php foreach ( $links as $link ) : ?>
                    <li class="quick-link-item">
                        <a href="<?php echo esc_url( $link['url'] ); ?>" class="quick-link">
                            <span class="link-icon">📄</span>
                            <span class="link-text"><?php echo esc_html( $link['title'] ); ?></span>
                            <span class="link-arrow">→</span>
                        </a>
                    </li>
                <?php endforeach; ?>
            </ul>
            <?php
        else :
            echo '<p>' . esc_html__( 'Please configure links in widget settings.', 'themisdb' ) . '</p>';
        endif;

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'Quick Links', 'themisdb' );
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <?php for ( $i = 1; $i <= 5; $i++ ) : 
            $link_title = ! empty( $instance['link_title_' . $i] ) ? $instance['link_title_' . $i] : '';
            $link_url = ! empty( $instance['link_url_' . $i] ) ? $instance['link_url_' . $i] : '';
        ?>
            <p><strong><?php echo sprintf( esc_html__( 'Link %d', 'themisdb' ), $i ); ?></strong></p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'link_title_' . $i ) ); ?>">
                    <?php esc_html_e( 'Link Text:', 'themisdb' ); ?>
                </label>
                <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'link_title_' . $i ) ); ?>" 
                       name="<?php echo esc_attr( $this->get_field_name( 'link_title_' . $i ) ); ?>" type="text" 
                       value="<?php echo esc_attr( $link_title ); ?>">
            </p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'link_url_' . $i ) ); ?>">
                    <?php esc_html_e( 'Link URL:', 'themisdb' ); ?>
                </label>
                <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'link_url_' . $i ) ); ?>" 
                       name="<?php echo esc_attr( $this->get_field_name( 'link_url_' . $i ) ); ?>" type="url" 
                       value="<?php echo esc_url( $link_url ); ?>">
            </p>
        <?php endfor; ?>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        
        for ( $i = 1; $i <= 5; $i++ ) {
            $instance['link_title_' . $i] = ( ! empty( $new_instance['link_title_' . $i] ) ) ? sanitize_text_field( $new_instance['link_title_' . $i] ) : '';
            $instance['link_url_' . $i] = ( ! empty( $new_instance['link_url_' . $i] ) ) ? esc_url_raw( $new_instance['link_url_' . $i] ) : '';
        }
        
        return $instance;
    }
}

/**
 * Testimonials/Quotes Carousel Widget
 * Rotating testimonials or quotes display
 */
class ThemisDB_Testimonials_Carousel_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_testimonials_carousel',
            esc_html__( 'ThemisDB: Testimonials Carousel', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display rotating testimonials or quotes in a carousel', 'themisdb' ),
                'classname' => 'themisdb-testimonials-carousel-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $title = apply_filters( 'widget_title', $title, $instance, $this->id_base );

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        $testimonials = array();
        for ( $i = 1; $i <= 3; $i++ ) {
            $quote = ! empty( $instance['quote_' . $i] ) ? $instance['quote_' . $i] : '';
            $author = ! empty( $instance['author_' . $i] ) ? $instance['author_' . $i] : '';
            $role = ! empty( $instance['role_' . $i] ) ? $instance['role_' . $i] : '';
            
            if ( $quote && $author ) {
                $testimonials[] = array(
                    'quote'  => $quote,
                    'author' => $author,
                    'role'   => $role,
                );
            }
        }

        if ( ! empty( $testimonials ) ) :
            ?>
            <div class="themisdb-testimonials-container">
                <div class="themisdb-testimonials-carousel">
                    <?php foreach ( $testimonials as $index => $testimonial ) : ?>
                        <div class="testimonial-item <?php echo $index === 0 ? 'active' : ''; ?>">
                            <div class="testimonial-quote">
                                <span class="quote-icon">❝</span>
                                <p><?php echo esc_html( $testimonial['quote'] ); ?></p>
                            </div>
                            <div class="testimonial-author">
                                <strong><?php echo esc_html( $testimonial['author'] ); ?></strong>
                                <?php if ( $testimonial['role'] ) : ?>
                                    <span class="author-role"><?php echo esc_html( $testimonial['role'] ); ?></span>
                                <?php endif; ?>
                            </div>
                        </div>
                    <?php endforeach; ?>
                </div>
                <?php if ( count( $testimonials ) > 1 ) : ?>
                    <div class="testimonial-nav">
                        <button class="testimonial-prev" aria-label="<?php esc_attr_e( 'Previous testimonial', 'themisdb' ); ?>">‹</button>
                        <button class="testimonial-next" aria-label="<?php esc_attr_e( 'Next testimonial', 'themisdb' ); ?>">›</button>
                    </div>
                    <div class="testimonial-dots"></div>
                <?php endif; ?>
            </div>
            <?php
        else :
            echo '<p>' . esc_html__( 'Please configure testimonials in widget settings.', 'themisdb' ) . '</p>';
        endif;

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'What People Say', 'themisdb' );
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <?php for ( $i = 1; $i <= 3; $i++ ) : 
            $quote = ! empty( $instance['quote_' . $i] ) ? $instance['quote_' . $i] : '';
            $author = ! empty( $instance['author_' . $i] ) ? $instance['author_' . $i] : '';
            $role = ! empty( $instance['role_' . $i] ) ? $instance['role_' . $i] : '';
        ?>
            <p><strong><?php echo sprintf( esc_html__( 'Testimonial %d', 'themisdb' ), $i ); ?></strong></p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'quote_' . $i ) ); ?>">
                    <?php esc_html_e( 'Quote:', 'themisdb' ); ?>
                </label>
                <textarea class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'quote_' . $i ) ); ?>" 
                          name="<?php echo esc_attr( $this->get_field_name( 'quote_' . $i ) ); ?>" rows="3"><?php echo esc_textarea( $quote ); ?></textarea>
            </p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'author_' . $i ) ); ?>">
                    <?php esc_html_e( 'Author:', 'themisdb' ); ?>
                </label>
                <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'author_' . $i ) ); ?>" 
                       name="<?php echo esc_attr( $this->get_field_name( 'author_' . $i ) ); ?>" type="text" 
                       value="<?php echo esc_attr( $author ); ?>">
            </p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'role_' . $i ) ); ?>">
                    <?php esc_html_e( 'Role/Company:', 'themisdb' ); ?>
                </label>
                <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'role_' . $i ) ); ?>" 
                       name="<?php echo esc_attr( $this->get_field_name( 'role_' . $i ) ); ?>" type="text" 
                       value="<?php echo esc_attr( $role ); ?>">
            </p>
        <?php endfor; ?>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        
        for ( $i = 1; $i <= 3; $i++ ) {
            $instance['quote_' . $i] = ( ! empty( $new_instance['quote_' . $i] ) ) ? sanitize_textarea_field( $new_instance['quote_' . $i] ) : '';
            $instance['author_' . $i] = ( ! empty( $new_instance['author_' . $i] ) ) ? sanitize_text_field( $new_instance['author_' . $i] ) : '';
            $instance['role_' . $i] = ( ! empty( $new_instance['role_' . $i] ) ) ? sanitize_text_field( $new_instance['role_' . $i] ) : '';
        }
        
        return $instance;
    }
}

/**
 * Image Gallery Carousel Widget
 * Rotating images with captions
 */
class ThemisDB_Image_Carousel_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_image_carousel',
            esc_html__( 'ThemisDB: Image Carousel', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display rotating images with captions in a carousel', 'themisdb' ),
                'classname' => 'themisdb-image-carousel-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $title = apply_filters( 'widget_title', $title, $instance, $this->id_base );

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        $images = array();
        for ( $i = 1; $i <= 5; $i++ ) {
            $image_url = ! empty( $instance['image_url_' . $i] ) ? $instance['image_url_' . $i] : '';
            $caption = ! empty( $instance['caption_' . $i] ) ? $instance['caption_' . $i] : '';
            $link = ! empty( $instance['link_' . $i] ) ? $instance['link_' . $i] : '';
            
            if ( $image_url ) {
                $images[] = array(
                    'url'     => $image_url,
                    'caption' => $caption,
                    'link'    => $link,
                );
            }
        }

        if ( ! empty( $images ) ) :
            ?>
            <div class="themisdb-image-carousel-container">
                <div class="themisdb-image-carousel">
                    <?php foreach ( $images as $index => $image ) : ?>
                        <div class="carousel-image-item <?php echo $index === 0 ? 'active' : ''; ?>">
                            <?php if ( $image['link'] ) : ?>
                                <a href="<?php echo esc_url( $image['link'] ); ?>">
                            <?php endif; ?>
                                <img src="<?php echo esc_url( $image['url'] ); ?>" alt="<?php echo esc_attr( $image['caption'] ); ?>">
                            <?php if ( $image['link'] ) : ?>
                                </a>
                            <?php endif; ?>
                            <?php if ( $image['caption'] ) : ?>
                                <div class="carousel-caption">
                                    <?php echo esc_html( $image['caption'] ); ?>
                                </div>
                            <?php endif; ?>
                        </div>
                    <?php endforeach; ?>
                </div>
                <?php if ( count( $images ) > 1 ) : ?>
                    <button class="carousel-nav carousel-prev" aria-label="<?php esc_attr_e( 'Previous image', 'themisdb' ); ?>">‹</button>
                    <button class="carousel-nav carousel-next" aria-label="<?php esc_attr_e( 'Next image', 'themisdb' ); ?>">›</button>
                    <div class="carousel-indicators"></div>
                <?php endif; ?>
            </div>
            <?php
        else :
            echo '<p>' . esc_html__( 'Please configure images in widget settings.', 'themisdb' ) . '</p>';
        endif;

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'Gallery', 'themisdb' );
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <?php for ( $i = 1; $i <= 5; $i++ ) : 
            $image_url = ! empty( $instance['image_url_' . $i] ) ? $instance['image_url_' . $i] : '';
            $caption = ! empty( $instance['caption_' . $i] ) ? $instance['caption_' . $i] : '';
            $link = ! empty( $instance['link_' . $i] ) ? $instance['link_' . $i] : '';
        ?>
            <p><strong><?php echo sprintf( esc_html__( 'Image %d', 'themisdb' ), $i ); ?></strong></p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'image_url_' . $i ) ); ?>">
                    <?php esc_html_e( 'Image URL:', 'themisdb' ); ?>
                </label>
                <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'image_url_' . $i ) ); ?>" 
                       name="<?php echo esc_attr( $this->get_field_name( 'image_url_' . $i ) ); ?>" type="url" 
                       value="<?php echo esc_url( $image_url ); ?>">
            </p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'caption_' . $i ) ); ?>">
                    <?php esc_html_e( 'Caption:', 'themisdb' ); ?>
                </label>
                <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'caption_' . $i ) ); ?>" 
                       name="<?php echo esc_attr( $this->get_field_name( 'caption_' . $i ) ); ?>" type="text" 
                       value="<?php echo esc_attr( $caption ); ?>">
            </p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'link_' . $i ) ); ?>">
                    <?php esc_html_e( 'Link URL (optional):', 'themisdb' ); ?>
                </label>
                <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'link_' . $i ) ); ?>" 
                       name="<?php echo esc_attr( $this->get_field_name( 'link_' . $i ) ); ?>" type="url" 
                       value="<?php echo esc_url( $link ); ?>">
            </p>
        <?php endfor; ?>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        
        for ( $i = 1; $i <= 5; $i++ ) {
            $instance['image_url_' . $i] = ( ! empty( $new_instance['image_url_' . $i] ) ) ? esc_url_raw( $new_instance['image_url_' . $i] ) : '';
            $instance['caption_' . $i] = ( ! empty( $new_instance['caption_' . $i] ) ) ? sanitize_text_field( $new_instance['caption_' . $i] ) : '';
            $instance['link_' . $i] = ( ! empty( $new_instance['link_' . $i] ) ) ? esc_url_raw( $new_instance['link_' . $i] ) : '';
        }
        
        return $instance;
    }
}

/**
 * Timeline/Milestones Carousel Widget
 * Display events or milestones in a sliding timeline
 */
class ThemisDB_Timeline_Carousel_Widget extends WP_Widget {

    public function __construct() {
        parent::__construct(
            'themisdb_timeline_carousel',
            esc_html__( 'ThemisDB: Timeline Carousel', 'themisdb' ),
            array( 
                'description' => esc_html__( 'Display timeline events or milestones in a carousel', 'themisdb' ),
                'classname' => 'themisdb-timeline-carousel-widget'
            )
        );
    }

    public function widget( $args, $instance ) {
        echo $args['before_widget'];

        $title = ! empty( $instance['title'] ) ? $instance['title'] : '';
        $title = apply_filters( 'widget_title', $title, $instance, $this->id_base );

        if ( $title ) {
            echo $args['before_title'] . esc_html( $title ) . $args['after_title'];
        }

        $events = array();
        for ( $i = 1; $i <= 5; $i++ ) {
            $date = ! empty( $instance['date_' . $i] ) ? $instance['date_' . $i] : '';
            $event_title = ! empty( $instance['event_title_' . $i] ) ? $instance['event_title_' . $i] : '';
            $description = ! empty( $instance['description_' . $i] ) ? $instance['description_' . $i] : '';
            
            if ( $date && $event_title ) {
                $events[] = array(
                    'date'        => $date,
                    'title'       => $event_title,
                    'description' => $description,
                );
            }
        }

        if ( ! empty( $events ) ) :
            ?>
            <div class="themisdb-timeline-container">
                <div class="themisdb-timeline-carousel">
                    <?php foreach ( $events as $index => $event ) : ?>
                        <div class="timeline-item <?php echo $index === 0 ? 'active' : ''; ?>">
                            <div class="timeline-date">
                                <?php echo esc_html( $event['date'] ); ?>
                            </div>
                            <div class="timeline-content">
                                <h4 class="timeline-title"><?php echo esc_html( $event['title'] ); ?></h4>
                                <?php if ( $event['description'] ) : ?>
                                    <p class="timeline-description"><?php echo esc_html( $event['description'] ); ?></p>
                                <?php endif; ?>
                            </div>
                        </div>
                    <?php endforeach; ?>
                </div>
                <?php if ( count( $events ) > 1 ) : ?>
                    <div class="timeline-nav">
                        <button class="timeline-prev" aria-label="<?php esc_attr_e( 'Previous event', 'themisdb' ); ?>">‹</button>
                        <button class="timeline-next" aria-label="<?php esc_attr_e( 'Next event', 'themisdb' ); ?>">›</button>
                    </div>
                    <div class="timeline-progress"></div>
                <?php endif; ?>
            </div>
            <?php
        else :
            echo '<p>' . esc_html__( 'Please configure timeline events in widget settings.', 'themisdb' ) . '</p>';
        endif;

        echo $args['after_widget'];
    }

    public function form( $instance ) {
        $title = ! empty( $instance['title'] ) ? $instance['title'] : esc_html__( 'Timeline', 'themisdb' );
        ?>
        <p>
            <label for="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>">
                <?php esc_html_e( 'Title:', 'themisdb' ); ?>
            </label>
            <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'title' ) ); ?>" 
                   name="<?php echo esc_attr( $this->get_field_name( 'title' ) ); ?>" type="text" 
                   value="<?php echo esc_attr( $title ); ?>">
        </p>
        <?php for ( $i = 1; $i <= 5; $i++ ) : 
            $date = ! empty( $instance['date_' . $i] ) ? $instance['date_' . $i] : '';
            $event_title = ! empty( $instance['event_title_' . $i] ) ? $instance['event_title_' . $i] : '';
            $description = ! empty( $instance['description_' . $i] ) ? $instance['description_' . $i] : '';
        ?>
            <p><strong><?php echo sprintf( esc_html__( 'Event %d', 'themisdb' ), $i ); ?></strong></p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'date_' . $i ) ); ?>">
                    <?php esc_html_e( 'Date:', 'themisdb' ); ?>
                </label>
                <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'date_' . $i ) ); ?>" 
                       name="<?php echo esc_attr( $this->get_field_name( 'date_' . $i ) ); ?>" type="text" 
                       value="<?php echo esc_attr( $date ); ?>" placeholder="e.g., 2024 or Jan 2024">
            </p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'event_title_' . $i ) ); ?>">
                    <?php esc_html_e( 'Event Title:', 'themisdb' ); ?>
                </label>
                <input class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'event_title_' . $i ) ); ?>" 
                       name="<?php echo esc_attr( $this->get_field_name( 'event_title_' . $i ) ); ?>" type="text" 
                       value="<?php echo esc_attr( $event_title ); ?>">
            </p>
            <p>
                <label for="<?php echo esc_attr( $this->get_field_id( 'description_' . $i ) ); ?>">
                    <?php esc_html_e( 'Description:', 'themisdb' ); ?>
                </label>
                <textarea class="widefat" id="<?php echo esc_attr( $this->get_field_id( 'description_' . $i ) ); ?>" 
                          name="<?php echo esc_attr( $this->get_field_name( 'description_' . $i ) ); ?>" rows="2"><?php echo esc_textarea( $description ); ?></textarea>
            </p>
        <?php endfor; ?>
        <?php
    }

    public function update( $new_instance, $old_instance ) {
        $instance = array();
        $instance['title'] = ( ! empty( $new_instance['title'] ) ) ? sanitize_text_field( $new_instance['title'] ) : '';
        
        for ( $i = 1; $i <= 5; $i++ ) {
            $instance['date_' . $i] = ( ! empty( $new_instance['date_' . $i] ) ) ? sanitize_text_field( $new_instance['date_' . $i] ) : '';
            $instance['event_title_' . $i] = ( ! empty( $new_instance['event_title_' . $i] ) ) ? sanitize_text_field( $new_instance['event_title_' . $i] ) : '';
            $instance['description_' . $i] = ( ! empty( $new_instance['description_' . $i] ) ) ? sanitize_textarea_field( $new_instance['description_' . $i] ) : '';
        }
        
        return $instance;
    }
}

/**
 * Register all custom widgets
 */
function themisdb_register_widgets() {
    register_widget( 'ThemisDB_Featured_Slider_Widget' );
    register_widget( 'ThemisDB_Recent_Posts_Widget' );
    register_widget( 'ThemisDB_Category_Highlights_Widget' );
    register_widget( 'ThemisDB_CTA_Widget' );
    register_widget( 'ThemisDB_About_Widget' );
    register_widget( 'ThemisDB_Social_Widget' );
    register_widget( 'ThemisDB_Tag_Cloud_Widget' );
    register_widget( 'ThemisDB_Quick_Links_Widget' );
    register_widget( 'ThemisDB_Testimonials_Carousel_Widget' );
    register_widget( 'ThemisDB_Image_Carousel_Widget' );
    register_widget( 'ThemisDB_Timeline_Carousel_Widget' );
}
add_action( 'widgets_init', 'themisdb_register_widgets' );
