Log 1 / Asset management using a pool allocator
===============================================

## Note about data types in C/C++ for this project

C and C++'s default primative type systems can be messy especially the integers. Depending on the platform `int` could refer to a range of bit widths. Luckily the types defined in `stdint.h`/`cstdint` have a fixed width, making them less ambigous than default integer types, for example `uint32_t` will always be a 32-bit integer type. To make the code a bit more readble, I've put together a header file `ldata.h` that shortens some of these names, making them reminicient of modern languages like Zig and Rust. For simplicity, all primative types (exept `void`) will use these type aliases to make the code more readable. In cases where I need to work with an external C API I will also use the normal type system.

## Note about style

I am quite pedantic about my own coding style for unknown reasons to myself. Generally I dislike overuse of OOP features and prefer a simple imperitive/functional style for my own code. That being said I won't pretend that sometines OOP can't be the best solution to a problem so I will be using it throughout this project.

## A pool allocator

Being able to manage memory in a game can be tricky, especially in unmanaged languages like C or C++. One common misstep is to overuse C's `malloc`/`free` or C++ vectors to hold large amounts of dynamicly stored structures in a game, because frequent allocation and deallocation can lead to tanking performance. In addition, memory leaks can corrupt or crash the game so I want to avoid that at all costs. A way of storing large ammounts of smaller structs (e.g. enemy data) is the pool allocator, which has a typical allocation/ deallocation complexity of O(1). A lovely article on pool allocation (for which I based my own implementation) is [Writing a Pool Allocator](http://dmitrysoshnikov.com/compilers/writing-a-pool-allocator/). The article explains the implementation better than I ever could, so give it a read if you have a minute.

The basic Idea of a pool allocator is that all unallocated chunks of data are in a linked list, so one simply needs to ask the pool allocator for the item at the end of the list to allocate it. When the program is done using a chunk, the pool allocator simply appends it to the linked list as free again. All of these chunks are contained in blocks of equal size, so if the pool allocator runs out of new chunks, a fresh block is allocated and all of its chunks are appended to the linked list.

Although the implementation in the article is basically complete, I thought it could do with a few additional features, namely:

 - Deallocation of blocks when the entire allocator is nolonger needed

 - A maximum number of blocks that can't be exceeded

 - Templating for a specific type contained in chunks reducing API complexity

Templating was achieved rather easily, so the width of chunks is predefined. I simply made a private constant variable for chunk width by calling the preprocessor function `sizeof` onto the templated type, so that it's width is known to the pool allocator object.

```c++
template <class T> struct PoolAllocator {
// ...
    // Size of a chunk (in bytes)
    usize const m_chunk_size = sizeof(T);
// ...
};
  
```

Deallocation of blocks after use and max block width were both solved at the same time by allocating a array of pointers to each block, with a length of the maximum number of blocks. The latest block to be allocated is tracked by an index for the array. 

```c++ 
template <class T> struct PoolAllocator {
// ...
    // Current number of blocks
    usize m_current_block_index;

    // The block pointers in a dynamically allocated array
    Chunk **m_blocks;
// ...
};
```

Every block pointer is recorded at the array position corresponding to that index. The index then increments to potentially record the next block, up to the max number of blocks.
 
If the max number of blocks is reached, then allocating a new block will just result in `nullptr` being given. Chunk allocation will also return `nullptr`, if no more blocks can be allocated, so its up to the mutator code to handle such an event.

```c++ 
template <class T> struct PoolAllocator {
// ...

    auto _allocate_block(void) -> Chunk* {
        // If max blocks are reached, don't add more just return `nullptr`.
        if (m_current_block_index >= m_max_blocks) return nullptr;    

// ...

    }

// ...
};

```

When the pool allocator is deconstructed, the index is used as the invarient of a for loop, so each block is freed using it's pointer in the array.

```c++
template <class T> struct PoolAllocator {   
// ...

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

// ...
};
```

## Testing

