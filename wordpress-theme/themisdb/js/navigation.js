/**
 * Navigation scripts for ThemisDB theme
 */
(function() {
    'use strict';

    // Mobile menu toggle
    const menuToggle = document.querySelector('.menu-toggle');
    const navigation = document.querySelector('.main-navigation');

    if (menuToggle && navigation) {
        menuToggle.addEventListener('click', function() {
            const expanded = menuToggle.getAttribute('aria-expanded') === 'true';
            menuToggle.setAttribute('aria-expanded', !expanded);
            navigation.classList.toggle('toggled');
        });
    }

    // Add dropdown toggle for mobile
    const menuItemsWithChildren = document.querySelectorAll('.main-navigation .menu-item-has-children');
    
    // Desktop dropdown delay to prevent accidental closing
    let dropdownTimeout;
    if (window.innerWidth > 768) {
        menuItemsWithChildren.forEach(function(item) {
            const submenu = item.querySelector('ul');
            if (submenu) {
                // Clear any existing timeout when mouse enters
                item.addEventListener('mouseenter', function() {
                    clearTimeout(dropdownTimeout);
                });
            }
        });
    }
    
    menuItemsWithChildren.forEach(function(item) {
        const link = item.querySelector('a');
        if (link) {
            link.addEventListener('click', function(e) {
                if (window.innerWidth <= 768) {
                    e.preventDefault();
                    item.classList.toggle('toggled');
                }
            });
        }
    });

    // Close menu when clicking outside
    document.addEventListener('click', function(event) {
        if (!event.target.closest('.site-header') && navigation && navigation.classList.contains('toggled')) {
            navigation.classList.remove('toggled');
            if (menuToggle) {
                menuToggle.setAttribute('aria-expanded', 'false');
            }
        }
    });

    // Handle window resize
    let resizeTimer;
    window.addEventListener('resize', function() {
        clearTimeout(resizeTimer);
        resizeTimer = setTimeout(function() {
            if (window.innerWidth > 768 && navigation && navigation.classList.contains('toggled')) {
                navigation.classList.remove('toggled');
                if (menuToggle) {
                    menuToggle.setAttribute('aria-expanded', 'false');
                }
            }
        }, 250);
    });

    // Smooth scroll for anchor links
    const anchorLinks = document.querySelectorAll('a[href^="#"]');
    anchorLinks.forEach(function(link) {
        link.addEventListener('click', function(e) {
            const targetId = this.getAttribute('href');
            if (targetId !== '#' && targetId !== '#0') {
                const targetElement = document.querySelector(targetId);
                if (targetElement) {
                    e.preventDefault();
                    targetElement.scrollIntoView({
                        behavior: 'smooth',
                        block: 'start'
                    });
                }
            }
        });
    });

    // Hamburger menu toggle
    const hamburgerButton = document.querySelector('.hamburger-menu-button');
    const hamburgerDropdown = document.querySelector('.hamburger-dropdown');

    if (hamburgerButton && hamburgerDropdown) {
        hamburgerButton.addEventListener('click', function(e) {
            e.stopPropagation();
            const expanded = hamburgerButton.getAttribute('aria-expanded') === 'true';
            hamburgerButton.setAttribute('aria-expanded', !expanded);
        });

        // Close hamburger menu when clicking outside
        document.addEventListener('click', function(event) {
            if (!event.target.closest('.hamburger-menu-item')) {
                hamburgerButton.setAttribute('aria-expanded', 'false');
            }
        });

        // Close on escape key
        document.addEventListener('keydown', function(e) {
            if (e.key === 'Escape' && hamburgerButton.getAttribute('aria-expanded') === 'true') {
                hamburgerButton.setAttribute('aria-expanded', 'false');
                hamburgerButton.focus();
            }
        });
    }

})();
