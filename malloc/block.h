#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>

#define MAGIC_BYTES 23072000
#define REGION_MIN_SIZE 256
#define REGION_HEADER_SIZE sizeof(struct region)
#define MAX_BLOCKS 50

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

typedef enum {
	SMALL_BLOCK = 16384,
	MEDIUM_BLOCK = 1048576,
	LARGE_BLOCK = 33554432
} block_size_t;

struct region {
	int checksum;
	bool free;
	size_t size;
	struct region *next;
	struct region *prev;
};

typedef struct arena {
	block_size_t block_size;
	struct region *blocks[MAX_BLOCKS];
} arena_t;

arena_t *get_arena(size_t size);

struct region *find_free_region(size_t size);

struct region *search_strategy(size_t size, struct region **blocks);

struct region *create_block(size_t size);

struct region *
create_region(void *memory, size_t size, struct region *next, struct region *prev);

void splitting(struct region *node, size_t used_size);

struct region *coalescing(struct region *node);

struct region *coalesce_regions(struct region *left, struct region *right);

void delete_block(struct region *region);

#endif  // _BLOCK_H_
