# Content Processing Sources (Document/Media Processors)
# These handle content type detection and processing
# Enabled conditionally based on THEMIS_ENABLE_CONTENT

if(THEMIS_ENABLE_CONTENT)
    list(APPEND THEMIS_CORE_SOURCES
        # Document processors
        ../src/content/audio_processor.cpp
        ../src/content/image_processor.cpp
        ../src/content/pdf_processor.cpp
        ../src/content/cad_processor.cpp
        ../src/content/geo_processor.cpp
        ../src/content/ocr_processor.cpp
    )
endif()
