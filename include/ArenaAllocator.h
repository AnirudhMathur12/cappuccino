#ifndef ARENAALLOCATOR_H_
#define ARENAALLOCATOR_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class ArenaAllocator {
    struct Chunk {
        std::unique_ptr<uint8_t[]> data;

        size_t capacity;
        size_t used;

        explicit Chunk(size_t size)
            : data(std::make_unique<uint8_t[]>(size)), capacity(size), used(0) {}
    };

    std::vector<Chunk> chunks;
    size_t default_chunk_size;

    void allocate_new_chunk(size_t min_size);

  public:
    explicit ArenaAllocator(size_t chunk_size = 64 * 1024); // 64 KB
    ~ArenaAllocator() = default;

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    template <typename T, typename... Args> T* make(Args&&... args) {
        void* memory = allocate(sizeof(T), alignof(T));
        return new (memory) T(std::forward<Args>(args)...);
    }

    void reset();
};

#endif
