#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "testlib.h"
#include "malloc.h"

// TEST UTILS //

int count_regions(struct region *block);

int
count_regions(struct region *block)
{
	int regions = 1;
	struct region *next_region = block->next;
	while (next_region != NULL) {
		regions++;
		next_region = next_region->next;
	}
	return regions;
}

// PUBLIC API TESTS //

static void
successful_malloc_returns_non_null_pointer(void)
{
	char *var = malloc(100);

	ASSERT_TRUE("TEST 1: successful malloc returns non-null pointer",
	            var != NULL);

	free(var);
}

static void
amount_of_mallocs_after_successful_malloc_is_one(void)
{
	struct malloc_stats stats;
	char *var = malloc(100);

	get_stats(&stats);

	ASSERT_TRUE("TEST 2: amount of mallocs after successful malloc is one",
	            stats.mallocs == 1);

	free(var);
}

static void
amount_of_frees_after_successful_malloc_is_one(void)
{
	struct malloc_stats stats;
	char *var = malloc(100);
	free(var);

	get_stats(&stats);

	ASSERT_TRUE("TEST 3: amount of frees after successful malloc is one",
	            stats.frees == 1);
}

static void
correct_amount_of_requested_memory_after_successful_malloc(void)
{
	struct malloc_stats stats;
	char *var = malloc(100);
	free(var);

	get_stats(&stats);

	ASSERT_TRUE("TEST 4: amount of requested memory after successful "
	            "malloc(100) is 100",
	            stats.requested_memory == 100);
}

static void
string_in_successful_malloc_is_correct(void)
{
	char *test_string = "FISOP malloc is working!";
	char *var = malloc(100);
	strcpy(var, test_string);

	ASSERT_TRUE(
	        "TEST 5: string in succesful malloc contains the copied value",
	        strcmp(var, test_string) == 0);

	free(var);
}

static void
multiple_mallocs_generate_correct_amount_of_regions(void)
{
	void *region1 = malloc(100);
	void *region2 = malloc(100);
	void *region3 = malloc(100);
	void *region4 = malloc(100);
	void *region5 = malloc(100);
	void *region6 = malloc(100);

	ASSERT_TRUE(
	        "TEST 6: multiple mallocs generate correct amount of regions",
	        count_regions(PTR2REGION(region1)) == 7);

	free(region1);
	free(region2);
	free(region3);
	free(region4);
	free(region5);
	free(region6);
}

static void
multiple_mallocs_generate_correct_amount_of_blocks(void)
{
	struct malloc_stats stats;
	void *small_block1 = malloc(SMALL_BLOCK - REGION_HEADER_SIZE);
	void *small_block2 = malloc(SMALL_BLOCK - REGION_HEADER_SIZE);
	void *medium_block1 = malloc(MEDIUM_BLOCK - REGION_HEADER_SIZE);
	void *medium_block2 = malloc(MEDIUM_BLOCK - REGION_HEADER_SIZE);
	void *large_block1 = malloc(LARGE_BLOCK - REGION_HEADER_SIZE);
	void *large_block2 = malloc(LARGE_BLOCK - REGION_HEADER_SIZE);

	get_stats(&stats);

	ASSERT_TRUE(
	        "TEST 7: multiple mallocs generate correct amount of blocks",
	        stats.blocks == 6);

	free(small_block1);
	free(small_block2);
	free(medium_block1);
	free(medium_block2);
	free(large_block1);
	free(large_block2);
}

static void
region_limits_are_set_correctly(void)
{
	char *test_string =
	        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed "
	        "non placerat elit. Donec a cursus dui. Integer sed enim ac "
	        "est rutrum ultrices. Fusce sollicitudin tincidunt "
	        "pellentesque. Pellentesque at est gravida, commodo velit ut, "
	        "finibus nulla. In hac lectus.";
	size_t test_string_size = (strlen(test_string) + 1) * sizeof(char);
	char *region1 = malloc(test_string_size);
	strcpy(region1, test_string);
	char *region2 = malloc(test_string_size);
	strcpy(region2, test_string);

	ASSERT_TRUE("TEST 8: region limits are set correctly (contents don't "
	            "overlap)",
	            strcmp(region1, region2) == 0);


	free(region1);
	free(region2);
}

static void
calloc_set_memory_to_zero(void)
{
	int size = 10;
	int *testing = (int *) calloc(10, sizeof(int));
	bool expected = true;

	for (int i = 0; i < size; i++) {
		if (testing[i] != 0) {
			expected = false;
			break;
		}
	}

	ASSERT_TRUE("TEST 9: calloc set memory to zero", expected);

	free(testing);
}

