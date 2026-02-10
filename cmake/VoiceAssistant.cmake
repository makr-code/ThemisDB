# Voice Assistant Framework Sources
# Speech-to-Text, Text-to-Speech, and LLM-based voice interaction

if(THEMIS_ENABLE_VOICE_ASSISTANT)
    list(APPEND THEMIS_CORE_SOURCES
        # Voice Assistant Core
        ../src/voice/voice_assistant.cpp
        ../src/voice/voice_assistant_llm.cpp
    )
endif()
