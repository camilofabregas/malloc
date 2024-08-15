#include "block.h"

arena_t small_arena = { .block_size = SMALL_BLOCK, .blocks = { NULL } };
arena_t medium_arena = { .block_size = MEDIUM_BLOCK, .blocks = { NULL } };
arena_t large_arena = { .block_size = LARGE_BLOCK, .blocks = { NULL } };

arena_t *
get_arena(size_t size)
{
	arena_t *arena = NULL;
	if (size + REGION_HEADER_SIZE <= SMALL_BLOCK) {
		arena = &small_arena;
	} else if (size + REGION_HEADER_SIZE <= MEDIUM_BLOCK) {
		arena = &medium_arena;
	} else if (size + REGION_HEADER_SIZE <= LARGE_BLOCK) {
		arena = &large_arena;
	}
	return arena;
}

// finds the next free region
// that holds the requested size
struct region *
find_free_region(size_t size)
{
	struct region *free_region = NULL;
	if (size + REGION_HEADER_SIZE <= SMALL_BLOCK) {
		free_region = search_strategy(size, small_arena.blocks);
	}
	if (!free_region && size + REGION_HEADER_SIZE <= MEDIUM_BLOCK) {
		free_region = search_strategy(size, medium_arena.blocks);
	}
	if (!free_region && size + REGION_HEADER_SIZE <= LARGE_BLOCK) {
		free_region = search_strategy(size, large_arena.blocks);
	}
	return free_region;
}

#ifdef FIRST_FIT
struct region *
search_strategy(size_t size, struct region **blocks)
{
	for (int i = 0; i < MAX_BLOCKS; i++) {
		struct region *region = blocks[i];
		while (region != NULL) {
			if (region->size >= size && region->free) {
				region->free = false;
				return region;
			}
			region = region->next;
		}
	}
	return NULL;
}
#endif

#ifdef BEST_FIT
struct region *
search_strategy(size_t size, struct region **blocks)
{
	struct region *best_region = NULL;

	for (int i = 0; i < MAX_BLOCKS; i++) {
		struct region *current_region = blocks[i];
		while (current_region != NULL) {
			// If the region can hold the size
			if (current_region->size >= size && current_region->free) {
				// If the region is a better fit than the actual one
				if (best_region == NULL ||
				    best_region->size > current_region->size) {
					best_region = current_region;
				}
			}
			current_region = current_region->next;
		}
	}
	// If there is a best_region, set free to false
	if (best_region != NULL) {
		best_region->free = false;
	}
	return best_region;
}
#endif

struct region *
create_block(size_t size)
{
	arena_t *arena = get_arena(size);
	block_size_t block_size = arena->block_size;
	struct region **blocks = arena->blocks;

	void *block =
	        mmap(NULL,
	             block_size,
	             PROT_READ | PROT_WRITE,  // Memory is readable and writable
	             MAP_PRIVATE |
	                     MAP_ANONYMOUS,  // Memory not backed by a file and anonymous to the process
	             -1,  // No fd used
	             0);  // Due to not using a fd, no need to use offset
	if (block == MAP_FAILED) {
		return NULL;
	}

	struct region *new_region = create_region(block, block_size, NULL, NULL);

	bool assigned = false;
	for (int i = 0; i < MAX_BLOCKS && !assigned; i++) {
		if (!blocks[i]) {
			blocks[i] = new_region;
			assigned = true;
		}
	}
	if (assigned == false) {
		munmap(block, block_size);
		return NULL;
	}
	return new_region;
}

struct region *
create_region(void *memory, size_t size, struct region *next, struct region *prev)
{
	struct region *new_region = (struct region *) memory;

	new_region->checksum = MAGIC_BYTES;
	new_region->size = size - REGION_HEADER_SIZE;
	new_region->next = next;
	new_region->prev = prev;
	new_region->free = true;

	return new_region;
}

void
splitting(struct region *node, size_t requested_size)
{
	// If the minimum new free region doesn't fit in the free space, do nothing
	if (node->size < requested_size + REGION_HEADER_SIZE + REGION_MIN_SIZE) {
		return;
	}

	// If the requested_size is less than the minimum, the actual node
	// must have the REGION_MIN_SIZE
	if (requested_size < REGION_MIN_SIZE) {
		requested_size = REGION_MIN_SIZE;
	}

	// Get the pointer to the empty region
	void *ptr_empty_region = REGION2PTR(node);
	ptr_empty_region += requested_size;

	// Create header metadata where the memory ends
	struct region *new_region = create_region(
	        ptr_empty_region, node->size - requested_size, node->next, node);

	// Update prev of next node if exists
	if (node->next != NULL) {
		node->next->prev = new_region;
	}
	node->next = new_region;  // Update next of current node
	node->size = requested_size;
}

struct region *
coalescing(struct region *node)
{
	if (node->next && node->next->free) {
		node = coalesce_regions(node, node->next);
	}
	if (node->prev && node->prev->free) {
		node = coalesce_regions(node->prev, node);
	}
	return node;
}

struct region *
coalesce_regions(struct region *left, struct region *right)
{
	left->size += right->size + REGION_HEADER_SIZE;
	left->next = right->next;
	if (right->next) {
		right->next->prev = left;
	}
	return left;
}

void
delete_block(struct region *region)
{
	if (region->prev || region->next)
		return;

	arena_t *arena = get_arena(region->size);
	block_size_t block_size = arena->block_size;
	struct region **blocks = arena->blocks;

	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i] == region) {
			blocks[i] = NULL;
			munmap(REGION2PTR(region), block_size);
			return;
		}
	}
}