static void
realloc_of_null_pointer_equals_malloc(void)
{
	struct malloc_stats stats;

	void *var = realloc(NULL, 100);

	get_stats(&stats);

	ASSERT_TRUE("TEST 10: realloc of null pointer equals malloc",
	            stats.mallocs == 1);

	free(var);
}

static void
realloc_of_size_zero_equals_free(void)
{
	struct malloc_stats stats;

	void *var1 = malloc(100);
	void *var2 = realloc(var1, 0);

	get_stats(&stats);

	ASSERT_TRUE("TEST 11: realloc of size zero equals free",
	            stats.frees == 1 && var2 == NULL);
}

static void
realloc_of_bigger_size_coalesces_right_region(void)
{
	void *var1 = malloc(3000);
	void *var2 = malloc(3000);
	void *var3 = malloc(3000);
	void *var4 = malloc(3000);
	free(var3);
	void *var5 = realloc(var2, 3500);
	struct region *region1 = PTR2REGION(var1);
	struct region *region5 = PTR2REGION(var5);

	ASSERT_TRUE("TEST 12: realloc of bigger size coalesces right region",
	            count_regions(region1) == 5 && region5->size == 3500 &&
	                    region5->free == false && region5->next->size == 2500 &&
	                    region5->next->free == true);

	free(var1);
	free(var4);
	free(var5);
}

static void
realloc_of_bigger_size_coalesces_left_region(void)
{
	void *var1 = malloc(3000);
	void *var2 = malloc(3000);
	void *var3 = malloc(3000);
	void *var4 = malloc(3000);
	free(var2);
	void *var5 = realloc(var3, 3500);
	struct region *region1 = PTR2REGION(var1);
	struct region *region5 = PTR2REGION(var5);

	ASSERT_TRUE("TEST 13: realloc of bigger size coalesces left region",
	            count_regions(region1) == 5 && region5->size == 3500 &&
	                    region5->free == false && region5->next->size == 2500 &&
	                    region5->next->free == true);

	free(var1);
	free(var4);
	free(var5);
}

static void
realloc_of_bigger_size_finds_new_region(void)
{
	void *var1 = malloc(2000);
	void *var2 = malloc(2000);
	void *var3 = malloc(2000);
	void *var4 = malloc(2000);
	void *var5 = realloc(var2, 4000);
	struct region *region1 = PTR2REGION(var1);
	struct region *region2 = PTR2REGION(var2);
	struct region *region5 = PTR2REGION(var5);

	ASSERT_TRUE("TEST 14: realloc of bigger size finds new region",
	            count_regions(region1) == 6 && region5->size == 4000 &&
	                    region2->free == true);

	free(var1);
	free(var3);
	free(var4);
	free(var5);
}

static void
realloc_of_bigger_size_creates_new_block(void)
{
	struct malloc_stats stats;

	void *var1 = malloc(4000);
	void *var2 = malloc(4000);
	void *var3 = malloc(3000);
	void *var4 = malloc(3000);         // Block 1 is full
	void *var5 = realloc(var2, 8000);  // Should create new block
	struct region *region1 = PTR2REGION(var1);
	struct region *region2 = PTR2REGION(var2);
	struct region *region5 = PTR2REGION(var5);

	get_stats(&stats);

	ASSERT_TRUE("TEST 15: realloc of bigger size creates new block",
	            stats.blocks == 2 && region5->size == 8000 &&
	                    region2->free == true && count_regions(region1) == 5 &&
	                    count_regions(region5) == 2);

	free(var1);
	free(var3);
	free(var4);
	free(var5);
}

static void
realloc_of_smaller_size_shrinks_region(void)
{
	void *var1 = malloc(1000);
	void *var2 = realloc(var1, 500);
	struct region *region1 = PTR2REGION(var1);
	struct region *region2 = PTR2REGION(var2);

	ASSERT_TRUE("TEST 16: realloc of smaller size shrinks region",
	            region2->size == 500 && region1 == region2);
	ASSERT_TRUE("TEST 17: realloc of smaller size reuses the unused space",
	            count_regions(region2) == 2 &&
	                    region2->next->size ==
	                            SMALL_BLOCK - 2 * REGION_HEADER_SIZE - 500);

	free(var2);
}

