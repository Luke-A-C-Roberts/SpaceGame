// Based on http://dmitrysoshnikov.com/compilers/writing-a-pool-allocator/
#ifndef LPOOL_HPP
#define LPOOL_HPP

#include <cstddef> 
#include <cstdlib>
#include <cstdio>
#include "ldata.h"

namespace llib {
    enum LPoolLogLevel {
        LPOOL_NO_LOG,
        LPOOL_SHOW_ALLOCATIONS
    };

    static LPoolLogLevel lpool_log_level = LPOOL_NO_LOG;

    /*
     * The chunk class, a linked list that points to the next chunk, whether that
     * be in or outside of its containing pool allocator block.
     */
    struct Chunk {
        /*
         * When a chunk is free, the `next` contains the
         * address of the next chunk in a list.
         *
         * When it's allocated, this space is used by
         * the user.
         */
        struct Chunk *next;
    };

    /*
     * The allocator class.
     *
     * Features:
     *
     *   - Parametrized by number of chunks per block
     *   - Keeps track of the allocation pointer
     *   - Bump-allocates chunks
     *   - Requires a new larger block when needed
     *
     */
    template <class T> struct PoolAllocator {   
        PoolAllocator(void) = delete;
        PoolAllocator operator=(PoolAllocator&) = delete;
        
        /*
         * Initialises the pool allocator's fields.
         *
         * - chunks_per_block is the number of items of type T found in an allocated block
         * - max_blocks is the number of blocks available from the allocator
         */
        PoolAllocator(usize const chunks_per_block, usize const max_blocks) {
            m_chunks_per_block = chunks_per_block;
            m_max_blocks = max_blocks;
            m_current_block_index = 0;
            m_blocks = reinterpret_cast<Chunk **>(
                std::calloc(m_max_blocks, sizeof(ptr))
            );

            if (lpool_log_level) (void)std::fprintf(
                stderr,
                "Allocating Block Pointer Array: %p\n",
                (void *)m_blocks
            );

            m_alloc = nullptr;
        }

        /*
         * Frees all of the blocks allocated for the pool allocator, along with the
         * array that keeps track of their pointers.
         */
        ~PoolAllocator(void) {
            for (usize i = 0; i < m_current_block_index; ++i) {
                if (lpool_log_level) (void)std::fprintf(
                    stderr,
                    "Freeing Block: %p\n",
                    (void *)m_blocks[i]
                );

                std::free(m_blocks[i]);
            }
            std::free(m_blocks);
        }

        /*
         * Returns the first free chunk in the block.
         * If there are no chunks left in the block, allocates a new block.
         */
        auto allocate(void) -> T* {

            // No chunks left in the current block, or no any block exists yet. 
            // Allocate a new one, passing the chunk size:
            if (m_alloc == nullptr) {
                m_alloc = _allocate_block();
                if (lpool_log_level) (void)std::fprintf(
                    stderr,
                    "Allocating Block: %p\n",
                    (void *)m_alloc
                );
            }

            // The return value is the current position of the allocation pointer:
            Chunk *const free_chunk = m_alloc;

            // Advance (bump) the allocation pointer to the next chunk.
            // When no chunks left the `m_alloc` will be set to `nullptr`, and this will
            // cause allocation of a new block on the next request:
            m_alloc = m_alloc->next;

            if (lpool_log_level) (void)std::fprintf(
                stderr,
                "Mutator Code Allocated: %p\n",
                (void *)free_chunk
            );
            
            return reinterpret_cast<T *>(free_chunk); 
        }

        /*
         * Frees a chunk in the pool allocator within a block.
         */
        void deallocate(void *const chunk) { 
            if (lpool_log_level) (void)std::fprintf(
                stderr,
                "Mutator Code Deallocated: %p\n",
                (void *)chunk
            );
            
            // The freed chunk's next pointer points to the current allocation pointer:
            (reinterpret_cast<Chunk *const>(chunk))->next = m_alloc;

            // And the allocation pointer is now set to the returned (free) chunk:
            m_alloc = (Chunk *)(chunk); 
        }

        // Size of a chunk (in bytes)
        auto chunk_size(void) -> usize { return m_chunk_size; }

        // Max number of blocks for the allocator
        auto max_blocks(void) -> usize { return m_max_blocks; }

        // Num chunks per larger block.
        auto chunks_per_block(void) -> usize { return m_chunks_per_block; }
    
    private:
        /*
         * Allocates a new block from the OS.
         * 
         * Returns a Chunk pointer set to the beginning of a block.
         * If the function returns `nullptr` then the max number of blocks is reached
         */
        auto _allocate_block(void) -> Chunk* {
            // If max blocks are reached, don't add more just return `nullptr`.
            if (m_current_block_index >= m_max_blocks) return nullptr;    

            // The first chunk of the new block.
            usize const block_size = m_chunks_per_block * m_chunk_size;

            // Once the block is allocated, we need to chain all the chunks in this block.
            Chunk *const block_begin = reinterpret_cast<Chunk *>(
                std::malloc(block_size)
            );

            if (block_begin == nullptr) return nullptr;

            // Add the next block index to the array of block pointers.
            m_blocks[m_current_block_index++] = block_begin;

            Chunk *chunk = block_begin;

            for (usize i = 0; i < m_chunks_per_block - 1; ++i) {
                chunk->next = reinterpret_cast<Chunk *>(
                    reinterpret_cast<char *>(chunk) + m_chunk_size
                );
                chunk = chunk->next;
            }

            chunk->next = nullptr;
            return block_begin; 
        }

        // Size of a chunk (in bytes)
        usize const m_chunk_size = sizeof(T);

        // Num chunks per larger block.
        usize m_chunks_per_block;

        // Max number of blocks for the allocator
        usize m_max_blocks;

        // Current number of blocks
        usize m_current_block_index;

        // The block pointers in a dynamically allocated array
        Chunk **m_blocks;

        // Allocation pointer.
        Chunk *m_alloc;
    };    
}

#endif
