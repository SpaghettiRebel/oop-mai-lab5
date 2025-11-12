#pragma once
#include <memory_resource>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <new>
#include <cassert>

class StaticVectorBlocks: public std::pmr::memory_resource {
public:
    explicit StaticVectorBlocks(std::size_t pool_size): pool_size(pool_size) {
        pool = ::operator new(pool_size);
        chunks.push_back({0, pool_size, true});
    }

    ~StaticVectorBlocks() override {
        ::operator delete(pool);
    }

private:
    struct Chunk { 
        std::size_t off; 
        std::size_t sz; 
        bool free; 
    };

    static std::uintptr_t align_up(std::uintptr_t p, std::size_t a) {
        return (p + (a - 1)) & ~(a - 1);
    }

    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (bytes == 0) {
            bytes = 1;
        }
        if (alignment == 0) {
            alignment = alignof(std::max_align_t);
        }

        std::uintptr_t base = reinterpret_cast<std::uintptr_t>(pool);
        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk &c = chunks[i];
            if (!c.free) continue;

            std::uintptr_t start = base + c.off;
            std::uintptr_t aligned = align_up(start, alignment);
            std::size_t pad = static_cast<std::size_t>(aligned - start);
            if (pad + bytes > c.sz) {
                continue;
            }

            std::vector<Chunk> add;
            if (pad > 0) {
                add.push_back({c.off, pad, true});
            }
            add.push_back({c.off + pad, bytes, false});
            std::size_t suffix = c.sz - (pad + bytes);
            if (suffix > 0) {
                add.push_back({c.off + pad + bytes, suffix, true});
            }

            chunks.erase(chunks.begin() + i);
            chunks.insert(chunks.begin() + i, add.begin(), add.end());
            
            return reinterpret_cast<void*>(aligned);
        }
        throw std::bad_alloc();
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        if (!p) {
            return;
        }

        std::uintptr_t base = reinterpret_cast<std::uintptr_t>(pool);
        std::uintptr_t ptr = reinterpret_cast<std::uintptr_t>(p);
        if (ptr < base || ptr >= base + pool_size) {
            assert(false && "передан неверный указатель");
            return;
        }
        std::size_t off = static_cast<std::size_t>(ptr - base);

        for (size_t i = 0; i < chunks.size(); ++i) {
            if (chunks[i].off == off) {
                chunks[i].free = true;

                if (i > 0 && chunks[i-1].free && chunks[i-1].off + chunks[i-1].sz == chunks[i].off) {
                    chunks[i - 1].sz += chunks[i].sz;
                    chunks.erase(chunks.begin() + i);
                    i -= 1;
                }

                if (i + 1 < chunks.size() && chunks[i+1].free && chunks[i].off + chunks[i].sz == chunks[i+1].off) {
                    chunks[i].sz += chunks[i + 1].sz;
                    chunks.erase(chunks.begin() + i + 1);
                }
                return;
            }
        }
        assert(false && "блок памяти не найден");
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    void* pool = nullptr;
    std::size_t pool_size = 0;
    std::vector<Chunk> chunks;
};