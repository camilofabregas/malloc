#define _DEFAULT_SOURCE

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "malloc.h"
#include "printfmt.h"

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;
int amount_of_blocks = 0;

/// Public API of malloc library ///

void *
malloc(size_t size)
{
	if (size + REGION_HEADER_SIZE > LARGE_BLOCK || size == 0)
		return NULL;

	struct region *region;

	size = ALIGN4(size);  // aligns to multiple of 4 bytes

	region = find_free_region(size);

	if (!region) {
		region = create_block(size);
		if (!region) {
			errno = ENOMEM;
			return NULL;
		}
		amount_of_blocks++;  // updates statistics
	}
	amount_of_mallocs++;  // updates statistics
	requested_memory += size;

	region->free = false;
	splitting(region, size);

	return REGION2PTR(region);
}

void
free(void *ptr)
{
	if (!ptr)
		return;

	struct region *region = PTR2REGION(ptr);
	if (region->checksum != MAGIC_BYTES)
		return;

	if (region->free == true)
		return;
	region->free = true;

	struct region *coalesced_region = coalescing(region);

	delete_block(coalesced_region);

	amount_of_frees++;  // updates statistics
}

void *
calloc(size_t nmemb, size_t size)
{
	if (nmemb == 0 || size == 0)
		return NULL;

	size_t total_size = nmemb * size;  // Calculate the total size needed

	if (nmemb != 0 &&
	    total_size / nmemb != size) {  // Check for integer overflow
		errno = ENOMEM;
		return NULL;
	}

	void *ptr = malloc(total_size);  // Call malloc to allocate memory
	if (ptr == NULL) {
		return NULL;
	}

	memset(ptr, 0, total_size);  // Initialize the allocated memory with zeros

	return ptr;
}

void *
realloc(void *ptr, size_t size)
{
	if (!ptr) {
		return malloc(size);
	} else if (size == 0) {
		free(ptr);
		return NULL;
	}

	struct region *region = PTR2REGION(ptr);
	size = ALIGN4(size);
	requested_memory -= region->size;
	if (region->checksum != MAGIC_BYTES)
		return NULL;

	if (size > region->size) {  // Get bigger region
		if (region->next && region->next->free &&
		    (region->size + region->next->size + REGION_HEADER_SIZE >=
		     size)) {  // Coalesce with right region
			region = coalesce_regions(region, region->next);
			splitting(region, size);

		} else if (region->prev && region->prev->free &&
		           (region->size + region->prev->size + REGION_HEADER_SIZE >=
		            size)) {  // Coalesce with left region
			region = coalesce_regions(region->prev, region);
			memcpy(REGION2PTR(region), ptr, size);
			splitting(region, size);
			region->free = false;

		} else {  // Find new region or create new block
			void *new_ptr = malloc(size);
			amount_of_mallocs--;
			if (!new_ptr) {
				errno = ENOMEM;
				return NULL;
			}
			memcpy(new_ptr, ptr, region->size);
			free(ptr);
			region = PTR2REGION(new_ptr);
		}

	} else if (size < region->size) {  // Shrink region
		splitting(region, size);
		if (region->next)
			coalescing(region->next);
	}
	// If it's the same size, return the same pointer
	requested_memory += size;
	return REGION2PTR(region);
}

void
get_stats(struct malloc_stats *stats)
{
	stats->mallocs = amount_of_mallocs;
	stats->frees = amount_of_frees;
	stats->requested_memory = requested_memory;
	stats->blocks = amount_of_blocks;
}
