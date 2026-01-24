#include <llama.h>

extern "C" {
// Stub implementation to satisfy linkage when llama.cpp build lacks LoRA adapter APIs
int llama_lora_adapter_set(struct llama_context*, const char*) {
    return -1;
}
}