static void
realloc_of_smaller_size_doesnt_split_if_theres_not_enough_space(void)
{
	void *var1 = malloc(1000);
	void *var2 = realloc(var1, 900);
	struct region *region2 = PTR2REGION(var2);

	ASSERT_TRUE("TEST 18: realloc of smaller size doesn't split if there's "
	            "not enough space",
	            count_regions(region2) == 2 &&
	                    region2->next->size ==
	                            SMALL_BLOCK - 2 * REGION_HEADER_SIZE - 1000);

	free(var2);
}

static void
realloc_of_same_size_returns_same_pointer(void)
{
	void *var1 = malloc(100);
	void *var2 = realloc(var1, 100);

	ASSERT_TRUE("TEST 19: realloc of same size returns same pointer",
	            var1 == var2);

	free(var1);
}

// ERROR TESTS //

static void
malloc_bigger_than_biggest_block_returns_null_pointer(void)
{
	char *var = malloc(LARGE_BLOCK + 1);

	ASSERT_TRUE("TEST 20: malloc bigger than biggest block returns null "
	            "pointer",
	            var == NULL);

	free(var);
}

static void
malloc_of_size_zero_returns_null_pointer(void)
{
	char *var = malloc(0);

	ASSERT_TRUE("TEST 21: malloc of size zero returns null pointer",
	            var == NULL);

	free(var);
}

static void
amount_of_mallocs_after_malloc_bigger_than_biggest_block_is_zero(void)
{
	struct malloc_stats stats;
	char *var = malloc(LARGE_BLOCK + 1);

	get_stats(&stats);

	ASSERT_TRUE("TEST 22: amount of mallocs after malloc bigger than "
	            "biggest block is zero",
	            stats.mallocs == 0);

	free(var);
}

static void
amount_of_frees_after_unsuccessful_malloc_is_zero(void)
{
	struct malloc_stats stats;
	char *var = malloc(LARGE_BLOCK + 1);
	free(var);

	get_stats(&stats);

	ASSERT_TRUE("TEST 23: amount of frees after unsuccessful malloc is one",
	            stats.frees == 0);
}

static void
amount_of_requested_memory_after_unsuccessful_malloc_is_zero(void)
{
	struct malloc_stats stats;
	char *var = malloc(LARGE_BLOCK + 1);
	free(var);

	get_stats(&stats);

	ASSERT_TRUE("TEST 24: amount of requested memory after unsuccessful "
	            "malloc is 0",
	            stats.requested_memory == 0);
}

static void
freeing_null_pointer_does_nothing(void)
{
	struct malloc_stats stats;
	free(NULL);

	get_stats(&stats);

	ASSERT_TRUE("TEST 25: freeing a null pointer does nothing, amount of "
	            "frees is 0",
	            stats.frees == 0);
}

static void
calloc_of_nmemb_or_size_zero_returns_null_pointer(void)
{
	void *var1 = calloc(0, 8);
	void *var2 = calloc(8, 0);

	ASSERT_TRUE(
	        "TEST 26: calloc of nmemb or size zero returns null pointer",
	        var1 == NULL && var2 == NULL);
}

/*
static void
freeing_pointer_that_wasnt_malloced_does_nothing(void)
{
    struct malloc_stats stats;

    void *var1 = malloc(1000);
    void *wrong_pointer = var1 + 200;
    free(wrong_pointer);

    get_stats(&stats);

    ASSERT_TRUE("TEST 27: freeing a pointer that wasn't malloc'd (wrong "
                "checksum) does nothing",
                stats.frees == 0);

    free(var1);
}

static void
realloc_pointer_that_wasnt_malloced_returns_null(void)
{
    void *var1 = malloc(1000);
    void *wrong_pointer = var1 + 200;
    void *var2 = realloc(wrong_pointer, 2000);
    struct region *region1 = PTR2REGION(var1);

    ASSERT_TRUE("TEST 28: realloc pointer that wasn't malloc'd (wrong "
                "checksum) returns null",
                var2 == NULL && region1->size == 1000);

    free(var1);
}
*/

// INTERNAL FUNCTIONS TESTS //

static void
block_starts_with_1_region(void)
{
	struct region *test_block = create_block(1000);  // Creates small block.

	ASSERT_TRUE(
	        "\nTEST 29: successful block creation returns non-null pointer",
	        test_block != NULL);
	ASSERT_TRUE("TEST 29: initial region of successful block creation has "
	            "size of block type - header",
	            test_block->size == SMALL_BLOCK - REGION_HEADER_SIZE);
	ASSERT_TRUE("TEST 29: initial region of successful block creation has "
	            "free state",
	            test_block->free == true);

	free(test_block);
}

