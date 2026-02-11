/**
 * ThemisDB Taxonomy Manager - Tree View JavaScript
 */

jQuery(document).ready(function($) {
    
    // Make tree sortable
    $('.taxonomy-tree').sortable({
        handle: '.term-handle',
        placeholder: 'tree-placeholder',
        tolerance: 'pointer',
        update: function(event, ui) {
            const order = $(this).sortable('toArray', {attribute: 'data-term-id'});
            const taxonomy = $(this).data('taxonomy');
            
            $.post(themisdbTaxonomy.ajaxurl, {
                action: 'themisdb_save_term_order',
                nonce: themisdbTaxonomy.nonce,
                taxonomy: taxonomy,
                order: order
            }, function(response) {
                if (response.success) {
                    showNotice(response.data.message, 'success');
                } else {
                    showNotice(response.data || 'Failed to save order', 'error');
                }
            }).fail(function() {
                showNotice('Failed to save order', 'error');
            });
        }
    });
    
    // Collapsible branches
    $('.tree-toggle').on('click', function(e) {
        e.preventDefault();
        e.stopPropagation();
        
        const $children = $(this).closest('.tree-item').find('> .tree-children');
        $children.slideToggle(200);
        $(this).toggleClass('collapsed');
    });
    
    // Expand all
    $('#expand-all').on('click', function(e) {
        e.preventDefault();
        $('.tree-children').slideDown(200);
        $('.tree-toggle').removeClass('collapsed');
    });
    
    // Collapse all
    $('#collapse-all').on('click', function(e) {
        e.preventDefault();
        $('.tree-children').slideUp(200);
        $('.tree-toggle').addClass('collapsed');
    });
    
    // Show notice helper
    function showNotice(message, type) {
        const $notice = $('<div>')
            .addClass('tree-notice')
            .addClass(type)
            .text(message);
        
        $('.taxonomy-tree-container').prepend($notice);
        
        setTimeout(function() {
            $notice.fadeOut(300, function() {
                $(this).remove();
            });
        }, 3000);
    }
});
