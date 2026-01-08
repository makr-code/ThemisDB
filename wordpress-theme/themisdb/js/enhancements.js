/**
 * Modern enhancements for ThemisDB theme
 * Includes: Reading progress, lazy loading, animations, and interactive features
 */
(function() {
    'use strict';

    /**
     * Reading Progress Indicator
     */
    function initReadingProgress() {
        // Create progress bar element
        const progressBar = document.createElement('div');
        progressBar.className = 'reading-progress';
        document.body.appendChild(progressBar);

        // Update progress on scroll
        function updateProgress() {
            const winScroll = document.body.scrollTop || document.documentElement.scrollTop;
            const height = document.documentElement.scrollHeight - document.documentElement.clientHeight;
            const scrolled = (winScroll / height) * 100;
            progressBar.style.width = scrolled + '%';
        }

        // Throttle scroll events
        let ticking = false;
        window.addEventListener('scroll', function() {
            if (!ticking) {
                window.requestAnimationFrame(function() {
                    updateProgress();
                    ticking = false;
                });
                ticking = true;
            }
        });
    }

    /**
     * Lazy Loading for Images
     */
    function initLazyLoading() {
        if ('IntersectionObserver' in window) {
            const imageObserver = new IntersectionObserver((entries, observer) => {
                entries.forEach(entry => {
                    if (entry.isIntersecting) {
                        const img = entry.target;
                        if (img.dataset.src) {
                            img.src = img.dataset.src;
                            img.classList.add('loaded');
                            observer.unobserve(img);
                        }
                    }
                });
            });

            // Observe all images with data-src attribute
            const lazyImages = document.querySelectorAll('img[data-src]');
            lazyImages.forEach(img => imageObserver.observe(img));
        }
    }

    /**
     * Accordion functionality
     */
    function initAccordion() {
        const accordionHeaders = document.querySelectorAll('.accordion-header');
        
        accordionHeaders.forEach(header => {
            header.addEventListener('click', function() {
                const content = this.nextElementSibling;
                const isActive = this.classList.contains('active');
                
                // Close all accordions
                document.querySelectorAll('.accordion-header').forEach(h => {
                    h.classList.remove('active');
                    if (h.nextElementSibling) {
                        h.nextElementSibling.classList.remove('active');
                    }
                });
                
                // Toggle current accordion
                if (!isActive) {
                    this.classList.add('active');
                    if (content) {
                        content.classList.add('active');
                    }
                }
            });
        });
    }

    /**
     * Smooth reveal animations on scroll
     */
    function initScrollAnimations() {
        if ('IntersectionObserver' in window) {
            const revealElements = document.querySelectorAll('.reveal-on-scroll');
            
            const revealObserver = new IntersectionObserver((entries) => {
                entries.forEach(entry => {
                    if (entry.isIntersecting) {
                        entry.target.classList.add('revealed');
                    }
                });
            }, {
                threshold: 0.15
            });

            revealElements.forEach(el => revealObserver.observe(el));
        }
    }

    /**
     * Image Gallery Lightbox
     */
    function initImageGallery() {
        const galleryImages = document.querySelectorAll('.gallery img, .wp-block-gallery img');
        
        if (galleryImages.length === 0) return;

        // Create lightbox overlay
        const lightbox = document.createElement('div');
        lightbox.className = 'lightbox-overlay';
        lightbox.innerHTML = `
            <div class="lightbox-content">
                <button class="lightbox-close" aria-label="Close">✕</button>
                <button class="lightbox-prev" aria-label="Previous">‹</button>
                <img src="" alt="" class="lightbox-image">
                <button class="lightbox-next" aria-label="Next">›</button>
            </div>
        `;
        document.body.appendChild(lightbox);

        const lightboxImg = lightbox.querySelector('.lightbox-image');
        const closeBtn = lightbox.querySelector('.lightbox-close');
        const prevBtn = lightbox.querySelector('.lightbox-prev');
        const nextBtn = lightbox.querySelector('.lightbox-next');
        let currentIndex = 0;
        let images = [];

        function showLightbox(index) {
            currentIndex = index;
            lightboxImg.src = images[currentIndex].src;
            lightboxImg.alt = images[currentIndex].alt;
            lightbox.classList.add('active');
            document.body.style.overflow = 'hidden';
        }

        function hideLightbox() {
            lightbox.classList.remove('active');
            document.body.style.overflow = '';
        }

        function showNext() {
            currentIndex = (currentIndex + 1) % images.length;
            lightboxImg.src = images[currentIndex].src;
            lightboxImg.alt = images[currentIndex].alt;
        }

        function showPrev() {
            currentIndex = (currentIndex - 1 + images.length) % images.length;
            lightboxImg.src = images[currentIndex].src;
            lightboxImg.alt = images[currentIndex].alt;
        }

        // Store all gallery images
        images = Array.from(galleryImages);

        // Add click events to gallery images
        galleryImages.forEach((img, index) => {
            img.style.cursor = 'pointer';
            img.addEventListener('click', () => showLightbox(index));
        });

        // Lightbox controls
        closeBtn.addEventListener('click', hideLightbox);
        prevBtn.addEventListener('click', showPrev);
        nextBtn.addEventListener('click', showNext);
        lightbox.addEventListener('click', (e) => {
            if (e.target === lightbox) hideLightbox();
        });

        // Keyboard navigation
        document.addEventListener('keydown', (e) => {
            if (!lightbox.classList.contains('active')) return;
            
            if (e.key === 'Escape') hideLightbox();
            if (e.key === 'ArrowRight') showNext();
            if (e.key === 'ArrowLeft') showPrev();
        });
    }

    /**
     * Animated counter for stats
     */
    function initCounters() {
        const counters = document.querySelectorAll('.stat-number');
        
        if (counters.length === 0) return;

        const observerOptions = {
            threshold: 0.5
        };

        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    const counter = entry.target;
                    const target = parseInt(counter.dataset.count || counter.textContent);
                    const duration = 2000; // 2 seconds
                    const increment = target / (duration / 16); // 60fps
                    let current = 0;

                    const updateCounter = () => {
                        current += increment;
                        if (current < target) {
                            counter.textContent = Math.floor(current);
                            requestAnimationFrame(updateCounter);
                        } else {
                            counter.textContent = target;
                        }
                    };

                    updateCounter();
                    observer.unobserve(counter);
                }
            });
        }, observerOptions);

        counters.forEach(counter => observer.observe(counter));
    }

    /**
     * Back to top button
     */
    function initBackToTop() {
        const backToTop = document.createElement('button');
        backToTop.className = 'back-to-top';
        backToTop.innerHTML = '↑';
        backToTop.setAttribute('aria-label', 'Back to top');
        backToTop.style.cssText = `
            position: fixed;
            bottom: 30px;
            right: 30px;
            width: 50px;
            height: 50px;
            border-radius: 50%;
            background: var(--accent-purple, #7c4dff);
            color: white;
            border: none;
            font-size: 24px;
            cursor: pointer;
            opacity: 0;
            visibility: hidden;
            transition: all 0.3s ease;
            z-index: 999;
            box-shadow: 0 4px 12px rgba(124, 77, 255, 0.4);
        `;
        document.body.appendChild(backToTop);

        // Show/hide button based on scroll position
        window.addEventListener('scroll', () => {
            if (window.pageYOffset > 300) {
                backToTop.style.opacity = '1';
                backToTop.style.visibility = 'visible';
            } else {
                backToTop.style.opacity = '0';
                backToTop.style.visibility = 'hidden';
            }
        });

        // Scroll to top on click with fallback for older browsers
        backToTop.addEventListener('click', () => {
            // Try modern smooth scroll first
            if ('scrollBehavior' in document.documentElement.style) {
                window.scrollTo({
                    top: 0,
                    behavior: 'smooth'
                });
            } else {
                // Fallback for older browsers - animated scroll
                const scrollToTop = () => {
                    const c = document.documentElement.scrollTop || document.body.scrollTop;
                    if (c > 0) {
                        window.requestAnimationFrame(scrollToTop);
                        window.scrollTo(0, c - c / 8);
                    }
                };
                scrollToTop();
            }
        });
    }

    /**
     * Tooltips initialization
     */
    function initTooltips() {
        // Automatically add tooltips to abbreviations
        const abbrs = document.querySelectorAll('abbr[title]');
        abbrs.forEach(abbr => {
            abbr.classList.add('tooltip');
            const tooltipText = document.createElement('span');
            tooltipText.className = 'tooltiptext';
            tooltipText.textContent = abbr.getAttribute('title');
            abbr.appendChild(tooltipText);
        });
    }

    /**
     * Copy code blocks to clipboard
     */
    function initCodeCopy() {
        const codeBlocks = document.querySelectorAll('pre code');
        
        codeBlocks.forEach(code => {
            const pre = code.parentElement;
            const copyBtn = document.createElement('button');
            copyBtn.className = 'copy-code-btn';
            copyBtn.innerHTML = '📋 Copy';
            copyBtn.style.cssText = `
                position: absolute;
                top: 8px;
                right: 8px;
                padding: 4px 12px;
                background: rgba(255, 255, 255, 0.2);
                color: white;
                border: 1px solid rgba(255, 255, 255, 0.3);
                border-radius: 4px;
                cursor: pointer;
                font-size: 12px;
                transition: all 0.3s ease;
            `;
            
            pre.style.position = 'relative';
            pre.appendChild(copyBtn);

            copyBtn.addEventListener('click', async () => {
                const text = code.textContent;
                try {
                    // Modern clipboard API with fallback
                    if (navigator.clipboard && window.isSecureContext) {
                        await navigator.clipboard.writeText(text);
                        copyBtn.innerHTML = '✅ Copied!';
                    } else {
                        // Fallback for older browsers
                        const textArea = document.createElement('textarea');
                        textArea.value = text;
                        textArea.style.position = 'fixed';
                        textArea.style.left = '-999999px';
                        document.body.appendChild(textArea);
                        textArea.select();
                        try {
                            document.execCommand('copy');
                            copyBtn.innerHTML = '✅ Copied!';
                        } catch (err) {
                            copyBtn.innerHTML = '❌ Failed';
                        }
                        document.body.removeChild(textArea);
                    }
                    setTimeout(() => {
                        copyBtn.innerHTML = '📋 Copy';
                    }, 2000);
                } catch (err) {
                    copyBtn.innerHTML = '❌ Failed';
                    setTimeout(() => {
                        copyBtn.innerHTML = '📋 Copy';
                    }, 2000);
                }
            });
        });
    }

    /**
     * External links - open in new tab with icon
     */
    function initExternalLinks() {
        const links = document.querySelectorAll('a[href^="http"]');
        links.forEach(link => {
            const url = new URL(link.href);
            if (url.hostname !== window.location.hostname) {
                link.setAttribute('target', '_blank');
                link.setAttribute('rel', 'noopener noreferrer');
                if (!link.querySelector('.external-icon')) {
                    const icon = document.createElement('span');
                    icon.className = 'external-icon';
                    icon.innerHTML = ' 🔗';
                    icon.style.fontSize = '0.8em';
                    link.appendChild(icon);
                }
            }
        });
    }

    /**
     * Initialize all enhancements
     */
    function init() {
        try {
            initReadingProgress();
            initLazyLoading();
            initAccordion();
            initScrollAnimations();
            initImageGallery();
            initCounters();
            initBackToTop();
            initTooltips();
            initCodeCopy();
            initExternalLinks();
        } catch (error) {
            console.warn('ThemisDB enhancements initialization error:', error);
            // Continue execution even if some features fail
        }
    }

    // Initialize when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})();

