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
 * Register all custom widgets
 */
function themisdb_register_widgets() {
    register_widget( 'ThemisDB_Featured_Slider_Widget' );
    register_widget( 'ThemisDB_Recent_Posts_Widget' );
    register_widget( 'ThemisDB_Category_Highlights_Widget' );
    register_widget( 'ThemisDB_CTA_Widget' );
}
add_action( 'widgets_init', 'themisdb_register_widgets' );
