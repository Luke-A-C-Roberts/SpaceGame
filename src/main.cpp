#include "../headers/ldata.h"
#include "../headers/lpool.hpp"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <iterator>

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