I used the same structure as the article for testing to see if my allocator works correctly. To organise my code, I'm keeping all of my utilities in a namespace `llib` for use in the project, and for demonstration I have a static variable `lpool_log_level` which can be set an enum value to show allocator operations. The test code creates a pool allocator with 2 blocks containing 8 chunks each, allocates 10 items and the deallocates the same 10 items.

```c++ 
// Test code

##include "../headers/ldata.h"
##include "../headers/lpool.hpp"
##include <cstdio>
##include <cstdlib>
##include <algorithm>
##include <array>
##include <iterator>

namespace example {
    // An example item that could be allocated in a game
    struct Item {
        Item(i32 const x, i32 const y, f64 const speed):
            x(x), y(y), speed(speed) {}
        
        i32 x;
        i32 y;
        f64 speed;
    };
}

auto main(void) -> int {
    // Show allocations in debug
    llib::lpool_log_level = llib::LPOOL_SHOW_ALLOCATIONS;

    // Create pool allocator with 2 blocks containing 8 items each, of type Item
    llib::PoolAllocator<example::Item> pallocator(8, 2);

    // Array of pointers to allocated memory in the pool allocator, set to null for now
    std::array<example::Item *, 10> example_items;
    std::fill(example_items.begin(), example_items.end(), nullptr);

    // Go forward through the array and ask the pallocator to allocate a space for an item
    std::for_each(
        example_items.begin(),
        example_items.end(),
        [&pallocator](example::Item*& item)
    {
        item = pallocator.allocate();
    });

    // Using memory and printing example item
    *example_items[0] = example::Item(10, 10, 0.5);
    (void)std::fprintf(
        stderr,
        "Item (%d, %d, %lf): %p\n",
        example_items[0]->x,
        example_items[0]->y,
        example_items[0]->speed,
        (void *)example_items[0]
    );

    // Go backwards through the array and deallocate the items previously allocated
    std::for_each(
        example_items.rbegin(),
        example_items.rend(),
        [&pallocator](example::Item* item)
    { 
        pallocator.deallocate(item);
    });

    // pallocator implicitly deallocates blocks
 
    return EXIT_SUCCESS;
}
```

Output:
```
Allocating Block Pointer Array: 0x55ec814ee2a0
Allocating Block: 0x55ec814ee2c0
Mutator Code Allocated: 0x55ec814ee2c0
Mutator Code Allocated: 0x55ec814ee2d0
Mutator Code Allocated: 0x55ec814ee2e0
Mutator Code Allocated: 0x55ec814ee2f0
Mutator Code Allocated: 0x55ec814ee300
Mutator Code Allocated: 0x55ec814ee310
Mutator Code Allocated: 0x55ec814ee320
Mutator Code Allocated: 0x55ec814ee330
Allocating Block: 0x55ec814ee350
Mutator Code Allocated: 0x55ec814ee350
Mutator Code Allocated: 0x55ec814ee360
Item (10, 10, 0.500000): 0x55ec814ee2c0
Mutator Code Deallocated: 0x55ec814ee360
Mutator Code Deallocated: 0x55ec814ee350
Mutator Code Deallocated: 0x55ec814ee330
Mutator Code Deallocated: 0x55ec814ee320
Mutator Code Deallocated: 0x55ec814ee310
Mutator Code Deallocated: 0x55ec814ee300
Mutator Code Deallocated: 0x55ec814ee2f0
Mutator Code Deallocated: 0x55ec814ee2e0
Mutator Code Deallocated: 0x55ec814ee2d0
Mutator Code Deallocated: 0x55ec814ee2c0
Freeing Block: 0x55ec814ee2c0
Freeing Block: 0x55ec814ee350 
```

Full Implementation:
```c++
// Based on http://dmitrysoshnikov.com/compilers/writing-a-pool-allocator/
##ifndef LPOOL_HPP
##define LPOOL_HPP

##include <cstddef> 
##include <cstdlib>
##include <cstdio>
##include "ldata.h"

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

##endif

```