static void
find_free_region_with_initial_block_no_splitting(void)
{
	struct region *test_block = create_block(1000);  // Creates small block.
	struct region *free_region = find_free_region(2000);

	ASSERT_TRUE("\nTEST 30: successfully finds free region in free initial "
	            "block",
	            free_region != NULL);
	ASSERT_TRUE("TEST 30: the found region is the whole block",
	            free_region == test_block);
	ASSERT_TRUE("TEST 30: the found region has the full block size",
	            free_region->size == SMALL_BLOCK - REGION_HEADER_SIZE);

	free(free_region);
	free(test_block);
}

static void
find_free_region_with_initial_block_with_splitting(void)
{
	struct region *test_block = create_block(1000);  // Creates small block.
	splitting(test_block, 1000);  // Splits the block in 2 regions.
	struct region *free_region = find_free_region(2000);

	ASSERT_TRUE("\nTEST 31: successfully finds free region in block "
	            "previously splitted",
	            free_region != NULL);
	ASSERT_TRUE("TEST 31: the first region has the requested size",
	            test_block->size == 1000);
	ASSERT_TRUE(
	        "TEST 31: the free region has the remaining space of the block",
	        free_region->size == SMALL_BLOCK - 1000 - 2 * REGION_HEADER_SIZE);

	free(free_region);
	free(test_block);
}

static void
splitting_block_with_size_of_full_block_doesnt_split(void)
{
	struct region *test_block = create_block(
	        SMALL_BLOCK -
	        REGION_HEADER_SIZE);  // Creates small block with region size of whole block.
	splitting(test_block, SMALL_BLOCK - REGION_HEADER_SIZE);  // Shouldnt split

	ASSERT_TRUE("\nTEST 32: block with no remaining space has only one "
	            "region after trying to split",
	            count_regions(test_block) == 1);
	ASSERT_TRUE("TEST 32: first region of the block has no next",
	            test_block->next == NULL);

	free(test_block);
}

static void
successful_split_with_initial_block(void)
{
	printfmt("\n");
	size_t testing_size[] = { 10, 20, 280, 400 };

	for (size_t i = 0; i < sizeof(testing_size) / sizeof(size_t); i++) {
		char first_message[100];
		char second_message[100];


		struct region *test_block =
		        create_block(SMALL_BLOCK - REGION_HEADER_SIZE);
		splitting(test_block,
		          SMALL_BLOCK - REGION_HEADER_SIZE);  // Shouldn't split.
		struct region *free_region = find_free_region(testing_size[i]);
		splitting(free_region, testing_size[i]);  // Splits in 2 regions.

		sprintf(first_message,
		        "TEST 33: successfully splits block in 2 regions with "
		        "size %zu",
		        testing_size[i]);
		sprintf(second_message,
		        "TEST 33: successfully assign region after split with "
		        "size %zu",
		        testing_size[i]);
		ASSERT_TRUE(first_message, 2 == count_regions(free_region));
		ASSERT_TRUE(second_message,
		            (testing_size[i] > REGION_MIN_SIZE)
		                    ? testing_size[i] == free_region->size
		                    : REGION_MIN_SIZE == free_region->size);

		free(free_region);
		free(test_block);
	}
}

static void
successful_split_with_multiple_regions(void)
{
	size_t testing_size[] = { 10, 20, 280, 400 };
	int expected_size = sizeof(testing_size) / sizeof(size_t) + 1;
	struct region *test_block =
	        create_block(SMALL_BLOCK - REGION_HEADER_SIZE);

	for (size_t i = 0; i < sizeof(testing_size) / sizeof(size_t); i++) {
		struct region *free_region = find_free_region(testing_size[i]);
		splitting(free_region, testing_size[i]);
	}

	ASSERT_TRUE("\nTEST 34: successfully split multiple regions",
	            expected_size == count_regions(test_block));

	struct region *current = test_block;
	struct region *next = current->next;
	while (next) {
		next = current->next;
		free(current);
		current = next;
	}
}

static void
successful_coalesce_with_2_regions(void)
{
	struct region *test_block =
	        create_block(SMALL_BLOCK - REGION_HEADER_SIZE);
	struct region *free_region = find_free_region(10);
	splitting(free_region, 10);

	ASSERT_TRUE("\nTEST 35: successfully splitted into 2 regions",
	            2 == count_regions(test_block));

	coalescing(free_region);

	ASSERT_TRUE("TEST 35: successfully coalesce 2 regions into 1",
	            1 == count_regions(test_block));

	free(test_block);
}

