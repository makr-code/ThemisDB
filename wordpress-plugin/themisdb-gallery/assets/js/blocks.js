/**
 * ThemisDB Gallery - Gutenberg Blocks
 */

(function(blocks, element, editor, components, i18n) {
    
    var el = element.createElement;
    var registerBlockType = blocks.registerBlockType;
    var InspectorControls = editor.InspectorControls;
    var TextControl = components.TextControl;
    var SelectControl = components.SelectControl;
    var RangeControl = components.RangeControl;
    var ToggleControl = components.ToggleControl;
    var PanelBody = components.PanelBody;
    var __ = i18n.__;
    
    /**
     * Register Image Search Block
     */
    registerBlockType('themisdb-gallery/image-search', {
        title: __('ThemisDB Bildsuche'),
        icon: 'search',
        category: 'media',
        attributes: {
            query: {
                type: 'string',
                default: ''
            },
            provider: {
                type: 'string',
                default: 'all'
            },
            columns: {
                type: 'number',
                default: 3
            },
            limit: {
                type: 'number',
                default: 12
            },
            showAttribution: {
                type: 'boolean',
                default: true
            }
        },
        
        edit: function(props) {
            var attributes = props.attributes;
            var setAttributes = props.setAttributes;
            
            return el('div', { className: 'themisdb-block-editor' },
                el(InspectorControls, {},
                    el(PanelBody, { title: __('Einstellungen') },
                        el(TextControl, {
                            label: __('Suchbegriff'),
                            value: attributes.query,
                            onChange: function(value) {
                                setAttributes({ query: value });
                            }
                        }),
                        el(SelectControl, {
                            label: __('Anbieter'),
                            value: attributes.provider,
                            options: [
                                { label: __('Alle'), value: 'all' },
                                { label: 'Unsplash', value: 'unsplash' },
                                { label: 'Pexels', value: 'pexels' },
                                { label: 'Pixabay', value: 'pixabay' }
                            ],
                            onChange: function(value) {
                                setAttributes({ provider: value });
                            }
                        }),
                        el(RangeControl, {
                            label: __('Spalten'),
                            value: attributes.columns,
                            min: 1,
                            max: 4,
                            onChange: function(value) {
                                setAttributes({ columns: value });
                            }
                        }),
                        el(RangeControl, {
                            label: __('Anzahl Bilder'),
                            value: attributes.limit,
                            min: 1,
                            max: 50,
                            onChange: function(value) {
                                setAttributes({ limit: value });
                            }
                        }),
                        el(ToggleControl, {
                            label: __('Quellenangaben anzeigen'),
                            checked: attributes.showAttribution,
                            onChange: function(value) {
                                setAttributes({ showAttribution: value });
                            }
                        })
                    )
                ),
                el('div', { className: 'themisdb-block-preview' },
                    el('p', { style: { padding: '20px', background: '#f0f0f0', textAlign: 'center' } },
                        attributes.query 
                            ? __('Bildergalerie: ') + attributes.query + ' (' + attributes.limit + ' Bilder von ' + attributes.provider + ')'
                            : __('Bitte geben Sie einen Suchbegriff in den Block-Einstellungen ein.')
                    )
                )
            );
        },
        
        save: function() {
            // Server-side rendering
            return null;
        }
    });
    
    /**
     * Register Gallery Block
     */
    registerBlockType('themisdb-gallery/gallery', {
        title: __('ThemisDB Galerie'),
        icon: 'format-gallery',
        category: 'media',
        attributes: {
            ids: {
                type: 'array',
                default: []
            },
            columns: {
                type: 'number',
                default: 3
            },
            showAttribution: {
                type: 'boolean',
                default: true
            }
        },
        
        edit: function(props) {
            var attributes = props.attributes;
            var setAttributes = props.setAttributes;
            
            return el('div', { className: 'themisdb-block-editor' },
                el(InspectorControls, {},
                    el(PanelBody, { title: __('Einstellungen') },
                        el(RangeControl, {
                            label: __('Spalten'),
                            value: attributes.columns,
                            min: 1,
                            max: 4,
                            onChange: function(value) {
                                setAttributes({ columns: value });
                            }
                        }),
                        el(ToggleControl, {
                            label: __('Quellenangaben anzeigen'),
                            checked: attributes.showAttribution,
                            onChange: function(value) {
                                setAttributes({ showAttribution: value });
                            }
                        })
                    )
                ),
                el('div', { className: 'themisdb-block-preview' },
                    el('p', { style: { padding: '20px', background: '#f0f0f0', textAlign: 'center' } },
                        attributes.ids.length > 0
                            ? __('Galerie mit ') + attributes.ids.length + __(' Bildern')
                            : __('Verwenden Sie das "Medien hinzufügen" Button um Bilder auszuwählen.')
                    )
                )
            );
        },
        
        save: function() {
            // Server-side rendering
            return null;
        }
    });
    
})(
    window.wp.blocks,
    window.wp.element,
    window.wp.blockEditor || window.wp.editor,
    window.wp.components,
    window.wp.i18n
);
