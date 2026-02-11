/**
 * ThemisDB Taxonomy Manager - Term Editor JavaScript
 * Handles icon and color picker functionality
 */

jQuery(document).ready(function($) {
    
    // Initialize color picker
    if ($.fn.wpColorPicker) {
        $('.color-picker').wpColorPicker();
    }
    
    // Icon preset buttons (future enhancement)
    const iconPresets = ['📦', '🗄️', '🕸️', '📄', '🎯', '⏱️', '🤖', '🧠', '🔍', '🚀'];
    
    // Add icon preset selector if on term edit page
    if ($('#term_icon').length) {
        const $iconPresets = $('<div class="icon-presets">');
        
        iconPresets.forEach(function(icon) {
            const $btn = $('<button type="button" class="icon-preset-btn">')
                .text(icon)
                .on('click', function(e) {
                    e.preventDefault();
                    $('#term_icon').val(icon);
                });
            $iconPresets.append($btn);
        });
        
        $('#term_icon').after($iconPresets);
    }
});