static void
successful_coalesce_with_3_regions(void)
{
	struct region *test_block =
	        create_block(SMALL_BLOCK - REGION_HEADER_SIZE);
	struct region *free_region1 = find_free_region(10);
	splitting(free_region1, 10);
	struct region *free_region2 = find_free_region(1000);
	splitting(free_region2, 1000);

	ASSERT_TRUE("\nTEST 36: successfully splitted into 3 regions",
	            3 == count_regions(test_block));

	free_region1->free = true;
	coalescing(free_region2);

	ASSERT_TRUE("TEST 36: successfully coalesce 3 regions into 1",
	            1 == count_regions(test_block));

	free(test_block);
}

static void
successful_coalesce_with_multiple_regions(void)
{
	size_t testing_size[] = { 10, 20, 280, 400 };
	struct region *test_block =
	        create_block(SMALL_BLOCK - REGION_HEADER_SIZE);

	struct region *free_region1 = find_free_region(testing_size[0]);
	splitting(free_region1, testing_size[0]);
	struct region *free_region2 = find_free_region(testing_size[1]);
	splitting(free_region2, testing_size[1]);
	struct region *free_region3 = find_free_region(testing_size[2]);
	splitting(free_region3, testing_size[2]);
	struct region *free_region4 = find_free_region(testing_size[3]);
	splitting(free_region4, testing_size[3]);

	ASSERT_TRUE("\nTEST 37: successfully splitted into 5 regions",
	            5 == count_regions(test_block));

	free_region1->free = true;
	free_region3->free = true;

	coalescing(free_region2);
	ASSERT_TRUE("TEST 37: successfully coalesced regions 1, 2, 3",
	            3 == count_regions(test_block));

	coalescing(free_region4);
	ASSERT_TRUE("TEST 37: successfully coalesced regions 1 (prev. 1, 2, "
	            "3), 4 and free space",
	            1 == count_regions(test_block));

	free(test_block);
}

int
main(void)
{
	printfmt("\nPUBLIC API TESTS:\n");
	run_test(successful_malloc_returns_non_null_pointer);
	run_test(amount_of_mallocs_after_successful_malloc_is_one);
	run_test(amount_of_frees_after_successful_malloc_is_one);
	run_test(correct_amount_of_requested_memory_after_successful_malloc);
	run_test(string_in_successful_malloc_is_correct);
	run_test(multiple_mallocs_generate_correct_amount_of_regions);
	run_test(multiple_mallocs_generate_correct_amount_of_blocks);
	run_test(region_limits_are_set_correctly);
	run_test(calloc_set_memory_to_zero);
	run_test(realloc_of_null_pointer_equals_malloc);
	run_test(realloc_of_size_zero_equals_free);
	run_test(realloc_of_bigger_size_coalesces_right_region);
	run_test(realloc_of_bigger_size_coalesces_left_region);
	run_test(realloc_of_bigger_size_finds_new_region);
	run_test(realloc_of_bigger_size_creates_new_block);
	run_test(realloc_of_smaller_size_shrinks_region);
	run_test(realloc_of_smaller_size_doesnt_split_if_theres_not_enough_space);
	run_test(realloc_of_same_size_returns_same_pointer);

	printfmt("\nERROR TESTS:\n");
	run_test(malloc_bigger_than_biggest_block_returns_null_pointer);
	run_test(malloc_of_size_zero_returns_null_pointer);
	run_test(amount_of_mallocs_after_malloc_bigger_than_biggest_block_is_zero);
	run_test(amount_of_frees_after_unsuccessful_malloc_is_zero);
	run_test(amount_of_requested_memory_after_unsuccessful_malloc_is_zero);
	run_test(freeing_null_pointer_does_nothing);
	run_test(calloc_of_nmemb_or_size_zero_returns_null_pointer);
	// Tests with warnings for using wrong pointers with offset
	// run_test(freeing_pointer_that_wasnt_malloced_does_nothing);
	// run_test(realloc_pointer_that_wasnt_malloced_returns_null);

	printfmt("\nINTERNAL FUNCTIONS TESTS:");
	run_test(block_starts_with_1_region);
	run_test(find_free_region_with_initial_block_no_splitting);
	run_test(find_free_region_with_initial_block_with_splitting);
	run_test(splitting_block_with_size_of_full_block_doesnt_split);
	run_test(successful_split_with_initial_block);
	run_test(successful_split_with_multiple_regions);
	run_test(successful_coalesce_with_2_regions);
	run_test(successful_coalesce_with_3_regions);
	run_test(successful_coalesce_with_multiple_regions);

	return 0;
}