/**
 * Copy URL to clipboard function for social share
 */
function themisdbCopyUrl(button) {
    const url = button.getAttribute('data-url');
    
    if (navigator.clipboard && window.isSecureContext) {
        // Modern async clipboard API
        navigator.clipboard.writeText(url).then(() => {
            showCopySuccess(button);
        }).catch(() => {
            fallbackCopyUrl(url, button);
        });
    } else {
        // Fallback for older browsers
        fallbackCopyUrl(url, button);
    }
}

function fallbackCopyUrl(url, button) {
    const textArea = document.createElement('textarea');
    textArea.value = url;
    textArea.style.position = 'fixed';
    textArea.style.left = '-999999px';
    textArea.style.top = '-999999px';
    document.body.appendChild(textArea);
    textArea.focus();
    textArea.select();
    
    try {
        document.execCommand('copy');
        showCopySuccess(button);
    } catch (err) {
        console.error('Failed to copy URL:', err);
    }
    
    document.body.removeChild(textArea);
}

function showCopySuccess(button) {
    const originalText = button.innerHTML;
    button.innerHTML = '✅ ' + (button.getAttribute('data-copied-text') || 'Copied!');
    button.classList.add('copied');
    
    setTimeout(() => {
        button.innerHTML = originalText;
        button.classList.remove('copied');
    }, 2000);
}
