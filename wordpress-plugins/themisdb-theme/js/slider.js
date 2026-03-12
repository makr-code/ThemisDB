/**
 * Featured Slider functionality for ThemisDB Theme
 * Handles slider navigation, autoplay, and touch gestures
 *
 * @package ThemisDB
 * @since 1.0.0
 */

(function() {
    'use strict';

    /**
     * Initialize all sliders on the page
     */
    function initSliders() {
        const sliders = document.querySelectorAll('.themisdb-slider-container');
        
        sliders.forEach(function(container) {
            new ThemisDBSlider(container);
        });
    }

    /**
     * ThemisDB Slider Class
     */
    function ThemisDBSlider(container) {
        this.container = container;
        this.slider = container.querySelector('.themisdb-slider');
        this.slides = container.querySelectorAll('.slider-item');
        this.prevBtn = container.querySelector('.slider-prev');
        this.nextBtn = container.querySelector('.slider-next');
        this.dotsContainer = container.querySelector('.slider-dots');
        
        this.currentSlide = 0;
        this.slideCount = this.slides.length;
        this.autoplayInterval = null;
        this.autoplayDelay = 5000; // 5 seconds
        
        if (this.slideCount <= 1) {
            return; // No need for slider if only one slide
        }

        this.init();
    }

    ThemisDBSlider.prototype = {
        /**
         * Initialize the slider
         */
        init: function() {
            this.createDots();
            this.updateSlider();
            this.attachEvents();
            this.startAutoplay();
        },

        /**
         * Create navigation dots
         */
        createDots: function() {
            if (!this.dotsContainer) return;

            for (let i = 0; i < this.slideCount; i++) {
                const dot = document.createElement('button');
                dot.className = 'slider-dot';
                dot.setAttribute('aria-label', 'Go to slide ' + (i + 1));
                dot.addEventListener('click', function() {
                    this.goToSlide(i);
                }.bind(this));
                this.dotsContainer.appendChild(dot);
            }
        },

        /**
         * Update slider position and active states
         */
        updateSlider: function() {
            // Update slides visibility
            this.slides.forEach(function(slide, index) {
                slide.classList.remove('active');
                if (index === this.currentSlide) {
                    slide.classList.add('active');
                }
            }.bind(this));

            // Update dots
            if (this.dotsContainer) {
                const dots = this.dotsContainer.querySelectorAll('.slider-dot');
                dots.forEach(function(dot, index) {
                    dot.classList.remove('active');
                    if (index === this.currentSlide) {
                        dot.classList.add('active');
                    }
                }.bind(this));
            }

            // Transform slider
            const translateX = -this.currentSlide * 100;
            this.slider.style.transform = 'translateX(' + translateX + '%)';
        },

        /**
         * Go to specific slide
         */
        goToSlide: function(index) {
            this.currentSlide = index;
            this.updateSlider();
            this.resetAutoplay();
        },

        /**
         * Go to next slide
         */
        nextSlide: function() {
            this.currentSlide = (this.currentSlide + 1) % this.slideCount;
            this.updateSlider();
            this.resetAutoplay();
        },

        /**
         * Go to previous slide
         */
        prevSlide: function() {
            this.currentSlide = (this.currentSlide - 1 + this.slideCount) % this.slideCount;
            this.updateSlider();
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
                    this.prevSlide();
                }.bind(this));
            }

            if (this.nextBtn) {
                this.nextBtn.addEventListener('click', function(e) {
                    e.preventDefault();
                    this.nextSlide();
                }.bind(this));
            }

            // Keyboard navigation
            this.container.addEventListener('keydown', function(e) {
                if (e.key === 'ArrowLeft') {
                    this.prevSlide();
                } else if (e.key === 'ArrowRight') {
                    this.nextSlide();
                }
            }.bind(this));

            // Touch gestures
            let touchStartX = 0;
            let touchEndX = 0;

            this.slider.addEventListener('touchstart', function(e) {
                touchStartX = e.changedTouches[0].screenX;
            });

            this.slider.addEventListener('touchend', function(e) {
                touchEndX = e.changedTouches[0].screenX;
                this.handleSwipe(touchStartX, touchEndX);
            }.bind(this));

            // Pause autoplay on hover
            this.container.addEventListener('mouseenter', function() {
                this.stopAutoplay();
            }.bind(this));

            this.container.addEventListener('mouseleave', function() {
                this.startAutoplay();
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
                    // Swipe left - next slide
                    this.nextSlide();
                } else {
                    // Swipe right - previous slide
                    this.prevSlide();
                }
            }
        },

        /**
         * Start autoplay
         */
        startAutoplay: function() {
            if (this.autoplayInterval) return;

            this.autoplayInterval = setInterval(function() {
                this.nextSlide();
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
            this.stopAutoplay();
            this.startAutoplay();
        }
    };

    // Initialize sliders when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initSliders);
    } else {
        initSliders();
    }

    // Re-initialize sliders when widgets are added/updated (Customizer support)
    document.addEventListener('widget-added', initSliders);
    document.addEventListener('widget-updated', initSliders);

})();
