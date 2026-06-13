#include "ArenaAllocator.h"

ArenaAllocator::ArenaAllocator(size_t chunk_size) : default_chunk_size(chunk_size) {
    allocate_new_chunk(default_chunk_size);
}

void* ArenaAllocator::allocate(size_t size, size_t alignment) {
    if (chunks.empty())
        allocate_new_chunk(std::max(default_chunk_size, size));

    Chunk* active_chunk = &chunks.back();

    uintptr_t current_address =
        reinterpret_cast<uintptr_t>(active_chunk->data.get() + active_chunk->used);
    size_t padding = (alignment - (current_address % alignment)) % alignment;

    if (active_chunk->used + padding + size > active_chunk->capacity) {
        allocate_new_chunk(std::max(default_chunk_size, size + alignment));

        active_chunk = &chunks.back();

        current_address =
            reinterpret_cast<uintptr_t>(active_chunk->data.get() + active_chunk->used);
        padding = (alignment - (current_address % alignment)) % alignment;
    }

    size_t aligned_offset = active_chunk->used + padding;
    active_chunk->used = aligned_offset + size;

    return active_chunk->data.get() + aligned_offset;
}

void ArenaAllocator::allocate_new_chunk(size_t min_size) {
    chunks.emplace_back(min_size);
}

void ArenaAllocator::reset() {
    if (chunks.empty())
        return;

    Chunk first = std::move(chunks.front());
    first.used = 0;

    chunks.clear();

    chunks.push_back(std::move(first));
}
