/**
 * Carousel functionality for ThemisDB Theme
 * Handles testimonials, image carousel, and timeline carousels
 *
 * @package ThemisDB
 * @since 1.0.0
 */

(function() {
    'use strict';

    /**
     * Initialize all carousels on the page
     */
    function initCarousels() {
        // Testimonials Carousel
        const testimonialContainers = document.querySelectorAll('.themisdb-testimonials-container');
        testimonialContainers.forEach(function(container) {
            new ThemisDBCarousel(container, {
                itemSelector: '.testimonial-item',
                prevSelector: '.testimonial-prev',
                nextSelector: '.testimonial-next',
                dotsContainer: '.testimonial-dots',
                autoplay: true,
                autoplayDelay: 7000
            });
        });

        // Image Carousel
        const imageCarousels = document.querySelectorAll('.themisdb-image-carousel-container');
        imageCarousels.forEach(function(container) {
            new ThemisDBCarousel(container, {
                itemSelector: '.carousel-image-item',
                prevSelector: '.carousel-prev',
                nextSelector: '.carousel-next',
                dotsContainer: '.carousel-indicators',
                autoplay: true,
                autoplayDelay: 5000
            });
        });

        // Timeline Carousel
        const timelineContainers = document.querySelectorAll('.themisdb-timeline-container');
        timelineContainers.forEach(function(container) {
            new ThemisDBCarousel(container, {
                itemSelector: '.timeline-item',
                prevSelector: '.timeline-prev',
                nextSelector: '.timeline-next',
                progressBar: '.timeline-progress',
                autoplay: false
            });
        });
    }

    /**
     * ThemisDB Carousel Class
     */
    function ThemisDBCarousel(container, options) {
        this.container = container;
        this.items = container.querySelectorAll(options.itemSelector);
        this.prevBtn = container.querySelector(options.prevSelector);
        this.nextBtn = container.querySelector(options.nextSelector);
        this.dotsContainer = options.dotsContainer ? container.querySelector(options.dotsContainer) : null;
        this.progressBar = options.progressBar ? container.querySelector(options.progressBar) : null;
        
        this.currentIndex = 0;
        this.itemCount = this.items.length;
        this.autoplay = options.autoplay || false;
        this.autoplayDelay = options.autoplayDelay || 5000;
        this.autoplayInterval = null;
        
        if (this.itemCount <= 1) {
            return; // No need for carousel if only one item
        }

        this.init();
    }

    ThemisDBCarousel.prototype = {
        /**
         * Initialize the carousel
         */
        init: function() {
            this.createDots();
            this.updateCarousel();
            this.attachEvents();
            
            if (this.autoplay) {
                this.startAutoplay();
            }
        },

        /**
         * Create navigation dots
         */
        createDots: function() {
            if (!this.dotsContainer) return;

            for (let i = 0; i < this.itemCount; i++) {
                const dot = document.createElement('button');
                dot.setAttribute('aria-label', 'Go to item ' + (i + 1));
                dot.addEventListener('click', function() {
                    this.goToItem(i);
                }.bind(this));
                this.dotsContainer.appendChild(dot);
            }
        },

        /**
         * Update carousel display
         */
        updateCarousel: function() {
            // Update items visibility
            this.items.forEach(function(item, index) {
                item.classList.remove('active');
                if (index === this.currentIndex) {
                    item.classList.add('active');
                }
            }.bind(this));

            // Update dots
            if (this.dotsContainer) {
                const dots = this.dotsContainer.querySelectorAll('button');
                dots.forEach(function(dot, index) {
                    dot.classList.remove('active');
                    if (index === this.currentIndex) {
                        dot.classList.add('active');
                    }
                }.bind(this));
            }

            // Update progress bar
            if (this.progressBar) {
                const progressWidth = ((this.currentIndex + 1) / this.itemCount) * 100;
                const progressElement = this.progressBar.querySelector('::after') || this.progressBar;
                if (progressElement.style) {
                    progressElement.style.setProperty('--progress-width', progressWidth + '%');
                }
            }
        },

        /**
         * Go to specific item
         */
        goToItem: function(index) {
            this.currentIndex = index;
            this.updateCarousel();
            this.resetAutoplay();
        },

        /**
         * Go to next item
         */
        nextItem: function() {
            this.currentIndex = (this.currentIndex + 1) % this.itemCount;
            this.updateCarousel();
            this.resetAutoplay();
        },

        /**
         * Go to previous item
         */
        prevItem: function() {
            this.currentIndex = (this.currentIndex - 1 + this.itemCount) % this.itemCount;
            this.updateCarousel();
            this.resetAutoplay();
        },

        /**
         * Attach event listeners
         */
        attachEvents: function() {
            // Navigation buttons
            if (this.prevBtn) {
                this.prevBtn.addEventListener('click', function(e) {
                    e.preventDefault();
                    this.prevItem();
                }.bind(this));
            }

            if (this.nextBtn) {
                this.nextBtn.addEventListener('click', function(e) {
                    e.preventDefault();
                    this.nextItem();
                }.bind(this));
            }

            // Keyboard navigation
            this.container.addEventListener('keydown', function(e) {
                if (e.key === 'ArrowLeft') {
                    this.prevItem();
                } else if (e.key === 'ArrowRight') {
                    this.nextItem();
                }
            }.bind(this));

            // Touch gestures
            let touchStartX = 0;
            let touchEndX = 0;

            this.container.addEventListener('touchstart', function(e) {
                touchStartX = e.changedTouches[0].screenX;
            }, { passive: true });

            this.container.addEventListener('touchend', function(e) {
                touchEndX = e.changedTouches[0].screenX;
                this.handleSwipe(touchStartX, touchEndX);
            }.bind(this), { passive: true });

            // Pause autoplay on hover
            this.container.addEventListener('mouseenter', function() {
                this.stopAutoplay();
            }.bind(this));

            this.container.addEventListener('mouseleave', function() {
                if (this.autoplay) {
                    this.startAutoplay();
                }
            }.bind(this));
        },

        /**
         * Handle swipe gesture
         */
        handleSwipe: function(startX, endX) {
            const threshold = 50; // Minimum swipe distance
            const diff = startX - endX;

            if (Math.abs(diff) > threshold) {
                if (diff > 0) {
                    // Swipe left - next item
                    this.nextItem();
                } else {
                    // Swipe right - previous item
                    this.prevItem();
                }
            }
        },

        /**
         * Start autoplay
         */
        startAutoplay: function() {
            if (this.autoplayInterval) return;

            this.autoplayInterval = setInterval(function() {
                this.nextItem();
            }.bind(this), this.autoplayDelay);
        },

        /**
         * Stop autoplay
         */
        stopAutoplay: function() {
            if (this.autoplayInterval) {
                clearInterval(this.autoplayInterval);
                this.autoplayInterval = null;
            }
        },

        /**
         * Reset autoplay (restart the timer)
         */
        resetAutoplay: function() {
            if (this.autoplay) {
                this.stopAutoplay();
                this.startAutoplay();
            }
        }
    };

    // Initialize carousels when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initCarousels);
    } else {
        initCarousels();
    }

    // Re-initialize carousels when widgets are added/updated (Customizer support)
    document.addEventListener('widget-added', initCarousels);
    document.addEventListener('widget-updated', initCarousels);

})();
